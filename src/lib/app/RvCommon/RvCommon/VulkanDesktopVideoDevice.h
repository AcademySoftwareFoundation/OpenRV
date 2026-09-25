//
//  Copyright (c) 2026 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
#ifndef __RvCommon__VulkanDesktopVideoDevice__h__
#define __RvCommon__VulkanDesktopVideoDevice__h__

#include <RvCommon/DesktopVideoDevice.h>

namespace Rv
{
    class VulkanView;

    //
    //  VulkanDesktopVideoDevice
    //
    //  A second-display presentation output that presents through a 10-bit
    //  Vulkan swapchain on Linux and Windows. The inherited transfer() and
    //  transfer2() composite into the view's QTVulkanVideoDevice; only the
    //  window lifecycle and presentation are overridden.
    //
    class VulkanDesktopVideoDevice : public DesktopVideoDevice
    {
    public:
        VulkanDesktopVideoDevice(TwkApp::VideoModule* module, const std::string& name, int screen, const QTGLVideoDevice* shareDevice);
        ~VulkanDesktopVideoDevice() override;

        //  None of these chain to the base, which drives a QOpenGLWidget
        //  (m_view) that is never created here.
        void open(const StringVector& args) override;
        void close() override;
        bool isOpen() const override;
        void makeCurrent() const override;

        void redraw() const override;
        void redrawImmediately() const override;
        void syncBuffers() const override;

    private:
        //  Owns the QTVulkanVideoDevice used as the base m_viewDevice.
        VulkanView* m_vulkanView;
    };

} // namespace Rv

#endif // __RvCommon__VulkanDesktopVideoDevice__h__
