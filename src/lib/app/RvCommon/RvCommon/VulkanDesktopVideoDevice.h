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
    //  A desktop (second-display) presentation output device that delivers the
    //  final frame through a Vulkan swapchain for true 10-bit output on Linux
    //  and Windows, instead of the OpenGL ScreenView the base
    //  DesktopVideoDevice uses.
    //
    //  It reuses the base class's frame handoff wholesale: the renderer's
    //  transfer() / transfer2() composite (including every stereo mode) into
    //  m_viewDevice->defaultFBO(), where m_viewDevice is the presentation
    //  VulkanView's QTVulkanVideoDevice. The only backend-specific behaviour is
    //  owning the Vulkan window and presenting it explicitly -- Vulkan has no
    //  QOpenGLWidget auto-composite -- so this subclass overrides only the
    //  window-lifecycle and present methods and inherits everything else
    //  (transfer/transfer2/fillWithTexture/format/data-format/sync) unchanged.
    //
    class VulkanDesktopVideoDevice : public DesktopVideoDevice
    {
    public:
        VulkanDesktopVideoDevice(TwkApp::VideoModule* module, const std::string& name, int screen, const QTGLVideoDevice* shareDevice);
        ~VulkanDesktopVideoDevice() override;

        //
        //  DesktopVideoDevice / VideoDevice API -- the backend-specific
        //  overrides. Note that none of these chain to the base
        //  implementation: the base drives m_view, a QOpenGLWidget, which is
        //  never created here.
        //
        void open(const StringVector& args) override;
        void close() override;
        bool isOpen() const override;
        void makeCurrent() const override;

        void redraw() const override;
        void redrawImmediately() const override;
        void syncBuffers() const override;

    private:
        //
        //  The presentation output window. It owns its own QTVulkanVideoDevice
        //  (VulkanView::videoDevice()), which is handed to the base
        //  m_viewDevice so the inherited transfer()/transfer2() drive it.
        //
        VulkanView* m_vulkanView;
    };

} // namespace Rv

#endif // __RvCommon__VulkanDesktopVideoDevice__h__
