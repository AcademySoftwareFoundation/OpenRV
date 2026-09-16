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
#include <QOpenGLContext>
#include <QtGui/QGuiApplication>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QtWidgets/QMenu>
#include <iostream>
#include <sstream>

namespace Rv
{
    using namespace std;
    using namespace TwkApp;
    using namespace IPCore;

    GLWindow::GLWindow(QOpenGLContext* sharedContext, RvDocument* doc, bool stereo, bool vsync, bool doubleBuffer, int red, int green,
                       int blue, int alpha, bool noResize)
        //
        //  The share context has to be handed to QOpenGLWindow here: Qt creates
        //  the window's QOpenGLContext inside QOpenGLWindowPrivate::initialize()
        //  and calls setShareContext() on it *before* create(). Setting it after
        //  the fact, which is all a later context()->setShareContext() call
        //  could do, is documented as having no effect, so the viewport would
        //  silently end up in a share group of its own.
        //
        : QOpenGLWindow(sharedContext, QOpenGLWindow::NoPartialUpdate)
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
    {
        setFormat(GLView::rvGLFormat(stereo, vsync, doubleBuffer, red, green, blue, alpha));

        m_videoDevice = nullptr; // set later by the hosting GLView

        m_activityTimer.start();

        m_eventProcessingTimer.setSingleShot(true);
        connect(&m_eventProcessingTimer, SIGNAL(timeout()), this, SLOT(eventProcessingTimeout()));
    }

    GLWindow::~GLWindow() {}

    void GLWindow::stopProcessingEvents() { m_stopProcessingEvents = true; }

    void GLWindow::eventProcessingTimeout() { m_doc->session()->userGenericEvent("per-render-event-processing", ""); }

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
            return;

        // If a separate output device is presenting, sync it. The control
        // (window) surface presents itself: QOpenGLWindow swaps automatically
        // after paintGL returns.
        if (session->outputVideoDevice() != m_videoDevice)
        {
            session->outputVideoDevice()->syncBuffers();
        }

        session->addSyncSample();
        session->postRender();

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
