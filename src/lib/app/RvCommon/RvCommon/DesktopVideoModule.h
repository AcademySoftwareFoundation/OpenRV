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
        //  shareDevice is the controller's main view device and may be a GL
        //  (QTGLVideoDevice) or Metal (QTMetalVideoDevice) device, so it is
        //  typed as their common base TwkGLF::GLVideoDevice. It is null on the
        //  Vulkan main-view path.
        //
        DesktopVideoModule(NativeDisplayPtr np, TwkGLF::GLVideoDevice* shareDevice);
        virtual ~DesktopVideoModule();

        //
        //  Rebuild the per-screen presentation devices onto targetNative (GL
        //  ScreenView vs the platform's native 10-bit output: a Vulkan
        //  swapchain on Linux and Windows, an IOSurface/CALayer on macOS),
        //  using shareDevice as the new share device. This is the post-startup
        //  analogue of the constructor's one-time createDesktopVideoDevices
        //  call, needed because the backend decision is no longer frozen at
        //  launch.
        //
        //  targetNative is supplied by the caller rather than re-derived here:
        //  it must be the backend the main view is *actually* running, which
        //  the persisted display-depth preference does not reliably reflect
        //  (see DesktopVideoDevice::shouldUseNativePresentation). A
        //  presentation output on the opposite backend to the viewport is a
        //  black display.
        //
        //  The old devices are RETIRED, not destroyed: the caller must call
        //  purgeRetiredDevices() once it has re-pointed everything that
        //  referenced them (see that method for why the order matters). To
        //  avoid a needless teardown and a transient on the second display,
        //  this is a no-op when the effective backend has not changed; it then
        //  returns false and leaves the devices untouched (the caller still
        //  re-binds the share device). Returns true when the devices were
        //  actually rebuilt.
        //
        //  This deliberately does not touch the session's output video device.
        //  The caller (RvApplication::rebuildDesktopVideoDevices) owns
        //  re-binding the share device and re-opening the presentation output,
        //  because the retired devices may be referenced as the session output.
        //
        bool rebuildDevices(const TwkGLF::GLVideoDevice* shareDevice, bool targetNative);

        //
        //  Close and destroy the devices retired by the last rebuildDevices().
        //
        //  Call this only AFTER the IP graph's display groups have been
        //  re-pointed at the new devices. Closing a device destroys its
        //  top-level presentation window, and destroying a visible native
        //  window pumps the event loop: the uncovered main window repaints
        //  synchronously and walks the graph, so any DisplayGroupIPNode still
        //  holding a retired device dereferences freed memory.
        //
        void purgeRetiredDevices();

        //
        //  Devices replaced by the last rebuildDevices() and not yet purged.
        //  The caller needs them to map old -> new by screen so it can repoint
        //  whatever still references them.
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
