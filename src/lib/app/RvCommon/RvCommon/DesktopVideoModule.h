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
        //  Rebuild the per-screen presentation devices onto targetNative (Vulkan
        //  on Linux and Windows, Metal on macOS), the backend the main view runs
        //  (not the persisted preference). Returns false and
        //  leaves the devices untouched when the backend has not changed. The
        //  caller re-binds the share device and re-opens the session's
        //  presentation output.
        //
        //  The old devices are RETIRED, not destroyed: the caller must call
        //  purgeRetiredDevices() once it has re-pointed the IP graph at the new
        //  devices.
        //
        bool rebuildDevices(const QTGLVideoDevice* shareDevice, bool targetNative);

        //
        //  Close and destroy the devices retired by the last rebuildDevices().
        //
        //  Call this only AFTER the IP graph's display groups have been
        //  refreshed against the new devices. Closing a device destroys its
        //  top-level presentation window, and destroying a visible native window
        //  pumps the event loop: the uncovered main window repaints
        //  synchronously and walks the graph, so any DisplayGroupIPNode still
        //  holding a retired device dereferences freed memory.
        //
        void purgeRetiredDevices();

        virtual std::string name() const;
        virtual void open();
        virtual void close();
        virtual bool isOpen() const;

        //
        //  If possible, derive the appropriate refresh rate or devicefrom
        //  the absolute position of the window on the desktop.
        //

        virtual TwkApp::VideoDevice* deviceFromPosition(int x, int y) const;

    private:
        //  Devices replaced by rebuildDevices(), awaiting purgeRetiredDevices().
        VideoDevices m_retiredDevices;
    };

} // namespace Rv

#endif // __RvCommon__DesktopVideoModule__h__
