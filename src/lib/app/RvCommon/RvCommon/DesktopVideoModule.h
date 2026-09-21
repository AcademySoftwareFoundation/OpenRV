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
        //  Rebuild the per-screen presentation devices onto targetVulkan (GL
        //  ScreenView vs Vulkan swapchain), using shareDevice as the new GL
        //  share device. This is the post-startup analogue of the
        //  constructor's one-time createDesktopVideoDevices call, needed
        //  because the backend decision is no longer frozen at launch.
        //
        //  targetVulkan is supplied by the caller rather than re-derived here:
        //  it must be the backend the main view is *actually* running, which
        //  the persisted display-depth preference does not reliably reflect
        //  (see DesktopVideoDevice::shouldUseVulkanPresentation). A presentation
        //  output on the opposite backend to the viewport is a black display.
        //
        //  A cleanly open device is closed -- releasing its Vulkan swapchain or
        //  GL ScreenView -- before it is destroyed. To avoid a needless
        //  teardown and a transient on the second display, this is a no-op when
        //  the effective backend has not changed; it then returns false and
        //  leaves the devices untouched (the caller still re-binds the share
        //  device). Returns true when the devices were actually rebuilt.
        //
        //  This deliberately does not touch the session's output video device.
        //  The caller (RvApplication::rebuildDesktopVideoDevices) owns
        //  re-binding the share device and re-opening the presentation output,
        //  because the devices destroyed here may be referenced as the session
        //  output.
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
