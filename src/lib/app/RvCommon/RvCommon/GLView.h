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
#include <QtOpenGL/QGLWidget>
#include <QtCore/QEvent>
#include <QtCore/QTimer>
#include <TwkUtil/Timer.h>
#include <boost/thread/thread.hpp>

namespace Rv
{
    class RvDocument;
    class QTFrameBuffer;
    class QTGLVideoDevice;

    class GLView : public QGLWidget
    {
        Q_OBJECT

    public:
        typedef TwkUtil::Timer Timer;

        GLView(QWidget* parent, const QGLWidget* share, RvDocument* doc,
               bool stereo = false, bool vsync = true, bool doubleBuffer = true,
               int red = 0, int green = 0, int blue = 0, int alpha = 0,
               bool noResize = true);
        ~GLView();

        static QGLFormat rvGLFormat(bool stereo = false, bool vsync = true,
                                    bool doubleBuffer = true, int red = 8,
                                    int green = 8, int blue = 8, int alpha = 8);

        void absolutePosition(int& x, int& y) const;

        // QTFrameBuffer* frameBuffer() const { return m_frameBuffer; }
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

    public slots:
        void eventProcessingTimeout();

    protected:
        void initializeGL();
        void resizeGL(int w, int h);
        void paintGL();
        void swapBuffersNoSync();

    private:
        RvDocument* m_doc;
        // QTFrameBuffer*   m_frameBuffer;
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
    };

} // namespace Rv

#endif // __rv-qt__GLView__h__
