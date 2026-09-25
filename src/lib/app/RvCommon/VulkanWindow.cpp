//
//  Copyright (c) 2026 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//

#if defined(PLATFORM_LINUX) || defined(PLATFORM_WINDOWS)

#include <RvCommon/VulkanWindow.h>
#include <RvCommon/QTVulkanVideoDevice.h>
#include <RvCommon/RvDocument.h>
#include <RvApp/Options.h>
#include <RvApp/RvSession.h>
#include <IPCore/Session.h>
#include <IPCore/ImageRenderer.h>
#include <TwkApp/Event.h>
#include <TwkApp/VideoDevice.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QTimer>
#include <QtCore/QVersionNumber>
#include <QtGui/QResizeEvent>
#include <QtGui/QShowEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QPaintEvent>
#include <QtGui/QPlatformSurfaceEvent>
#include <QtGui/QWindow>
#include <QtGui/QVulkanInstance>
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>
#include <QtWidgets/QMenu>
#include <QtWidgets/QWidget>

#include <algorithm>
#include <cctype>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#ifdef PLATFORM_WINDOWS
// Keep <windows.h> from pulling in <winsock.h>, which collides with Qt's
// <winsock2.h>.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <unistd.h>
#endif

//
//  Environment overrides, all reported in the startup record:
//
//    RV_VULKAN_FORCE_CPU_PRESENT     skip zero-copy interop; CPU readback.
//    RV_VULKAN_FORCE_TILING          optimal | linear; refused if the driver
//                                    does not report it exportable.
//    RV_VULKAN_FORCE_NO_DEDICATED    suppress preferred dedicated allocation;
//                                    refused for DEDICATED_ONLY. Alias:
//                                    RV_VULKAN_DISABLE_DEDICATED_ALLOCATION.
//

namespace Rv
{
    using namespace std;

    //  -debug gpu frame-time accumulators. Only one VulkanWindow drives the
    //  frame loop, so file statics suffice.
    static unsigned int s_diagFrames = 0;
    static double s_diagRenderMs = 0.0;
    static double s_diagMainPresentMs = 0.0;
    static double s_diagOutPresentMs = 0.0;
    static double s_diagFenceWaitMs = 0.0;
    static double s_diagAcquireMs = 0.0;
    static double s_diagLoopMs = 0.0;
    static TwkUtil::Timer s_diagLoopTimer;
    //  Post-present work: in the frame period but not in "total".
    static double s_diagPostRenderMs = 0.0;
    //  handler: time in the pointer handler. eventToRender: age of the newest
    //  pointer event when its frame starts rendering.
    static double s_diagPointerHandlerMs = 0.0;
    static unsigned int s_diagPointerEvents = 0;
    static double s_diagPointerAgeMs = 0.0;
    static unsigned int s_diagPointerAgeSamples = 0;
    static TwkUtil::Timer s_diagPointerTimer;
    static bool s_diagPointerPending = false;

    //  eventToRetire: end-to-end latency, from the pointer event to its
    //  frame's GPU retirement. Needs an absolute clock since retirement lags
    //  by several frames.
    static TwkUtil::Timer s_diagClock;

    static double diagNow()
    {
        if (!s_diagClock.isRunning())
        {
            s_diagClock.start();
        }
        return s_diagClock.elapsed();
    }

    //  Event time for the frame being rendered (-1 if none), handed to its
    //  slot at submit.
    static double s_diagFrameEventTime = -1.0;
    static std::array<double, VulkanWindow::kFramesInFlight> s_diagSlotEventTime{};
    static std::array<bool, VulkanWindow::kFramesInFlight> s_diagSlotArmed{};
    static double s_diagEventToRetireMs = 0.0;
    static unsigned int s_diagEventToRetireSamples = 0;

    using namespace TwkApp;
    using namespace IPCore;

    // R/B order is handled where pixels are packed or blitted. Many RADV
    // surfaces only advertise A2R10G10B10.
    static bool isTenBitFormat(VkFormat f) { return f == VK_FORMAT_A2B10G10R10_UNORM_PACK32 || f == VK_FORMAT_A2R10G10B10_UNORM_PACK32; }

    static bool findGraphicsPresentQueue(VkPhysicalDevice device, VkSurfaceKHR surface, uint32_t& familyIndex)
    {
        uint32_t familyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, nullptr);
        std::vector<VkQueueFamilyProperties> families(familyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, families.data());

        for (uint32_t i = 0; i < familyCount; ++i)
        {
            VkBool32 presentSupport = VK_FALSE;
            if (vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport) == VK_SUCCESS
                && (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && presentSupport)
            {
                familyIndex = i;
                return true;
            }
        }
        return false;
    }

    static bool surfaceHasTenBitFormat(VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        uint32_t formatCount = 0;
        if (vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr) != VK_SUCCESS || formatCount == 0)
        {
            return false;
        }

        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        if (vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, formats.data()) != VK_SUCCESS)
        {
            return false;
        }

        return std::any_of(formats.begin(), formats.end(), [](const VkSurfaceFormatKHR& format) { return isTenBitFormat(format.format); });
    }

    static bool deviceHasExtension(VkPhysicalDevice device, const char* name)
    {
        uint32_t extensionCount = 0;
        if (vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr) != VK_SUCCESS)
        {
            return false;
        }

        std::vector<VkExtensionProperties> extensions(extensionCount);
        if (vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data()) != VK_SUCCESS)
        {
            return false;
        }

        return std::any_of(extensions.begin(), extensions.end(),
                           [name](const VkExtensionProperties& extension) { return strcmp(extension.extensionName, name) == 0; });
    }

    static const char* formatName(VkFormat f)
    {
        switch (f)
        {
        case VK_FORMAT_B8G8R8A8_UNORM:
            return "B8G8R8A8_UNORM";
        case VK_FORMAT_B8G8R8A8_SRGB:
            return "B8G8R8A8_SRGB";
        case VK_FORMAT_R8G8B8A8_UNORM:
            return "R8G8B8A8_UNORM";
        case VK_FORMAT_R8G8B8A8_SRGB:
            return "R8G8B8A8_SRGB";
        case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
            return "A2B10G10R10_UNORM_PACK32";
        case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
            return "A2R10G10B10_UNORM_PACK32";
        case VK_FORMAT_R16G16B16A16_SFLOAT:
            return "R16G16B16A16_SFLOAT";
        default:
            return "(other)";
        }
    }

    //--------------------------------------------------------------------------
    // VulkanWindow implementation
    //--------------------------------------------------------------------------

    VulkanWindow::VulkanWindow(RvDocument* doc, bool noResize)
        : QWindow()
        , m_doc(doc)
        , m_videoDevice(nullptr)
        , m_initialized(false)
        , m_firstPaintCompleted(false)
        , m_postFirstNonEmptyRender(noResize)
        , m_stopProcessingEvents(false)
        , m_userActive(true)
        , m_eventWidget(nullptr)
        , m_lastKey(0)
        , m_lastKeyType(QEvent::None)
    {
        setSurfaceType(QSurface::VulkanSurface);

        QSurfaceFormat fmt;
        fmt.setRedBufferSize(10);
        fmt.setGreenBufferSize(10);
        fmt.setBlueBufferSize(10);
        fmt.setAlphaBufferSize(2);
        setFormat(fmt);

        m_activityTimer.start();

        m_eventProcessingTimer.setSingleShot(true);
        connect(&m_eventProcessingTimer, SIGNAL(timeout()), this, SLOT(eventProcessingTimeout()));
    }

    VulkanWindow::~VulkanWindow()
    {
        m_videoDevice = nullptr;

        //  Usually a no-op: the SurfaceAboutToBeDestroyed handler in event()
        //  already released everything.
        releaseVulkanResources();
    }

    //--------------------------------------------------------------------------

    void VulkanWindow::stopProcessingEvents() { m_stopProcessingEvents = true; }

    void VulkanWindow::absolutePosition(int& x, int& y) const
    {
        QPoint gp = mapToGlobal(QPoint(0, 0));
        x = gp.x();
        y = gp.y();
    }

    float VulkanWindow::devicePixelRatioF() const { return static_cast<float>(QWindow::devicePixelRatio()); }

    bool VulkanWindow::physicalDeviceMatchesUUID(const unsigned char* uuid, size_t size) const
    {
        if (!m_vkPhysicalDevice || !uuid || size != VK_UUID_SIZE)
        {
            return false;
        }

        VkPhysicalDeviceIDProperties idProperties = {};
        idProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;

        VkPhysicalDeviceProperties2 properties = {};
        properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        properties.pNext = &idProperties;
        vkGetPhysicalDeviceProperties2(m_vkPhysicalDevice, &properties);

        return std::equal(idProperties.deviceUUID, idProperties.deviceUUID + VK_UUID_SIZE, uuid);
    }

    //--------------------------------------------------------------------------
    // Vulkan Initialisation
    //--------------------------------------------------------------------------

    void VulkanWindow::initialize()
    {
        if (m_initialized)
        {
            return;
        }

        if (!initVulkan())
        {
            cerr << "ERROR: VulkanWindow: initVulkan failed; falling back to OpenGL" << endl;
            requestGLFallback();
            return;
        }

        m_initialized = true;
        m_initializedHandle = handle();

        //  Session init must not run from here: loading packages can create a
        //  QWebEngineView, which destroys this window while it is on the stack.
        //  RvApplication::newSessionFromFiles() does it after show() instead.
    }

    //
    //  One process-lifetime QVulkanInstance, shared by the 10-bit probe and
    //  every VulkanWindow, never destroyed: destroying a VkInstance shortly
    //  before another init corrupts RADV's X11 WSI state and segfaults in
    //  vkGetPhysicalDeviceSurfaceSupportKHR.
    //
    static QVulkanInstance* sharedVulkanInstance()
    {
        static QVulkanInstance* instance = []() -> QVulkanInstance*
        {
            auto* inst = new QVulkanInstance();
            // 1.1 for vkGetPhysicalDeviceProperties2 (device UUID matching).
            inst->setApiVersion(QVersionNumber(1, 1));
            if (!inst->create())
            {
                cerr << "ERROR: VulkanWindow: shared QVulkanInstance create failed" << endl;
                delete inst;
                return nullptr;
            }
            return inst;
        }();
        return instance;
    }

    bool VulkanWindow::supports10BitPresentation()
    {
        //  A fixed hardware property: probe once per process.
        static const bool cached = []() -> bool
        {
            QVulkanInstance* qtVkInst = sharedVulkanInstance();
            if (!qtVkInst)
            {
                return false;
            }

            VkInstance instance = qtVkInst->vkInstance();
            if (instance == VK_NULL_HANDLE)
            {
                return false;
            }

            //  Leaked for the same RADV WSI reason as sharedVulkanInstance().
            static QWindow* dummyWindow = []() -> QWindow*
            {
                auto* w = new QWindow();
                w->setSurfaceType(QSurface::VulkanSurface);
                w->create();
                return w;
            }();
            dummyWindow->setVulkanInstance(qtVkInst);

            VkSurfaceKHR dummySurface = qtVkInst->surfaceForWindow(dummyWindow);
            if (!dummySurface)
            {
                cerr << "ERROR: VulkanWindow: supports10BitPresentation: failed to create dummy surface" << endl;
                return false;
            }

            uint32_t deviceCount = 0;
            vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
            if (deviceCount == 0)
            {
                cerr << "ERROR: VulkanWindow: supports10BitPresentation: vkEnumeratePhysicalDevices returned 0 devices" << endl;
                return false;
            }
            std::vector<VkPhysicalDevice> devices(deviceCount);
            vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

            if (ImageRenderer::debugGpu())
            {
                cout << "INFO: VulkanWindow: supports10BitPresentation: probing " << deviceCount << " physical device(s)" << endl;
            }

            bool any10bit = false;
            for (uint32_t di = 0; di < devices.size(); ++di)
            {
                VkPhysicalDevice dev = devices[di];

                uint32_t queueFamily = 0;
                const bool canPresent =
                    deviceHasExtension(dev, VK_KHR_SWAPCHAIN_EXTENSION_NAME) && findGraphicsPresentQueue(dev, dummySurface, queueFamily);
                const bool has10bit = canPresent && surfaceHasTenBitFormat(dev, dummySurface);
                any10bit = any10bit || has10bit;

                VkPhysicalDeviceProperties props = {};
                vkGetPhysicalDeviceProperties(dev, &props);
                if (ImageRenderer::debugGpu())
                {
                    cout << "INFO: VulkanWindow:   device[" << di << "] '" << props.deviceName
                         << "': graphics+present=" << (canPresent ? "YES" : "NO") << "  10-bit surface format=" << (has10bit ? "YES" : "NO")
                         << endl;
                }
            }

            if (ImageRenderer::debugGpu())
            {
                cout << "INFO: VulkanWindow: supports10BitPresentation: returning " << (any10bit ? "true" : "false") << endl;
            }
            return any10bit;
        }();

        return cached;
    }

    bool VulkanWindow::initVulkan()
    {
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "RV VulkanWindow";
        appInfo.apiVersion = VK_API_VERSION_1_1;

        std::vector<const char*> instanceExtensions = {
            VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(VK_USE_PLATFORM_WIN32_KHR)
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
            VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
            VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
#elif defined(VK_USE_PLATFORM_XCB_KHR)
            VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#endif
        };

        QVulkanInstance* qtVkInst = sharedVulkanInstance();
        if (!qtVkInst)
        {
            cerr << "ERROR: VulkanWindow: shared QVulkanInstance unavailable" << endl;
            return false;
        }

        m_vkInstance = qtVkInst->vkInstance();

        // Re-creating after a reparent can get here before the platform window.
        if (!handle())
        {
            create();
        }

        setVulkanInstance(qtVkInst);

        m_vkSurface = qtVkInst->surfaceForWindow(this);
        if (!m_vkSurface)
        {
            cerr << "ERROR: VulkanWindow: Failed to create Vulkan surface" << endl;
            return false;
        }

        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, nullptr);
        if (deviceCount == 0)
        {
            return false;
        }
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, devices.data());

        m_vkPhysicalDevice = VK_NULL_HANDLE;
        bool foundQueue = false;
        m_queueFamilyIndex = 0;

        for (VkPhysicalDevice dev : devices)
        {
            uint32_t queueFamily = 0;
            if (findGraphicsPresentQueue(dev, m_vkSurface, queueFamily) && surfaceHasTenBitFormat(dev, m_vkSurface))
            {
                m_vkPhysicalDevice = dev;
                m_queueFamilyIndex = queueFamily;
                foundQueue = true;
                break;
            }
        }

        if (!foundQueue)
        {
            cerr << "ERROR: VulkanWindow: initVulkan: No physical device with graphics, present, and 10-bit surface support found." << endl;
            return false;
        }

        {
            //  Unconditional: compared against GL_RENDERER to spot hybrid-GPU
            //  machines.
            VkPhysicalDeviceProperties props = {};
            vkGetPhysicalDeviceProperties(m_vkPhysicalDevice, &props);
            cout << "INFO: VulkanWindow: initVulkan: picked physical device '" << props.deviceName << "' (of " << deviceCount
                 << " available)" << endl;
        }

        float queuePriority = 1.0f;
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = m_queueFamilyIndex;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        if (!deviceHasExtension(m_vkPhysicalDevice, VK_KHR_SWAPCHAIN_EXTENSION_NAME))
        {
            cerr << "ERROR: VulkanWindow: selected device does not support VK_KHR_swapchain." << endl;
            return false;
        }

#ifdef PLATFORM_WINDOWS
        const char* externalMemoryExtension = VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME;
        const char* externalSemaphoreExtension = VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME;
#else
        const char* externalMemoryExtension = VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME;
        const char* externalSemaphoreExtension = VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME;
#endif
        m_externalInteropSupported = deviceHasExtension(m_vkPhysicalDevice, externalMemoryExtension)
                                     && deviceHasExtension(m_vkPhysicalDevice, externalSemaphoreExtension);
        if (m_externalInteropSupported)
        {
            deviceExtensions.push_back(externalMemoryExtension);
            deviceExtensions.push_back(externalSemaphoreExtension);
        }
        else if (ImageRenderer::debugGpu())
        {
            cout << "INFO: VulkanWindow: external memory/semaphore extensions unavailable; using CPU fallback." << endl;
        }

        VkDeviceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = &queueCreateInfo;
        createInfo.queueCreateInfoCount = 1;
        createInfo.enabledExtensionCount = deviceExtensions.size();
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (vkCreateDevice(m_vkPhysicalDevice, &createInfo, nullptr, &m_vkDevice) != VK_SUCCESS)
        {
            return false;
        }

        vkGetDeviceQueue(m_vkDevice, m_queueFamilyIndex, 0, &m_vkQueue);

        negotiateInteropConfig();
        if (ImageRenderer::debugGpu())
        {
            cout << "INFO: VulkanWindow: initVulkan: interop negotiation ran (once per device); result="
                 << (m_interopConfig.supported ? "supported" : "unsupported") << endl;
        }

        auto failInit = [this]()
        {
            cleanupVulkan();
            return false;
        };

        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = m_queueFamilyIndex;
        if (vkCreateCommandPool(m_vkDevice, &poolInfo, nullptr, &m_vkCommandPool) != VK_SUCCESS)
        {
            return failInit();
        }

        // Per-image renderFinished semaphores are created in createSwapchain().
        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        // Signaled so the first wait on a slot passes.
        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        for (uint32_t i = 0; i < kFramesInFlight; ++i)
        {
            if (vkCreateSemaphore(m_vkDevice, &semaphoreInfo, nullptr, &m_vkImageAvailableSemaphore[i]) != VK_SUCCESS)
            {
                return failInit();
            }
            if (vkCreateFence(m_vkDevice, &fenceInfo, nullptr, &m_vkFence[i]) != VK_SUCCESS)
            {
                return failInit();
            }
        }

        return true;
    }

    // Queue fallbackVulkanToGLView on the next event-loop tick (at most once).
    void VulkanWindow::requestGLFallback()
    {
        if (m_glFallbackRequested || !m_doc || m_stopProcessingEvents || m_doc->isClosing())
        {
            return;
        }
        m_glFallbackRequested = true;

        //  Logged explicitly since OpenGL forgoes 10-bit.
        reportPresentPath(PresentPath::OpenGL,
                          m_presentPathReason.empty() ? std::string("Vulkan presentation could not be established") : m_presentPathReason);

        QTimer::singleShot(0, m_doc, [doc = m_doc]() { doc->fallbackVulkanToGLView(); });
    }

    // Skip swapchain work during close or zero-size resize.
    bool VulkanWindow::presentationAllowed() const
    {
        if (m_stopProcessingEvents)
        {
            return false;
        }
        if (width() <= 0 || height() <= 0)
        {
            return false;
        }
        if (m_doc && m_doc->isClosing())
        {
            return false;
        }
        return true;
    }

    // Rebuild all Vulkan state after the native surface is lost.
    void VulkanWindow::handleSurfaceLost()
    {
        //  Qt replaces the platform window when the top-level QWidgetWindow is
        //  recreated (e.g. inserting a QWebEngineView). The VkSurfaceKHR itself
        //  is stale, so a swapchain recreate is not enough.
        cout << "INFO: VulkanWindow: platform window recreated; rebuilding Vulkan surface" << endl;

        releaseVulkanResources();
    }

    void VulkanWindow::releaseVulkanResources()
    {
        //  Drop the GL imports first so GL never aliases memory freed below.
        if (m_videoDevice)
        {
            m_videoDevice->releaseSharedGLObjects();

            //  The next initVulkan() may pick a different physical device.
            m_videoDevice->resetInteropDeviceMatch();
        }

        for (uint32_t i = 0; i < kFramesInFlight; ++i)
        {
            cleanupSharedImage(i);
        }
        cleanupSwapchain();
        cleanupVulkan();

        //  Owned by QVulkanInstance; not destroyed here.
        m_vkSurface = VK_NULL_HANDLE;
        m_vkPhysicalDevice = VK_NULL_HANDLE;
        m_vkSwapchainFormat = VK_FORMAT_UNDEFINED;
        m_vkSwapchainExtent = {};
        m_vkSwapchainImages.clear();
        m_currentFrame = 0;

        m_initialized = false;
        m_initializedHandle = nullptr;
    }

    void VulkanWindow::handleSwapchainOutOfDate()
    {
        if (!m_vkDevice || !presentationAllowed())
        {
            return;
        }

        // The shared image is content-sized and the acquire semaphores are
        // per frame and may still be referenced by queued submissions, so only
        // the swapchain is recreated.
        if (!createSwapchain())
        {
            requestGLFallback();
        }
    }

    void VulkanWindow::cleanupVulkan()
    {
        if (m_vkDevice)
        {
            vkDeviceWaitIdle(m_vkDevice);

            for (uint32_t i = 0; i < kFramesInFlight; ++i)
            {
                if (m_vkImageAvailableSemaphore[i])
                {
                    vkDestroySemaphore(m_vkDevice, m_vkImageAvailableSemaphore[i], nullptr);
                    m_vkImageAvailableSemaphore[i] = VK_NULL_HANDLE;
                }
                if (m_vkFence[i])
                {
                    vkDestroyFence(m_vkDevice, m_vkFence[i], nullptr);
                    m_vkFence[i] = VK_NULL_HANDLE;
                }
            }

            if (m_vkCommandPool)
            {
                vkDestroyCommandPool(m_vkDevice, m_vkCommandPool, nullptr);
                m_vkCommandPool = VK_NULL_HANDLE;
            }

            vkDestroyDevice(m_vkDevice, nullptr);
            m_vkDevice = VK_NULL_HANDLE;
        }
        m_vkQueue = VK_NULL_HANDLE;
        m_externalInteropSupported = false;
        // The VkSurfaceKHR is owned by QVulkanInstance; do not destroy it here.
    }

    //
    //  FIFO everywhere. For measurement, RV_VULKAN_PRESENT_MODE (control
    //  viewport) and RV_VULKAN_OUTPUT_PRESENT_MODE (presentation output) take
    //  fifo | relaxed | mailbox | immediate.
    //
    static const char* presentModeName(VkPresentModeKHR m)
    {
        switch (m)
        {
        case VK_PRESENT_MODE_IMMEDIATE_KHR:
            return "IMMEDIATE";
        case VK_PRESENT_MODE_MAILBOX_KHR:
            return "MAILBOX";
        case VK_PRESENT_MODE_FIFO_KHR:
            return "FIFO";
        case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
            return "FIFO_RELAXED";
        default:
            return "(other)";
        }
    }

    static bool presentModeFromName(const char* name, VkPresentModeKHR& mode)
    {
        if (!name)
        {
            return false;
        }
        const string n(name);
        if (n == "fifo")
        {
            mode = VK_PRESENT_MODE_FIFO_KHR;
        }
        else if (n == "relaxed")
        {
            mode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
        }
        else if (n == "mailbox")
        {
            mode = VK_PRESENT_MODE_MAILBOX_KHR;
        }
        else if (n == "immediate")
        {
            mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
        else
        {
            return false;
        }
        return true;
    }

    static VkPresentModeKHR choosePresentMode(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, bool passiveOutput)
    {
        uint32_t count = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &count, nullptr);
        std::vector<VkPresentModeKHR> available(count);
        if (count)
        {
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &count, available.data());
        }

        const auto supported = [&](VkPresentModeKHR m) { return std::find(available.begin(), available.end(), m) != available.end(); };

        VkPresentModeKHR forced = VK_PRESENT_MODE_FIFO_KHR;
        if (presentModeFromName(getenv(passiveOutput ? "RV_VULKAN_OUTPUT_PRESENT_MODE" : "RV_VULKAN_PRESENT_MODE"), forced))
        {
            if (supported(forced))
            {
                return forced;
            }
            cout << "WARNING: VulkanWindow: requested present mode " << presentModeName(forced) << " is unsupported; using FIFO" << endl;
            return VK_PRESENT_MODE_FIFO_KHR;
        }

        (void)passiveOutput;
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    bool VulkanWindow::createSwapchain()
    {
        if (!m_vkDevice || !m_vkSurface)
        {
            return false;
        }

        if (!presentationAllowed())
        {
            return false;
        }

        VkSurfaceCapabilitiesKHR capabilities;
        if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_vkPhysicalDevice, m_vkSurface, &capabilities) != VK_SUCCESS)
        {
            requestGLFallback();
            return false;
        }

        uint32_t formatCount = 0;
        if (vkGetPhysicalDeviceSurfaceFormatsKHR(m_vkPhysicalDevice, m_vkSurface, &formatCount, nullptr) != VK_SUCCESS || formatCount == 0)
        {
            requestGLFallback();
            return false;
        }
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        if (vkGetPhysicalDeviceSurfaceFormatsKHR(m_vkPhysicalDevice, m_vkSurface, &formatCount, formats.data()) != VK_SUCCESS)
        {
            requestGLFallback();
            return false;
        }

        //  Unconditional, once per window: the colour space and order of each
        //  10-bit entry are vendor-specific.
        if (!m_loggedSurfaceFormatList)
        {
            m_loggedSurfaceFormatList = true;

            cout << "INFO: VulkanWindow: createSwapchain: surface offers " << formatCount << " format(s):" << endl;
            for (uint32_t i = 0; i < formats.size(); ++i)
            {
                cout << "INFO: VulkanWindow:   [" << i << "] format=" << formats[i].format << " (" << formatName(formats[i].format)
                     << ")  colorSpace=" << formats[i].colorSpace << endl;
            }
        }

        VkSurfaceFormatKHR surfaceFormat = formats[0];
        bool found10bit = false;
        // Prefer A2B10G10R10 (== GL_RGB10_A2) so the transfer is a plain copy.
        // RV emits sRGB, so require SRGB_NONLINEAR: in HDR mode NVIDIA lists a
        // 10-bit HDR10_ST2084 entry first, which renders the viewport black.
        const auto findTenBit = [&formats](VkFormat wanted, bool requireSrgbNonlinear, VkSurfaceFormatKHR& out) -> bool
        {
            for (const auto& fmt : formats)
            {
                if (fmt.format != wanted)
                {
                    continue;
                }
                if (requireSrgbNonlinear && fmt.colorSpace != VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                {
                    continue;
                }
                out = fmt;
                return true;
            }
            return false;
        };

        for (const VkFormat wanted : {VK_FORMAT_A2B10G10R10_UNORM_PACK32, VK_FORMAT_A2R10G10B10_UNORM_PACK32})
        {
            if (findTenBit(wanted, true, surfaceFormat))
            {
                found10bit = true;
                break;
            }
        }

        // No SRGB_NONLINEAR pairing: keep 10-bit anyway and warn below.
        if (!found10bit)
        {
            for (const VkFormat wanted : {VK_FORMAT_A2B10G10R10_UNORM_PACK32, VK_FORMAT_A2R10G10B10_UNORM_PACK32})
            {
                if (findTenBit(wanted, false, surfaceFormat))
                {
                    found10bit = true;
                    break;
                }
            }
        }

        if (found10bit)
        {
            //  Unconditional, but only on change (runs on every resize).
            if (surfaceFormat.format != m_loggedSurfaceFormat.format || surfaceFormat.colorSpace != m_loggedSurfaceFormat.colorSpace)
            {
                m_loggedSurfaceFormat = surfaceFormat;

                cout << "INFO: VulkanWindow: createSwapchain: chose " << formatName(surfaceFormat.format)
                     << " colorSpace=" << surfaceFormat.colorSpace
                     << (surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR ? " (SRGB_NONLINEAR)" : " (NOT SRGB_NONLINEAR)")
                     << " (10-bit OK)" << endl;

                if (surfaceFormat.colorSpace != VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                {
                    cout << "WARNING: VulkanWindow: no 10-bit surface format is paired with SRGB_NONLINEAR on this surface. RV emits "
                            "sRGB, so the image may look wrong (dark or washed out)."
                         << endl;
                }
            }
        }
        else
        {
            cout << "WARNING: VulkanWindow: Real surface lacks a 10-bit format (A2B10G10R10/A2R10G10B10); requesting OpenGL fallback"
                 << endl;
            requestGLFallback();
            return false;
        }

        m_vkSwapchainFormat = surfaceFormat.format;

        m_vkSwapchainExtent = capabilities.currentExtent;
        if (m_vkSwapchainExtent.width == UINT32_MAX)
        {
            const qreal dpr = devicePixelRatio();
            const uint32_t pixelWidth = static_cast<uint32_t>(std::max<qreal>(1.0, width() * dpr));
            const uint32_t pixelHeight = static_cast<uint32_t>(std::max<qreal>(1.0, height() * dpr));
            m_vkSwapchainExtent = {std::clamp(pixelWidth, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
                                   std::clamp(pixelHeight, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)};
        }
        if (m_vkSwapchainExtent.width == 0 || m_vkSwapchainExtent.height == 0)
        {
            requestGLFallback();
            return false;
        }

        const VkPresentModeKHR presentMode = choosePresentMode(m_vkPhysicalDevice, m_vkSurface, /*passiveOutput*/ m_doc == nullptr);

        uint32_t imageCount = capabilities.minImageCount + 1;
        //  MAILBOX needs three images (scanout, queued, rendering) or acquire
        //  blocks.
        if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR && imageCount < 3)
        {
            imageCount = 3;
        }
        if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
        {
            imageCount = capabilities.maxImageCount;
        }

        if (ImageRenderer::debugGpu())
        {
            cout << "INFO: VulkanWindow: createSwapchain: " << (m_doc ? "control viewport" : "presentation output")
                 << ": presentMode=" << presentModeName(presentMode) << "  images=" << imageCount
                 << " (surface min=" << capabilities.minImageCount << " max=" << capabilities.maxImageCount << ")" << endl;
        }

        VkSwapchainCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = m_vkSurface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = m_vkSwapchainExtent;
        createInfo.imageArrayLayers = 1;
        if (!(capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT))
        {
            cout << "WARNING: VulkanWindow: surface does not support transfer-destination swapchain images; requesting OpenGL fallback"
                 << endl;
            requestGLFallback();
            return false;
        }
        createInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.preTransform = capabilities.currentTransform;
        const VkCompositeAlphaFlagBitsKHR compositeAlphaPreference[] = {
            VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
        };
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        for (const VkCompositeAlphaFlagBitsKHR alpha : compositeAlphaPreference)
        {
            if (capabilities.supportedCompositeAlpha & alpha)
            {
                createInfo.compositeAlpha = alpha;
                break;
            }
        }
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        // Lets the driver reuse the retiring swapchain's resources on resize.
        createInfo.oldSwapchain = m_vkSwapchain;

        // A failed create leaves the existing swapchain intact.
        VkSwapchainKHR newSwapchain = VK_NULL_HANDLE;
        if (vkCreateSwapchainKHR(m_vkDevice, &createInfo, nullptr, &newSwapchain) != VK_SUCCESS)
        {
            requestGLFallback();
            return false;
        }

        if (m_vkSwapchain != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(m_vkDevice);
            if (!m_vkCommandBuffers.empty())
            {
                vkFreeCommandBuffers(m_vkDevice, m_vkCommandPool, static_cast<uint32_t>(m_vkCommandBuffers.size()),
                                     m_vkCommandBuffers.data());
                m_vkCommandBuffers.clear();
            }
            vkDestroySwapchainKHR(m_vkDevice, m_vkSwapchain, nullptr);
        }
        m_vkSwapchain = newSwapchain;

        vkGetSwapchainImagesKHR(m_vkDevice, m_vkSwapchain, &imageCount, nullptr);
        m_vkSwapchainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(m_vkDevice, m_vkSwapchain, &imageCount, m_vkSwapchainImages.data());

        m_vkCommandBuffers.resize(imageCount);
        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_vkCommandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = static_cast<uint32_t>(m_vkCommandBuffers.size());
        if (vkAllocateCommandBuffers(m_vkDevice, &allocInfo, m_vkCommandBuffers.data()) != VK_SUCCESS)
        {
            cleanupSwapchain();
            requestGLFallback();
            return false;
        }

        // The device is idle here, so old renderFinished semaphores are safe
        // to destroy.
        for (VkSemaphore sem : m_vkRenderFinished)
        {
            if (sem)
            {
                vkDestroySemaphore(m_vkDevice, sem, nullptr);
            }
        }
        m_vkRenderFinished.assign(imageCount, VK_NULL_HANDLE);
        VkSemaphoreCreateInfo rfInfo = {};
        rfInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        for (uint32_t i = 0; i < imageCount; ++i)
        {
            if (vkCreateSemaphore(m_vkDevice, &rfInfo, nullptr, &m_vkRenderFinished[i]) != VK_SUCCESS)
            {
                cleanupSwapchain();
                requestGLFallback();
                return false;
            }
        }
        m_imagesInFlight.assign(imageCount, VK_NULL_HANDLE);

        return true;
    }

    void VulkanWindow::cleanupSwapchain()
    {
        if (m_vkDevice)
        {
            vkDeviceWaitIdle(m_vkDevice);

            for (VkSemaphore sem : m_vkRenderFinished)
            {
                if (sem)
                {
                    vkDestroySemaphore(m_vkDevice, sem, nullptr);
                }
            }
            m_vkRenderFinished.clear();
            m_imagesInFlight.clear();

            for (uint32_t i = 0; i < kFramesInFlight; ++i)
            {
                if (m_vkStagingBuffer[i])
                {
                    vkDestroyBuffer(m_vkDevice, m_vkStagingBuffer[i], nullptr);
                    m_vkStagingBuffer[i] = VK_NULL_HANDLE;
                }
                if (m_vkStagingBufferMemory[i])
                {
                    vkFreeMemory(m_vkDevice, m_vkStagingBufferMemory[i], nullptr);
                    m_vkStagingBufferMemory[i] = VK_NULL_HANDLE;
                }
                m_stagingBufferSize[i] = 0;
            }

            if (!m_vkCommandBuffers.empty())
            {
                vkFreeCommandBuffers(m_vkDevice, m_vkCommandPool, m_vkCommandBuffers.size(), m_vkCommandBuffers.data());
                m_vkCommandBuffers.clear();
            }

            if (m_vkSwapchain)
            {
                vkDestroySwapchainKHR(m_vkDevice, m_vkSwapchain, nullptr);
                m_vkSwapchain = VK_NULL_HANDLE;
            }
        }
    }

    uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }
        return UINT32_MAX;
    }

    namespace
    {
        bool envFlagSet(const char* name) { return getenv(name) != nullptr; }

        const char* tilingName(VkImageTiling t)
        {
            switch (t)
            {
            case VK_IMAGE_TILING_OPTIMAL:
                return "OPTIMAL";
            case VK_IMAGE_TILING_LINEAR:
                return "LINEAR";
            default:
                return "(other)";
            }
        }

        const char* colorSpaceName(VkColorSpaceKHR cs)
        {
            switch (cs)
            {
            case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR:
                return "SRGB_NONLINEAR";
            case VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT:
                return "EXTENDED_SRGB_LINEAR";
            case VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT:
                return "EXTENDED_SRGB_NONLINEAR";
            case VK_COLOR_SPACE_HDR10_ST2084_EXT:
                return "HDR10_ST2084";
            case VK_COLOR_SPACE_HDR10_HLG_EXT:
                return "HDR10_HLG";
            case VK_COLOR_SPACE_BT2020_LINEAR_EXT:
                return "BT2020_LINEAR";
            case VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT:
                return "DISPLAY_P3_NONLINEAR";
            case VK_COLOR_SPACE_PASS_THROUGH_EXT:
                return "PASS_THROUGH";
            default:
                return "(other)";
            }
        }

        // False when RV_VULKAN_FORCE_TILING is unset or unrecognized.
        bool forcedTilingRequested(VkImageTiling& out)
        {
            const char* v = getenv("RV_VULKAN_FORCE_TILING");
            if (!v)
            {
                return false;
            }

            std::string s(v);
            std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(::tolower(c)); });

            if (s == "optimal")
            {
                out = VK_IMAGE_TILING_OPTIMAL;
                return true;
            }
            if (s == "linear")
            {
                out = VK_IMAGE_TILING_LINEAR;
                return true;
            }

            cout << "WARNING: VulkanWindow: RV_VULKAN_FORCE_TILING='" << v << "' is not recognized (expected 'optimal' or 'linear'); "
                 << "ignoring it and using the negotiated tiling" << endl;
            return false;
        }

        // Resolved dynamically (with the KHR alias) so a 1.0-only loader
        // degrades to "not exportable" instead of crashing.
        PFN_vkGetPhysicalDeviceImageFormatProperties2 getImageFormatProperties2(VkInstance instance)
        {
            static PFN_vkGetPhysicalDeviceImageFormatProperties2 fn = nullptr;
            static bool resolved = false;
            if (!resolved)
            {
                resolved = true;
                fn = reinterpret_cast<PFN_vkGetPhysicalDeviceImageFormatProperties2>(
                    vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceImageFormatProperties2"));
                if (!fn)
                {
                    fn = reinterpret_cast<PFN_vkGetPhysicalDeviceImageFormatProperties2>(
                        vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceImageFormatProperties2KHR"));
                }
            }
            return fn;
        }

        bool isNvidiaPhysicalDevice(VkPhysicalDevice dev)
        {
            VkPhysicalDeviceProperties props = {};
            vkGetPhysicalDeviceProperties(dev, &props);
            return props.vendorID == 0x10DE;
        }

        bool nvidiaInteropWorkaroundDisabled()
        {
            static const bool disabled = getenv("RV_VULKAN_DISABLE_NVIDIA_INTEROP_WORKAROUND") != nullptr;
            return disabled;
        }

        bool dedicatedAllocationDisabled()
        {
            static const bool disabled =
                getenv("RV_VULKAN_FORCE_NO_DEDICATED") != nullptr || getenv("RV_VULKAN_DISABLE_DEDICATED_ALLOCATION") != nullptr;
            return disabled;
        }

        // Frames the control viewport may keep in flight. Default 1: depth is
        // input latency, and the loop is GPU-bound so a second frame buys no
        // throughput. RV_VULKAN_MAX_FRAMES_IN_FLIGHT=2 restores the deeper ring.
        unsigned int maxFramesInFlight()
        {
            static const unsigned int depth = []
            {
                const unsigned int kDefault = 1;
                const char* v = getenv("RV_VULKAN_MAX_FRAMES_IN_FLIGHT");
                if (!v)
                {
                    return kDefault;
                }
                const int n = atoi(v);
                if (n < 1 || n > static_cast<int>(VulkanWindow::kFramesInFlight))
                {
                    cout << "WARNING: VulkanWindow: RV_VULKAN_MAX_FRAMES_IN_FLIGHT must be 1.." << VulkanWindow::kFramesInFlight
                         << "; using " << kDefault << endl;
                    return kDefault;
                }
                return static_cast<unsigned int>(n);
            }();
            return depth;
        }

        // Vendor heuristic: OPTIMAL on NVIDIA, LINEAR elsewhere. NVIDIA 550+
        // returns blank pixels to GL for LINEAR shared images >= ~2 MiB
        // (forum thread #349436); its GL and Vulkan share one driver, so the
        // optimal layout matches on import. Under Mesa, GL and Vulkan are
        // different drivers and OPTIMAL renders tile garbage (RADV PHOENIX2);
        // making it usable there needs VK_EXT_image_drm_format_modifier.
        bool useOptimalTilingForInterop(VkPhysicalDevice dev)
        {
#if defined(PLATFORM_LINUX) || defined(PLATFORM_WINDOWS)
            return !nvidiaInteropWorkaroundDisabled() && isNvidiaPhysicalDevice(dev);
#else
            (void)dev;
            return false;
#endif
        }
    } // namespace

    void VulkanWindow::negotiateInteropConfig()
    {
        if (m_interopNegotiated)
        {
            return;
        }
        m_interopNegotiated = true;

        InteropConfig cfg;
        cfg.format = VK_FORMAT_A2B10G10R10_UNORM_PACK32; // == GL_RGB10_A2
        //  Must cover GL's use too (it renders into the image): with only
        //  TRANSFER_SRC the driver may pick a compressed layout GL cannot read.
        cfg.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

#ifdef PLATFORM_WINDOWS
        const VkExternalMemoryHandleTypeFlagBits handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
        const VkExternalMemoryHandleTypeFlagBits handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif

        PFN_vkGetPhysicalDeviceImageFormatProperties2 probe = getImageFormatProperties2(m_vkInstance);
        if (!probe)
        {
            cfg.supported = false;
            cfg.rejectReason = "vkGetPhysicalDeviceImageFormatProperties2 is unavailable "
                               "(Vulkan instance predates 1.1 and lacks VK_KHR_get_physical_device_properties2), "
                               "so exportability cannot be established";
            m_interopConfig = cfg;
            return;
        }

        //  OPTIMAL first (avoids NVIDIA's blank large-LINEAR-image bug);
        //  LINEAR is the portable fallback.
        const VkImageTiling candidates[] = {VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_TILING_LINEAR};

        auto probeTiling = [&](VkImageTiling tiling, VkExternalMemoryFeatureFlags& features) -> bool
        {
            VkPhysicalDeviceExternalImageFormatInfo extInfo = {};
            extInfo.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO;
            extInfo.handleType = handleType;

            VkPhysicalDeviceImageFormatInfo2 fmtInfo = {};
            fmtInfo.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2;
            fmtInfo.pNext = &extInfo;
            fmtInfo.format = cfg.format;
            fmtInfo.type = VK_IMAGE_TYPE_2D;
            fmtInfo.tiling = tiling;
            fmtInfo.usage = cfg.usage;
            fmtInfo.flags = 0;

            VkExternalImageFormatProperties extProps = {};
            extProps.sType = VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES;

            VkImageFormatProperties2 props = {};
            props.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2;
            props.pNext = &extProps;

            const VkResult r = probe(m_vkPhysicalDevice, &fmtInfo, &props);
            features = extProps.externalMemoryProperties.externalMemoryFeatures;

            if (r != VK_SUCCESS)
            {
                return false;
            }

            return (extProps.externalMemoryProperties.compatibleHandleTypes & handleType) != 0
                   && (features & VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT) != 0
                   && (features & VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT) != 0;
        };

        bool found = false;
        for (VkImageTiling tiling : candidates)
        {
            VkExternalMemoryFeatureFlags features = 0;
            const bool exportable = probeTiling(tiling, features);

            ostringstream entry;
            entry << tilingName(tiling) << ": ";

            if (!exportable)
            {
                entry << "not exportable+importable for this handle type"
                      << " (features=0x" << std::hex << features << std::dec << ")";
                cfg.candidateLog.push_back(entry.str());
                continue;
            }

            //  Only the floor; the per-image decision is in getSharedImageInfo().
            const bool dedicatedOnly = (features & VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT) != 0;

            cfg.supported = true;
            cfg.tiling = tiling;
            cfg.externalFeatures = features;
            cfg.dedicatedAllocation = dedicatedOnly;
            cfg.probedTiling = tiling;
            cfg.probedDedicated = dedicatedOnly;

            entry << "exportable (features=0x" << std::hex << features << std::dec << ") -- selected";
            cfg.candidateLog.push_back(entry.str());
            found = true;
            break;
        }

        if (!found)
        {
            cfg.supported = false;
            cfg.rejectReason = "no candidate tiling is exportable at A2B10G10R10 with "
                               "COLOR_ATTACHMENT|TRANSFER_SRC usage";
            m_interopConfig = cfg;
            return;
        }

        //  Overrides last, so the record reports both values.
        VkImageTiling forcedTiling = VK_IMAGE_TILING_LINEAR;
        if (forcedTilingRequested(forcedTiling) && forcedTiling != cfg.tiling)
        {
            VkExternalMemoryFeatureFlags features = 0;
            if (probeTiling(forcedTiling, features))
            {
                cfg.tilingOverridden = true;
                cfg.tiling = forcedTiling;
                cfg.externalFeatures = features;
                cfg.dedicatedAllocation = (features & VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT) != 0;
            }
            else
            {
                cout << "WARNING: VulkanWindow: RV_VULKAN_FORCE_TILING=" << tilingName(forcedTiling)
                     << " refused -- the driver does not report it as exportable; using the negotiated " << tilingName(cfg.tiling) << endl;
            }
        }

        if (dedicatedAllocationDisabled() && cfg.dedicatedAllocation)
        {
            if ((cfg.externalFeatures & VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT) != 0)
            {
                cout << "WARNING: VulkanWindow: RV_VULKAN_FORCE_NO_DEDICATED refused -- the driver reports "
                     << "DEDICATED_ONLY for this configuration, which is a requirement rather than a preference" << endl;
            }
            else
            {
                cfg.dedicatedOverridden = true;
                cfg.dedicatedAllocation = false;
            }
        }
        else if (dedicatedAllocationDisabled())
        {
            //  getSharedImageInfo() still needs to know the override is active.
            cfg.dedicatedOverridden = true;
        }

        m_interopConfig = cfg;
    }

    void VulkanWindow::reportPresentPath(PresentPath path, const std::string& reason)
    {
        m_presentPath = path;
        m_presentPathReason = reason;
        emitPresentationRecord();
    }

    void VulkanWindow::reportGLImportState(VkImageTiling tiling, bool dedicated)
    {
        m_glImportTiling = tiling;
        m_glImportDedicated = dedicated;
        m_glImportReported = true;
    }

    void VulkanWindow::emitPresentationRecord()
    {
        if (m_recordEmitted)
        {
            return;
        }
        m_recordEmitted = true;

        VkPhysicalDeviceProperties props = {};
        if (m_vkPhysicalDevice != VK_NULL_HANDLE)
        {
            vkGetPhysicalDeviceProperties(m_vkPhysicalDevice, &props);
        }

        const char* pathName = "undetermined";
        switch (m_presentPath)
        {
        case PresentPath::ZeroCopy:
            pathName = "GPU zero-copy interop (10-bit)";
            break;
        case PresentPath::CpuReadback:
            pathName = "CPU readback (10-bit, slower)";
            break;
        case PresentPath::OpenGL:
            pathName = "OpenGL (Vulkan abandoned; not 10-bit)";
            break;
        case PresentPath::Undetermined:
            break;
        }

        ostringstream o;
        o << "INFO: RV Vulkan presentation report\n";
        o << "INFO:   Role           : " << (isPassiveOutput() ? "presentation output" : "control viewport") << "\n";
        o << "INFO:   GPU            : " << (m_vkPhysicalDevice != VK_NULL_HANDLE ? props.deviceName : "(none)") << "  vendorID=0x"
          << std::hex << props.vendorID << std::dec << "  driverVersion=" << props.driverVersion
          << "  apiVersion=" << VK_VERSION_MAJOR(props.apiVersion) << "." << VK_VERSION_MINOR(props.apiVersion) << "."
          << VK_VERSION_PATCH(props.apiVersion) << "\n";
        o << "INFO:   Present path   : " << pathName << "\n";
        if (!m_presentPathReason.empty())
        {
            o << "INFO:   Reason         : " << m_presentPathReason << "\n";
        }
        o << "INFO:   Swapchain      : " << formatName(m_vkSwapchainFormat) << " / " << colorSpaceName(m_loggedSurfaceFormat.colorSpace)
          << "\n";

        const InteropConfig& c = m_interopConfig;
        if (c.supported)
        {
            o << "INFO:   Shared image   : " << formatName(c.format) << " tiling=" << tilingName(c.tiling)
              << " usage=COLOR_ATTACHMENT|TRANSFER_SRC\n";
            o << "INFO:   Dedicated alloc: " << (c.dedicatedAllocation ? "yes" : "no") << "  (driver externalMemoryFeatures=0x" << std::hex
              << c.externalFeatures << std::dec
              << (c.externalFeatures & VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT ? " DEDICATED_ONLY" : "") << ")\n";
            if (c.tilingOverridden)
            {
                o << "INFO:   Tiling override: RV_VULKAN_FORCE_TILING forced " << tilingName(c.tiling) << "; negotiation chose "
                  << tilingName(c.probedTiling) << "\n";
            }
            if (c.dedicatedOverridden)
            {
                o << "INFO:   Dedicated ovr  : RV_VULKAN_FORCE_NO_DEDICATED suppressed dedicated allocation; negotiation chose "
                  << (c.probedDedicated ? "yes" : "no") << "\n";
            }
            if (m_glImportReported)
            {
                const bool agree = m_glImportTiling == c.tiling;
                o << "INFO:   GL import      : tiling=" << tilingName(m_glImportTiling)
                  << " dedicated=" << (m_glImportDedicated ? "yes" : "no") << "  -- "
                  << (agree ? "tiling matches the Vulkan export" : "TILING DISAGREES WITH THE VULKAN EXPORT (expect a corrupted image)")
                  << "\n";
            }
        }
        else
        {
            o << "INFO:   Shared image   : not used -- " << (c.rejectReason.empty() ? "interop not negotiated" : c.rejectReason) << "\n";
        }

        for (const std::string& entry : c.candidateLog)
        {
            o << "INFO:   Probe candidate: " << entry << "\n";
        }

        //  TODO: remove once the probe is confirmed on Linux/NVIDIA.
        o << "INFO:   Legacy vendor heuristic would have chosen: "
          << (m_vkPhysicalDevice != VK_NULL_HANDLE && useOptimalTilingForInterop(m_vkPhysicalDevice) ? "OPTIMAL" : "LINEAR") << "\n";

        if (envFlagSet("RV_VULKAN_FORCE_CPU_PRESENT"))
        {
            o << "INFO:   Override       : RV_VULKAN_FORCE_CPU_PRESENT is set\n";
        }

        cout << o.str() << flush;
    }

    void VulkanWindow::cleanupSharedImage(uint32_t slot)
    {
        SharedImageInfo& info = m_sharedImageInfo[slot];

        if (m_vkDevice)
        {
            vkDeviceWaitIdle(m_vkDevice);

            if (m_vkSharedImage[slot])
            {
                vkDestroyImage(m_vkDevice, m_vkSharedImage[slot], nullptr);
                m_vkSharedImage[slot] = VK_NULL_HANDLE;
            }
            if (m_vkSharedImageMemory[slot])
            {
                vkFreeMemory(m_vkDevice, m_vkSharedImageMemory[slot], nullptr);
                m_vkSharedImageMemory[slot] = VK_NULL_HANDLE;
            }
            if (m_vkGlReadySemaphore[slot])
            {
                vkDestroySemaphore(m_vkDevice, m_vkGlReadySemaphore[slot], nullptr);
                m_vkGlReadySemaphore[slot] = VK_NULL_HANDLE;
            }
            if (m_vkVkReadySemaphore[slot])
            {
                vkDestroySemaphore(m_vkDevice, m_vkVkReadySemaphore[slot], nullptr);
                m_vkVkReadySemaphore[slot] = VK_NULL_HANDLE;
            }
        }

#ifdef PLATFORM_WINDOWS
        if (info.memoryHandle)
        {
            ::CloseHandle(static_cast<HANDLE>(info.memoryHandle));
            info.memoryHandle = nullptr;
        }
        if (info.glReadySemaphoreHandle)
        {
            ::CloseHandle(static_cast<HANDLE>(info.glReadySemaphoreHandle));
            info.glReadySemaphoreHandle = nullptr;
        }
        if (info.vkReadySemaphoreHandle)
        {
            ::CloseHandle(static_cast<HANDLE>(info.vkReadySemaphoreHandle));
            info.vkReadySemaphoreHandle = nullptr;
        }
#else
        if (info.memoryFd != -1)
        {
            ::close(info.memoryFd);
            info.memoryFd = -1;
        }
        if (info.glReadySemaphoreFd != -1)
        {
            ::close(info.glReadySemaphoreFd);
            info.glReadySemaphoreFd = -1;
        }
        if (info.vkReadySemaphoreFd != -1)
        {
            ::close(info.vkReadySemaphoreFd);
            info.vkReadySemaphoreFd = -1;
        }
#endif
        info.width = 0;
        info.height = 0;
        info.size = 0;
        info.capacityHeight = 0;
        info.tiling = VK_IMAGE_TILING_LINEAR;
        info.dedicatedAllocation = false;
        m_sharedCapacityW[slot] = 0;
        m_sharedCapacityH[slot] = 0;
    }

    //
    //  Skip frames under load (as Qt does for the OpenGL output) instead of
    //  deepening the GPU queue the control viewport then waits on. Gates on
    //  this device's own GPU work; the swapchain queue always has room.
    //
    bool VulkanWindow::canPresentNow()
    {
        if (!isPassiveOutput())
        {
            return true;
        }

        if (!m_vkDevice || !m_vkSwapchain)
        {
            return true;
        }

        //  Forward progress: under sustained load, force one blocking present
        //  rather than freezing the output.
        static const double kMaxStaleSeconds = 0.1;
        if (!m_lastPresentTimer.isRunning() || m_lastPresentTimer.elapsed() > kMaxStaleSeconds)
        {
            return true;
        }

        //  Every in-flight frame must have retired, not only this slot's.
        const VkResult r = vkWaitForFences(m_vkDevice, kFramesInFlight, m_vkFence.data(), VK_TRUE, 0);
        if (r == VK_SUCCESS)
        {
            return true;
        }

        requestBestEffortRetry();
        return false;
    }

    //  Re-present a skipped best-effort frame; requestUpdate() coalesces, so at
    //  most one retry is pending.
    void VulkanWindow::requestBestEffortRetry()
    {
        if (!m_stopProcessingEvents && isExposed())
        {
            requestUpdate();
        }
    }

    void VulkanWindow::drainSharedSemaphores(uint32_t slot)
    {
        if (!m_vkDevice || !m_vkGlReadySemaphore[slot] || !m_vkVkReadySemaphore[slot])
        {
            return;
        }

        VkSubmitInfo drain = {};
        drain.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        VkSemaphore waitSemaphores[] = {m_vkGlReadySemaphore[slot]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_TRANSFER_BIT};
        drain.waitSemaphoreCount = 1;
        drain.pWaitSemaphores = waitSemaphores;
        drain.pWaitDstStageMask = waitStages;
        drain.commandBufferCount = 0;
        VkSemaphore signalSemaphores[] = {m_vkVkReadySemaphore[slot]};
        drain.signalSemaphoreCount = 1;
        drain.pSignalSemaphores = signalSemaphores;

        VkResult r = vkQueueSubmit(m_vkQueue, 1, &drain, VK_NULL_HANDLE);
        if (r == VK_ERROR_DEVICE_LOST)
        {
            requestGLFallback();
        }
    }

    const VulkanWindow::SharedImageInfo* VulkanWindow::getSharedImageInfo(int w, int h)
    {
        if (!m_vkDevice || !m_externalInteropSupported)
        {
            return nullptr;
        }

        const uint32_t slot = m_currentFrame;
        SharedImageInfo& info = m_sharedImageInfo[slot];

        // Compare against the surface extent, not the requested size:
        // createSwapchain() can only match the surface, so a mismatched request
        // would recreate the swapchain every frame.
        VkExtent2D surfaceExtent = m_vkSwapchainExtent;
        {
            VkSurfaceCapabilitiesKHR caps = {};
            if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_vkPhysicalDevice, m_vkSurface, &caps) == VK_SUCCESS
                && caps.currentExtent.width != UINT32_MAX)
            {
                surfaceExtent = caps.currentExtent;
            }
        }

        if (!m_vkSwapchain || m_vkSwapchainExtent.width != surfaceExtent.width || m_vkSwapchainExtent.height != surfaceExtent.height)
        {
            if (!createSwapchain())
            {
                return nullptr;
            }
        }

        // Within capacity: reuse the export, update the used sub-region.
        if (m_vkSharedImage[slot] && w <= m_sharedCapacityW[slot] && h <= m_sharedCapacityH[slot])
        {
            info.width = w;
            info.height = h;
            return &info;
        }

        // Grow to at least this window's screen (not the primary: an output
        // lives on a second display) so drag-to-fullscreen allocates once.
        int screenW = 0;
        int screenH = 0;
        if (QScreen* scr = screen() ? screen() : QGuiApplication::primaryScreen())
        {
            const qreal dpr = scr->devicePixelRatio();
            screenW = static_cast<int>(scr->geometry().width() * dpr);
            screenH = static_cast<int>(scr->geometry().height() * dpr);
        }
        const int capW = std::max({w, screenW, m_sharedCapacityW[slot]});
        const int capH = std::max({h, screenH, m_sharedCapacityH[slot]});

        cleanupSharedImage(slot);

        if (!m_interopConfig.supported)
        {
            if (ImageRenderer::debugGpu())
            {
                cout << "INFO: VulkanWindow: getSharedImageInfo: interop unavailable (" << m_interopConfig.rejectReason
                     << "); using the CPU readback path" << endl;
            }
            return nullptr;
        }

        const bool optimalTiling = m_interopConfig.tiling == VK_IMAGE_TILING_OPTIMAL;

        //  Unconditional: fires about once per slot per session.
        cout << "INFO: VulkanWindow: getSharedImageInfo: (re)allocating shared image slot " << slot << " capacity " << capW << "x" << capH
             << " for request " << w << "x" << h << " tiling=" << tilingName(m_interopConfig.tiling) << endl;

        VkExternalMemoryImageCreateInfo extMemInfo = {};
        extMemInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
#ifdef PLATFORM_WINDOWS
        extMemInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
        extMemInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif

        VkImageCreateInfo imageInfo = {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.pNext = &extMemInfo;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        // Matches GL_RGB10_A2 whatever the swapchain format; an A2R10G10B10
        // swapchain is reconciled by a blit in presentSharedImage().
        imageInfo.format = VK_FORMAT_A2B10G10R10_UNORM_PACK32;
        imageInfo.extent = {static_cast<uint32_t>(capW), static_cast<uint32_t>(capH), 1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = m_interopConfig.tiling;
        imageInfo.usage = m_interopConfig.usage;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (vkCreateImage(m_vkDevice, &imageInfo, nullptr, &m_vkSharedImage[slot]) != VK_SUCCESS)
        {
            cerr << "ERROR: VulkanWindow: Failed to create shared image" << endl;
            return nullptr;
        }

        // A format mismatch needs a blit; without blit support, use the CPU
        // fallback.
        if (m_vkSwapchainFormat != VK_FORMAT_A2B10G10R10_UNORM_PACK32)
        {
            VkFormatProperties srcProps = {};
            VkFormatProperties dstProps = {};
            vkGetPhysicalDeviceFormatProperties(m_vkPhysicalDevice, VK_FORMAT_A2B10G10R10_UNORM_PACK32, &srcProps);
            vkGetPhysicalDeviceFormatProperties(m_vkPhysicalDevice, m_vkSwapchainFormat, &dstProps);
            const VkFormatFeatureFlags srcFeatures = optimalTiling ? srcProps.optimalTilingFeatures : srcProps.linearTilingFeatures;
            const bool blitOk =
                (srcFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) && (dstProps.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT);
            if (!blitOk)
            {
                if (ImageRenderer::debugGpu())
                {
                    cout << "INFO: VulkanWindow: GPU interop unavailable for " << formatName(m_vkSwapchainFormat)
                         << " swapchain (blit unsupported); using CPU fallback." << endl;
                }
                cleanupSharedImage(slot);
                return nullptr;
            }
        }

        // vkGetImageSubresourceLayout is only valid for LINEAR tiling.
        if (optimalTiling)
        {
            info.strideWidth = capW;
        }
        else
        {
            // Match the GL texture stride to a padded rowPitch.
            VkImageSubresource subresource = {};
            subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresource.mipLevel = 0;
            subresource.arrayLayer = 0;
            VkSubresourceLayout layout;
            vkGetImageSubresourceLayout(m_vkDevice, m_vkSharedImage[slot], &subresource, &layout);

            if (layout.rowPitch % 4 != 0)
            {
                // Not an integer pixel width; use the CPU fallback.
                cleanupSharedImage(slot);
                return nullptr;
            }
            info.strideWidth = static_cast<int>(layout.rowPitch / 4);
        }
        info.capacityHeight = capH;
        info.tiling = m_interopConfig.tiling;

        //  Some drivers (AMD on Windows) require dedicated memory for external
        //  images; binding non-dedicated memory silently yields an all-zero GL
        //  texture.
        VkMemoryDedicatedRequirements dedicatedReqs = {};
        dedicatedReqs.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS;

        VkMemoryRequirements2 memReqs2 = {};
        memReqs2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
        memReqs2.pNext = &dedicatedReqs;

        VkImageMemoryRequirementsInfo2 memReqsInfo = {};
        memReqsInfo.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2;
        memReqsInfo.image = m_vkSharedImage[slot];

        vkGetImageMemoryRequirements2(m_vkDevice, &memReqsInfo, &memReqs2);

        const VkMemoryRequirements& memReqs = memReqs2.memoryRequirements;

        //  The single decision GL mirrors. An override may relax a preference,
        //  never a requirement.
        bool useDedicated =
            m_interopConfig.dedicatedAllocation || dedicatedReqs.requiresDedicatedAllocation || dedicatedReqs.prefersDedicatedAllocation;
        if (dedicatedAllocationDisabled() && !dedicatedReqs.requiresDedicatedAllocation
            && (m_interopConfig.externalFeatures & VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT) == 0)
        {
            useDedicated = false;
        }

        info.dedicatedAllocation = useDedicated;

        cout << "INFO: VulkanWindow: getSharedImageInfo: shared image slot " << slot
             << " memory = " << (useDedicated ? "dedicated" : "non-dedicated")
             << " (driver requires=" << (dedicatedReqs.requiresDedicatedAllocation ? "yes" : "no")
             << " prefers=" << (dedicatedReqs.prefersDedicatedAllocation ? "yes" : "no") << ")" << endl;

        //  pNext chain, built back to front:
        //    allocInfo -> exportAllocInfo [-> dedicatedAllocInfo] [-> exportWin32Info]
        void* chain = nullptr;

#ifdef PLATFORM_WINDOWS
        //  Required by the spec for OPAQUE_WIN32 handles.
        VkExportMemoryWin32HandleInfoKHR exportWin32Info = {};
        exportWin32Info.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_KHR;
        exportWin32Info.pNext = chain;
        exportWin32Info.pAttributes = nullptr; // default security attributes
        exportWin32Info.dwAccess = GENERIC_ALL;
        exportWin32Info.name = nullptr; // unnamed: shared within this process only
        chain = &exportWin32Info;
#endif

        VkMemoryDedicatedAllocateInfo dedicatedAllocInfo = {};
        if (useDedicated)
        {
            dedicatedAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
            dedicatedAllocInfo.pNext = chain;
            dedicatedAllocInfo.image = m_vkSharedImage[slot];
            dedicatedAllocInfo.buffer = VK_NULL_HANDLE;
            chain = &dedicatedAllocInfo;
        }

        VkExportMemoryAllocateInfo exportAllocInfo = {};
        exportAllocInfo.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
        exportAllocInfo.pNext = chain;
#ifdef PLATFORM_WINDOWS
        exportAllocInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
        exportAllocInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.pNext = &exportAllocInfo;
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = findMemoryType(m_vkPhysicalDevice, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (allocInfo.memoryTypeIndex == UINT32_MAX)
        {
            cerr << "ERROR: VulkanWindow: No device-local memory type for shared image" << endl;
            cleanupSharedImage(slot);
            return nullptr;
        }

        if (vkAllocateMemory(m_vkDevice, &allocInfo, nullptr, &m_vkSharedImageMemory[slot]) != VK_SUCCESS)
        {
            cerr << "ERROR: VulkanWindow: Failed to allocate shared image memory" << endl;
            cleanupSharedImage(slot);
            return nullptr;
        }

        if (vkBindImageMemory(m_vkDevice, m_vkSharedImage[slot], m_vkSharedImageMemory[slot], 0) != VK_SUCCESS)
        {
            cerr << "ERROR: VulkanWindow: Failed to bind shared image memory" << endl;
            cleanupSharedImage(slot);
            return nullptr;
        }

#ifdef PLATFORM_WINDOWS
        auto pfnGetMemoryWin32HandleKHR =
            reinterpret_cast<PFN_vkGetMemoryWin32HandleKHR>(vkGetDeviceProcAddr(m_vkDevice, "vkGetMemoryWin32HandleKHR"));
        if (!pfnGetMemoryWin32HandleKHR)
        {
            cerr << "ERROR: VulkanWindow: vkGetMemoryWin32HandleKHR not found" << endl;
            cleanupSharedImage(slot);
            return nullptr;
        }

        VkMemoryGetWin32HandleInfoKHR getHandleInfo = {};
        getHandleInfo.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
        getHandleInfo.memory = m_vkSharedImageMemory[slot];
        getHandleInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;

        HANDLE memHandle = nullptr;
        if (pfnGetMemoryWin32HandleKHR(m_vkDevice, &getHandleInfo, &memHandle) != VK_SUCCESS || !memHandle)
        {
            cerr << "ERROR: VulkanWindow: Failed to get memory HANDLE" << endl;
            cleanupSharedImage(slot);
            return nullptr;
        }
#else
        auto pfnGetMemoryFdKHR = reinterpret_cast<PFN_vkGetMemoryFdKHR>(vkGetDeviceProcAddr(m_vkDevice, "vkGetMemoryFdKHR"));
        if (!pfnGetMemoryFdKHR)
        {
            cerr << "ERROR: VulkanWindow: vkGetMemoryFdKHR not found" << endl;
            cleanupSharedImage(slot);
            return nullptr;
        }

        VkMemoryGetFdInfoKHR getFdInfo = {};
        getFdInfo.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
        getFdInfo.memory = m_vkSharedImageMemory[slot];
        getFdInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;

        int memFd = -1;
        if (pfnGetMemoryFdKHR(m_vkDevice, &getFdInfo, &memFd) != VK_SUCCESS)
        {
            cerr << "ERROR: VulkanWindow: Failed to get memory FD" << endl;
            cleanupSharedImage(slot);
            return nullptr;
        }
#endif

        VkExportSemaphoreCreateInfo exportSemInfo = {};
        exportSemInfo.sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO;
#ifdef PLATFORM_WINDOWS
        exportSemInfo.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
        exportSemInfo.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif

        VkSemaphoreCreateInfo semInfo = {};
        semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semInfo.pNext = &exportSemInfo;

        if (vkCreateSemaphore(m_vkDevice, &semInfo, nullptr, &m_vkGlReadySemaphore[slot]) != VK_SUCCESS
            || vkCreateSemaphore(m_vkDevice, &semInfo, nullptr, &m_vkVkReadySemaphore[slot]) != VK_SUCCESS)
        {
            cerr << "ERROR: VulkanWindow: Failed to create shared semaphores" << endl;
            cleanupSharedImage(slot);
            return nullptr;
        }

#ifdef PLATFORM_WINDOWS
        auto pfnGetSemaphoreWin32HandleKHR =
            reinterpret_cast<PFN_vkGetSemaphoreWin32HandleKHR>(vkGetDeviceProcAddr(m_vkDevice, "vkGetSemaphoreWin32HandleKHR"));
        if (!pfnGetSemaphoreWin32HandleKHR)
        {
            cerr << "ERROR: VulkanWindow: vkGetSemaphoreWin32HandleKHR not found" << endl;
            cleanupSharedImage(slot);
            return nullptr;
        }

        VkSemaphoreGetWin32HandleInfoKHR getSemHandleInfo = {};
        getSemHandleInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_WIN32_HANDLE_INFO_KHR;
        getSemHandleInfo.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;

        HANDLE glReadyHandle = nullptr;
        HANDLE vkReadyHandle = nullptr;

        getSemHandleInfo.semaphore = m_vkGlReadySemaphore[slot];
        if (pfnGetSemaphoreWin32HandleKHR(m_vkDevice, &getSemHandleInfo, &glReadyHandle) != VK_SUCCESS || !glReadyHandle)
        {
            cerr << "ERROR: VulkanWindow: Failed to get glReady semaphore HANDLE" << endl;
            cleanupSharedImage(slot);
            return nullptr;
        }

        getSemHandleInfo.semaphore = m_vkVkReadySemaphore[slot];
        if (pfnGetSemaphoreWin32HandleKHR(m_vkDevice, &getSemHandleInfo, &vkReadyHandle) != VK_SUCCESS || !vkReadyHandle)
        {
            cerr << "ERROR: VulkanWindow: Failed to get vkReady semaphore HANDLE" << endl;
            cleanupSharedImage(slot);
            return nullptr;
        }

        info.memoryHandle = memHandle;
        info.size = memReqs.size;
        info.width = w;
        info.height = h;
        info.glReadySemaphoreHandle = glReadyHandle;
        info.vkReadySemaphoreHandle = vkReadyHandle;
#else
        auto pfnGetSemaphoreFdKHR = reinterpret_cast<PFN_vkGetSemaphoreFdKHR>(vkGetDeviceProcAddr(m_vkDevice, "vkGetSemaphoreFdKHR"));
        if (!pfnGetSemaphoreFdKHR)
        {
            cerr << "ERROR: VulkanWindow: vkGetSemaphoreFdKHR not found" << endl;
            cleanupSharedImage(slot);
            return nullptr;
        }

        VkSemaphoreGetFdInfoKHR getSemFdInfo = {};
        getSemFdInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR;
        getSemFdInfo.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT;

        int glReadyFd = -1;
        int vkReadyFd = -1;

        getSemFdInfo.semaphore = m_vkGlReadySemaphore[slot];
        if (pfnGetSemaphoreFdKHR(m_vkDevice, &getSemFdInfo, &glReadyFd) != VK_SUCCESS || glReadyFd < 0)
        {
            cerr << "ERROR: VulkanWindow: Failed to get glReady semaphore FD" << endl;
            cleanupSharedImage(slot);
            return nullptr;
        }

        getSemFdInfo.semaphore = m_vkVkReadySemaphore[slot];
        if (pfnGetSemaphoreFdKHR(m_vkDevice, &getSemFdInfo, &vkReadyFd) != VK_SUCCESS || vkReadyFd < 0)
        {
            cerr << "ERROR: VulkanWindow: Failed to get vkReady semaphore FD" << endl;
            cleanupSharedImage(slot);
            return nullptr;
        }

        info.memoryFd = memFd;
        info.size = memReqs.size;
        info.width = w;
        info.height = h;
        info.glReadySemaphoreFd = glReadyFd;
        info.vkReadySemaphoreFd = vkReadyFd;
#endif

        VkCommandBuffer cb = m_vkCommandBuffers[0];
        vkResetCommandBuffer(cb, 0);

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cb, &beginInfo);

        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = m_vkSharedImage[slot];
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        vkEndCommandBuffer(cb);

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cb;

        vkResetFences(m_vkDevice, 1, &m_vkFence[slot]);
        VkResult layoutSubmitResult = vkQueueSubmit(m_vkQueue, 1, &submitInfo, m_vkFence[slot]);
        if (layoutSubmitResult != VK_SUCCESS)
        {
            if (layoutSubmitResult == VK_ERROR_DEVICE_LOST)
            {
                requestGLFallback();
            }
            cleanupSharedImage(slot);
            return nullptr;
        }
        vkWaitForFences(m_vkDevice, 1, &m_vkFence[slot], VK_TRUE, UINT64_MAX);

        // Signal vkReady so GL can write the first frame.
        VkSubmitInfo signalInfo = {};
        signalInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        signalInfo.signalSemaphoreCount = 1;
        signalInfo.pSignalSemaphores = &m_vkVkReadySemaphore[slot];
        VkResult signalResult = vkQueueSubmit(m_vkQueue, 1, &signalInfo, VK_NULL_HANDLE);
        if (signalResult != VK_SUCCESS)
        {
            if (signalResult == VK_ERROR_DEVICE_LOST)
            {
                requestGLFallback();
            }
            cleanupSharedImage(slot);
            return nullptr;
        }

        m_sharedCapacityW[slot] = capW;
        m_sharedCapacityH[slot] = capH;

        return &info;
    }

    //--------------------------------------------------------------------------
    // presentSharedImage
    //--------------------------------------------------------------------------

    void VulkanWindow::presentSharedImage()
    {
        const uint32_t slot = m_currentFrame;
        const SharedImageInfo& info = m_sharedImageInfo[slot];

        if (!m_vkDevice || !m_vkSharedImage[slot] || !m_vkSwapchain)
        {
            return;
        }

        //  Fence wait vs acquire timing tells GPU from vblank back-pressure.
        const bool diagPresent = IPCore::ImageRenderer::debugGpu() && m_doc;
        Timer diagTimer;

        //  The fence wait and acquire pace the control viewport to refresh. A
        //  passive output polls instead, so a second display's vblank never
        //  enters the loop; a skipped frame keeps the previous image.
        const bool bestEffort = isPassiveOutput();
        static const double kMaxStaleSeconds = 0.1;
        const bool forceProgress = bestEffort && (!m_lastPresentTimer.isRunning() || m_lastPresentTimer.elapsed() > kMaxStaleSeconds);
        const uint64_t waitTimeout = (!bestEffort || forceProgress) ? UINT64_MAX : 0;

        if (diagPresent)
        {
            diagTimer.start();
        }

        VkResult fenceResult = vkWaitForFences(m_vkDevice, 1, &m_vkFence[slot], VK_TRUE, waitTimeout);

        if (diagPresent)
        {
            s_diagFenceWaitMs += diagTimer.elapsed() * 1000.0;
        }

        if (fenceResult == VK_TIMEOUT)
        {
            //  The slot is not advanced: the next frame retries it.
            drainSharedSemaphores(slot);
            requestBestEffortRetry();
            return;
        }
        if (fenceResult != VK_SUCCESS)
        {
            requestGLFallback();
            return;
        }

        uint32_t imageIndex;
        if (diagPresent)
        {
            diagTimer.start();
        }

        VkResult result =
            vkAcquireNextImageKHR(m_vkDevice, m_vkSwapchain, waitTimeout, m_vkImageAvailableSemaphore[slot], VK_NULL_HANDLE, &imageIndex);

        if (diagPresent)
        {
            s_diagAcquireMs += diagTimer.elapsed() * 1000.0;
        }

        if (result == VK_NOT_READY || result == VK_TIMEOUT)
        {
            //  Skip here, not after a successful acquire: a failed acquire
            //  leaves the semaphore unsignaled.
            drainSharedSemaphores(slot);
            requestBestEffortRetry();
            return;
        }

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            drainSharedSemaphores(slot);
            handleSwapchainOutOfDate();
            return;
        }
        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            drainSharedSemaphores(slot);
            if (result == VK_ERROR_DEVICE_LOST)
            {
                requestGLFallback();
            }
            return;
        }

        if (m_imagesInFlight[imageIndex] != VK_NULL_HANDLE)
        {
            vkWaitForFences(m_vkDevice, 1, &m_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
        }
        m_imagesInFlight[imageIndex] = m_vkFence[slot];

        vkResetFences(m_vkDevice, 1, &m_vkFence[slot]);

        VkCommandBuffer cb = m_vkCommandBuffers[imageIndex];
        vkResetCommandBuffer(cb, 0);

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cb, &beginInfo);

        VkImageMemoryBarrier sharedBarrier = {};
        sharedBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        sharedBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        sharedBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        sharedBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        sharedBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        sharedBarrier.image = m_vkSharedImage[slot];
        sharedBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        sharedBarrier.subresourceRange.baseMipLevel = 0;
        sharedBarrier.subresourceRange.levelCount = 1;
        sharedBarrier.subresourceRange.baseArrayLayer = 0;
        sharedBarrier.subresourceRange.layerCount = 1;
        sharedBarrier.srcAccessMask = 0;
        sharedBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                             &sharedBarrier);

        // Transition swapchain image to transfer dst
        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = m_vkSwapchainImages[imageIndex];
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        // Same format: raw copy. A2R10G10B10: a raw copy would swap R and B,
        // so blit (per-component conversion). The destination is bounded by
        // the swapchain: a stale devicePixelRatio can inflate the request.
        if (m_vkSwapchainFormat == VK_FORMAT_A2B10G10R10_UNORM_PACK32)
        {
            //  vkCmdCopyImage cannot scale, so clamp to the overlapping region.
            VkImageCopy region = {};
            region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.srcSubresource.layerCount = 1;
            region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.dstSubresource.layerCount = 1;
            region.extent = {std::min(static_cast<uint32_t>(info.width), m_vkSwapchainExtent.width),
                             std::min(static_cast<uint32_t>(info.height), m_vkSwapchainExtent.height), 1};

            vkCmdCopyImage(cb, m_vkSharedImage[slot], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_vkSwapchainImages[imageIndex],
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        }
        else
        {
            VkImageBlit blit = {};
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.layerCount = 1;
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.layerCount = 1;
            blit.srcOffsets[0] = {0, 0, 0};
            blit.srcOffsets[1] = {info.width, info.height, 1};
            blit.dstOffsets[0] = {0, 0, 0};
            blit.dstOffsets[1] = {static_cast<int32_t>(m_vkSwapchainExtent.width), static_cast<int32_t>(m_vkSwapchainExtent.height), 1};

            vkCmdBlitImage(cb, m_vkSharedImage[slot], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_vkSwapchainImages[imageIndex],
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_NEAREST);
        }

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = 0;

        vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1,
                             &barrier);

        vkEndCommandBuffer(cb);

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {m_vkGlReadySemaphore[slot], m_vkImageAvailableSemaphore[slot]};
        // TRANSFER, not COLOR_ATTACHMENT_OUTPUT: the swapchain image is first
        // touched by its TRANSFER_DST transition.
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT};
        submitInfo.waitSemaphoreCount = 2;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cb;

        VkSemaphore signalSemaphores[] = {m_vkRenderFinished[imageIndex], m_vkVkReadySemaphore[slot]};
        submitInfo.signalSemaphoreCount = 2;
        submitInfo.pSignalSemaphores = signalSemaphores;

        VkResult submitResult = vkQueueSubmit(m_vkQueue, 1, &submitInfo, m_vkFence[slot]);
        if (submitResult != VK_SUCCESS)
        {
            if (submitResult == VK_ERROR_DEVICE_LOST)
            {
                requestGLFallback();
            }
            return;
        }

        if (m_doc && s_diagFrameEventTime >= 0.0)
        {
            s_diagSlotEventTime[slot] = s_diagFrameEventTime;
            s_diagSlotArmed[slot] = true;
            s_diagFrameEventTime = -1.0;
        }

        m_currentFrame = (m_currentFrame + 1) % kFramesInFlight;

        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &m_vkRenderFinished[imageIndex];
        VkSwapchainKHR swapchains[] = {m_vkSwapchain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = &imageIndex;

        VkResult presentResult = vkQueuePresentKHR(m_vkQueue, &presentInfo);

        m_lastPresentTimer.stop();
        m_lastPresentTimer.start();

        //  See maxFramesInFlight(). After the present so the driver gets the
        //  frame early; never for the passive output.
        if (maxFramesInFlight() == 1 && !isPassiveOutput())
        {
            vkWaitForFences(m_vkDevice, 1, &m_vkFence[slot], VK_TRUE, UINT64_MAX);
        }
        // Recreate only on OUT_OF_DATE: some X11/RADV compositors report
        // SUBOPTIMAL persistently.
        if (presentResult == VK_ERROR_OUT_OF_DATE_KHR)
        {
            handleSwapchainOutOfDate();
            return;
        }
        if (presentResult != VK_SUCCESS && presentResult != VK_SUBOPTIMAL_KHR)
        {
            if (presentResult == VK_ERROR_DEVICE_LOST)
            {
                requestGLFallback();
            }
            return;
        }
    }

    //--------------------------------------------------------------------------
    // presentPixelData
    //--------------------------------------------------------------------------

    void VulkanWindow::presentPixelData(const void* pixels, int w, int h)
    {
        const uint32_t slot = m_currentFrame;

        if (!m_vkDevice)
        {
            return;
        }

        const bool diagPresent = IPCore::ImageRenderer::debugGpu() && m_doc;
        Timer diagTimer;

        if (!m_vkSwapchain || m_vkSwapchainExtent.width != static_cast<uint32_t>(w)
            || m_vkSwapchainExtent.height != static_cast<uint32_t>(h))
        {
            if (!createSwapchain())
            {
                return;
            }
        }

        // Same best-effort throttle as presentSharedImage().
        const bool bestEffort = isPassiveOutput();
        static const double kMaxStaleSeconds = 0.1;
        const bool forceProgress = bestEffort && (!m_lastPresentTimer.isRunning() || m_lastPresentTimer.elapsed() > kMaxStaleSeconds);
        const uint64_t waitTimeout = (!bestEffort || forceProgress) ? UINT64_MAX : 0;

        if (diagPresent)
        {
            diagTimer.start();
        }
        const VkResult fenceResult = vkWaitForFences(m_vkDevice, 1, &m_vkFence[slot], VK_TRUE, waitTimeout);
        if (diagPresent)
        {
            s_diagFenceWaitMs += diagTimer.elapsed() * 1000.0;
        }

        if (fenceResult == VK_TIMEOUT)
        {
            requestBestEffortRetry();
            return;
        }
        if (fenceResult != VK_SUCCESS)
        {
            requestGLFallback();
            return;
        }

        size_t size = w * h * 4;

        if (size > m_stagingBufferSize[slot])
        {
            if (m_vkStagingBuffer[slot])
            {
                vkDestroyBuffer(m_vkDevice, m_vkStagingBuffer[slot], nullptr);
            }
            if (m_vkStagingBufferMemory[slot])
            {
                vkFreeMemory(m_vkDevice, m_vkStagingBufferMemory[slot], nullptr);
            }

            VkBufferCreateInfo bufferInfo = {};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = size;
            bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            vkCreateBuffer(m_vkDevice, &bufferInfo, nullptr, &m_vkStagingBuffer[slot]);

            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(m_vkDevice, m_vkStagingBuffer[slot], &memRequirements);

            VkMemoryAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = findMemoryType(m_vkPhysicalDevice, memRequirements.memoryTypeBits,
                                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            if (allocInfo.memoryTypeIndex == UINT32_MAX)
            {
                cerr << "ERROR: VulkanWindow: No host-visible memory type for staging buffer" << endl;
                return;
            }

            vkAllocateMemory(m_vkDevice, &allocInfo, nullptr, &m_vkStagingBufferMemory[slot]);
            vkBindBufferMemory(m_vkDevice, m_vkStagingBuffer[slot], m_vkStagingBufferMemory[slot], 0);

            m_stagingBufferSize[slot] = size;
        }

        void* data;
        vkMapMemory(m_vkDevice, m_vkStagingBufferMemory[slot], 0, size, 0, &data);
        memcpy(data, pixels, size);
        vkUnmapMemory(m_vkDevice, m_vkStagingBufferMemory[slot]);

        uint32_t imageIndex;
        if (diagPresent)
        {
            diagTimer.start();
        }
        VkResult result =
            vkAcquireNextImageKHR(m_vkDevice, m_vkSwapchain, waitTimeout, m_vkImageAvailableSemaphore[slot], VK_NULL_HANDLE, &imageIndex);
        if (diagPresent)
        {
            s_diagAcquireMs += diagTimer.elapsed() * 1000.0;
        }
        if (result == VK_NOT_READY || result == VK_TIMEOUT)
        {
            requestBestEffortRetry();
            return;
        }
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            handleSwapchainOutOfDate();
            return;
        }
        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            if (result == VK_ERROR_DEVICE_LOST)
            {
                requestGLFallback();
            }
            return;
        }

        if (m_imagesInFlight[imageIndex] != VK_NULL_HANDLE)
        {
            vkWaitForFences(m_vkDevice, 1, &m_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
        }
        m_imagesInFlight[imageIndex] = m_vkFence[slot];

        vkResetFences(m_vkDevice, 1, &m_vkFence[slot]);

        VkCommandBuffer cb = m_vkCommandBuffers[imageIndex];
        vkResetCommandBuffer(cb, 0);

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cb, &beginInfo);

        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = m_vkSwapchainImages[imageIndex];
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkBufferImageCopy region = {};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {static_cast<uint32_t>(w), static_cast<uint32_t>(h), 1};

        vkCmdCopyBufferToImage(cb, m_vkStagingBuffer[slot], m_vkSwapchainImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                               &region);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = 0;

        vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1,
                             &barrier);

        vkEndCommandBuffer(cb);

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        VkSemaphore waitSemaphores[] = {m_vkImageAvailableSemaphore[slot]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_TRANSFER_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cb;
        VkSemaphore signalSemaphores[] = {m_vkRenderFinished[imageIndex]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        VkResult submitResult = vkQueueSubmit(m_vkQueue, 1, &submitInfo, m_vkFence[slot]);
        if (submitResult != VK_SUCCESS)
        {
            if (submitResult == VK_ERROR_DEVICE_LOST)
            {
                requestGLFallback();
            }
            return;
        }

        if (m_doc && s_diagFrameEventTime >= 0.0)
        {
            s_diagSlotEventTime[slot] = s_diagFrameEventTime;
            s_diagSlotArmed[slot] = true;
            s_diagFrameEventTime = -1.0;
        }

        m_currentFrame = (m_currentFrame + 1) % kFramesInFlight;

        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        VkSwapchainKHR swapchains[] = {m_vkSwapchain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = &imageIndex;

        VkResult presentResult = vkQueuePresentKHR(m_vkQueue, &presentInfo);

        m_lastPresentTimer.stop();
        m_lastPresentTimer.start();

        //  See maxFramesInFlight(). After the present so the driver gets the
        //  frame early; never for the passive output.
        if (maxFramesInFlight() == 1 && !isPassiveOutput())
        {
            vkWaitForFences(m_vkDevice, 1, &m_vkFence[slot], VK_TRUE, UINT64_MAX);
        }
        // See presentSharedImage() on SUBOPTIMAL.
        if (presentResult == VK_ERROR_OUT_OF_DATE_KHR)
        {
            handleSwapchainOutOfDate();
            return;
        }
        if (presentResult != VK_SUCCESS && presentResult != VK_SUBOPTIMAL_KHR)
        {
            if (presentResult == VK_ERROR_DEVICE_LOST)
            {
                requestGLFallback();
            }
            return;
        }
    }

    //--------------------------------------------------------------------------

    void VulkanWindow::render()
    {
        if (m_stopProcessingEvents)
        {
            return;
        }

        //  resizeEvent() can request an update before the first expose.
        if (!m_initialized)
        {
            return;
        }

        //  A passive output only gets its own UpdateRequest from
        //  requestBestEffortRetry(): re-present what is already composited.
        if (isPassiveOutput())
        {
            if (m_videoDevice)
            {
                m_videoDevice->syncBuffers();
            }
            return;
        }

        IPCore::Session* session = m_doc ? m_doc->session() : nullptr;
        if (!session)
        {
            return;
        }

        if (IPCore::ImageRenderer::debugGpu())
        {
            if (s_diagLoopTimer.isRunning())
            {
                s_diagLoopMs += s_diagLoopTimer.elapsed() * 1000.0;
            }
            s_diagLoopTimer.start();

            if (s_diagPointerPending)
            {
                s_diagPointerAgeMs += s_diagPointerTimer.elapsed() * 1000.0;
                ++s_diagPointerAgeSamples;
                s_diagPointerPending = false;
                s_diagFrameEventTime = diagNow() - s_diagPointerTimer.elapsed();
            }

            //  Close out retired slots (non-blocking; one frame of quantisation).
            for (uint32_t i = 0; i < kFramesInFlight; ++i)
            {
                if (s_diagSlotArmed[i] && m_vkDevice && m_vkFence[i] && vkGetFenceStatus(m_vkDevice, m_vkFence[i]) == VK_SUCCESS)
                {
                    s_diagEventToRetireMs += (diagNow() - s_diagSlotEventTime[i]) * 1000.0;
                    ++s_diagEventToRetireSamples;
                    s_diagSlotArmed[i] = false;
                }
            }
        }

        if (m_doc && session && m_videoDevice)
        {
            m_videoDevice->makeCurrent();

            if (m_userActive && m_activityTimer.elapsed() > 1.0)
            {
                if (m_doc->mainPopup() && !m_doc->mainPopup()->isVisible() && m_eventWidget && m_eventWidget->hasFocus())
                {
                    TwkApp::ActivityChangeEvent aevent("user-inactive", m_videoDevice);
                    m_videoDevice->sendEvent(aevent);
                    m_userActive = false;
                }
            }

            int x = 0, y = 0;
            absolutePosition(x, y);
            m_videoDevice->setAbsolutePosition(x, y);

            const bool diagTiming = IPCore::ImageRenderer::debugGpu();
            Timer diagTimer;
            if (diagTiming)
            {
                diagTimer.start();
            }

            session->render();

            if (diagTiming)
            {
                s_diagRenderMs += diagTimer.elapsed() * 1000.0;
            }

            if (!m_postFirstNonEmptyRender && session->postFirstNonEmptyRender())
            {
                m_postFirstNonEmptyRender = true;
                if (!session->isFullScreen())
                {
                    m_doc->resizeToFit(false, false);
                    m_doc->center();
                }
            }

            m_firstPaintCompleted = true;
        }

        if (m_stopProcessingEvents)
        {
            return;
        }

        if (session && m_videoDevice)
        {
            //  Always present the control viewport, even with a separate output
            //  device: unlike GL, nothing else composites it.
            const bool diagPresent = IPCore::ImageRenderer::debugGpu();
            Timer diagPresentTimer;
            if (diagPresent)
            {
                diagPresentTimer.start();
            }

            m_videoDevice->syncBuffers();

            if (diagPresent)
            {
                s_diagMainPresentMs += diagPresentTimer.elapsed() * 1000.0;
            }

            if (session->outputVideoDevice() && session->outputVideoDevice() != videoDevice())
            {
                if (diagPresent)
                {
                    diagPresentTimer.start();
                }

                session->outputVideoDevice()->syncBuffers();

                if (diagPresent)
                {
                    s_diagOutPresentMs += diagPresentTimer.elapsed() * 1000.0;
                }

                //  The output made its own GL context current; restore ours.
                m_videoDevice->makeCurrent();
            }
        }

        if (session)
        {
            const bool diagPost = IPCore::ImageRenderer::debugGpu();
            Timer diagPostTimer;
            if (diagPost)
            {
                diagPostTimer.start();
            }

            session->addSyncSample();
            session->postRender();

            if (diagPost)
            {
                s_diagPostRenderMs += diagPostTimer.elapsed() * 1000.0;
            }
        }

        //  Averaged -debug gpu breakdown every 60 frames.
        if (IPCore::ImageRenderer::debugGpu() && m_doc)
        {
            if (++s_diagFrames >= 60)
            {
                const double n = double(s_diagFrames);
                const double loopMs = s_diagLoopMs / n;
                cout << "INFO: VulkanWindow frame avg over " << s_diagFrames << " [depth=" << maxFramesInFlight()
                     << " tiling=" << tilingName(m_sharedImageInfo[0].tiling) << "]"
                     << ": session->render()=" << (s_diagRenderMs / n) << "ms  mainPresent=" << (s_diagMainPresentMs / n)
                     << "ms  outputPresent=" << (s_diagOutPresentMs / n)
                     << "ms  total=" << ((s_diagRenderMs + s_diagMainPresentMs + s_diagOutPresentMs) / n)
                     << "ms   [mainPresent breakdown: fenceWait=" << (s_diagFenceWaitMs / n) << "ms acquire=" << (s_diagAcquireMs / n)
                     << "ms]"
                     << "  postRender=" << (s_diagPostRenderMs / n) << "ms  frameInterval=" << loopMs << "ms ("
                     << (loopMs > 0.0 ? 1000.0 / loopMs : 0.0) << " fps)"
                     << "  pointer: events=" << s_diagPointerEvents
                     << " handler=" << (s_diagPointerEvents ? s_diagPointerHandlerMs / s_diagPointerEvents : 0.0)
                     << "ms eventToRender=" << (s_diagPointerAgeSamples ? s_diagPointerAgeMs / s_diagPointerAgeSamples : 0.0)
                     << "ms eventToRetire=" << (s_diagEventToRetireSamples ? s_diagEventToRetireMs / s_diagEventToRetireSamples : 0.0)
                     << "ms" << endl;
                s_diagFrames = 0;
                s_diagRenderMs = 0.0;
                s_diagMainPresentMs = 0.0;
                s_diagOutPresentMs = 0.0;
                s_diagFenceWaitMs = 0.0;
                s_diagAcquireMs = 0.0;
                s_diagLoopMs = 0.0;
                s_diagPostRenderMs = 0.0;
                s_diagPointerHandlerMs = 0.0;
                s_diagPointerEvents = 0;
                s_diagPointerAgeMs = 0.0;
                s_diagPointerAgeSamples = 0;
                s_diagEventToRetireMs = 0.0;
                s_diagEventToRetireSamples = 0;
            }
        }

        m_eventProcessingTimer.start();
    }

    //--------------------------------------------------------------------------
    // QWindow overrides
    //--------------------------------------------------------------------------

    void VulkanWindow::exposeEvent(QExposeEvent* event)
    {
        QWindow::exposeEvent(event);

        if (m_stopProcessingEvents || !isExposed())
        {
            return;
        }

        if (m_initialized && handle() != m_initializedHandle)
        {
            handleSurfaceLost();
        }

        if (!m_initialized)
        {
            initialize();
        }

        //  Show the last composited frame instead of blank until the next
        //  main-view frame.
        if (!m_doc && m_initialized && m_videoDevice)
        {
            m_videoDevice->syncBuffers();
            return;
        }

        requestUpdate();
    }

    void VulkanWindow::resizeEvent(QResizeEvent* event)
    {
        if (m_doc)
        {
            m_doc->viewSizeChanged(event->size().width(), event->size().height());
        }
        QWindow::resizeEvent(event);

        // Nothing else repaints this surface on resize; requestUpdate() coalesces.
        if (!m_stopProcessingEvents)
        {
            requestUpdate();
        }
    }

    //--------------------------------------------------------------------------
    // eventProcessingTimeout slot
    //--------------------------------------------------------------------------

    void VulkanWindow::eventProcessingTimeout()
    {
        if (m_doc && m_doc->session())
        {
            m_doc->session()->userGenericEvent("per-render-event-processing", "");
        }
    }

    //--------------------------------------------------------------------------
    // event()
    //--------------------------------------------------------------------------

    bool VulkanWindow::event(QEvent* event)
    {
        //
        //  Before every guard below: this is the last point the Vulkan objects
        //  can legally be destroyed. The VkSurfaceKHR dies with the
        //  QPlatformWindow, and QWindowContainer destroys it before deleting
        //  this window, so releasing later segfaults inside the driver.
        //
        if (event->type() == QEvent::PlatformSurface)
        {
            if (static_cast<QPlatformSurfaceEvent*>(event)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed)
            {
                releaseVulkanResources();
            }
            return QWindow::event(event);
        }

        if (!m_videoDevice)
        {
            return QWindow::event(event);
        }

        bool keyevent = false;
        Rv::Session* session = m_doc ? m_doc->session() : nullptr;

        if (m_stopProcessingEvents)
        {
            event->accept();
            return true;
        }

        if (event->type() == QEvent::WindowActivate)
        {
            m_activationTimer.start();
        }

        float activationTime = 0.0f;
        if (m_activationTimer.isRunning())
        {
            if (event->type() == QEvent::MouseButtonPress)
            {
                activationTime = m_activationTimer.elapsed();
                m_activationTimer.stop();
            }
            if (event->type() == QEvent::MouseMove)
            {
                m_activationTimer.stop();
            }
        }

        if (event->type() != QEvent::Paint)
        {
            m_activityTimer.stop();
            m_activityTimer.start();

            if (!m_userActive)
            {
                TwkApp::ActivityChangeEvent aevent("user-active", m_videoDevice);
                m_userActive = true;
                m_videoDevice->sendEvent(aevent);
            }
        }

        if (QKeyEvent* kevent = dynamic_cast<QKeyEvent*>(event))
        {
            keyevent = true;
            if (m_lastKey == kevent->key()
                && (m_lastKeyType == QEvent::ShortcutOverride && (kevent->type() == QEvent::KeyPress) || (m_lastKeyType == kevent->type())))
            {
                m_lastKey = kevent->key();
                m_lastKeyType = kevent->type();
                event->accept();
                return true;
            }
            m_lastKeyType = kevent->type();
            m_lastKey = kevent->key();
        }

        switch (event->type())
        {
        case QEvent::FocusIn:
            //  Drop stale modifiers. A passive output has no translator.
            if (m_videoDevice->hasTranslator())
            {
                m_videoDevice->translator().resetModifiers();
            }
            break;

        case QEvent::Enter:
            //  A widget focus change, not a window activation, so hovering
            //  never steals activation from another window. Skipped when
            //  already focused: a repeat FocusIn makes QWindowContainer move
            //  focus to the next widget in the tab chain.
            if (QGuiApplication::focusWindow() != this && m_eventWidget)
            {
                m_eventWidget->setFocus(Qt::MouseFocusReason);
            }
            break;

        default:
            break;
        }

        if (event->type() == QEvent::Resize)
        {
            QResizeEvent* e = static_cast<QResizeEvent*>(event);
            if (!isVisible())
            {
                return true;
            }
            if (e->oldSize().width() != -1 && e->oldSize().height() != -1)
            {
                ostringstream contents;
                contents << e->oldSize().width() << " " << e->oldSize().height() << "|" << e->size().width() << " " << e->size().height();
                if (m_doc && session)
                {
                    session->userGenericEvent("view-resized", contents.str());
                }
            }
            return QWindow::event(event);
        }

        if (event->type() == QEvent::UpdateRequest)
        {
            render();
            return true;
        }

        if (!m_videoDevice || !m_videoDevice->hasTranslator())
        {
            return QWindow::event(event);
        }

        auto resetTranslator = [this]()
        {
            m_videoDevice->translator().setScaleAndOffset(0, 0, 1.0f, 1.0f);
            m_videoDevice->translator().setRelativeDomain(width(), height());
        };

        if (session && session->outputVideoDevice()
            && session->outputVideoDevice()->displayMode() == TwkApp::VideoDevice::MirrorDisplayMode)
        {
            if (const TwkApp::VideoDevice* cdv = session->controlVideoDevice())
            {
                const TwkApp::VideoDevice* odv = session->outputVideoDevice();
                if (odv && cdv != odv && cdv == videoDevice())
                {
                    const float w = static_cast<float>(width());
                    const float h = static_cast<float>(height());
                    const float ow = static_cast<float>(odv->width());
                    const float oh = static_cast<float>(odv->height());
                    const float aspect = w / h;
                    const float oaspect = ow / oh;

                    m_videoDevice->translator().setRelativeDomain(ow, oh);

                    if (aspect >= oaspect)
                    {
                        const float yscale = oh / h;
                        const float yoffset = 0.0f;
                        const float xscale = yscale;
                        const float xoffset = -(w * yscale - ow) / 2.0f;
                        m_videoDevice->translator().setScaleAndOffset(xoffset, yoffset, xscale, yscale);
                    }
                    else
                    {
                        const float xscale = ow / w;
                        const float xoffset = 0.0f;
                        const float yscale = xscale;
                        const float yoffset = -(xscale * h - oh) / 2.0f;
                        m_videoDevice->translator().setScaleAndOffset(xoffset, yoffset, xscale, yscale);
                    }
                }
                else
                {
                    resetTranslator();
                }
            }
            else
            {
                resetTranslator();
            }
        }
        else
        {
            resetTranslator();
        }

        if (session)
        {
            session->setEventVideoDevice(videoDevice());
        }

        //  Dispatch into Mu is synchronous, so this times the handler.
        const bool diagPointer =
            IPCore::ImageRenderer::debugGpu()
            && (event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonPress || event->type() == QEvent::TabletMove);
        Timer diagPointerTimer;
        if (diagPointer)
        {
            diagPointerTimer.start();
            s_diagPointerTimer.start();
            s_diagPointerPending = true;
        }

        const bool handled = m_videoDevice->translator().sendQTEvent(event, activationTime);

        if (diagPointer)
        {
            s_diagPointerHandlerMs += diagPointerTimer.elapsed() * 1000.0;
            ++s_diagPointerEvents;
        }

        if (handled)
        {
            event->accept();
            return true;
        }
        else
        {
            return QWindow::event(event);
        }
    }

} // namespace Rv

#endif // PLATFORM_LINUX || PLATFORM_WINDOWS
