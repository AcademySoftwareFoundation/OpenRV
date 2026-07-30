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
#include <QtWidgets/QVBoxLayout>
#include <QOpenGLContext>
#include <iostream>
#include <sstream>

namespace Rv
{
    using namespace std;

    GLView::GLView(QWidget* parent, QOpenGLContext* sharedContext, RvDocument* doc, bool stereo, bool vsync, bool doubleBuffer, int red,
                   int green, int blue, int alpha, bool noResize)
        : QWidget(parent)
        , m_doc(doc)
        , m_container(0)
        , m_videoDevice(0)
        , m_csize(1024, 576)
        , m_msize(128, 128)
        , m_sharedContext(sharedContext)
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
    }

    GLView::~GLView() { delete m_videoDevice; }

    QOpenGLContext* GLView::context() const { return m_glWindow->context(); }

    QSurfaceFormat GLView::format() const { return m_glWindow->format(); }

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

    void GLView::absolutePosition(int& x, int& y) const { m_glWindow->absolutePosition(x, y); }

    void GLView::stopProcessingEvents() { m_glWindow->stopProcessingEvents(); }

    bool GLView::firstPaintCompleted() const { return m_glWindow->firstPaintCompleted(); }

    QSize GLView::sizeHint() const { return m_csize; }

    QSize GLView::minimumSizeHint() const { return m_msize; }

    QImage GLView::readPixels(int x, int y, int w, int h) { return m_glWindow->readPixels(x, y, w, h); }

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

        return fmt;
    }

} // namespace Rv
