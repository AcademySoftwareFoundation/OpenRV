//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __rv_qt__GLView__h__
#define __rv_qt__GLView__h__
#include <TwkGLF/GL.h>
#include <QtWidgets/QWidget>
#include <QSurfaceFormat>
#include <QImage>
#include <QSize>
#include <string>

class QOpenGLContext;
class QWindow;

namespace Rv
{
    class RvDocument;
    class QTGLVideoDevice;
    class GLWindow;

    //
    //  -debug gpu (ImageRenderer::debugGpu()) diagnostics helpers, shared by
    //  GLView (which logs the format it asks for) and GLWindow (which logs the
    //  format and driver it actually got). Defined in GLView.cpp.
    //
    std::string glDebugEnvOrUnset(const char* name);
    std::string glDebugFormatSummary(const QSurfaceFormat&);

    //
    //  GLView
    //
    //  Host QWidget that embeds the native GL viewport (GLWindow) via
    //  QWidget::createWindowContainer(). GLView keeps the public API the rest
    //  of RV expects -- view()->context()/format()/makeCurrent()/videoDevice()
    //  etc. -- by delegating to the GLWindow.
    //
    //  Crucially, GLView is no longer a QOpenGLWidget. That keeps the top-level
    //  QMainWindow off the OpenGL RHI backend (so docked QWebEngineView panels
    //  render on the platform default backend), and lets the viewport present
    //  on its own native surface instead of via the full-window widget
    //  composite.
    //

    class GLView : public QWidget
    {
        Q_OBJECT

    public:
        GLView(QWidget* parent, QOpenGLContext* sharedContext, RvDocument* doc, bool stereo = false, bool vsync = true,
               bool doubleBuffer = true, int red = 0, int green = 0, int blue = 0, int alpha = 0, bool noResize = true);
        ~GLView() override;

        static QSurfaceFormat rvGLFormat(bool stereo = false, bool vsync = true, bool doubleBuffer = true, int red = 8, int green = 8,
                                         int blue = 8, int alpha = 8);

        GLWindow* glWindow() const { return m_glWindow; }

        QTGLVideoDevice* videoDevice() const { return m_videoDevice; }

        //
        //  Delegated QOpenGLWidget-compatible API still used across RV.
        //
        QOpenGLContext* context() const;
        QSurfaceFormat format() const;
        void makeCurrent();

        //  Compatibility shim for the former QOpenGLWidget::isValid(): the
        //  native GL window is created up-front, so the viewport is considered
        //  valid once the window exists.
        bool isValid() const;

        void absolutePosition(int& x, int& y) const;

        void stopProcessingEvents();

        bool firstPaintCompleted() const;

        void setContentSize(int w, int h) { m_csize = QSize(w, h); }

        void setMinimumContentSize(int w, int h) { m_msize = QSize(w, h); }

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

        QImage readPixels(int x, int y, int w, int h);

        // Device pixel ratio for high DPI displays
        // For reference: https://doc.qt.io/qt-6/highdpi.html
        float devicePixelRatio() const;

    private:
        //
        //  Keeping the viewport window alive across top-level window churn.
        //
        //  createWindowContainer() transfers ownership of the viewport window to
        //  the container, which parents it to the top-level QWidgetWindow. Qt
        //  destroys and recreates that QWidgetWindow when a widget is reparented
        //  into the window -- QWidget::setParent() -> destroy() -> ~QWidgetWindow
        //  -- as happens when a plugin adds a QWebEngineView to a layout, and
        //  ~QObject deletes its child QWindows, viewport included. Nothing in Qt
        //  puts it back, and QWindowContainer then dereferences the window it no
        //  longer has on the next layout pass.
        //
        //  QObject::destroyed is emitted at the top of ~QObject, before
        //  deleteChildren() runs, so watching the parent window gives us a
        //  moment where the viewport can still be detached and kept -- which is
        //  much cheaper than rebuilding it, and keeps the GL context, the device
        //  wiring and the uploaded textures intact.
        //
        void watchParentWindow();
        void parentWindowDestroyed();
        void reattachGLWindow();

    protected:
        void showEvent(QShowEvent*) override;

    private:
        RvDocument* m_doc;
        GLWindow* m_glWindow;
        QWidget* m_container;
        QTGLVideoDevice* m_videoDevice;
        QSize m_csize;
        QSize m_msize;
        QOpenGLContext* m_sharedContext;

        //
        //  The parent QWindow whose destruction is being watched, plus the
        //  connection to it so it can be rewired when the viewport window is
        //  re-parented. See watchParentWindow().
        //
        QWindow* m_watchedParentWindow;
        QMetaObject::Connection m_watchedParentConnection;
        bool m_reattachPending;
    };

} // namespace Rv

#endif // __rv-qt__GLView__h__
