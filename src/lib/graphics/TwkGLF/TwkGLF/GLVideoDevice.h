//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __TwkGLF__GLVideoDevice__h__
#define __TwkGLF__GLVideoDevice__h__
#include <string>
#include <iostream>
#include <TwkApp/VideoDevice.h>
#include <TwkGLF/GL.h>
#include <TwkGLText/TwkGLText.h>

namespace TwkApp
{
    class VideoModule;
}

namespace TwkFB
{
    class FrameBuffer;
}

namespace TwkGLF
{
    class GLFBO;
    typedef std::pair<unsigned int, unsigned int> GLenumPair;

    //
    //  Utilitiy functions which map the VideoDevice formats to GL formats
    //

    unsigned int
        internalFormatFromDataFormat(TwkApp::VideoDevice::InternalDataFormat);
    GLenumPair
        textureFormatFromDataFormat(TwkApp::VideoDevice::InternalDataFormat);
    size_t pixelSizeFromTextureFormat(GLenum format, GLenum type);

    //
    //  GLVideoDevice
    //
    //  A TwkApp:VideoDevice which can be used with the OpenGL
    //  API. Calling makeCurrent() on this video device will set the GL
    //  context on this video device and route all GL to it.
    //
    //  NOTE: this class replaces the TwkGL::FrameBuffer
    //  class. VideoDevice is more general and provides a similar API.
    //

    class GLVideoDevice : public TwkApp::VideoDevice
    {
    public:
        GLVideoDevice(TwkApp::VideoModule*, const std::string& name,
                      unsigned int capabilities);

        ~GLVideoDevice();

        //
        //  VideoDevice API
        //

        virtual Resolution resolution() const;
        virtual Offset offset() const;
        virtual Timing timing() const;
        virtual VideoFormat format() const;

        virtual void open(const StringVector&);
        virtual void close();
        virtual bool isOpen() const;

        //
        //  GL API
        //

        virtual GLVideoDevice* newSharedContextWorkerDevice() const;

        virtual void makeCurrent() const;
        virtual void clearCaches() const;
        virtual void bind() const;
        virtual void unbind() const;

        virtual GLFBO* defaultFBO(); // GLVideoDevice owns it
        virtual const GLFBO* defaultFBO() const;

        virtual int defaultFBOIndex() const { return 0; }

        virtual void setDefaultFBOIndex(int i) {}

        //
        //  Additional Querys
        //

        virtual bool isQuadBuffer() const;
        virtual bool sharesWith(const GLVideoDevice*) const;

        //
        //  By default, just like makeCurrent, otherwise if this could be
        //  a software FB it will use the specified TwkFB::GLVideoDevice
        //  as the target
        //

        virtual void makeCurrent(TwkFB::FrameBuffer* target) const;
        virtual void redraw() const;
        virtual void redrawImmediately() const;

        void setTextContext(TwkGLText::Context, bool share = false);

        TwkGLText::Context textContext() const { return m_textContext; }

    protected:
        mutable GLFBO* m_fbo;

    private:
        bool m_textContextOwner;
        TwkGLText::Context m_textContext;
    };

    //
    //  GLBindableVideoDevice
    //
    //  This is a device which doesn't allow drawing to using GL, but does
    //  "bind" to an existing GL context in order to get pixels from one
    //  its GL_READ_FRAMEBUFFER_EXT bound FBO
    //

    class GLBindableVideoDevice : public TwkApp::VideoDevice
    {
    public:
        GLBindableVideoDevice(TwkApp::VideoModule* module,
                              const std::string& name,
                              unsigned int capabilities);

        ~GLBindableVideoDevice();

        //
        //  VideoDevice API
        //

        virtual Resolution resolution() const;
        virtual Offset offset() const;
        virtual Timing timing() const;
        virtual VideoFormat format() const;

        virtual void open(const StringVector&);
        virtual void close();
        virtual bool isOpen() const;

        //
        //  GLBindableVideoDevice API
        //

        virtual bool willBlockOnTransfer() const;

        virtual void unbind() const;
        virtual void bind(const GLVideoDevice*) const;
        virtual void transfer(const GLFBO*) const;

        virtual void bind2(const GLVideoDevice*, const GLVideoDevice*) const;
        virtual void transfer2(const GLFBO*, const GLFBO*) const;

        virtual bool readyForTransfer() const;
    };

} // namespace TwkGLF

#endif // __TwkGLF__GLVideoDevice__h__
