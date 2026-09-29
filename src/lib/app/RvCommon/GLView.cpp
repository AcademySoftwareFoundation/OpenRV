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
#include <RvCommon/GLWindow.h>
#include <RvCommon/QTGLVideoDevice.h>
#include <RvCommon/RvDocument.h>
#include <RvApp/Options.h>
#include <IPCore/Session.h>
#include <IPCore/ImageRenderer.h>
#include <QtWidgets/QVBoxLayout>
#include <QOpenGLContext>
#include <QTimer>
#include <iostream>
#include <sstream>
#include <cstdlib>

namespace Rv
{
    using namespace std;

    std::string glDebugEnvOrUnset(const char* name)
    {
        const char* value = std::getenv(name);
        return value ? value : "<unset>";
    }

    std::string glDebugFormatSummary(const QSurfaceFormat& f)
    {
        ostringstream out;
        out << "rgba " << f.redBufferSize() << " " << f.greenBufferSize() << " " << f.blueBufferSize() << " "
            << (f.alphaBufferSize() <= 0 ? 0 : f.alphaBufferSize());
        out << ", depth " << f.depthBufferSize() << ", stencil " << f.stencilBufferSize();
        out << ", swapInterval " << f.swapInterval();
        out << ", stereo " << (f.stereo() ? "true" : "false");
        out << ", major.minor " << f.majorVersion() << "." << f.minorVersion();
        return out.str();
    }

    GLView::GLView(QWidget* parent, QOpenGLContext* sharedContext, RvDocument* doc, bool stereo, bool vsync, bool doubleBuffer, int red,
                   int green, int blue, int alpha, bool noResize)
        : QWidget(parent)
        , m_doc(doc)
        , m_glWindow(nullptr)
        , m_container(0)
        , m_videoDevice(0)
        , m_csize(1024, 576)
        , m_msize(128, 128)
        , m_sharedContext(sharedContext)
        , m_watchedParentWindow(nullptr)
        , m_reattachPending(false)
    {
        //
        //  Native GL viewport window (renders + presents on its own surface).
        //
        m_glWindow = new GLWindow(sharedContext, doc, stereo, vsync, doubleBuffer, red, green, blue, alpha, noResize);

        //
        //  Embed the native window in the widget tree. The container is a
        //  normal QWidget; there is no QOpenGLWidget in the window, so the
        //  top-level window is not forced onto the OpenGL RHI backend.
        //
        m_container = QWidget::createWindowContainer(m_glWindow, this);
        m_container->setFocusPolicy(Qt::StrongFocus);

        //
        //  Create the native platform surface up-front. Unlike a QOpenGLWidget
        //  (which renders offscreen to an FBO), a QOpenGLWindow has no GL
        //  context until its window surface exists; RV performs GL setup
        //  (makeCurrent + capability queries) during startup before the window
        //  is shown, so the surface must exist by then.
        //
        m_glWindow->create();

        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(m_container);

        //
        //  Last-resort guard: if the viewport window is destroyed anyway (i.e.
        //  detaching it in parentWindowDestroyed() did not get there first),
        //  make sure nothing here is left holding it.
        //
        connect(m_glWindow, &QObject::destroyed, this, [this]() { m_glWindow = nullptr; });

        //
        //  The device drives the GL surface (the window) for rendering, and
        //  uses the container QWidget for event / coordinate translation
        //  (height-based y-flip, mapToGlobal, mouse grab).
        //
        ostringstream str;
        str << UI_APPLICATION_NAME " Main Window" << "/" << m_doc;
        m_videoDevice = new QTGLVideoDevice(0, str.str(), m_glWindow, m_container);
        m_glWindow->setVideoDevice(m_videoDevice);

        setObjectName((m_doc->session()) ? m_doc->session()->name().c_str() : "no session");
        setFocusProxy(m_container);

        //
        //  Realize the top-level's window now so its destruction can be hooked
        //  before anything else gets the chance to trigger it. Qt would create
        //  it on the first show anyway.
        //
        //
        //  Realize the top-level's window now, and watch for Qt replacing it.
        //
        //  Creating it here has a cost that is worth stating plainly: before any
        //  render-to-texture widget is in the tree, the window's composition gets
        //  pinned to OpenGL. Qt Quick defaults to Direct3D 11 on Windows, so
        //  every QQuickWidget in the window -- which is what a QWebEngineView's
        //  page is -- would then fail to get a QRhi and render nothing. RV
        //  therefore also pins Qt Quick to OpenGL (see main.cpp), which is the
        //  same pairing the QOpenGLWidget-based viewport produced before this
        //  branch.
        //
        //  It is done up front because the container can only be detached safely
        //  once a teardown has already happened at a benign point; arming later
        //  puts the first teardown at the dangerous one, which faults inside
        //  Qt's focus handling. See parentWindowDestroyed().
        //
        if (QWidget* topLevel = window())
            topLevel->createWinId();

        watchParentWindow();
    }

    GLView::~GLView()
    {
        //
        //  Two things have to be undone before the device goes away, both of
        //  them consequences of the viewport window outliving the widget tree in
        //  the detached state (see parentWindowDestroyed()).
        //
        //  The window holds a raw back-pointer to the device and would keep
        //  using it -- GLWindow::event() and paintGL() both dereference it -- so
        //  clear that first. And while detached the container has no parent
        //  widget, so it would not be destroyed along with this widget: it would
        //  survive as a stray top-level owning the viewport window, still
        //  pointing at a deleted device.
        //
        if (m_glWindow)
            m_glWindow->setVideoDevice(nullptr);

        if (m_container && !m_container->parentWidget())
        {
            delete m_container;
            m_container = nullptr;
        }

        delete m_videoDevice;
    }

    void GLView::showEvent(QShowEvent* event)
    {
        QWidget::showEvent(event);

        //
        //  The container parents the viewport window to the top-level window
        //  while being shown, so the parent to watch only becomes known here --
        //  and one turn of the event loop later, since the container's own show
        //  is nested inside this one.
        //
        watchParentWindow();
        QTimer::singleShot(0, this, &GLView::watchParentWindow);
    }

    void GLView::watchParentWindow()
    {
        //
        //  Watch the top-level widget's window rather than the viewport window's
        //  current parent: it is the object Qt destroys, and it is knowable
        //  before the container gets around to re-parenting the viewport into
        //  it. RV shows the document and runs its session initialisation (where
        //  a plugin can create the QWebEngineView that triggers all this) within
        //  a single call stack, so there is no turn of the event loop in which a
        //  deferred hook could be installed.
        //
        QWidget* topLevel = window();
        QWindow* topLevelWindow = topLevel ? topLevel->windowHandle() : nullptr;

        if (topLevelWindow == m_watchedParentWindow)
            return;

        if (m_watchedParentConnection)
            disconnect(m_watchedParentConnection);

        m_watchedParentWindow = topLevelWindow;

        if (topLevelWindow)
            m_watchedParentConnection = connect(topLevelWindow, &QObject::destroyed, this, &GLView::parentWindowDestroyed);
    }

    void GLView::parentWindowDestroyed()
    {
        //
        //  Emitted at the top of the window's ~QObject, before it deletes its
        //  children, so detaching here is what saves the viewport from being
        //  deleted along with it. The window becomes parentless for the moment;
        //  it is hidden so it cannot flash on screen as a stray top-level, and
        //  re-attached once the top-level has its new window.
        //
        QWindow* destroyedWindow = m_watchedParentWindow;
        m_watchedParentWindow = nullptr;

        //
        //  Only the viewport's actual parent matters. Before the container has
        //  re-parented it, the viewport still belongs to QWindowContainer's
        //  internal placeholder parent, and pulling it off that would break the
        //  container's own bookkeeping.
        //
        if (m_glWindow && m_glWindow->parent() == destroyedWindow)
        {
            m_glWindow->hide();
            m_glWindow->setParent(nullptr);
        }

        //
        //  Take the container out of the widget tree for the duration as well.
        //  Qt reaches window containers through QWindowContainer::parentWasMoved()
        //  on every layout pass and dereferences the top-level's windowHandle()
        //  without checking it -- and that is null from here until Qt recreates
        //  the window, which it does lazily on the next show. A layout pass runs
        //  before then. A container that is not in the tree is never visited.
        //
        //
        //  Take the container out of the widget tree for the duration as well.
        //  Qt reaches window containers through QWindowContainer::parentWasMoved()
        //  on every layout pass and dereferences the top-level's windowHandle()
        //  without checking it -- and that is null from here until Qt recreates
        //  the window. A layout pass runs before then, inside this same reparent,
        //  so a container left in the tree faults there.
        //
        //  This is the fragile part of the workaround: reparenting a widget from
        //  inside Qt's window teardown runs its focus machinery against the
        //  half-destroyed QWidgetWindow. It is safe here only because the window
        //  is realized up front (see the constructor), which means the first of
        //  these teardowns happens at a benign point and later ones find the
        //  container already detached. See the note in the constructor.
        //
        if (m_container)
        {
            if (layout())
                layout()->removeWidget(m_container);

            m_container->hide();
            m_container->setParent(nullptr);
        }

        if (m_reattachPending)
            return;

        m_reattachPending = true;
        QTimer::singleShot(0, this, &GLView::reattachGLWindow);
    }

    void GLView::reattachGLWindow()
    {
        m_reattachPending = false;

        if (!m_glWindow)
            return;

        QWidget* topLevel = window();
        QWindow* topLevelWindow = topLevel ? topLevel->windowHandle() : nullptr;

        if (!topLevelWindow)
        {
            //
            //  Qt recreates the top-level's window lazily (on the next show), so
            //  keep waiting rather than forcing it here.
            //
            m_reattachPending = true;
            QTimer::singleShot(0, this, &GLView::reattachGLWindow);
            return;
        }

        //
        //  Put the container back first: re-parenting it makes QWindowContainer
        //  re-adopt the viewport window into the new top-level window itself.
        //
        if (m_container)
        {
            m_container->setParent(this);

            if (layout())
                layout()->addWidget(m_container);

            m_container->show();
            setFocusProxy(m_container);
        }

        if (m_glWindow->parent() != topLevelWindow)
            m_glWindow->setParent(topLevelWindow);

        m_glWindow->show();

        watchParentWindow();

        //
        //  The container drives the viewport's geometry from its own, so nudge a
        //  layout pass to put the re-attached window back in place.
        //
        if (m_container)
        {
            m_container->updateGeometry();

            if (layout())
                layout()->activate();
        }

        if (m_doc && m_doc->session())
            m_doc->session()->askForRedraw();
    }

    QOpenGLContext* GLView::context() const { return m_glWindow ? m_glWindow->context() : nullptr; }

    QSurfaceFormat GLView::format() const { return m_glWindow ? m_glWindow->format() : QSurfaceFormat(); }

    void GLView::makeCurrent()
    {
        // Route through the device, which guards on context validity. Unlike a
        // QOpenGLWidget (which can render to its FBO before being shown), a
        // QOpenGLWindow has no GL context until it is created/exposed, so a
        // direct makeCurrent() here can deref a null context during startup.
        if (m_videoDevice)
            m_videoDevice->makeCurrent();
    }

    bool GLView::isValid() const { return m_glWindow != nullptr; }

    void GLView::absolutePosition(int& x, int& y) const
    {
        x = 0;
        y = 0;

        if (m_glWindow)
            m_glWindow->absolutePosition(x, y);
    }

    void GLView::stopProcessingEvents()
    {
        if (m_glWindow)
            m_glWindow->stopProcessingEvents();
    }

    bool GLView::firstPaintCompleted() const { return m_glWindow && m_glWindow->firstPaintCompleted(); }

    QSize GLView::sizeHint() const { return m_csize; }

    QSize GLView::minimumSizeHint() const { return m_msize; }

    QImage GLView::readPixels(int x, int y, int w, int h)
    {
        return m_glWindow ? m_glWindow->readPixels(x, y, w, h) : QImage(0, 0, QImage::Format_RGBA8888);
    }

    float GLView::devicePixelRatio() const { return videoDevice() ? videoDevice()->devicePixelRatio() : 1.0f; }

    QSurfaceFormat GLView::rvGLFormat(bool stereo, bool vsync, bool doubleBuffer, int red, int green, int blue, int alpha)
    {
        // NOTE_QT6: QGLFormat into QSurfaceFormat
        QSurfaceFormat fmt;
        fmt.setDepthBufferSize(24);
        fmt.setSwapBehavior(doubleBuffer ? QSurfaceFormat::DoubleBuffer : QSurfaceFormat::SingleBuffer);
        fmt.setStencilBufferSize(8);
        fmt.setStereo(stereo);

        fmt.setRenderableType(QSurfaceFormat::OpenGL);

        // NOTE_QT: Set to version 2.1 for now.
        fmt.setMajorVersion(2);
        fmt.setMinorVersion(1);

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

        if (IPCore::ImageRenderer::debugGpu())
        {
            cout << "INFO: GLView requested QSurfaceFormat: " << glDebugFormatSummary(fmt) << endl;
        }

        return fmt;
    }

} // namespace Rv
