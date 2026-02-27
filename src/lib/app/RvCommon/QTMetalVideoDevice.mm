//
//  Copyright (c) 2024 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//

#ifdef PLATFORM_DARWIN

#import <AppKit/AppKit.h>

#include <RvCommon/QTMetalVideoDevice.h>
#include <RvCommon/MetalView.h>
#include <RvCommon/DesktopVideoDevice.h>
#include <TwkApp/Application.h>
#include <TwkApp/VideoModule.h>
#include <TwkApp/Event.h>
#include <IPCore/Session.h>
#include <TwkGLF/GLFBO.h>

#include <QScreen>
#include <QGuiApplication>
#include <QSurfaceFormat>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <vector>

namespace Rv
{
    using namespace std;
    using namespace TwkApp;

    QTMetalVideoDevice::QTMetalVideoDevice(VideoModule* module,
                                           const string& name,
                                           MetalView* view,
                                           QWidget* eventWidget,
                                           int bitsPerChannel)
        : TwkGLF::GLVideoDevice(module, name,
                                 VideoDevice::ImageOutput | VideoDevice::ProvidesSync | VideoDevice::SubWindow)
        , m_view(view)
        , m_eventWidget(eventWidget)
        , m_translator(eventWidget ? new QTTranslator(this, eventWidget) : nullptr)
        , m_bitsPerChannel(bitsPerChannel)
    {
        assert(view);
    }

    QTMetalVideoDevice::~QTMetalVideoDevice()
    {
        // Delete the FBO and its colour texture while the GL context is current.
        // GLFBO::attachColorTexture passes owner=false so GLFBO does NOT delete
        // the colour texture — we must do it explicitly here.
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

    void QTMetalVideoDevice::setEventWidget(QWidget* widget)
    {
        m_eventWidget = widget;
        delete m_translator;
        m_translator = widget ? new QTTranslator(this, widget) : nullptr;
    }

    //--------------------------------------------------------------------------

    void QTMetalVideoDevice::ensureGLContext() const
    {
        if (!m_glContext)
        {
            //
            // Create a QOpenGLContext with the same format that GLView uses
            // (GL 2.1, no profile).  On macOS, Qt backs QOffscreenSurface with
            // a hidden NSView, so the context gets a real native surface —
            // required for GL-on-Metal on Apple Silicon where a raw
            // NSOpenGLContext without a view silently fails to become current.
            //
            QSurfaceFormat fmt;
            fmt.setRenderableType(QSurfaceFormat::OpenGL);
            // Use GL 2.1 Compatibility profile — the existing IPCore pipeline
            // calls legacy APIs (glPushAttrib, GL_LIGHTING, GL_MAX_TEXTURE_UNITS,
            // etc.) that are absent in Core profile.  With GL 2.1, GLSL is 1.20
            // and ShaderFunction::initGLSLVersion() is hardcoded to 1.20 on the
            // USE_METAL path, so replaceTextureCalls() uses texture2DRect() which
            // is available with GL_ARB_texture_rectangle.
            fmt.setMajorVersion(2);
            fmt.setMinorVersion(1);
            // No setProfile() — defaults to NoProfile (Compatibility) on macOS.

            m_glContext = new QOpenGLContext();
            m_glContext->setFormat(fmt);

            if (!m_glContext->create())
            {
                std::cerr << "[QTMetalVideoDevice] QOpenGLContext::create() failed\n";
                delete m_glContext;
                m_glContext = nullptr;
                return;
            }

            // Surface format MUST be set from the *actual* context format, not
            // the requested one, to guarantee compatibility.
            m_offscreenSurface = new QOffscreenSurface();
            m_offscreenSurface->setFormat(m_glContext->format());
            m_offscreenSurface->create();

            if (!m_offscreenSurface->isValid())
            {
                std::cerr << "[QTMetalVideoDevice] QOffscreenSurface::create() failed\n";
                delete m_offscreenSurface;
                m_offscreenSurface = nullptr;
                delete m_glContext;
                m_glContext = nullptr;
                return;
            }
        }

        if (!m_glContext->makeCurrent(m_offscreenSurface))
        {
            std::cerr << "[QTMetalVideoDevice] makeCurrent() failed\n";
            return;
        }

        // Size the FBO in PHYSICAL pixels so it matches the IOSurface
        // (which is sized at bounds × contentsScale = bounds × DPR).
        // IPCore receives width()/height() in physical pixels and renders at
        // that resolution.  Event coordinates from Qt are in logical pixels
        // and are mapped through setRelativeDomain(QWindow::width(), height())
        // which uses logical dimensions — so input handling is unaffected.
        const float dpr = m_view ? m_view->devicePixelRatio() : 1.0f;
        int newW = m_view ? static_cast<int>(m_view->width()  * dpr + 0.5f) : 128;
        int newH = m_view ? static_cast<int>(m_view->height() * dpr + 0.5f) : 128;
        if (newW < 1) newW = 128;
        if (newH < 1) newH = 128;

        if (!m_fbo || m_fboWidth != newW || m_fboHeight != newH)
        {
            // Destroy the old FBO.  GLFBO::attachColorTexture passes owner=false
            // so GLFBO does NOT delete the attached texture — we must do it here.
            delete m_fbo;
            m_fbo = nullptr;
            if (m_fboColorTex)
            {
                glDeleteTextures(1, &m_fboColorTex);
                m_fboColorTex = 0;
            }

            // Use a texture attachment for the colour buffer.
            // GL_RGBA16F_ARB is supported for *textures* (with ARB_texture_float)
            // on macOS GL 2.1, but is NOT a valid renderbuffer internal format on
            // that profile — an incomplete FBO would silently swallow all draws.
            // A GL_TEXTURE_RECTANGLE_ARB attachment gives us full float precision
            // and guarantees a complete FBO.
            glGenTextures(1, &m_fboColorTex);
            glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_fboColorTex);
            glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA16F_ARB,
                         newW, newH, 0, GL_RGBA, GL_FLOAT, nullptr);
            glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

            m_fbo = new TwkGLF::GLFBO(newW, newH, GL_RGBA16F_ARB);
            m_fbo->attachColorTexture(GL_TEXTURE_RECTANGLE_ARB, m_fboColorTex);
            // attachColorTexture stores owner=false; we retain the texID and
            // delete it ourselves on the next resize or in the destructor.

            GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
            if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
                std::cerr << "[QTMetalVideoDevice] FBO incomplete: 0x"
                          << std::hex << status << std::dec << "\n";

            m_fboWidth  = newW;
            m_fboHeight = newH;
        }

        // Bind FBO so all GL rendering goes into it
        m_fbo->bind();

        // Let GLVideoDevice do its bookkeeping (GLtext context, etc.)
        GLVideoDevice::makeCurrent();
    }

    //--------------------------------------------------------------------------

    void QTMetalVideoDevice::setAbsolutePosition(int x, int y)
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

    void QTMetalVideoDevice::setPhysicalDevice(VideoDevice* d)
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

    float QTMetalVideoDevice::devicePixelRatio() const
    {
        // Always read from the widget's actual screen DPR rather than the
        // cached m_devicePixelRatio, which is 1.0 until setPhysicalDevice() is
        // first called.  This guarantees the FBO and IPCore viewport agree from
        // the very first frame.
        if (m_view)
            return static_cast<float>(m_view->devicePixelRatio());
        return m_devicePixelRatio;  // fallback when no view
    }

    //--------------------------------------------------------------------------
    // GLVideoDevice API
    //--------------------------------------------------------------------------

    void QTMetalVideoDevice::makeCurrent() const
    {
        // Ensure QOpenGLContext + FBO exist, make GL context current, bind FBO
        ensureGLContext();
    }

    TwkGLF::GLFBO* QTMetalVideoDevice::defaultFBO()
    {
        ensureGLContext();
        return m_fbo;
    }

    const TwkGLF::GLFBO* QTMetalVideoDevice::defaultFBO() const
    {
        ensureGLContext();
        return m_fbo;
    }

    std::string QTMetalVideoDevice::hardwareIdentification() const
    {
        return "metal-hybrid";
    }

    //--------------------------------------------------------------------------
    // syncBuffers — GL FBO readback → CPU format conversion → IOSurface present
    //--------------------------------------------------------------------------

    void QTMetalVideoDevice::syncBuffers() const
    {
        if (!m_view)
            return;

        if (!m_glContext || !m_fbo)
            return;  // Nothing rendered yet

        TwkGLF::GLFBO* fbo = m_fbo;
        const int w = static_cast<int>(fbo->width());
        const int h = static_cast<int>(fbo->height());

        // Log dimensions on first few calls to aid debugging
        if (m_syncLogCount < 5)
        {
            ++m_syncLogCount;
            std::cerr << "[syncBuffers #" << m_syncLogCount << "]"
                      << " view=" << (m_view ? m_view->width() : -1)
                      << "x" << (m_view ? m_view->height() : -1)
                      << " fbo=" << w << "x" << h << "\n";
        }

        if (w <= 0 || h <= 0)
            return;

        // Make the GL context current before calling GL functions
        if (!m_glContext->makeCurrent(m_offscreenSurface))
            return;

        fbo->bind();

        // Force the GPU to complete all pending GL commands before reading back.
        // On macOS (GL-on-Metal), glReadPixels does not implicitly synchronise
        // the Metal command stream, so without glFinish() the FBO content is
        // still uninitialised/zero when we sample it.
        glFinish();

        // --- GL readback ---
        // Read as 32-bit float RGBA — safe regardless of the FBO's internal
        // format (GL_RGBA16F).  We do the y-flip and format conversion in CPU.
        const size_t floatCount = static_cast<size_t>(w) * h * 4;
        std::vector<float> floatPx(floatCount);
        glReadPixels(0, 0, w, h, GL_RGBA, GL_FLOAT, floatPx.data());

        fbo->unbind();

        // --- Format conversion + y-flip ---
        //
        // For 10-bit: ARGB2101010LE — (A<<30)|(R<<20)|(G<<10)|B
        //   where A=3 (fully opaque, 2-bit), R/G/B are 10-bit [0..1023]
        //   This is kCVPixelFormatType_ARGB2101010LEPacked = 'l30r' = 0x6C333072
        //
        // For 8-bit: BGRA8 — B,G,R,A bytes
        //   This is kCVPixelFormatType_32BGRA = 'BGRA'
        //
        // GL origin is bottom-left; IOSurface/CALayer origin is top-left.
        // We y-flip by iterating src rows in reverse.

        const bool is10Bit = (m_bitsPerChannel >= 10);

        if (is10Bit)
        {
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
                    // ARGB2101010LE: bits[31:30]=A(2), bits[29:20]=R(10),
                    //                bits[19:10]=G(10), bits[9:0]=B(10)
                    // Alpha always opaque: A=3 (= 0b11, max 2-bit value).
                    dst[x] = (3u << 30) | (ri << 20) | (gi << 10) | bi;
                }
            }

            m_view->presentPixelData(packed.data(), w, h, true);
        }
        else
        {
            std::vector<uint8_t> bytes(static_cast<size_t>(w) * h * 4);

            for (int y = 0; y < h; ++y)
            {
                const float*  src = floatPx.data() + (size_t)(h - 1 - y) * w * 4;
                uint8_t*      dst = bytes.data()   + (size_t)y * w * 4;

                for (int x = 0; x < w; ++x, src += 4, dst += 4)
                {
                    // BGRA8: B, G, R, A
                    dst[0] = static_cast<uint8_t>(std::max(0.f, std::min(1.f, src[2])) * 255.f + 0.5f);
                    dst[1] = static_cast<uint8_t>(std::max(0.f, std::min(1.f, src[1])) * 255.f + 0.5f);
                    dst[2] = static_cast<uint8_t>(std::max(0.f, std::min(1.f, src[0])) * 255.f + 0.5f);
                    dst[3] = 255;  // fully opaque
                }
            }

            m_view->presentPixelData(bytes.data(), w, h, false);
        }
    }

    //--------------------------------------------------------------------------
    // VideoDevice API
    //--------------------------------------------------------------------------

    void QTMetalVideoDevice::redraw() const
    {
        if (m_view)
            QCoreApplication::postEvent(m_view, new QEvent(QEvent::UpdateRequest));
    }

    void QTMetalVideoDevice::redrawImmediately() const
    {
        redraw();
    }

    void QTMetalVideoDevice::clearCaches() const
    {
        // No device-level cache to clear.
    }

    VideoDevice::Resolution QTMetalVideoDevice::resolution() const
    {
        if (!m_view)
            return Resolution(0, 0, 1.0f, 1.0f);
        const float dpr = m_view->devicePixelRatio();
        return Resolution(static_cast<int>(m_view->width()  * dpr + 0.5f),
                          static_cast<int>(m_view->height() * dpr + 0.5f),
                          1.0f, 1.0f);
    }

    VideoDevice::Offset QTMetalVideoDevice::offset() const
    {
        return Offset(m_x, m_y);
    }

    VideoDevice::Timing QTMetalVideoDevice::timing() const
    {
        return Timing((m_refresh != -1.0f) ? m_refresh : 0.0f);
    }

    VideoDevice::VideoFormat QTMetalVideoDevice::format() const
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

    size_t QTMetalVideoDevice::width() const
    {
        if (!m_view) return 0;
        return static_cast<size_t>(m_view->width() * m_view->devicePixelRatio() + 0.5f);
    }

    size_t QTMetalVideoDevice::height() const
    {
        if (!m_view) return 0;
        return static_cast<size_t>(m_view->height() * m_view->devicePixelRatio() + 0.5f);
    }

    void QTMetalVideoDevice::open(const StringVector& /*args*/)
    {
        if (m_view)
            m_view->show();
        m_isOpen = true;
    }

    void QTMetalVideoDevice::close()
    {
        if (m_view)
            m_view->hide();
        m_isOpen = false;
    }

    bool QTMetalVideoDevice::isOpen() const
    {
        if (m_view)
            return m_view->isVisible();
        return false;
    }

} // namespace Rv

#endif // PLATFORM_DARWIN
