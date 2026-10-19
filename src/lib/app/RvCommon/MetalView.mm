//
//  Copyright (c) 2026 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//

#ifdef PLATFORM_DARWIN

#import <QuartzCore/QuartzCore.h>
#import <AppKit/AppKit.h>
#import <IOSurface/IOSurface.h>
#import <IOSurface/IOSurfaceObjC.h>
#import <CoreVideo/CoreVideo.h>
#import <Metal/Metal.h>

#include <RvCommon/MetalView.h>
#include <RvCommon/DesktopVideoDevice.h>
#include <RvCommon/QTMetalVideoDevice.h>
#include <RvCommon/RvDocument.h>
#include <RvApp/Options.h>
#include <RvApp/RvSession.h>
#include <IPCore/Session.h>
#include <TwkApp/Event.h>
#include <TwkApp/VideoDevice.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QMetaObject>
#include <QtGui/QResizeEvent>
#include <QtGui/QShowEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QPaintEvent>
#include <QtWidgets/QMenu>
#include <QtWidgets/QWidget>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <sstream>

namespace
{
    constexpr int bytesPerPixel = 4; // ARGB2101010LE

    constexpr int probeSurfaceSize = 16;

    // render() calls without a GL context before falling back to OpenGL.
    constexpr int contextFailureThreshold = 30;

    // Idle time after which the user is reported inactive.
    constexpr double userIdleSeconds = 1.0;

    bool probeTenBitPresentation()
    {
        // MetalView.mm is compiled with -fobjc-arc, which releases the device.
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        if (device == nil)
        {
            std::cerr << "INFO: [MetalView] no Metal device available; 10-bit "
                         "Metal presentation unavailable.\n";
            return false;
        }

        // Check that the OS can create the 10-bit format before committing the
        // window to Metal, as the Vulkan side validates its surface format.
        auto* probe = static_cast<IOSurfaceRef>(Rv::createArgb2101010Surface(probeSurfaceSize, probeSurfaceSize));
        if (probe == nullptr)
        {
            std::cerr << "INFO: [MetalView] IOSurface ARGB2101010 probe failed; "
                         "10-bit Metal presentation unavailable.\n";
            return false;
        }
        CFRelease(probe);

        // Informational only: an SDR panel still composites 10-bit content but
        // may dither or truncate it at scanout (Vulkan also presents 10-bit there).
        if (NSScreen* screen = [NSScreen mainScreen])
        {
            const NSInteger bps = NSBitsPerSampleFromDepth([screen depth]);
            const char* note = bps >= 10 ? " (deep color: 10-bit reaches the panel).\n"
                                         : " (SDR 8-bit: 10-bit content is composited but "
                                           "may be dithered/truncated at scanout).\n";
            std::cerr << "INFO: [MetalView] main display bits-per-sample = " << static_cast<long>(bps) << note;
        }

        return true;
    }
} // namespace

namespace Rv
{
    using namespace TwkApp;

    void* createArgb2101010Surface(int w, int h)
    {
        // The pixel format matches GL_BGRA + GL_UNSIGNED_INT_2_10_10_10_REV.
        NSDictionary* props = @{
            IOSurfacePropertyKeyWidth : @(w),
            IOSurfacePropertyKeyHeight : @(h),
            IOSurfacePropertyKeyBytesPerElement : @(bytesPerPixel),
            IOSurfacePropertyKeyPixelFormat : @(kCVPixelFormatType_ARGB2101010LEPacked),
        };
        return IOSurfaceCreate((__bridge CFDictionaryRef)props);
    }

    //--------------------------------------------------------------------------
    // MetalView implementation
    //--------------------------------------------------------------------------

    MetalView::MetalView(RvDocument* doc, QWidget* parent, bool noResize, bool presentationMode)
        : QWidget(parent)
        , m_doc(doc)
        , m_presentationMode(presentationMode)
        , m_postFirstNonEmptyRender(noResize)
    {
        // Create the native NSView now so winId() is valid in initialize().
        setAttribute(Qt::WA_NativeWindow);
        setAttribute(Qt::WA_NoSystemBackground);
        // Pixels go through the CALayer, so stop Qt's backing store from
        // painting black over it on expose (paintEngine() returns nullptr).
        setAttribute(Qt::WA_PaintOnScreen);
        setAttribute(Qt::WA_OpaquePaintEvent);
        setAutoFillBackground(false);

        std::ostringstream str;
        if (m_presentationMode)
        {
            // One presentation view can exist per screen and has no doc, so key on the view.
            str << UI_APPLICATION_NAME " Presentation (Metal)" << "/" << static_cast<const void*>(this);
        }
        else
        {
            str << UI_APPLICATION_NAME " Main Window (Metal)" << "/" << m_doc;
        }
        m_videoDevice = std::make_unique<QTMetalVideoDevice>(nullptr, str.str(), this, nullptr);

        m_activityTimer.start();

        m_eventProcessingTimer.setSingleShot(true);
        connect(&m_eventProcessingTimer, SIGNAL(timeout()), this, SLOT(eventProcessingTimeout()));
    }

    MetalView::~MetalView()
    {
        // The device's IOSurfaces and GL objects go before the layer that shows them.
        m_videoDevice.reset();

        releaseCpuSurfaces();

        // Balances the __bridge_retained in initialize().
        if (m_caLayer)
        {
            CFBridgingRelease(m_caLayer);
            m_caLayer = nullptr;
        }
    }

    //--------------------------------------------------------------------------

    bool MetalView::supports10BitPresentation()
    {
        // Fixed per machine and re-asked on every device rebuild, so probe once.
        static const bool supported = probeTenBitPresentation();
        return supported;
    }

    //--------------------------------------------------------------------------

    void MetalView::setEventWidget(QWidget* widget)
    {
        m_eventWidget = widget;
        m_videoDevice->setEventWidget(widget);
    }

    void MetalView::stopProcessingEvents() { m_stopProcessingEvents = true; }

    void MetalView::absolutePosition(int& x, int& y) const
    {
        const QPoint gp = mapToGlobal(QPoint(0, 0));
        x = gp.x();
        y = gp.y();
    }

    float MetalView::devicePixelRatio() const { return static_cast<float>(devicePixelRatioF()); }

    //--------------------------------------------------------------------------
    // Initialisation
    //--------------------------------------------------------------------------

    void MetalView::updateLayerScale()
    {
        CALayer* layer = (__bridge CALayer*)m_caLayer;
        layer.contentsScale = std::max<CGFloat>(devicePixelRatioF(), 1.0);
    }

    void MetalView::attachLayer(WId nativeView)
    {
        auto* nsView = (__bridge NSView*)reinterpret_cast<void*>(nativeView);
        if (!nsView)
        {
            return;
        }

        // A plain CALayer whose contents are the presented IOSurface. With
        // IOSurface contents (not a Metal drawable), the view's backing layer
        // does not intercept or tile the content.
        CALayer* caLayer = [CALayer layer];
        caLayer.opaque = YES;
        caLayer.contentsGravity = kCAGravityResize; // IOSurface fills layer exactly

        [nsView setWantsLayer:YES];
        [nsView setLayer:caLayer];

        void* oldLayer = m_caLayer;
        m_caLayer = (__bridge_retained void*)caLayer;
        updateLayerScale();

        // The new layer must retain the last frame before the old layer can
        // release it: m_lastPresentedSurface does not own it.
        if (m_lastPresentedSurface)
        {
            presentIOSurface(m_lastPresentedSurface);
        }

        if (oldLayer)
        {
            CFBridgingRelease(oldLayer);
        }
    }

    void MetalView::handleNativeViewChange()
    {
        // internalWinId(), unlike winId(), does not create a native view
        // while Qt is tearing this one down.
        const WId nativeView = internalWinId();
        if (!m_initialized || !nativeView)
        {
            return;
        }

        auto* nsView = (__bridge NSView*)reinterpret_cast<void*>(nativeView);
        if (nsView.layer == (__bridge CALayer*)m_caLayer)
        {
            return;
        }

        std::cout << "INFO: MetalView: native view recreated; re-attaching the presentation layer" << std::endl;
        attachLayer(nativeView);
        requestUpdate();
    }

    void MetalView::initialize()
    {
        if (m_initialized)
        {
            return;
        }
        m_initialized = true;

        // winId() creates the native NSView if Qt has not yet.
        attachLayer(winId());

        //  Session init must not run from here: loading packages can create a
        //  QWebEngineView, which destroys this view's native window while it is
        //  on the stack. RvApplication::newSessionFromFiles() does it after show().
    }

    //--------------------------------------------------------------------------
    // Presentation
    //--------------------------------------------------------------------------

    void MetalView::releaseCpuSurfaces()
    {
        for (void*& surface : m_cpuSurfaces)
        {
            if (surface)
            {
                CFRelease(static_cast<IOSurfaceRef>(surface));
                surface = nullptr;
            }
        }
        m_cpuSurfaceWidth = 0;
        m_cpuSurfaceHeight = 0;
        m_cpuSurfaceIndex = 0;
    }

    bool MetalView::ensureCpuSurfaces(int w, int h)
    {
        if (m_cpuSurfaces[0] && m_cpuSurfaceWidth == w && m_cpuSurfaceHeight == h)
        {
            return true;
        }

        // Allocate the replacements before releasing the old surfaces. The
        // CALayer retains the one it shows, so releasing ours is safe.
        std::array<void*, cpuSurfaceCount> fresh{};
        for (void*& surface : fresh)
        {
            surface = createArgb2101010Surface(w, h);
            if (!surface)
            {
                std::cerr << "[MetalView::presentPixelData] IOSurfaceCreate failed " << w << "x" << h << "\n";
                for (void* created : fresh)
                {
                    if (created)
                    {
                        CFRelease(static_cast<IOSurfaceRef>(created));
                    }
                }
                return false;
            }
        }

        releaseCpuSurfaces();
        m_cpuSurfaces = fresh;
        m_cpuSurfaceWidth = w;
        m_cpuSurfaceHeight = h;
        return true;
    }

    void* MetalView::nextCpuSurface()
    {
        // Starts at the surface not presented last; if the window server holds
        // both, writing into that one is the least visible choice.
        int chosen = m_cpuSurfaceIndex;
        for (int i = 0; i < cpuSurfaceCount; ++i)
        {
            const int slot = (m_cpuSurfaceIndex + i) % cpuSurfaceCount;
            if (!IOSurfaceIsInUse(static_cast<IOSurfaceRef>(m_cpuSurfaces[slot])))
            {
                chosen = slot;
                break;
            }
        }
        m_cpuSurfaceIndex = (chosen + 1) % cpuSurfaceCount;
        return m_cpuSurfaces[chosen];
    }

    void MetalView::presentPixelData(const void* pixels, int w, int h)
    {
        if (!m_caLayer || w <= 0 || h <= 0 || !ensureCpuSurfaces(w, h))
        {
            return;
        }

        void* target = nextCpuSurface();
        auto* surf = static_cast<IOSurfaceRef>(target);

        if (IOSurfaceLock(surf, 0, nullptr) != KERN_SUCCESS)
        {
            std::cerr << "[MetalView::presentPixelData] IOSurfaceLock failed\n";
            return;
        }

        void* base = IOSurfaceGetBaseAddress(surf);
        const size_t bpr = IOSurfaceGetBytesPerRow(surf);
        const size_t srcBpr = static_cast<size_t>(w) * bytesPerPixel;

        if (!base)
        {
            std::cerr << "[MetalView::presentPixelData] IOSurface base address "
                         "is null\n";
            IOSurfaceUnlock(surf, 0, nullptr);
            return;
        }

        if (bpr == srcBpr)
        {
            std::memcpy(base, pixels, static_cast<size_t>(h) * bpr);
        }
        else
        {
            // IOSurface may pad rows, so copy row by row.
            const auto* src = static_cast<const uint8_t*>(pixels);
            auto* dst = static_cast<uint8_t*>(base);
            for (int row = 0; row < h; ++row, src += srcBpr, dst += bpr)
            {
                std::memcpy(dst, src, srcBpr);
            }
        }

        IOSurfaceUnlock(surf, 0, nullptr);

        presentIOSurface(target);
    }

    void MetalView::presentIOSurface(void* ioSurfaceRef)
    {
        CALayer* layer = (__bridge CALayer*)m_caLayer;
        auto* surf = static_cast<IOSurfaceRef>(ioSurfaceRef);
        if (!layer || !surf)
        {
            return;
        }

        // Disable implicit animations so the new contents show immediately.
        [CATransaction begin];
        [CATransaction setDisableActions:YES];
        layer.contents = (__bridge id)surf;
        [CATransaction commit];

        // The CALayer retains its contents, so this stays valid while it is shown.
        m_lastPresentedSurface = ioSurfaceRef;
    }

    void MetalView::requestUpdate()
    {
        if (m_updatePending)
        {
            return;
        }
        m_updatePending = true;
        QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
    }

    void MetalView::renderImmediately()
    {
        m_updatePending = false;
        render();
    }

    void MetalView::syncOutputDevice(const IPCore::Session* session) const
    {
        const TwkApp::VideoDevice* output = session->outputVideoDevice();
        if (output && output != videoDevice())
        {
            output->syncBuffers();

            //  The output made its own GL context current; restore ours.
            m_videoDevice->makeCurrent();
        }
    }

    //--------------------------------------------------------------------------

    void MetalView::render()
    {
        // A presentation view is composited by its MetalDesktopVideoDevice and
        // must never drive the session. Its own UpdateRequests (a skipped
        // frame's retry, show, resize) re-present what is already composited,
        // as VulkanWindow::render() does for a passive output.
        if (m_presentationMode)
        {
            m_videoDevice->syncBuffers();
            return;
        }

        IPCore::Session* session = m_doc ? m_doc->session() : nullptr;
        if (!session)
        {
            return;
        }

        m_videoDevice->makeCurrent();
        if (!m_videoDevice->isGLContextReady())
        {
            // A persistent failure means this view can never paint, so swap to
            // the 8-bit GLView. Queued, so this view is not destroyed inside render().
            if (++m_contextFailureCount == contextFailureThreshold)
            {
                std::cerr << "WARNING: [MetalView] offscreen GL context not ready after " << contextFailureThreshold
                          << " attempts; falling back to OpenGL.\n";
                QMetaObject::invokeMethod(m_doc, "fallbackMetalToGLView", Qt::QueuedConnection);
            }
            return;
        }

        m_contextFailureCount = 0;

        if (m_userActive && m_activityTimer.elapsed() > userIdleSeconds)
        {
            if (m_doc->mainPopup() && !m_doc->mainPopup()->isVisible() && m_eventWidget && m_eventWidget->hasFocus())
            {
                const TwkApp::ActivityChangeEvent aevent("user-inactive", m_videoDevice.get());
                m_videoDevice->sendEvent(aevent);
                m_userActive = false;
            }
        }

        int x = 0;
        int y = 0;
        absolutePosition(x, y);
        m_videoDevice->setAbsolutePosition(x, y);

        session->render();

        // As in GLView::paintGL(): fit and center the window on the first non-empty render.
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

        if (m_stopProcessingEvents)
        {
            return;
        }

        // Unlike QOpenGLWidget, nothing composites this view implicitly, so
        // present it even when a separate presentation output is active.
        m_videoDevice->syncBuffers();
        syncOutputDevice(session);

        session->addSyncSample();
        session->postRender();

        m_eventProcessingTimer.start();
    }

    //--------------------------------------------------------------------------
    // QWidget overrides
    //--------------------------------------------------------------------------

    void MetalView::showEvent(QShowEvent* event)
    {
        if (!m_initialized)
        {
            initialize();
        }
        // Render first from the event loop, once the widget is laid out.
        requestUpdate();
        QWidget::showEvent(event);
    }

    void MetalView::resizeEvent(QResizeEvent* event)
    {
        // AppKit resizes the layer frame with the view; only the scale needs updating.
        if (m_caLayer)
        {
            updateLayerScale();
        }

        if (m_doc)
        {
            m_doc->viewSizeChanged(event->size().width(), event->size().height());
        }

        QWidget::resizeEvent(event);

        // QOpenGLWidget repaints after resizeGL(); a plain QWidget must ask,
        // so the FBO and IOSurface rebuild at the new size.
        requestUpdate();

        // A presentation output is only filled by the main view's frame.
        if (m_presentationMode)
        {
            DesktopVideoDevice::requestOutputRecomposite();
        }
    }

    void MetalView::paintEvent(QPaintEvent* /*event*/)
    {
        // A presentation output shows its last frame instead of blank, then
        // asks for a real one (there is none on first expose).
        if (m_presentationMode)
        {
            if (m_lastPresentedSurface)
            {
                presentIOSurface(m_lastPresentedSurface);
            }
            DesktopVideoDevice::requestOutputRecomposite();
            return;
        }

        // WA_PaintOnScreen means Qt does not restore this surface on expose.
        // Re-present the last IOSurface (no GL readback), or render if there is none.
        if (!m_lastPresentedSurface)
        {
            m_updatePending = false;
            render();
            return;
        }

        presentIOSurface(m_lastPresentedSurface);

        // A separate presentation output has no render loop of its own.
        if (const IPCore::Session* session = m_doc ? m_doc->session() : nullptr)
        {
            syncOutputDevice(session);
        }
    }

    void MetalView::eventProcessingTimeout()
    {
        if (m_doc && m_doc->session())
        {
            m_doc->session()->userGenericEvent("per-render-event-processing", "");
        }
    }

    //--------------------------------------------------------------------------
    // Event handling, mirrors GLView::event() and GLView::eventFilter()
    //--------------------------------------------------------------------------

    bool MetalView::isRepeatedKey(const QKeyEvent* kevent)
    {
        const bool repeated = m_lastKey == kevent->key()
                              && ((m_lastKeyType == QEvent::ShortcutOverride && kevent->type() == QEvent::KeyPress)
                                  || m_lastKeyType == kevent->type());
        m_lastKey = kevent->key();
        m_lastKeyType = kevent->type();
        return repeated;
    }

    void MetalView::updateTranslatorMapping(const IPCore::Session* session)
    {
        const QTTranslator& translator = m_videoDevice->translator();
        const TwkApp::VideoDevice* odv = session ? session->outputVideoDevice() : nullptr;
        const TwkApp::VideoDevice* cdv = session ? session->controlVideoDevice() : nullptr;
        const bool mirroring =
            odv && odv->displayMode() == TwkApp::VideoDevice::MirrorDisplayMode && cdv && cdv != odv && cdv == videoDevice();

        if (!mirroring)
        {
            translator.setScaleAndOffset(0, 0, 1.0f, 1.0f);
            translator.setRelativeDomain(static_cast<float>(width()), static_cast<float>(height()));
            return;
        }

        const auto w = static_cast<float>(width());
        const auto h = static_cast<float>(height());
        const auto ow = static_cast<float>(odv->width());
        const auto oh = static_cast<float>(odv->height());

        translator.setRelativeDomain(ow, oh);

        if (w / h >= ow / oh)
        {
            const float scale = oh / h;
            translator.setScaleAndOffset(-((w * scale) - ow) / 2.0f, 0.0f, scale, scale);
        }
        else
        {
            const float scale = ow / w;
            translator.setScaleAndOffset(0.0f, -((scale * h) - oh) / 2.0f, scale, scale);
        }
    }

    float MetalView::trackActivity(const QEvent* event)
    {
        if (event->type() == QEvent::WindowActivate)
        {
            m_activationTimer.start();
        }

        float activationTime = 0.0f;
        if (m_activationTimer.isRunning())
        {
            if (event->type() == QEvent::MouseButtonPress)
            {
                activationTime = static_cast<float>(m_activationTimer.elapsed());
                m_activationTimer.stop();
            }
            if (event->type() == QEvent::MouseMove)
            {
                m_activationTimer.stop();
            }
        }

        if (event->type() != QEvent::Paint && event->type() != QEvent::UpdateRequest)
        {
            m_activityTimer.stop();
            m_activityTimer.start();

            if (!m_userActive)
            {
                const TwkApp::ActivityChangeEvent aevent("user-active", m_videoDevice.get());
                m_userActive = true;
                m_videoDevice->sendEvent(aevent);
            }
        }
        return activationTime;
    }

    bool MetalView::event(QEvent* event)
    {
        IPCore::Session* session = m_doc ? m_doc->session() : nullptr;

        if (m_stopProcessingEvents)
        {
            event->accept();
            return true;
        }

        // Qt recreates the native view when it rebuilds the top-level window,
        // e.g. when a QWebEngineView panel is added; the old view takes the
        // layer with it.
        if (event->type() == QEvent::WinIdChange)
        {
            handleNativeViewChange();
            return QWidget::event(event);
        }

        const float activationTime = trackActivity(event);

        if (const auto* kevent = dynamic_cast<const QKeyEvent*>(event))
        {
            if (isRepeatedKey(kevent))
            {
                event->accept();
                return true;
            }
        }

        switch (event->type())
        {
        case QEvent::FocusIn:
            // The translator only exists once setEventWidget() has run.
            if (m_videoDevice->hasTranslator())
            {
                m_videoDevice->translator().resetModifiers();
            }
            [[fallthrough]];
        case QEvent::Enter:
            if (m_eventWidget)
            {
                m_eventWidget->setFocus(Qt::MouseFocusReason);
            }
            break;
        default:
            break;
        }

        if (event->type() == QEvent::Resize)
        {
            const auto* e = static_cast<const QResizeEvent*>(event);
            if (!isVisible())
            {
                return true;
            }
            if (e->oldSize().width() != -1 && e->oldSize().height() != -1 && session)
            {
                std::ostringstream contents;
                contents << e->oldSize().width() << " " << e->oldSize().height() << "|" << e->size().width() << " "
                         << e->size().height();
                session->userGenericEvent("view-resized", contents.str());
            }
            return QWidget::event(event);
        }

        if (event->type() == QEvent::UpdateRequest)
        {
            m_updatePending = false;
            render();
            return true;
        }

        // Events can arrive before setEventWidget() creates the translator.
        if (!m_videoDevice->hasTranslator())
        {
            return QWidget::event(event);
        }

        updateTranslatorMapping(session);

        if (session)
        {
            session->setEventVideoDevice(videoDevice());
        }

        if (m_videoDevice->translator().sendQTEvent(event, activationTime))
        {
            event->accept();
            return true;
        }
        return QWidget::event(event);
    }

    bool MetalView::eventFilter(QObject* /*object*/, QEvent* event)
    {
        if (event->type() != QEvent::KeyPress && event->type() != QEvent::KeyRelease && event->type() != QEvent::Shortcut
            && event->type() != QEvent::ShortcutOverride)
        {
            return false;
        }

        if (const auto* kevent = dynamic_cast<const QKeyEvent*>(event))
        {
            if (isRepeatedKey(kevent))
            {
                event->accept();
                return true;
            }
        }

        IPCore::Session* session = m_doc ? m_doc->session() : nullptr;
        if (session && m_videoDevice->hasTranslator())
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

} // namespace Rv

#endif // PLATFORM_DARWIN
