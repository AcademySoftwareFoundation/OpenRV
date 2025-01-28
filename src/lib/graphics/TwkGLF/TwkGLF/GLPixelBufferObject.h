///*****************************************************************************/
// Copyright (c) 2019 Autodesk, Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//*****************************************************************************/

#ifndef __TwkGLF__GLPixelBufferObject__h__
#define __TwkGLF__GLPixelBufferObject__h__

#include <TwkGLF/GL.h>

namespace TwkGLF
{

    //==============================================================================
    class GLPixelBufferObject
    {
    public:
        enum PackDir
        {
            TO_GPU,
            FROM_GPU
        };

    public:
        GLPixelBufferObject(PackDir dir, unsigned int num_bytes,
                            unsigned int alignment = 1);
        virtual ~GLPixelBufferObject();

        void bind();
        void unbind();

        void* map();
        void unmap();

        void resize(unsigned int num_bytes);
        void release();

        unsigned int getSize() const;
        void* getMappedPtr();
        unsigned int getOffset() const;

        void copyBufferData(void* data, unsigned size);

        bool isValid() const;
        GLuint getId() const;

        PackDir getPackDir() const;

    private:
        class BufferObject* _bufferObject;
    };

    extern const char MEM_DEV_PBO_TO_GPU[];
    extern const char MEM_DEV_PBO_FROM_GPU[];

} // namespace TwkGLF

#endif // __TwkGLF__GLPixelBufferObject__h__
