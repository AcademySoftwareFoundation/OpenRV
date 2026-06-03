//
//  Copyright (c) 2026 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//

#ifdef PLATFORM_LINUX

#include <GL/glew.h>
#include <RvCommon/QTVulkanVideoDevice.h>
#include <RvCommon/VulkanView.h>
#include <RvCommon/DesktopVideoDevice.h>
#include <TwkApp/Application.h>
#include <TwkApp/VideoModule.h>
#include <TwkApp/Event.h>
#include <IPCore/Session.h>
#include <TwkGLF/GLFBO.h>

#include <QScreen>
#include <QGuiApplication>
#include <QSurfaceFormat>
#include <QOpenGLContext>
#include <QOffscreenSurface>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <vector>

namespace Rv
{
    using namespace std;
    using namespace TwkApp;

    QTVulkanVideoDevice::QTVulkanVideoDevice(VideoModule* module,
                                           const string& name,
                                           VulkanView* view,
                                           QWidget* eventWidget)
        : TwkGLF::GLVideoDevice(module, name,
                                 VideoDevice::ImageOutput | VideoDevice::ProvidesSync | VideoDevice::SubWindow)
        , m_view(view)
        , m_eventWidget(eventWidget)
        , m_translator(eventWidget ? new QTTranslator(this, eventWidget) : nullptr)
    {
        assert(view);
    }

    QTVulkanVideoDevice::~QTVulkanVideoDevice()
    {
        // Delete the FBO and its colour texture while the GL context is current.
        if (m_glContext && (m_fbo || m_fboColorTex))
        {
            m_glContext->makeCurrent(m_offscreenSurface);
            delete m_fbo;
            m_fbo = nullptr;
            if (m_fboColorTex)
            {
                glDeleteTextures(1, &m_fboColorTex);
                m_fboColorTex = 0;
            }
            m_glContext->doneCurrent();
        }

        delete m_offscreenSurface;
        m_offscreenSurface = nullptr;

        delete m_glContext;
        m_glContext = nullptr;

        delete m_translator;
    }

    void QTVulkanVideoDevice::setEventWidget(QWidget* widget)
    {
        m_eventWidget = widget;
        delete m_translator;
        m_translator = widget ? new QTTranslator(this, widget) : nullptr;
    }

    //--------------------------------------------------------------------------

    void QTVulkanVideoDevice::ensureGLContext() const
    {
        if (!m_glContext)
        {
            QSurfaceFormat fmt;
            fmt.setRenderableType(QSurfaceFormat::OpenGL);
            fmt.setMajorVersion(2);
            fmt.setMinorVersion(1);

            m_glContext = new QOpenGLContext();
            m_glContext->setFormat(fmt);

            if (!m_glContext->create())
            {
                std::cerr << "[QTVulkanVideoDevice] QOpenGLContext::create() failed\n";
                delete m_glContext;
                m_glContext = nullptr;
                return;
            }

            m_offscreenSurface = new QOffscreenSurface();
            m_offscreenSurface->setFormat(m_glContext->format());
            m_offscreenSurface->create();

            if (!m_offscreenSurface->isValid())
            {
                std::cerr << "[QTVulkanVideoDevice] QOffscreenSurface::create() failed\n";
                delete m_offscreenSurface;
                m_offscreenSurface = nullptr;
                delete m_glContext;
                m_glContext = nullptr;
                return;
            }
            
            m_glContext->makeCurrent(m_offscreenSurface);
            glewExperimental = GL_TRUE;
            GLenum err = glewInit();
            if (err != GLEW_OK)
            {
                std::cerr << "[QTVulkanVideoDevice] glewInit failed: " << glewGetErrorString(err) << "\n";
            }
        }

        if (!m_glContext->makeCurrent(m_offscreenSurface))
        {
            std::cerr << "[QTVulkanVideoDevice] makeCurrent() failed\n";
            return;
        }

        const float dpr = m_view ? m_view->devicePixelRatio() : 1.0f;
        int newW = m_view ? static_cast<int>(m_view->width()  * dpr + 0.5f) : 128;
        int newH = m_view ? static_cast<int>(m_view->height() * dpr + 0.5f) : 128;
        if (newW < 1) newW = 128;
        if (newH < 1) newH = 128;

        if (!m_fbo || m_fboWidth != newW || m_fboHeight != newH)
        {
            delete m_fbo;
            m_fbo = nullptr;
            if (m_fboColorTex)
            {
                glDeleteTextures(1, &m_fboColorTex);
                m_fboColorTex = 0;
            }

            glGenTextures(1, &m_fboColorTex);
            glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_fboColorTex);
            glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA16F_ARB,
                         newW, newH, 0, GL_RGBA, GL_FLOAT, nullptr);
            glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

            m_fbo = new TwkGLF::GLFBO(newW, newH, GL_RGBA16F_ARB);
            m_fbo->attachColorTexture(GL_TEXTURE_RECTANGLE_ARB, m_fboColorTex);

            GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
            if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
                std::cerr << "[QTVulkanVideoDevice] FBO incomplete: 0x"
                          << std::hex << status << std::dec << "\n";

            m_fboWidth  = newW;
            m_fboHeight = newH;
        }

        m_fbo->bind();

        GLVideoDevice::makeCurrent();
    }

    //--------------------------------------------------------------------------

    void QTVulkanVideoDevice::setAbsolutePosition(int x, int y)
    {
        if (x != m_x || y != m_y || m_refresh == -1.0f)
        {
            float refresh = -1.0f;

            int w  = m_view ? m_view->width()  : 0;
            int h  = m_view ? m_view->height() : 0;
            int tx = x + w / 2;
            int ty = y + h / 2;

            if (const TwkApp::VideoModule* mod = TwkApp::App()->primaryVideoModule())
            {
                if (TwkApp::VideoDevice* d = mod->deviceFromPosition(tx, ty))
                {
                    setPhysicalDevice(d);
                    refresh = d->timing().hz;

                    VideoDeviceContextChangeEvent event("video-device-changed", this, this, d);
                    sendEvent(event);
                }
            }

            if (refresh != m_refresh)
            {
                if (refresh > 0)
                    m_refresh = refresh;
                else if (IPCore::debugPlayback)
                    cout << "WARNING: ignoring intended desktop refresh rate = " << refresh << endl;

                if (IPCore::debugPlayback)
                    cout << "INFO: new desktop refresh rate " << m_refresh << endl;
            }
        }
        m_x = x;
        m_y = y;
    }

    void QTVulkanVideoDevice::setPhysicalDevice(VideoDevice* d)
    {
        TwkApp::VideoDevice::setPhysicalDevice(d);

        m_devicePixelRatio = 1.0f;

        static bool noQtHighDPISupport = (getenv("RV_NO_QT_HDPI_SUPPORT") != nullptr);
        if (noQtHighDPISupport)
            return;

        if (const DesktopVideoDevice* desktopDev = dynamic_cast<const DesktopVideoDevice*>(d))
        {
            const QList<QScreen*> screens = QGuiApplication::screens();
            if (desktopDev->qtScreen() < screens.size())
                m_devicePixelRatio = screens[desktopDev->qtScreen()]->devicePixelRatio();
        }
    }

    float QTVulkanVideoDevice::devicePixelRatio() const
    {
        if (m_view)
            return static_cast<float>(m_view->devicePixelRatio());
        return m_devicePixelRatio;
    }

    //--------------------------------------------------------------------------
    // GLVideoDevice API
    //--------------------------------------------------------------------------

    void QTVulkanVideoDevice::makeCurrent() const
    {
        ensureGLContext();
    }

    TwkGLF::GLFBO* QTVulkanVideoDevice::defaultFBO()
    {
        ensureGLContext();
        return m_fbo;
    }

    const TwkGLF::GLFBO* QTVulkanVideoDevice::defaultFBO() const
    {
        ensureGLContext();
        return m_fbo;
    }

    std::string QTVulkanVideoDevice::hardwareIdentification() const
    {
        return "vulkan-hybrid";
    }

    //--------------------------------------------------------------------------
    // syncBuffers
    //--------------------------------------------------------------------------

    void QTVulkanVideoDevice::syncBuffers() const
    {
        if (!m_view)
            return;

        if (!m_glContext || !m_fbo)
            return;

        TwkGLF::GLFBO* fbo = m_fbo;
        const int w = static_cast<int>(fbo->width());
        const int h = static_cast<int>(fbo->height());

        if (w <= 0 || h <= 0)
            return;

        if (!m_glContext->makeCurrent(m_offscreenSurface))
            return;

        fbo->bind();

        glFinish();

        const size_t floatCount = static_cast<size_t>(w) * h * 4;
        std::vector<float> floatPx(floatCount);
        glReadPixels(0, 0, w, h, GL_RGBA, GL_FLOAT, floatPx.data());

        fbo->unbind();

        // Pack into A2B10G10R10 
        std::vector<uint32_t> packed(static_cast<size_t>(w) * h);

        for (int y = 0; y < h; ++y)
        {
            // src row: y-flip (GL origin is bottom-left)
            const float* src = floatPx.data() + (size_t)(h - 1 - y) * w * 4;
            uint32_t*    dst = packed.data()  + (size_t)y * w;

            for (int x = 0; x < w; ++x, src += 4)
            {
                const float r = std::max(0.f, std::min(1.f, src[0]));
                const float g = std::max(0.f, std::min(1.f, src[1]));
                const float b = std::max(0.f, std::min(1.f, src[2]));
                const uint32_t ri = static_cast<uint32_t>(r * 1023.f + 0.5f) & 0x3FF;
                const uint32_t gi = static_cast<uint32_t>(g * 1023.f + 0.5f) & 0x3FF;
                const uint32_t bi = static_cast<uint32_t>(b * 1023.f + 0.5f) & 0x3FF;
                // A2B10G10R10: bits[31:30]=A(2), bits[29:20]=B(10), bits[19:10]=G(10), bits[9:0]=R(10)
                dst[x] = (3u << 30) | (bi << 20) | (gi << 10) | ri;
            }
        }

        m_view->presentPixelData(packed.data(), w, h);
    }

    //--------------------------------------------------------------------------
    // VideoDevice API
    //--------------------------------------------------------------------------

    void QTVulkanVideoDevice::redraw() const
    {
        if (m_view)
            QCoreApplication::postEvent(m_view, new QEvent(QEvent::UpdateRequest));
    }

    void QTVulkanVideoDevice::redrawImmediately() const
    {
        redraw();
    }

    void QTVulkanVideoDevice::clearCaches() const
    {
    }

    VideoDevice::Resolution QTVulkanVideoDevice::resolution() const
    {
        if (!m_view)
            return Resolution(0, 0, 1.0f, 1.0f);
        const float dpr = m_view->devicePixelRatio();
        return Resolution(static_cast<int>(m_view->width()  * dpr + 0.5f),
                          static_cast<int>(m_view->height() * dpr + 0.5f),
                          1.0f, 1.0f);
    }

    VideoDevice::Offset QTVulkanVideoDevice::offset() const
    {
        return Offset(m_x, m_y);
    }

    VideoDevice::Timing QTVulkanVideoDevice::timing() const
    {
        return Timing((m_refresh != -1.0f) ? m_refresh : 0.0f);
    }

    VideoDevice::VideoFormat QTVulkanVideoDevice::format() const
    {
        if (!m_view)
            return VideoFormat(0, 0, 1.0, 1.0, 0.0, hardwareIdentification());
        const float dpr = m_view->devicePixelRatio();
        return VideoFormat(static_cast<int>(m_view->width()  * dpr + 0.5f),
                           static_cast<int>(m_view->height() * dpr + 0.5f),
                           1.0, 1.0,
                           (m_refresh != -1.0f) ? m_refresh : 0.0f,
                           hardwareIdentification());
    }

    size_t QTVulkanVideoDevice::width() const
    {
        if (!m_view) return 0;
        return static_cast<size_t>(m_view->width() * m_view->devicePixelRatio() + 0.5f);
    }

    size_t QTVulkanVideoDevice::height() const
    {
        if (!m_view) return 0;
        return static_cast<size_t>(m_view->height() * m_view->devicePixelRatio() + 0.5f);
    }

    void QTVulkanVideoDevice::open(const StringVector& /*args*/)
    {
        if (m_view)
            m_view->show();
        m_isOpen = true;
    }

    void QTVulkanVideoDevice::close()
    {
        if (m_view)
            m_view->hide();
        m_isOpen = false;
    }

    bool QTVulkanVideoDevice::isOpen() const
    {
        if (m_view)
            return m_view->isVisible();
        return false;
    }

} // namespace Rv

#endif // PLATFORM_LINUX