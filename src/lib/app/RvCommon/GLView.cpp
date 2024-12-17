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

#include <RvCommon/GLView.h>
#include <RvCommon/QTGLVideoDevice.h>
#include <TwkGLF/GLFence.h>
#include <RvCommon/RvDocument.h>
#include <RvApp/Options.h>
#include <iostream>
#include <TwkApp/Event.h>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

#include <QtWidgets/QMenu>

namespace Rv
{
    using namespace boost;
    using namespace std;
    using namespace TwkApp;
    using namespace IPCore;

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
            SyncBufferThreadData(SyncBufferThreadData&) {}

            void operator=(SyncBufferThreadData&) {}

        private:
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
            ThreadTrampoline(GLView* view)
                : m_view(view)
            {
            }

            void operator()()
            {
                SyncBufferThreadData* closure =
                    reinterpret_cast<SyncBufferThreadData*>(
                        m_view->syncClosure());
                closure->run();
            }

        private:
            GLView* m_view;
        };

    } // namespace

    GLView::GLView(QWidget* parent, const QGLWidget* share, RvDocument* doc,
                   bool strereo, bool vsync, bool doubleBuffer, int red,
                   int green, int blue, int alpha, bool noResize)
        : QGLWidget(GLView::rvGLFormat(strereo, vsync, doubleBuffer, red, green,
                                       blue, alpha),
                    parent, share)
        , m_doc(doc)
        , m_red(red)
        , m_green(green)
        , m_blue(blue)
        , m_alpha(alpha)
        , m_lastKey(0)
        , m_lastKeyType(QEvent::None)
        , m_userActive(true)
        , m_renderCount(0)
        , m_firstPaintCompleted(false)
        , m_csize(1024, 576)
        , m_msize(128, 128)
        , m_postFirstNonEmptyRender(noResize)
        , m_stopProcessingEvents(false)
        , m_syncThreadData(0)
    {
        setObjectName((m_doc->session()) ? m_doc->session()->name().c_str()
                                         : "no session");
        // m_frameBuffer = new QTFrameBuffer( this );
        ostringstream str;
        str << UI_APPLICATION_NAME " Main Window" << "/" << m_doc;
        m_videoDevice = new QTGLVideoDevice(0, str.str(), this);
        m_activityTimer.start();
        setMouseTracking(true);
        setAcceptDrops(true);
        setFocusPolicy(Qt::StrongFocus);
        setAutoBufferSwap(false);

        m_eventProcessingTimer.setSingleShot(true);
        connect(&m_eventProcessingTimer, SIGNAL(timeout()), this,
                SLOT(eventProcessingTimeout()));

        QGLFormat f = format();
    }

    GLView::~GLView()
    {
        // delete m_frameBuffer;
        delete m_videoDevice;

        if (m_syncThreadData)
        {
            SyncBufferThreadData* closure =
                reinterpret_cast<SyncBufferThreadData*>(m_syncThreadData);
            closure->notify(true);
            m_swapThread.join();

            delete closure;
        }
    }

    void GLView::stopProcessingEvents() { m_stopProcessingEvents = true; }

    QSize GLView::sizeHint() const { return m_csize; }

    QSize GLView::minimumSizeHint() const { return m_msize; }

    void GLView::absolutePosition(int& x, int& y) const
    {
        x = 0;
        y = 0;

        QPoint p(0, 0);
        QPoint gp = mapToGlobal(p);

        x = gp.x();
        y = gp.y();
    }

    QGLFormat GLView::rvGLFormat(bool stereo, bool vsync, bool doubleBuffer,
                                 int red, int green, int blue, int alpha)
    {
        const Rv::Options& opts = Rv::Options::sharedOptions();

        QGLFormat fmt;
        fmt.setDepthBufferSize(0);
        fmt.setDoubleBuffer(doubleBuffer);
        fmt.setStencilBufferSize(8);
        fmt.setStencil(true);
        fmt.setDepth(false);
        fmt.setStereo(stereo);

        //
        //  The default value for these buffer sizes is -1, but it is
        //  illegal to set to that value so test for positive red, not
        //  just non-zero red.  If any of these values is < 0, we ignore it.
        //
        if (red > 0)
            fmt.setRedBufferSize(red);
        if (green > 0)
            fmt.setGreenBufferSize(green);
        if (blue > 0)
            fmt.setBlueBufferSize(blue);
        if (alpha >= 0)
        {
            fmt.setAlpha(alpha != 0);
            fmt.setAlphaBufferSize(alpha);
        }

        fmt.setSwapInterval(vsync ? 1 : 0);

        return fmt;
    }

    void GLView::initializeGL()
    {
        //
        //  At this point the format is known. Can't do this in the constructor
        //

        if (isValid())
        {
            QGLFormat f = context()->format();

#ifndef PLATFORM_DARWIN
            //
            //  Doesn't work on OS X
            //
            if (f.redBufferSize() != m_red && m_red != 0)
            {
                // QMessageBox box(this);
                // box.setWindowTitle(tr("Ouput Display Format"));

                ostringstream str;

                str << "WARNING: asked for"
                    << " " << m_red << " " << m_green << " " << m_blue << " "
                    << m_alpha << " RGBA color but got"
                    << " " << f.redBufferSize() << " " << f.greenBufferSize()
                    << " " << f.blueBufferSize() << " "
                    << (f.alphaBufferSize() <= 0 ? 0 : f.alphaBufferSize())
                    << " RGBA instead";

                cout << str.str() << endl;

                // box.setText(str.str().c_str());
                // box.setDetailedText("You can change the default display color
                // depth and target "
                //                     "from the preferences under
                //                     Rendering->Display Output Format.\n"
                //                     "Choosing \"OpenGL Default Format\" will
                //                     tell RV to ask for the " "default
                //                     prefered format for this display.");
                // box.setWindowModality(Qt::WindowModal);
                // QPushButton* b1 = box.addButton(tr("Continue"),
                // QMessageBox::AcceptRole); box.setIcon(QMessageBox::Critical);
                // box.exec();
            }
#endif

            if (!f.stencil())
            {
                cout << "WARNING: no stencil buffer available" << endl;
            }
        }
        else
        {
            cout << "WARNING: invalid GL context" << endl;
        }
    }

    void GLView::resizeGL(int w, int h)
    {
        if (m_doc)
            m_doc->viewSizeChanged(w, h);
#ifdef PLATFORM_WINDOWS
        SetWindowRgn(reinterpret_cast<HWND>(this->winId()), 0, false);
#endif
    }

    void GLView::swapBuffersNoSync()
    {
        glFlush();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glReadBuffer(GL_BACK);
        glDrawBuffer(GL_FRONT);
        glBlitFramebufferEXT(0, 0, width(), height(), 0, 0, width(), height(),
                             GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glDrawBuffer(GL_BACK);
    }

    void GLView::paintGL()
    {
        IPCore::Session* session = m_doc->session();
        bool debug = IPCore::debugProfile && session;

        if (!m_postFirstNonEmptyRender && session
            && session->postFirstNonEmptyRender())
        {
            m_postFirstNonEmptyRender = true;

            if (!session->isFullScreen())
            {
                m_doc->resizeToFit(false, false);
                m_doc->center();
            }
        }

        if (debug)
        {
            Session::ProfilingRecord& trecord = session->beginProfilingSample();
            trecord.renderStart = session->profilingElapsedTime();
        }

        // should be no longer necessary, moved it to resize()
#if 0
    //
    //  This is necessary to stop the Windows Desktop Window Manager
    //  (new in Vista/win7) fromc caching portions of rv's glview
    //  and holding them in the display even when rv redraws.  the
    //  effect being that parts of previous displays will be "left
    //  behind" and not updated even when rv plays.  especially when
    //  going to/from fullscreen.
    //
#ifdef PLATFORM_WINDOWS
    SetWindowRgn (this->winId(), 0, false);
#endif
#endif

        if (m_doc && session && m_videoDevice)
        {
            // m_frameBuffer->makeCurrent();
            m_videoDevice->makeCurrent();

            if (m_userActive && m_activityTimer.elapsed() > 1.0)
            {
                if (m_doc->mainPopup() && !m_doc->mainPopup()->isVisible()
                    && hasFocus())
                {
                    TwkApp::ActivityChangeEvent aevent("user-inactive",
                                                       m_videoDevice);
                    m_videoDevice->sendEvent(aevent);
                    m_userActive = false;
                }
            }

            //
            //  Make sure the video device knows where it is on screen.
            //
            int x = 0, y = 0;
            absolutePosition(x, y);
            m_videoDevice->setAbsolutePosition(x, y);

            session->render();
            m_firstPaintCompleted = true;

            // Starting with Qt 5.12.1, the resulting texture is later
            // composited over white which creates undesirable artifacts. As a
            // work around, we set the resulting alpha channel to 1 to make sure
            // that the resulting texture is fully opaque.
            glPushAttrib(GL_COLOR_BUFFER_BIT);
            TWK_GLDEBUG;
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
            TWK_GLDEBUG;
            glClearColor(0.f, 0.f, 0.f, 1.0f);
            TWK_GLDEBUG;
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            TWK_GLDEBUG;
            glPopAttrib();
            TWK_GLDEBUG;
        }
        else
        {
            glClearColor(0.f, 0.f, 0.f, 1.0f);
            TWK_GLDEBUG;
            glClear(GL_COLOR_BUFFER_BIT);
            TWK_GLDEBUG;
        }

        if (m_stopProcessingEvents)
            return;

        //
        //  We're done with drawing and about to (possibly) wait for
        //  vsync before the buffer swaps, but if the driver is layzy,
        //  it may not performs some requested gl actions until after
        //  the wait on vsync, in which case we get tearing.
        //
        //  A glFlush here should make these gl actions happen during
        //  the wait.
        //
        //  This could be a problem if further drawing is done to this
        //  buffer by Qt, for example if it's doing it's graphics area
        //  widget drawing.  Seems fine for now.
        //
        //  Update: We want to actually wait until the gfx operations are
        //  complete.  glFlush just flushes the command buffer to
        //  hardware, glFinish blocks until the hardware is finished
        //  processing the commands.
        //

        // DONT
        // glFlush();
        // glFinish();

        //
        //  Do the swap, the vsync code is in the swapBuffers() code in
        //  Qt. It should block here waiting for the refresh. The debug
        //  code here is accumulating profiling information in the session
        //  ProfilingRecord struct. This can be dumped on exit to figure out
        //  what's going on.
        //

        if (debug)
        {
            Session::ProfilingRecord& trecord =
                session->currentProfilingSample();
            trecord.renderEnd = session->profilingElapsedTime();
            trecord.swapStart = trecord.renderEnd;

            if (session->outputVideoDevice() != videoDevice())
            {
#ifdef PLATFORM_DARWIN
                session->outputVideoDevice()->syncBuffers();
                makeCurrent();
                swapBuffers();
#else
                session->outputVideoDevice()->syncBuffers();
                makeCurrent();
                swapBuffersNoSync();
#endif
            }
            else
            {
                swapBuffers();
            }

            trecord.swapEnd = session->profilingElapsedTime();
            session->endProfilingSample();
        }
        else
        {
            if (session->outputVideoDevice() != videoDevice())
            {

#ifdef PLATFORM_DARWIN
                if (!m_syncThreadData)
                {
                    m_syncThreadData =
                        new SyncBufferThreadData(session->outputVideoDevice());
                    m_swapThread = boost::thread(ThreadTrampoline(this));
                }
                else
                {
                    SyncBufferThreadData* sb =
                        reinterpret_cast<SyncBufferThreadData*>(
                            m_syncThreadData);

                    if (sb->device() != session->outputVideoDevice())
                    {
                        //
                        //  This is questionable without shutting down and
                        //  restarting the sync thread, but the worst case
                        //  seems to be an extra swap on the closed (not
                        //  visible) device
                        //

                        sb->setDevice(session->outputVideoDevice());
                    }
                }

                reinterpret_cast<SyncBufferThreadData*>(m_syncThreadData)
                    ->notify();

                // session->outputVideoDevice()->syncBuffers();

                makeCurrent();
                swapBuffers();
#else
                session->outputVideoDevice()->syncBuffers();
                makeCurrent();
                swapBuffersNoSync();
#endif
            }
            else
            {
                swapBuffers();
            }
        }

        session->addSyncSample();

        session->postRender();

        m_eventProcessingTimer.start();
    }

    void GLView::eventProcessingTimeout()
    {
        m_doc->session()->userGenericEvent("per-render-event-processing", "");
    }

    bool GLView::event(QEvent* event)
    {
        bool keyevent = false;
        Rv::Session* session = m_doc->session();

        if (m_stopProcessingEvents)
        {
            event->accept();
            return true;
        }

        //
        //  We want to exclude the "click-through" clicks on
        //  click-to-focus systems (like osX).  Turns out the event
        //  sequence in these cases is exactly the same as a
        //  tab-to-focus, then click sequence.  So the only way we have
        //  to identify the "click-through" is by how quickly the click
        //  (button push) follows the windowActivate event.
        //

        if (event->type() == QEvent::WindowActivate)
            m_activationTimer.start();

#if 0
    //
    //  This doesn't work.  Nothing I've tried will make a stylus
    //  press raise and activate the window when we are handling
    //  native tablet events.  It works fine when we are treating
    //  stylus events as mouse events.
    //
    if (event->type() == QEvent::TabletPress && !isActiveWindow())
    {
        cerr << "activating window" << endl;
        QEvent activateEvent(QEvent::WindowActivate);
        //m_frameBuffer->translator().sendQTEvent(new QEvent(QEvent::WindowActivate));
        session->setEventVideoDevice(videoDevice());
        m_videoDevice->translator().sendQTEvent(new QEvent(QEvent::WindowActivate));
        event->accept();
        /*
        activateWindow();
        raise();
        event->accept();
        */
        return true;
    }
#endif

        float activationTime = 0.0;
        if (m_activationTimer.isRunning())
        {
            if (event->type() == QEvent::MouseButtonPress)
            {
                //
                //  Pass this time through with the event, so that
                //  event handling code can decide witehr or not to ignore
                //  this 'click-through' event.
                //
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
                TwkApp::ActivityChangeEvent aevent("user-active",
                                                   m_videoDevice);

                //
                //  m_userActive set first will prevent recursive nightmare. In
                //  Qt 4.4 an event may recursively produce more
                //  events. For example, changing the cursor in 4.4 will
                //  cause a Paint event immediately (not after the
                //  previous event returns).
                //

                m_userActive = true;
                m_videoDevice->sendEvent(aevent);
            }
        }

        if (QKeyEvent* kevent = dynamic_cast<QKeyEvent*>(event))
        {
            keyevent = true;

            //
            //  Get around really annoying event bugs on Qt/Mac
            //

            if (m_lastKey == kevent->key()
                && (m_lastKeyType == QEvent::ShortcutOverride
                        && (kevent->type() == QEvent::KeyPress)
                    || (m_lastKeyType == kevent->type())))
            {
                //
                //  Qt 4.3.3 (4.5 too) will give both override and press events
                //  even if key is not in menu. Filter that here.
                //
                //  Remember the new key/type, otherwise we filter out
                //  any number of ShortcutOverride/Press pairs, and
                //  auto-repeat doesn't work.
                //
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
            m_videoDevice->translator().resetModifiers();
        case QEvent::Enter:
            setFocus(Qt::MouseFocusReason);
            break;
        default:
            break;
        }
        if (event->type() == QEvent::Resize)
        {
            QResizeEvent* e = static_cast<QResizeEvent*>(event);

            // QT5 BUG -- results in invalid drawable
            if (!isVisible())
                return true;

            if (e->oldSize().width() != -1 && e->oldSize().height() != -1)
            {
                ostringstream contents;
                contents << e->oldSize().width() << " " << e->oldSize().height()
                         << "|" << e->size().width() << " "
                         << e->size().height();

                if (m_doc && session)
                {
                    session->userGenericEvent("view-resized", contents.str());
                }
            }
            return QGLWidget::event(event);
        }

        if (session && session->outputVideoDevice()
            && session->outputVideoDevice()->displayMode()
                   == TwkApp::VideoDevice::MirrorDisplayMode)
        {
            if (const TwkApp::VideoDevice* cdv = session->controlVideoDevice())
            {
                const TwkApp::VideoDevice* odv = session->outputVideoDevice();

                if (odv && cdv != odv && cdv == videoDevice())
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
                        const float yoffset = 0.0;
                        const float xscale = yscale;
                        const float xoffset = -(w * yscale - ow) / 2.0;
                        m_videoDevice->translator().setScaleAndOffset(
                            xoffset, yoffset, xscale, yscale);
                    }
                    else
                    {
                        const float xscale = ow / w;
                        const float xoffset = 0.0;
                        const float yscale = xscale;
                        const float yoffset = -(xscale * h - oh) / 2.0;
                        m_videoDevice->translator().setScaleAndOffset(
                            xoffset, yoffset, xscale, yscale);
                    }
                }
                else
                {
                    m_videoDevice->translator().setScaleAndOffset(0, 0, 1.0,
                                                                  1.0);
                    m_videoDevice->translator().setRelativeDomain(width(),
                                                                  height());
                }
            }
            else
            {
                m_videoDevice->translator().setScaleAndOffset(0, 0, 1.0, 1.0);
                m_videoDevice->translator().setRelativeDomain(width(),
                                                              height());
            }
        }
        else
        {
            m_videoDevice->translator().setScaleAndOffset(0, 0, 1.0, 1.0);
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
            bool result = QGLWidget::event(event);

            return result;
        }
    }

    bool GLView::eventFilter(QObject* object, QEvent* event)
    {
        if (event->type() == QEvent::KeyPress
            || event->type() == QEvent::KeyRelease
            || event->type() == QEvent::Shortcut
            || event->type() == QEvent::ShortcutOverride)
        {

            //
            //  Get around really annoying event bugs on Qt/Mac
            //  (copied from GLView::event... same bug applies -lo)
            //
            //  Qt 4.3.3 (4.5 too) will give both override and press events even
            //  if key is not in menu. Filter that here.
            //
            //  Remember the new key/type, otherwise we filter out
            //  any number of ShortcutOverride/Press pairs, and
            //  auto-repeat doesn't work.
            //

            if (QKeyEvent* kevent = dynamic_cast<QKeyEvent*>(event))
            {
                if (m_lastKey == kevent->key()
                    && (m_lastKeyType == QEvent::ShortcutOverride
                            && (kevent->type() == QEvent::KeyPress)
                        || (m_lastKeyType == kevent->type())))
                {
                    m_lastKey = kevent->key();
                    m_lastKeyType = kevent->type();

                    event->accept();
                    return true;
                }

                m_lastKeyType = kevent->type();
                m_lastKey = kevent->key();
            }

            // if (m_frameBuffer->translator().sendQTEvent(event))
            Session* session = m_doc->session();
            session->setEventVideoDevice(videoDevice());
            if (m_videoDevice->translator().sendQTEvent(event))
            {

                event->accept();
                return true;
            }

            event->accept();
            return true;
        }

        return false;
    }

} // namespace Rv
