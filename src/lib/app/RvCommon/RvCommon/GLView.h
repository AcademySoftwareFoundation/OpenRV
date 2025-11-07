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
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QSurfaceFormat>
#include <QOffscreenSurface>
#include <QtCore/QEvent>
#include <QtCore/QTimer>
#include <TwkUtil/Timer.h>
#include <boost/thread/thread.hpp>

namespace Rv
{
    class RvDocument;
    class QTGLVideoDevice;

    class GLView
        : public QOpenGLWidget
        , protected QOpenGLFunctions
    {
        Q_OBJECT

    public:
        typedef TwkUtil::Timer Timer;

        GLView(QWidget* parent, QOpenGLContext* sharedContext, RvDocument* doc,
               bool stereo = false, bool vsync = true, bool doubleBuffer = true,
               int red = 0, int green = 0, int blue = 0, int alpha = 0,
               bool noResize = true);
        ~GLView();

        static QSurfaceFormat rvGLFormat(bool stereo = false, bool vsync = true,
                                         bool doubleBuffer = true, int red = 8,
                                         int green = 8, int blue = 8,
                                         int alpha = 8);

        void absolutePosition(int& x, int& y) const;

        QTGLVideoDevice* videoDevice() const { return m_videoDevice; }

        void stopProcessingEvents();

        virtual bool event(QEvent*);
        virtual bool eventFilter(QObject* object, QEvent* event);

        bool firstPaintCompleted() const { return m_firstPaintCompleted; };

        void setContentSize(int w, int h) { m_csize = QSize(w, h); }

        void setMinimumContentSize(int w, int h) { m_msize = QSize(w, h); }

        virtual QSize sizeHint() const;
        virtual QSize minimumSizeHint() const;

        void* syncClosure() const { return m_syncThreadData; }

        QImage readPixels(int x, int y, int w, int h);

        // Device pixel ratio for high DPI displays
        // For reference: https://doc.qt.io/qt-6/highdpi.html
        float devicePixelRatio() const;

    public slots:
        void eventProcessingTimeout();

    protected:
        void initializeGL();
        void resizeGL(int w, int h);
        void paintGL();
        void paintEvent(QPaintEvent* event) override;
        bool validateReadPixels(int x, int y, int w, int h);
        void debugSaveFramebuffer();

    private:
        RvDocument* m_doc;
        boost::thread m_swapThread;
        QTGLVideoDevice* m_videoDevice;
        unsigned int m_lastKey;
        QEvent::Type m_lastKeyType;
        Timer m_activityTimer;
        Timer m_renderTimer;
        QTimer m_eventProcessingTimer;
        bool m_userActive;
        size_t m_renderCount;
        Timer m_activationTimer;
        bool m_firstPaintCompleted;
        QSize m_csize;
        QSize m_msize;
        int m_red;
        int m_green;
        int m_blue;
        int m_alpha;
        bool m_postFirstNonEmptyRender;
        bool m_stopProcessingEvents;
        void* m_syncThreadData;
        QOpenGLContext* m_sharedContext;
    };

} // namespace Rv

#endif // __rv-qt__GLView__h__
