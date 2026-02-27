//
//  Copyright (c) 2024 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
#pragma once

#ifdef PLATFORM_DARWIN

#include <QWindow>
#include <QtCore/QSize>
#include <QtCore/QEvent>
#include <QtCore/QTimer>
#include <QtGui/QImage>
#include <TwkUtil/Timer.h>
#include <boost/thread/thread.hpp>

class QWidget;
class QExposeEvent;
class QMoveEvent;

namespace Rv
{
    class RvDocument;
    class QTMetalVideoDevice;

    //
    //  MetalView
    //
    //  A QWindow subclass that attaches a CALayer backed by an IOSurface to an
    //  overlay NSWindow for true 10-bit display on macOS.  All IPCore image
    //  processing runs in OpenGL via a separate QOpenGLContext+QOffscreenSurface;
    //  the IOSurface path is used only for final pixel delivery.
    //
    //  WHY IOSurface (not CAMetalLayer):
    //    CAMetalLayer with MTLPixelFormatBGR10A2Unorm is mishandled by the CA
    //    compositor — it treats each 4-byte packed pixel as 1 byte, resulting in
    //    an effective texture width of 1920÷4=480, which it then tiles 4× to fill
    //    the display.  Using a plain CALayer backed by an IOSurface with pixel
    //    format kCVPixelFormatType_ARGB2101010LEPacked bypasses this CA bug: the
    //    window server composites IOSurface content natively and correctly for both
    //    10-bit and 8-bit formats.
    //
    //  WHY QWindow (not QWidget):
    //    On macOS, a QWidget participates in Qt's backing-store compositing pipeline.
    //    On a 2× Retina display Qt composites child widgets into a 1× logical bitmap
    //    and the OS upscales it, which causes the Metal layer content to appear as a
    //    2×2 tile of quarter-size copies.  Using QWindow bypasses this — Qt treats
    //    an embedded QWindow (via createWindowContainer) as an opaque native surface
    //    and does not include it in the backing-store composite.
    //
    //  Use this class only when USE_METAL is defined.
    //
    class MetalView : public QWindow
    {
        Q_OBJECT

    public:
        typedef TwkUtil::Timer Timer;

        // Note: no QWidget* parent — QWindow::setParent() (QObject) will be used
        // internally by createWindowContainer; pass nullptr here.
        explicit MetalView(RvDocument* doc, bool vsync = true, int bitsPerChannel = 10);
        ~MetalView();

        QTMetalVideoDevice* videoDevice() const { return m_videoDevice; }

        void setEventWidget(QWidget* widget);

        void stopProcessingEvents();

        virtual bool event(QEvent* event) override;
        virtual bool eventFilter(QObject* object, QEvent* event) override;

        bool firstPaintCompleted() const { return m_firstPaintCompleted; }

        void setContentSize(int w, int h) { m_csize = QSize(w, h); }

        void setMinimumContentSize(int w, int h) { m_msize = QSize(w, h); }

        // Used by RvDocument to pre-size the window before createWindowContainer.
        QSize sizeHint() const { return m_csize; }

        QSize minimumSizeHint() const { return m_msize; }

        void absolutePosition(int& x, int& y) const;

        void* syncClosure() const { return m_syncThreadData; }

        QImage readPixels(int x, int y, int w, int h);

        float devicePixelRatio() const;

        //
        //  IOSurface presentation — called by QTMetalVideoDevice::syncBuffers().
        //
        //  pixels: packed pixel data row-major, y=0 is TOP row (Metal/IOSurface
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
        //  Called once when the window is first exposed.
        void initialize();

        //  Called each time a new frame should be rendered.
        void render();

        void exposeEvent(QExposeEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void moveEvent(QMoveEvent* event) override;
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

        //  Presentation objects — stored as void* so this header compiles in
        //  plain C++ translation units.  The .mm implementation casts them to
        //  the right ObjC / CoreFoundation types.
        void* m_caLayer;       // CALayer*     — root layer of overlay contentView
        void* m_ioSurface;     // IOSurfaceRef  — current pixel buffer
        int m_ioSurfaceWidth;  // width  of current IOSurface (physical pixels)
        int m_ioSurfaceHeight; // height of current IOSurface (physical pixels)
        bool m_ioSurface10bit; // pixel format of current IOSurface

        //  Overlay NSWindow that hosts the CALayer.  The layer must NOT be
        //  attached to the embedded NSView because its superlayer is
        //  _NSOpenGLViewBackingLayer (Qt's GL compositor), which incorrectly
        //  composites layer content on 2× Retina producing 4-copies tiling.
        void* m_overlayWindow; // NSWindow*
    };

} // namespace Rv

#endif // PLATFORM_DARWIN
