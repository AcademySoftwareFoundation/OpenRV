//
//  Copyright (c) 2024 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//

#ifdef PLATFORM_DARWIN

#import <QuartzCore/QuartzCore.h>
#import <AppKit/AppKit.h>
#import <IOSurface/IOSurface.h>
#import <CoreVideo/CoreVideo.h>

#include <RvCommon/MetalView.h>
#include <RvCommon/QTMetalVideoDevice.h>
#include <RvCommon/RvDocument.h>
#include <RvApp/Options.h>
#include <RvApp/RvSession.h>
#include <IPCore/Session.h>
#include <TwkApp/Event.h>
#include <TwkApp/VideoDevice.h>

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

#include <QtCore/QCoreApplication>
#include <QtGui/QExposeEvent>
#include <QtGui/QResizeEvent>
#include <QtGui/QMoveEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QWheelEvent>
#include <QtWidgets/QMenu>
#include <QtWidgets/QWidget>

#include <iostream>
#include <sstream>

namespace Rv
{
    using namespace boost;
    using namespace std;
    using namespace TwkApp;
    using namespace IPCore;

    //--------------------------------------------------------------------------
    // SyncBufferThreadData  (same pattern as in GLView.cpp)
    //--------------------------------------------------------------------------

    namespace
    {
        class SyncBufferThreadData
        {
        public:
            explicit SyncBufferThreadData(const VideoDevice* device)
                : m_device(device)
                , m_done(false)
                , m_doSync(false)
                , m_running(false)
                , m_mutex()
                , m_cond()
            {
            }

            void run()
            {
                while (!m_done)
                {
                    boost::mutex::scoped_lock lock(m_mutex);
                    m_running = true;
                    m_doSync = false;

                    m_cond.wait(lock);

                    if (m_device && m_doSync && !m_done)
                        m_device->syncBuffers();

                    m_doSync = false;
                }
            }

            void notify(bool finish = false)
            {
                {
                    boost::mutex::scoped_lock lock(m_mutex);
                    if (finish)
                        m_done = true;
                    else
                        m_doSync = true;
                    if (!m_running)
                        return;
                }
                m_cond.notify_one();
            }

            const VideoDevice* device() const { return m_device; }
            void setDevice(const VideoDevice* d) { m_device = d; }

        private:
            SyncBufferThreadData(const SyncBufferThreadData&) = delete;
            void operator=(const SyncBufferThreadData&) = delete;

            bool m_done;
            bool m_doSync;
            bool m_running;
            boost::mutex m_mutex;
            boost::condition_variable m_cond;
            const VideoDevice* m_device;
        };

        class ThreadTrampoline
        {
        public:
            explicit ThreadTrampoline(MetalView* view)
                : m_view(view)
            {
            }

            void operator()()
            {
                SyncBufferThreadData* closure =
                    reinterpret_cast<SyncBufferThreadData*>(m_view->syncClosure());
                closure->run();
            }

        private:
            MetalView* m_view;
        };

    } // anonymous namespace

    //--------------------------------------------------------------------------
    // MetalView implementation
    //--------------------------------------------------------------------------

    MetalView::MetalView(RvDocument* doc, bool vsync, int bitsPerChannel)
        : QWindow()          // QWindow, not QWidget — bypasses Qt's backing-store
                             // compositor so the IOSurface layer renders at full
                             // resolution without being composited as a 1× bitmap.
        , m_doc(doc)
        , m_videoDevice(nullptr)
        , m_syncThreadData(nullptr)
        , m_initialized(false)
        , m_firstPaintCompleted(false)
        , m_postFirstNonEmptyRender(false)
        , m_stopProcessingEvents(false)
        , m_userActive(true)
        , m_vsync(vsync)
        , m_bitsPerChannel(bitsPerChannel)
        , m_csize(1024, 576)
        , m_msize(128, 128)
        , m_eventWidget(nullptr)
        , m_lastKey(0)
        , m_lastKeyType(QEvent::None)
        , m_caLayer(nullptr)
        , m_ioSurface(nullptr)
        , m_ioSurfaceWidth(0)
        , m_ioSurfaceHeight(0)
        , m_ioSurface10bit(false)
        , m_overlayWindow(nullptr)
    {
        // Use RasterSurface so Qt creates a plain NSView for this QWindow.
        // We do not use Qt's rendering pipeline here at all — rendering goes
        // through an overlay NSWindow (see initialize()) that hosts the
        // CALayer/IOSurface as its contentView root layer, completely bypassing
        // the NSViewBackingLayer compositing that causes tiling artifacts.
        setSurfaceType(QSurface::RasterSurface);

        ostringstream str;
        str << UI_APPLICATION_NAME " Main Window (Metal)" << "/" << m_doc;
        m_videoDevice = new QTMetalVideoDevice(nullptr, str.str(), this, nullptr, m_bitsPerChannel);

        m_activityTimer.start();

        m_eventProcessingTimer.setSingleShot(true);
        connect(&m_eventProcessingTimer, SIGNAL(timeout()), this, SLOT(eventProcessingTimeout()));
    }

    MetalView::~MetalView()
    {
        delete m_videoDevice;

        if (m_syncThreadData)
        {
            SyncBufferThreadData* closure =
                reinterpret_cast<SyncBufferThreadData*>(m_syncThreadData);
            closure->notify(true);
            m_swapThread.join();
            delete closure;
        }

        // Release IOSurface
        if (m_ioSurface)
        {
            CFRelease((IOSurfaceRef)m_ioSurface);
            m_ioSurface = nullptr;
        }

        // Release CALayer
        if (m_caLayer)
        {
            CALayer* layer = (__bridge_transfer CALayer*)m_caLayer;
            (void)layer;
            m_caLayer = nullptr;
        }

        // Close and release the overlay NSWindow.
        if (m_overlayWindow)
        {
            NSWindow* overlay = (__bridge NSWindow*)m_overlayWindow;
            [overlay close];
            NSWindow* rel = (__bridge_transfer NSWindow*)m_overlayWindow;
            (void)rel;
            m_overlayWindow = nullptr;
        }
    }

    //--------------------------------------------------------------------------

    void MetalView::setEventWidget(QWidget* widget)
    {
        m_eventWidget = widget;
        if (m_videoDevice)
            m_videoDevice->setEventWidget(widget);
    }

    void MetalView::stopProcessingEvents() { m_stopProcessingEvents = true; }

    void MetalView::absolutePosition(int& x, int& y) const
    {
        x = 0;
        y = 0;
        QPoint gp = mapToGlobal(QPoint(0, 0));
        x = gp.x();
        y = gp.y();
    }

    float MetalView::devicePixelRatio() const
    {
        return static_cast<float>(QWindow::devicePixelRatio());
    }

    //--------------------------------------------------------------------------
    // Initialisation
    //--------------------------------------------------------------------------

    void MetalView::initialize()
    {
        if (m_initialized)
            return;
        m_initialized = true;

        // Create a plain CALayer — the root layer of the overlay's contentView.
        // We use IOSurface (not CAMetalLayer) for pixel delivery because
        // CAMetalLayer with BGR10A2Unorm is mishandled by the CA compositor
        // on macOS: it treats 4-byte packed pixels as 1-byte, producing 4×
        // tiling.  IOSurface + kCVPixelFormatType_ARGB2101010LEPacked is
        // composited natively and correctly for both 10-bit and 8-bit formats.
        CALayer* caLayer = [CALayer layer];
        caLayer.opaque           = YES;
        caLayer.contentsGravity  = kCAGravityResize;  // IOSurface fills layer exactly

        // ---------------------------------------------------------------
        // Overlay NSWindow approach (same as before)
        // ---------------------------------------------------------------

        NSView*   nsView       = (__bridge NSView*)reinterpret_cast<void*>(winId());
        NSWindow* parentWindow = nsView.window;

        if (!parentWindow)
        {
            // Fallback: no window yet — attach layer directly to nsView.
            std::cerr << "[MetalView::initialize] nsView.window is nil — "
                         "using inline layer (fallback)\n";
            [nsView setLayer:caLayer];
            [nsView setWantsLayer:YES];
            CGFloat scale = static_cast<CGFloat>(QWindow::devicePixelRatio());
            if (scale < 1.0) scale = 1.0;
            caLayer.contentsScale = scale;
            m_caLayer       = (__bridge_retained void*)caLayer;
            m_overlayWindow = nullptr;
        }
        else
        {
            CGFloat scale       = static_cast<CGFloat>(QWindow::devicePixelRatio());
            if (scale < 1.0) scale = 1.0;
            NSRect viewInWindow = [nsView convertRect:nsView.bounds toView:nil];
            NSRect viewOnScreen = [parentWindow convertRectToScreen:viewInWindow];

            // Create a transparent, borderless overlay window
            NSWindow* overlay = [[NSWindow alloc]
                initWithContentRect:viewOnScreen
                          styleMask:NSWindowStyleMaskBorderless
                            backing:NSBackingStoreBuffered
                              defer:NO];
            [overlay setBackgroundColor:[NSColor clearColor]];
            overlay.opaque             = NO;
            overlay.hasShadow          = NO;
            overlay.ignoresMouseEvents = YES;

            // Attach our CALayer as the ROOT layer of the overlay's contentView.
            // Set layer BEFORE wantsLayer to prevent AppKit creating its own first.
            NSView* ov = overlay.contentView;
            [ov setLayer:caLayer];
            [ov setWantsLayer:YES];

            caLayer.contentsScale = scale;

            // Standalone overlay: NOT a child of the parent window, so the
            // window server composites it directly without going through the
            // parent's _NSOpenGLViewBackingLayer.
            overlay.level = parentWindow.level + 1;
            [overlay orderFront:nil];

            m_caLayer       = (__bridge_retained void*)caLayer;
            m_overlayWindow = (__bridge_retained void*)overlay;

            std::cerr << "[MetalView::initialize]"
                      << " overlay=(" << viewOnScreen.origin.x << ","
                      << viewOnScreen.origin.y << " "
                      << viewOnScreen.size.width << "x" << viewOnScreen.size.height << ")"
                      << " QWindow_dpr=" << scale
                      << " overlay.backingScaleFactor=" << overlay.backingScaleFactor
                      << "\n";
        }

        // Kick off session initialization (equivalent to GLView::initializeGL)
        if (m_doc)
            m_doc->initializeSession();

        // Start the sync-buffer background thread
        SyncBufferThreadData* closure = new SyncBufferThreadData(m_videoDevice);
        m_syncThreadData = closure;
        m_swapThread = boost::thread(ThreadTrampoline(this));
    }

    //--------------------------------------------------------------------------
    // presentPixelData — called by QTMetalVideoDevice::syncBuffers()
    //--------------------------------------------------------------------------

    void MetalView::presentPixelData(const void* pixels, int w, int h, bool is10bit)
    {
        CALayer* layer = (__bridge CALayer*)m_caLayer;
        if (!layer || w <= 0 || h <= 0)
            return;

        // Create/recreate IOSurface if size or format changed
        if (!m_ioSurface
            || m_ioSurfaceWidth  != w
            || m_ioSurfaceHeight != h
            || m_ioSurface10bit  != is10bit)
        {
            if (m_ioSurface)
            {
                CFRelease((IOSurfaceRef)m_ioSurface);
                m_ioSurface = nullptr;
            }

            // kCVPixelFormatType_ARGB2101010LEPacked ('l30r') = 0x6C333072:
            //   32-bit word, big-endian bit order: A[31:30] R[29:20] G[19:10] B[9:0]
            //   Stored little-endian in memory; CA handles this format natively
            //   for 10-bit wide-color display.
            // kCVPixelFormatType_32BGRA = 'BGRA': standard 8-bit BGRA.
            uint32_t pixelFormat = is10bit
                ? kCVPixelFormatType_ARGB2101010LEPacked
                : kCVPixelFormatType_32BGRA;

            NSDictionary* props = @{
                (NSString*)kIOSurfaceWidth:           @(w),
                (NSString*)kIOSurfaceHeight:          @(h),
                (NSString*)kIOSurfaceBytesPerElement: @(4),
                (NSString*)kIOSurfacePixelFormat:     @(pixelFormat),
            };
            IOSurfaceRef surf = IOSurfaceCreate((__bridge CFDictionaryRef)props);
            if (!surf)
            {
                std::cerr << "[MetalView::presentPixelData] IOSurfaceCreate failed "
                          << w << "x" << h << " fmt=" << pixelFormat << "\n";
                return;
            }

            m_ioSurface      = surf;
            m_ioSurfaceWidth  = w;
            m_ioSurfaceHeight = h;
            m_ioSurface10bit  = is10bit;
        }

        IOSurfaceRef surf = (IOSurfaceRef)m_ioSurface;

        // Lock the IOSurface for CPU write
        IOSurfaceLock(surf, 0, nullptr);

        void*  base    = IOSurfaceGetBaseAddress(surf);
        size_t bpr     = IOSurfaceGetBytesPerRow(surf);
        size_t srcBpr  = (size_t)w * 4;

        if (bpr == srcBpr)
        {
            memcpy(base, pixels, (size_t)h * bpr);
        }
        else
        {
            // IOSurface may add row padding — copy row by row
            const uint8_t* src = static_cast<const uint8_t*>(pixels);
            uint8_t*       dst = static_cast<uint8_t*>(base);
            for (int row = 0; row < h; ++row, src += srcBpr, dst += bpr)
                memcpy(dst, src, srcBpr);
        }

        IOSurfaceUnlock(surf, 0, nullptr);

        // Push to display: assign IOSurface as layer contents inside a
        // CATransaction with animations disabled for immediate presentation.
        [CATransaction begin];
        [CATransaction setDisableActions:YES];
        layer.contents = (__bridge id)surf;
        [CATransaction commit];
    }

    //--------------------------------------------------------------------------

    void MetalView::render()
    {
        IPCore::Session* session = m_doc ? m_doc->session() : nullptr;
        if (!session)
            return;

        if (m_doc && session && m_videoDevice)
        {
            m_videoDevice->makeCurrent();

            if (m_userActive && m_activityTimer.elapsed() > 1.0)
            {
                if (m_doc->mainPopup() && !m_doc->mainPopup()->isVisible()
                    && m_eventWidget && m_eventWidget->hasFocus())
                {
                    TwkApp::ActivityChangeEvent aevent("user-inactive", m_videoDevice);
                    m_videoDevice->sendEvent(aevent);
                    m_userActive = false;
                }
            }

            int x = 0, y = 0;
            absolutePosition(x, y);
            m_videoDevice->setAbsolutePosition(x, y);

            session->render();

            // Mirror GLView::paintGL(): on the first non-empty render, resize the
            // window to fit the media content and center it on screen.
            if (!m_postFirstNonEmptyRender && session->postFirstNonEmptyRender())
            {
                m_postFirstNonEmptyRender = true;
                if (!session->isFullScreen())
                {
                    m_doc->resizeToFit(false, false);
                    m_doc->center();
                }
            }

            m_firstPaintCompleted = true;
        }

        if (m_stopProcessingEvents)
            return;

        if (session)
        {
            if (session->outputVideoDevice() != videoDevice())
                session->outputVideoDevice()->syncBuffers();
            else
                m_videoDevice->syncBuffers();
        }

        if (session)
        {
            session->addSyncSample();
            session->postRender();
        }

        m_eventProcessingTimer.start();
    }

    //--------------------------------------------------------------------------
    // QWindow overrides
    //--------------------------------------------------------------------------

    void MetalView::exposeEvent(QExposeEvent* event)
    {
        if (!m_initialized && isExposed())
            initialize();
        // Post an UpdateRequest so the first render fires from the event loop
        // after the window is fully laid out (same pattern as QOpenGLWindow).
        if (isExposed())
            QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
        QWindow::exposeEvent(event);
    }

    //  Shared helper: reposition and resize the standalone overlay to match the
    //  QWindow's current screen rect.  Called from resizeEvent and moveEvent.
    static void repositionOverlay(NSWindow* overlay, CALayer* layer,
                                  NSView* nsView)
    {
        if (!overlay || !layer || !nsView || !nsView.window)
            return;
        NSRect viewInWindow = [nsView convertRect:nsView.bounds toView:nil];
        NSRect viewOnScreen = [nsView.window convertRectToScreen:viewInWindow];
        [overlay setFrame:viewOnScreen display:YES];

        CGFloat scale = overlay.backingScaleFactor;
        if (scale < 1.0) scale = 1.0;
        layer.contentsScale = scale;
    }

    void MetalView::resizeEvent(QResizeEvent* event)
    {
        NSWindow* overlay = (__bridge NSWindow*)m_overlayWindow;
        CALayer*  layer   = (__bridge CALayer*)m_caLayer;
        NSView*   nsView  = (__bridge NSView*)reinterpret_cast<void*>(winId());

        if (overlay && layer)
        {
            repositionOverlay(overlay, layer, nsView);
        }
        else if (layer)
        {
            // Fallback (no overlay): update contentsScale directly
            CGFloat scale = static_cast<CGFloat>(QWindow::devicePixelRatio());
            if (scale < 1.0) scale = 1.0;
            layer.contentsScale = scale;
        }

        if (m_doc)
            m_doc->viewSizeChanged(event->size().width(), event->size().height());

        QWindow::resizeEvent(event);
    }

    void MetalView::moveEvent(QMoveEvent* event)
    {
        // The overlay is a standalone floating window, so it doesn't auto-follow
        // parent moves.  Reposition it here whenever the QWindow moves.
        NSWindow* overlay = (__bridge NSWindow*)m_overlayWindow;
        CALayer*  layer   = (__bridge CALayer*)m_caLayer;
        NSView*   nsView  = (__bridge NSView*)reinterpret_cast<void*>(winId());
        repositionOverlay(overlay, layer, nsView);
        QWindow::moveEvent(event);
    }

    void MetalView::keyPressEvent(QKeyEvent* event)
    {
        QWindow::keyPressEvent(event);
    }

    void MetalView::keyReleaseEvent(QKeyEvent* event)
    {
        QWindow::keyReleaseEvent(event);
    }

    void MetalView::mousePressEvent(QMouseEvent* event)
    {
        QWindow::mousePressEvent(event);
    }

    void MetalView::mouseReleaseEvent(QMouseEvent* event)
    {
        QWindow::mouseReleaseEvent(event);
    }

    void MetalView::mouseMoveEvent(QMouseEvent* event)
    {
        QWindow::mouseMoveEvent(event);
    }

    void MetalView::wheelEvent(QWheelEvent* event)
    {
        QWindow::wheelEvent(event);
    }

    //--------------------------------------------------------------------------
    // eventProcessingTimeout slot
    //--------------------------------------------------------------------------

    void MetalView::eventProcessingTimeout()
    {
        if (m_doc && m_doc->session())
            m_doc->session()->userGenericEvent("per-render-event-processing", "");
    }

    //--------------------------------------------------------------------------
    // event() — mirrors GLView::event() exactly, replacing QOpenGLWindow base
    //--------------------------------------------------------------------------

    bool MetalView::event(QEvent* event)
    {
        bool keyevent = false;
        Rv::Session* session = m_doc ? m_doc->session() : nullptr;

        if (m_stopProcessingEvents)
        {
            event->accept();
            return true;
        }

        if (event->type() == QEvent::WindowActivate)
            m_activationTimer.start();

        float activationTime = 0.0f;
        if (m_activationTimer.isRunning())
        {
            if (event->type() == QEvent::MouseButtonPress)
            {
                activationTime = m_activationTimer.elapsed();
                m_activationTimer.stop();
            }
            if (event->type() == QEvent::MouseMove)
                m_activationTimer.stop();
        }

        if (event->type() != QEvent::Paint)
        {
            m_activityTimer.stop();
            m_activityTimer.start();

            if (!m_userActive)
            {
                TwkApp::ActivityChangeEvent aevent("user-active", m_videoDevice);
                m_userActive = true;
                m_videoDevice->sendEvent(aevent);
            }
        }

        if (QKeyEvent* kevent = dynamic_cast<QKeyEvent*>(event))
        {
            keyevent = true;
            if (m_lastKey == kevent->key()
                && (m_lastKeyType == QEvent::ShortcutOverride
                        && (kevent->type() == QEvent::KeyPress)
                    || (m_lastKeyType == kevent->type())))
            {
                m_lastKey     = kevent->key();
                m_lastKeyType = kevent->type();
                event->accept();
                return true;
            }
            m_lastKeyType = kevent->type();
            m_lastKey     = kevent->key();
        }

        switch (event->type())
        {
        case QEvent::FocusIn:
            m_videoDevice->translator().resetModifiers();
            // fall-through
        case QEvent::Enter:
            if (m_eventWidget)
                m_eventWidget->setFocus(Qt::MouseFocusReason);
            break;
        default:
            break;
        }

        if (event->type() == QEvent::Resize)
        {
            QResizeEvent* e = static_cast<QResizeEvent*>(event);
            if (!isExposed())
                return true;
            if (e->oldSize().width() != -1 && e->oldSize().height() != -1)
            {
                ostringstream contents;
                contents << e->oldSize().width() << " " << e->oldSize().height()
                         << "|" << e->size().width() << " " << e->size().height();
                if (m_doc && session)
                    session->userGenericEvent("view-resized", contents.str());
            }
            return QWindow::event(event);
        }

        if (event->type() == QEvent::UpdateRequest)
        {
            render();
            return true;
        }

        // Guard: translator may not be initialized yet (e.g. events fired
        // during construction before setEventWidget() is called).
        if (!m_videoDevice || !m_videoDevice->hasTranslator())
            return QWindow::event(event);

        if (session && session->outputVideoDevice()
            && session->outputVideoDevice()->displayMode()
                   == TwkApp::VideoDevice::MirrorDisplayMode)
        {
            if (const TwkApp::VideoDevice* cdv = session->controlVideoDevice())
            {
                const TwkApp::VideoDevice* odv = session->outputVideoDevice();
                if (odv && cdv != odv && cdv == videoDevice())
                {
                    const float w  = static_cast<float>(width());
                    const float h  = static_cast<float>(height());
                    const float ow = static_cast<float>(odv->width());
                    const float oh = static_cast<float>(odv->height());
                    const float aspect  = w / h;
                    const float oaspect = ow / oh;

                    m_videoDevice->translator().setRelativeDomain(ow, oh);

                    if (aspect >= oaspect)
                    {
                        const float yscale  = oh / h;
                        const float yoffset = 0.0f;
                        const float xscale  = yscale;
                        const float xoffset = -(w * yscale - ow) / 2.0f;
                        m_videoDevice->translator().setScaleAndOffset(
                            xoffset, yoffset, xscale, yscale);
                    }
                    else
                    {
                        const float xscale  = ow / w;
                        const float xoffset = 0.0f;
                        const float yscale  = xscale;
                        const float yoffset = -(xscale * h - oh) / 2.0f;
                        m_videoDevice->translator().setScaleAndOffset(
                            xoffset, yoffset, xscale, yscale);
                    }
                }
                else
                {
                    m_videoDevice->translator().setScaleAndOffset(0, 0, 1.0f, 1.0f);
                    m_videoDevice->translator().setRelativeDomain(width(), height());
                }
            }
            else
            {
                m_videoDevice->translator().setScaleAndOffset(0, 0, 1.0f, 1.0f);
                m_videoDevice->translator().setRelativeDomain(width(), height());
            }
        }
        else
        {
            m_videoDevice->translator().setScaleAndOffset(0, 0, 1.0f, 1.0f);
            m_videoDevice->translator().setRelativeDomain(width(), height());
        }

        if (session)
            session->setEventVideoDevice(videoDevice());

        if (m_videoDevice->translator().sendQTEvent(event, activationTime))
        {
            event->accept();
            return true;
        }
        else
        {
            return QWindow::event(event);
        }
    }

    //--------------------------------------------------------------------------
    // eventFilter() — mirrors GLView::eventFilter()
    //--------------------------------------------------------------------------

    bool MetalView::eventFilter(QObject* object, QEvent* event)
    {
        if (event->type() == QEvent::KeyPress    || event->type() == QEvent::KeyRelease
            || event->type() == QEvent::Shortcut || event->type() == QEvent::ShortcutOverride)
        {
            if (QKeyEvent* kevent = dynamic_cast<QKeyEvent*>(event))
            {
                if (m_lastKey == kevent->key()
                    && (m_lastKeyType == QEvent::ShortcutOverride
                            && (kevent->type() == QEvent::KeyPress)
                        || (m_lastKeyType == kevent->type())))
                {
                    m_lastKey     = kevent->key();
                    m_lastKeyType = kevent->type();
                    event->accept();
                    return true;
                }
                m_lastKeyType = kevent->type();
                m_lastKey     = kevent->key();
            }

            Session* session = m_doc ? m_doc->session() : nullptr;
            if (session)
            {
                session->setEventVideoDevice(videoDevice());
                if (m_videoDevice->translator().sendQTEvent(event))
                {
                    event->accept();
                    return true;
                }
            }

            event->accept();
            return true;
        }

        return false;
    }

    //--------------------------------------------------------------------------
    // readPixels
    //--------------------------------------------------------------------------

    QImage MetalView::readPixels(int x, int y, int w, int h)
    {
        return QImage(0, 0, QImage::Format_RGBA8888);
    }

} // namespace Rv

#endif // PLATFORM_DARWIN
