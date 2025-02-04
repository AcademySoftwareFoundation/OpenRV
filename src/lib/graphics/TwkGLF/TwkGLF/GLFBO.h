//******************************************************************************
// Copyright (c) 2010 Tweak Software Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkGLF__GLFBO__h__
#define __TwkGLF__GLFBO__h__
#include <TwkExc/TwkExcException.h>
#include <TwkGLF/GL.h>
#include <TwkGLF/GLFence.h>
#include <vector>
#include <boost/thread.hpp>

#ifdef PLATFORM_DARWIN
//
//  Somebody (system header) is defining "check" and leaving it defined --
//  brilliant
//
#ifdef check
#undef check
#endif
#endif

namespace TwkGLF
{

    class GLVideoDevice;

    //
    //  Frame Buffer Object. This attempts to enforce some of the
    //  constraints on the FBO by reducing the number of degrees of
    //  freedom usually available when creating them.
    //
    //  Don't confuse this with TwkGLF::FrameBuffer. Its deprecated.  This
    //  class is managing the underlying GL FBO object not a window system
    //  object.
    //
    //  To further confuse: there can be an FBO (instance of this class)
    //  which represents the default FBO of the
    //  TwkGLF::GLVideoDevice. That object has no understanding of the GL
    //  context its just a way to represent the default window system FBO
    //  which GL explicitly refers to a id number 0.
    //
    //  Each GLFBO object keeps track of how much card memory it uses in
    //  m_totalSizeInBytes right now we only track colorTexture
    //

    class GLFBO
    {
    public:
        typedef boost::mutex::scoped_lock ScopedLock;

        //
        //  Attachment is either a RenderBuffer or a Texture. You can
        //  attach an externally created texture IFF the internal format
        //  and geometry match those passed into the FBO
        //  constructor. Otherwise you get an "incomplete" FBO in GL
        //  terminology.
        //
        //  You don't create one of these yourself, its used internally in
        //  the class.
        //

        struct Attachment
        {
            Attachment(GLuint _id = 0, GLuint _attachPoint = 0,
                       GLenum _target = GL_TEXTURE_RECTANGLE_ARB,
                       GLenum _type = 0, bool _istexture = false,
                       bool _isstencil = false, bool _isdepth = false,
                       bool _owner = true)
                : id(_id)
                , attachPoint(_attachPoint)
                , type(_type)
                , texture(_istexture)
                , stencil(_isstencil)
                , depth(_isdepth)
                , owner(_owner)
                , target(_target)
            {
            }

            GLuint id;
            GLuint attachPoint;
            GLuint target; // rect or 2D
            GLenum type;
            bool texture;
            bool stencil;
            bool depth;
            bool owner;
        };

        typedef std::vector<Attachment> AttachmentVector;

        enum State
        {
            Renderable,
            FenceInserted,
            FenceWait,
            Reading,
            Mapped,
            ExternalReading
        };

        //
        //  FBOs have a single size (w x h) and in the case of the color
        //  attachments a single internal format. Likewise, a single
        //  multisample value is allowed for the entire FBO. So all of these
        //  parameters are passed-in up front and cannot be changed.
        //
        //  colorFormat is something like GL_RGBA8, GL_RGB8, GL_RGBA16F_ARB,
        //  GL_RGB16F_ARB, etc. Its the internalFormat parameter of
        //  glTexImage2D and for creating RenderBuffers.
        //
        //  multiSampleSize determines the number of samples for all images. 0
        //  means 1 or no multisampling.
        //
        //  Using the constructor that takes a TwkGLF::GLVideoDevice
        //  creates a "default FBO" which is meant to represent the
        //  default window system FBO (id == 0). Deleting a default FBO
        //  does not result in deletion of the underly VideoDevice
        //
        //  The data is blind data not used by FBO
        //

        GLFBO(size_t width, size_t height,
              GLenum colorFormat, // internal format of color attachment point
              size_t multiSampleSize = 0, // 0 == 1
              void* data = 0);

        GLFBO(const GLVideoDevice*);

        ~GLFBO();

        size_t multiSampleSize() const { return m_samples; }

        GLenum primaryColorFormat() const;
        size_t width() const;
        size_t height() const;

        template <class T> T* data() const
        {
            return reinterpret_cast<T*>(m_data);
        }

        void setData(void* d) { m_data = d; }

        bool isDefaultFBO() const { return m_device != 0; }

        State state() const;

        const AttachmentVector& attachments() const { return m_attachments; }

        bool hasColorAttachment() const { return !m_attachments.empty(); }

        const Attachment* colorAttachment(size_t i) const;
        const Attachment* colorTextureAttachment(size_t i) const;
        GLuint colorID(size_t i) const;
        bool isColorTexture(size_t i) const;
        GLuint colorTarget(size_t i) const;
        GLuint primaryColorTarget() const;
        GLuint primaryColorType() const;

        GLuint fboID() const { return m_id; }

        //
        //  Color Attachments. You can create one here which can be referred to
        //  by id or you can attach an existing one (assuming its internal
        //  format matches what you passed into this FBO object). The
        //  Attachment id is the render buffer id. The render buffer is owned
        //  by the FBO and will be destroyed when it is destroyed.
        //

        Attachment newColorRenderBuffer();

        //
        //  Color texture requires format (e.g. GL_RGBA) and type
        //  (GL_UNSIGNED_BYTE) for the call to glTexImage2D. The format
        //  must be compatible with the colorFormat (internal format)
        //  specified in the FBO cons tructor. The Attachment id field is
        //  the texture id. The FBO owns the texture and will destroy it
        //  when it is destroyed.
        //

        Attachment newColorTexture(GLenum target, // texture target
                                   GLenum format, // internal format
                                   GLenum type,   //
                                   GLenum minFilter = GL_LINEAR,
                                   GLenum magFilter = GL_LINEAR,
                                   GLenum clamping = GL_CLAMP_TO_EDGE);

        //
        //  Attach existing texture as color attachment. The internalFormat has
        //  to match the one passed into the FBO constructor -- otherwise the
        //  FBO will not be complete and this will throw. The FBO does not own
        //  the passed in texture so you need handle destroying it.
        //
        //  Target is a tecture target: GL_TEXTURE_RECTANGLE_ARB or
        //  GL_TEXTURE_2D. This needs to match the target that was used to
        //  define the texture originally.
        //

        Attachment attachColorTexture(GLuint target, GLuint id);

        //
        //  Attach existing renderbuffer. The internalFormat has to match
        //  the one passed into the FBO constructor -- otherwise the FBO
        //  will not be complete and this will throw. The FBO does not own
        //  the passed in renderbuffer so you need handle destroying it.
        //

        Attachment attachColorRenderBuffer(GLuint);

        //
        //  Stencil buffer attachment
        //

        Attachment newPackedDepthStencilBuffer();

        //
        //  Attaches DEPTH+STENCIL from other FBO
        //

        Attachment attachPackedDepthStencilBuffer(GLuint);

        //
        //  FBO operations bind and unbind accept: GL_FRAMEBUFFER_EXT,
        //  GL_DRAW_FRAMEBUFFER_EXT, GL_READ_FRAMEBUFFER_EXT
        //

        void bind(GLenum kind = GL_FRAMEBUFFER_EXT) const;
        void unbind(GLenum kind = GL_FRAMEBUFFER_EXT) const;

        //
        //  Binding the texture means you're going to use it as a texture
        //  (not as a render target). So typically you'd rendering into it
        //  and then bind it and use it to rendering into something else
        //  for multi-pass rendering.
        //

        void bindColorTexture(size_t) const;
        void unbindColorTexture() const;

        //
        //  Checks the completeness. throws if its not. Called by the above
        //  functions to ensure completeness.
        //

        void check() const;

        //
        //  Copy uses glBlitFramebuffer to do the work. The entire image
        //  is copyed from the window of one to the other (so if aspect
        //  ratios differ the image will be stretched/squashed).
        //
        //  The read and write buffers will be set
        //

        void copyTo(const GLFBO* destinationFBO,
                    GLenum mask = GL_COLOR_BUFFER_BIT,
                    GLenum filter = GL_NEAREST) const;

        //
        //  src and dst coordinates are in NDC coords
        //

        void copyRegionTo(const GLFBO* destinationFBO, float srcX, float srcY,
                          float srcW, float srcH, float dstX, float dstY,
                          float dstW, float dstH,
                          GLenum mask = GL_COLOR_BUFFER_BIT,
                          GLenum filter = GL_NEAREST) const;

        //
        //  For reading/processing back to host memory possibly with
        //  multiple threads. The mutex and condition variable are not
        //  used by GLFBO but can be used by a transfer thread and a GL
        //  thread if desired. In that case the transfer thread can
        //

        boost::mutex& mutex() const { return m_mutex; }

        boost::condition_variable& readCond() const { return m_readCond; }

        void initReadBack(GLenum hint = GL_STATIC_READ);

        void insertFence() const; // after rendering commands are complete
        void waitForFence(bool client = true) const; // waits on inserted fence

        void beginExternalReadback() const;
        void endExternalReadback() const;
        void waitForExternalReadback() const;

        void beginAsyncReadBack() const;
        void* mapBuffer() const;
        void unmapBuffer() const;

        void* mappedBuffer() const { return m_mappedBuffer; }

        size_t totalSizeInBytes() const { return m_totalSizeInBytes; }

        std::string identifier() const;

    protected:
        GLFBO() {}

    private:
        GLuint m_id;
        std::string m_idstr;

        // these 3 defaults are the properties of GL default framebuffers (as
        // opposed to FBOs)
        GLuint m_defaultTarget;        // example: GL_TEXTURE_2D
        GLenum m_defaultType;          // example: GL_UNSIGNED_BYTE
        GLenum m_defaultTextureFormat; // example: GL_RGBA

        GLenum m_colorFormat; // example: GL_RGB8
        size_t m_width;
        size_t m_height;
        size_t m_samples;
        AttachmentVector m_attachments;
        size_t m_colorCount;
        void* m_data;
        const GLVideoDevice* m_device;

        mutable boost::mutex m_mutex;
        mutable boost::condition_variable m_readCond;
        mutable State m_state;
        mutable GLFence* m_fence;
        mutable GLuint m_pbo;
        mutable void* m_mappedBuffer;
        mutable size_t m_totalSizeInBytes;
    };

    typedef std::vector<const GLFBO*> ConstFBOVector;
    typedef std::vector<GLFBO*> FBOVector;

} // namespace TwkGLF

#endif // __TwkGLF__GLFBO__h__
