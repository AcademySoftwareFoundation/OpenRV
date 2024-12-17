//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <TwkGLF/GLFBO.h>
#include <TwkGLF/GLVideoDevice.h>
#include <TwkExc/Exception.h>
#include <TwkUtil/sgcHop.h>
#include <TwkUtil/sgcHopTools.h>

namespace TwkGLF
{
    using namespace std;

    GLFBO::GLFBO(size_t width, size_t height, GLenum colorFormat,
                 size_t numSamples, void* data)
        : m_width(width)
        , m_height(height)
        , m_samples(numSamples == 0 ? 1 : numSamples)
        , m_colorFormat(colorFormat)
        , m_data(data)
        , m_device(0)
        , m_colorCount(0)
        , m_state(Renderable)
        , m_fence(0)
        , m_pbo(0)
        , m_totalSizeInBytes(0)
        , m_mappedBuffer(0)
    {
        glGenFramebuffersEXT(1, &m_id);
        TWK_GLDEBUG;
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_id);
        TWK_GLDEBUG;
    }

    GLFBO::GLFBO(const GLVideoDevice* d)
        : m_id(0)
        , m_width(0)
        , m_height(0)
        , m_samples(0)
        , m_colorFormat(0)
        , m_data(0)
        , m_device(d)
        , m_colorCount(0)
        , m_state(Renderable)
        , m_fence(0)
        , m_pbo(0)
        , m_totalSizeInBytes(0)
        , m_mappedBuffer(0)
    {
        const TwkApp::VideoDevice::DataFormat& df =
            d->dataFormatAtIndex(d->currentDataFormat());
        m_colorFormat = TwkGLF::internalFormatFromDataFormat(df.iformat);
        m_defaultTarget =
            d->capabilities() & TwkApp::VideoDevice::NormalizedCoordinates
                ? GL_TEXTURE_2D
                : GL_TEXTURE_RECTANGLE_ARB;

        TwkGLF::GLenumPair tformat =
            TwkGLF::textureFormatFromDataFormat(df.iformat);
        m_defaultTextureFormat = tformat.first;
        m_defaultType = tformat.second;
    }

    GLFBO::~GLFBO()
    {
        if (m_id)
        {
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
            glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
            glDeleteFramebuffersEXT(1, &m_id);

            for (size_t i = 0; i < m_attachments.size(); i++)
            {
                const Attachment& a = m_attachments[i];

                if (a.owner)
                {
                    if (a.texture)
                    {
                        glDeleteTextures(1, &a.id);
                    }
                    else
                    {
                        glDeleteRenderbuffersEXT(1, &a.id);
                    }
                }
            }
        }

        if (m_pbo)
        {
            if (m_fence)
                m_fence->wait();
            delete m_fence;
            glDeleteBuffers(1, &m_pbo);
            TWK_GLDEBUG;
        }
    }

    std::string GLFBO::identifier() const
    {
        ostringstream o;
        o << "fbo" << m_id;
        return o.str();
    }

    size_t GLFBO::width() const
    {
        if (m_device)
            return m_device->width();
        return m_width;
    }

    size_t GLFBO::height() const
    {
        if (m_device)
            return m_device->height();
        return m_height;
    }

    const GLFBO::Attachment* GLFBO::colorAttachment(size_t i) const
    {
        try
        {
            for (size_t q = 0, count = 0; q < m_attachments.size(); q++)
            {
                const Attachment& a = m_attachments[q];
                GLuint apoint = a.attachPoint;

                if (apoint >= GL_COLOR_ATTACHMENT0_EXT
                    && apoint < GL_COLOR_ATTACHMENT0_EXT + m_colorCount)
                {
                    if (count == i)
                        return &a;
                    count++;
                }
            }
        }

        catch (std::exception& exc)
        {
            throw;
        }

        return 0;
    }

    const GLFBO::Attachment* GLFBO::colorTextureAttachment(size_t i) const
    {
        try
        {
            for (size_t q = 0, count = 0; q < m_attachments.size(); q++)
            {
                const Attachment& a = m_attachments[q];
                bool texture = a.texture;

                if (texture)
                {
                    if (count == i)
                        return &a;
                    count++;
                }
            }
        }

        catch (std::exception& exc)
        {
            throw;
        }

        return 0;
    }

    GLuint GLFBO::colorID(size_t i) const
    {
        const Attachment* attach = colorAttachment(i);
        assert(attach);
        return attach->id;
    }

    GLuint GLFBO::colorTarget(size_t i) const
    {
        const Attachment* attach = colorAttachment(i);
        assert(attach);
        return attach->target;
    }

    // there are two types of GLFBO
    // one is a opengl default framebuffer, which does not have any attachments
    // but do have target, format, type etc.
    // one is a framebuffer object we create, which has at least 1 attachment
    // for opengl default ones, we return the default info
    // for our fbos, we return always the first attachment's info
    GLuint GLFBO::primaryColorTarget() const
    {
        if (m_attachments.empty())
            return m_defaultTarget;

        const Attachment* attach = colorAttachment(0);
        assert(attach);
        return attach->target;
    }

    GLuint GLFBO::primaryColorType() const
    {
        if (m_attachments.empty())
            return m_defaultTarget;

        const Attachment* attach = colorAttachment(0);
        assert(attach);
        return attach->type;
    }

    GLenum GLFBO::primaryColorFormat() const { return m_colorFormat; }

    bool GLFBO::isColorTexture(size_t i) const
    {
        const Attachment* attach = colorAttachment(i);
        assert(attach);
        return attach->texture;
    }

    GLFBO::Attachment GLFBO::attachColorRenderBuffer(GLuint id)
    {
        bind();

        glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
                                     GL_COLOR_ATTACHMENT0_EXT + m_colorCount,
                                     GL_RENDERBUFFER_EXT, id);

        m_attachments.push_back(
            Attachment(id, GL_COLOR_ATTACHMENT0_EXT + m_colorCount,
                       GL_UNSIGNED_BYTE, false, false, false, false));
        m_colorCount++;

        // avoid checking gl status unless in debug mode
#ifdef NDEBUG
        check();
#endif

        return m_attachments.back();
    }

    GLFBO::Attachment GLFBO::newColorRenderBuffer()
    {
        bind();
        TWK_GLDEBUG;

        GLuint id;
        glGenRenderbuffersEXT(1, &id);
        TWK_GLDEBUG;
        glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, id);
        TWK_GLDEBUG;

        if (m_samples > 1)
        {
            glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT, m_samples,
                                                m_colorFormat, m_width,
                                                m_height);
            TWK_GLDEBUG;
        }
        else
        {
            glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, m_colorFormat,
                                     m_width, m_height);
            TWK_GLDEBUG;
        }

        glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
                                     GL_COLOR_ATTACHMENT0_EXT + m_colorCount,
                                     GL_RENDERBUFFER_EXT, id);
        TWK_GLDEBUG;

        // avoid checking gl status unless in debug mode
#ifdef NDEBUG
        check();
#endif

        m_attachments.push_back(
            Attachment(id, m_colorCount + GL_COLOR_ATTACHMENT0_EXT,
                       GL_UNSIGNED_BYTE, false));
        m_colorCount++;

        return m_attachments.back();
    }

    GLFBO::Attachment GLFBO::attachColorTexture(GLuint target, GLuint id)
    {
        bind();

        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,
                                  GL_COLOR_ATTACHMENT0_EXT + m_colorCount,
                                  target, id, 0);

        m_attachments.push_back(
            Attachment(id, GL_COLOR_ATTACHMENT0_EXT + m_colorCount, target,
                       GL_UNSIGNED_BYTE, true, false, false, false));
        m_colorCount++;

        // avoid checking gl status unless in debug mode
#ifdef NDEBUG
        check();
#endif

        return m_attachments.back();
    }

    GLFBO::Attachment GLFBO::newColorTexture(GLenum target, GLenum format,
                                             GLenum type, GLenum minFilter,
                                             GLenum magFilter, GLenum clamping)
    {
        bind();

        // format and type can determine pixelSize
        size_t pixelSize = TwkGLF::pixelSizeFromTextureFormat(format, type);
        size_t totalSize = m_width * m_height * pixelSize;
        m_totalSizeInBytes += totalSize;

        GLuint id;
        glGenTextures(1, &id);
        glBindTexture(target, id);
        glTexParameterf(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_PRIORITY, 1.0f);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER,
                        minFilter);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER,
                        magFilter);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, clamping);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, clamping);

        // rectangle textures can't have mipmaps
        // glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_GENERATE_MIPMAP,
        // GL_TRUE); // automatic mipmap

        glTexImage2D(target, 0, m_colorFormat, m_width, m_height, 0, format,
                     type, 0);

        TWK_GLDEBUG;

        glBindTexture(target, 0);

        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,
                                  GL_COLOR_ATTACHMENT0_EXT + m_colorCount,
                                  target, id, 0);

        m_attachments.push_back(Attachment(
            id, GL_COLOR_ATTACHMENT0_EXT + m_colorCount, target, type, true));
        m_colorCount++;

        // avoid checking gl status unless in debug mode
#ifdef NDEBUG
        check();
#endif

        return m_attachments.back();
    }

    GLFBO::Attachment GLFBO::newPackedDepthStencilBuffer()
    {
        bind();

        GLuint id;
        glGenRenderbuffersEXT(1, &id);
        glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, id);

        if (m_samples > 1)
        {
            glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER_EXT, m_samples,
                                                GL_DEPTH24_STENCIL8_EXT,
                                                m_width, m_height);
        }
        else
        {
            glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,
                                     GL_DEPTH24_STENCIL8_EXT, m_width,
                                     m_height);
            TWK_GLDEBUG;
        }

        glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
                                     GL_STENCIL_ATTACHMENT_EXT,
                                     GL_RENDERBUFFER_EXT, id);
        TWK_GLDEBUG;

        glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
                                     GL_DEPTH_ATTACHMENT_EXT,
                                     GL_RENDERBUFFER_EXT, id);
        TWK_GLDEBUG;

        glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
        TWK_GLDEBUG;
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
        TWK_GLDEBUG;

        m_attachments.push_back(Attachment(id, GL_STENCIL_ATTACHMENT_EXT, 0,
                                           GL_UNSIGNED_BYTE, false, true, true,
                                           true));
        TWK_GLDEBUG;
        // avoid checking gl status unless in debug mode
#ifdef NDEBUG
        check();
#endif

        return m_attachments.back();
    }

    GLFBO::Attachment GLFBO::attachPackedDepthStencilBuffer(GLuint id)
    {
        bind();

        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT,
                                  GL_RENDERBUFFER_EXT, id, 0);

        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
                                  GL_RENDERBUFFER_EXT, id, 0);

        m_attachments.push_back(Attachment(id, GL_STENCIL_ATTACHMENT_EXT, 0,
                                           GL_UNSIGNED_BYTE, false, true, true,
                                           false));
        // avoid checking gl status unless in debug mode
#ifdef NDEBUG
        check();
#endif

        return m_attachments.back();
    }

    void GLFBO::bind(GLenum kind) const
    {
        waitForExternalReadback();
        glBindFramebufferEXT(kind, m_id);
        TWK_GLDEBUG;
    }

    void GLFBO::unbind(GLenum kind) const
    {
        glBindFramebufferEXT(kind, 0);
        TWK_GLDEBUG;
    }

    void GLFBO::check() const
    {
        //
        //  GLFBO must be bound for glCheckFramebufferStatus()
        //

        bind();

        GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);

        if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
        {
            //
            //  NOTE: This is not an error so gluErrorString() will not produce
            //        a message
            //

            TWK_THROW_EXC_STREAM(
                "ERROR: OpenGL: frame buffer incomplete: status = " << status);
        }
    }

    void GLFBO::bindColorTexture(size_t i) const
    {
        assert(i < m_attachments.size());
        const Attachment* attach = colorTextureAttachment(i);
        assert(attach);
        glBindTexture(colorTarget(i), attach->id);
    }

    void GLFBO::unbindColorTexture() const
    {
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
    }

    void GLFBO::copyTo(const GLFBO* destinationGLFBO, GLenum mask,
                       GLenum filter) const
    {
        HOP_ZONE(HOP_ZONE_COLOR_6);
        HOP_CALL(glFinish();)
        HOP_PROF_FUNC();

        //
        //  NOTE: upper bounds on blit is *exclusive* not inclusive:
        //
        //  "The lower bounds of the rectangle are inclusive, while
        //   the upper bounds are exclusive."
        //

        const GLint srcX0 = 0;
        const GLint srcY0 = 0;
        const GLint srcX1 = width();
        const GLint srcY1 = height();

        const GLint dstX0 = 0;
        const GLint dstY0 = 0;
        const GLint dstX1 = destinationGLFBO->width();
        const GLint dstY1 = destinationGLFBO->height();

        bind(GL_READ_FRAMEBUFFER_EXT);
        destinationGLFBO->bind(GL_DRAW_FRAMEBUFFER_EXT);

        glBlitFramebufferEXT(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1,
                             dstY1, mask, filter);

        HOP_CALL(glFinish();)
    }

    void GLFBO::copyRegionTo(const GLFBO* destinationGLFBO, float srcX,
                             float srcY, float srcW, float srcH, float dstX,
                             float dstY, float dstW, float dstH, GLenum mask,
                             GLenum filter) const
    {
        const float sw = float(width());
        const float sh = float(height());
        const float dw = float(destinationGLFBO->width());
        const float dh = float(destinationGLFBO->height());

        const GLint srcX0 = srcX * sw;
        const GLint srcY0 = srcY * sh;
        const GLint srcX1 = srcX0 + srcW * sw;
        const GLint srcY1 = srcY0 + srcH * sh;

        const GLint dstX0 = dstX * dw;
        const GLint dstY0 = dstY * dh;
        const GLint dstX1 = dstX0 + dstW * dw;
        const GLint dstY1 = dstY0 + dstH * dh;

        bind(GL_READ_FRAMEBUFFER_EXT);
        destinationGLFBO->bind(GL_DRAW_FRAMEBUFFER_EXT);

        glBlitFramebufferEXT(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1,
                             dstY1, mask, filter);
    }

    namespace
    {

        size_t pixelSizeOfInternalFormat(GLenum format)
        {
            size_t n = 0;

            switch (format)
            {
            case GL_RGB8:
                n = sizeof(GLbyte) * 3;
                break;
            case GL_RGBA8:
                n = sizeof(GLbyte) * 4;
                break;
            case GL_RGB16:
                n = sizeof(GLshort) * 3;
                break;
            case GL_RGBA16:
                n = sizeof(GLshort) * 4;
                break;
            case GL_RGB10_A2:
                n = sizeof(GLuint);
                break;
            case GL_RGB16F_ARB:
                n = sizeof(unsigned short) * 3;
                break;
            case GL_RGBA16F_ARB:
                n = sizeof(unsigned short) * 4;
                break;
            case GL_RGB32F_ARB:
                n = sizeof(GLfloat) * 3;
                break;
            case GL_RGBA32F_ARB:
                n = sizeof(GLfloat) * 4;
                break;
            }

            return n;
        }

#ifdef PLATFORM_DARWIN
        typedef std::pair<GLenum, GLenum> GLenumPair;
#endif

        GLenumPair dataTypeOfColorFormat(GLenum format)
        {
            switch (format)
            {
            case GL_RGB8:
                return GLenumPair(GL_RGB, GL_UNSIGNED_BYTE);
            default:
            case GL_RGBA8:
                return GLenumPair(GL_RGBA, GL_UNSIGNED_BYTE);
            case GL_RGB16:
                return GLenumPair(GL_RGB, GL_UNSIGNED_SHORT);
            case GL_RGBA16:
                return GLenumPair(GL_RGBA, GL_UNSIGNED_SHORT);
            case GL_RGB10_A2:
                return GLenumPair(GL_RGBA, GL_UNSIGNED_INT_10_10_10_2);
            case GL_RGB16F_ARB:
                return GLenumPair(GL_RGB, GL_HALF_FLOAT_ARB);
            case GL_RGBA16F_ARB:
                return GLenumPair(GL_RGBA, GL_HALF_FLOAT_ARB);
            case GL_RGB32F_ARB:
                return GLenumPair(GL_RGB, GL_FLOAT);
            case GL_RGBA32F_ARB:
                return GLenumPair(GL_RGBA, GL_FLOAT);
            }
        }

    } // namespace

    void GLFBO::initReadBack(GLenum hint)
    {
        if (m_pbo)
            return;
        size_t totalSizeInBytes =
            m_width * m_height * pixelSizeOfInternalFormat(m_colorFormat);
        glGenBuffers(1, &m_pbo);
        TWK_GLDEBUG;
        glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, m_pbo);
        TWK_GLDEBUG;
        glBufferData(GL_PIXEL_PACK_BUFFER_ARB, totalSizeInBytes, NULL, hint);
        TWK_GLDEBUG;
        glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0);
        TWK_GLDEBUG;
    }

    void GLFBO::insertFence() const
    {
        assert(m_fence == NULL);
        m_fence = new GLFence();
        m_fence->set();
        m_state = FenceInserted;
    }

    void GLFBO::waitForFence(bool client) const
    {
        assert(m_fence);
        m_fence->wait(client);
        m_state = FenceWait;
        delete m_fence;
        m_fence = NULL;
    }

    void GLFBO::beginExternalReadback() const
    {
        ScopedLock lock(m_mutex);
        m_state = ExternalReading;
    }

    void GLFBO::endExternalReadback() const
    {
        ScopedLock lock(m_mutex);
        m_state = Renderable;
        readCond().notify_all();
    }

    void GLFBO::waitForExternalReadback() const
    {
        ScopedLock lock(m_mutex);
        while (m_state == ExternalReading)
            readCond().wait(lock);
    }

    void GLFBO::beginAsyncReadBack() const
    {
        assert(m_pbo);
        GLenumPair p = dataTypeOfColorFormat(m_colorFormat);
        glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, m_pbo);
        TWK_GLDEBUG;
        glReadPixels(0, 0, m_width, m_height, p.first, p.second, NULL);
        TWK_GLDEBUG;
        m_state = Reading;
    }

    void* GLFBO::mapBuffer() const
    {
        glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, m_pbo);
        TWK_GLDEBUG;
        m_mappedBuffer =
            glMapBuffer(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY_ARB);
        TWK_GLDEBUG;
        m_state = Mapped;

        if (!m_mappedBuffer)
        {
            unmapBuffer();
            cout << "ERROR: GLFBO::mapBuffer: failed to map PBO" << endl;
        }

        return m_mappedBuffer;
    }

    void GLFBO::unmapBuffer() const
    {
        glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, m_pbo);
        TWK_GLDEBUG;
        glUnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB);
        TWK_GLDEBUG;
        m_state = Renderable;
        m_mappedBuffer = NULL;
    }

    GLFBO::State GLFBO::state() const
    {
        ScopedLock lock(m_mutex);
        return m_state;
    }

} // namespace TwkGLF
