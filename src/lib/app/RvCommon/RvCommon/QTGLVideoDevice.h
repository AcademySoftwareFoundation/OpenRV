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
        QTGLVideoDevice(TwkApp::VideoModule*, const std::string& name);
        virtual ~QTGLVideoDevice();

        void setWidget(QOpenGLWidget*);

        QOpenGLWidget* widget() const { return m_view; }

        virtual void makeCurrent() const override;

        const QTTranslator& translator() const { return *m_translator; }

        virtual void redraw() const override;
        virtual void redrawImmediately() const override;

        void setAbsolutePosition(int x, int y);

        //
        //  VideoDevice API
        //

        virtual GLVideoDevice* newSharedContextWorkerDevice() const override;
        virtual Resolution resolution() const override;
        virtual Offset offset() const override;
        virtual Timing timing() const override;
        virtual VideoFormat format() const override;

        virtual size_t width() const override;
        virtual size_t height() const override;

        virtual Resolution internalResolution() const override;

        virtual void open(const StringVector&) override;
        virtual void close() override;
        virtual bool isOpen() const override;

        virtual void syncBuffers() const override;

        virtual GLuint fboID() const override;

        bool isWorkerDevice() const { return Capabilities(capabilities()) == NoCapabilities; }

        virtual void setPhysicalDevice(VideoDevice* d) override;

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
        QTTranslator* m_translator;
    };

} // namespace Rv

#endif // __RvCommon__QTGLVideoDevice__h__
