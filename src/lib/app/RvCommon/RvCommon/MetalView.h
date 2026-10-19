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

#include <memory>

class QKeyEvent;

namespace IPCore
{
    class Session;
}

namespace Rv
{
    class RvDocument;
    class QTMetalVideoDevice;

    //  Creates a w x h kCVPixelFormatType_ARGB2101010LEPacked IOSurface.
    //  Returns a retained IOSurfaceRef (the caller CFReleases it), or nullptr.
    void* createArgb2101010Surface(int w, int h);

    //
    //  MetalView
    //
    //  A QWidget whose native NSView hosts a CALayer backed by an IOSurface,
    //  for true 10-bit display on macOS. IPCore renders in OpenGL through
    //  QTMetalVideoDevice's offscreen context; the IOSurface only delivers the
    //  final pixels.
    //
    //  CAMetalLayer is not used because the CA compositor tiles
    //  MTLPixelFormatBGR10A2Unorm 4x horizontally; it composites
    //  kCVPixelFormatType_ARGB2101010LEPacked IOSurfaces correctly at 8 and
    //  10 bits. A QWidget (not a QWindow) gets Qt layout and event routing,
    //  and Qt's backing store does not affect the layer's contents.
    //
    //  Use this class only when USE_METAL is defined.
    //
    class MetalView : public QWidget
    {
        Q_OBJECT

    public:
        using Timer = TwkUtil::Timer;

        //  presentationMode builds a passive second-display surface driven by a
        //  MetalDesktopVideoDevice: doc must be null, and the view never drives
        //  the session or triggers the main-window GL fallback.
        explicit MetalView(RvDocument* doc, QWidget* parent = nullptr, bool noResize = true, bool presentationMode = false);
        ~MetalView() override;

        QTMetalVideoDevice* videoDevice() const { return m_videoDevice.get(); }

        bool isPresentationMode() const { return m_presentationMode; }

        //
        //  True when a Metal device exists and the OS can create the 10-bit
        //  IOSurface format. Static and side-effect-free so RvDocument can pick
        //  the Metal (10-bit) or OpenGL (8-bit) backend before building a view.
        //  Mirrors VulkanView::supports10BitPresentation().
        //
        static bool supports10BitPresentation();

        //  Pixels go through the CALayer/IOSurface, so Qt gets no paint engine
        //  (see WA_PaintOnScreen in the .mm).
        QPaintEngine* paintEngine() const override { return nullptr; }

        void setEventWidget(QWidget* widget);

        void stopProcessingEvents();

        bool event(QEvent* event) override;
        bool eventFilter(QObject* object, QEvent* event) override;

        bool firstPaintCompleted() const { return m_firstPaintCompleted; }

        void setContentSize(int w, int h) { m_csize = QSize(w, h); }

        void setMinimumContentSize(int w, int h) { m_msize = QSize(w, h); }

        QSize sizeHint() const override { return m_csize; }

        QSize minimumSizeHint() const override { return m_msize; }

        void absolutePosition(int& x, int& y) const;

        float devicePixelRatio() const;

        //
        //  CPU-fallback present, called by QTMetalVideoDevice::syncBuffers().
        //  pixels: packed ARGB2101010LE, (3<<30)|(R<<20)|(G<<10)|B, row 0 at
        //  the top (the caller flips GL's bottom-left origin). w, h are
        //  physical pixels (view size x DPR).
        //
        void presentPixelData(const void* pixels, int w, int h);

        //  Shows an IOSurface the caller owns (the zero-copy path) and records
        //  it for re-present on expose.
        void presentIOSurface(void* ioSurfaceRef);

        //  Posts at most one QEvent::UpdateRequest per event-loop cycle, the
        //  coalescing QWidget::update() gives the GL path.
        void requestUpdate();

        // Synchronous render (used by redrawImmediately); bypasses UpdateRequest coalescing.
        void renderImmediately();

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
        //  Presents the session's output device when it is a separate window,
        //  then makes this view's GL context current again.
        void syncOutputDevice(const IPCore::Session* session) const;

        //  Maps events into the output device's domain in mirror display mode.
        void updateTranslatorMapping(const IPCore::Session* session);

        //  Restarts the activity timers; returns the seconds from window
        //  activation to the first click, else 0.
        float trackActivity(const QEvent* event);

        //  True when kevent repeats the previous key event; records kevent.
        bool isRepeatedKey(const QKeyEvent* kevent);

        void updateLayerScale();

        RvDocument* m_doc{nullptr};
        std::unique_ptr<QTMetalVideoDevice> m_videoDevice;
        bool m_presentationMode{false};

        bool m_initialized{false};
        bool m_firstPaintCompleted{false};
        bool m_postFirstNonEmptyRender{false}; // mirrors GLView::m_postFirstNonEmptyRender
        bool m_stopProcessingEvents{false};
        bool m_userActive{true};

        QSize m_csize{1024, 576};
        QSize m_msize{128, 128};
        QWidget* m_eventWidget{nullptr};

        int m_lastKey{0};
        QEvent::Type m_lastKeyType{QEvent::None};
        Timer m_activityTimer;
        Timer m_activationTimer;
        QTimer m_eventProcessingTimer;

        //  Stored as void* to keep the header pure C++.
        void* m_caLayer{nullptr};   // CALayer*
        void* m_ioSurface{nullptr}; // IOSurfaceRef, owned; used only by the CPU fallback
        int m_ioSurfaceWidth{0};
        int m_ioSurfaceHeight{0};

        //  Last IOSurface on the CALayer, re-presented on expose. Not owned:
        //  the CALayer retains its contents.
        void* m_lastPresentedSurface{nullptr};

        //  Set while an UpdateRequest posted by requestUpdate() is pending.
        bool m_updatePending{false};

        //  Consecutive render() calls without a ready GL context; reset on success.
        int m_contextFailureCount{0};
    };

} // namespace Rv

#endif // PLATFORM_DARWIN
