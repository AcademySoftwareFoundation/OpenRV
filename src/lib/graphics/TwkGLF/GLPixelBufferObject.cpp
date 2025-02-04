///*****************************************************************************/
// Copyright (c) 2019 Autodesk, Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//*****************************************************************************/

//==============================================================================
// EXTERNAL DECLARATIONS
//==============================================================================

#include <TwkGLF/GLPixelBufferObject.h>

#include <TwkUtil/EnvVar.h>
#include <TwkUtil/sgcHop.h>
#include <TwkUtil/sgcHopTools.h>
#include <TwkUtil/SystemInfo.h>

#include <errno.h>
#include <cstring>

//==============================================================================
namespace TwkGLF
{

    static ENVVAR_BOOL(evDebug, "RV_GL_BUFFER_OBJECT_DEBUG", false);

    const char MEM_DEV_PBO_TO_GPU[] = "PixelBufferObjectToGPU";
    const char MEM_DEV_PBO_FROM_GPU[] = "PixelBufferObjectFromGPU";

    static int countBOs(int inc)
    {
        static int count(0);
        return count += inc;
    }

    //==============================================================================
    class BufferObject
    {
    public:
        BufferObject(GLenum target, GLenum usage, unsigned int num_bytes,
                     unsigned int alignment);
        ~BufferObject();

        void bind(); // will create the buffer object if it is not created yet
        void unbind();

        void* map(GLenum access = GL_READ_WRITE);
        void unmap();

        unsigned int getSize() const { return _numBytes; }

        void* getMappedPtr() { return _mappedPtr; }

        unsigned int getOffset() { return _alignmentOffset; }

        void copyBufferData(void* data, unsigned size);

        GLuint getId() const { return _id; }

        GLenum getTarget() const { return _target; }

        GLenum getUsage() const { return _usage; }

        // the previous mapped data will not be affected by the resize until the
        // next map()
        void resize(unsigned int num_bytes);
        void release();

    private:
        GLuint _id;
        GLenum _target;
        GLenum _usage;

        unsigned int _numBytes;
        unsigned int _alignment;
        unsigned int _alignmentOffset;
        void* _mappedPtr;
    };

    //------------------------------------------------------------------------------
    BufferObject::BufferObject(GLenum target, GLenum usage,
                               unsigned int num_bytes, unsigned int alignment)
        : _id(0)
        , _target(target)
        , _usage(usage)
        , _numBytes(num_bytes)
        , _alignment(alignment)
        , _alignmentOffset(0)
        , _mappedPtr(NULL)
    {
        TWK_GLDEBUG;
    }

    //------------------------------------------------------------------------------
    BufferObject::~BufferObject()
    {
        if (_id > 0)
        {
            RV_LOG("~BufferObject (#%d) -- _id = %d, size = %u kB alignment = "
                   "%u\n",
                   countBOs(-1) + 1, _id, _numBytes / 1024, _alignment);

            HOP_PROF_FUNC();
            glDeleteBuffers(1, &_id);
            TWK_GLDEBUG;
        }
    }

    //------------------------------------------------------------------------------
    void BufferObject::bind()
    {
        if (_id == 0)
        {
            // the 1st bind of an unintilaized buffer object will intialize it
            {
                HOP_PROF("BufferObject::bind new");

                glGenBuffers(1, &_id);
                TWK_GLDEBUG;
                glBindBuffer(_target, _id);
                TWK_GLDEBUG;
                RV_ASSERT_INTERNAL(_id != 0);

                glBufferData(_target, _numBytes + _alignment, 0, _usage);
                TWK_GLDEBUG;
            }

            RV_LOG("BufferObject::bind new (#%d) -- _id = %d, size = %u kB "
                   "alignment = %u\n",
                   countBOs(1), _id, _numBytes / 1024, _alignment);
        }
        else
        {
            RV_LOG("BufferObject::bind (#%d) -- _id = %d, size = %u kB "
                   "alignment = %u\n",
                   countBOs(0), _id, _numBytes / 1024, _alignment);

            HOP_PROF("BufferObject::bind existing");

            glBindBuffer(_target, _id);
            TWK_GLDEBUG;
        }
    }

    //------------------------------------------------------------------------------
    void BufferObject::unbind()
    {
        HOP_PROF_FUNC();

        glBindBuffer(_target, 0);

        RV_LOG(
            "BufferObject::unbind -- _id = %d, size = %u kB alignment = %u\n",
            _id, _numBytes / 1024, _alignment);
    }

    //------------------------------------------------------------------------------
    void* BufferObject::map(GLenum access)
    {
        if (!_mappedPtr)
        {
            {
                HOP_PROF_FUNC();

                bind();
                _mappedPtr = glMapBuffer(_target, access);
                TWK_GLDEBUG;
                if (_alignment > 1)
                {
                    ptrdiff_t ptr = reinterpret_cast<ptrdiff_t>(_mappedPtr);
                    unsigned int offset = (ptr % _alignment);
                    _alignmentOffset = (offset > 0) ? (_alignment - offset) : 0;
                    _mappedPtr =
                        reinterpret_cast<void*>(ptr + _alignmentOffset);
                }
                unbind();
            }

            RV_LOG("BufferObject::map -- _id=%d, _mappedPtr=%p "
                   "_alignmentOffset=%u\n",
                   _id, _mappedPtr, _alignmentOffset);
        }
        return _mappedPtr;
    }

    //------------------------------------------------------------------------------
    void BufferObject::unmap()
    {
        if (_mappedPtr)
        {
            RV_LOG("BufferObject::unmap -- _id=%d, _mappedPtr=%p "
                   "_alignmentOffset=%u\n",
                   _id, _mappedPtr, _alignmentOffset);

            HOP_PROF_FUNC();

            bind();
            glUnmapBuffer(_target);
            TWK_GLDEBUG;
            unbind();

            _mappedPtr = NULL;
        }
    }

    //------------------------------------------------------------------------------
    void BufferObject::copyBufferData(void* data, unsigned size)
    {
        RV_ASSERT_INTERNAL(_mappedPtr == NULL);

        HOP_PROF_FUNC();

        bind();
        // We don't want to read more than the output buffer, nor do we want to
        // read more than the actual gl buffer contains otherwise
        // GL_INVALID_VALUE occurs
        const unsigned readBackSize = std::min(size, _numBytes);
        glGetBufferSubData(_target, _alignmentOffset, readBackSize, data);
        unbind();
    }

    //------------------------------------------------------------------------------
    void BufferObject::resize(unsigned int num_bytes)
    {
        RV_ASSERT_INTERNAL(_mappedPtr == NULL);

        if (num_bytes != _numBytes)
        {
            HOP_PROF_FUNC();

            _numBytes = num_bytes;

            bind();
            glBufferData(_target, _numBytes + _alignment, 0, _usage);
            unbind();
        }
    }

    //------------------------------------------------------------------------------
    void BufferObject::release()
    {
        if (_id)
        {
            HOP_PROF_FUNC();

            glDeleteBuffers(1, &_id);
            TWK_GLDEBUG;
            _id = 0;
        }
        _mappedPtr = NULL;
        _alignmentOffset = 0;
    }

    //==============================================================================
    GLPixelBufferObject::GLPixelBufferObject(GLPixelBufferObject::PackDir d,
                                             unsigned int num_bytes,
                                             unsigned int alignment)
    {
        RV_ASSERT_INTERNAL(d != FROM_GPU || alignment <= 1);

        _bufferObject = new BufferObject(
            d == TO_GPU ? GL_PIXEL_UNPACK_BUFFER : GL_PIXEL_PACK_BUFFER,
            d == TO_GPU ? GL_STREAM_DRAW : GL_STREAM_READ, num_bytes,
            alignment);
    }

    //------------------------------------------------------------------------------
    GLPixelBufferObject::~GLPixelBufferObject() { delete _bufferObject; }

    //------------------------------------------------------------------------------
    void GLPixelBufferObject::bind() { _bufferObject->bind(); }

    //------------------------------------------------------------------------------
    void GLPixelBufferObject::unbind() { _bufferObject->unbind(); }

    //------------------------------------------------------------------------------
    void* GLPixelBufferObject::map()
    {
        void* res = NULL;
        if (_bufferObject->getTarget() == GL_PIXEL_UNPACK_BUFFER)
        {
            res = _bufferObject->map(GL_WRITE_ONLY);
        }
        else
        {
            res = _bufferObject->map(GL_READ_ONLY);
        }
        return res;
    }

    //------------------------------------------------------------------------------
    void GLPixelBufferObject::unmap() { _bufferObject->unmap(); }

    //------------------------------------------------------------------------------
    void GLPixelBufferObject::resize(unsigned int num_bytes)
    {
        _bufferObject->resize(num_bytes);
    }

    //------------------------------------------------------------------------------
    void GLPixelBufferObject::release() { _bufferObject->release(); }

    //------------------------------------------------------------------------------
    unsigned int GLPixelBufferObject::getSize() const
    {
        return _bufferObject->getSize();
    }

    //------------------------------------------------------------------------------
    void* GLPixelBufferObject::getMappedPtr()
    {
        return _bufferObject->getMappedPtr();
    }

    //------------------------------------------------------------------------------
    unsigned int GLPixelBufferObject::getOffset() const
    {
        return _bufferObject->getOffset();
    }

    //------------------------------------------------------------------------------
    void GLPixelBufferObject::copyBufferData(void* data, unsigned size)
    {
        return _bufferObject->copyBufferData(data, size);
    }

    //------------------------------------------------------------------------------
    bool GLPixelBufferObject::isValid() const
    {
        return _bufferObject->getId() != 0;
    }

    //------------------------------------------------------------------------------
    GLuint GLPixelBufferObject::getId() const { return _bufferObject->getId(); }

    //------------------------------------------------------------------------------
    GLPixelBufferObject::PackDir GLPixelBufferObject::getPackDir() const
    {
        return _bufferObject->getTarget() == GL_PIXEL_UNPACK_BUFFER ? TO_GPU
                                                                    : FROM_GPU;
    }

} // namespace TwkGLF
