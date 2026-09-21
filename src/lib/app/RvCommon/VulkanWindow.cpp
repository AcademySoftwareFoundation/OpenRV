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
#include <climits>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
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

namespace Rv
{
    using namespace std;

    //  Accumulators for the -debug gpu frame-time report in render(). One
    //  VulkanWindow drives the frame loop, so file statics are sufficient.
    static unsigned int s_diagFrames = 0;
    static double s_diagRenderMs = 0.0;
    static double s_diagMainPresentMs = 0.0;
    static double s_diagOutPresentMs = 0.0;
    static double s_diagFenceWaitMs = 0.0;
    static double s_diagAcquireMs = 0.0;
    //  Wall clock between successive render() entries: the loop period.
    static double s_diagLoopMs = 0.0;
    static TwkUtil::Timer s_diagLoopTimer;
    //  addSyncSample()/postRender() run inside render() but after the presents,
    //  so they are part of the frame period without being part of "total".
    static double s_diagPostRenderMs = 0.0;
    //  Pointer side of the same report. handler = time spent inside the
    //  Mu/annotation handler for one pointer event; eventToRender = age of the
    //  newest pointer event when the frame answering it starts rendering, so it
    //  includes the handler and any wait in the event loop.
    static double s_diagPointerHandlerMs = 0.0;
    static unsigned int s_diagPointerEvents = 0;
    static double s_diagPointerAgeMs = 0.0;
    static unsigned int s_diagPointerAgeSamples = 0;
    static TwkUtil::Timer s_diagPointerTimer;
    static bool s_diagPointerPending = false;

    //  eventToRetire: age of the pointer event a frame answered, measured when
    //  that frame's GPU work retires. This is the end-to-end interactive
    //  latency; eventToRender covers only the input half of it.
    //
    //  Closing a sample out needs an absolute clock, because a frame retires
    //  some frames after the event that produced it, so one free-running timer
    //  is read as a timestamp source.
    static TwkUtil::Timer s_diagClock;

    static double diagNow()
    {
        if (!s_diagClock.isRunning())
            s_diagClock.start();
        return s_diagClock.elapsed();
    }

    //  Timestamp of the pointer event the frame currently being rendered
    //  answers (-1 when this frame answers no new event), handed to the
    //  in-flight slot when that frame is submitted.
    static double s_diagFrameEventTime = -1.0;
    static std::array<double, VulkanWindow::FRAMES_IN_FLIGHT> s_diagSlotEventTime{};
    static std::array<bool, VulkanWindow::FRAMES_IN_FLIGHT> s_diagSlotArmed{};
    static double s_diagEventToRetireMs = 0.0;
    static unsigned int s_diagEventToRetireSamples = 0;

    using namespace TwkApp;
    using namespace IPCore;

    // Both A2B10G10R10 and A2R10G10B10 are 10-bit-per-channel packed formats;
    // they differ only in R/B component order. Both are acceptable for 10-bit
    // presentation -- the R/B order is handled where pixels are packed (CPU
    // fallback) or blitted (GPU interop). A2B10G10R10 (== GL_RGB10_A2) is
    // preferred when the surface offers it, but many Linux/RADV surfaces only
    // advertise A2R10G10B10.
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
            return false;

        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        if (vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, formats.data()) != VK_SUCCESS)
            return false;

        return std::any_of(formats.begin(), formats.end(), [](const VkSurfaceFormatKHR& format) { return isTenBitFormat(format.format); });
    }

    static bool deviceHasExtension(VkPhysicalDevice device, const char* name)
    {
        uint32_t extensionCount = 0;
        if (vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr) != VK_SUCCESS)
            return false;

        std::vector<VkExtensionProperties> extensions(extensionCount);
        if (vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data()) != VK_SUCCESS)
            return false;

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
        //
        //  This is a real QWindow, so the surface type and format can simply be
        //  declared here -- no WA_NativeWindow / WA_PaintOnScreen dance, and no
        //  waiting for a widget to grow a windowHandle().
        //
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
        //
        //  m_videoDevice is owned by the hosting VulkanView, not by this window
        //  (see setVideoDevice); only the Vulkan resources are torn down here.
        //
        m_videoDevice = nullptr;

        //
        //  Normally a no-op: the QEvent::PlatformSurface / SurfaceAboutToBeDestroyed
        //  handler in event() has already released everything, because by the time
        //  a QWindow reaches its destructor its surface is usually gone. This is
        //  only the backstop for the paths where the window is deleted without
        //  ever having had a platform window destroyed under it.
        //
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
            return false;

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

        //
        //  NOTE: session initialization is deliberately NOT driven from here.
        //  Loading packages creates web panels, and adding a QWebEngineView
        //  makes Qt tear down the main window's native subtree -- destroying
        //  this view while this method is still on the stack, so every later
        //  member access is a use-after-free.
        //  RvApplication::newSessionFromFiles() calls
        //  RvDocument::initializeSession() after show() instead, with no view
        //  callback in the call chain.
        //
    }

    //
    //  One process-lifetime QVulkanInstance, shared by the 10-bit probe
    //  (supports10BitPresentation) and every VulkanWindow (initVulkan). Created
    //  lazily and never destroyed.
    //
    //  Tearing a VkInstance down and then creating or using another shortly
    //  after corrupts RADV's shared X11/xcb WSI state and segfaults a
    //  subsequent vkGetPhysicalDeviceSurfaceSupportKHR. That is exactly the
    //  sequence a mid-session GL->Vulkan promotion produces: the probe runs,
    //  drops its throwaway instance, and a VulkanWindow initializes moments
    //  later. Keeping one instance alive for the whole process removes the
    //  teardown entirely. initVulkan already relied on a never-destroyed
    //  static instance, so this only extends the same lifetime to the probe.
    //
    static QVulkanInstance* sharedVulkanInstance()
    {
        static QVulkanInstance* instance = []() -> QVulkanInstance*
        {
            auto* inst = new QVulkanInstance();
            // Device UUID matching uses vkGetPhysicalDeviceProperties2, which
            // is core in Vulkan 1.1. QVulkanInstance otherwise defaults to a
            // 1.0 instance even though the presentation code targets 1.1.
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
        //
        //  Memoized: 10-bit presentation support is a fixed hardware/driver
        //  property, so probe at most once per process. The probe uses the
        //  shared, never-destroyed instance and a leaked probe window (see
        //  sharedVulkanInstance), so it tears down nothing that could corrupt
        //  RADV's WSI state ahead of a later VulkanWindow init; memoization is
        //  then just an optimization that avoids re-running the device scan on
        //  every DesktopVideoDevice::shouldUseVulkanPresentation() call.
        //
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

            //
            //  Leak the probe window (process-lifetime, never shown).
            //  Destroying its Vulkan surface right before a real VulkanWindow
            //  init is part of the same WSI-teardown hazard as destroying the
            //  instance, so it is never torn down either.
            //
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

            //
            //  The probe window, its surface and the shared instance are all
            //  kept alive for the process lifetime, so nothing is torn down
            //  here.
            //
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
        // Create Instance
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "RV VulkanWindow";
        appInfo.apiVersion = VK_API_VERSION_1_1;

        // Need surface extensions
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

        // Reuse the one process-lifetime instance, shared with the 10-bit probe.
        // It is never destroyed: see sharedVulkanInstance for why tearing a
        // VkInstance down near another init crashes RADV's WSI.
        QVulkanInstance* qtVkInst = sharedVulkanInstance();
        if (!qtVkInst)
        {
            cerr << "ERROR: VulkanWindow: shared QVulkanInstance unavailable" << endl;
            return false;
        }

        m_vkInstance = qtVkInst->vkInstance();

        // Create Surface. The platform window must exist before Qt can hand out
        // a VkSurfaceKHR for it; VulkanView::create()s it up front, but
        // re-creating after a reparent can land here first.
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

        // Pick Physical Device
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
            //  Unconditional: runs once per window init, and pairing this name
            //  against QTVulkanVideoDevice's GL_RENDERER is how a hybrid-GPU
            //  machine (GL on the iGPU, Vulkan on the dGPU) is spotted from a
            //  plain QA log.
            VkPhysicalDeviceProperties props = {};
            vkGetPhysicalDeviceProperties(m_vkPhysicalDevice, &props);
            cout << "INFO: VulkanWindow: initVulkan: picked physical device '" << props.deviceName << "' (of " << deviceCount
                 << " available)" << endl;
        }

        // Create Logical Device
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

        auto failInit = [this]()
        {
            cleanupVulkan();
            return false;
        };

        // Command pool
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = m_queueFamilyIndex;
        if (vkCreateCommandPool(m_vkDevice, &poolInfo, nullptr, &m_vkCommandPool) != VK_SUCCESS)
        {
            return failInit();
        }

        // Sync objects. The per-swapchain-image renderFinished semaphores live
        // in createSwapchain (sized to the image count); here we create only the
        // per-in-flight-slot acquire semaphores and frame fences.
        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        // Per-in-flight-slot acquire semaphore + frame fence. Fences are created
        // signaled so the first wait on a slot passes without a prior submit.
        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
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
        //
        //  The platform window was destroyed and recreated underneath us, so
        //  the VkSurfaceKHR (and everything derived from it) belongs to a
        //  window that no longer exists. Qt does this whenever the top-level
        //  QWidgetWindow is replaced -- inserting a QWebEngineView is the
        //  common trigger -- and VulkanView re-parents this window into the
        //  new one afterwards (see VulkanView::reattachVulkanWindow).
        //
        //  Tear the Vulkan device down completely and re-initialize against the
        //  new handle. A swapchain recreate is not enough: the surface handle
        //  itself is stale, so vkGetPhysicalDeviceSurfaceCapabilitiesKHR and
        //  vkCreateSwapchainKHR would both be querying a dead object.
        //
        cout << "INFO: VulkanWindow: platform window recreated; rebuilding Vulkan surface" << endl;

        releaseVulkanResources();
    }

    //
    //  Release everything initVulkan()/createSwapchain()/getSharedImageInfo()
    //  built, in dependency order, and return to the pre-initialize() state so
    //  the next exposeEvent() re-initializes from scratch.
    //
    //  Ordering constraint: every one of these destroy calls is made against
    //  objects the driver ties back to the presentation surface -- and the
    //  VkSurfaceKHR is only valid while the platform window that produced it
    //  lives. Callers must therefore reach here *before* the platform window is
    //  destroyed, not after (see the QEvent::PlatformSurface handler).
    //
    void VulkanWindow::releaseVulkanResources()
    {
        //
        //  The GL side imported this window's shared device memory and
        //  semaphores as GL memory objects; drop those first so nothing on the
        //  GL side is left aliasing memory freed just below. syncBuffers()
        //  re-imports on the next frame if the window comes back.
        //
        if (m_videoDevice)
        {
            m_videoDevice->releaseSharedGLObjects();

            //  The next initVulkan() may land on a different VkPhysicalDevice,
            //  so the GL/Vulkan device-UUID match has to be probed again
            //  rather than reused from the device just torn down.
            m_videoDevice->resetInteropDeviceMatch();
        }

        for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
        {
            cleanupSharedImage(i);
        }
        cleanupSwapchain();
        cleanupVulkan();

        //  Owned by the platform window / QVulkanInstance, never destroyed here;
        //  just forget it, since it does not outlive the window it came from.
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

        // Recreate only the swapchain, not the shared image. The shared image is
        // a content-sized TRANSFER_SRC image, independent of the window-sized
        // swapchain. Keep the acquire semaphores too: they are per-frame
        // resources, not swapchain resources, and can still be referenced by
        // queued submissions when OUT_OF_DATE is reported. createSwapchain()
        // waits for the device before retiring swapchain-owned resources.
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

            for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
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
            // m_vkRenderFinished are per-swapchain-image; freed in cleanupSwapchain.

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
        // Surface is managed by QVulkanInstance? We shouldn't destroy it here if QVulkanInstance owns it, but wait, we got it from
        // surfaceForWindow. Actually QVulkanWindow destroys it. We can just leave it for QVulkanInstance to clean up, or we can
        // vkDestroySurfaceKHR if needed. For safety we don't destroy instance/surface here, they are tied to Qt.
    }

    //
    //  Present mode
    //
    //  FIFO everywhere, which is what a viewport and a presentation output
    //  both want: every image scanned out, none torn.
    //
    //  The env overrides are for measurement. Note that MAILBOX only differs
    //  from FIFO once the loop is fast enough to fill a swapchain queue; below
    //  that, both acquires return immediately and the mode is not observable.
    //
    //      RV_VULKAN_PRESENT_MODE         (control viewport)
    //      RV_VULKAN_OUTPUT_PRESENT_MODE  (passive presentation output)
    //  with values fifo | relaxed | mailbox | immediate.
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
            return false;
        const string n(name);
        if (n == "fifo")
            mode = VK_PRESENT_MODE_FIFO_KHR;
        else if (n == "relaxed")
            mode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
        else if (n == "mailbox")
            mode = VK_PRESENT_MODE_MAILBOX_KHR;
        else if (n == "immediate")
            mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        else
            return false;
        return true;
    }

    //  FIFO is the only mode required to be supported, so it is always the
    //  last resort of the preference list.
    static VkPresentModeKHR choosePresentMode(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, bool passiveOutput)
    {
        uint32_t count = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &count, nullptr);
        std::vector<VkPresentModeKHR> available(count);
        if (count)
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &count, available.data());

        const auto supported = [&](VkPresentModeKHR m) { return std::find(available.begin(), available.end(), m) != available.end(); };

        VkPresentModeKHR forced = VK_PRESENT_MODE_FIFO_KHR;
        if (presentModeFromName(getenv(passiveOutput ? "RV_VULKAN_OUTPUT_PRESENT_MODE" : "RV_VULKAN_PRESENT_MODE"), forced))
        {
            if (supported(forced))
                return forced;
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

        // Negotiate 10-bit format
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

        //
        //  Unconditional but once per window: a handful of lines, and the pair
        //  that matters is not the format alone but which colour space each
        //  10-bit entry is offered with, plus the order they come in. Vendor
        //  ordering differences in this exact list are what made a black
        //  NVIDIA viewport look like a working AMD one.
        //
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
        // Prefer A2B10G10R10 (== GL_RGB10_A2, the layout the GPU interop shared
        // texture and CPU fallback packing produce natively) so the transfer is
        // a plain copy. If the surface only offers A2R10G10B10 (common on
        // Linux/RADV), accept it too: the opposite R/B order is handled where
        // pixels are packed (CPU fallback) and by a component-wise blit (GPU
        // interop), so red and blue are not swapped.
        //
        // The colour space has to be matched as carefully as the format. A
        // 10-bit format is commonly advertised more than once, paired with a
        // different VkColorSpaceKHR each time, and the enumeration order is
        // vendor-specific: with the display in HDR mode NVIDIA lists
        // A2B10G10R10 + HDR10_ST2084 ahead of A2B10G10R10 + SRGB_NONLINEAR,
        // while AMD lists SRGB_NONLINEAR first. Taking the first format match
        // therefore gave NVIDIA a PQ swapchain fed with sRGB-encoded pixels,
        // which crushes everything below mid-grey to a couple of nits -- the
        // whole viewport, RV's own overlays included, reads as black.
        //
        // RV's renderer emits sRGB, so SRGB_NONLINEAR is the only correct
        // pairing. Honouring an HDR colour space would mean re-encoding the
        // shader output to that transfer function, which is a colour-pipeline
        // change, not a swapchain choice.
        //
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

        // No 10-bit format is paired with SRGB_NONLINEAR on this surface. Take
        // the 10-bit format anyway rather than dropping to the 8-bit OpenGL
        // path: the depth is what the user asked for, and the colour space is
        // reported below so a wrong-looking image is traceable to it.
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
            //  Unconditional, and the colour space is part of it: the format
            //  alone was never enough to explain a black NVIDIA viewport.
            //  Latched, because createSwapchain() re-runs on every resize step.
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

        //  A doc-less window is a passive presentation output; see
        //  choosePresentMode().
        const VkPresentModeKHR presentMode = choosePresentMode(m_vkPhysicalDevice, m_vkSurface, /*passiveOutput*/ m_doc == nullptr);

        uint32_t imageCount = capabilities.minImageCount + 1;
        //  MAILBOX only stays non-blocking with an image to spare: one being
        //  scanned out, one queued as the newest-wins candidate, one to render
        //  into. With fewer, acquire blocks and the mode buys nothing.
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
        // Warm recreate: hand the retiring swapchain to the driver so it can reuse
        // its backing resources (much cheaper than a cold create on every resize).
        createInfo.oldSwapchain = m_vkSwapchain;

        // Create into a local handle so a failed create leaves the existing
        // swapchain and command buffers intact (the fallback paths stay valid).
        VkSwapchainKHR newSwapchain = VK_NULL_HANDLE;
        if (vkCreateSwapchainKHR(m_vkDevice, &createInfo, nullptr, &newSwapchain) != VK_SUCCESS)
        {
            requestGLFallback();
            return false;
        }

        // New swapchain is live. Retire the old one only now: wait for its last
        // submitted frame to finish, free its command buffers, then destroy it.
        if (m_vkSwapchain != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(m_vkDevice);
            if (!m_vkCommandBuffers.empty())
            {
                vkFreeCommandBuffers(m_vkDevice, m_vkCommandPool, (uint32_t)m_vkCommandBuffers.size(), m_vkCommandBuffers.data());
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
        allocInfo.commandBufferCount = (uint32_t)m_vkCommandBuffers.size();
        if (vkAllocateCommandBuffers(m_vkDevice, &allocInfo, m_vkCommandBuffers.data()) != VK_SUCCESS)
        {
            cleanupSwapchain();
            requestGLFallback();
            return false;
        }

        // Per-swapchain-image present-wait semaphores + in-flight fence map. The
        // device is idle here (the retire path above waited on it), so any old
        // renderFinished semaphores from a previous swapchain are safe to destroy.
        for (VkSemaphore sem : m_vkRenderFinished)
        {
            if (sem)
                vkDestroySemaphore(m_vkDevice, sem, nullptr);
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
        // Fresh swapchain images: none are in flight yet.
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
                    vkDestroySemaphore(m_vkDevice, sem, nullptr);
            }
            m_vkRenderFinished.clear();
            m_imagesInFlight.clear();

            for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
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

    // Helper to find memory type
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

        // Reverts the shared image to a plain (non-dedicated) allocation, the
        // behaviour before the dedicated-allocation query was added. The
        // dedicated path is what the spec asks for and what a driver reporting
        // requiresDedicatedAllocation needs for an externally-shared image, but
        // it changes a code path that already worked, so keep a way to rule it
        // out on a machine without a rebuild.
        bool dedicatedAllocationDisabled()
        {
            static const bool disabled = getenv("RV_VULKAN_DISABLE_DEDICATED_ALLOCATION") != nullptr;
            return disabled;
        }

        // How many frames the control viewport may keep in flight. Default 1:
        // block on this frame's fence before returning, rather than letting the
        // next frame's start-of-frame wait absorb it two frames later.
        // RV_VULKAN_MAX_FRAMES_IN_FLIGHT=2 restores the deeper pipeline.
        //
        // Depth is latency, not throughput, and separating the two is what
        // fixed the "annotation trails the cursor in presentation mode" bug.
        // Both backends ran presentation mode at a similar frame interval, so
        // throughput was never the difference; what differed was how old the
        // displayed pixels were. With two frames in flight the screen answers
        // input from two frames back, and the OpenGL path is effectively one
        // deep because paintGL() draws and Qt's following swap waits on that
        // same work.
        //
        // Giving up the second frame costs no measurable frame rate here
        // because the loop is GPU-bound either way, and it buys a whole frame:
        // measured on a 4K presentation output, eventToRetire settled to
        // eventToRender + one frame interval, with no queue left behind it.
        unsigned int maxFramesInFlight()
        {
            static const unsigned int depth = []
            {
                const unsigned int kDefault = 1;
                const char* v = getenv("RV_VULKAN_MAX_FRAMES_IN_FLIGHT");
                if (!v)
                    return kDefault;
                const int n = atoi(v);
                if (n < 1 || n > static_cast<int>(VulkanWindow::FRAMES_IN_FLIGHT))
                {
                    cout << "WARNING: VulkanWindow: RV_VULKAN_MAX_FRAMES_IN_FLIGHT must be 1.." << VulkanWindow::FRAMES_IN_FLIGHT
                         << "; using " << kDefault << endl;
                    return kDefault;
                }
                return static_cast<unsigned int>(n);
            }();
            return depth;
        }

        // Tiling for the GL<->Vulkan shared image: OPTIMAL on NVIDIA, LINEAR
        // everywhere else.
        //
        // NVIDIA needs OPTIMAL. Its 550+ drivers return blank pixels to OpenGL
        // for LINEAR shared images >= ~2 MiB (forum thread #349436), and
        // OPTIMAL avoids that broken linear path. It is safe there because the
        // GL and Vulkan sides are the same driver, so the vendor-private
        // optimal layout matches on import. Set
        // RV_VULKAN_DISABLE_NVIDIA_INTEROP_WORKAROUND to revert NVIDIA to
        // LINEAR (reproduces the blank-image bug, for debugging).
        //
        // The vendor decides this, not the platform: NVIDIA ships one driver
        // core behind both GL and Vulkan on Windows as well as Linux, which is
        // why the check below covers both, and the shared image is always well
        // past the threshold because getSharedImageInfo() allocates at screen
        // capacity -- ~8 MiB for a 1080p A2B10G10R10 image, ~33 MiB at 4K.
        //
        // Everywhere else LINEAR is not conservatism, it is the only correct
        // choice: OPTIMAL was measured on AMD (RADV PHOENIX2, Mesa) and renders
        // tile-pattern garbage -- sparse tile-aligned fragments of the frame,
        // the rest dropped. Under Mesa, GL and Vulkan are different drivers
        // (radeonsi vs RADV) and GL_OPTIMAL_TILING_EXT carries no cross-driver
        // layout guarantee, so the importer reads a layout the exporter never
        // wrote. Making OPTIMAL usable off NVIDIA means negotiating the layout
        // explicitly with VK_EXT_image_drm_format_modifier.
        //
        // This costs real time at a 4K presentation output -- the GL side blits
        // a full frame into the linear image and Vulkan blits it back out every
        // present, both without their tiled fast paths -- so it is worth
        // revisiting, but not by flipping this flag.
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
        info.optimalTiling = 0;
        m_sharedCapacityW[slot] = 0;
        m_sharedCapacityH[slot] = 0;
    }

    //
    //  A best-effort present that skipped has to be retried, or the output is
    //  left showing the frame before the one just composited -- and nothing
    //  else will come back for it, because the control viewport only renders
    //  when the session asks. The OpenGL output never needed this: its
    //  m_view->update() is a dirty flag Qt is obliged to honour eventually.
    //
    //  QWindow::requestUpdate() coalesces, so at most one retry is ever
    //  outstanding. render() picks it up in the passive-output branch and
    //  re-presents the frame already sitting in the device's FBO.
    //
    //
    //  Best-effort gate for a passive presentation output.
    //
    //  The OpenGL presentation path gets this for free: its syncBuffers() is a
    //  QOpenGLWidget::update() that Qt coalesces and drops when it falls
    //  behind, so the second display skips frames under load rather than
    //  deepening the GPU queue. The Vulkan path queues its work
    //  unconditionally, and at a 4K output that work is what the control
    //  viewport ends up waiting for in vkWaitForFences.
    //
    //  Gate on whether this device's *own* GPU work has caught up, not on
    //  whether the swapchain is full -- with a slower loop than display the
    //  queue always has room, so a swapchain-full test never fires.
    //
    //  Called before any GL work, so a skipped frame costs nothing -- in
    //  particular no GL semaphore has been signaled yet, so there is nothing to
    //  rebalance.
    //
    bool VulkanWindow::canPresentNow()
    {
        if (!isPassiveOutput())
            return true;

        //  Nothing allocated yet: let the frame through so syncBuffers() can
        //  build the swapchain and shared image.
        if (!m_vkDevice || !m_vkSwapchain)
            return true;

        //
        //  Forward progress. Under sustained GPU pressure the catch-up test
        //  below can be false indefinitely, which would freeze the presentation
        //  display rather than merely thin it out. Once stale, let syncBuffers()
        //  reach the present functions; they convert their normal zero-timeout
        //  polling into one blocking present.
        //
        static const double kMaxStaleSeconds = 0.1;
        if (!m_lastPresentTimer.isRunning() || m_lastPresentTimer.elapsed() > kMaxStaleSeconds)
            return true;

        //  waitAll with a zero timeout: every in-flight frame of this device
        //  must have retired, not just the one two frames back that this slot
        //  happens to own.
        const VkResult r = vkWaitForFences(m_vkDevice, FRAMES_IN_FLIGHT, m_vkFence.data(), VK_TRUE, 0);
        if (r == VK_SUCCESS)
            return true;

        requestBestEffortRetry();
        return false;
    }

    void VulkanWindow::requestBestEffortRetry()
    {
        if (!m_stopProcessingEvents && isExposed())
            requestUpdate();
    }

    void VulkanWindow::drainSharedSemaphores(uint32_t slot)
    {
        // No shared image for this slot yet -> the GL side never signaled/waited
        // its pair, so there is nothing to rebalance.
        if (!m_vkDevice || !m_vkGlReadySemaphore[slot] || !m_vkVkReadySemaphore[slot])
            return;

        // Consume the pending glReady signal from the GL side and re-signal
        // vkReady so the next use of this slot starts balanced (exactly what a
        // normal present's submit would have done for the pair).
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
            requestGLFallback();
    }

    const VulkanWindow::SharedImageInfo* VulkanWindow::getSharedImageInfo(int w, int h)
    {
        if (!m_vkDevice || !m_externalInteropSupported)
            return nullptr;

        // Build/return the shared image for the current in-flight ring slot.
        const uint32_t slot = m_currentFrame;
        SharedImageInfo& info = m_sharedImageInfo[slot];

        // The swapchain always tracks the window size, so recreate it on any size
        // change. This is independent of the grow-only shared image below: a drag
        // still recreates the (warm) swapchain each step, but no longer rebuilds
        // or re-exports the shared image.
        //
        // The test is against the *surface's* extent, not against the caller's
        // requested size. createSwapchain() takes its extent from
        // capabilities.currentExtent, so it cannot be driven to match a request
        // that disagrees with the surface: comparing to the request instead
        // meant that any caller whose size was off by even a pixel -- e.g. a
        // presentation output that sampled a devicePixelRatio belonging to the
        // screen it was created on rather than the one it was moved to --
        // recreated the swapchain on *every frame*, forever and silently, since
        // both surface-format reports are latched.
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
            // Warm recreate via oldSwapchain (createSwapchain retires the old one).
            if (!createSwapchain())
                return nullptr;
        }

        // Grow-only: if the request fits the slot's current allocated capacity,
        // reuse the existing image/export and just update the used sub-region
        // (presentSharedImage copies/blits info.width x info.height from it).
        if (m_vkSharedImage[slot] && w <= m_sharedCapacityW[slot] && h <= m_sharedCapacityH[slot])
        {
            info.width = w;
            info.height = h;
            return &info;
        }

        // Grow (or first allocation): rebuild at a capacity that is the
        // componentwise max of the request, the screen size, and the current
        // capacity, so it grows monotonically and the common drag-to-fullscreen
        // case allocates at most once.
        //
        //  This window's own screen, not the primary one: a presentation output
        //  lives on a second display, and sizing its headroom from the primary
        //  screen is both wrong and, when the primary is the smaller of the two,
        //  useless as headroom.
        //
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

        // OPTIMAL tiling on NVIDIA (the fix for the blank large-image bug); LINEAR
        // elsewhere. See useOptimalTilingForInterop().
        const bool optimalTiling = useOptimalTilingForInterop(m_vkPhysicalDevice);

        //  Unconditional, and reported after the tiling decision so it can name
        //  it. The shared image is allocated at screen capacity, so this fires
        //  about once per ring slot per session rather than per resize, and the
        //  tiling it reports is exactly the fact that separates a working NVIDIA
        //  viewport from a black one.
        cout << "INFO: VulkanWindow: getSharedImageInfo: (re)allocating shared image slot " << slot << " capacity " << capW << "x" << capH
             << " for request " << w << "x" << h << " tiling=" << (optimalTiling ? "OPTIMAL" : "LINEAR") << endl;

        // 1. Create Shared Image
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
        // The shared image is imported into GL as GL_RGB10_A2, whose bit layout
        // is A2B10G10R10, so the Vulkan side must use the matching format
        // regardless of the swapchain format. When the swapchain is A2R10G10B10
        // the difference is reconciled by a component-wise blit in
        // presentSharedImage() (not a raw copy).
        imageInfo.format = VK_FORMAT_A2B10G10R10_UNORM_PACK32; // matches GL_RGB10_A2
        // Allocate at capacity; presentSharedImage transfers only the used
        // info.width x info.height sub-region (anchored at origin 0,0).
        imageInfo.extent = {(uint32_t)capW, (uint32_t)capH, 1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = optimalTiling ? VK_IMAGE_TILING_OPTIMAL : VK_IMAGE_TILING_LINEAR;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT; // Only used as transfer src in Vulkan
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (vkCreateImage(m_vkDevice, &imageInfo, nullptr, &m_vkSharedImage[slot]) != VK_SUCCESS)
        {
            cerr << "ERROR: VulkanWindow: Failed to create shared image" << endl;
            return nullptr;
        }

        // When the swapchain format differs from the shared image format
        // (A2B10G10R10), presentSharedImage() reconciles them with a blit rather
        // than a raw copy. That requires the shared image to be a valid blit
        // source and the swapchain image a valid blit destination. If the driver
        // does not support that, refuse the GPU-interop path so syncBuffers()
        // uses the (channel-correct) CPU fallback instead.
        if (m_vkSwapchainFormat != VK_FORMAT_A2B10G10R10_UNORM_PACK32)
        {
            VkFormatProperties srcProps = {};
            VkFormatProperties dstProps = {};
            vkGetPhysicalDeviceFormatProperties(m_vkPhysicalDevice, VK_FORMAT_A2B10G10R10_UNORM_PACK32, &srcProps);
            vkGetPhysicalDeviceFormatProperties(m_vkPhysicalDevice, m_vkSwapchainFormat, &dstProps);
            // The shared image's blit-source support depends on its actual tiling
            // (OPTIMAL on NVIDIA, LINEAR elsewhere).
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

        // rowPitch is only meaningful (and vkGetImageSubresourceLayout only valid)
        // for LINEAR tiling. For OPTIMAL tiling the GL import uses the logical
        // capacity width and lets the driver resolve the layout.
        if (optimalTiling)
        {
            info.strideWidth = capW;
        }
        else
        {
            // Handle padded linear row pitch by matching the GL texture stride to Vulkan's rowPitch.
            VkImageSubresource subresource = {};
            subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            subresource.mipLevel = 0;
            subresource.arrayLayer = 0;
            VkSubresourceLayout layout;
            vkGetImageSubresourceLayout(m_vkDevice, m_vkSharedImage[slot], &subresource, &layout);

            if (layout.rowPitch % 4 != 0)
            {
                // Cannot represent this stride as an integer pixel-width texture; fall back to CPU bridge.
                cleanupSharedImage(slot);
                return nullptr;
            }
            info.strideWidth = static_cast<int>(layout.rowPitch / 4);
        }
        info.capacityHeight = capH; // GL imports the texture at capacity dimensions
        info.optimalTiling = optimalTiling ? 1 : 0;

        //
        //  Ask through the 2-variant so the dedicated-allocation requirement can
        //  be read. An image created with an external handle type is reported
        //  requiresDedicatedAllocation by some drivers (AMD's Windows driver
        //  does), and binding non-dedicated memory to such an image is invalid
        //  -- the GL import of it then yields a texture with undefined (in
        //  practice all-zero) contents and no error on any path. Honor whatever
        //  this driver asks for rather than forcing dedicated everywhere.
        //
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
        const bool useDedicated =
            !dedicatedAllocationDisabled() && (dedicatedReqs.requiresDedicatedAllocation || dedicatedReqs.prefersDedicatedAllocation);

        info.dedicated = useDedicated ? 1 : 0;

        cout << "INFO: VulkanWindow: getSharedImageInfo: shared image slot " << slot
             << " memory = " << (useDedicated ? "dedicated" : "non-dedicated")
             << " (driver requires=" << (dedicatedReqs.requiresDedicatedAllocation ? "yes" : "no")
             << " prefers=" << (dedicatedReqs.prefersDedicatedAllocation ? "yes" : "no") << ")" << endl;

        VkExportMemoryAllocateInfo exportAllocInfo = {};
        exportAllocInfo.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
#ifdef PLATFORM_WINDOWS
        exportAllocInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
        exportAllocInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif

        //  Chained ahead of the export info when in use; both live to the
        //  vkAllocateMemory call below.
        VkMemoryDedicatedAllocateInfo dedicatedAllocInfo = {};
        dedicatedAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
        dedicatedAllocInfo.image = m_vkSharedImage[slot];
        if (useDedicated)
        {
            dedicatedAllocInfo.pNext = &exportAllocInfo;
        }

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.pNext = useDedicated ? static_cast<void*>(&dedicatedAllocInfo) : static_cast<void*>(&exportAllocInfo);
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

        // Export device memory as a platform-specific external handle
        // (opaque FD on Linux, Win32 HANDLE on Windows). The receiving GL
        // side imports this with the matching GL_EXT_memory_object_{fd,win32}
        // extension so writes from GL land in this Vulkan image.
#ifdef PLATFORM_WINDOWS
        auto pfnGetMemoryWin32HandleKHR = (PFN_vkGetMemoryWin32HandleKHR)vkGetDeviceProcAddr(m_vkDevice, "vkGetMemoryWin32HandleKHR");
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
        auto pfnGetMemoryFdKHR = (PFN_vkGetMemoryFdKHR)vkGetDeviceProcAddr(m_vkDevice, "vkGetMemoryFdKHR");
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

        // 2. Create Shared Semaphores
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

        // Export the GL<->Vulkan sync semaphores as external handles.
#ifdef PLATFORM_WINDOWS
        auto pfnGetSemaphoreWin32HandleKHR =
            (PFN_vkGetSemaphoreWin32HandleKHR)vkGetDeviceProcAddr(m_vkDevice, "vkGetSemaphoreWin32HandleKHR");
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
        auto pfnGetSemaphoreFdKHR = (PFN_vkGetSemaphoreFdKHR)vkGetDeviceProcAddr(m_vkDevice, "vkGetSemaphoreFdKHR");
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

        // Transition the shared image to TRANSFER_SRC optimal initially
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

        // Signal vkReady initially so GL can start writing to it
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

        // Commit the new capacity only now that the (re)build fully succeeded.
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
            return;

        //  Split the present cost into fence-wait vs acquire, the only two
        //  blocking calls here, so GPU back-pressure can be told apart from
        //  swapchain/vblank back-pressure. Main viewport only.
        const bool diagPresent = IPCore::ImageRenderer::debugGpu() && m_doc;
        Timer diagTimer;

        //
        //  Best-effort present for a passive presentation output.
        //
        //  This function has exactly two blocking calls, the fence wait and the
        //  acquire, and on the control viewport they are the throttle: FIFO
        //  acquire back-pressure plus the start-of-frame fence is what paces
        //  the loop to display refresh.
        //
        //  A presentation output should not be part of that. render() presents
        //  the control viewport and then the output inside one frame, so a
        //  second blocking pair puts a second display's vblank in the loop's
        //  path. The OpenGL output is never in it: DesktopVideoDevice's
        //  syncBuffers() is a coalesced QOpenGLWidget::update() on an empty
        //  paintGL() that Qt drops when it falls behind.
        //
        //  So do the same explicitly -- poll with a zero timeout and skip the
        //  frame when the swapchain cannot take an image right now, leaving the
        //  output on its previous frame as a dropped Qt update would. See
        //  canPresentNow() for the gate that fires first, before any GL work.
        //
        const bool bestEffort = isPassiveOutput();
        static const double kMaxStaleSeconds = 0.1;
        const bool forceProgress = bestEffort && (!m_lastPresentTimer.isRunning() || m_lastPresentTimer.elapsed() > kMaxStaleSeconds);
        const uint64_t waitTimeout = (!bestEffort || forceProgress) ? UINT64_MAX : 0;

        if (diagPresent)
            diagTimer.start();

        VkResult fenceResult = vkWaitForFences(m_vkDevice, 1, &m_vkFence[slot], VK_TRUE, waitTimeout);

        if (diagPresent)
            s_diagFenceWaitMs += diagTimer.elapsed() * 1000.0;

        if (fenceResult == VK_TIMEOUT)
        {
            //  The GL side already signaled glReady[slot] and waited vkReady[slot]
            //  for this frame, so the pair has to be rebalanced before bailing --
            //  same contract as the VK_ERROR_OUT_OF_DATE_KHR path below. The slot
            //  is deliberately not advanced: the next frame retries this one.
            drainSharedSemaphores(slot);
            requestBestEffortRetry();
            return;
        }
        if (fenceResult != VK_SUCCESS)
        {
            requestGLFallback();
            return;
        }

        // Acquire image
        uint32_t imageIndex;
        if (diagPresent)
            diagTimer.start();

        VkResult result =
            vkAcquireNextImageKHR(m_vkDevice, m_vkSwapchain, waitTimeout, m_vkImageAvailableSemaphore[slot], VK_NULL_HANDLE, &imageIndex);

        if (diagPresent)
            s_diagAcquireMs += diagTimer.elapsed() * 1000.0;

        if (result == VK_NOT_READY || result == VK_TIMEOUT)
        {
            //  No image free this frame. An acquire that fails this way leaves
            //  m_vkImageAvailableSemaphore[slot] unsignaled, so nothing leaks --
            //  which is why the skip has to happen here and not after a
            //  successful acquire.
            drainSharedSemaphores(slot);
            requestBestEffortRetry();
            return;
        }

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            // The GL side already signaled glReady[slot]/waited vkReady[slot] this
            // frame; rebalance the pair before bailing so the next frame on this
            // slot can't desync.
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

        // If this swapchain image is still owned by another in-flight frame, wait
        // for that frame's fence before rendering into it, then mark the image as
        // now owned by this frame.
        if (m_imagesInFlight[imageIndex] != VK_NULL_HANDLE)
        {
            vkWaitForFences(m_vkDevice, 1, &m_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
        }
        m_imagesInFlight[imageIndex] = m_vkFence[slot];

        // Reset the frame fence only now, right before the submit that re-signals it.
        vkResetFences(m_vkDevice, 1, &m_vkFence[slot]);

        VkCommandBuffer cb = m_vkCommandBuffers[imageIndex];
        vkResetCommandBuffer(cb, 0);

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cb, &beginInfo);

        // Transition shared image from COLOR_ATTACHMENT_OPTIMAL to TRANSFER_SRC_OPTIMAL
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

        // Transfer the shared image (always A2B10G10R10, == GL_RGB10_A2) to the
        // swapchain image. When the swapchain is also A2B10G10R10 the layouts
        // match and a raw copy is correct and cheapest. When the swapchain is
        // A2R10G10B10 a raw copy would swap red and blue, so use a blit instead:
        // vkCmdBlitImage converts per component (R->R, G->G, B->B) between the
        // two formats. Whether the (linear-tiled) shared image can be a blit
        // source is checked at shared-image creation; if not, that path is
        // refused and syncBuffers() uses the CPU fallback instead.
        //
        //  The destination is bounded by the swapchain, never by the shared
        //  image. They are normally the same size, but the shared image is
        //  sized from the caller's request and a stale devicePixelRatio can
        //  inflate that (a 3840x2160 output asking for 5760x3240), which would
        //  otherwise write outside the swapchain image -- invalid usage, so
        //  undefined contents or a faulted submit rather than a visible error.
        //
        if (m_vkSwapchainFormat == VK_FORMAT_A2B10G10R10_UNORM_PACK32)
        {
            //  vkCmdCopyImage cannot scale, so clamp to the overlapping region.
            VkImageCopy region = {};
            region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.srcSubresource.layerCount = 1;
            region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.dstSubresource.layerCount = 1;
            region.extent = {std::min((uint32_t)info.width, m_vkSwapchainExtent.width),
                             std::min((uint32_t)info.height, m_vkSwapchainExtent.height), 1};

            vkCmdCopyImage(cb, m_vkSharedImage[slot], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_vkSwapchainImages[imageIndex],
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        }
        else
        {
            //  vkCmdBlitImage can scale, so fill the swapchain from the used
            //  sub-region of the shared image instead of truncating.
            VkImageBlit blit = {};
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.layerCount = 1;
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.layerCount = 1;
            blit.srcOffsets[0] = {0, 0, 0};
            blit.srcOffsets[1] = {info.width, info.height, 1};
            blit.dstOffsets[0] = {0, 0, 0};
            blit.dstOffsets[1] = {(int32_t)m_vkSwapchainExtent.width, (int32_t)m_vkSwapchainExtent.height, 1};

            vkCmdBlitImage(cb, m_vkSharedImage[slot], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_vkSwapchainImages[imageIndex],
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_NEAREST);
        }

        // Transition swapchain image to present
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = 0;

        vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1,
                             &barrier);

        vkEndCommandBuffer(cb);

        // Submit
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        // Wait for GL to finish writing (glReady) AND swapchain image to be available
        VkSemaphore waitSemaphores[] = {m_vkGlReadySemaphore[slot], m_vkImageAvailableSemaphore[slot]};
        // Both waits protect transfer operations. The acquired swapchain image
        // is first touched by its TRANSFER_DST layout transition, so waiting at
        // COLOR_ATTACHMENT_OUTPUT would not block that earlier stage.
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT};
        submitInfo.waitSemaphoreCount = 2;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cb;

        // Signal the image's renderFinished (present waits on it) AND vkReady (so
        // GL can write the next frame into this slot's shared image).
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

        //  Hand this frame's pointer-event timestamp to the slot so
        //  eventToRetire can be closed out when the fence signals.
        if (m_doc && s_diagFrameEventTime >= 0.0)
        {
            s_diagSlotEventTime[slot] = s_diagFrameEventTime;
            s_diagSlotArmed[slot] = true;
            s_diagFrameEventTime = -1.0;
        }

        // The frame is committed to the GPU; advance the ring now so the next
        // frame uses the other slot. imageIndex/slot below are locals, so this is
        // safe before the present call.
        m_currentFrame = (m_currentFrame + 1) % FRAMES_IN_FLIGHT;

        // Present, waiting on the image's own renderFinished semaphore.
        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &m_vkRenderFinished[imageIndex];
        VkSwapchainKHR swapchains[] = {m_vkSwapchain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = &imageIndex;

        VkResult presentResult = vkQueuePresentKHR(m_vkQueue, &presentInfo);

        //  This device has presented; the forward-progress guard in
        //  canPresentNow() measures staleness from here.
        m_lastPresentTimer.stop();
        m_lastPresentTimer.start();

        //  Depth-1 pipeline, opt-in: see maxFramesInFlight(). Done after the
        //  present is queued so the driver still gets the frame as early as
        //  possible; this only stops the CPU running a second frame ahead. The
        //  passive output is excluded -- it is best-effort by design and must
        //  never block the loop.
        if (maxFramesInFlight() == 1 && !isPassiveOutput())
        {
            vkWaitForFences(m_vkDevice, 1, &m_vkFence[slot], VK_TRUE, UINT64_MAX);
        }
        // Recreate only on OUT_OF_DATE. VK_SUBOPTIMAL_KHR still presents fine and
        // can be reported persistently by some X11/RADV compositors; recreating
        // on it every frame caused a swapchain-recreate loop that starved the Qt
        // event loop (dead input, no fullscreen). Real resizes report OUT_OF_DATE.
        // The submit above is tracked by m_vkFence[slot] (waited at the start of
        // the next use of this slot), so no end-of-frame block is needed here.
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
            return;

        const bool diagPresent = IPCore::ImageRenderer::debugGpu() && m_doc;
        Timer diagTimer;

        if (!m_vkSwapchain || m_vkSwapchainExtent.width != (uint32_t)w || m_vkSwapchainExtent.height != (uint32_t)h)
        {
            // Warm recreate via oldSwapchain (createSwapchain retires the old one).
            if (!createSwapchain())
                return;
        }

        // Start-of-frame throttle (matches presentSharedImage): wait for this
        // slot's previous frame to finish before reusing its staging buffer,
        // acquire semaphore and command resources. A passive presentation output
        // polls instead of blocking and skips the frame -- see the best-effort
        // present in presentSharedImage() for why.
        const bool bestEffort = isPassiveOutput();
        static const double kMaxStaleSeconds = 0.1;
        const bool forceProgress = bestEffort && (!m_lastPresentTimer.isRunning() || m_lastPresentTimer.elapsed() > kMaxStaleSeconds);
        const uint64_t waitTimeout = (!bestEffort || forceProgress) ? UINT64_MAX : 0;

        if (diagPresent)
            diagTimer.start();
        const VkResult fenceResult = vkWaitForFences(m_vkDevice, 1, &m_vkFence[slot], VK_TRUE, waitTimeout);
        if (diagPresent)
            s_diagFenceWaitMs += diagTimer.elapsed() * 1000.0;

        if (fenceResult == VK_TIMEOUT)
        {
            //  Unlike the interop path there are no GL<->Vulkan semaphores to
            //  rebalance here: presentCpuFallback() hands over plain pixels.
            requestBestEffortRetry();
            return;
        }
        if (fenceResult != VK_SUCCESS)
        {
            requestGLFallback();
            return;
        }

        size_t size = w * h * 4;

        // Recreate this slot's staging buffer if needed
        if (size > m_stagingBufferSize[slot])
        {
            if (m_vkStagingBuffer[slot])
                vkDestroyBuffer(m_vkDevice, m_vkStagingBuffer[slot], nullptr);
            if (m_vkStagingBufferMemory[slot])
                vkFreeMemory(m_vkDevice, m_vkStagingBufferMemory[slot], nullptr);

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

        // Copy to staging buffer
        void* data;
        vkMapMemory(m_vkDevice, m_vkStagingBufferMemory[slot], 0, size, 0, &data);
        memcpy(data, pixels, size);
        vkUnmapMemory(m_vkDevice, m_vkStagingBufferMemory[slot]);

        // Acquire image
        uint32_t imageIndex;
        if (diagPresent)
            diagTimer.start();
        VkResult result =
            vkAcquireNextImageKHR(m_vkDevice, m_vkSwapchain, waitTimeout, m_vkImageAvailableSemaphore[slot], VK_NULL_HANDLE, &imageIndex);
        if (diagPresent)
            s_diagAcquireMs += diagTimer.elapsed() * 1000.0;
        if (result == VK_NOT_READY || result == VK_TIMEOUT)
        {
            // Best-effort: no image free this frame, leave the output on the one
            // it is already showing and come back for it.
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

        // If this swapchain image is still owned by another in-flight frame, wait
        // for its fence, then mark it owned by this frame.
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

        // Transition image to transfer dst
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

        // Copy buffer to image
        VkBufferImageCopy region = {};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {(uint32_t)w, (uint32_t)h, 1};

        vkCmdCopyBufferToImage(cb, m_vkStagingBuffer[slot], m_vkSwapchainImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                               &region);

        // Transition image to present
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = 0;

        vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1,
                             &barrier);

        vkEndCommandBuffer(cb);

        // Submit
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        VkSemaphore waitSemaphores[] = {m_vkImageAvailableSemaphore[slot]};
        // The acquired image is first used by a TRANSFER_DST layout transition
        // and vkCmdCopyBufferToImage, not as a color attachment.
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

        // Frame committed; advance the ring (imageIndex/slot below are locals).
        m_currentFrame = (m_currentFrame + 1) % FRAMES_IN_FLIGHT;

        // Present, waiting on the image's own renderFinished semaphore.
        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        VkSwapchainKHR swapchains[] = {m_vkSwapchain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = &imageIndex;

        VkResult presentResult = vkQueuePresentKHR(m_vkQueue, &presentInfo);

        //  This device has presented; the forward-progress guard in
        //  canPresentNow() measures staleness from here.
        m_lastPresentTimer.stop();
        m_lastPresentTimer.start();

        //  Depth-1 pipeline, opt-in: see maxFramesInFlight(). Done after the
        //  present is queued so the driver still gets the frame as early as
        //  possible; this only stops the CPU running a second frame ahead. The
        //  passive output is excluded -- it is best-effort by design and must
        //  never block the loop.
        if (maxFramesInFlight() == 1 && !isPassiveOutput())
        {
            vkWaitForFences(m_vkDevice, 1, &m_vkFence[slot], VK_TRUE, UINT64_MAX);
        }
        // Recreate only on OUT_OF_DATE. VK_SUBOPTIMAL_KHR still presents fine and
        // can be reported persistently by some X11/RADV compositors; recreating
        // on it every frame caused a swapchain-recreate loop that starved the Qt
        // event loop (dead input, no fullscreen). Real resizes report OUT_OF_DATE.
        // The submit is tracked by m_vkFence[slot] (waited at the next use of this
        // slot), so no end-of-frame block is needed here.
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

        //
        //  Vulkan is not ready until the surface and swapchain exist, which
        //  happens in initialize() on first expose. resizeEvent() also calls
        //  requestUpdate(), so an UpdateRequest can land here before then --
        //  newly reachable now that swapGLViewToVulkan() builds a VulkanView
        //  mid-session. Presenting into a null device would fault; another
        //  render is requested once initialized.
        //
        if (!m_initialized)
        {
            return;
        }

        //
        //  A passive presentation output never drives the frame loop below: it
        //  is composited into and presented by its owning
        //  VulkanDesktopVideoDevice, in-frame, from the control viewport's
        //  render(). The only reason it gets an UpdateRequest of its own is a
        //  best-effort present that was skipped (see requestBestEffortRetry),
        //  so re-present what the device already composited -- the same handoff
        //  exposeEvent() uses.
        //
        if (isPassiveOutput())
        {
            if (m_videoDevice)
                m_videoDevice->syncBuffers();
            return;
        }

        IPCore::Session* session = m_doc ? m_doc->session() : nullptr;
        if (!session)
            return;

        //  See s_diagLoopTimer.
        if (IPCore::ImageRenderer::debugGpu())
        {
            if (s_diagLoopTimer.isRunning())
                s_diagLoopMs += s_diagLoopTimer.elapsed() * 1000.0;
            s_diagLoopTimer.start();

            if (s_diagPointerPending)
            {
                s_diagPointerAgeMs += s_diagPointerTimer.elapsed() * 1000.0;
                ++s_diagPointerAgeSamples;
                s_diagPointerPending = false;
                //  This frame answers that event; presentSharedImage() pins the
                //  timestamp to the slot it submits into.
                s_diagFrameEventTime = diagNow() - s_diagPointerTimer.elapsed();
            }

            //  Close out any slot whose GPU work has retired since last frame.
            //  vkGetFenceStatus does not block, so this costs two calls a frame
            //  and works at any pipeline depth, with one frame of quantisation.
            for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
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

            //  Frame-time breakdown; see the report at the end of render().
            const bool diagTiming = IPCore::ImageRenderer::debugGpu();
            Timer diagTimer;
            if (diagTiming)
                diagTimer.start();

            session->render();

            if (diagTiming)
                s_diagRenderMs += diagTimer.elapsed() * 1000.0;

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
            //
            //  Always present the main (control) viewport's own swapchain.
            //  Unlike the GL path, where QOpenGLWidget composites the control
            //  widget after paintGL regardless, the Vulkan viewport only
            //  appears via an explicit present -- so it must NOT be skipped
            //  when a separate output (presentation) device is active.
            //  Skipping it leaves the main window on a stale frame once
            //  presentation mode is on.
            //
            const bool diagPresent = IPCore::ImageRenderer::debugGpu();
            Timer diagPresentTimer;
            if (diagPresent)
                diagPresentTimer.start();

            m_videoDevice->syncBuffers();

            if (diagPresent)
                s_diagMainPresentMs += diagPresentTimer.elapsed() * 1000.0;

            //
            //  In presentation mode the output is a distinct fullscreen window
            //  that must also be presented this frame.
            //
            if (session->outputVideoDevice() && session->outputVideoDevice() != videoDevice())
            {
                if (diagPresent)
                    diagPresentTimer.start();

                session->outputVideoDevice()->syncBuffers();

                if (diagPresent)
                    s_diagOutPresentMs += diagPresentTimer.elapsed() * 1000.0;

                //
                //  Presenting the output device made *its* offscreen GL context
                //  current and did not put ours back, so restore it before
                //  postRender() and anything else that runs after this frame
                //  expects the viewport's context. The GL output path never
                //  needed this: its syncBuffers() is a QOpenGLWidget update(),
                //  which schedules a composite without touching the current
                //  context.
                //
                m_videoDevice->makeCurrent();
            }
        }

        if (session)
        {
            //  See s_diagPostRenderMs.
            const bool diagPost = IPCore::ImageRenderer::debugGpu();
            Timer diagPostTimer;
            if (diagPost)
                diagPostTimer.start();

            session->addSyncSample();
            session->postRender();

            if (diagPost)
                s_diagPostRenderMs += diagPostTimer.elapsed() * 1000.0;
        }

        //
        //  Report an averaged breakdown every 60 frames: where the frame goes
        //  (session render vs viewport present vs output present), and what the
        //  pointer sees end to end.
        //
        if (IPCore::ImageRenderer::debugGpu() && m_doc)
        {
            if (++s_diagFrames >= 60)
            {
                const double n = double(s_diagFrames);
                const double loopMs = s_diagLoopMs / n;
                cout << "INFO: VulkanWindow frame avg over " << s_diagFrames << " [depth=" << maxFramesInFlight()
                     << " tiling=" << (m_sharedImageInfo[0].optimalTiling ? "OPTIMAL" : "LINEAR") << "]"
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

        //
        //  The VkSurfaceKHR is derived from the platform window, so it does not
        //  survive Qt destroying and recreating it -- which happens when the
        //  container re-parents the viewport after the top-level QWidgetWindow
        //  is replaced (see VulkanView::reattachVulkanWindow). Rebuild the
        //  surface and swapchain when we come back exposed on a new handle.
        //
        if (m_initialized && handle() != m_initializedHandle)
        {
            handleSurfaceLost();
        }

        if (!m_initialized)
        {
            initialize();
        }

        //
        //  A doc-less window is a passive presentation output: it is rendered
        //  into and presented by its owning VulkanDesktopVideoDevice, and
        //  render() returns at `!session` so it never drives itself. Present
        //  once here so a freshly exposed (or re-exposed) presentation surface
        //  shows the last composited frame instead of staying blank until the
        //  next main-view frame.
        //
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
            m_doc->viewSizeChanged(event->size().width(), event->size().height());
        QWindow::resizeEvent(event);

        // Nothing repaints this native surface on resize, so drive a render now
        // to recreate the swapchain at the new size and present immediately
        // (instead of waiting for a mouse Enter event). QWindow::requestUpdate()
        // coalesces, so a fast drag does not queue one heavy recreate per event.
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
            m_doc->session()->userGenericEvent("per-render-event-processing", "");
    }

    //--------------------------------------------------------------------------
    // event()
    //--------------------------------------------------------------------------

    bool VulkanWindow::event(QEvent* event)
    {
        //
        //  This must be handled before every guard below (including the
        //  m_stopProcessingEvents / missing-device early-outs): it is the only
        //  point at which the Vulkan objects can still legally be destroyed.
        //
        //  Qt sends SurfaceAboutToBeDestroyed from QWindow::destroy(), just
        //  before it deletes the QPlatformWindow -- and the VkSurfaceKHR, the
        //  swapchain and the X11 drawable behind them all die with it. Anything
        //  released later is released against a surface that no longer exists,
        //  which is a segfault inside the driver rather than an error code.
        //
        //  On quit that "later" is the destructor: QWindowContainer's own
        //  destructor calls window->destroy() and only then deletes the window,
        //  so ~VulkanWindow always runs on a dead surface. The same applies on
        //  the reparent path, where Qt replaces the top-level QWidgetWindow
        //  (adding a QWebEngineView is the usual trigger). exposeEvent()'s
        //  handle() != m_initializedHandle check notices that one, but only
        //  after the fact; this notices it in time.
        //
        if (event->type() == QEvent::PlatformSurface)
        {
            if (static_cast<QPlatformSurfaceEvent*>(event)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed)
            {
                releaseVulkanResources();
            }
            return QWindow::event(event);
        }

        // The device (and its translator) is wired by the hosting VulkanView
        // just after construction; ignore any events that arrive before then.
        if (!m_videoDevice)
            return QWindow::event(event);

        bool keyevent = false;
        Rv::Session* session = m_doc ? m_doc->session() : nullptr;

        if (m_stopProcessingEvents)
        {
            event->accept();
            return true;
        }

        if (event->type() == QEvent::WindowActivate)
            m_activationTimer.start();

        float activationTime = 0.0f;
        if (m_activationTimer.isRunning())
        {
            if (event->type() == QEvent::MouseButtonPress)
            {
                activationTime = m_activationTimer.elapsed();
                m_activationTimer.stop();
            }
            if (event->type() == QEvent::MouseMove)
                m_activationTimer.stop();
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
            //
            //  Qt has already made this the focus window by the time FocusIn is
            //  delivered, so there is nothing to hand over here. The case exists
            //  only to drop modifier state that went stale while the keyboard
            //  was elsewhere.
            //
            //  Guarded: a passive presentation output window is built with no
            //  event widget, so its device has no translator (the general
            //  hasTranslator() check below this switch is too late).
            //
            if (m_videoDevice->hasTranslator())
            {
                m_videoDevice->translator().resetModifiers();
            }
            break;

        case QEvent::Enter:
            //
            //  Hovering hands the keyboard to the viewport as a *widget* focus
            //  change, never as a window activation. QWidget::setFocus() only
            //  delivers FocusIn when the top-level is already active; otherwise
            //  it just records the window's focus_child and the keyboard
            //  arrives once the user activates RV. That keeps a hover from
            //  stealing activation from another top-level of ours (the Console)
            //  or from another application entirely.
            //
            //  Skipped when this window already holds focus: QWindowContainer
            //  clears the container's widget focus once it has handed focus
            //  over, so a repeat FocusIn would take its "return to the normal
            //  focus chain" branch and push the keyboard to the next widget in
            //  the tab chain instead. This is why FocusIn above must not fall
            //  through into this case.
            //
            if (QGuiApplication::focusWindow() != this && m_eventWidget)
                m_eventWidget->setFocus(Qt::MouseFocusReason);
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
                    session->userGenericEvent("view-resized", contents.str());
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
            session->setEventVideoDevice(videoDevice());

        //  See s_diagPointerHandlerMs. A drag arrives here and is dispatched
        //  synchronously into Mu, so this call *is* the handler's cost.
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
