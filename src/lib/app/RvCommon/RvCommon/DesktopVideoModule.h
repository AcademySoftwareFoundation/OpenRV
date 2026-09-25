//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __RvCommon__DesktopVideoModule__h__
#define __RvCommon__DesktopVideoModule__h__
#include <TwkGLF/GL.h>
#include <iostream>
#include <TwkApp/VideoModule.h>

namespace Rv
{

    //
    //  class DesktopVideoModule
    //
    //  This instantiates whichever video devices it can based on the
    //  platform and availability.
    //

    class QTGLVideoDevice;

    class DesktopVideoModule : public TwkApp::VideoModule
    {
    public:
        DesktopVideoModule(NativeDisplayPtr np, QTGLVideoDevice* shareDevice);
        virtual ~DesktopVideoModule();

        //
        //  Rebuild the per-screen presentation devices onto targetVulkan, the
        //  backend the main view is actually running (not the persisted
        //  preference). Returns false and leaves the devices untouched when the
        //  backend has not changed. The caller re-binds the share device and
        //  re-opens the session's presentation output.
        //
        bool rebuildDevices(const QTGLVideoDevice* shareDevice, bool targetVulkan);

        virtual std::string name() const;
        virtual void open();
        virtual void close();
        virtual bool isOpen() const;

        //
        //  If possible, derive the appropriate refresh rate or devicefrom
        //  the absolute position of the window on the desktop.
        //

        virtual TwkApp::VideoDevice* deviceFromPosition(int x, int y) const;
    };

} // namespace Rv

#endif // __RvCommon__DesktopVideoModule__h__
