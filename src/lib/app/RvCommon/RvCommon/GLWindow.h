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

        bool eventFilter(QObject* object, QEvent* event) override;

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
        //  Re-establish the surface when the device pixel ratio changes
        //  without a change of logical size. See the definition.
        void syncDevicePixelRatio();

        void onFrameSwapped();

        RvDocument* m_doc{nullptr};
        QTGLVideoDevice* m_videoDevice{nullptr};
        unsigned int m_lastKey{0};
        QEvent::Type m_lastKeyType{QEvent::None};
        Timer m_activityTimer;
        QTimer m_eventProcessingTimer;
        bool m_userActive{true};
        Timer m_activationTimer;
        bool m_firstPaintCompleted{false};
        int m_red{0};
        int m_green{0};
        int m_blue{0};
        int m_alpha{0};
        bool m_postFirstNonEmptyRender{false};
        bool m_stopProcessingEvents{false};
        float m_devicePixelRatio{1.0f};
        bool m_syncingDevicePixelRatio{false};
        bool m_profilingSwapPending{false};
        bool m_syncSamplePending{false};
    };

} // namespace Rv

#endif // __rv_qt__GLWindow__h__
