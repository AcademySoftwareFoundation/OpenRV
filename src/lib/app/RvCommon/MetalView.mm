//
//  Copyright (c) 2026 Autodesk, Inc. All Rights Reserved.
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
#include <QtGui/QResizeEvent>
#include <QtGui/QShowEvent>
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

    MetalView::MetalView(RvDocument* doc, QWidget* parent, bool vsync, int bitsPerChannel, bool noResize)
        : QWidget(parent)
        , m_doc(doc)
        , m_videoDevice(nullptr)
        , m_syncThreadData(nullptr)
        , m_initialized(false)
        , m_firstPaintCompleted(false)
        , m_postFirstNonEmptyRender(noResize)
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
    {
        // Ensure a native NSView is created immediately so winId() is valid
        // when initialize() is called from showEvent().
        setAttribute(Qt::WA_NativeWindow);
        setAttribute(Qt::WA_NoSystemBackground);
        setAutoFillBackground(false);

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

        // Release CALayer (balance the __bridge_retained in initialize())
        if (m_caLayer)
        {
            CALayer* layer = (__bridge_transfer CALayer*)m_caLayer;
            (void)layer;
            m_caLayer = nullptr;
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
        QPoint gp = mapToGlobal(QPoint(0, 0));
        x = gp.x();
        y = gp.y();
    }

    float MetalView::devicePixelRatio() const
    {
        return static_cast<float>(devicePixelRatioF());
    }

    //--------------------------------------------------------------------------
    // Initialisation
    //--------------------------------------------------------------------------

    void MetalView::initialize()
    {
        if (m_initialized)
            return;
        m_initialized = true;

        // Create a plain CALayer — IOSurface content is uploaded to it each frame
        // by presentPixelData().  We use IOSurface (not CAMetalLayer) for pixel
        // delivery because CAMetalLayer with BGR10A2Unorm is mishandled by the CA
        // compositor on macOS: it treats 4-byte packed pixels as 1 byte, producing
        // 4× horizontal tiling.  IOSurface + kCVPixelFormatType_ARGB2101010LEPacked
        // is composited natively and correctly for both 10-bit and 8-bit formats.
        CALayer* caLayer = [CALayer layer];
        caLayer.opaque          = YES;
        caLayer.contentsGravity = kCAGravityResize;  // IOSurface fills layer exactly

        // Attach directly to the widget's NSView.  Since we use IOSurface (not a
        // Metal drawable), _NSOpenGLViewBackingLayer does not intercept or tile the
        // content — the CA render server composites IOSurface data independently.
        NSView* nsView = (__bridge NSView*)reinterpret_cast<void*>(winId());
        [nsView setLayer:caLayer];
        [nsView setWantsLayer:YES];

        CGFloat scale = static_cast<CGFloat>(devicePixelRatioF());
        if (scale < 1.0) scale = 1.0;
        caLayer.contentsScale = scale;

        m_caLayer = (__bridge_retained void*)caLayer;

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

            m_ioSurface       = surf;
            m_ioSurfaceWidth  = w;
            m_ioSurfaceHeight = h;
            m_ioSurface10bit  = is10bit;
        }

        IOSurfaceRef surf = (IOSurfaceRef)m_ioSurface;

        // Lock the IOSurface for CPU write
        IOSurfaceLock(surf, 0, nullptr);

        void*  base   = IOSurfaceGetBaseAddress(surf);
        size_t bpr    = IOSurfaceGetBytesPerRow(surf);
        size_t srcBpr = (size_t)w * 4;

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
    // QWidget overrides
    //--------------------------------------------------------------------------

    void MetalView::showEvent(QShowEvent* event)
    {
        if (!m_initialized)
            initialize();
        // Post an UpdateRequest so the first render fires from the event loop
        // after the widget is fully laid out (same pattern as QOpenGLWindow).
        QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
        QWidget::showEvent(event);
    }

    void MetalView::resizeEvent(QResizeEvent* event)
    {
        // Update the layer's contentsScale to match the current DPR.
        // The NSView layer frame is managed automatically by AppKit as the
        // widget is resized — no manual frame update required.
        CALayer* layer = (__bridge CALayer*)m_caLayer;
        if (layer)
        {
            CGFloat scale = static_cast<CGFloat>(devicePixelRatioF());
            if (scale < 1.0) scale = 1.0;
            layer.contentsScale = scale;
        }

        if (m_doc)
            m_doc->viewSizeChanged(event->size().width(), event->size().height());

        QWidget::resizeEvent(event);
    }

    void MetalView::keyPressEvent(QKeyEvent* event)
    {
        QWidget::keyPressEvent(event);
    }

    void MetalView::keyReleaseEvent(QKeyEvent* event)
    {
        QWidget::keyReleaseEvent(event);
    }

    void MetalView::mousePressEvent(QMouseEvent* event)
    {
        QWidget::mousePressEvent(event);
    }

    void MetalView::mouseReleaseEvent(QMouseEvent* event)
    {
        QWidget::mouseReleaseEvent(event);
    }

    void MetalView::mouseMoveEvent(QMouseEvent* event)
    {
        QWidget::mouseMoveEvent(event);
    }

    void MetalView::wheelEvent(QWheelEvent* event)
    {
        QWidget::wheelEvent(event);
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
    // event() — mirrors GLView::event() exactly
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
            if (!isVisible())
                return true;
            if (e->oldSize().width() != -1 && e->oldSize().height() != -1)
            {
                ostringstream contents;
                contents << e->oldSize().width() << " " << e->oldSize().height()
                         << "|" << e->size().width() << " " << e->size().height();
                if (m_doc && session)
                    session->userGenericEvent("view-resized", contents.str());
            }
            return QWidget::event(event);
        }

        if (event->type() == QEvent::UpdateRequest)
        {
            render();
            return true;
        }

        // Guard: translator may not be initialized yet (e.g. events fired
        // during construction before setEventWidget() is called).
        if (!m_videoDevice || !m_videoDevice->hasTranslator())
            return QWidget::event(event);

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
            return QWidget::event(event);
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
