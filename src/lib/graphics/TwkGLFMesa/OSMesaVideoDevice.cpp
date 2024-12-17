//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkGLFMesa/OSMesaVideoDevice.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/osmesa.h>
#include <iostream>
#include <TwkGLText/TwkGLText.h>

namespace TwkGLF
{
    using namespace std;
    using namespace TwkApp;

    struct OSMesaImp
    {
        OSMesaContext context;
    };

    OSMesaVideoDevice::OSMesaVideoDevice(VideoModule* m, int w, int h,
                                         bool alpha, bool floatbuffer,
                                         bool topleft)
        : TwkGLF::GLVideoDevice(m, "mesa", VideoDevice::ImageOutput)
        , m_width(w)
        , m_height(h)
        , m_alpha(alpha)
        , m_float(floatbuffer)
        , m_topleft(topleft)
        , m_imp(0)
        , m_currentFB(0)
    {
        m_imp = new OSMesaImp();

        m_imp->context =
            OSMesaCreateContextExt(alpha ? OSMESA_RGBA : OSMESA_RGB,
                                   0,  // depth bits
                                   8,  // stencil bits
                                   0,  // accum bits
                                   0); // share context
    }

    OSMesaVideoDevice::~OSMesaVideoDevice()
    {
        OSMesaDestroyContext(m_imp->context);
        delete m_imp;
    }

    static OSMesaVideoDevice::VoidFunc GetAddressProc(const char* name)
    {
        return OSMesaGetProcAddress(name);
    }

    OSMesaVideoDevice::ProcAddressFunc OSMesaVideoDevice::mesaProcAddressFunc()
    {
        return GetAddressProc;
    }

    void OSMesaVideoDevice::resize(int w, int h, bool alpha)
    {
        m_width = w;
        m_height = h;
        m_alpha = alpha;

        OSMesaDestroyContext(m_imp->context);

        m_imp->context =
            OSMesaCreateContextExt(alpha ? OSMESA_RGBA : OSMESA_RGB,
                                   0,  // depth bits
                                   8,  // stencil bits
                                   0,  // accum bits
                                   0); // share context
    }

    size_t OSMesaVideoDevice::width() const { return m_width; }

    size_t OSMesaVideoDevice::height() const { return m_height; }

    void OSMesaVideoDevice::redraw() const {}

    void OSMesaVideoDevice::redrawImmediately() const {}

    void OSMesaVideoDevice::makeCurrent() const
    {
        // you have to use the other one
        if (m_currentFB)
        {
            makeCurrent(m_currentFB);
        }
        else
        {
            cout << "WARNING: OSMesaVideoDevice::makeCurrent requires FB"
                 << endl;
            GLVideoDevice::makeCurrent();
        }
    }

    void OSMesaVideoDevice::makeCurrent(TwkFB::FrameBuffer* fb) const
    {
        OSMesaMakeCurrent(m_imp->context, fb->pixels<void>(),
                          m_float ? GL_FLOAT : GL_UNSIGNED_BYTE, fb->width(),
                          fb->height());

        if (m_topleft)
        {
            fb->setOrientation(TwkFB::FrameBuffer::TOPLEFT);
            OSMesaPixelStore(OSMESA_Y_UP, 0);
        }

        OSMesaPixelStore(OSMESA_ROW_LENGTH, fb->width());
        OSMesaColorClamp(GL_FALSE);
        m_currentFB = fb;
        GLVideoDevice::makeCurrent();
    }

    string OSMesaVideoDevice::hardwareIdentification() const
    {
        return "osmesa";
    }

} // namespace TwkGLF
