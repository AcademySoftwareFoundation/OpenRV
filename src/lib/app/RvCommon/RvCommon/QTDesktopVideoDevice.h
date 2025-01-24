//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __RvCommon__QTDesktopVideoDevice__h__
#define __RvCommon__QTDesktopVideoDevice__h__
#include <iostream>
#include <RvCommon/DesktopVideoDevice.h>
#include <TwkGLF/GLVideoDevice.h>
#include <TwkApp/VideoModule.h>
#include <QtWidgets/QWidget>
#include <QtOpenGL/QGLWidget>
#include <RvCommon/QTGLVideoDevice.h>

namespace Rv
{

    //
    //  QTDesktopVideoDevice
    //
    //  Wraps QDesktop "screens" as DesktopVideoDevices. The underlying
    //  code will create a fullscreen widget when bound and tree the
    //  "screen" like one giant output. The QTDesktopVideoDevice cannot
    //  change output resolution, etc, it can only report the existing
    //  desktop setup.
    //
    //  QTDesktopVideoDevice is the fall back screen device when
    //  CoreGraphics, NVApi, or NvCtrl are not available (i.e. its an ATI
    //  card on linux or windows).
    //
    //  See also: CGDesktopVideoDevice and NVDesktopVideoDevice which are
    //  both preferred over QTDesktopVideoDevice since they allow the user
    //  to control the screen more directly.
    //

    class QTDesktopVideoDevice : public DesktopVideoDevice
    {
    public:
        QTDesktopVideoDevice(TwkApp::VideoModule*, const std::string& name,
                             int screen, const QTGLVideoDevice* share);

        virtual ~QTDesktopVideoDevice();

        //  From QTGLVideoDevice

        void setWidget(QGLWidget*);

        QGLWidget* widget() const { return m_view; }

        virtual void makeCurrent() const;

        const QTTranslator& translator() const { return *m_translator; }

        virtual void redraw() const;
        virtual void redrawImmediately() const;

        void setAbsolutePosition(int x, int y)
        {
            m_x = x;
            m_y = y;
        }

        //
        //  VideoDevice API
        //

        virtual Resolution resolution() const;
        virtual Offset offset() const;
        virtual Timing timing() const;
        virtual VideoFormat format() const;

        virtual size_t width() const;
        virtual size_t height() const;

        virtual void open(const StringVector&);
        virtual void close();
        virtual bool isOpen() const;

        virtual void syncBuffers() const;

        virtual int qtScreen() const { return m_screen; }
#ifdef PLATFORM_WINDOWS
        virtual ColorProfile colorProfile() const;
#endif
    private:
        int m_screen;
        int m_x;
        int m_y;
        QGLWidget* m_view;
        QTTranslator* m_translator;
        mutable ColorProfile m_colorProfile;
    };

} // namespace Rv

#endif // __RvCommon__QTDesktopVideoDevice__h__
