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
#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>
#include <unistd.h>

#ifndef GL_EXT_memory_object
#define GL_EXT_memory_object 1
#define GL_TEXTURE_TILING_EXT 0x9580
#define GL_DEDICATED_MEMORY_OBJECT_EXT 0x9581
#define GL_NUM_TILING_TYPES_EXT 0x9582
#define GL_TILING_TYPES_EXT 0x9583
#define GL_OPTIMAL_TILING_EXT 0x9584
#define GL_LINEAR_TILING_EXT 0x9585
#define GL_NUM_DEVICE_UUIDS_EXT 0x9596
#define GL_DEVICE_UUID_EXT 0x9597
#define GL_DRIVER_UUID_EXT 0x9598
#define GL_UUID_SIZE_EXT 16
#endif

#ifndef GL_EXT_memory_object_fd
#define GL_EXT_memory_object_fd 1
#define GL_HANDLE_TYPE_OPAQUE_FD_EXT 0x9586
#endif

#ifndef GL_EXT_semaphore
#define GL_EXT_semaphore 1
#define GL_NUM_SUPPORTED_SEMAPHORE_WAIT_LAYOUTS_EXT 0x958A
#define GL_SUPPORTED_SEMAPHORE_WAIT_LAYOUTS_EXT 0x958B
#define GL_NUM_SUPPORTED_SEMAPHORE_SIGNAL_LAYOUTS_EXT 0x958C
#define GL_SUPPORTED_SEMAPHORE_SIGNAL_LAYOUTS_EXT 0x958D
#define GL_LAYOUT_COLOR_ATTACHMENT_EXT 0x958E
#define GL_LAYOUT_DEPTH_STENCIL_ATTACHMENT_EXT 0x958F
#define GL_LAYOUT_DEPTH_STENCIL_READ_ONLY_EXT 0x9590
#define GL_LAYOUT_SHADER_READ_ONLY_EXT 0x9591
#define GL_LAYOUT_TRANSFER_SRC_EXT 0x9592
#define GL_LAYOUT_TRANSFER_DST_EXT 0x9593
#define GL_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_EXT 0x9530
#define GL_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_EXT 0x9531
#endif

namespace Rv
{
    using namespace std;
    using namespace TwkApp;

    QTVulkanVideoDevice::QTVulkanVideoDevice(VideoModule* module, const string& name, VulkanView* view, QWidget* eventWidget)
        : TwkGLF::GLVideoDevice(module, name, VideoDevice::ImageOutput | VideoDevice::ProvidesSync | VideoDevice::SubWindow)
        , m_view(view)
        , m_eventWidget(eventWidget)
        , m_translator(eventWidget ? new QTTranslator(this, eventWidget) : nullptr)
    {
        assert(view);
    }

    QTVulkanVideoDevice::~QTVulkanVideoDevice()
    {
        // Delete the FBO and its colour texture while the GL context is current.
        if (m_glContext && (m_fbo || m_fboColorTex || m_glMemoryObject))
        {
            m_glContext->makeCurrent(m_offscreenSurface);
            delete m_fbo;
            m_fbo = nullptr;
            if (m_fboColorTex)
            {
                glDeleteTextures(1, &m_fboColorTex);
                m_fboColorTex = 0;
            }
            cleanupSharedGLObjects();
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

            // Join RV's global GL resource-sharing group (enabled via
            // Qt::AA_ShareOpenGLContexts at startup). Without this the offscreen
            // context is isolated and FTGL font-atlas textures created in
            // another context have no storage here, so glyph uploads fail with
            // GL_INVALID_OPERATION.
            m_glContext->setShareContext(QOpenGLContext::globalShareContext());

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
                m_glContext->doneCurrent();
                delete m_offscreenSurface;
                m_offscreenSurface = nullptr;
                delete m_glContext;
                m_glContext = nullptr;
                return;
            }
        }

        if (!m_glContext->makeCurrent(m_offscreenSurface))
        {
            std::cerr << "[QTVulkanVideoDevice] makeCurrent() failed\n";
            return;
        }

        const float dpr = m_view ? m_view->devicePixelRatio() : 1.0f;
        int newW = m_view ? static_cast<int>(m_view->width() * dpr + 0.5f) : 128;
        int newH = m_view ? static_cast<int>(m_view->height() * dpr + 0.5f) : 128;
        if (newW < 1)
            newW = 128;
        if (newH < 1)
            newH = 128;

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
            glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA16F_ARB, newW, newH, 0, GL_RGBA, GL_FLOAT, nullptr);
            glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);

            m_fbo = new TwkGLF::GLFBO(newW, newH, GL_RGBA16F_ARB);
            m_fbo->attachColorTexture(GL_TEXTURE_RECTANGLE_ARB, m_fboColorTex);

            GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
            if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
                std::cerr << "[QTVulkanVideoDevice] FBO incomplete: 0x" << std::hex << status << std::dec << "\n";

            m_fboWidth = newW;
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

            int w = m_view ? m_view->width() : 0;
            int h = m_view ? m_view->height() : 0;
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

    void QTVulkanVideoDevice::makeCurrent() const { ensureGLContext(); }

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

    std::string QTVulkanVideoDevice::hardwareIdentification() const { return "vulkan-hybrid"; }

    void QTVulkanVideoDevice::cleanupSharedGLObjects() const
    {
        if (m_glSharedTexture)
        {
            glDeleteTextures(1, &m_glSharedTexture);
            m_glSharedTexture = 0;
        }
        if (m_glMemoryObject)
        {
            glDeleteMemoryObjectsEXT(1, &m_glMemoryObject);
            m_glMemoryObject = 0;
        }
        if (m_glReadySemaphore)
        {
            glDeleteSemaphoresEXT(1, &m_glReadySemaphore);
            m_glReadySemaphore = 0;
        }
        if (m_vkReadySemaphore)
        {
            glDeleteSemaphoresEXT(1, &m_vkReadySemaphore);
            m_vkReadySemaphore = 0;
        }
        m_sharedWidth = 0;
        m_sharedHeight = 0;
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

        // Get shared image info from VulkanView
        const VulkanView::SharedImageInfo* sharedInfo = m_view->getSharedImageInfo(w, h);
        if (!sharedInfo)
        {
            // Fallback to CPU readback if GPU interop fails.
            // Packing below assumes A2B10G10R10_UNORM_PACK32 (= GL_RGB10_A2 bit layout).
            // createSwapchain() guarantees this format is selected when GPU interop is used;
            // a surface offering only A2R10G10B10 falls back to 8-bit and never reaches here.
            assert(!m_view || m_view->swapchainFormat() == VK_FORMAT_A2B10G10R10_UNORM_PACK32
                   || m_view->swapchainFormat() == VK_FORMAT_UNDEFINED);
            fbo->bind();
            glFinish();
            const size_t floatCount = static_cast<size_t>(w) * h * 4;
            std::vector<float> floatPx(floatCount);
            glReadPixels(0, 0, w, h, GL_RGBA, GL_FLOAT, floatPx.data());
            fbo->unbind();

            std::vector<uint32_t> packed(static_cast<size_t>(w) * h);
            for (int y = 0; y < h; ++y)
            {
                const float* src = floatPx.data() + (size_t)(h - 1 - y) * w * 4;
                uint32_t* dst = packed.data() + (size_t)y * w;
                for (int x = 0; x < w; ++x, src += 4)
                {
                    const float r = std::max(0.f, std::min(1.f, src[0]));
                    const float g = std::max(0.f, std::min(1.f, src[1]));
                    const float b = std::max(0.f, std::min(1.f, src[2]));
                    const uint32_t ri = static_cast<uint32_t>(r * 1023.f + 0.5f) & 0x3FF;
                    const uint32_t gi = static_cast<uint32_t>(g * 1023.f + 0.5f) & 0x3FF;
                    const uint32_t bi = static_cast<uint32_t>(b * 1023.f + 0.5f) & 0x3FF;
                    dst[x] = (3u << 30) | (bi << 20) | (gi << 10) | ri;
                }
            }
            m_view->presentPixelData(packed.data(), w, h);
            return;
        }

        // Check if we need to re-import the shared objects
        if (m_sharedWidth != w || m_sharedHeight != h || !m_glMemoryObject)
        {
            cleanupSharedGLObjects();

            glCreateMemoryObjectsEXT(1, &m_glMemoryObject);
            // Duplicate the FD because glImportMemoryFdEXT takes ownership
            int memFd = dup(sharedInfo->memoryFd);
            glImportMemoryFdEXT(m_glMemoryObject, sharedInfo->size, GL_HANDLE_TYPE_OPAQUE_FD_EXT, memFd);

            glGenTextures(1, &m_glSharedTexture);
            glBindTexture(GL_TEXTURE_2D, m_glSharedTexture);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_TILING_EXT, GL_LINEAR_TILING_EXT);

            glTexStorageMem2DEXT(GL_TEXTURE_2D, 1, GL_RGB10_A2, sharedInfo->strideWidth, h, m_glMemoryObject, 0);
            glBindTexture(GL_TEXTURE_2D, 0);

            glGenSemaphoresEXT(1, &m_glReadySemaphore);
            int glReadyFd = dup(sharedInfo->glReadySemaphoreFd);
            glImportSemaphoreFdEXT(m_glReadySemaphore, GL_HANDLE_TYPE_OPAQUE_FD_EXT, glReadyFd);

            glGenSemaphoresEXT(1, &m_vkReadySemaphore);
            int vkReadyFd = dup(sharedInfo->vkReadySemaphoreFd);
            glImportSemaphoreFdEXT(m_vkReadySemaphore, GL_HANDLE_TYPE_OPAQUE_FD_EXT, vkReadyFd);

            m_sharedWidth = w;
            m_sharedHeight = h;
        }

        // Wait for Vulkan to be ready
        GLuint waitDstLayouts[] = {GL_LAYOUT_COLOR_ATTACHMENT_EXT};
        glWaitSemaphoreEXT(m_vkReadySemaphore, 0, nullptr, 1, &m_glSharedTexture, waitDstLayouts);

        // Blit from FBO to shared texture
        GLuint readFbo = fbo->fboID();
        GLuint drawFbo;
        glGenFramebuffersEXT(1, &drawFbo);
        glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, drawFbo);
        glFramebufferTexture2DEXT(GL_DRAW_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, m_glSharedTexture, 0);

        glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, readFbo);

        // Note: GL origin is bottom-left, Vulkan origin is top-left. We need to flip Y.
        glBlitFramebufferEXT(0, 0, w, h, 0, h, w, 0, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, readFbo); // restore
        glDeleteFramebuffersEXT(1, &drawFbo);

        // Signal Vulkan that GL is done
        GLuint signalSrcLayouts[] = {GL_LAYOUT_TRANSFER_SRC_EXT};
        glSignalSemaphoreEXT(m_glReadySemaphore, 0, nullptr, 1, &m_glSharedTexture, signalSrcLayouts);

        glFlush();

        // Tell VulkanView to present
        m_view->presentSharedImage();
    }

    //--------------------------------------------------------------------------
    // VideoDevice API
    //--------------------------------------------------------------------------

    void QTVulkanVideoDevice::redraw() const
    {
        if (m_view)
        {
            QCoreApplication::postEvent(m_view, new QEvent(QEvent::UpdateRequest));
        }
    }

    void QTVulkanVideoDevice::redrawImmediately() const { redraw(); }

    void QTVulkanVideoDevice::clearCaches() const {}

    VideoDevice::Resolution QTVulkanVideoDevice::resolution() const
    {
        if (!m_view)
        {
            return Resolution(0, 0, 1.0f, 1.0f);
        }
        const float dpr = m_view->devicePixelRatio();
        return Resolution(static_cast<int>(m_view->width() * dpr + 0.5f), static_cast<int>(m_view->height() * dpr + 0.5f), 1.0f, 1.0f);
    }

    VideoDevice::Offset QTVulkanVideoDevice::offset() const { return Offset(m_x, m_y); }

    VideoDevice::Timing QTVulkanVideoDevice::timing() const { return Timing((m_refresh != -1.0f) ? m_refresh : 0.0f); }

    VideoDevice::VideoFormat QTVulkanVideoDevice::format() const
    {
        if (!m_view)
        {
            return VideoFormat(0, 0, 1.0, 1.0, 0.0, hardwareIdentification());
        }
        const float dpr = m_view->devicePixelRatio();
        return VideoFormat(static_cast<int>(m_view->width() * dpr + 0.5f), static_cast<int>(m_view->height() * dpr + 0.5f), 1.0, 1.0,
                           (m_refresh != -1.0f) ? m_refresh : 0.0f, hardwareIdentification());
    }

    size_t QTVulkanVideoDevice::width() const
    {
        if (!m_view)
        {
            return 0;
        }
        return static_cast<size_t>(m_view->width() * m_view->devicePixelRatio() + 0.5f);
    }

    size_t QTVulkanVideoDevice::height() const
    {
        if (!m_view)
        {
            return 0;
        }
        return static_cast<size_t>(m_view->height() * m_view->devicePixelRatio() + 0.5f);
    }

    void QTVulkanVideoDevice::open(const StringVector& /*args*/)
    {
        if (m_view)
        {
            m_view->show();
        }
        m_isOpen = true;
    }

    void QTVulkanVideoDevice::close()
    {
        if (m_view)
        {
            m_view->hide();
        }
        m_isOpen = false;
    }

    bool QTVulkanVideoDevice::isOpen() const
    {
        if (m_view)
        {
            return m_view->isVisible();
        }
        return false;
    }

} // namespace Rv

#endif // PLATFORM_LINUX
