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

namespace TwkGLF
{
    class GLVideoDevice;
}

namespace Rv
{

    //
    //  class DesktopVideoModule
    //
    //  This instantiates whichever video devices it can based on the
    //  platform and availability.
    //

    class DesktopVideoModule : public TwkApp::VideoModule
    {
    public:
        //
        //  shareDevice is the main view's GL or Metal device (null on Vulkan).
        //
        DesktopVideoModule(NativeDisplayPtr np, TwkGLF::GLVideoDevice* shareDevice);
        virtual ~DesktopVideoModule();

        //
        //  Rebuild the per-screen presentation devices onto targetNative (Vulkan
        //  or Metal rather than GL), the backend the main view is actually
        //  running (not the persisted preference). Returns false and leaves the
        //  devices untouched when the backend has not changed. The old devices
        //  are retired, not destroyed: the caller re-points the IP graph, then
        //  calls purgeRetiredDevices(). The caller also re-binds the share
        //  device and re-opens the session's presentation output.
        //
        bool rebuildDevices(const TwkGLF::GLVideoDevice* shareDevice, bool targetNative);

        //
        //  Close and destroy the devices retired by the last rebuildDevices().
        //  Call only after the IP graph has been re-pointed: closing a device
        //  destroys a native window, which pumps the event loop and repaints
        //  through any DisplayGroupIPNode still holding a retired device.
        //
        void purgeRetiredDevices();

        //
        //  Devices replaced by the last rebuildDevices() and not yet purged.
        //
        const VideoDevices& retiredDevices() const { return m_retiredDevices; }

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
