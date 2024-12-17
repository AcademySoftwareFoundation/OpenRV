//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __RvCommon__CGDesktopVideoDevice__h__
#define __RvCommon__CGDesktopVideoDevice__h__
#include <TwkApp/VideoModule.h>
#include <RvCommon/DesktopVideoDevice.h>
#include <RvCommon/QTGLVideoDevice.h>
#include <TwkGLF/GLVideoDevice.h>
#include <TwkGLFCoreGraphics/CGGLVideoDevice.h>
#include <iostream>

namespace Rv
{

    //
    //  CGDesktopVideoDevice
    //
    //  OS X only: Wraps each CGDirectDisplayID as a DesktopVideoDevice
    //

    class CGDesktopVideoDevice : public DesktopVideoDevice
    {
    public:
        typedef std::map<int32_t, size_t> IOModeMap;

        CGDesktopVideoDevice(TwkApp::VideoModule*, const std::string& name,
                             CGDirectDisplayID cgScreen, int qtScreen,
                             const QTGLVideoDevice* share);

        virtual ~CGDesktopVideoDevice();

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

        virtual int qtScreen() const { return m_qtScreen; }

        const CGDirectDisplayID cgDisplay() const { return m_cgScreen; }

        virtual ColorProfile colorProfile() const;

        void createFormats();

    private:
        CGDirectDisplayID m_cgScreen;
        IOModeMap m_ioModeMap;
        int m_qtScreen;
        CGLContextObj m_context;
        CGLPixelFormatObj m_pfo;
        GLint m_npfo;
        CFArrayRef m_modes;
        CGDisplayModeRef m_savedMode;
        mutable ColorProfile m_colorProfile;
    };

} // namespace Rv

#endif // __RvCommon__CGDesktopVideoDevice__h__
