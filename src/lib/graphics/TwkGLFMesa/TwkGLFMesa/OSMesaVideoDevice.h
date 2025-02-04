//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef __TwkGLF__OSMesaVideoDevice__h__
#define __TwkGLF__OSMesaVideoDevice__h__
#include <TwkFB/FrameBuffer.h>
#include <TwkGLF/GLVideoDevice.h>

namespace TwkGLF
{

    struct OSMesaImp;

    class OSMesaVideoDevice : public TwkGLF::GLVideoDevice
    {
    public:
        OSMesaVideoDevice(TwkApp::VideoModule*, int w, int h, bool alpha,
                          bool floatbuffer = true, bool topleftOrigin = false);

        virtual ~OSMesaVideoDevice();

        virtual std::string hardwareIdentification() const;

        typedef void (*VoidFunc)();
        typedef VoidFunc (*ProcAddressFunc)(const char*);

        static ProcAddressFunc mesaProcAddressFunc();

        //
        //  VideoDevice API
        //

        void resize(int w, int h, bool alpha);

        virtual size_t width() const;
        virtual size_t height() const;
        virtual void makeCurrent() const;
        virtual void redraw() const;
        virtual void redrawImmediately() const;

        virtual void makeCurrent(TwkFB::FrameBuffer*) const;

    private:
        int m_width;
        int m_height;
        bool m_alpha;
        bool m_float;
        bool m_topleft;
        OSMesaImp* m_imp;
        mutable TwkFB::FrameBuffer* m_currentFB;
    };

} // namespace TwkGLF

#endif // __TwkGLF__OSMesaVideoDevice__h__
