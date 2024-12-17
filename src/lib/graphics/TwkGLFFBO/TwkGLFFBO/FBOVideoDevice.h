//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkGLF__FBOVideoDevice__h__
#define __TwkGLF__FBOVideoDevice__h__
#include <TwkGLF/GL.h>
#include <TwkGLF/GLVideoDevice.h>

namespace TwkGLF
{
    struct FBOImp;

    class FBOVideoDevice : public GLVideoDevice
    {
    public:
        //
        //  If contextFB is no NULL, the FBOVideoDevice will use its
        //  context (by calling makeCurrent()
        //

        FBOVideoDevice(TwkApp::VideoModule* m, int w, int h, bool alpha,
                       int numFBOs = 1);
        virtual ~FBOVideoDevice();

        bool hasAlpha() const { return m_alpha; }

        virtual int defaultFBOIndex() const { return m_defaultFBOIndex; }

        virtual void setDefaultFBOIndex(int i)
        {
            m_defaultFBOIndex = i;
            m_fbo = m_fbos[i];
        }

        //
        //  GLVideoDevice API
        //

        virtual std::string hardwareIdentification() const;

        virtual size_t width() const;
        virtual size_t height() const;
        virtual void makeCurrent() const;
        virtual void redraw() const;

        virtual void bind() const;
        virtual void unbind() const;

        virtual GLFBO* defaultFBO();
        virtual const GLFBO* defaultFBO() const;

        static void useSWRendererOnMac(bool v) { m_swRendererOnMac = v; }

    private:
        int m_width;
        int m_height;
        bool m_alpha;
        FBOImp* m_imp;
        std::vector<GLFBO*> m_fbos;
        int m_defaultFBOIndex;

        static bool m_swRendererOnMac;
    };

} // namespace TwkGLF

#endif // __TwkGLF__FBOVideoDevice__h__
