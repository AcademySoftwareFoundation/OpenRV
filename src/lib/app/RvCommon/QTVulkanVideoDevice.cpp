//
//  Copyright (c) 2026 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//

#if defined(PLATFORM_LINUX) || defined(PLATFORM_WINDOWS)

#include <GL/glew.h>
#include <RvCommon/QTVulkanVideoDevice.h>
#include <RvCommon/VulkanView.h>
#include <RvCommon/DesktopVideoDevice.h>
#include <TwkApp/Application.h>
#include <TwkApp/VideoModule.h>
#include <TwkApp/Event.h>
#include <IPCore/Session.h>
#include <IPCore/ImageRenderer.h>
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
#ifdef PLATFORM_WINDOWS
// WIN32_LEAN_AND_MEAN prevents <windows.h> from including the legacy
// <winsock.h>, which otherwise collides with the <winsock2.h> already
// pulled in transitively by Qt headers above.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <unistd.h>
#endif

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

#ifndef GL_EXT_memory_object_win32
#define GL_EXT_memory_object_win32 1
#define GL_HANDLE_TYPE_OPAQUE_WIN32_EXT 0x9587
#define GL_HANDLE_TYPE_OPAQUE_WIN32_KMT_EXT 0x9588
#endif

#ifndef GL_EXT_semaphore_win32
#define GL_EXT_semaphore_win32 1
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

#ifdef PLATFORM_WINDOWS
// The bundled GLEW under src/pub/glew (version 2.3.0) does not declare the
// EXT_memory_object / EXT_semaphore (or their Win32 companions) entry points.
// The Linux build uses a newer managed GLEW that does, so the Linux call sites
// can resolve the symbols at link time. On Windows we declare the function
// pointer typedefs locally and resolve them at first use via wglGetProcAddress;
// if any are missing the GPU interop path is disabled and VulkanView falls
// back to its CPU pack-and-upload presentation path.
typedef void(GLAPIENTRY* PFNGLCREATEMEMORYOBJECTSEXTPROC_RV)(GLsizei n, GLuint* memoryObjects);
typedef void(GLAPIENTRY* PFNGLDELETEMEMORYOBJECTSEXTPROC_RV)(GLsizei n, const GLuint* memoryObjects);
typedef void(GLAPIENTRY* PFNGLTEXSTORAGEMEM2DEXTPROC_RV)(GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width,
                                                         GLsizei height, GLuint memory, GLuint64 offset);
typedef void(GLAPIENTRY* PFNGLIMPORTMEMORYWIN32HANDLEEXTPROC_RV)(GLuint memory, GLuint64 size, GLenum handleType, void* handle);
typedef void(GLAPIENTRY* PFNGLGENSEMAPHORESEXTPROC_RV)(GLsizei n, GLuint* semaphores);
typedef void(GLAPIENTRY* PFNGLDELETESEMAPHORESEXTPROC_RV)(GLsizei n, const GLuint* semaphores);
typedef void(GLAPIENTRY* PFNGLIMPORTSEMAPHOREWIN32HANDLEEXTPROC_RV)(GLuint semaphore, GLenum handleType, void* handle);
typedef void(GLAPIENTRY* PFNGLWAITSEMAPHOREEXTPROC_RV)(GLuint semaphore, GLuint numBufferBarriers, const GLuint* buffers,
                                                       GLuint numTextureBarriers, const GLuint* textures, const GLenum* dstLayouts);
typedef void(GLAPIENTRY* PFNGLSIGNALSEMAPHOREEXTPROC_RV)(GLuint semaphore, GLuint numBufferBarriers, const GLuint* buffers,
                                                         GLuint numTextureBarriers, const GLuint* textures, const GLenum* srcLayouts);

namespace
{
    PFNGLCREATEMEMORYOBJECTSEXTPROC_RV g_glCreateMemoryObjectsEXT = nullptr;
    PFNGLDELETEMEMORYOBJECTSEXTPROC_RV g_glDeleteMemoryObjectsEXT = nullptr;
    PFNGLTEXSTORAGEMEM2DEXTPROC_RV g_glTexStorageMem2DEXT = nullptr;
    PFNGLIMPORTMEMORYWIN32HANDLEEXTPROC_RV g_glImportMemoryWin32HandleEXT = nullptr;
    PFNGLGENSEMAPHORESEXTPROC_RV g_glGenSemaphoresEXT = nullptr;
    PFNGLDELETESEMAPHORESEXTPROC_RV g_glDeleteSemaphoresEXT = nullptr;
    PFNGLIMPORTSEMAPHOREWIN32HANDLEEXTPROC_RV g_glImportSemaphoreWin32HandleEXT = nullptr;
    PFNGLWAITSEMAPHOREEXTPROC_RV g_glWaitSemaphoreEXT = nullptr;
    PFNGLSIGNALSEMAPHOREEXTPROC_RV g_glSignalSemaphoreEXT = nullptr;

    bool g_glInteropProbed = false;
    bool g_glInteropAvailable = false;

    // Must be called with a current GL context.
    bool loadGLInteropExtensions()
    {
        if (g_glInteropProbed)
            return g_glInteropAvailable;
        g_glInteropProbed = true;

        g_glCreateMemoryObjectsEXT = reinterpret_cast<PFNGLCREATEMEMORYOBJECTSEXTPROC_RV>(wglGetProcAddress("glCreateMemoryObjectsEXT"));
        g_glDeleteMemoryObjectsEXT = reinterpret_cast<PFNGLDELETEMEMORYOBJECTSEXTPROC_RV>(wglGetProcAddress("glDeleteMemoryObjectsEXT"));
        g_glTexStorageMem2DEXT = reinterpret_cast<PFNGLTEXSTORAGEMEM2DEXTPROC_RV>(wglGetProcAddress("glTexStorageMem2DEXT"));
        g_glImportMemoryWin32HandleEXT =
            reinterpret_cast<PFNGLIMPORTMEMORYWIN32HANDLEEXTPROC_RV>(wglGetProcAddress("glImportMemoryWin32HandleEXT"));
        g_glGenSemaphoresEXT = reinterpret_cast<PFNGLGENSEMAPHORESEXTPROC_RV>(wglGetProcAddress("glGenSemaphoresEXT"));
        g_glDeleteSemaphoresEXT = reinterpret_cast<PFNGLDELETESEMAPHORESEXTPROC_RV>(wglGetProcAddress("glDeleteSemaphoresEXT"));
        g_glImportSemaphoreWin32HandleEXT =
            reinterpret_cast<PFNGLIMPORTSEMAPHOREWIN32HANDLEEXTPROC_RV>(wglGetProcAddress("glImportSemaphoreWin32HandleEXT"));
        g_glWaitSemaphoreEXT = reinterpret_cast<PFNGLWAITSEMAPHOREEXTPROC_RV>(wglGetProcAddress("glWaitSemaphoreEXT"));
        g_glSignalSemaphoreEXT = reinterpret_cast<PFNGLSIGNALSEMAPHOREEXTPROC_RV>(wglGetProcAddress("glSignalSemaphoreEXT"));

        g_glInteropAvailable = g_glCreateMemoryObjectsEXT && g_glDeleteMemoryObjectsEXT && g_glTexStorageMem2DEXT
                               && g_glImportMemoryWin32HandleEXT && g_glGenSemaphoresEXT && g_glDeleteSemaphoresEXT
                               && g_glImportSemaphoreWin32HandleEXT && g_glWaitSemaphoreEXT && g_glSignalSemaphoreEXT;

        // Identify the GL driver alongside the interop probe result. Useful when
        // the Windows GL context happens to be the Microsoft GDI Generic
        // software renderer, in which case interop is expected to fail.
        const GLubyte* vendor = glGetString(GL_VENDOR);
        const GLubyte* renderer = glGetString(GL_RENDERER);
        const GLubyte* version = glGetString(GL_VERSION);
        if (IPCore::ImageRenderer::debugGpu())
        {
            std::cout << "INFO: QTVulkanVideoDevice: GL_VENDOR='" << (vendor ? reinterpret_cast<const char*>(vendor) : "?")
                      << "' GL_RENDERER='" << (renderer ? reinterpret_cast<const char*>(renderer) : "?") << "' GL_VERSION='"
                      << (version ? reinterpret_cast<const char*>(version) : "?") << "'" << std::endl;

            if (!g_glInteropAvailable)
            {
                std::cout << "INFO: QTVulkanVideoDevice: GL_EXT_memory_object_win32 / GL_EXT_semaphore_win32 NOT available; "
                             "falling back to CPU presentation path"
                          << std::endl;
            }
            else
            {
                std::cout << "INFO: QTVulkanVideoDevice: GL interop extensions resolved (GPU-interop available)" << std::endl;
            }
        }
        return g_glInteropAvailable;
    }
} // namespace

// Redirect direct GL extension references in this translation unit to the
// dynamically resolved pointers. Linux still uses the real GLEW symbols.
#define glCreateMemoryObjectsEXT g_glCreateMemoryObjectsEXT
#define glDeleteMemoryObjectsEXT g_glDeleteMemoryObjectsEXT
#define glTexStorageMem2DEXT g_glTexStorageMem2DEXT
#define glImportMemoryWin32HandleEXT g_glImportMemoryWin32HandleEXT
#define glGenSemaphoresEXT g_glGenSemaphoresEXT
#define glDeleteSemaphoresEXT g_glDeleteSemaphoresEXT
#define glImportSemaphoreWin32HandleEXT g_glImportSemaphoreWin32HandleEXT
#define glWaitSemaphoreEXT g_glWaitSemaphoreEXT
#define glSignalSemaphoreEXT g_glSignalSemaphoreEXT
#endif // PLATFORM_WINDOWS

namespace Rv
{
    using namespace std;
    using namespace TwkApp;
    using namespace IPCore;

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
        if (m_glContext && (m_fbo || m_fboColorTex || m_glMemoryObject[0]))
        {
            m_glContext->makeCurrent(m_offscreenSurface);
            delete m_fbo;
            m_fbo = nullptr;
            if (m_fboColorTex)
            {
                glDeleteTextures(1, &m_fboColorTex);
                m_fboColorTex = 0;
            }
            for (uint32_t i = 0; i < VulkanView::FRAMES_IN_FLIGHT; ++i)
                cleanupSharedGLObjects(i);
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
                cerr << "ERROR: QTVulkanVideoDevice: QOpenGLContext::create() failed" << endl;
                delete m_glContext;
                m_glContext = nullptr;
                return;
            }

            m_offscreenSurface = new QOffscreenSurface();
            m_offscreenSurface->setFormat(m_glContext->format());
            m_offscreenSurface->create();

            if (!m_offscreenSurface->isValid())
            {
                cerr << "ERROR: QTVulkanVideoDevice: QOffscreenSurface::create() failed" << endl;
                delete m_offscreenSurface;
                m_offscreenSurface = nullptr;
                delete m_glContext;
                m_glContext = nullptr;
                return;
            }

            m_glContext->makeCurrent(m_offscreenSurface);
            glewExperimental = GL_TRUE;
#ifdef PLATFORM_WINDOWS
            // The bundled Windows GLEW (src/pub/glew) has a Tweak-modified
            // signature: glewInit(GLEWGetProcAddress F). Pass nullptr to use
            // the default GL entry-point loader, matching every other Windows
            // glewInit call site (rvio main.cpp, InitGL.cpp, FBOVideoDevice.cpp,
            // NDIModule.cpp, BlackMagicModule.cpp, AJAModule.cpp).
            GLenum err = glewInit(nullptr);
#else
            GLenum err = glewInit();
#endif
            if (err != GLEW_OK)
            {
                cerr << "ERROR: QTVulkanVideoDevice: glewInit failed: " << glewGetErrorString(err) << endl;
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
            cerr << "ERROR: QTVulkanVideoDevice: makeCurrent() failed" << endl;
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
                cerr << "ERROR: QTVulkanVideoDevice: FBO incomplete: 0x" << hex << status << dec << endl;

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

    void QTVulkanVideoDevice::cleanupSharedGLObjects(uint32_t slot) const
    {
        if (m_drawFbo[slot])
        {
            glDeleteFramebuffersEXT(1, &m_drawFbo[slot]);
            m_drawFbo[slot] = 0;
        }
        if (m_glSharedTexture[slot])
        {
            glDeleteTextures(1, &m_glSharedTexture[slot]);
            m_glSharedTexture[slot] = 0;
        }
        if (m_glMemoryObject[slot])
        {
            glDeleteMemoryObjectsEXT(1, &m_glMemoryObject[slot]);
            m_glMemoryObject[slot] = 0;
        }
        if (m_glReadySemaphore[slot])
        {
            glDeleteSemaphoresEXT(1, &m_glReadySemaphore[slot]);
            m_glReadySemaphore[slot] = 0;
        }
        if (m_vkReadySemaphore[slot])
        {
            glDeleteSemaphoresEXT(1, &m_vkReadySemaphore[slot]);
            m_vkReadySemaphore[slot] = 0;
        }
        m_sharedWidth[slot] = 0;
        m_sharedHeight[slot] = 0;
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

        // Pair this frame's GL ring objects with the Vulkan in-flight slot the
        // frame renders into. getSharedImageInfo()/presentSharedImage() below use
        // this same slot; presentSharedImage() advances it only at frame end, so
        // the value is stable for the whole call.
        const uint32_t slot = m_view->currentFrame();

        TwkGLF::GLFBO* fbo = m_fbo;
        const int w = static_cast<int>(fbo->width());
        const int h = static_cast<int>(fbo->height());

        if (w <= 0 || h <= 0)
            return;

        if (!m_glContext->makeCurrent(m_offscreenSurface))
            return;

        // Get shared image info from VulkanView
#ifdef PLATFORM_WINDOWS
        // Probe the EXT_memory_object/EXT_semaphore (+ _win32) entry points
        // while the GL context is current. If the driver does not expose them,
        // skip the Vulkan-side export work entirely and fall through to the
        // CPU pack-and-upload path below.
        const bool glInteropAvailable = loadGLInteropExtensions();
        const VulkanView::SharedImageInfo* sharedInfo = glInteropAvailable ? m_view->getSharedImageInfo(w, h) : nullptr;
#else
        const bool glInteropAvailable = GLEW_EXT_memory_object && GLEW_EXT_semaphore && GLEW_EXT_memory_object_fd && GLEW_EXT_semaphore_fd;
        const VulkanView::SharedImageInfo* sharedInfo = glInteropAvailable ? m_view->getSharedImageInfo(w, h) : nullptr;
#endif

        static bool firstFrameLogged = false;
        if (!firstFrameLogged)
        {
            firstFrameLogged = true;
            if (ImageRenderer::debugGpu())
            {
                const VkFormat scFmt = m_view ? m_view->swapchainFormat() : VK_FORMAT_UNDEFINED;
                cout << "INFO: QTVulkanVideoDevice: syncBuffers: first frame path = " << (sharedInfo ? "GPU-interop" : "CPU-fallback")
                     << "  swapchainFormat=" << scFmt
                     << (scFmt == VK_FORMAT_A2B10G10R10_UNORM_PACK32   ? " (A2B10G10R10 / 10-bit)"
                         : scFmt == VK_FORMAT_A2R10G10B10_UNORM_PACK32 ? " (A2R10G10B10 / 10-bit)"
                         : scFmt == VK_FORMAT_UNDEFINED                ? " (UNDEFINED -- swapchain not created yet)"
                                                                       : " (NOT 10-bit)")
                     << endl;
            }
        }

        if (!sharedInfo)
        {
            // Fallback to CPU readback if GPU interop fails. The packed 32-bit
            // words are copied unconverted into the swapchain image, so pack in
            // the swapchain's own component order: A2B10G10R10 (R in low bits,
            // == GL_RGB10_A2) or A2R10G10B10 (R in high bits). Linux/RADV
            // surfaces commonly offer only A2R10G10B10.
            const VkFormat scFmt = m_view ? m_view->swapchainFormat() : VK_FORMAT_A2B10G10R10_UNORM_PACK32;
            const bool rgbOrder = (scFmt == VK_FORMAT_A2R10G10B10_UNORM_PACK32);
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
                    // A2R10G10B10: A|R|G|B (R high).  A2B10G10R10: A|B|G|R (R low).
                    dst[x] = rgbOrder ? ((3u << 30) | (ri << 20) | (gi << 10) | bi) : ((3u << 30) | (bi << 20) | (gi << 10) | ri);
                }
            }
            m_view->presentPixelData(packed.data(), w, h);
            return;
        }

        // Re-import only when the shared image was actually reallocated, i.e. its
        // capacity (stride width + capacity height) changed. Within capacity the
        // Vulkan side keeps the same export, so a resize does not re-import here;
        // m_sharedWidth/m_sharedHeight cache the imported capacity, not the used
        // (requested) size.
        if (m_sharedWidth[slot] != sharedInfo->strideWidth || m_sharedHeight[slot] != sharedInfo->capacityHeight || !m_glMemoryObject[slot])
        {
            cleanupSharedGLObjects(slot);

            glCreateMemoryObjectsEXT(1, &m_glMemoryObject[slot]);
#ifdef PLATFORM_WINDOWS
            // Windows GL import does NOT take ownership of the HANDLE; the
            // Vulkan side and this GL side each keep their own reference.
            // VulkanView's cleanupSharedImage() calls CloseHandle on its
            // copy; this device's cleanupSharedGLObjects() does not need to
            // close anything because glImportMemoryWin32HandleEXT does not
            // create a new handle.
            glImportMemoryWin32HandleEXT(m_glMemoryObject[slot], sharedInfo->size, GL_HANDLE_TYPE_OPAQUE_WIN32_EXT,
                                         static_cast<HANDLE>(sharedInfo->memoryHandle));
#else
            // Duplicate the FD because glImportMemoryFdEXT takes ownership
            int memFd = dup(sharedInfo->memoryFd);
            if (memFd == -1)
            {
                cerr << "ERROR: QTVulkanVideoDevice: dup(memoryFd) failed." << endl;
                cleanupSharedGLObjects(slot);
                return;
            }
            glImportMemoryFdEXT(m_glMemoryObject[slot], sharedInfo->size, GL_HANDLE_TYPE_OPAQUE_FD_EXT, memFd);
#endif

            glGenTextures(1, &m_glSharedTexture[slot]);
            glBindTexture(GL_TEXTURE_2D, m_glSharedTexture[slot]);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_TILING_EXT,
                            sharedInfo->optimalTiling ? GL_OPTIMAL_TILING_EXT : GL_LINEAR_TILING_EXT);

            // Allocate the imported texture at the image's capacity dimensions
            // (stride width x capacity height); the FBO blit below writes only the
            // used w x h sub-region into its origin corner.
            glTexStorageMem2DEXT(GL_TEXTURE_2D, 1, GL_RGB10_A2, sharedInfo->strideWidth, sharedInfo->capacityHeight, m_glMemoryObject[slot],
                                 0);
            glBindTexture(GL_TEXTURE_2D, 0);

            glGenSemaphoresEXT(1, &m_glReadySemaphore[slot]);
            glGenSemaphoresEXT(1, &m_vkReadySemaphore[slot]);
#ifdef PLATFORM_WINDOWS
            glImportSemaphoreWin32HandleEXT(m_glReadySemaphore[slot], GL_HANDLE_TYPE_OPAQUE_WIN32_EXT,
                                            static_cast<HANDLE>(sharedInfo->glReadySemaphoreHandle));
            glImportSemaphoreWin32HandleEXT(m_vkReadySemaphore[slot], GL_HANDLE_TYPE_OPAQUE_WIN32_EXT,
                                            static_cast<HANDLE>(sharedInfo->vkReadySemaphoreHandle));
#else
            int glReadyFd = dup(sharedInfo->glReadySemaphoreFd);
            if (glReadyFd == -1)
            {
                cerr << "ERROR: QTVulkanVideoDevice: dup(glReadySemaphoreFd) failed." << endl;
                cleanupSharedGLObjects(slot);
                return;
            }
            glImportSemaphoreFdEXT(m_glReadySemaphore[slot], GL_HANDLE_TYPE_OPAQUE_FD_EXT, glReadyFd);

            int vkReadyFd = dup(sharedInfo->vkReadySemaphoreFd);
            if (vkReadyFd == -1)
            {
                cerr << "ERROR: QTVulkanVideoDevice: dup(vkReadySemaphoreFd) failed." << endl;
                cleanupSharedGLObjects(slot);
                return;
            }
            glImportSemaphoreFdEXT(m_vkReadySemaphore[slot], GL_HANDLE_TYPE_OPAQUE_FD_EXT, vkReadyFd);
#endif

            // Cache the imported capacity so we re-import only when it grows.
            m_sharedWidth[slot] = sharedInfo->strideWidth;
            m_sharedHeight[slot] = sharedInfo->capacityHeight;
        }

        // Wait for Vulkan to be ready
        GLuint waitSrcLayouts[] = {GL_LAYOUT_TRANSFER_SRC_EXT};
        glWaitSemaphoreEXT(m_vkReadySemaphore[slot], 0, nullptr, 1, &m_glSharedTexture[slot], waitSrcLayouts);

        // Blit from FBO to shared texture
        GLuint readFbo = fbo->fboID();
        if (!m_drawFbo[slot])
        {
            glGenFramebuffersEXT(1, &m_drawFbo[slot]);
            glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, m_drawFbo[slot]);
            glFramebufferTexture2DEXT(GL_DRAW_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, m_glSharedTexture[slot], 0);
        }
        else
        {
            glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, m_drawFbo[slot]);
        }

        glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, readFbo);

        // Note: GL origin is bottom-left, Vulkan origin is top-left. We need to flip Y.
        glBlitFramebufferEXT(0, 0, w, h, 0, h, w, 0, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, readFbo); // restore

        // Signal Vulkan that GL is done
        GLuint signalDstLayouts[] = {GL_LAYOUT_COLOR_ATTACHMENT_EXT};
        glSignalSemaphoreEXT(m_glReadySemaphore[slot], 0, nullptr, 1, &m_glSharedTexture[slot], signalDstLayouts);

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

#endif // PLATFORM_LINUX || PLATFORM_WINDOWS
