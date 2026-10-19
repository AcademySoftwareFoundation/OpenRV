//
//  Copyright (c) 2026 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//

#ifdef PLATFORM_DARWIN

#import <AppKit/AppKit.h>
#import <IOSurface/IOSurface.h>
#import <CoreVideo/CoreVideo.h>
#import <OpenGL/OpenGL.h>       // CGLGetCurrentContext, CGLError
#import <OpenGL/CGLIOSurface.h> // CGLTexImageIOSurface2D

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

#include <cassert>
#include <cmath>
#include <iostream>
#include <new>

namespace
{
    // FBO edge used while the view has no size yet.
    constexpr int fallbackFboSize = 128;

    // Frames to wait before retrying zero-copy interop (about 2 s at 60 Hz).
    constexpr int interopRetryFrames = 120;

    size_t toPhysicalPixels(int logical, float dpr) { return static_cast<size_t>(std::lround(static_cast<float>(logical) * dpr)); }

    //--------------------------------------------------------------------------
    // QTMetalSharedContextWorkerDevice
    //
    // GLVideoDevice for ImageRenderer's upload thread, which only calls
    // makeCurrent(). Its QOpenGLContext shares resources with the creating
    // QTMetalVideoDevice's context, so uploaded textures are visible to the
    // render thread. Qt requires QOffscreenSurface::create() on the GUI
    // thread, so both are created here; only makeCurrent() runs on the
    // upload thread (AA_DontCheckOpenGLContextThreadAffinity in main.cpp).
    //--------------------------------------------------------------------------
    class QTMetalSharedContextWorkerDevice : public TwkGLF::GLVideoDevice
    {
    public:
        QTMetalSharedContextWorkerDevice(const std::string& name, QOpenGLContext* shareContext)
            : TwkGLF::GLVideoDevice(nullptr, name, TwkApp::VideoDevice::NoCapabilities)
        {
            if (!shareContext)
            {
                return;
            }

            m_surface = std::make_unique<QOffscreenSurface>();
            m_surface->setFormat(shareContext->format());
            m_surface->create();
            if (!m_surface->isValid())
            {
                std::cerr << "[QTMetalVideoDevice] worker QOffscreenSurface::create() failed\n";
                m_surface.reset();
                return;
            }

            m_context = std::make_unique<QOpenGLContext>();
            m_context->setFormat(shareContext->format());
            m_context->setShareContext(shareContext);
            if (!m_context->create())
            {
                std::cerr << "[QTMetalVideoDevice] worker QOpenGLContext::create() failed\n";
                m_context.reset();
                m_surface.reset();
            }
        }

        ~QTMetalSharedContextWorkerDevice() override
        {
            // The context goes before the surface it was made current on.
            m_context.reset();
            m_surface.reset();
        }

        bool isValid() const { return m_surface && m_context; }

        // Does not call GLVideoDevice::makeCurrent(): its text-context
        // bookkeeping must not run on a background thread (see
        // QTGLVideoDevice's isWorkerDevice() guard).
        void makeCurrent() const override
        {
            if (!isValid())
            {
                return;
            }

            m_context->makeCurrent(m_surface.get());

            if (!m_versionChecked)
            {
                m_versionChecked = true;
                const char* ver = reinterpret_cast<const char*>(glGetString(GL_VERSION));
                if (!ver || !*ver)
                {
                    std::cerr << "[QTMetalVideoDevice] worker GL context has no version string\n";
                }
            }
        }

        size_t width() const override { return 0; }

        size_t height() const override { return 0; }

    private:
        std::unique_ptr<QOpenGLContext> m_context;
        std::unique_ptr<QOffscreenSurface> m_surface;
        mutable bool m_versionChecked{false};
    };

    // Empties the GL error queue and returns its first error, or GL_NO_ERROR.
    GLenum takeGLError()
    {
        const GLenum first = glGetError();
        while (glGetError() != GL_NO_ERROR)
        {
        }
        return first;
    }

    // Blits readFbo into drawFbo, flipping GL's bottom-left origin to the
    // IOSurface top-left origin. The GPU converts RGBA16F to RGB10_A2.
    void blitFlippedY(GLuint readFbo, GLuint drawFbo, int w, int h)
    {
        glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, readFbo);
        glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, drawFbo);
        glBlitFramebufferEXT(0, 0, w, h, 0, h, w, 0, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }

    // IPCore clears the background with alpha 0 and CA honours per-pixel
    // IOSurface alpha, so write alpha = 1 into the bound draw FBO, leaving RGB.
    void forceOpaqueAlpha()
    {
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }

    // Returns a GL_TEXTURE_RECTANGLE_ARB texture whose storage is the
    // IOSurface's memory, or 0 on failure. The IOSurface format must match
    // GL_BGRA + GL_UNSIGNED_INT_2_10_10_10_REV.
    GLuint createIOSurfaceTexture(CGLContextObj cgl, IOSurfaceRef surface, int w, int h)
    {
        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tex);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        const CGLError cglErr = CGLTexImageIOSurface2D(cgl, GL_TEXTURE_RECTANGLE_ARB, GL_RGB10_A2, w, h, GL_BGRA,
                                                       GL_UNSIGNED_INT_2_10_10_10_REV, surface, 0);
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

        if (cglErr != kCGLNoError)
        {
            std::cerr << "[QTMetalVideoDevice] CGLTexImageIOSurface2D failed: " << CGLErrorString(cglErr)
                      << "; CPU fallback\n";
            glDeleteTextures(1, &tex);
            return 0;
        }
        return tex;
    }
} // namespace

namespace Rv
{
    using namespace TwkApp;

    QTMetalVideoDevice::QTMetalVideoDevice(VideoModule* module, const std::string& name, MetalView* view,
                                           QWidget* eventWidget)
        : TwkGLF::GLVideoDevice(module, name,
                                VideoDevice::ImageOutput | VideoDevice::ProvidesSync | VideoDevice::SubWindow)
        , m_view(view)
        , m_translator(eventWidget ? std::make_unique<QTTranslator>(this, eventWidget) : nullptr)
    {
        assert(view);
    }

    QTMetalVideoDevice::~QTMetalVideoDevice()
    {
        // GL objects need the context current; the context goes last.
        if (m_glContext && (m_fbo || m_fboColorTex || m_ioSlots[0].fbo || m_cpuFlipFbo))
        {
            m_glContext->makeCurrent(m_offscreenSurface.get());
            releaseRenderFbo();
            cleanupIOSurfaceTextures();
            cleanupCpuFallbackTarget();
            m_glContext->doneCurrent();
        }

        m_offscreenSurface.reset();
        m_glContext.reset();
    }

    void QTMetalVideoDevice::setEventWidget(QWidget* widget)
    {
        m_translator = widget ? std::make_unique<QTTranslator>(this, widget) : nullptr;
    }

    //--------------------------------------------------------------------------

    bool QTMetalVideoDevice::createOffscreenContext() const
    {
        // GL 2.1 with no profile (compatibility): IPCore uses legacy APIs
        // (glPushAttrib, GL_LIGHTING) and GLSL 1.20 with texture2DRect().
        QSurfaceFormat fmt;
        fmt.setRenderableType(QSurfaceFormat::OpenGL);
        fmt.setMajorVersion(2);
        fmt.setMinorVersion(1);

        m_glContext = std::make_unique<QOpenGLContext>();
        m_glContext->setFormat(fmt);

        // Share with the global context (Qt::AA_ShareOpenGLContexts) so the
        // diagnostics QOpenGLWidget can use textures uploaded here.
        m_glContext->setShareContext(QOpenGLContext::globalShareContext());

        if (!m_glContext->create())
        {
            std::cerr << "[QTMetalVideoDevice] QOpenGLContext::create() failed\n";
            m_glContext.reset();
            return false;
        }

        // The surface format must come from the actual context format, not the requested one.
        m_offscreenSurface = std::make_unique<QOffscreenSurface>();
        m_offscreenSurface->setFormat(m_glContext->format());
        m_offscreenSurface->create();

        if (!m_offscreenSurface->isValid())
        {
            std::cerr << "[QTMetalVideoDevice] QOffscreenSurface::create() failed\n";
            m_offscreenSurface.reset();
            m_glContext.reset();
            return false;
        }
        return true;
    }

    void QTMetalVideoDevice::releaseRenderFbo() const
    {
        // GLFBO::attachColorTexture does not take ownership of the texture.
        m_fbo.reset();
        if (m_fboColorTex)
        {
            glDeleteTextures(1, &m_fboColorTex);
            m_fboColorTex = 0;
        }
        m_fboWidth = 0;
        m_fboHeight = 0;
    }

    bool QTMetalVideoDevice::ensureRenderFbo(int w, int h) const
    {
        if (m_fbo && m_fboWidth == w && m_fboHeight == h)
        {
            return true;
        }

        releaseRenderFbo();

        // The control view composites blended passes here, so it needs RGBA16F.
        // A presentation output only receives already-composited blits, so
        // RGB10_A2 (the IOSurface format) halves its bandwidth.
        const bool passiveOutput = m_view && m_view->isPresentationMode();
        const GLenum fboFormat = passiveOutput ? GL_RGB10_A2 : GL_RGBA16F_ARB;
        const GLenum fboType = passiveOutput ? GL_UNSIGNED_INT_2_10_10_10_REV : GL_FLOAT;

        // GL_RGBA16F_ARB is a valid texture format on macOS GL 2.1 but not a
        // valid renderbuffer format, so attach a rectangle texture.
        glGenTextures(1, &m_fboColorTex);
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_fboColorTex);
        glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, fboFormat, w, h, 0, GL_RGBA, fboType, nullptr);
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

        m_fbo = std::make_unique<TwkGLF::GLFBO>(w, h, fboFormat);
        m_fbo->attachColorTexture(GL_TEXTURE_RECTANGLE_ARB, m_fboColorTex);

        const GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
        if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
        {
            std::cerr << "[QTMetalVideoDevice] FBO incomplete: 0x" << std::hex << status << std::dec << "\n";
            releaseRenderFbo();
            return false;
        }

        m_fboWidth = w;
        m_fboHeight = h;
        // The IOSurface ring is rebuilt at the new size, so retry interop now.
        m_interopDisabled = false;
        return true;
    }

    bool QTMetalVideoDevice::ensureGLContext() const
    {
        m_glContextReady = false;

        if (!m_glContext && !createOffscreenContext())
        {
            return false;
        }

        if (!m_glContext->makeCurrent(m_offscreenSurface.get()))
        {
            std::cerr << "[QTMetalVideoDevice] makeCurrent() failed\n";
            return false;
        }

        // The FBO is in physical pixels to match the IOSurface (bounds x DPR).
        // Qt events stay in logical pixels through setRelativeDomain().
        int newW = static_cast<int>(width());
        int newH = static_cast<int>(height());
        if (newW < 1)
        {
            newW = fallbackFboSize;
        }
        if (newH < 1)
        {
            newH = fallbackFboSize;
        }

        if (!ensureRenderFbo(newW, newH))
        {
            return false;
        }

        m_fbo->bind();

        // GLVideoDevice bookkeeping (GLtext context, etc.)
        GLVideoDevice::makeCurrent();

        m_glContextReady = true;
        return true;
    }

    //--------------------------------------------------------------------------

    void QTMetalVideoDevice::setAbsolutePosition(int x, int y)
    {
        if (x != m_x || y != m_y || m_refresh == -1.0f)
        {
            float refresh = -1.0f;

            const int w = m_view ? m_view->width() : 0;
            const int h = m_view ? m_view->height() : 0;
            const int tx = x + (w / 2);
            const int ty = y + (h / 2);

            if (const TwkApp::VideoModule* mod = TwkApp::App()->primaryVideoModule())
            {
                if (TwkApp::VideoDevice* d = mod->deviceFromPosition(tx, ty))
                {
                    setPhysicalDevice(d);
                    refresh = d->timing().hz;

                    const VideoDeviceContextChangeEvent event("video-device-changed", this, this, d);
                    sendEvent(event);
                }
            }

            if (refresh != m_refresh)
            {
                if (refresh > 0)
                {
                    m_refresh = refresh;
                }
                else if (IPCore::debugPlayback)
                {
                    std::cout << "WARNING: ignoring intended desktop refresh rate = " << refresh << std::endl;
                }

                if (IPCore::debugPlayback)
                {
                    std::cout << "INFO: new desktop refresh rate " << m_refresh << std::endl;
                }
            }
        }
        m_x = x;
        m_y = y;
    }

    void QTMetalVideoDevice::setPhysicalDevice(VideoDevice* d)
    {
        TwkApp::VideoDevice::setPhysicalDevice(d);

        m_devicePixelRatio = 1.0f;

        static const bool noQtHighDPISupport = (getenv("RV_NO_QT_HDPI_SUPPORT") != nullptr);
        if (noQtHighDPISupport)
        {
            return;
        }

        if (const auto* desktopDev = dynamic_cast<const DesktopVideoDevice*>(d))
        {
            const QList<QScreen*> screens = QGuiApplication::screens();
            if (desktopDev->qtScreen() < screens.size())
            {
                m_devicePixelRatio = static_cast<float>(screens[desktopDev->qtScreen()]->devicePixelRatio());
            }
        }
    }

    float QTMetalVideoDevice::devicePixelRatio() const
    {
        // Read the widget's DPR, not m_devicePixelRatio (1.0 until
        // setPhysicalDevice()), so the FBO and IPCore viewport agree from the first frame.
        if (m_view)
        {
            return m_view->devicePixelRatio();
        }
        return m_devicePixelRatio;
    }

    //--------------------------------------------------------------------------
    // GLVideoDevice API
    //--------------------------------------------------------------------------

    void QTMetalVideoDevice::makeCurrent() const { ensureGLContext(); }

    TwkGLF::GLVideoDevice* QTMetalVideoDevice::newSharedContextWorkerDevice() const
    {
        // ImageRenderer::setupUploadThread() calls this on the render thread and
        // re-establishes its own makeCurrent() afterwards, so binding here is safe.
        if (!ensureGLContext())
        {
            std::cerr << "[QTMetalVideoDevice] newSharedContextWorkerDevice: "
                         "offscreen GL context unavailable\n";
            return nullptr;
        }
        auto worker = std::make_unique<QTMetalSharedContextWorkerDevice>(name() + "-workerContextDevice", m_glContext.get());
        if (!worker->isValid())
        {
            return nullptr;
        }
        return worker.release();
    }

    TwkGLF::GLFBO* QTMetalVideoDevice::defaultFBO() { return ensureGLContext() ? m_fbo.get() : nullptr; }

    const TwkGLF::GLFBO* QTMetalVideoDevice::defaultFBO() const { return ensureGLContext() ? m_fbo.get() : nullptr; }

    GLuint QTMetalVideoDevice::fboID() const { return m_fbo ? m_fbo->fboID() : 0; }

    std::string QTMetalVideoDevice::hardwareIdentification() const { return "metal-hybrid"; }

    //--------------------------------------------------------------------------
    // Zero-copy IOSurface present ring
    //--------------------------------------------------------------------------

    void QTMetalVideoDevice::latchInteropFailure() const
    {
        m_interopDisabled = true;
        m_interopRetryCountdown = interopRetryFrames;
    }

    bool QTMetalVideoDevice::ensureIOSurfaceTextures(int w, int h) const
    {
        static const bool forceCpu = getenv("RV_METAL_FORCE_CPU_PRESENT") != nullptr;
        if (forceCpu)
        {
            return false;
        }

        // The ring is rebuilt on a size change, so that is always a fresh attempt.
        if (m_sharedWidth != w || m_sharedHeight != h)
        {
            m_interopDisabled = false;
        }

        if (m_interopDisabled)
        {
            if (--m_interopRetryCountdown > 0)
            {
                return false;
            }
            m_interopDisabled = false;
        }

        if (m_sharedWidth == w && m_sharedHeight == h && m_ioSlots[0].fbo)
        {
            return true;
        }

        cleanupIOSurfaceTextures();

        const auto fail = [this]
        {
            cleanupIOSurfaceTextures();
            latchInteropFailure();
            return false;
        };

        // The caller made the Qt context current; CGLTexImageIOSurface2D binds into its CGL context.
        CGLContextObj cgl = CGLGetCurrentContext();
        if (!cgl)
        {
            std::cerr << "[QTMetalVideoDevice] CGLGetCurrentContext() returned null; "
                         "falling back to CPU readback\n";
            return fail();
        }

        // Each slot is filled as it is built, so cleanupIOSurfaceTextures() releases partial work.
        for (IOSurfaceSlot& slot : m_ioSlots)
        {
            slot.surface = createArgb2101010Surface(w, h);
            if (!slot.surface)
            {
                std::cerr << "[QTMetalVideoDevice] IOSurfaceCreate failed " << w << "x" << h << "; CPU fallback\n";
                return fail();
            }

            slot.texture = createIOSurfaceTexture(cgl, static_cast<IOSurfaceRef>(slot.surface), w, h);
            if (!slot.texture)
            {
                return fail();
            }

            slot.fbo = std::make_unique<TwkGLF::GLFBO>(w, h, GL_RGB10_A2);
            slot.fbo->attachColorTexture(GL_TEXTURE_RECTANGLE_ARB, slot.texture);

            const GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
            if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
            {
                std::cerr << "[QTMetalVideoDevice] IOSurface FBO incomplete: 0x" << std::hex << status << std::dec
                          << "; CPU fallback\n";
                return fail();
            }
        }

        m_sharedWidth = w;
        m_sharedHeight = h;
        m_ringIndex = 0;
        return true;
    }

    void QTMetalVideoDevice::cleanupIOSurfaceTextures() const
    {
        // Needs the GL context current (GLFBO, glDeleteTextures).
        for (IOSurfaceSlot& slot : m_ioSlots)
        {
            slot.fbo.reset();

            if (slot.texture)
            {
                glDeleteTextures(1, &slot.texture);
                slot.texture = 0;
            }
            if (slot.surface)
            {
                CFRelease(static_cast<IOSurfaceRef>(slot.surface));
                slot.surface = nullptr;
            }
        }
        m_sharedWidth = 0;
        m_sharedHeight = 0;
        m_ringIndex = 0;
    }

    //--------------------------------------------------------------------------
    // CPU-fallback flip FBO
    //--------------------------------------------------------------------------

    void QTMetalVideoDevice::ensureCpuFallbackTarget(int w, int h) const
    {
        if (m_cpuFlipFbo && m_cpuFlipWidth == w && m_cpuFlipHeight == h)
        {
            return;
        }

        cleanupCpuFallbackTarget();

        glGenTextures(1, &m_cpuFlipTex);
        glBindTexture(GL_TEXTURE_2D, m_cpuFlipTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB10_A2, w, h, 0, GL_BGRA, GL_UNSIGNED_INT_2_10_10_10_REV, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);

        glGenFramebuffersEXT(1, &m_cpuFlipFbo);
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_cpuFlipFbo);
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, m_cpuFlipTex, 0);
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

        m_cpuFlipWidth = w;
        m_cpuFlipHeight = h;
    }

    void QTMetalVideoDevice::cleanupCpuFallbackTarget() const
    {
        if (m_cpuFlipFbo)
        {
            glDeleteFramebuffersEXT(1, &m_cpuFlipFbo);
            m_cpuFlipFbo = 0;
        }
        if (m_cpuFlipTex)
        {
            glDeleteTextures(1, &m_cpuFlipTex);
            m_cpuFlipTex = 0;
        }
        m_cpuFlipWidth = 0;
        m_cpuFlipHeight = 0;
    }

    //--------------------------------------------------------------------------
    // syncBuffers: GL FBO -> IOSurface -> CALayer
    //--------------------------------------------------------------------------

    bool QTMetalVideoDevice::presentZeroCopy(int w, int h) const
    {
        const int slot = m_ringIndex;
        const IOSurfaceSlot& target = m_ioSlots[slot];

        // session->render() can leave errors pending; clear them so the check
        // below sees only the blit's.
        takeGLError();

        blitFlippedY(m_fbo->fboID(), target.fbo->fboID(), w, h);
        if (const GLenum error = takeGLError(); error != GL_NO_ERROR)
        {
            std::cerr << "[QTMetalVideoDevice] glBlitFramebufferEXT failed (GL 0x" << std::hex << error << std::dec
                      << "); falling back to CPU readback\n";
            latchInteropFailure();
            return false;
        }

        forceOpaqueAlpha();

        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_fbo->fboID());

        // glFlush publishes the writes to the CA render server; glFinish would
        // stall the main thread, and IOSurface synchronises across contexts.
        glFlush();

        m_view->presentIOSurface(target.surface);

        m_ringIndex = (slot + 1) % ringSize;

        if (m_cpuFallbackLogged)
        {
            std::cerr << "INFO: [QTMetalVideoDevice] zero-copy IOSurface interop restored.\n";
            m_cpuFallbackLogged = false;
        }
        return true;
    }

    void QTMetalVideoDevice::presentViaCpuReadback(int w, int h) const
    {
        if (!m_cpuFallbackLogged)
        {
            std::cerr << "INFO: [QTMetalVideoDevice] presenting via CPU readback (" << w << "x" << h
                      << "); zero-copy IOSurface interop unavailable, retrying periodically.\n";
            m_cpuFallbackLogged = true;
        }

        ensureCpuFallbackTarget(w, h);

        blitFlippedY(m_fbo->fboID(), m_cpuFlipFbo, w, h);
        forceOpaqueAlpha();

        // On GL-on-Metal glReadPixels does not wait for the blit; glFinish commits it.
        glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, m_cpuFlipFbo);
        glFinish();

        try
        {
            m_cpuPackedScratch.resize(static_cast<size_t>(w) * h);
        }
        catch (const std::bad_alloc&)
        {
            std::cerr << "ERROR: [QTMetalVideoDevice] bad_alloc in CPU fallback (" << w << "x" << h << ")\n";
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_fbo->fboID());
            return;
        }

        // GL_BGRA + GL_UNSIGNED_INT_2_10_10_10_REV reads kCVPixelFormatType_ARGB2101010LEPacked.
        glReadPixels(0, 0, w, h, GL_BGRA, GL_UNSIGNED_INT_2_10_10_10_REV, m_cpuPackedScratch.data());

        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_fbo->fboID());
        m_view->presentPixelData(m_cpuPackedScratch.data(), w, h);
    }

    void QTMetalVideoDevice::syncBuffers() const
    {
        if (!m_view || !m_glContext || !m_fbo)
        {
            return;
        }

        const int w = static_cast<int>(m_fbo->width());
        const int h = static_cast<int>(m_fbo->height());
        if (w <= 0 || h <= 0)
        {
            return;
        }

        if (!m_glContext->makeCurrent(m_offscreenSurface.get()))
        {
            return;
        }

        if (ensureIOSurfaceTextures(w, h) && presentZeroCopy(w, h))
        {
            return;
        }

        presentViaCpuReadback(w, h);
    }

    //--------------------------------------------------------------------------
    // VideoDevice API
    //--------------------------------------------------------------------------

    void QTMetalVideoDevice::redraw() const
    {
        // Coalesced into one render per event-loop cycle, like QWidget::update() on the GL path.
        if (m_view)
        {
            m_view->requestUpdate();
        }
    }

    void QTMetalVideoDevice::redrawImmediately() const
    {
        if (!m_view)
        {
            return;
        }

        if (m_view->isVisible())
        {
            // Keep playback alive when the window is fully occluded (see QTGLVideoDevice::redrawImmediately()).
            m_view->setAttribute(Qt::WA_Mapped);
            m_view->renderImmediately();
        }
        else
        {
            redraw();
        }
    }

    VideoDevice::Resolution QTMetalVideoDevice::resolution() const { return {width(), height(), 1.0f, 1.0f}; }

    VideoDevice::Offset QTMetalVideoDevice::offset() const { return {m_x, m_y}; }

    VideoDevice::Timing QTMetalVideoDevice::timing() const { return {(m_refresh != -1.0f) ? m_refresh : 0.0f}; }

    VideoDevice::VideoFormat QTMetalVideoDevice::format() const
    {
        return {width(), height(), 1.0f, 1.0f, timing().hz, hardwareIdentification()};
    }

    size_t QTMetalVideoDevice::width() const { return m_view ? toPhysicalPixels(m_view->width(), m_view->devicePixelRatio()) : 0; }

    size_t QTMetalVideoDevice::height() const { return m_view ? toPhysicalPixels(m_view->height(), m_view->devicePixelRatio()) : 0; }

    void QTMetalVideoDevice::open(const StringVector& /*args*/)
    {
        if (m_view)
        {
            m_view->show();
        }
    }

    void QTMetalVideoDevice::close()
    {
        if (m_view)
        {
            m_view->hide();
        }
    }

    bool QTMetalVideoDevice::isOpen() const { return m_view && m_view->isVisible(); }

} // namespace Rv

#endif // PLATFORM_DARWIN
