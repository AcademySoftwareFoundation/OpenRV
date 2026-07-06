//
//  Copyright (c) 2026 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//

#if defined(PLATFORM_LINUX) || defined(PLATFORM_WINDOWS)

#include <RvCommon/VulkanView.h>
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
#include <QtGui/QResizeEvent>
#include <QtGui/QShowEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QPaintEvent>
#include <QtGui/QWindow>
#include <QtGui/QVulkanInstance>
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>
#include <QtWidgets/QMenu>
#include <QtWidgets/QWidget>

#include <algorithm>
#include <climits>
#include <cstdlib>
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
    using namespace TwkApp;
    using namespace IPCore;

    // Both A2B10G10R10 and A2R10G10B10 are 10-bit-per-channel packed formats;
    // they differ only in R/B component order. Both are acceptable for 10-bit
    // presentation -- the R/B order is handled where pixels are packed (CPU
    // fallback) or blitted (GPU interop). A2B10G10R10 (== GL_RGB10_A2) is
    // preferred when the surface offers it, but many Linux/RADV surfaces only
    // advertise A2R10G10B10.
    static bool isTenBitFormat(VkFormat f) { return f == VK_FORMAT_A2B10G10R10_UNORM_PACK32 || f == VK_FORMAT_A2R10G10B10_UNORM_PACK32; }

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
    // VulkanView implementation
    //--------------------------------------------------------------------------

    VulkanView::VulkanView(RvDocument* doc, QWidget* parent, bool noResize)
        : QWidget(parent)
        , m_doc(doc)
        , m_videoDevice(nullptr)
        , m_initialized(false)
        , m_firstPaintCompleted(false)
        , m_postFirstNonEmptyRender(noResize)
        , m_stopProcessingEvents(false)
        , m_userActive(true)
        , m_csize(1024, 576)
        , m_msize(128, 128)
        , m_eventWidget(nullptr)
        , m_lastKey(0)
        , m_lastKeyType(QEvent::None)
    {
        // Force the creation of a native window early.
        setAttribute(Qt::WA_NativeWindow);
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_OpaquePaintEvent);
        setAttribute(Qt::WA_PaintOnScreen);
        setAttribute(Qt::WA_TranslucentBackground);
        setAutoFillBackground(false);

        // Wait to configure the QWindow until it's created
        if (QWindow* w = windowHandle())
        {
            w->setSurfaceType(QSurface::VulkanSurface);

            // Set 10-bit format
            QSurfaceFormat fmt;
            fmt.setRedBufferSize(10);
            fmt.setGreenBufferSize(10);
            fmt.setBlueBufferSize(10);
            fmt.setAlphaBufferSize(2);
            w->setFormat(fmt);
        }

        ostringstream str;
        str << UI_APPLICATION_NAME " Main Window (Vulkan)" << "/" << m_doc;
        m_videoDevice = new QTVulkanVideoDevice(nullptr, str.str(), this, nullptr);

        m_activityTimer.start();

        m_eventProcessingTimer.setSingleShot(true);
        connect(&m_eventProcessingTimer, SIGNAL(timeout()), this, SLOT(eventProcessingTimeout()));
    }

    VulkanView::~VulkanView()
    {
        delete m_videoDevice;
        m_videoDevice = nullptr;
        for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
            cleanupSharedImage(i);
        cleanupSwapchain();
        cleanupVulkan();
    }

    //--------------------------------------------------------------------------

    void VulkanView::setEventWidget(QWidget* widget)
    {
        m_eventWidget = widget;
        if (m_videoDevice)
        {
            m_videoDevice->setEventWidget(widget);
        }
    }

    void VulkanView::stopProcessingEvents() { m_stopProcessingEvents = true; }

    void VulkanView::absolutePosition(int& x, int& y) const
    {
        QPoint gp = mapToGlobal(QPoint(0, 0));
        x = gp.x();
        y = gp.y();
    }

    float VulkanView::devicePixelRatio() const { return static_cast<float>(devicePixelRatioF()); }

    //--------------------------------------------------------------------------
    // Vulkan Initialisation
    //--------------------------------------------------------------------------

    void VulkanView::initialize()
    {
        if (m_initialized)
        {
            return;
        }

        if (QWindow* w = windowHandle())
        {
            w->setSurfaceType(QSurface::VulkanSurface);
            QSurfaceFormat fmt;
            fmt.setRedBufferSize(10);
            fmt.setGreenBufferSize(10);
            fmt.setBlueBufferSize(10);
            fmt.setAlphaBufferSize(2);
            w->setFormat(fmt);
        }

        if (!initVulkan())
        {
            cerr << "ERROR: VulkanView: initVulkan failed; falling back to OpenGL" << endl;
            requestGLFallback();
            return;
        }

        m_initialized = true;

        if (m_doc)
        {
            m_doc->initializeSession();
        }
    }

    bool VulkanView::supports10BitPresentation()
    {
        QVulkanInstance qtVkInst;
        if (!qtVkInst.create())
        {
            cerr << "ERROR: VulkanView: supports10BitPresentation: QVulkanInstance create failed" << endl;
            return false;
        }

        VkInstance instance = qtVkInst.vkInstance();
        if (instance == VK_NULL_HANDLE)
        {
            return false;
        }

        QWindow dummyWindow;
        dummyWindow.setSurfaceType(QSurface::VulkanSurface);
        dummyWindow.create();
        dummyWindow.setVulkanInstance(&qtVkInst);

        VkSurfaceKHR dummySurface = qtVkInst.surfaceForWindow(&dummyWindow);
        if (!dummySurface)
        {
            cerr << "ERROR: VulkanView: supports10BitPresentation: failed to create dummy surface" << endl;
            return false;
        }

        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount == 0)
        {
            cerr << "ERROR: VulkanView: supports10BitPresentation: vkEnumeratePhysicalDevices returned 0 devices" << endl;
            return false;
        }
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        if (ImageRenderer::debugGpu())
        {
            cout << "INFO: VulkanView: supports10BitPresentation: probing " << deviceCount << " physical device(s)" << endl;
        }

        bool any10bit = false;
        for (uint32_t di = 0; di < devices.size(); ++di)
        {
            VkPhysicalDevice dev = devices[di];

            uint32_t formatCount = 0;
            if (vkGetPhysicalDeviceSurfaceFormatsKHR(dev, dummySurface, &formatCount, nullptr) != VK_SUCCESS || formatCount == 0)
            {
                continue;
            }
            std::vector<VkSurfaceFormatKHR> formats(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(dev, dummySurface, &formatCount, formats.data());

            bool has10bit = false;
            for (const auto& fmt : formats)
            {
                if (isTenBitFormat(fmt.format))
                {
                    has10bit = true;
                    any10bit = true;
                    break;
                }
            }

            VkPhysicalDeviceProperties props = {};
            vkGetPhysicalDeviceProperties(dev, &props);
            if (ImageRenderer::debugGpu())
            {
                cout << "INFO: VulkanView:   device[" << di << "] '" << props.deviceName
                     << "': 10-bit surface format=" << (has10bit ? "YES" : "NO") << endl;
            }
        }

        // The surface returned by surfaceForWindow() is owned by the platform
        // integration and is released when dummyWindow is destroyed on return;
        // QVulkanInstance has no destroySurface() in this Qt version.
        if (ImageRenderer::debugGpu())
        {
            cout << "INFO: VulkanView: supports10BitPresentation: returning " << (any10bit ? "true" : "false") << endl;
        }
        return any10bit;
    }

    bool VulkanView::initVulkan()
    {
        // Create Instance
        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "RV VulkanView";
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

        // Try to get Qt's extensions
        static QVulkanInstance* qtVkInst = nullptr;
        if (!qtVkInst)
        {
            qtVkInst = new QVulkanInstance();
            if (!qtVkInst->create())
            {
                cerr << "ERROR: VulkanView: QVulkanInstance create failed" << endl;
                delete qtVkInst;
                qtVkInst = nullptr;
                return false;
            }
        }

        m_vkInstance = qtVkInst->vkInstance();

        // Create Surface
        QWindow* w = windowHandle();
        if (!w)
        {
            return false;
        }

        w->setVulkanInstance(qtVkInst);

        m_vkSurface = qtVkInst->surfaceForWindow(w);
        if (!m_vkSurface)
        {
            cerr << "ERROR: VulkanView: Failed to create Vulkan surface" << endl;
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
            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, nullptr);
            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, queueFamilies.data());

            for (uint32_t i = 0; i < queueFamilyCount; i++)
            {
                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, m_vkSurface, &presentSupport);
                if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && presentSupport)
                {
                    m_vkPhysicalDevice = dev;
                    m_queueFamilyIndex = i;
                    foundQueue = true;
                    break;
                }
            }
            if (foundQueue)
                break;
        }

        if (!foundQueue)
        {
            cerr << "ERROR: VulkanView: initVulkan: No physical device with graphics and present support found." << endl;
            return false;
        }

        {
            VkPhysicalDeviceProperties props = {};
            vkGetPhysicalDeviceProperties(m_vkPhysicalDevice, &props);
            if (ImageRenderer::debugGpu())
            {
                cout << "INFO: VulkanView: initVulkan: picked physical device '" << props.deviceName << "' (of " << deviceCount
                     << " available)" << endl;
            }
        }

        // Create Logical Device
        float queuePriority = 1.0f;
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = m_queueFamilyIndex;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        std::vector<const char*> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#ifdef PLATFORM_WINDOWS
            VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME,
            VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME,
#else
            VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
            VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME,
#endif
        };

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
    void VulkanView::requestGLFallback()
    {
        if (m_glFallbackRequested || !m_doc || m_stopProcessingEvents || m_doc->isClosing())
        {
            return;
        }
        m_glFallbackRequested = true;
        QTimer::singleShot(0, m_doc, [doc = m_doc]() { doc->fallbackVulkanToGLView(); });
    }

    // Skip swapchain work during close or zero-size resize.
    bool VulkanView::presentationAllowed() const
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

    // Reset acquire semaphore and rebuild swapchain/shared image at the new size.
    void VulkanView::handleSwapchainOutOfDate()
    {
        if (!m_vkDevice || !presentationAllowed())
        {
            return;
        }

        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
        {
            if (m_vkImageAvailableSemaphore[i])
            {
                vkDestroySemaphore(m_vkDevice, m_vkImageAvailableSemaphore[i], nullptr);
                m_vkImageAvailableSemaphore[i] = VK_NULL_HANDLE;
            }
            if (vkCreateSemaphore(m_vkDevice, &semaphoreInfo, nullptr, &m_vkImageAvailableSemaphore[i]) != VK_SUCCESS)
            {
                m_vkImageAvailableSemaphore[i] = VK_NULL_HANDLE;
                requestGLFallback();
                return;
            }
        }

        // Recreate only the swapchain, not the shared image. The shared image is
        // a content-sized TRANSFER_SRC image, independent of the window-sized
        // swapchain; createSwapchain() reuses the old swapchain (oldSwapchain) so
        // this is a warm recreate. The next render()'s getSharedImageInfo() will
        // rebuild the shared image only if the content size actually changed.
        if (!createSwapchain())
        {
            requestGLFallback();
        }
    }

    void VulkanView::cleanupVulkan()
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
        // Surface is managed by QVulkanInstance? We shouldn't destroy it here if QVulkanInstance owns it, but wait, we got it from
        // surfaceForWindow. Actually QVulkanWindow destroys it. We can just leave it for QVulkanInstance to clean up, or we can
        // vkDestroySurfaceKHR if needed. For safety we don't destroy instance/surface here, they are tied to Qt.
    }

    bool VulkanView::createSwapchain()
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
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_vkPhysicalDevice, m_vkSurface, &capabilities);

        // Negotiate 10-bit format
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_vkPhysicalDevice, m_vkSurface, &formatCount, nullptr);
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_vkPhysicalDevice, m_vkSurface, &formatCount, formats.data());

        if (ImageRenderer::debugGpu())
        {
            cout << "INFO: VulkanView: createSwapchain: surface offers " << formatCount << " format(s):" << endl;
            for (uint32_t i = 0; i < formats.size(); ++i)
            {
                cout << "INFO: VulkanView:   [" << i << "] format=" << formats[i].format << " (" << formatName(formats[i].format)
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
        for (const auto& fmt : formats)
        {
            if (fmt.format == VK_FORMAT_A2B10G10R10_UNORM_PACK32)
            {
                surfaceFormat = fmt;
                found10bit = true;
                break;
            }
        }
        if (!found10bit)
        {
            for (const auto& fmt : formats)
            {
                if (fmt.format == VK_FORMAT_A2R10G10B10_UNORM_PACK32)
                {
                    surfaceFormat = fmt;
                    found10bit = true;
                    break;
                }
            }
        }

        if (found10bit)
        {
            if (ImageRenderer::debugGpu())
            {
                cout << "INFO: VulkanView: createSwapchain: chose " << formatName(surfaceFormat.format) << " (10-bit OK)" << endl;
            }
        }
        else
        {
            cout << "WARNING: VulkanView: Real surface lacks a 10-bit format (A2B10G10R10/A2R10G10B10); requesting OpenGL fallback" << endl;
            requestGLFallback();
            return false;
        }

        m_vkSwapchainFormat = surfaceFormat.format;

        m_vkSwapchainExtent = capabilities.currentExtent;
        if (m_vkSwapchainExtent.width == 0xFFFFFFFF)
        {
            m_vkSwapchainExtent = {(uint32_t)width(), (uint32_t)height()};
        }
        if (m_vkSwapchainExtent.width == 0 || m_vkSwapchainExtent.height == 0)
        {
            requestGLFallback();
            return false;
        }

        uint32_t imageCount = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
        {
            imageCount = capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = m_vkSurface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = m_vkSwapchainExtent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.preTransform = capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; // VSync
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

    void VulkanView::cleanupSwapchain()
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

        // NVIDIA Linux 550+ drivers return blank pixels to OpenGL for LINEAR shared
        // images >= ~2 MiB (forum thread #349436). Allocating the shared image with
        // VK_IMAGE_TILING_OPTIMAL avoids that broken linear path and restores
        // correct zero-copy interop, so OPTIMAL is the default on NVIDIA. Both the
        // GL and Vulkan sides here are the same NVIDIA driver/GPU, so the
        // vendor-private optimal layout matches on import without needing explicit
        // DRM-format-modifier negotiation. AMD/Intel keep the existing LINEAR path.
        // Set RV_VULKAN_DISABLE_NVIDIA_INTEROP_WORKAROUND to revert NVIDIA to LINEAR
        // (reproduces the blank-image bug, for debugging).
        bool useOptimalTilingForInterop(VkPhysicalDevice dev)
        {
#if defined(PLATFORM_LINUX)
            return !nvidiaInteropWorkaroundDisabled() && isNvidiaPhysicalDevice(dev);
#else
            (void)dev;
            return false;
#endif
        }
    } // namespace

    void VulkanView::cleanupSharedImage(uint32_t slot)
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

    void VulkanView::drainSharedSemaphores(uint32_t slot)
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

    const VulkanView::SharedImageInfo* VulkanView::getSharedImageInfo(int w, int h)
    {
        if (!m_vkDevice)
            return nullptr;

        // Build/return the shared image for the current in-flight ring slot.
        const uint32_t slot = m_currentFrame;
        SharedImageInfo& info = m_sharedImageInfo[slot];

        // The swapchain always tracks the window size, so recreate it on any size
        // change. This is independent of the grow-only shared image below: a drag
        // still recreates the (warm) swapchain each step, but no longer rebuilds
        // or re-exports the shared image.
        if (!m_vkSwapchain || m_vkSwapchainExtent.width != (uint32_t)w || m_vkSwapchainExtent.height != (uint32_t)h)
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
        int screenW = 0;
        int screenH = 0;
        if (QScreen* scr = QGuiApplication::primaryScreen())
        {
            const qreal dpr = scr->devicePixelRatio();
            screenW = static_cast<int>(scr->geometry().width() * dpr);
            screenH = static_cast<int>(scr->geometry().height() * dpr);
        }
        const int capW = std::max({w, screenW, m_sharedCapacityW[slot]});
        const int capH = std::max({h, screenH, m_sharedCapacityH[slot]});

        cleanupSharedImage(slot);

        if (ImageRenderer::debugGpu())
        {
            cout << "INFO: VulkanView: getSharedImageInfo: (re)allocating shared image slot " << slot << " capacity " << capW << "x" << capH
                 << " for request " << w << "x" << h << endl;
        }

        // OPTIMAL tiling on NVIDIA (the fix for the blank large-image bug); LINEAR
        // elsewhere. See useOptimalTilingForInterop().
        const bool optimalTiling = useOptimalTilingForInterop(m_vkPhysicalDevice);

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
            cerr << "ERROR: VulkanView: Failed to create shared image" << endl;
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
            const VkFormatFeatureFlags srcFeatures =
                optimalTiling ? srcProps.optimalTilingFeatures : srcProps.linearTilingFeatures;
            const bool blitOk = (srcFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT)
                                && (dstProps.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT);
            if (!blitOk)
            {
                if (ImageRenderer::debugGpu())
                {
                    cout << "INFO: VulkanView: GPU interop unavailable for " << formatName(m_vkSwapchainFormat)
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

        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(m_vkDevice, m_vkSharedImage[slot], &memReqs);

        VkExportMemoryAllocateInfo exportAllocInfo = {};
        exportAllocInfo.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
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
            cerr << "ERROR: VulkanView: No device-local memory type for shared image" << endl;
            cleanupSharedImage(slot);
            return nullptr;
        }

        if (vkAllocateMemory(m_vkDevice, &allocInfo, nullptr, &m_vkSharedImageMemory[slot]) != VK_SUCCESS)
        {
            cerr << "ERROR: VulkanView: Failed to allocate shared image memory" << endl;
            cleanupSharedImage(slot);
            return nullptr;
        }

        if (vkBindImageMemory(m_vkDevice, m_vkSharedImage[slot], m_vkSharedImageMemory[slot], 0) != VK_SUCCESS)
        {
            cerr << "ERROR: VulkanView: Failed to bind shared image memory" << endl;
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
            cerr << "ERROR: VulkanView: vkGetMemoryWin32HandleKHR not found" << endl;
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
            cerr << "ERROR: VulkanView: Failed to get memory HANDLE" << endl;
            cleanupSharedImage(slot);
            return nullptr;
        }
#else
        auto pfnGetMemoryFdKHR = (PFN_vkGetMemoryFdKHR)vkGetDeviceProcAddr(m_vkDevice, "vkGetMemoryFdKHR");
        if (!pfnGetMemoryFdKHR)
        {
            cerr << "ERROR: VulkanView: vkGetMemoryFdKHR not found" << endl;
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
            cerr << "ERROR: VulkanView: Failed to get memory FD" << endl;
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
            cerr << "ERROR: VulkanView: Failed to create shared semaphores" << endl;
            cleanupSharedImage(slot);
            return nullptr;
        }

        // Export the GL<->Vulkan sync semaphores as external handles.
#ifdef PLATFORM_WINDOWS
        auto pfnGetSemaphoreWin32HandleKHR =
            (PFN_vkGetSemaphoreWin32HandleKHR)vkGetDeviceProcAddr(m_vkDevice, "vkGetSemaphoreWin32HandleKHR");
        if (!pfnGetSemaphoreWin32HandleKHR)
        {
            cerr << "ERROR: VulkanView: vkGetSemaphoreWin32HandleKHR not found" << endl;
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
            cerr << "ERROR: VulkanView: Failed to get glReady semaphore HANDLE" << endl;
            cleanupSharedImage(slot);
            return nullptr;
        }

        getSemHandleInfo.semaphore = m_vkVkReadySemaphore[slot];
        if (pfnGetSemaphoreWin32HandleKHR(m_vkDevice, &getSemHandleInfo, &vkReadyHandle) != VK_SUCCESS || !vkReadyHandle)
        {
            cerr << "ERROR: VulkanView: Failed to get vkReady semaphore HANDLE" << endl;
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
            cerr << "ERROR: VulkanView: vkGetSemaphoreFdKHR not found" << endl;
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
            cerr << "ERROR: VulkanView: Failed to get glReady semaphore FD" << endl;
            cleanupSharedImage(slot);
            return nullptr;
        }

        getSemFdInfo.semaphore = m_vkVkReadySemaphore[slot];
        if (pfnGetSemaphoreFdKHR(m_vkDevice, &getSemFdInfo, &vkReadyFd) != VK_SUCCESS || vkReadyFd < 0)
        {
            cerr << "ERROR: VulkanView: Failed to get vkReady semaphore FD" << endl;
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

    void VulkanView::presentSharedImage()
    {
        const uint32_t slot = m_currentFrame;
        const SharedImageInfo& info = m_sharedImageInfo[slot];

        if (!m_vkDevice || !m_vkSharedImage[slot] || !m_vkSwapchain)
            return;

        // Start-of-frame throttle: wait for this slot's previous frame to finish
        // before reusing its acquire semaphore and per-frame resources. This
        // replaces the old end-of-frame block; with FIFO acquire back-pressure it
        // is what paces the loop to display refresh while still allowing
        // FRAMES_IN_FLIGHT frames outstanding.
        vkWaitForFences(m_vkDevice, 1, &m_vkFence[slot], VK_TRUE, UINT64_MAX);

        // Acquire image
        uint32_t imageIndex;
        VkResult result =
            vkAcquireNextImageKHR(m_vkDevice, m_vkSwapchain, UINT64_MAX, m_vkImageAvailableSemaphore[slot], VK_NULL_HANDLE, &imageIndex);
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
        if (m_vkSwapchainFormat == VK_FORMAT_A2B10G10R10_UNORM_PACK32)
        {
            VkImageCopy region = {};
            region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.srcSubresource.layerCount = 1;
            region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.dstSubresource.layerCount = 1;
            region.extent = {(uint32_t)info.width, (uint32_t)info.height, 1};

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
            blit.dstOffsets[1] = {info.width, info.height, 1};

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
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
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
        if (presentResult != VK_SUCCESS)
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

    void VulkanView::presentPixelData(const void* pixels, int w, int h)
    {
        const uint32_t slot = m_currentFrame;

        if (!m_vkDevice)
            return;

        if (!m_vkSwapchain || m_vkSwapchainExtent.width != (uint32_t)w || m_vkSwapchainExtent.height != (uint32_t)h)
        {
            // Warm recreate via oldSwapchain (createSwapchain retires the old one).
            if (!createSwapchain())
                return;
        }

        // Start-of-frame throttle (matches presentSharedImage): wait for this
        // slot's previous frame to finish before reusing its staging buffer,
        // acquire semaphore and command resources.
        vkWaitForFences(m_vkDevice, 1, &m_vkFence[slot], VK_TRUE, UINT64_MAX);

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
                cerr << "ERROR: VulkanView: No host-visible memory type for staging buffer" << endl;
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
        VkResult result =
            vkAcquireNextImageKHR(m_vkDevice, m_vkSwapchain, UINT64_MAX, m_vkImageAvailableSemaphore[slot], VK_NULL_HANDLE, &imageIndex);
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
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
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
        if (presentResult != VK_SUCCESS)
        {
            if (presentResult == VK_ERROR_DEVICE_LOST)
            {
                requestGLFallback();
            }
            return;
        }
    }

    //--------------------------------------------------------------------------

    void VulkanView::requestUpdate()
    {
        if (m_stopProcessingEvents)
        {
            return;
        }
        // Coalesce: only queue a render if one isn't already pending. The flag is
        // cleared at the start of render(), so a resize arriving mid-render
        // schedules exactly one follow-up render at the newest size.
        if (m_updatePending)
            return;
        m_updatePending = true;
        QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
    }

    void VulkanView::render()
    {
        m_updatePending = false;

        if (m_stopProcessingEvents)
        {
            return;
        }

        IPCore::Session* session = m_doc ? m_doc->session() : nullptr;
        if (!session)
            return;

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

            session->render();

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

        if (session)
        {
            if (session->outputVideoDevice() && session->outputVideoDevice() != videoDevice())
            {
                session->outputVideoDevice()->syncBuffers();
            }
            else
            {
                m_videoDevice->syncBuffers();
            }
        }

        if (session)
        {
            session->addSyncSample();
            session->postRender();
        }

        m_eventProcessingTimer.start();
    }

    //--------------------------------------------------------------------------
    // QWidget overrides
    //--------------------------------------------------------------------------

    void VulkanView::showEvent(QShowEvent* event)
    {
        if (!m_initialized)
            initialize();
        requestUpdate();
        QWidget::showEvent(event);
    }

    void VulkanView::resizeEvent(QResizeEvent* event)
    {
        if (m_doc)
            m_doc->viewSizeChanged(event->size().width(), event->size().height());
        QWidget::resizeEvent(event);

        // WA_PaintOnScreen means Qt won't repaint this native surface on resize,
        // so drive a render now to recreate the swapchain at the new size and
        // present immediately (instead of waiting for a mouse Enter event).
        // Coalesced so a fast drag doesn't queue one heavy recreate per event.
        if (!m_stopProcessingEvents)
        {
            requestUpdate();
        }
    }

    void VulkanView::paintEvent(QPaintEvent* event)
    {
        if (m_stopProcessingEvents)
        {
            return;
        }

        if (m_doc && m_doc->session() && m_doc->session()->outputVideoDevice())
        {
            m_doc->session()->outputVideoDevice()->syncBuffers();
        }
        else if (m_videoDevice)
        {
            m_videoDevice->syncBuffers();
        }
    }

    //--------------------------------------------------------------------------
    // eventProcessingTimeout slot
    //--------------------------------------------------------------------------

    void VulkanView::eventProcessingTimeout()
    {
        if (m_doc && m_doc->session())
            m_doc->session()->userGenericEvent("per-render-event-processing", "");
    }

    //--------------------------------------------------------------------------
    // event()
    //--------------------------------------------------------------------------

    bool VulkanView::event(QEvent* event)
    {
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
            m_videoDevice->translator().resetModifiers();
            // fall-through
        case QEvent::Enter:
            if (m_eventWidget)
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
            return QWidget::event(event);
        }

        if (event->type() == QEvent::UpdateRequest)
        {
            render();
            return true;
        }

        if (!m_videoDevice || !m_videoDevice->hasTranslator())
        {
            return QWidget::event(event);
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

        if (m_videoDevice->translator().sendQTEvent(event, activationTime))
        {
            event->accept();
            return true;
        }
        else
        {
            return QWidget::event(event);
        }
    }

    //--------------------------------------------------------------------------
    // eventFilter()
    //--------------------------------------------------------------------------

    bool VulkanView::eventFilter(QObject* object, QEvent* event)
    {
        if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease || event->type() == QEvent::Shortcut
            || event->type() == QEvent::ShortcutOverride)
        {
            if (QKeyEvent* kevent = dynamic_cast<QKeyEvent*>(event))
            {
                if (m_lastKey == kevent->key()
                    && (m_lastKeyType == QEvent::ShortcutOverride && (kevent->type() == QEvent::KeyPress)
                        || (m_lastKeyType == kevent->type())))
                {
                    m_lastKey = kevent->key();
                    m_lastKeyType = kevent->type();
                    event->accept();
                    return true;
                }
                m_lastKeyType = kevent->type();
                m_lastKey = kevent->key();
            }

            Session* session = m_doc ? m_doc->session() : nullptr;
            if (session)
            {
                session->setEventVideoDevice(videoDevice());
                if (m_videoDevice->translator().sendQTEvent(event))
                {
                    event->accept();
                    return true;
                }
            }

            event->accept();
            return true;
        }

        return false;
    }

} // namespace Rv

#endif // PLATFORM_LINUX || PLATFORM_WINDOWS
