//
//  Copyright (c) 2026 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
#pragma once

#ifdef PLATFORM_DARWIN

#include <TwkGLF/GLVideoDevice.h>
#include <RvCommon/QTTranslator.h>
#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <string>

namespace Rv
{
    class MetalView;

    //
    //  QTMetalVideoDevice
    //
    //  Wraps a MetalView as a TwkGLF::GLVideoDevice so that ImageRenderer's
    //  existing GL rendering pipeline (renderMain, shader cache, etc.) can run
    //  unchanged on the Metal path.
    //
    //  How it works:
    //    • A QOpenGLContext + QOffscreenSurface provides the GL context.  On
    //      macOS, Qt backs QOffscreenSurface with a hidden NSView, giving the
    //      context a real native surface (required on Apple Silicon).
    //    • A GLFBO is created in this context; all IPCore rendering goes there.
    //    • syncBuffers() reads the FBO pixels back to CPU, packs them as
    //      ARGB2101010LE, and calls MetalView::presentPixelData() which writes
    //      to an IOSurface and assigns it to a plain CALayer for display.
    //
    //  The IOSurface + CALayer path is used instead of CAMetalLayer because
    //  the CA compositor mishandles MTLPixelFormatBGR10A2Unorm, producing
    //  4× horizontal tiling.  IOSurface with kCVPixelFormatType_ARGB2101010LEPacked
    //  is composited correctly by the macOS window server.
    //
    class QTMetalVideoDevice : public TwkGLF::GLVideoDevice
    {
    public:
        QTMetalVideoDevice(TwkApp::VideoModule* module, const std::string& name, MetalView* view, QWidget* eventWidget);
        virtual ~QTMetalVideoDevice();

        MetalView* metalView() const { return m_view; }

        QWidget* eventWidget() const { return m_eventWidget; }

        void setEventWidget(QWidget* widget);

        const QTTranslator& translator() const { return *m_translator; }

        bool hasTranslator() const { return m_translator != nullptr; }

        void setAbsolutePosition(int x, int y);

        // VideoDevice API
        virtual void makeCurrent() const override;
        virtual void syncBuffers() const override;
        virtual void redraw() const override;
        virtual void redrawImmediately() const override;
        virtual void clearCaches() const override;

        virtual Resolution resolution() const override;
        virtual Offset offset() const override;
        virtual Timing timing() const override;
        virtual VideoFormat format() const override;

        virtual size_t width() const override;
        virtual size_t height() const override;

        virtual void open(const StringVector& args) override;
        virtual void close() override;
        virtual bool isOpen() const override;

        virtual float devicePixelRatio() const override;

        virtual void setPhysicalDevice(VideoDevice* d) override;

        // True when the offscreen GL context, surface, and FBO are ready for rendering.
        bool isGLContextReady() const { return m_glContextReady; }

        // GLVideoDevice API
        virtual TwkGLF::GLFBO* defaultFBO() override;
        virtual const TwkGLF::GLFBO* defaultFBO() const override;
        virtual std::string hardwareIdentification() const override;

    private:
        // Ensure the NSOpenGLContext + FBO exist and match the current view size.
        // Makes the GL context current and binds the FBO on return.
        // Returns false when context creation, makeCurrent, or FBO setup fails.
        bool ensureGLContext() const;

        // Zero-copy present: (re)create a small ring of IOSurfaces, each bound as
        // a GL_TEXTURE_RECTANGLE_ARB / GL_RGB10_A2 texture (via
        // CGLTexImageIOSurface2D) wrapped in a GLFBO, so GL can render straight
        // into IOSurface memory that CoreAnimation composites — no CPU readback.
        // Returns false (and latches m_interopDisabled) if interop is
        // unavailable, in which case syncBuffers() uses the CPU fallback.
        // Assumes the GL context is already current.
        bool ensureIOSurfaceTextures(int w, int h) const;
        void cleanupIOSurfaceTextures() const;

        // Latch the zero-copy IOSurface interop off after a failure, but schedule
        // a retry (kInteropRetryFrames later) instead of disabling it for the
        // lifetime of the window — the failure may be transient (e.g. a
        // zero-size FBO at startup or a momentary context issue).
        void latchInteropFailure() const;

        MetalView* m_view;
        QWidget* m_eventWidget;
        QTTranslator* m_translator;
        float m_devicePixelRatio{1.0f};
        int m_x{0};
        int m_y{0};
        float m_refresh{-1.0f};
        bool m_isOpen{false};

        // Qt GL context + offscreen surface for GL rendering.
        // QOffscreenSurface on macOS is backed by a hidden NSView, which gives
        // the context a proper native surface (required for GL-on-Metal on
        // Apple Silicon where a headless NSOpenGLContext silently fails).
        mutable QOpenGLContext* m_glContext{nullptr};
        mutable QOffscreenSurface* m_offscreenSurface{nullptr};
        mutable TwkGLF::GLFBO* m_fbo{nullptr};
        mutable GLuint m_fboColorTex{0}; // Texture attached to m_fbo; GLFBO does not own it
        mutable int m_fboWidth{0};
        mutable int m_fboHeight{0};
        mutable bool m_glContextReady{false};

        // IOSurface present ring (zero-copy GL->IOSurface->CALayer path).
        // Double-buffered so the CA compositor can read the just-presented
        // surface while GL renders the next frame into the other one.
        // Stored as void* to keep this header free of ObjC/CoreVideo types.
        static constexpr int kRingSize = 2;
        mutable void* m_ioSurfaces[kRingSize]{nullptr, nullptr}; // IOSurfaceRef
        mutable GLuint m_ioTextures[kRingSize]{0, 0};
        mutable TwkGLF::GLFBO* m_ioFbos[kRingSize]{nullptr, nullptr};
        mutable int m_ringIndex{0};
        mutable int m_sharedWidth{0};
        mutable int m_sharedHeight{0};
        mutable bool m_interopDisabled{false};   // interop currently off (with retry)
        mutable int m_interopRetryCountdown{0};  // frames until interop is retried
        mutable bool m_cpuFallbackLogged{false}; // one-shot CPU-fallback log guard

        // Frames to wait before retrying zero-copy interop after a failure
        // (~2 seconds at 60 fps).  Bounds the cost of a permanently-broken
        // interop to one rebuild attempt every kInteropRetryFrames frames.
        static constexpr int kInteropRetryFrames = 120;
    };

} // namespace Rv

#endif // PLATFORM_DARWIN
