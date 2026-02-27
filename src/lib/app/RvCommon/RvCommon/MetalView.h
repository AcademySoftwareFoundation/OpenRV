//
//  Copyright (c) 2024 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
#pragma once

#ifdef PLATFORM_DARWIN

#include <QWidget>
#include <QtCore/QSize>
#include <QtCore/QEvent>
#include <QtCore/QTimer>
#include <QtGui/QImage>
#include <TwkUtil/Timer.h>
#include <boost/thread/thread.hpp>

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

        explicit MetalView(RvDocument* doc, QWidget* parent = nullptr, bool vsync = true, int bitsPerChannel = 10);
        ~MetalView();

        QTMetalVideoDevice* videoDevice() const { return m_videoDevice; }

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

        void* syncClosure() const { return m_syncThreadData; }

        QImage readPixels(int x, int y, int w, int h);

        float devicePixelRatio() const;

        //
        //  IOSurface presentation — called by QTMetalVideoDevice::syncBuffers().
        //
        //  pixels: packed pixel data row-major, y=0 is TOP row (IOSurface
        //          convention; caller must y-flip from GL bottom-left origin).
        //    is10bit=true  → ARGB2101010LE: (3<<30)|(R<<20)|(G<<10)|B per pixel
        //    is10bit=false → BGRA8:         B,G,R,A bytes per pixel
        //  w,h: physical pixel dimensions (FBO size = view.size × DPR)
        //
        void presentPixelData(const void* pixels, int w, int h, bool is10bit);

        bool isInitialized() const { return m_initialized; }

        int bitsPerChannel() const { return m_bitsPerChannel; }

    public slots:
        void eventProcessingTimeout();

    protected:
        //  Called once when the widget is first shown.
        void initialize();

        //  Called each time a new frame should be rendered.
        void render();

        void showEvent(QShowEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void wheelEvent(QWheelEvent* event) override;

    private:
        RvDocument* m_doc;
        QTMetalVideoDevice* m_videoDevice;
        boost::thread m_swapThread;
        void* m_syncThreadData; // SyncBufferThreadData*

        bool m_initialized;
        bool m_firstPaintCompleted;
        bool m_postFirstNonEmptyRender; // mirrors GLView::m_postFirstNonEmptyRender
        bool m_stopProcessingEvents;
        bool m_userActive;
        bool m_vsync;
        int m_bitsPerChannel;

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
        void* m_ioSurface; // IOSurfaceRef
        int m_ioSurfaceWidth;
        int m_ioSurfaceHeight;
        bool m_ioSurface10bit;
    };

} // namespace Rv

#endif // PLATFORM_DARWIN
