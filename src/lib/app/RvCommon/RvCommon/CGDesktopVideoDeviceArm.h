//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __RvCommon__CGDesktopVideoDeviceArm__h__
#define __RvCommon__CGDesktopVideoDeviceArm__h__
#include <TwkApp/VideoModule.h>
#include <RvCommon/DesktopVideoDevice.h>
#include <RvCommon/QTGLVideoDevice.h>
#include <TwkGLF/GLVideoDevice.h>
#include <TwkGLFCoreGraphics/CGGLVideoDevice.h>
#include <iostream>

#include <QOpenGLWindow>
#include <QOpenGLFunctions>
#include <QScreen>
#include <QGuiApplication>

namespace Rv
{

    //
    //  CGDesktopVideoDeviceArm
    //
    //  OS X only: Wraps each CGDirectDisplayID as a DesktopVideoDevice  (for
    //  Apple Silicon only. For Apple Intel, see old implementation in
    //  CGDesktopVideoDevice)
    //

    class CGDesktopVideoDeviceArm : public DesktopVideoDevice
    {
    public:
        class Mode
        {
        public:
            int32_t ioModeId = -1;
            uint32_t ioFlags = 0;
            int32_t width = 0;
            int32_t height = 0;
            double refreshRate = 0;
            int32_t videoFormatIndex = -1;
            CGDirectDisplayID cgScreen = 0;
            std::string description;
            bool isGUI = false;
        };

        class Display
        {
        public:
            CGDirectDisplayID cgScreen = 0;
            int32_t currentIoModeId = -1;
            std::string name;
            std::vector<CGDesktopVideoDeviceArm::Mode> modes;
        };

    public:
        CGDesktopVideoDeviceArm(TwkApp::VideoModule*, const CGDesktopVideoDeviceArm::Display& display, const QTGLVideoDevice* share);

        virtual ~CGDesktopVideoDeviceArm();

        virtual void makeCurrent() const;

        //
        //  VideoDevice API
        //

        virtual Resolution resolution() const;
        virtual Offset offset() const;
        virtual Timing timing() const;
        virtual VideoFormat format() const;

        virtual size_t width() const;
        virtual size_t height() const;
        virtual float pixelScale() const;

        virtual void open(const StringVector&);
        virtual void close();
        virtual bool isOpen() const;

        virtual void syncBuffers() const;
        virtual bool isSyncing() const;

        virtual int qtScreen() const { return qtScreenFromCG(m_display.cgScreen); }

        const CGDirectDisplayID cgDisplay() const { return m_display.cgScreen; }

        virtual ColorProfile colorProfile() const;

        void createFormats();

    public:
        // public static helpers
        static std::vector<VideoDevice*> createDesktopVideoDevices(TwkApp::VideoModule*, const QTGLVideoDevice* share);

    private:
        // private helpers
        static std::vector<CGDesktopVideoDeviceArm::Display> getDisplays();
        static CGDisplayModeRef displayModeRefFromIoModeId(CGDirectDisplayID cgScreen, int32_t ioModeId);
        static bool switchToMode(const CGDesktopVideoDeviceArm::Mode& mode, QRect& bounds);
        static int32_t qtScreenFromCG(CGDirectDisplayID cgScreen);
        static CGDesktopVideoDeviceArm::Mode constructModeFromDisplayModeRef(CGDirectDisplayID cgScreen, CGDisplayModeRef modeRef);

        static std::vector<CGDesktopVideoDeviceArm::Mode> getDisplayModes(CGDirectDisplayID screen);
        static QRect cgScreenBounds(CGDirectDisplayID cgScreen);
        static int32_t getCurrentIoModeId(CGDirectDisplayID cgScreen);

    private:
        // Saved screen mode state (resolution+hz)
        CGDisplayModeRef m_savedMode;
        int32_t m_savedScreen;

        // Valid display information for the screen
        CGDesktopVideoDeviceArm::Display m_display;

        // color profile
        mutable ColorProfile m_colorProfile;

        // Qt OpenGL window and gl functions
        QOpenGLWidget m_view;
    };

} // namespace Rv

#endif // __RvCommon__CGDesktopVideoDeviceArm__h__
