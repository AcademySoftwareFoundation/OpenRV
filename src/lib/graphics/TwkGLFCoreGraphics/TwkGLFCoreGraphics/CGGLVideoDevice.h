//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkGLF__CGGLVideoDevice__h__
#define __TwkGLF__CGGLVideoDevice__h__
#include <ApplicationServices/ApplicationServices.h>
#include <OpenGL/OpenGL.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkGLF/GLVideoDevice.h>

namespace TwkGLF
{

    class CGGLVideoDevice : public TwkGLF::GLVideoDevice
    {
    public:
        CGGLVideoDevice(TwkApp::VideoModule*, int w, int h, bool alpha,
                        CGLContextObj context, bool owner);

        virtual ~CGGLVideoDevice();

        virtual std::string hardwareIdentification() const;

        //
        //  VideoDevice API
        //

        virtual size_t width() const;
        virtual size_t height() const;
        virtual void makeCurrent() const;
        virtual void redraw() const;
        virtual void redrawImmediately() const;

    private:
        CGLContextObj m_context;
        int m_width;
        int m_height;
        bool m_alpha;
        bool m_owner;
    };

} // namespace TwkGLF

#endif // __TwkGLF__CGGLVideoDevice__h__
