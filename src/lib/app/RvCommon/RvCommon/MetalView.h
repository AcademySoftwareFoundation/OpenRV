//
//  Copyright (c) 2026 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
#pragma once

#ifdef PLATFORM_DARWIN

#include <QWidget>
#include <QtCore/QSize>
#include <QtCore/QEvent>
#include <QtCore/QTimer>
#include <TwkUtil/Timer.h>

namespace Rv
{
    class RvDocument;
    class QTMetalVideoDevice;

    //
    //  MetalView
    //
    //  A QWidget subclass that attaches a CALayer backed by an IOSurface to its
    //  native NSView for true 10-bit display on macOS.  All IPCore image
    //  processing runs in OpenGL via a separate QOpenGLContext+QOffscreenSurface;
    //  the IOSurface path is used only for final pixel delivery.
    //
    //  Why IOSurface (not CAMetalLayer):
    //    CAMetalLayer with MTLPixelFormatBGR10A2Unorm is mishandled by the CA
    //    compositor — it treats 4-byte packed pixels as 1 byte, resulting in an
    //    effective texture width of 1920÷4=480 which is then tiled 4× to fill the
    //    display.  Using a plain CALayer backed by an IOSurface with pixel format
    //    kCVPixelFormatType_ARGB2101010LEPacked bypasses this CA bug: the window
    //    server composites IOSurface content natively and correctly for both 10-bit
    //    and 8-bit formats.
    //
    //  Why QWidget (not QWindow):
    //    Qt manages layout, positioning, and event routing for QWidget automatically.
    //    We attach a CALayer directly to the widget's NSView; Qt's backing-store does
    //    not affect IOSurface layer contents (those are composited independently by
    //    the CA render server at the layer's native resolution).
    //
    //  Use this class only when USE_METAL is defined.
    //
    class MetalView : public QWidget
    {
        Q_OBJECT

    public:
        typedef TwkUtil::Timer Timer;

        explicit MetalView(RvDocument* doc, QWidget* parent = nullptr, bool noResize = true);
        ~MetalView();

        QTMetalVideoDevice* videoDevice() const { return m_videoDevice; }

        //
        //  Returns true when the platform can present 10-bit content through the
        //  Metal/IOSurface path (i.e. a Metal device is available).  Static and
        //  side-effect-free so RvDocument can query it before constructing any
        //  view, to decide between the Metal (10-bit) and OpenGL (8-bit) backends.
        //  Mirrors VulkanView::supports10BitPresentation() so the call site stays
        //  backend-agnostic when the Vulkan and Metal branches merge.
        //
        static bool supports10BitPresentation();

        //  We own pixel delivery via the CALayer/IOSurface; tell Qt not to use a
        //  backing store for this widget (see WA_PaintOnScreen in the .mm).
        QPaintEngine* paintEngine() const override { return nullptr; }

        void setEventWidget(QWidget* widget);

        void stopProcessingEvents();

        virtual bool event(QEvent* event) override;
        virtual bool eventFilter(QObject* object, QEvent* event) override;

        bool firstPaintCompleted() const { return m_firstPaintCompleted; }

        void setContentSize(int w, int h) { m_csize = QSize(w, h); }

        void setMinimumContentSize(int w, int h) { m_msize = QSize(w, h); }

        QSize sizeHint() const override { return m_csize; }

        QSize minimumSizeHint() const override { return m_msize; }

        void absolutePosition(int& x, int& y) const;

        float devicePixelRatio() const;

        //
        //  IOSurface presentation — called by QTMetalVideoDevice::syncBuffers().
        //
        //  pixels: packed ARGB2101010LE row-major, y=0 is TOP row (IOSurface
        //          convention; caller must y-flip from GL bottom-left origin).
        //  Format: (3<<30)|(R<<20)|(G<<10)|B per pixel.
        //  w,h: physical pixel dimensions (FBO size = view.size × DPR).
        //
        void presentPixelData(const void* pixels, int w, int h);

        //
        //  Assign an externally-owned IOSurface (the zero-copy path: the video
        //  device renders GL straight into it) to the CALayer for display.
        //  Records it as the last-presented surface for re-present on expose.
        //
        void presentIOSurface(void* ioSurfaceRef);

        //
        //  Re-assign the last-presented IOSurface to the CALayer (a lightweight
        //  re-present of the last frame, with no GL readback).  Used on expose.
        //
        void presentCachedSurface();

        //
        //  Coalesced redraw request.  Posts a single QEvent::UpdateRequest per
        //  event-loop cycle (subsequent calls are no-ops until the pending
        //  render runs), mirroring the implicit coalescing QWidget::update()
        //  gets from Qt on the GL path.
        //
        void requestUpdate();

        // Synchronous render (used by redrawImmediately); bypasses UpdateRequest coalescing.
        void renderImmediately();

        bool isInitialized() const { return m_initialized; }

    public slots:
        void eventProcessingTimeout();

    protected:
        //  Called once when the widget is first shown.
        void initialize();

        //  Called each time a new frame should be rendered.
        void render();

        void showEvent(QShowEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        RvDocument* m_doc;
        QTMetalVideoDevice* m_videoDevice;

        bool m_initialized;
        bool m_firstPaintCompleted;
        bool m_postFirstNonEmptyRender; // mirrors GLView::m_postFirstNonEmptyRender
        bool m_stopProcessingEvents;
        bool m_userActive;

        QSize m_csize;
        QSize m_msize;
        QWidget* m_eventWidget;

        unsigned int m_lastKey;
        QEvent::Type m_lastKeyType;
        Timer m_activityTimer;
        Timer m_activationTimer;
        QTimer m_eventProcessingTimer;

        //  Presentation objects — stored as void* to keep header pure C++.
        void* m_caLayer;   // CALayer*
        void* m_ioSurface; // IOSurfaceRef — owned here only by the CPU fallback path
        int m_ioSurfaceWidth;
        int m_ioSurfaceHeight;

        //  Last IOSurface assigned to the CALayer (may be device-owned in the
        //  zero-copy path or m_ioSurface in the fallback path).  Re-presented on
        //  expose.  Not owned here; the CALayer retains its own contents.
        void* m_lastPresentedSurface;

        //  Coalescing flag for requestUpdate(); cleared at the start of render().
        bool m_updatePending;

        //  Count of consecutive render() attempts where the offscreen GL context
        //  was not ready.  When it reaches kContextFailureThreshold the view
        //  requests a one-time fallback to the OpenGL GLView; reset to 0 on any
        //  successful render.
        int m_contextFailureCount;
        static constexpr int kContextFailureThreshold = 30;
    };

} // namespace Rv

#endif // PLATFORM_DARWIN
