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
#include <QOpenGLWidget>
#include <QOpenGLWindow>
#include <QOpenGLContext>
#include <QSurfaceFormat>
#include <QWindow>
#include <QtWidgets/QWidget>
#include <RvCommon/QTTranslator.h>

namespace Rv
{

    //
    //  QTGLVideoDevice
    //
    //  Wraps a QGLWidget as a TwkGLF::GLVideoDevice. This gives a handle
    //  on a concrete window system device. QTGLVideoDevice can generate
    //  events from the window system
    //

    class QTGLVideoDevice : public TwkGLF::GLVideoDevice
    {
    public:
        QTGLVideoDevice(TwkApp::VideoModule*, const std::string& name, QOpenGLWidget* view);
        //
        //  Native-window backed variant. The GL surface is a QOpenGLWindow
        //  (embedded in the widget tree via createWindowContainer); eventWidget
        //  is the container QWidget used by the QTTranslator for coordinate
        //  mapping (height/mapToGlobal) and mouse grab.
        //
        QTGLVideoDevice(TwkApp::VideoModule*, const std::string& name, QOpenGLWindow* window, QWidget* eventWidget);
        QTGLVideoDevice(TwkApp::VideoModule*, const std::string& name);
        virtual ~QTGLVideoDevice();

        void setWidget(QOpenGLWidget*);

        QOpenGLWidget* widget() const { return m_view; }

        QOpenGLWindow* window() const { return m_window; }

        //
        //  Backing-agnostic accessors for the shareable GL context and its
        //  surface format. The control view may be backed by either a
        //  QOpenGLWidget (m_view) or, in the native-window port, a
        //  QOpenGLWindow (m_window). Second-output devices (DesktopVideoDevice)
        //  share the control context and need these without caring which.
        //
        QOpenGLContext* glShareContext() const { return m_window ? m_window->context() : (m_view ? m_view->context() : nullptr); }

        QSurfaceFormat glSurfaceFormat() const { return m_window ? m_window->format() : (m_view ? m_view->format() : QSurfaceFormat()); }

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
        QTGLVideoDevice(const std::string& name, QOpenGLWidget* view);

    protected:
        int m_x;
        int m_y;
        float m_refresh;
        float m_devicePixelRatio{1.0f};
        QOpenGLWidget* m_view;
        QOpenGLWindow* m_window{nullptr};
        QTTranslator* m_translator;
    };

} // namespace Rv

#endif // __RvCommon__QTGLVideoDevice__h__
