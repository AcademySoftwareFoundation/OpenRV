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
#include <RvCommon/InitGL.h>
#include <RvCommon/RvDocument.h>
#include <RvApp/Options.h>
#include <iostream>
#include <sstream>
#include <TwkApp/Event.h>
#include <TwkUtil/PlaybackDiagnostics.h>
#include <TwkUtil/Clock.h>
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
                SyncBufferThreadData* closure = reinterpret_cast<SyncBufferThreadData*>(m_view->syncClosure());
                closure->run();
            }

        private:
            GLView* m_view;
        };

    } // namespace

    GLView::GLView(QWidget* parent, QOpenGLContext* sharedContext, RvDocument* doc, bool stereo, bool vsync, bool doubleBuffer, int red,
                   int green, int blue, int alpha, bool noResize)
        : QOpenGLWidget(parent)
        , m_sharedContext(sharedContext)
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
        setFormat(rvGLFormat(stereo, vsync, doubleBuffer, red, green, blue, alpha));

        ostringstream str;
        str << UI_APPLICATION_NAME " Main Window" << "/" << m_doc;
        m_videoDevice = new QTGLVideoDevice(0, str.str(), this);

        setObjectName((m_doc->session()) ? m_doc->session()->name().c_str() : "no session");

        m_activityTimer.start();
        setMouseTracking(true);
        setAcceptDrops(true);
        setFocusPolicy(Qt::StrongFocus);

        m_eventProcessingTimer.setSingleShot(true);
        connect(&m_eventProcessingTimer, SIGNAL(timeout()), this, SLOT(eventProcessingTimeout()));
    }

    GLView::~GLView()
    {
        // delete m_frameBuffer;
        delete m_videoDevice;

        if (m_syncThreadData)
        {
            SyncBufferThreadData* closure = reinterpret_cast<SyncBufferThreadData*>(m_syncThreadData);
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

    QSurfaceFormat GLView::rvGLFormat(bool stereo, bool vsync, bool doubleBuffer, int red, int green, int blue, int alpha)
    {
        const Rv::Options& opts = Rv::Options::sharedOptions();

        // NOTE_QT6: QGLFormat into QSurfaceFormat
        // NOTE_QT6: setStencil, setDepth does not exist anymore. Trying to use
        // setDepthBufferSize and setStencilBufferSize.
        QSurfaceFormat fmt;
        fmt.setDepthBufferSize(24);
        fmt.setSwapBehavior(doubleBuffer ? QSurfaceFormat::DoubleBuffer : QSurfaceFormat::SingleBuffer);
        fmt.setStencilBufferSize(8);
        fmt.setStereo(stereo);

        fmt.setRenderableType(QSurfaceFormat::OpenGL);

        // NOTE_QT: Set to version 2.1 for now.
        fmt.setMajorVersion(2);
        fmt.setMinorVersion(1);

        // fmt.setProfile(QSurfaceFormat::CoreProfile);
        // fmt.setProfile(QSurfaceFormat::CompatibilityProfile);

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

        // QUESTION_QT6: Should we use isValid from QOpenGLWidget or directly
        // using from QOpenGLContext? NOTE_QT6: Returns true if the widget and
        // OpenGL resources, like the context, have been successfully
        // initialized.
        //           Note that the return value is always false until the widget
        //           is shown.
        // NOTE_QT6: QOpenGLContext: Returns if this context is valid, i.e. has
        // been successfully created.
        if (context()->isValid())
        {
            initializeGLExtensions();
            initializeOpenGLFunctions();

            if (m_sharedContext)
            {
                context()->setShareContext(m_sharedContext);
            }

            if (m_doc)
            {
                m_doc->initializeSession();
            }

            // NOTE_QT6: QGLFormat is deprecated. Using QSurfaceFormat now.
            QSurfaceFormat f = context()->format();

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
                    << " " << m_red << " " << m_green << " " << m_blue << " " << m_alpha << " RGBA color but got"
                    << " " << f.redBufferSize() << " " << f.greenBufferSize() << " " << f.blueBufferSize() << " "
                    << (f.alphaBufferSize() <= 0 ? 0 : f.alphaBufferSize()) << " RGBA instead";

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

    void GLView::resizeGL(int w, int h)
    {
        if (m_doc)
            m_doc->viewSizeChanged(w, h);
#ifdef PLATFORM_WINDOWS
        SetWindowRgn(reinterpret_cast<HWND>(this->winId()), 0, false);
#endif
    }

    bool GLView::validateReadPixels(int x, int y, int w, int h)
    {
        int r = x + w;
        int t = y + h;

        // are the extents of the read region out of bounds?
        if (x < 0 || y < 0 || r > width() * devicePixelRatio() || t > height() * devicePixelRatio())
            return false;

        return true;
    }

    QImage GLView::readPixels(int x, int y, int w, int h)
    {
        // If out of bounds, return an empty image.
        if (validateReadPixels(x, y, w, h) == false)
        {
            QImage image(0, 0, QImage::Format_RGBA8888);
            return image;
        }

        makeCurrent();

        QImage image(w, h, QImage::Format_RGBA8888);
        glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, image.bits());

        return image;
    }

    void GLView::debugSaveFramebuffer()
    {
        // Create a QImage with the same size as the FBO
        QImage image(width(), height(), QImage::Format_RGBA8888);
        glReadPixels(0, 0, width(), height(), GL_RGBA, GL_UNSIGNED_BYTE, image.bits());

        // image.save("/home/<username>>/<orv_folder>/fbo.png");
    }

    void GLView::paintGL()
    {
        TWK_GLDEBUG;

        IPCore::Session* session = m_doc->session();
        bool debug = IPCore::debugProfile && session;

        //  Playback present-path diagnostics. The Session-side "outsideGap"
        //  measures render_v2-end -> next render_v2-start, which lumps together
        //  the Qt composite/swapBuffers and the event-loop work (notably the
        //  per-render-event-processing handler). Here we time the whole paintGL
        //  and the gap between successive paints so the analyzer can split that
        //  bucket into swap/composite vs event-loop handlers.
        static double s_diagPaintEntry = 0.0;
        static double s_diagPrevPaintExit = 0.0;
        double diagPaintGap = 0.0;
        const bool diagOn = session && session->isPlaying() && TwkUtil::PlaybackDiagnostics::enabled();
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
        //  small while the present still stalls, the block is the compositor/
        //  present path, not the GPU.
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
            TWK_GLDEBUG;
            m_videoDevice->makeCurrent();
            TWK_GLDEBUG;

            if (m_userActive && m_activityTimer.elapsed() > 1.0)
            {
                if (m_doc->mainPopup() && !m_doc->mainPopup()->isVisible() && hasFocus())
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

            // Starting with Qt 5.12.1, the resulting texture is later
            // composited over white which creates undesirable artifacts. As a
            // work around, we set the resulting alpha channel to 1 to make sure
            // that the resulting texture is fully opaque.

            // NOTE_QT: That code seems to fix an issue on MacOS only. (Not on
            // Linux, not test on Windows)
            //          The issue I see on MacOS that the background is brown
            //          istead of black with the default background.
            //
            // Even on Qt6/QOpenGLWidget, we need to call
            // glBindFramebuffer(); it otherwise complains and fails
            // on glClear() on macOS
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, QOpenGLContext::currentContext()->defaultFramebufferObject());
            TWK_GLDEBUG;

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

        // Note for Qt6 . QOpenGLWidget implementation:
        //
        // With the new QOpenGLWidget (Qt6 branch), all of the rendering of
        // each widget is done in its own FBOs (aka: off-screen buffers), as
        // opposed to the old Qt5/QGLWidget branch where the rendering was
        // done in each GGLWidget's backbuffer (aka: GL_BACK, an on-screen
        // framebuffer surface).
        //
        // With QOpenGLWidget, it is now the WainWindow's responsibility
        // (more technically, the MainWindow's rendering backend, which
        // happens to be Qt's OpenGL rendering backend when QOpenGLWidget are
        // present in the children tree) to gather and composite all of the
        // off-screen buffers (regardless of which image type they are, eg:
        // cpu-memory images, gpu/opengl images, etc) and finally to call
        // swapBuffers to show the final contents of the mainWindow.
        //
        // As a result, I'm not sure there's a point -- at all --
        // in calling swapBuffers on the GLView anywhere amnymore. From old
        // comments in the previous version of this tile, it appears that
        // calling swapBuffers was done to force a quicker visual update, or
        // to minimize visual tearing of some sort. This would have worked
        // with the old QGLWidget (becayuse each QGLWidget had its own
        // context, and its rendering target was directly the GL_BACK
        // framebuffer, but, again, with QOpenGLWidget, the rendering target
        // is no longer GL_BACK, it is an FBO that is meant to be used at
        // the end of the application's drawing / visual update pipeline.

        if (debug)
        {
            Session::ProfilingRecord& trecord = session->currentProfilingSample();
            trecord.renderEnd = session->profilingElapsedTime();
            trecord.swapStart = trecord.renderEnd;

            if (session->outputVideoDevice() != videoDevice())
            {
                session->outputVideoDevice()->syncBuffers();
            }
            else
            {
                m_videoDevice->widget()->context()->swapBuffers(m_videoDevice->widget()->context()->surface());
            }

            trecord.swapEnd = session->profilingElapsedTime();
            session->endProfilingSample();
        }
        else
        {
            if (session->outputVideoDevice() != videoDevice())
            {
                session->outputVideoDevice()->syncBuffers();
            }
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
            //  gap   = previous paint-exit -> this paint-entry, i.e. the Qt
            //          composite + swapBuffers/vsync + event loop between paints
            extra << "paint=" << paintMs << ";gap=" << diagPaintGap << ";gpuFinish=" << diagGpuMs;
            TwkUtil::PlaybackDiagnostics::instance().record("paint", -1, session->currentFrame(), paintMs, extra.str());
        }

        m_eventProcessingTimer.start();

        TWK_GLDEBUG;
    }

    void GLView::eventProcessingTimeout()
    {
        IPCore::Session* session = m_doc->session();

        //  Time the synchronous per-render event processing (Mu/Python handlers)
        //  that runs on the GUI thread after each paint. If this is large during
        //  stalls it is the event-loop half of the "outside render_v2" time; if
        //  it is small, the stall is the composite/swapBuffers present path.
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

    bool GLView::event(QEvent* event)
    {
        // qDebug() << "Event type: " << event->type();

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
                TwkApp::ActivityChangeEvent aevent("user-active", m_videoDevice);

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
                && (m_lastKeyType == QEvent::ShortcutOverride && (kevent->type() == QEvent::KeyPress) || (m_lastKeyType == kevent->type())))
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
                contents << e->oldSize().width() << " " << e->oldSize().height() << "|" << e->size().width() << " " << e->size().height();

                if (m_doc && session)
                {
                    session->userGenericEvent("view-resized", contents.str());
                }
            }
            return QOpenGLWidget::event(event);
        }

        if (session && session->outputVideoDevice()
            && session->outputVideoDevice()->displayMode() == TwkApp::VideoDevice::MirrorDisplayMode)
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
                        m_videoDevice->translator().setScaleAndOffset(xoffset, yoffset, xscale, yscale);
                    }
                    else
                    {
                        const float xscale = ow / w;
                        const float xoffset = 0.0;
                        const float yscale = xscale;
                        const float yoffset = -(xscale * h - oh) / 2.0;
                        m_videoDevice->translator().setScaleAndOffset(xoffset, yoffset, xscale, yscale);
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
            session->setEventVideoDevice(videoDevice());

        if (m_videoDevice->translator().sendQTEvent(event, activationTime))
        {
            event->accept();
            return true;
        }
        else
        {
            bool result = QOpenGLWidget::event(event);

            return result;
        }
    }

    bool GLView::eventFilter(QObject* object, QEvent* event)
    {
        if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease || event->type() == QEvent::Shortcut
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
                    && (m_lastKeyType == QEvent::ShortcutOverride && (kevent->type() == QEvent::KeyPress)
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

    float GLView::devicePixelRatio() const { return videoDevice() ? videoDevice()->devicePixelRatio() : 1.0f; }

} // namespace Rv
