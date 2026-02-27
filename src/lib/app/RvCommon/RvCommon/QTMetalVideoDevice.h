//
//  Copyright (c) 2024 Autodesk, Inc. All Rights Reserved.
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
    //    • syncBuffers() reads the FBO pixels back to CPU, converts to packed
    //      format (ARGB2101010LE for 10-bit, BGRA8 for 8-bit), and calls
    //      MetalView::presentPixelData() which writes to an IOSurface and
    //      assigns it to a plain CALayer for display.
    //
    //  The IOSurface + CALayer path is used instead of CAMetalLayer because
    //  the CA compositor mishandles MTLPixelFormatBGR10A2Unorm, producing
    //  4× horizontal tiling.  IOSurface with kCVPixelFormatType_ARGB2101010LEPacked
    //  is composited correctly by the macOS window server.
    //
    class QTMetalVideoDevice : public TwkGLF::GLVideoDevice
    {
    public:
        QTMetalVideoDevice(TwkApp::VideoModule* module, const std::string& name, MetalView* view, QWidget* eventWidget, int bitsPerChannel);
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

        // GLVideoDevice API
        virtual TwkGLF::GLFBO* defaultFBO() override;
        virtual const TwkGLF::GLFBO* defaultFBO() const override;
        virtual std::string hardwareIdentification() const override;

    private:
        // Ensure the NSOpenGLContext + FBO exist and match the current view size.
        // Makes the GL context current and binds the FBO on return.
        void ensureGLContext() const;

        MetalView* m_view;
        QWidget* m_eventWidget;
        QTTranslator* m_translator;
        float m_devicePixelRatio{1.0f};
        int m_x{0};
        int m_y{0};
        float m_refresh{-1.0f};
        bool m_isOpen{false};
        int m_bitsPerChannel;

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

        mutable int m_syncLogCount{0};
    };

} // namespace Rv

#endif // PLATFORM_DARWIN
