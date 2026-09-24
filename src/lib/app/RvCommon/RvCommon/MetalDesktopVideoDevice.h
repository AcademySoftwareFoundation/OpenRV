//
//  Copyright (c) 2026 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
#ifndef __RvCommon__MetalDesktopVideoDevice__h__
#define __RvCommon__MetalDesktopVideoDevice__h__

#ifdef PLATFORM_DARWIN

#include <RvCommon/DesktopVideoDevice.h>

namespace Rv
{
    class MetalView;

    //
    //  MetalDesktopVideoDevice
    //
    //  A desktop (second-display) presentation output device that delivers the
    //  final frame through an IOSurface-backed CALayer for true 10-bit output on
    //  macOS, instead of the 8-bit-limited OpenGL ScreenView used by the base
    //  DesktopVideoDevice.
    //
    //  It reuses the base class's frame handoff: the renderer's transfer() /
    //  transfer2() composite (including all stereo modes) into
    //  m_viewDevice->defaultFBO(), where m_viewDevice is the presentation
    //  MetalView's QTMetalVideoDevice. The only backend-specific behavior is
    //  owning the Metal window and presenting it explicitly (there is no
    //  QOpenGLWidget auto-composite), so this subclass overrides only the
    //  window-lifecycle and present methods and inherits everything else
    //  (transfer/transfer2/fillWithTexture/format/data-format/sync) unchanged.
    //
    class MetalDesktopVideoDevice : public DesktopVideoDevice
    {
    public:
        MetalDesktopVideoDevice(TwkApp::VideoModule* module, const std::string& name, int screen, const TwkGLF::GLVideoDevice* shareDevice);
        ~MetalDesktopVideoDevice() override;

        // DesktopVideoDevice / VideoDevice API (backend-specific overrides)
        void open(const StringVector& args) override;
        void close() override;
        bool isOpen() const override;
        void makeCurrent() const override;

        void redraw() const override;
        void redrawImmediately() const override;
        void syncBuffers() const override;

    private:
        //  The presentation output window. Owns its own QTMetalVideoDevice
        //  (MetalView::videoDevice()), which is assigned to the base
        //  m_viewDevice so inherited transfer()/transfer2() drive it.
        MetalView* m_metalView;
    };

} // namespace Rv

#endif // PLATFORM_DARWIN

#endif // __RvCommon__MetalDesktopVideoDevice__h__
