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

class QOpenGLContext;

namespace Rv
{
    class RvDocument;
    class QTGLVideoDevice;
    class GLWindow;

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
        RvDocument* m_doc;
        GLWindow* m_glWindow;
        QWidget* m_container;
        QTGLVideoDevice* m_videoDevice;
        QSize m_csize;
        QSize m_msize;
        QOpenGLContext* m_sharedContext;
    };

} // namespace Rv

#endif // __rv-qt__GLView__h__
