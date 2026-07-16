//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifdef PLATFORM_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#endif

#include <RvCommon/GLWindow.h>
#include <RvCommon/GLView.h>
#include <RvCommon/QTGLVideoDevice.h>
#include <RvCommon/InitGL.h>
#include <RvCommon/RvDocument.h>
#include <RvApp/Options.h>
#include <IPCore/Session.h>
#include <TwkApp/Event.h>
#include <TwkApp/VideoDevice.h>
#include <TwkGLF/GLVideoDevice.h>
#include <TwkUtil/PlaybackDiagnostics.h>
#include <TwkUtil/Clock.h>
#include <QOpenGLContext>
#include <QtGui/QGuiApplication>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QtWidgets/QMenu>
#include <cstdlib>
#include <iostream>
#include <sstream>

namespace Rv
{
    using namespace std;
    using namespace TwkApp;
    using namespace IPCore;

    GLWindow::GLWindow(QOpenGLContext* sharedContext, RvDocument* doc, bool stereo, bool vsync, bool doubleBuffer, int red, int green,
                       int blue, int alpha, bool noResize)
        : QOpenGLWindow(QOpenGLWindow::NoPartialUpdate)
        , m_doc(doc)
        , m_red(red)
        , m_green(green)
        , m_blue(blue)
        , m_alpha(alpha)
        , m_lastKey(0)
        , m_lastKeyType(QEvent::None)
        , m_userActive(true)
        , m_firstPaintCompleted(false)
        , m_postFirstNonEmptyRender(noResize)
        , m_stopProcessingEvents(false)
        , m_sharedContext(sharedContext)
    {
        setFormat(GLView::rvGLFormat(stereo, vsync, doubleBuffer, red, green, blue, alpha));

        m_videoDevice = nullptr; // set later by the hosting GLView

        m_activityTimer.start();

        m_eventProcessingTimer.setSingleShot(true);
        connect(&m_eventProcessingTimer, SIGNAL(timeout()), this, SLOT(eventProcessingTimeout()));
    }

    GLWindow::~GLWindow() {}

    void GLWindow::stopProcessingEvents() { m_stopProcessingEvents = true; }

    void GLWindow::eventProcessingTimeout()
    {
        IPCore::Session* session = m_doc->session();

        //  Time the synchronous per-render event processing (Mu/Python handlers)
        //  that runs on the GUI thread after each paint. If this is large during
        //  stalls it is the event-loop half of the "outside render_v2" time; if
        //  it is small, the stall is in the present path (swapBuffers/vsync or
        //  update-request delivery) rather than in a handler.
        if (session && session->isPlaying() && TwkUtil::PlaybackDiagnostics::enabled())
        {
            const double t0 = TwkUtil::SystemClock().now();
            session->userGenericEvent("per-render-event-processing", "");
            const double perRenderMs = (TwkUtil::SystemClock().now() - t0) * 1000.0;
            TwkUtil::PlaybackDiagnostics::instance().record("perrender", -1, session->currentFrame(), perRenderMs);
        }
        else if (session)
        {
            session->userGenericEvent("per-render-event-processing", "");
        }
    }

    float GLWindow::devicePixelRatioF() const { return static_cast<float>(devicePixelRatio()); }

    void GLWindow::absolutePosition(int& x, int& y) const
    {
        QPoint gp = mapToGlobal(QPoint(0, 0));
        x = gp.x();
        y = gp.y();
    }

    void GLWindow::initializeGL()
    {
        if (context()->isValid())
        {
            initializeGLExtensions();
            initializeOpenGLFunctions();

            if (m_sharedContext)
            {
                context()->setShareContext(m_sharedContext);
            }

            //
            //  NOTE: session initialization is deliberately NOT driven from
            //  here. Loading packages creates web panels, and adding a
            //  QWebEngineView makes Qt tear down the main window's native
            //  subtree -- destroying this very window while this method is
            //  still on the stack, so every later member access is a
            //  use-after-free. RvApplication::newSessionFromFiles() calls
            //  RvDocument::initializeSession() after show() instead, with no
            //  GL callback in the call chain.
            //

            QSurfaceFormat f = context()->format();

#ifndef PLATFORM_DARWIN
            if (f.redBufferSize() != m_red && m_red != 0)
            {
                ostringstream str;
                str << "WARNING: asked for"
                    << " " << m_red << " " << m_green << " " << m_blue << " " << m_alpha << " RGBA color but got"
                    << " " << f.redBufferSize() << " " << f.greenBufferSize() << " " << f.blueBufferSize() << " "
                    << (f.alphaBufferSize() <= 0 ? 0 : f.alphaBufferSize()) << " RGBA instead";
                cout << str.str() << endl;
            }
#endif
            if (f.stencilBufferSize() == 0)
            {
                cout << "WARNING: no stencil buffer available" << endl;
            }
        }
        else
        {
            cout << "WARNING: invalid GL context" << endl;
        }
    }

    void GLWindow::resizeGL(int w, int h)
    {
        if (m_doc)
            m_doc->viewSizeChanged(w, h);
    }

    QImage GLWindow::readPixels(int x, int y, int w, int h)
    {
        const int pw = width() * devicePixelRatio();
        const int ph = height() * devicePixelRatio();

        // If out of bounds, return an empty image.
        if (x < 0 || y < 0 || (x + w) > pw || (y + h) > ph)
            return QImage(0, 0, QImage::Format_RGBA8888);

        if (m_videoDevice)
            m_videoDevice->makeCurrent();
        else
            makeCurrent();

        QImage image(w, h, QImage::Format_RGBA8888);
        glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, image.bits());

        return image;
    }

    void GLWindow::paintGL()
    {
        TWK_GLDEBUG;

        IPCore::Session* session = m_doc->session();

        //  Playback present-path diagnostics. The Session-side "outsideGap"
        //  measures render_v2-end -> next render_v2-start, which lumps together
        //  the present and the event-loop work (notably the
        //  per-render-event-processing handler). Here we time the whole paintGL
        //  and the gap between successive paints so the analyzer can split that
        //  bucket into present vs event-loop handlers.
        //
        //  Note the viewport is a QOpenGLWindow presenting on its own native
        //  surface, so unlike the old QOpenGLWidget path the full-window widget
        //  composite is NOT part of the gap -- see the "gap" note below.
        //
        //  m_videoDevice is null until the hosting GLView assigns it after
        //  construction, and the window is created during that construction, so
        //  a paint can land before the device is wired up. Require it here so we
        //  never record a paint that did no rendering.
        static double s_diagPaintEntry = 0.0;
        static double s_diagPrevPaintExit = 0.0;
        double diagPaintGap = 0.0;
        const bool diagOn = session && m_videoDevice && session->isPlaying() && TwkUtil::PlaybackDiagnostics::enabled();
        if (diagOn)
        {
            s_diagPaintEntry = TwkUtil::SystemClock().now();
            if (s_diagPrevPaintExit > 0.0)
                diagPaintGap = (s_diagPaintEntry - s_diagPrevPaintExit) * 1000.0;
        }

        //  Optional GPU-completion probe (RV_DIAG_GLFINISH). session->render()
        //  only submits GL commands (texture upload + shaders); the GPU runs
        //  them asynchronously and the present later blocks until they finish.
        //  glFinish() here attributes that GPU time: if it is large on new
        //  frames the stall is the synchronous GPU upload/render; if it stays
        //  small while the present still stalls, the block is the present path
        //  (swapBuffers/vsync or update-request delivery), not the GPU.
        static int s_diagGlFinish = -1;
        if (s_diagGlFinish < 0)
            s_diagGlFinish = (getenv("RV_DIAG_GLFINISH") != nullptr) ? 1 : 0;
        double diagGpuMs = -1.0;

        if (!m_postFirstNonEmptyRender && session && session->postFirstNonEmptyRender())
        {
            m_postFirstNonEmptyRender = true;

            if (!session->isFullScreen())
            {
                m_doc->resizeToFit(false, false);
                m_doc->center();
                TWK_GLDEBUG;
            }
        }

        if (m_doc && session && m_videoDevice)
        {
            m_videoDevice->makeCurrent();
            TWK_GLDEBUG;

            if (m_userActive && m_activityTimer.elapsed() > 1.0)
            {
                if (m_doc->mainPopup() && !m_doc->mainPopup()->isVisible())
                {
                    TwkApp::ActivityChangeEvent aevent("user-inactive", m_videoDevice);
                    m_videoDevice->sendEvent(aevent);
                    TWK_GLDEBUG;
                    m_userActive = false;
                }
            }

            //
            //  Make sure the video device knows where it is on screen.
            //
            int x = 0, y = 0;
            absolutePosition(x, y);
            m_videoDevice->setAbsolutePosition(x, y);

            TWK_GLDEBUG;
            session->render();
            TWK_GLDEBUG;

            if (diagOn && s_diagGlFinish)
            {
                const double t0 = TwkUtil::SystemClock().now();
                glFinish();
                diagGpuMs = (TwkUtil::SystemClock().now() - t0) * 1000.0;
            }

            m_firstPaintCompleted = true;

            // Force the resulting alpha channel to 1 so the surface is fully
            // opaque (matches the former GLView behavior).
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, QOpenGLContext::currentContext()->defaultFramebufferObject());
            TWK_GLDEBUG;
            glPushAttrib(GL_COLOR_BUFFER_BIT);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
            glClearColor(0.f, 0.f, 0.f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glPopAttrib();
            TWK_GLDEBUG;
        }
        else
        {
            glClearColor(0.f, 0.f, 0.f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            TWK_GLDEBUG;
        }

        if (m_stopProcessingEvents)
        {
            //  This path skips the "paint" record below, so drop the exit
            //  timestamp too. Leaving it stale would make the next computed
            //  gap span everything that happened in between.
            s_diagPrevPaintExit = 0.0;
            return;
        }

        // If a separate output device is presenting, sync it. The control
        // (window) surface presents itself: QOpenGLWindow swaps automatically
        // after paintGL returns.
        if (session->outputVideoDevice() != m_videoDevice)
        {
            session->outputVideoDevice()->syncBuffers();
        }

        session->addSyncSample();
        session->postRender();

        if (diagOn)
        {
            const double nowSecs = TwkUtil::SystemClock().now();
            const double paintMs = (nowSecs - s_diagPaintEntry) * 1000.0;
            s_diagPrevPaintExit = nowSecs;
            std::ostringstream extra;
            //  paint = whole paintGL (render_v2 + glClear tail + postRender)
            //  gap   = previous paint-exit -> this paint-entry. The viewport is a
            //          QOpenGLWindow, which swaps after paintGL returns, so this
            //          covers swapBuffers/vsync + platform update-request
            //          delivery + the event loop between paints. It does NOT
            //          include a full-window widget composite: the viewport has
            //          its own native surface and no longer serializes with the
            //          rest of the window. Do not compare these numbers against
            //          gaps captured on the pre-QOpenGLWindow present path.
            extra << "paint=" << paintMs << ";gap=" << diagPaintGap << ";gpuFinish=" << diagGpuMs;
            TwkUtil::PlaybackDiagnostics::instance().record("paint", -1, session->currentFrame(), paintMs, extra.str());
        }

        m_eventProcessingTimer.start();

        TWK_GLDEBUG;
    }

    bool GLWindow::event(QEvent* event)
    {
        // The device (and its translator) is wired by the hosting GLView just
        // after construction; ignore any events that arrive before then.
        if (!m_videoDevice)
            return QOpenGLWindow::event(event);

        bool keyevent = false;
        Rv::Session* session = m_doc->session();

        if (m_stopProcessingEvents)
        {
            event->accept();
            return true;
        }

        if (event->type() == QEvent::WindowActivate)
            m_activationTimer.start();

        float activationTime = 0.0;
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

        if (event->type() != QEvent::Paint && event->type() != QEvent::UpdateRequest)
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
                && (m_lastKeyType == QEvent::ShortcutOverride && (kevent->type() == QEvent::KeyPress) || (m_lastKeyType == kevent->type())))
            {
                m_lastKey = kevent->key();
                m_lastKeyType = kevent->type();
                event->accept();
                return true;
            }

            m_lastKeyType = kevent->type();
            m_lastKey = kevent->key();
        }

        switch (event->type())
        {
        case QEvent::FocusIn:
            //
            //  Qt has already made this the focus window by the time FocusIn is
            //  delivered, so there is nothing to activate here. The case exists
            //  only to drop modifier state that went stale while the keyboard
            //  was elsewhere. (The missing break let execution fall into the
            //  hover case below.)
            //
            m_videoDevice->translator().resetModifiers();
            break;

        case QEvent::Enter:
            //
            //  Hovering hands the keyboard to the viewport as a *widget* focus
            //  change, never as a window activation. QWidget::setFocus() only
            //  delivers FocusIn when the top-level is already active; otherwise
            //  it just records the window's focus_child and the keyboard arrives
            //  once the user activates RV. That is what keeps a hover from
            //  stealing activation from another top-level of ours (the Console)
            //  or from another application: requestActivate() on a native child
            //  window activates the whole top-level, and on Windows it will even
            //  AttachThreadInput/SetForegroundWindow when RV is not the active
            //  application.
            //
            //  The container is GLView's focus proxy, so setFocus() on the view
            //  lands on it, and QWindowContainer turns its FocusIn into the
            //  QWindow::requestActivate() that hands the keyboard to this
            //  window -- the same path RvDocument and the Mu commands use.
            //
            //  Skipped when this window already holds focus: QWindowContainer
            //  clears the container's widget focus once it has handed focus
            //  over, so a repeat FocusIn would take its "return to the normal
            //  focus chain" branch and push the keyboard to the next widget in
            //  the tab chain instead. QWidget::setFocus()'s own focusWidget()
            //  early-out did this for us on the widget-based viewport.
            //
            if (QGuiApplication::focusWindow() != this)
            {
                if (GLView* view = m_doc ? m_doc->view() : nullptr)
                    view->setFocus(Qt::MouseFocusReason);
            }
            break;

        default:
            break;
        }

        if (session && session->outputVideoDevice()
            && session->outputVideoDevice()->displayMode() == TwkApp::VideoDevice::MirrorDisplayMode)
        {
            if (const TwkApp::VideoDevice* cdv = session->controlVideoDevice())
            {
                const TwkApp::VideoDevice* odv = session->outputVideoDevice();

                if (odv && cdv != odv && cdv == m_videoDevice)
                {
                    const float w = width();
                    const float h = height();
                    const float ow = odv->width();
                    const float oh = odv->height();

                    const float aspect = w / h;
                    const float oaspect = ow / oh;

                    m_videoDevice->translator().setRelativeDomain(ow, oh);

                    if (aspect >= oaspect)
                    {
                        const float yscale = oh / h;
                        const float xscale = yscale;
                        const float xoffset = -(w * yscale - ow) / 2.0;
                        m_videoDevice->translator().setScaleAndOffset(xoffset, 0.0, xscale, yscale);
                    }
                    else
                    {
                        const float xscale = ow / w;
                        const float yscale = xscale;
                        const float yoffset = -(xscale * h - oh) / 2.0;
                        m_videoDevice->translator().setScaleAndOffset(0.0, yoffset, xscale, yscale);
                    }
                }
                else
                {
                    m_videoDevice->translator().setScaleAndOffset(0, 0, 1.0, 1.0);
                    m_videoDevice->translator().setRelativeDomain(width(), height());
                }
            }
            else
            {
                m_videoDevice->translator().setScaleAndOffset(0, 0, 1.0, 1.0);
                m_videoDevice->translator().setRelativeDomain(width(), height());
            }
        }
        else
        {
            m_videoDevice->translator().setScaleAndOffset(0, 0, 1.0, 1.0);
            m_videoDevice->translator().setRelativeDomain(width(), height());
        }

        if (session)
            session->setEventVideoDevice(m_videoDevice);

        if (m_videoDevice->translator().sendQTEvent(event, activationTime))
        {
            event->accept();
            return true;
        }

        return QOpenGLWindow::event(event);
    }

} // namespace Rv
