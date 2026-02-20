//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __RvCommon__QTGLVideoDevice__h__
#define __RvCommon__QTGLVideoDevice__h__
#include <iostream>
#include <TwkGLF/GLVideoDevice.h>
#include <QOpenGLWindow>
#include <QOpenGLWidget>
#include <QWindow>
#include <QtWidgets/QWidget>
#include <RvCommon/QTTranslator.h>

namespace Rv
{

    //
    //  QTGLVideoDevice
    //
    //  Wraps a QOpenGLWindow as a TwkGLF::GLVideoDevice. This gives a handle
    //  on a concrete window system device. QTGLVideoDevice can generate
    //  events from the window system
    //

    class QTGLVideoDevice : public TwkGLF::GLVideoDevice
    {
    public:
        // QOpenGLWindow constructors (for main GLView)
        QTGLVideoDevice(TwkApp::VideoModule*, const std::string& name, QOpenGLWindow* view, QWidget* eventWidget = nullptr);
        
        // QOpenGLWidget constructors (for ScreenView and legacy code)
        QTGLVideoDevice(TwkApp::VideoModule*, const std::string& name, QOpenGLWidget* view);
        
        QTGLVideoDevice(TwkApp::VideoModule*, const std::string& name);
        virtual ~QTGLVideoDevice();

        void setWindow(QOpenGLWindow*);
        void setWidget(QOpenGLWidget*);
        void setEventWidget(QWidget* widget);

        QOpenGLWindow* window() const { return m_window; }
        QOpenGLWidget* widget() const { return m_widget; }
        QWidget* eventWidget() const { return m_eventWidget; }

        virtual void makeCurrent() const;

        const QTTranslator& translator() const { return *m_translator; }

        virtual void redraw() const;
        virtual void redrawImmediately() const;

        void setAbsolutePosition(int x, int y);

        //
        //  VideoDevice API
        //

        virtual GLVideoDevice* newSharedContextWorkerDevice() const;
        virtual Resolution resolution() const;
        virtual Offset offset() const;
        virtual Timing timing() const;
        virtual VideoFormat format() const;

        virtual size_t width() const;
        virtual size_t height() const;

        virtual Resolution internalResolution() const;

        virtual void open(const StringVector&);
        virtual void close();
        virtual bool isOpen() const;

        virtual void syncBuffers() const;

        virtual GLuint fboID() const;

        bool isWorkerDevice() const { return Capabilities(capabilities()) == NoCapabilities; }

        virtual void setPhysicalDevice(VideoDevice* d);

        // Device pixel ratio for high DPI displays
        // For reference: https://doc.qt.io/qt-6/highdpi.html
        float devicePixelRatio() const override { return m_devicePixelRatio; }

    protected:
        QTGLVideoDevice(const std::string& name, QOpenGLWindow* view, QWidget* eventWidget = nullptr);
        QTGLVideoDevice(const std::string& name, QOpenGLWidget* view);

    protected:
        int m_x;
        int m_y;
        float m_refresh;
        float m_devicePixelRatio{1.0f};
        QOpenGLWindow* m_window;
        QOpenGLWidget* m_widget;
        QWidget* m_eventWidget;
        QTTranslator* m_translator;
    };

} // namespace Rv

#endif // __RvCommon__QTGLVideoDevice__h__
