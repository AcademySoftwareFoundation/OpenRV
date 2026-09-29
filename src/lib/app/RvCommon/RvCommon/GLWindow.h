//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __rv_qt__GLWindow__h__
#define __rv_qt__GLWindow__h__
#include <TwkGLF/GL.h>
#include <QOpenGLWindow>
#include <QOpenGLFunctions>
#include <QSurfaceFormat>
#include <QtCore/QEvent>
#include <QtCore/QTimer>
#include <QImage>
#include <TwkUtil/Timer.h>

namespace Rv
{
    class RvDocument;
    class QTGLVideoDevice;

    //
    //  GLWindow
    //
    //  The RV viewport rendered as a *native* GL surface (QOpenGLWindow)
    //  instead of a composited QOpenGLWidget. Because it is a real window and
    //  not a QOpenGLWidget in the main window's widget tree, the top-level
    //  QMainWindow is no longer forced to the OpenGL RHI backend -- so docked
    //  QWebEngineView panels render on the platform default backend -- and the
    //  viewport presents on its own surface instead of via the ~90 ms
    //  full-window widget composite.
    //
    //  It is embedded in the widget hierarchy by GLView via
    //  QWidget::createWindowContainer(). Rendering and event routing are ported
    //  from the former QOpenGLWidget-based GLView.
    //

    class GLWindow
        : public QOpenGLWindow
        , protected QOpenGLFunctions
    {
        Q_OBJECT

    public:
        typedef TwkUtil::Timer Timer;

        GLWindow(QOpenGLContext* sharedContext, RvDocument* doc, bool stereo = false, bool vsync = true, bool doubleBuffer = true,
                 int red = 0, int green = 0, int blue = 0, int alpha = 0, bool noResize = true);
        ~GLWindow() override;

        QTGLVideoDevice* videoDevice() const { return m_videoDevice; }

        //  The device is created and owned by the hosting GLView (it needs the
        //  container QWidget for event/coordinate translation), then handed
        //  here. GLWindow does not take ownership.
        void setVideoDevice(QTGLVideoDevice* d) { m_videoDevice = d; }

        void stopProcessingEvents();

        bool event(QEvent*) override;

        bool firstPaintCompleted() const { return m_firstPaintCompleted; }

        // Absolute (global) top-left position of the surface in pixels.
        void absolutePosition(int& x, int& y) const;

        float devicePixelRatioF() const;

        QImage readPixels(int x, int y, int w, int h);

    public slots:
        void eventProcessingTimeout();

    protected:
        void initializeGL() override;
        void resizeGL(int w, int h) override;
        void paintGL() override;

    private:
        RvDocument* m_doc;
        QTGLVideoDevice* m_videoDevice;
        unsigned int m_lastKey;
        QEvent::Type m_lastKeyType;
        Timer m_activityTimer;
        QTimer m_eventProcessingTimer;
        bool m_userActive;
        Timer m_activationTimer;
        bool m_firstPaintCompleted;
        int m_red;
        int m_green;
        int m_blue;
        int m_alpha;
        bool m_postFirstNonEmptyRender;
        bool m_stopProcessingEvents;
    };

} // namespace Rv

#endif // __rv_qt__GLWindow__h__
