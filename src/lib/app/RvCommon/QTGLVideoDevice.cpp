//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//

#ifdef PLATFORM_WINDOWS
#include <GL/glew.h>
#include <GL/wglew.h>
#endif
#include <TwkGLF/GL.h>
#include <RvCommon/QTGLVideoDevice.h>
#include <RvCommon/GLView.h>
#include <RvCommon/DesktopVideoDevice.h>
#include <TwkGLF/GLFBO.h>
#include <IPCore/Session.h>
#include <TwkApp/Application.h>
#include <TwkApp/VideoModule.h>

#include <QScreen>

namespace Rv
{
    using namespace std;
    using namespace TwkGLF;
    using namespace TwkApp;

    // QOpenGLWindow constructors (for main GLView)
    QTGLVideoDevice::QTGLVideoDevice(VideoModule* m, const string& name, QOpenGLWindow* view, QWidget* eventWidget)
        : GLVideoDevice(m, name, ImageOutput | ProvidesSync | SubWindow)
        , m_window(view)
        , m_widget(nullptr)
        , m_eventWidget(eventWidget)
        , m_translator(eventWidget ? new QTTranslator(this, eventWidget) : nullptr)
        , m_x(0)
        , m_y(0)
        , m_refresh(-1.0)
    {
        assert(view);
    }

    QTGLVideoDevice::QTGLVideoDevice(const string& name, QOpenGLWindow* view, QWidget* eventWidget)
        : GLVideoDevice(NULL, name, NoCapabilities)
        , m_window(view)
        , m_widget(nullptr)
        , m_eventWidget(eventWidget)
        , m_translator(eventWidget ? new QTTranslator(this, eventWidget) : nullptr)
        , m_x(0)
        , m_y(0)
        , m_refresh(-1.0)
    {
        assert(view);
    }

    // QOpenGLWidget constructors (for ScreenView and legacy code)
    QTGLVideoDevice::QTGLVideoDevice(VideoModule* m, const string& name, QOpenGLWidget* view)
        : GLVideoDevice(m, name, ImageOutput | ProvidesSync | SubWindow)
        , m_window(nullptr)
        , m_widget(view)
        , m_eventWidget(view)
        , m_translator(view ? new QTTranslator(this, view) : nullptr)
        , m_x(0)
        , m_y(0)
        , m_refresh(-1.0)
    {
        assert(view);
    }

    QTGLVideoDevice::QTGLVideoDevice(const string& name, QOpenGLWidget* view)
        : GLVideoDevice(NULL, name, NoCapabilities)
        , m_window(nullptr)
        , m_widget(view)
        , m_eventWidget(view)
        , m_translator(view ? new QTTranslator(this, view) : nullptr)
        , m_x(0)
        , m_y(0)
        , m_refresh(-1.0)
    {
        assert(view);
    }

    QTGLVideoDevice::QTGLVideoDevice(VideoModule* m, const string& name)
        : GLVideoDevice(m, name, ImageOutput | ProvidesSync | SubWindow)
        , m_window(nullptr)
        , m_widget(nullptr)
        , m_eventWidget(nullptr)
        , m_translator(nullptr)
        , m_x(0)
        , m_y(0)
    {
    }

    QTGLVideoDevice::~QTGLVideoDevice() { delete m_translator; }

    void QTGLVideoDevice::setWindow(QOpenGLWindow* window)
    {
        m_window = window;
        m_widget = nullptr;
    }

    void QTGLVideoDevice::setWidget(QOpenGLWidget* widget)
    {
        m_widget = widget;
        m_window = nullptr;
        m_eventWidget = widget;
        if (m_translator)
        {
            delete m_translator;
        }
        m_translator = widget ? new QTTranslator(this, widget) : nullptr;
    }

    void QTGLVideoDevice::setEventWidget(QWidget* widget)
    {
        m_eventWidget = widget;
        if (m_translator)
        {
            delete m_translator;
        }
        m_translator = widget ? new QTTranslator(this, widget) : nullptr;
    }

    GLVideoDevice* QTGLVideoDevice::newSharedContextWorkerDevice() const
    {
        if (m_window)
        {
            // Create shared context QOpenGLWindow for worker
            QOpenGLWindow* openGLWindow = new QOpenGLWindow();
            openGLWindow->setFormat(m_window->format());
            openGLWindow->create();
            if (m_window->context())
            {
                openGLWindow->context()->setShareContext(m_window->context());
            }
            return new QTGLVideoDevice(name() + "-workerContextDevice", openGLWindow);
        }
        else if (m_widget)
        {
            // Create shared context QOpenGLWidget for worker
            QOpenGLWidget* openGLWidget = new QOpenGLWidget();
            openGLWidget->setFormat(m_widget->format());
            if (m_widget->context())
            {
                openGLWidget->context()->setShareContext(m_widget->context());
            }
            return new QTGLVideoDevice(name() + "-workerContextDevice", openGLWidget);
        }
        return nullptr;
    }

    void QTGLVideoDevice::makeCurrent() const
    {
        QOpenGLContext* ctx = nullptr;
        
        if (m_window)
        {
            ctx = m_window->context();
            if (ctx && ctx->isValid())
            {
                m_window->makeCurrent();
                TWK_GLDEBUG;
                // QOpenGLWindow renders directly - bind default framebuffer (0)
                glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
                TWK_GLDEBUG;
            }
        }
        else if (m_widget)
        {
            ctx = m_widget->context();
            if (ctx && ctx->isValid())
            {
                m_widget->makeCurrent();
                TWK_GLDEBUG;
                // QOpenGLWidget has FBO - bind it
                GLint widgetFBO = m_widget->defaultFramebufferObject();
                if (widgetFBO != 0)
                    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, widgetFBO);
                TWK_GLDEBUG;
            }
        }

        if (!isWorkerDevice())
            GLVideoDevice::makeCurrent();
    }

    GLuint QTGLVideoDevice::fboID() const
    {
        if (m_window)
        {
            // QOpenGLWindow renders directly - no FBO, return 0 for default framebuffer
            return 0;
        }
        else if (m_widget)
        {
            // QOpenGLWidget has FBO
            return m_widget->defaultFramebufferObject();
        }
        return 0;
    }

    void QTGLVideoDevice::setPhysicalDevice(VideoDevice* d)
    {
        TwkGLF::GLVideoDevice::setPhysicalDevice(d);

        // Update the DevicePixelRatio associated with the physical device.
        // This is important for high DPI displays, where the device pixel ratio
        // may differ from the logical pixel ratio on a per screen basis.
        m_devicePixelRatio = 1.0f;

        static bool noQtHighDPISupport = getenv("RV_NO_QT_HDPI_SUPPORT") != nullptr;
        if (noQtHighDPISupport)
        {
            return;
        }

        if (const DesktopVideoDevice* desktopVideoDevice = dynamic_cast<const DesktopVideoDevice*>(d))
        {
            const QList<QScreen*> screens = QGuiApplication::screens();
            if (desktopVideoDevice->qtScreen() < screens.size())
            {
                m_devicePixelRatio = screens[desktopVideoDevice->qtScreen()]->devicePixelRatio();
            }
        }
    }

    void QTGLVideoDevice::redraw() const
    {
        if (!isWorkerDevice())
        {
            if (m_window)
            {
                m_window->update();
            }
            else if (m_widget)
            {
                m_widget->update();
            }
        }
    }

    void QTGLVideoDevice::redrawImmediately() const
    {
        if (!isWorkerDevice())
        {
            bool isVisible = false;
            if (m_window)
            {
                isVisible = m_window->isVisible();
            }
            else if (m_widget)
            {
                isVisible = m_widget->isVisible();
            }
            
            if (isVisible)
            {
                redraw();
            }
            else
            {
                redraw();
            }
        }
    }

    VideoDevice::Resolution QTGLVideoDevice::resolution() const
    {
        int w = 0, h = 0;
        if (m_window)
        {
            w = m_window->width();
            h = m_window->height();
        }
        else if (m_widget)
        {
            w = m_widget->width();
            h = m_widget->height();
        }
        return Resolution(w * devicePixelRatio(), h * devicePixelRatio(), 1.0f, 1.0f);
    }

    VideoDevice::Resolution QTGLVideoDevice::internalResolution() const
    {
        int w = 0, h = 0;
        if (m_window)
        {
            w = m_window->width();
            h = m_window->height();
        }
        else if (m_widget)
        {
            w = m_widget->width();
            h = m_widget->height();
        }
        return Resolution(w, h, 1.0f, 1.0f);
    }

    VideoDevice::Offset QTGLVideoDevice::offset() const { return Offset(m_x, m_y); }

    VideoDevice::Timing QTGLVideoDevice::timing() const { return Timing((m_refresh != -1.0) ? m_refresh : 0.0); }

    VideoDevice::VideoFormat QTGLVideoDevice::format() const
    {
        QWindow* win = m_window ? (QWindow*)m_window : (QWindow*)m_widget;
        return VideoFormat(win->width() * devicePixelRatio(), win->height() * devicePixelRatio(), 1.0, 1.0,
                           (m_refresh != -1.0) ? m_refresh : 0.0, hardwareIdentification());
    }

    void QTGLVideoDevice::open(const StringVector& args)
    {
        if (!isWorkerDevice())
        {
            if (m_window) m_window->show();
            else if (m_widget) m_widget->show();
        }
    }

    void QTGLVideoDevice::close()
    {
        if (!isWorkerDevice())
        {
            if (m_window) m_window->hide();
            else if (m_widget) m_widget->hide();
        }
    }

    bool QTGLVideoDevice::isOpen() const
    {
        if (isWorkerDevice())
        {
            return false;
        }
        else
        {
            if (m_window) return m_window->isVisible();
            else if (m_widget) return m_widget->isVisible();
            return false;
        }
    }

    size_t QTGLVideoDevice::width() const
    {
        if (m_window) return m_window->width() * devicePixelRatio();
        else if (m_widget) return m_widget->width() * devicePixelRatio();
        return 0;
    }

    size_t QTGLVideoDevice::height() const
    {
        if (m_window) return m_window->height() * devicePixelRatio();
        else if (m_widget) return m_widget->height() * devicePixelRatio();
        return 0;
    }

    void QTGLVideoDevice::syncBuffers() const
    {
        if (!isWorkerDevice())
        {
            makeCurrent();
            QOpenGLContext* ctx = m_window ? m_window->context() : (m_widget ? m_widget->context() : nullptr);
            if (ctx) ctx->swapBuffers(ctx->surface());
        }
    }

    void QTGLVideoDevice::setAbsolutePosition(int x, int y)
    {
        if (isWorkerDevice())
            return;

        if (x != m_x || y != m_y || m_refresh == -1.0)
        {
            float refresh = -1.0;

            VideoDevice::Resolution res = internalResolution();
            int tx = x + res.width / 2;
            int ty = y + res.height / 2;

            if (const TwkApp::VideoModule* module = TwkApp::App()->primaryVideoModule())
            {
                if (TwkApp::VideoDevice* d = module->deviceFromPosition(tx, ty))
                {
                    setPhysicalDevice(d);
                    refresh = d->timing().hz;

                    VideoDeviceContextChangeEvent event("video-device-changed", this, this, d);
                    sendEvent(event);
                }
            }

            if (refresh != m_refresh)
            {
                // Check to see refresh has a legal value before assigning it to
                // m_refresh.
                if (refresh > 0)
                {
                    m_refresh = refresh;
                }
                else
                {
                    if (IPCore::debugPlayback)
                        cout << "WARNING: ignoring intended desktop refresh "
                                "rate = "
                             << refresh << endl;
                }

                if (IPCore::debugPlayback)
                    cout << "INFO: new desktop refresh rate " << m_refresh << endl;
            }
        }
        m_x = x;
        m_y = y;
    }

} // namespace Rv
