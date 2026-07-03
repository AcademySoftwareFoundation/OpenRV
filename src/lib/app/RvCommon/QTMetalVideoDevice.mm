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

#include <algorithm>
#include <cstring>
#include <iostream>
#include <new>

namespace
{
    //--------------------------------------------------------------------------
    // QTMetalSharedContextWorkerDevice
    //
    // Minimal GLVideoDevice for ImageRenderer's threaded-upload path (see
    // ImageRenderer::uploadThreadTrampoline()), which only ever calls
    // makeCurrent() on the device returned by newSharedContextWorkerDevice().
    // It owns its own QOpenGLContext + QOffscreenSurface, sharing GL resources
    // (textures, etc.) with the QTMetalVideoDevice's offscreen context that
    // created it, so textures uploaded on the worker thread are visible to the
    // render thread.  Deliberately does not reuse QTMetalVideoDevice itself —
    // that class builds a view-sized FBO/IOSurface ring that the upload thread
    // has no use for.
    //
    // Qt requires QOffscreenSurface::create() on the GUI thread. The worker
    // QOpenGLContext is also created there (before the upload thread starts);
    // only makeCurrent() runs on the upload thread
    // (AA_DontCheckOpenGLContextThreadAffinity in main.cpp).
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

            m_surface = new QOffscreenSurface();
            m_surface->setFormat(shareContext->format());
            m_surface->create();
            if (!m_surface->isValid())
            {
                std::cerr << "[QTMetalVideoDevice] worker QOffscreenSurface::create() failed\n";
                delete m_surface;
                m_surface = nullptr;
                return;
            }

            m_context = new QOpenGLContext();
            m_context->setFormat(shareContext->format());
            m_context->setShareContext(shareContext);
            if (!m_context->create())
            {
                std::cerr << "[QTMetalVideoDevice] worker QOpenGLContext::create() failed\n";
                delete m_context;
                m_context = nullptr;
                delete m_surface;
                m_surface = nullptr;
            }
        }

        ~QTMetalSharedContextWorkerDevice() override
        {
            // Surface lifetime is tied to ImageRenderer on the GUI/render thread.
            delete m_context;
            delete m_surface;
        }

        bool isValid() const { return m_surface && m_context; }

        // Deliberately does NOT call GLVideoDevice::makeCurrent() (text-context
        // bookkeeping there is not meant to be touched concurrently from a
        // background thread) — mirrors QTGLVideoDevice's isWorkerDevice() guard.
        void makeCurrent() const override
        {
            if (!m_context || !m_surface)
            {
                return;
            }

            m_context->makeCurrent(m_surface);

            if (!m_versionChecked)
            {
                m_versionChecked = true;
                const char* ver = reinterpret_cast<const char*>(glGetString(GL_VERSION));
                m_glValid = (ver && *ver);
                if (!m_glValid)
                {
                    std::cerr << "[QTMetalVideoDevice] worker GL context has no version string\n";
                }
            }
        }

        size_t width() const override { return 0; }

        size_t height() const override { return 0; }

    private:
        mutable QOpenGLContext* m_context = nullptr;
        QOffscreenSurface* m_surface = nullptr;
        mutable bool m_versionChecked = false;
        mutable bool m_glValid = false;
    };
} // namespace

namespace Rv
{
    using namespace std;
    using namespace TwkApp;

    QTMetalVideoDevice::QTMetalVideoDevice(VideoModule* module,
                                           const string& name,
                                           MetalView* view,
                                           QWidget* eventWidget)
        : TwkGLF::GLVideoDevice(module, name,
                                 VideoDevice::ImageOutput | VideoDevice::ProvidesSync | VideoDevice::SubWindow)
        , m_view(view)
        , m_eventWidget(eventWidget)
        , m_translator(eventWidget ? new QTTranslator(this, eventWidget) : nullptr)
    {
        assert(view);
    }

    QTMetalVideoDevice::~QTMetalVideoDevice()
    {
        // Delete the FBO and its colour texture while the GL context is current.
        // GLFBO::attachColorTexture passes owner=false so GLFBO does NOT delete
        // the colour texture — we must do it explicitly here.
        if (m_glContext && (m_fbo || m_fboColorTex || m_ioFbos[0] || m_cpuFlipFbo))
        {
            m_glContext->makeCurrent(m_offscreenSurface);
            delete m_fbo;
            m_fbo = nullptr;
            if (m_fboColorTex)
            {
                glDeleteTextures(1, &m_fboColorTex);
                m_fboColorTex = 0;
            }
            cleanupIOSurfaceTextures();
            cleanupCpuFallbackTarget();
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

    bool QTMetalVideoDevice::ensureGLContext() const
    {
        m_glContextReady = false;

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
            // and replaceTextureCalls() uses texture2DRect() which
            // is available with GL_ARB_texture_rectangle.
            fmt.setMajorVersion(2);
            fmt.setMinorVersion(1);
            // No setProfile() — defaults to NoProfile (Compatibility) on macOS.

            m_glContext = new QOpenGLContext();
            m_glContext->setFormat(fmt);

            // Share with the process-wide global context (enabled via
            // Qt::AA_ShareOpenGLContexts in main) so the diagnostics
            // QOpenGLWidget can use the textures uploaded by this offscreen
            // render context.
            m_glContext->setShareContext(QOpenGLContext::globalShareContext());

            if (!m_glContext->create())
            {
                std::cerr << "[QTMetalVideoDevice] QOpenGLContext::create() failed\n";
                delete m_glContext;
                m_glContext = nullptr;
                return false;
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
                return false;
            }
        }

        if (!m_glContext->makeCurrent(m_offscreenSurface))
        {
            std::cerr << "[QTMetalVideoDevice] makeCurrent() failed\n";
            return false;
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
            {
                std::cerr << "[QTMetalVideoDevice] FBO incomplete: 0x"
                          << std::hex << status << std::dec << "\n";
                delete m_fbo;
                m_fbo = nullptr;
                if (m_fboColorTex)
                {
                    glDeleteTextures(1, &m_fboColorTex);
                    m_fboColorTex = 0;
                }
                m_fboWidth  = 0;
                m_fboHeight = 0;
                return false;
            }

            m_fboWidth  = newW;
            m_fboHeight = newH;
            // IOSurface interop ring must be rebuilt at the new size; allow a
            // fresh attempt even if a prior transient failure latched interop off.
            m_interopDisabled = false;
        }

        if (!m_fbo)
            return false;

        // Bind FBO so all GL rendering goes into it
        m_fbo->bind();

        // Let GLVideoDevice do its bookkeeping (GLtext context, etc.)
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
        if (!ensureGLContext())
            return;
    }

    TwkGLF::GLVideoDevice* QTMetalVideoDevice::newSharedContextWorkerDevice() const
    {
        // Make sure our own offscreen context exists before sharing with it.
        // ImageRenderer::setupUploadThread() calls this from the render thread
        // before spawning the upload thread, and immediately re-establishes its
        // own makeCurrent() afterward, so it's safe to (re)bind our context here.
        if (!ensureGLContext())
        {
            std::cerr << "[QTMetalVideoDevice] newSharedContextWorkerDevice: "
                         "offscreen GL context unavailable\n";
            return nullptr;
        }
        auto* worker = new QTMetalSharedContextWorkerDevice(name() + "-workerContextDevice", m_glContext);
        if (!worker->isValid())
        {
            delete worker;
            return nullptr;
        }
        return worker;
    }

    TwkGLF::GLFBO* QTMetalVideoDevice::defaultFBO()
    {
        if (!ensureGLContext())
            return nullptr;
        return m_fbo;
    }

    const TwkGLF::GLFBO* QTMetalVideoDevice::defaultFBO() const
    {
        if (!ensureGLContext())
            return nullptr;
        return m_fbo;
    }

    std::string QTMetalVideoDevice::hardwareIdentification() const
    {
        return "metal-hybrid";
    }

    //--------------------------------------------------------------------------
    // ensureIOSurfaceTextures — (re)build the zero-copy IOSurface present ring
    //--------------------------------------------------------------------------

    void QTMetalVideoDevice::latchInteropFailure() const
    {
        m_interopDisabled = true;
        m_interopRetryCountdown = kInteropRetryFrames;
    }

    bool QTMetalVideoDevice::ensureIOSurfaceTextures(int w, int h) const
    {
        // A size change always warrants a fresh attempt (the ring is rebuilt at
        // the new size anyway).
        if (m_sharedWidth != w || m_sharedHeight != h)
            m_interopDisabled = false;

        if (m_interopDisabled)
        {
            // Don't disable the zero-copy path for the lifetime of the window:
            // the failure may have been transient.  Count down and retry once
            // the cooldown elapses instead of falling back to CPU readback
            // forever.
            if (--m_interopRetryCountdown > 0)
                return false;
            m_interopDisabled = false;
        }

        if (m_sharedWidth == w && m_sharedHeight == h && m_ioFbos[0])
            return true;  // ring already matches the current size

        cleanupIOSurfaceTextures();

        // The Qt context is current (caller does makeCurrent first); its native
        // CGL context is what CGLTexImageIOSurface2D binds the IOSurface into.
        CGLContextObj cgl = CGLGetCurrentContext();
        if (!cgl)
        {
            std::cerr << "[QTMetalVideoDevice] CGLGetCurrentContext() returned null "
                         "— falling back to CPU readback\n";
            latchInteropFailure();
            return false;
        }

        for (int i = 0; i < kRingSize; ++i)
        {
            // IOSurface pixel format must match the GL (format,type) below.
            // ARGB2101010LE matches GL_BGRA + GL_UNSIGNED_INT_2_10_10_10_REV.
            NSDictionary* props = @{
                (NSString*)kIOSurfaceWidth:           @(w),
                (NSString*)kIOSurfaceHeight:          @(h),
                (NSString*)kIOSurfaceBytesPerElement: @(4),
                (NSString*)kIOSurfacePixelFormat:     @(kCVPixelFormatType_ARGB2101010LEPacked),
            };
            IOSurfaceRef surf = IOSurfaceCreate((__bridge CFDictionaryRef)props);
            if (!surf)
            {
                std::cerr << "[QTMetalVideoDevice] IOSurfaceCreate failed "
                          << w << "x" << h << " — CPU fallback\n";
                cleanupIOSurfaceTextures();
                latchInteropFailure();
                return false;
            }

            GLuint tex = 0;
            glGenTextures(1, &tex);
            glBindTexture(GL_TEXTURE_RECTANGLE_ARB, tex);
            glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            // Alias the IOSurface's memory as the texture's storage — GPU writes
            // to this texture land directly in the IOSurface the CALayer shows.
            CGLError cglErr = CGLTexImageIOSurface2D(
                cgl, GL_TEXTURE_RECTANGLE_ARB, GL_RGB10_A2,
                w, h, GL_BGRA, GL_UNSIGNED_INT_2_10_10_10_REV,
                surf, 0);
            glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

            if (cglErr != kCGLNoError)
            {
                std::cerr << "[QTMetalVideoDevice] CGLTexImageIOSurface2D failed: "
                          << CGLErrorString(cglErr) << " — CPU fallback\n";
                glDeleteTextures(1, &tex);
                CFRelease(surf);
                cleanupIOSurfaceTextures();
                latchInteropFailure();
                return false;
            }

            // Wrap the IOSurface-backed texture in a GLFBO so we can blit into it.
            // owner=false (attachColorTexture) — we delete the texture ourselves.
            TwkGLF::GLFBO* iofbo = new TwkGLF::GLFBO(w, h, GL_RGB10_A2);
            iofbo->attachColorTexture(GL_TEXTURE_RECTANGLE_ARB, tex);

            GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
            if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
            {
                std::cerr << "[QTMetalVideoDevice] IOSurface FBO incomplete: 0x"
                          << std::hex << status << std::dec << " — CPU fallback\n";
                delete iofbo;
                glDeleteTextures(1, &tex);
                CFRelease(surf);
                cleanupIOSurfaceTextures();
                latchInteropFailure();
                return false;
            }

            m_ioSurfaces[i] = (void*)surf;  // retained; released in cleanup
            m_ioTextures[i] = tex;
            m_ioFbos[i]     = iofbo;
        }

        m_sharedWidth  = w;
        m_sharedHeight = h;
        m_ringIndex    = 0;
        return true;
    }

    void QTMetalVideoDevice::cleanupIOSurfaceTextures() const
    {
        // Must be called with the GL context current (GLFBO / glDeleteTextures).
        for (int i = 0; i < kRingSize; ++i)
        {
            delete m_ioFbos[i];
            m_ioFbos[i] = nullptr;

            if (m_ioTextures[i])
            {
                glDeleteTextures(1, &m_ioTextures[i]);
                m_ioTextures[i] = 0;
            }
            if (m_ioSurfaces[i])
            {
                CFRelease((IOSurfaceRef)m_ioSurfaces[i]);
                m_ioSurfaces[i] = nullptr;
            }
        }
        m_sharedWidth  = 0;
        m_sharedHeight = 0;
        m_ringIndex    = 0;
    }

    //--------------------------------------------------------------------------
    // CPU-fallback flip FBO helpers
    //--------------------------------------------------------------------------

    void QTMetalVideoDevice::ensureCpuFallbackTarget(int w, int h) const
    {
        if (m_cpuFlipFbo && m_cpuFlipWidth == w && m_cpuFlipHeight == h)
            return;

        cleanupCpuFallbackTarget();

        // GL_RGB10_A2 target: GPU Y-flip blit quantises RGBA16F → 10-bit.
        // glReadPixels(GL_BGRA, GL_UNSIGNED_INT_2_10_10_10_REV) reads
        // ARGB2101010LE directly — matching kCVPixelFormatType_ARGB2101010LEPacked.
        glGenTextures(1, &m_cpuFlipTex);
        glBindTexture(GL_TEXTURE_2D, m_cpuFlipTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB10_A2, w, h, 0,
                     GL_BGRA, GL_UNSIGNED_INT_2_10_10_10_REV, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);

        glGenFramebuffersEXT(1, &m_cpuFlipFbo);
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_cpuFlipFbo);
        glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                                  GL_TEXTURE_2D, m_cpuFlipTex, 0);
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

        m_cpuFlipWidth  = w;
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
        m_cpuFlipWidth  = 0;
        m_cpuFlipHeight = 0;
    }

    //--------------------------------------------------------------------------
    // syncBuffers — zero-copy GL FBO -> IOSurface blit -> CALayer present
    //               (CPU readback retained only as a fallback)
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

        if (w <= 0 || h <= 0)
            return;

        // Make the GL context current before calling GL functions
        if (!m_glContext->makeCurrent(m_offscreenSurface))
            return;

        // --- Fast path: zero-copy GL -> IOSurface (no glFinish, no readback) ---
        if (ensureIOSurfaceTextures(w, h))
        {
            const int slot = m_ringIndex;
            TwkGLF::GLFBO* ioFbo = m_ioFbos[slot];
            bool blitOk = true;

            // GPU blit from the RGBA16F render FBO into the IOSurface-backed
            // RGB10_A2 texture.  The GPU does the float->10-bit conversion.
            // Swapped destination Y (h -> 0) flips GL's bottom-left origin to the
            // IOSurface/CALayer top-left origin — no CPU y-flip needed.
            glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, fbo->fboID());
            glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, ioFbo->fboID());
            glBlitFramebufferEXT(0, 0, w, h,
                                 0, h, w, 0,
                                 GL_COLOR_BUFFER_BIT, GL_NEAREST);
            if (glGetError() != GL_NO_ERROR)
            {
                std::cerr << "[QTMetalVideoDevice] glBlitFramebufferEXT failed — "
                             "falling back to CPU readback\n";
                latchInteropFailure();
                blitOk = false;
            }

            if (blitOk)
            {
                // Force the IOSurface alpha channel to fully opaque.  IPCore clears
                // the viewport background with alpha 0 (ImageRenderer::
                // clearBackgroundToBlack), and the blit copies that through; the CA
                // compositor honours the IOSurface's per-pixel alpha (despite the
                // layer's opaque=YES hint), so a transparent background would be
                // blended against the gray window backdrop instead of showing black.
                // glClear respects glColorMask, so this writes only alpha (2-bit ->
                // 3 = opaque) and leaves the blitted RGB untouched — the GPU analog
                // of the CPU fallback's hard-coded (3u << 30) alpha.
                glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
                glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);
                glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

                // Restore the render FBO as the active target.
                glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo->fboID());

                // Publish the GL writes to the CA render server.  glFlush (not
                // glFinish) is enough — it does not stall the main thread waiting for
                // the GPU to go idle, and IOSurface provides the cross-context
                // synchronisation the compositor needs.  This is what removes the
                // per-frame main-thread stall behind menus and keyboard shortcuts.
                glFlush();

                m_view->presentIOSurface(m_ioSurfaces[slot]);

                m_ringIndex = (slot + 1) % kRingSize;

                // Recovery telemetry: report once when the zero-copy path resumes
                // after a stretch of CPU-readback presents.
                if (m_cpuFallbackLogged)
                {
                    std::cerr << "INFO: [QTMetalVideoDevice] zero-copy IOSurface "
                                 "interop restored.\n";
                    m_cpuFallbackLogged = false;
                }
                return;
            }
        }

        // --- Fallback: GL readback -> CPU pack -> IOSurface upload ---
        // Used only when IOSurface GL interop is unavailable on this system.
        // Log once on entry (not every frame) so the slow path is visible
        // without spamming; the recovery message above closes the loop.
        if (!m_cpuFallbackLogged)
        {
            std::cerr << "INFO: [QTMetalVideoDevice] presenting via CPU readback "
                         "(" << w << "x" << h << "); zero-copy IOSurface interop "
                         "unavailable — retrying periodically.\n";
            m_cpuFallbackLogged = true;
        }

        ensureCpuFallbackTarget(w, h);

        // Y-flip blit (GL bottom-left → IOSurface top-left) into the RGB10_A2
        // target. The GPU does the RGBA16F → 10-bit quantisation in one pass.
        glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, fbo->fboID());
        glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, m_cpuFlipFbo);
        glBlitFramebufferEXT(0, 0, w, h,  0, h, w, 0,  GL_COLOR_BUFFER_BIT, GL_NEAREST);

        // Force alpha to fully opaque (A=3, 2-bit max). IPCore clears the render
        // FBO with alpha=0 (ImageRenderer::clearBackgroundToBlack); the blit copies
        // that through. The CA compositor honours per-pixel alpha on IOSurface, so
        // without this the background is transparent. Mirrors the fast path.
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

        // On macOS GL-on-Metal, glReadPixels does not implicitly synchronise the
        // Metal command stream — glFinish() is required to commit the blit before
        // the readback samples the flip FBO.
        glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, m_cpuFlipFbo);
        glFinish();

        // GL_BGRA + GL_UNSIGNED_INT_2_10_10_10_REV packs A(31:30)|R(29:20)|G(19:10)|B(9:0)
        // = kCVPixelFormatType_ARGB2101010LEPacked. No per-pixel CPU work.
        try
        {
            m_cpuPackedScratch.resize(static_cast<size_t>(w) * h);
        }
        catch (const std::bad_alloc&)
        {
            std::cerr << "ERROR: [QTMetalVideoDevice] bad_alloc in CPU fallback ("
                      << w << "x" << h << ")\n";
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo->fboID());
            return;
        }
        glReadPixels(0, 0, w, h, GL_BGRA, GL_UNSIGNED_INT_2_10_10_10_REV,
                     m_cpuPackedScratch.data());

        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo->fboID());
        m_view->presentPixelData(m_cpuPackedScratch.data(), w, h);
    }

    //--------------------------------------------------------------------------
    // VideoDevice API
    //--------------------------------------------------------------------------

    void QTMetalVideoDevice::redraw() const
    {
        // Coalesced: collapses a burst of redraw requests into a single render
        // per event-loop cycle (the GL path gets this for free from QWidget::
        // update()).  Without this each request ran a separate present.
        if (m_view)
            m_view->requestUpdate();
    }

    void QTMetalVideoDevice::redrawImmediately() const
    {
        if (!m_view)
            return;

        if (m_view->isVisible())
        {
#ifdef PLATFORM_DARWIN
            // Keep playback alive when the window is fully occluded (mirrors
            // QTGLVideoDevice::redrawImmediately()).
            m_view->setAttribute(Qt::WA_Mapped);
#endif
            m_view->renderImmediately();
        }
        else
        {
            redraw();
        }
    }

    void QTMetalVideoDevice::clearCaches() const
    {
        GLVideoDevice::clearCaches();
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
    }

    void QTMetalVideoDevice::close()
    {
        if (m_view)
            m_view->hide();
    }

    bool QTMetalVideoDevice::isOpen() const
    {
        if (m_view)
            return m_view->isVisible();
        return false;
    }

} // namespace Rv

#endif // PLATFORM_DARWIN
