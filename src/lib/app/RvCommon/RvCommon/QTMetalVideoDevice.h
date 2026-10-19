//
//  Copyright (c) 2026 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
#pragma once

#ifdef PLATFORM_DARWIN

#include <TwkGLF/GLVideoDevice.h>
#include <TwkUtil/Timer.h>
#include <RvCommon/QTTranslator.h>
#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Rv
{
    class MetalView;

    //
    //  QTMetalVideoDevice
    //
    //  Wraps a MetalView as a TwkGLF::GLVideoDevice so ImageRenderer's GL
    //  pipeline runs unchanged on the Metal path. IPCore renders into an FBO
    //  in an offscreen QOpenGLContext (QOffscreenSurface is backed by a hidden
    //  NSView on macOS, which Apple Silicon requires). syncBuffers() blits that
    //  FBO into IOSurface-backed textures that MetalView shows on a CALayer,
    //  with CPU readback as the fallback.
    //
    //  IOSurface + CALayer is used instead of CAMetalLayer because the CA
    //  compositor tiles MTLPixelFormatBGR10A2Unorm 4x horizontally, while it
    //  composites kCVPixelFormatType_ARGB2101010LEPacked IOSurfaces correctly.
    //
    class QTMetalVideoDevice : public TwkGLF::GLVideoDevice
    {
    public:
        QTMetalVideoDevice(TwkApp::VideoModule* module, const std::string& name, MetalView* view, QWidget* eventWidget);
        ~QTMetalVideoDevice() override;

        void setEventWidget(QWidget* widget);

        const QTTranslator& translator() const { return *m_translator; }

        bool hasTranslator() const { return m_translator != nullptr; }

        void setAbsolutePosition(int x, int y);

        // VideoDevice API
        void makeCurrent() const override;
        void syncBuffers() const override;
        void redraw() const override;
        void redrawImmediately() const override;

        // Worker device for ImageRenderer's threaded upload: its own
        // QOpenGLContext sharing resources with this device's context.
        // Returns nullptr if the offscreen context cannot be established.
        TwkGLF::GLVideoDevice* newSharedContextWorkerDevice() const override;

        Resolution resolution() const override;
        Offset offset() const override;
        Timing timing() const override;
        VideoFormat format() const override;

        size_t width() const override;
        size_t height() const override;

        void open(const StringVector& args) override;
        void close() override;
        bool isOpen() const override;

        float devicePixelRatio() const override;

        void setPhysicalDevice(VideoDevice* d) override;

        // True when the offscreen GL context, surface, and FBO are ready for rendering.
        bool isGLContextReady() const { return m_glContextReady; }

        // GLVideoDevice API
        TwkGLF::GLFBO* defaultFBO() override;
        const TwkGLF::GLFBO* defaultFBO() const override;
        std::string hardwareIdentification() const override;

        //  The offscreen FBO's GL id, or 0 before it exists. Unlike
        //  defaultFBO() it does not create the context, so
        //  DesktopVideoDevice::transfer() can use it as a readiness probe.
        GLuint fboID() const override;

    private:
        // One zero-copy present target: an IOSurface aliased as a
        // GL_TEXTURE_RECTANGLE_ARB / GL_RGB10_A2 texture, wrapped in an FBO.
        struct IOSurfaceSlot
        {
            void* surface{nullptr}; // IOSurfaceRef, retained
            GLuint texture{0};
            std::unique_ptr<TwkGLF::GLFBO> fbo;
        };

        // Three surfaces so GL can render into one while the window server
        // still holds the last two (see findFreeSlot()).
        static constexpr int ringSize = 3;

        // Makes the GL context current and binds an FBO matching the view size.
        // Returns false when context creation, makeCurrent, or FBO setup fails.
        bool ensureGLContext() const;
        bool createOffscreenContext() const;
        bool ensureRenderFbo(int w, int h) const;
        void releaseRenderFbo() const;

        // (Re)builds the IOSurface ring at w x h. Returns false, and schedules
        // a retry, when interop is unavailable. Needs the GL context current.
        bool ensureIOSurfaceTextures(int w, int h) const;
        void cleanupIOSurfaceTextures() const;

        // Turns zero-copy interop off and schedules a retry: failures can be
        // transient (zero-size FBO at startup, a momentary context issue).
        void latchInteropFailure() const;

        // The next ring slot the window server no longer holds
        // (IOSurfaceIsInUse()), starting at m_ringIndex; empty if all are held.
        std::optional<int> findFreeSlot() const;

        // False when a presentation output should skip this frame because
        // every surface is still held; schedules a retry. Always true for
        // the control view. Mirrors VulkanWindow::canPresentNow().
        bool canPresentNow() const;

        bool presentZeroCopy(int w, int h) const;
        void presentViaCpuReadback(int w, int h) const;
        void ensureCpuFallbackTarget(int w, int h) const;
        void cleanupCpuFallbackTarget() const;

        MetalView* m_view{nullptr};
        std::unique_ptr<QTTranslator> m_translator;
        float m_devicePixelRatio{1.0f};
        int m_x{0};
        int m_y{0};
        float m_refresh{-1.0f};

        // Offscreen GL context and surface that IPCore renders through.
        mutable std::unique_ptr<QOpenGLContext> m_glContext;
        mutable std::unique_ptr<QOffscreenSurface> m_offscreenSurface;
        mutable std::unique_ptr<TwkGLF::GLFBO> m_fbo;
        mutable GLuint m_fboColorTex{0}; // Attached to m_fbo; GLFBO does not own it
        mutable int m_fboWidth{0};
        mutable int m_fboHeight{0};
        mutable bool m_glContextReady{false};

        // Zero-copy present ring. IOSurfaceRef is stored as void* to keep this
        // header free of CoreVideo types.
        mutable std::array<IOSurfaceSlot, ringSize> m_ioSlots;
        mutable int m_ringIndex{0};
        mutable int m_sharedWidth{0};
        mutable int m_sharedHeight{0};
        mutable bool m_interopDisabled{false};     // interop currently off (with retry)
        mutable int m_interopRetryCountdown{0};    // frames until interop is retried
        mutable bool m_cpuFallbackLogged{false};   // one-shot CPU-fallback log guard
        mutable TwkUtil::Timer m_lastPresentTimer; // time since the last zero-copy present

        // CPU fallback: a GL_RGB10_A2 blit target does the float to 10-bit
        // conversion and Y-flip on the GPU, so glReadPixels returns
        // ARGB2101010LE directly.
        mutable GLuint m_cpuFlipFbo{0};
        mutable GLuint m_cpuFlipTex{0};
        mutable int m_cpuFlipWidth{0};
        mutable int m_cpuFlipHeight{0};
        mutable std::vector<std::uint32_t> m_cpuPackedScratch;
    };

} // namespace Rv

#endif // PLATFORM_DARWIN
