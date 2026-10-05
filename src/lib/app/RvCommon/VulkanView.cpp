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
#include <array>
#include <cctype>
#include <climits>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
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

//
//  Environment variables recognized by the Vulkan presentation path
//  ----------------------------------------------------------------
//  These exist so a corrupted or failing display can be narrowed to a stage
//  without a rebuild -- notably by a tester running a build on hardware the
//  developer cannot access. Every override is reported in the startup record
//  (see VulkanView::emitPresentationRecord), alongside the value negotiation
//  would otherwise have chosen.
//
//    RV_VULKAN_FORCE_CPU_PRESENT
//        Set (to any value) to skip GL<->Vulkan zero-copy interop entirely and
//        present via the CPU readback path. Still 10-bit, just slower.
//
//    RV_VULKAN_FORCE_TILING = optimal | linear
//        Override the negotiated shared-image tiling. The override is honored
//        only if the driver reports that tiling as exportable; otherwise it is
//        logged and refused, because presenting through a configuration whose
//        correctness was not established is what this path is meant to avoid.
//        An unrecognized value is logged and ignored (negotiation proceeds).
//
//    RV_VULKAN_FORCE_NO_DEDICATED
//        Set (to any value) to suppress dedicated allocation even when the
//        driver reports it as preferred. Refused when the driver reports
//        DEDICATED_ONLY, since that is a requirement rather than a preference.
//
//  Both sides of the interop read their settings from one negotiated struct,
//  so an override applies to the Vulkan export and the GL import together.
//

namespace Rv
{
    using namespace std;
    using namespace TwkApp;
    using namespace IPCore;

    namespace
    {
        // Read an env var that is treated as a boolean flag by presence.
        bool envFlagSet(const char* name) { return getenv(name) != nullptr; }

        constexpr std::string_view tilingName(VkImageTiling tiling)
        {
            switch (tiling)
            {
            case VK_IMAGE_TILING_OPTIMAL:
                return "OPTIMAL";
            case VK_IMAGE_TILING_LINEAR:
                return "LINEAR";
            default:
                return "(other)";
            }
        }

        constexpr std::string_view colorSpaceName(VkColorSpaceKHR colorSpace)
        {
            switch (colorSpace)
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

        // Decode RV_VULKAN_FORCE_TILING. Returns nullopt when unset or when the
        // value is not recognized; an unrecognized value is reported rather
        // than silently behaving as if the variable were unset.
        std::optional<VkImageTiling> forcedTilingRequested()
        {
            const char* value = getenv("RV_VULKAN_FORCE_TILING");
            if (!value)
            {
                return std::nullopt;
            }

            std::string lowered(value);
            std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                           [](unsigned char ch) { return static_cast<char>(::tolower(ch)); });

            if (lowered == "optimal")
            {
                return VK_IMAGE_TILING_OPTIMAL;
            }
            if (lowered == "linear")
            {
                return VK_IMAGE_TILING_LINEAR;
            }

            cout << "WARNING: VulkanView: RV_VULKAN_FORCE_TILING='" << value << "' is not recognized (expected 'optimal' or 'linear'); "
                 << "ignoring it and using the negotiated tiling" << endl;
            return std::nullopt;
        }

        // Both A2B10G10R10 and A2R10G10B10 are 10-bit-per-channel packed formats;
        // they differ only in R/B component order. Both are acceptable for 10-bit
        // presentation -- the R/B order is handled where pixels are packed (CPU
        // fallback) or blitted (GPU interop). A2B10G10R10 (== GL_RGB10_A2) is
        // preferred when the surface offers it, but many Linux/RADV surfaces only
        // advertise A2R10G10B10.
        bool isTenBitFormat(VkFormat format)
        {
            return format == VK_FORMAT_A2B10G10R10_UNORM_PACK32 || format == VK_FORMAT_A2R10G10B10_UNORM_PACK32;
        }

        constexpr std::string_view formatName(VkFormat format)
        {
            switch (format)
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

        std::optional<uint32_t> findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
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
            return std::nullopt;
        }

        // Resolve a device-level Vulkan entry point as its PFN type.
        template <typename Fn> Fn deviceProc(VkDevice device, const char* name)
        {
            return reinterpret_cast<Fn>(vkGetDeviceProcAddr(device, name));
        }

        // Record a single-image layout transition (color aspect, 1 mip, 1 layer,
        // no queue family ownership transfer).
        void imageBarrier(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout,
                          VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage)
        {
            VkImageMemoryBarrier barrier = {};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = oldLayout;
            barrier.newLayout = newLayout;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.srcAccessMask = srcAccess;
            barrier.dstAccessMask = dstAccess;

            vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        }
    } // namespace

    //--------------------------------------------------------------------------
    // VulkanView implementation
    //--------------------------------------------------------------------------

    VulkanView::VulkanView(RvDocument* doc, QWidget* parent, bool noResize)
        : QWidget(parent)
        , m_doc(doc)
        , m_postFirstNonEmptyRender(noResize)
    {
        // Force the creation of a native window early.
        setAttribute(Qt::WA_NativeWindow);
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_OpaquePaintEvent);
        setAttribute(Qt::WA_PaintOnScreen);
        setAttribute(Qt::WA_TranslucentBackground);
        setAutoFillBackground(false);

        // Wait to configure the QWindow until it's created
        if (QWindow* window = windowHandle())
        {
            window->setSurfaceType(QSurface::VulkanSurface);

            // Set 10-bit format
            QSurfaceFormat fmt;
            fmt.setRedBufferSize(10);
            fmt.setGreenBufferSize(10);
            fmt.setBlueBufferSize(10);
            fmt.setAlphaBufferSize(2);
            window->setFormat(fmt);
        }

        ostringstream str;
        str << UI_APPLICATION_NAME " Main Window (Vulkan)" << "/" << m_doc;
        m_videoDevice = std::make_unique<QTVulkanVideoDevice>(nullptr, str.str(), this, nullptr);

        m_activityTimer.start();

        m_eventProcessingTimer.setSingleShot(true);
        connect(&m_eventProcessingTimer, SIGNAL(timeout()), this, SLOT(eventProcessingTimeout()));
    }

    VulkanView::~VulkanView()
    {
        // Release the device (and its GL imports) before the Vulkan memory they alias.
        m_videoDevice.reset();
        for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i)
        {
            cleanupSharedImage(i);
        }
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

        if (QWindow* window = windowHandle())
        {
            window->setSurfaceType(QSurface::VulkanSurface);
            QSurfaceFormat fmt;
            fmt.setRedBufferSize(10);
            fmt.setGreenBufferSize(10);
            fmt.setBlueBufferSize(10);
            fmt.setAlphaBufferSize(2);
            window->setFormat(fmt);
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
        // The VkInstance is created and owned by Qt.
        static QVulkanInstance* qtVkInst = nullptr;
        if (!qtVkInst)
        {
            // Ask Qt for a 1.1 instance: the interop capability probe uses
            // vkGetPhysicalDeviceImageFormatProperties2, which is core in 1.1.
            // Without it negotiateInteropConfig() cannot establish whether a
            // configuration is exportable and has to refuse interop outright.
            // A 1.0-only loader is still tolerated -- retry unversioned and let
            // the probe fall back to the KHR alias, or degrade if that is
            // absent too.
            qtVkInst = new QVulkanInstance();
            qtVkInst->setApiVersion(QVersionNumber(1, 1));
            if (!qtVkInst->create())
            {
                cout << "WARNING: VulkanView: QVulkanInstance create failed at apiVersion 1.1; retrying with the loader default" << endl;
                delete qtVkInst;
                qtVkInst = new QVulkanInstance();
                if (!qtVkInst->create())
                {
                    cerr << "ERROR: VulkanView: QVulkanInstance create failed" << endl;
                    delete qtVkInst;
                    qtVkInst = nullptr;
                    return false;
                }
            }
        }

        m_vkInstance = qtVkInst->vkInstance();

        // Create Surface
        QWindow* window = windowHandle();
        if (!window)
        {
            return false;
        }

        window->setVulkanInstance(qtVkInst);

        m_vkSurface = qtVkInst->surfaceForWindow(window);
        if (!m_vkSurface)
        {
            cerr << "ERROR: VulkanView: Failed to create Vulkan surface" << endl;
            return false;
        }

        // Qt owns the surface and destroys it with the native window, which on
        // close happens before this widget is deleted. Watch for that so the
        // swapchain is released first (see eventFilter()).
        window->installEventFilter(this);

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
            {
                break;
            }
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
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (vkCreateDevice(m_vkPhysicalDevice, &createInfo, nullptr, &m_vkDevice) != VK_SUCCESS)
        {
            return false;
        }

        vkGetDeviceQueue(m_vkDevice, m_queueFamilyIndex, 0, &m_vkQueue);

        // Negotiate the GL<->Vulkan interop configuration once, here. It is a
        // property of the device, not of a shared-image slot or of the current
        // window size, so it must not be recomputed per slot or on resize.
        negotiateInteropConfig();
        if (ImageRenderer::debugGpu())
        {
            cout << "INFO: VulkanView: initVulkan: interop negotiation ran (once per device); result="
                 << (m_interopConfig.supported ? "supported" : "unsupported") << endl;
        }

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
        for (FrameSync& sync : m_frameSync)
        {
            if (vkCreateSemaphore(m_vkDevice, &semaphoreInfo, nullptr, &sync.imageAvailable) != VK_SUCCESS)
            {
                return failInit();
            }
            if (vkCreateFence(m_vkDevice, &fenceInfo, nullptr, &sync.fence) != VK_SUCCESS)
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

        // The OpenGL rung forgoes 10-bit, so it must be visible in the log
        // rather than inferred from the absence of a Vulkan record. Callers
        // that know why set m_presentPathReason before calling.
        reportPresentPath(PresentPath::OpenGL,
                          m_presentPathReason.empty() ? std::string("Vulkan presentation could not be established") : m_presentPathReason);

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
        for (FrameSync& sync : m_frameSync)
        {
            if (sync.imageAvailable)
            {
                vkDestroySemaphore(m_vkDevice, sync.imageAvailable, nullptr);
                sync.imageAvailable = VK_NULL_HANDLE;
            }
            if (vkCreateSemaphore(m_vkDevice, &semaphoreInfo, nullptr, &sync.imageAvailable) != VK_SUCCESS)
            {
                sync.imageAvailable = VK_NULL_HANDLE;
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

            for (FrameSync& sync : m_frameSync)
            {
                if (sync.imageAvailable)
                {
                    vkDestroySemaphore(m_vkDevice, sync.imageAvailable, nullptr);
                    sync.imageAvailable = VK_NULL_HANDLE;
                }
                if (sync.fence)
                {
                    vkDestroyFence(m_vkDevice, sync.fence, nullptr);
                    sync.fence = VK_NULL_HANDLE;
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
        // The VkInstance and VkSurfaceKHR are owned by Qt (QVulkanInstance), so they are not destroyed here.
    }

    bool VulkanView::createSwapchain()
    {
        // The surface is dropped when Qt destroys the native window; pick up
        // the new one if the window has been recreated since.
        if (m_vkDevice && !m_vkSurface)
        {
            QWindow* window = windowHandle();
            if (window && window->handle() && window->vulkanInstance())
            {
                m_vkSurface = QVulkanInstance::surfaceForWindow(window);
            }
        }

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
            cout << "INFO: VulkanView: createSwapchain: surface offers " << formatCount << " format/colorSpace pair(s):" << endl;
            for (uint32_t i = 0; i < formats.size(); ++i)
            {
                cout << "INFO: VulkanView:   [" << i << "] format=" << formats[i].format << " (" << formatName(formats[i].format)
                     << ")  colorSpace=" << formats[i].colorSpace << " (" << colorSpaceName(formats[i].colorSpace) << ")" << endl;
            }
        }

        // Select the format and its color space TOGETHER, as one pairing the
        // surface actually offers. Picking a format first and inheriting
        // whatever color space accompanies it depends on driver list order: a
        // surface that lists A2B10G10R10 under HDR10_ST2084 before listing it
        // under SRGB_NONLINEAR would yield an HDR swapchain fed the SDR-encoded
        // pixels RV renders.
        //
        // A2B10G10R10 (== GL_RGB10_A2) is preferred because it is the layout
        // the interop shared texture and the CPU fallback packing produce
        // natively, making the transfer a plain copy. A2R10G10B10 (common on
        // Linux/RADV) is accepted too: the opposite R/B order is resolved by
        // component-wise packing on the CPU path and by vkCmdBlitImage on the
        // interop path, so red and blue are not swapped.
        VkSurfaceFormatKHR surfaceFormat = formats.empty() ? VkSurfaceFormatKHR{} : formats[0];
        bool found10bit = false;
        bool sawTenBitNonSdr = false;

        constexpr std::array<VkFormat, 2> preferredOrder = {VK_FORMAT_A2B10G10R10_UNORM_PACK32, VK_FORMAT_A2R10G10B10_UNORM_PACK32};
        for (VkFormat want : preferredOrder)
        {
            for (const auto& fmt : formats)
            {
                if (fmt.format != want)
                {
                    continue;
                }

                if (fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                {
                    surfaceFormat = fmt;
                    found10bit = true;
                    break;
                }

                // A 10-bit format, but only in a color space RV does not encode
                // for. Remember it so the fallback reason can say so.
                sawTenBitNonSdr = true;
            }
            if (found10bit)
            {
                break;
            }
        }

        if (!found10bit)
        {
            if (sawTenBitNonSdr)
            {
                cout << "WARNING: VulkanView: the surface offers 10-bit formats only in non-SDR color spaces "
                        "(RV renders SDR-encoded pixels, so presenting into one would mis-encode color); "
                        "requesting OpenGL fallback"
                     << endl;
                m_presentPathReason = "surface offers 10-bit only in a non-SDR color space";
            }
            else
            {
                cout << "WARNING: VulkanView: Real surface lacks a 10-bit format (A2B10G10R10/A2R10G10B10); requesting OpenGL fallback"
                     << endl;
                m_presentPathReason = "surface offers no 10-bit format";
            }
            requestGLFallback();
            return false;
        }

        if (ImageRenderer::debugGpu())
        {
            cout << "INFO: VulkanView: createSwapchain: chose " << formatName(surfaceFormat.format) << " / "
                 << colorSpaceName(surfaceFormat.colorSpace) << " (10-bit SDR OK)" << endl;
        }

        m_vkSwapchainFormat = surfaceFormat.format;
        m_vkSwapchainColorSpace = surfaceFormat.colorSpace;

        m_vkSwapchainExtent = capabilities.currentExtent;
        if (m_vkSwapchainExtent.width == std::numeric_limits<uint32_t>::max())
        {
            m_vkSwapchainExtent = {static_cast<uint32_t>(width()), static_cast<uint32_t>(height())};
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

        // Per-swapchain-image present-wait semaphores + in-flight fence map. The
        // device is idle here (the retire path above waited on it), so any old
        // renderFinished semaphores from a previous swapchain are safe to destroy.
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
                {
                    vkDestroySemaphore(m_vkDevice, sem, nullptr);
                }
            }
            m_vkRenderFinished.clear();
            m_imagesInFlight.clear();

            for (StagingBuffer& staging : m_staging)
            {
                if (staging.buffer)
                {
                    vkDestroyBuffer(m_vkDevice, staging.buffer, nullptr);
                    staging.buffer = VK_NULL_HANDLE;
                }
                if (staging.memory)
                {
                    vkFreeMemory(m_vkDevice, staging.memory, nullptr);
                    staging.memory = VK_NULL_HANDLE;
                }
                staging.size = 0;
            }

            if (!m_vkCommandBuffers.empty())
            {
                vkFreeCommandBuffers(m_vkDevice, m_vkCommandPool, static_cast<uint32_t>(m_vkCommandBuffers.size()),
                                     m_vkCommandBuffers.data());
                m_vkCommandBuffers.clear();
            }

            if (m_vkSwapchain)
            {
                vkDestroySwapchainKHR(m_vkDevice, m_vkSwapchain, nullptr);
                m_vkSwapchain = VK_NULL_HANDLE;
            }
        }
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

        // Resolve vkGetPhysicalDeviceImageFormatProperties2, preferring the
        // core 1.1 entry point and falling back to the KHR alias. Qt owns the
        // VkInstance, so which one exists depends on the apiVersion Qt created
        // it with; initVulkan asks Qt for 1.1 but must tolerate a 1.0 loader.
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
    } // namespace

    //
    //  Probe the driver for an exportable shared-image configuration.
    //
    //  The configuration is chosen from what the driver reports, not from GPU
    //  vendor identity or host platform. Candidates are ordered OPTIMAL before
    //  LINEAR: OPTIMAL is the layout drivers are built around, and LINEAR is
    //  the compatibility rung that additionally carries the rowPitch % 4
    //  constraint that can fail allocation outright.
    //
    //  Usage always includes COLOR_ATTACHMENT because the GL side attaches the
    //  imported texture to GL_COLOR_ATTACHMENT0 and renders into it. Declaring
    //  only TRANSFER_SRC lets the driver pick an internal compressed layout the
    //  GL import does not decode, which corrupts the image rather than failing.
    //  If no candidate with the honest usage is exportable, interop is refused
    //  rather than narrowed to a declaration the code then violates.
    //
    void VulkanView::negotiateInteropConfig()
    {
        if (m_interopNegotiated)
        {
            return;
        }
        m_interopNegotiated = true;

        InteropConfig cfg;
        cfg.format = VK_FORMAT_A2B10G10R10_UNORM_PACK32; // == GL_RGB10_A2
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

        constexpr std::array<VkImageTiling, 2> candidates = {VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_TILING_LINEAR};

        bool found = false;
        for (VkImageTiling tiling : candidates)
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

            const VkResult result = probe(m_vkPhysicalDevice, &fmtInfo, &props);

            ostringstream entry;
            entry << tilingName(tiling) << ": ";

            if (result != VK_SUCCESS)
            {
                entry << "not supported for this format/usage (VkResult " << result << ")";
                cfg.candidateLog.push_back(entry.str());
                continue;
            }

            const VkExternalMemoryFeatureFlags features = extProps.externalMemoryProperties.externalMemoryFeatures;
            const bool exportable = (extProps.externalMemoryProperties.compatibleHandleTypes & handleType) != 0
                                    && (features & VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT) != 0
                                    && (features & VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT) != 0;

            if (!exportable)
            {
                entry << "supported but not exportable+importable for this handle type"
                      << " (features=0x" << std::hex << features << std::dec << ")";
                cfg.candidateLog.push_back(entry.str());
                continue;
            }

            // DEDICATED_ONLY is the only dedicated-allocation signal available
            // from the external-memory probe: it is a hard requirement of the
            // handle type. The softer "prefers dedicated" signal is a property
            // of a concrete image, not of the format, and is read per-image
            // from VkMemoryDedicatedRequirements at allocation time -- see
            // getSharedImageInfo(). This value is therefore the floor, and the
            // per-slot SharedImageInfo::dedicatedAllocation is the final
            // decision the GL side must mirror.
            const bool dedicatedOnly = (features & VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT) != 0;

            cfg.supported = true;
            cfg.tiling = tiling;
            cfg.externalFeatures = features;
            cfg.dedicatedAllocation = dedicatedOnly;
            cfg.probedTiling = tiling;
            cfg.probedDedicated = cfg.dedicatedAllocation;

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

        // Apply the diagnostic overrides last, so the record can report both
        // the negotiated value and the forced one. An override is honored only
        // when the driver reported that configuration as usable.
        const std::optional<VkImageTiling> forcedTiling = forcedTilingRequested();
        if (forcedTiling && *forcedTiling != cfg.tiling)
        {
            // Re-probe the forced tiling rather than trusting the request:
            // presenting through an unverified configuration is exactly what
            // the fallback ladder exists to prevent.
            VkPhysicalDeviceExternalImageFormatInfo extInfo = {};
            extInfo.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO;
            extInfo.handleType = handleType;

            VkPhysicalDeviceImageFormatInfo2 fmtInfo = {};
            fmtInfo.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2;
            fmtInfo.pNext = &extInfo;
            fmtInfo.format = cfg.format;
            fmtInfo.type = VK_IMAGE_TYPE_2D;
            fmtInfo.tiling = *forcedTiling;
            fmtInfo.usage = cfg.usage;

            VkExternalImageFormatProperties extProps = {};
            extProps.sType = VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES;
            VkImageFormatProperties2 props = {};
            props.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2;
            props.pNext = &extProps;

            const VkResult result = probe(m_vkPhysicalDevice, &fmtInfo, &props);
            const VkExternalMemoryFeatureFlags features = extProps.externalMemoryProperties.externalMemoryFeatures;
            const bool ok = result == VK_SUCCESS && (extProps.externalMemoryProperties.compatibleHandleTypes & handleType) != 0
                            && (features & VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT) != 0
                            && (features & VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT) != 0;

            if (ok)
            {
                cfg.tilingOverridden = true;
                cfg.tiling = *forcedTiling;
                cfg.externalFeatures = features;
                cfg.dedicatedAllocation = (features & VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT) != 0;
            }
            else
            {
                cout << "WARNING: VulkanView: RV_VULKAN_FORCE_TILING=" << tilingName(*forcedTiling)
                     << " refused -- the driver does not report it as exportable; using the negotiated " << tilingName(cfg.tiling) << endl;
            }
        }

        if (envFlagSet("RV_VULKAN_FORCE_NO_DEDICATED") && cfg.dedicatedAllocation)
        {
            if ((cfg.externalFeatures & VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT) != 0)
            {
                cout << "WARNING: VulkanView: RV_VULKAN_FORCE_NO_DEDICATED refused -- the driver reports "
                     << "DEDICATED_ONLY for this configuration, which is a requirement rather than a preference" << endl;
            }
            else
            {
                cfg.dedicatedOverridden = true;
                cfg.dedicatedAllocation = false;
            }
        }

        m_interopConfig = cfg;
    }

    void VulkanView::reportPresentPath(PresentPath path, const std::string& reason)
    {
        m_presentPath = path;
        m_presentPathReason = reason;
        emitPresentationRecord();
    }

    void VulkanView::reportGLImportState(VkImageTiling tiling, bool dedicated)
    {
        m_glImportTiling = tiling;
        m_glImportDedicated = dedicated;
        m_glImportReported = true;
    }

    //
    //  One record per session, emitted unconditionally. Windows/NVIDIA is
    //  verified by testers against a produced build rather than by the
    //  developer, so this has to be sufficient on its own to establish which
    //  path ran and what was negotiated. Per-frame and per-candidate detail
    //  stays behind ImageRenderer::debugGpu().
    //
    void VulkanView::emitPresentationRecord()
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

        std::string_view pathName = "undetermined";
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

        ostringstream record;
        record << "INFO: RV Vulkan presentation report\n";
        record << "INFO:   GPU            : " << (m_vkPhysicalDevice != VK_NULL_HANDLE ? props.deviceName : "(none)") << "  vendorID=0x"
               << std::hex << props.vendorID << std::dec << "  driverVersion=" << props.driverVersion
               << "  apiVersion=" << VK_VERSION_MAJOR(props.apiVersion) << "." << VK_VERSION_MINOR(props.apiVersion) << "."
               << VK_VERSION_PATCH(props.apiVersion) << "\n";
        record << "INFO:   Present path   : " << pathName << "\n";
        if (!m_presentPathReason.empty())
        {
            record << "INFO:   Reason         : " << m_presentPathReason << "\n";
        }
        record << "INFO:   Swapchain      : " << formatName(m_vkSwapchainFormat) << " / " << colorSpaceName(m_vkSwapchainColorSpace)
               << "\n";

        const InteropConfig& config = m_interopConfig;
        if (config.supported)
        {
            record << "INFO:   Shared image   : " << formatName(config.format) << " tiling=" << tilingName(config.tiling)
                   << " usage=COLOR_ATTACHMENT|TRANSFER_SRC\n";
            record << "INFO:   Dedicated alloc: " << (config.dedicatedAllocation ? "yes" : "no") << "  (driver externalMemoryFeatures=0x"
                   << std::hex << config.externalFeatures << std::dec
                   << (config.externalFeatures & VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT ? " DEDICATED_ONLY" : "") << ")\n";
            if (config.tilingOverridden)
            {
                record << "INFO:   Tiling override: RV_VULKAN_FORCE_TILING forced " << tilingName(config.tiling) << "; negotiation chose "
                       << tilingName(config.probedTiling) << "\n";
            }
            if (config.dedicatedOverridden)
            {
                record << "INFO:   Dedicated ovr  : RV_VULKAN_FORCE_NO_DEDICATED suppressed dedicated allocation; negotiation chose "
                       << (config.probedDedicated ? "yes" : "no") << "\n";
            }
            if (m_glImportReported)
            {
                const bool agree = m_glImportTiling == config.tiling && m_glImportDedicated == config.dedicatedAllocation;
                record << "INFO:   GL import      : tiling=" << tilingName(m_glImportTiling)
                       << " dedicated=" << (m_glImportDedicated ? "yes" : "no") << "  -- "
                       << (agree ? "matches the Vulkan export" : "DISAGREES WITH THE VULKAN EXPORT (expect a corrupted image)") << "\n";
            }
        }
        else
        {
            record << "INFO:   Shared image   : not used -- "
                   << (config.rejectReason.empty() ? "interop not negotiated" : config.rejectReason) << "\n";
        }

        for (const std::string& entry : config.candidateLog)
        {
            record << "INFO:   Probe candidate: " << entry << "\n";
        }

        // Kept for comparison until the Linux/NVIDIA probe result is confirmed
        // to agree; the vendor heuristic is removed once it does.
        record << "INFO:   Legacy vendor heuristic would have chosen: "
               << (m_vkPhysicalDevice != VK_NULL_HANDLE && useOptimalTilingForInterop(m_vkPhysicalDevice) ? "OPTIMAL" : "LINEAR") << "\n";

        if (envFlagSet("RV_VULKAN_FORCE_CPU_PRESENT"))
        {
            record << "INFO:   Override       : RV_VULKAN_FORCE_CPU_PRESENT is set\n";
        }

        cout << record.str() << flush;
    }

    void VulkanView::cleanupSharedImage(uint32_t slot)
    {
        SharedImageInfo& info = m_shared[slot].info;

        if (m_vkDevice)
        {
            vkDeviceWaitIdle(m_vkDevice);

            if (m_shared[slot].image)
            {
                vkDestroyImage(m_vkDevice, m_shared[slot].image, nullptr);
                m_shared[slot].image = VK_NULL_HANDLE;
            }
            if (m_shared[slot].memory)
            {
                vkFreeMemory(m_vkDevice, m_shared[slot].memory, nullptr);
                m_shared[slot].memory = VK_NULL_HANDLE;
            }
            if (m_shared[slot].glReady)
            {
                vkDestroySemaphore(m_vkDevice, m_shared[slot].glReady, nullptr);
                m_shared[slot].glReady = VK_NULL_HANDLE;
            }
            if (m_shared[slot].vkReady)
            {
                vkDestroySemaphore(m_vkDevice, m_shared[slot].vkReady, nullptr);
                m_shared[slot].vkReady = VK_NULL_HANDLE;
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
        m_shared[slot].capacityW = 0;
        m_shared[slot].capacityH = 0;
    }

    void VulkanView::drainSharedSemaphores(uint32_t slot)
    {
        // No shared image for this slot yet -> the GL side never signaled/waited
        // its pair, so there is nothing to rebalance.
        if (!m_vkDevice || !m_shared[slot].glReady || !m_shared[slot].vkReady)
        {
            return;
        }

        // Consume the pending glReady signal from the GL side and re-signal
        // vkReady so the next use of this slot starts balanced (exactly what a
        // normal present's submit would have done for the pair).
        VkSubmitInfo drain = {};
        drain.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        const std::array<VkSemaphore, 1> waitSemaphores = {m_shared[slot].glReady};
        const std::array<VkPipelineStageFlags, 1> waitStages = {VK_PIPELINE_STAGE_TRANSFER_BIT};
        drain.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
        drain.pWaitSemaphores = waitSemaphores.data();
        drain.pWaitDstStageMask = waitStages.data();
        drain.commandBufferCount = 0;
        const std::array<VkSemaphore, 1> signalSemaphores = {m_shared[slot].vkReady};
        drain.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
        drain.pSignalSemaphores = signalSemaphores.data();

        const VkResult result = vkQueueSubmit(m_vkQueue, 1, &drain, VK_NULL_HANDLE);
        if (result == VK_ERROR_DEVICE_LOST)
        {
            requestGLFallback();
        }
    }

    const VulkanView::SharedImageInfo* VulkanView::getSharedImageInfo(int w, int h)
    {
        if (!m_vkDevice)
            return nullptr;

        // Build/return the shared image for the current in-flight ring slot.
        const uint32_t slot = m_currentFrame;
        SharedImageInfo& info = m_shared[slot].info;

        // The swapchain always tracks the window size, so recreate it on any size
        // change. This is independent of the grow-only shared image below: a drag
        // still recreates the (warm) swapchain each step, but no longer rebuilds
        // or re-exports the shared image.
        if (!m_vkSwapchain || m_vkSwapchainExtent.width != static_cast<uint32_t>(w)
            || m_vkSwapchainExtent.height != static_cast<uint32_t>(h))
        {
            // Warm recreate via oldSwapchain (createSwapchain retires the old one).
            if (!createSwapchain())
            {
                return nullptr;
            }
        }

        // Grow-only: if the request fits the slot's current allocated capacity,
        // reuse the existing image/export and just update the used sub-region
        // (presentSharedImage copies/blits info.width x info.height from it).
        if (m_shared[slot].image && w <= m_shared[slot].capacityW && h <= m_shared[slot].capacityH)
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
        const int capW = std::max({w, screenW, m_shared[slot].capacityW});
        const int capH = std::max({h, screenH, m_shared[slot].capacityH});

        cleanupSharedImage(slot);

        if (ImageRenderer::debugGpu())
        {
            cout << "INFO: VulkanView: getSharedImageInfo: (re)allocating shared image slot " << slot << " capacity " << capW << "x" << capH
                 << " for request " << w << "x" << h << endl;
        }

        // The interop configuration was negotiated once at device creation from
        // what the driver reports exportable. If nothing was exportable, refuse
        // the zero-copy path here so syncBuffers() takes the CPU readback rung
        // rather than presenting through a configuration whose correctness was
        // never established.
        if (!m_interopConfig.supported)
        {
            if (ImageRenderer::debugGpu())
            {
                cout << "INFO: VulkanView: getSharedImageInfo: interop unavailable (" << m_interopConfig.rejectReason
                     << "); using the CPU readback path" << endl;
            }
            return nullptr;
        }

        const bool optimalTiling = m_interopConfig.tiling == VK_IMAGE_TILING_OPTIMAL;

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
        imageInfo.extent = {static_cast<uint32_t>(capW), static_cast<uint32_t>(capH), 1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = m_interopConfig.tiling;
        // Usage must cover every use on BOTH sides: Vulkan reads the image as a
        // transfer source, and GL attaches it to GL_COLOR_ATTACHMENT0 and
        // renders into it. Declaring only TRANSFER_SRC lets the driver pick an
        // internal compressed layout that the GL import does not decode, which
        // corrupts the image rather than raising an error.
        imageInfo.usage = m_interopConfig.usage;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (vkCreateImage(m_vkDevice, &imageInfo, nullptr, &m_shared[slot].image) != VK_SUCCESS)
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
            // The shared image's blit-source support depends on its actual
            // (negotiated) tiling.
            const VkFormatFeatureFlags srcFeatures = optimalTiling ? srcProps.optimalTilingFeatures : srcProps.linearTilingFeatures;
            const bool blitOk =
                (srcFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) && (dstProps.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT);
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
            vkGetImageSubresourceLayout(m_vkDevice, m_shared[slot].image, &subresource, &layout);

            if (layout.rowPitch % 4 != 0)
            {
                // Cannot represent this stride as an integer pixel-width texture; fall back to CPU bridge.
                cleanupSharedImage(slot);
                return nullptr;
            }
            info.strideWidth = static_cast<int>(layout.rowPitch / 4);
        }
        info.capacityHeight = capH; // GL imports the texture at capacity dimensions
        info.tiling = m_interopConfig.tiling;

        // Resolve the final dedicated-allocation decision for THIS image. The
        // probe supplied the floor (DEDICATED_ONLY, a requirement of the handle
        // type); "prefers dedicated" is a property of a concrete image and is
        // only available here, from VkMemoryDedicatedRequirements. The GL side
        // mirrors info.dedicatedAllocation, so this is the single decision both
        // sides use -- deciding it independently is what corrupts the image.
        VkMemoryRequirements memReqs;
        bool useDedicated = m_interopConfig.dedicatedAllocation;
        {
            VkMemoryDedicatedRequirements dedicatedReqs = {};
            dedicatedReqs.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS;

            VkMemoryRequirements2 memReqs2 = {};
            memReqs2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
            memReqs2.pNext = &dedicatedReqs;

            VkImageMemoryRequirementsInfo2 reqInfo = {};
            reqInfo.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2;
            reqInfo.image = m_shared[slot].image;

            auto pfnGetImageMemoryRequirements2 =
                deviceProc<PFN_vkGetImageMemoryRequirements2>(m_vkDevice, "vkGetImageMemoryRequirements2");
            if (!pfnGetImageMemoryRequirements2)
            {
                pfnGetImageMemoryRequirements2 =
                    deviceProc<PFN_vkGetImageMemoryRequirements2>(m_vkDevice, "vkGetImageMemoryRequirements2KHR");
            }

            if (pfnGetImageMemoryRequirements2)
            {
                pfnGetImageMemoryRequirements2(m_vkDevice, &reqInfo, &memReqs2);
                memReqs = memReqs2.memoryRequirements;
                if (dedicatedReqs.requiresDedicatedAllocation || dedicatedReqs.prefersDedicatedAllocation)
                {
                    useDedicated = true;
                }
            }
            else
            {
                vkGetImageMemoryRequirements(m_vkDevice, m_shared[slot].image, &memReqs);
            }

            // An explicit override may only relax a preference, never a
            // requirement (DEDICATED_ONLY or requiresDedicatedAllocation).
            if (m_interopConfig.dedicatedOverridden && !m_interopConfig.dedicatedAllocation && !dedicatedReqs.requiresDedicatedAllocation)
            {
                useDedicated = false;
            }
        }
        info.dedicatedAllocation = useDedicated;

        // Build the allocation pNext chain back to front, so each link is
        // attached exactly once regardless of which options are active:
        //
        //   allocInfo -> exportAllocInfo [-> dedicatedAllocInfo] [-> exportWin32Info]
        //
        void* chain = nullptr;

#ifdef PLATFORM_WINDOWS
        // Required by the Vulkan specification for OPAQUE_WIN32 handles: the
        // export must state the access rights and security attributes the
        // handle is created with. Its absence is tolerated by some drivers but
        // is a real violation on the platform being debugged.
        VkExportMemoryWin32HandleInfoKHR exportWin32Info = {};
        exportWin32Info.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_KHR;
        exportWin32Info.pNext = chain;
        exportWin32Info.pAttributes = nullptr; // default security attributes
        exportWin32Info.dwAccess = GENERIC_ALL;
        exportWin32Info.name = nullptr; // unnamed: shared within this process only
        chain = &exportWin32Info;
#endif

        // Dedicated allocation when the driver requires or prefers it for this
        // image. NVIDIA's OPAQUE_WIN32 path in particular needs this paired
        // with GL_DEDICATED_MEMORY_OBJECT_EXT on the import side.
        VkMemoryDedicatedAllocateInfo dedicatedAllocInfo = {};
        if (useDedicated)
        {
            dedicatedAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
            dedicatedAllocInfo.pNext = chain;
            dedicatedAllocInfo.image = m_shared[slot].image;
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
        const std::optional<uint32_t> memoryTypeIndex =
            findMemoryType(m_vkPhysicalDevice, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (!memoryTypeIndex)
        {
            cerr << "ERROR: VulkanView: No device-local memory type for shared image" << endl;
            cleanupSharedImage(slot);
            return nullptr;
        }
        allocInfo.memoryTypeIndex = *memoryTypeIndex;

        if (vkAllocateMemory(m_vkDevice, &allocInfo, nullptr, &m_shared[slot].memory) != VK_SUCCESS)
        {
            cerr << "ERROR: VulkanView: Failed to allocate shared image memory" << endl;
            cleanupSharedImage(slot);
            return nullptr;
        }

        if (vkBindImageMemory(m_vkDevice, m_shared[slot].image, m_shared[slot].memory, 0) != VK_SUCCESS)
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
        auto pfnGetMemoryWin32HandleKHR = deviceProc<PFN_vkGetMemoryWin32HandleKHR>(m_vkDevice, "vkGetMemoryWin32HandleKHR");
        if (!pfnGetMemoryWin32HandleKHR)
        {
            cerr << "ERROR: VulkanView: vkGetMemoryWin32HandleKHR not found" << endl;
            cleanupSharedImage(slot);
            return nullptr;
        }

        VkMemoryGetWin32HandleInfoKHR getHandleInfo = {};
        getHandleInfo.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
        getHandleInfo.memory = m_shared[slot].memory;
        getHandleInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;

        HANDLE memHandle = nullptr;
        if (pfnGetMemoryWin32HandleKHR(m_vkDevice, &getHandleInfo, &memHandle) != VK_SUCCESS || !memHandle)
        {
            cerr << "ERROR: VulkanView: Failed to get memory HANDLE" << endl;
            cleanupSharedImage(slot);
            return nullptr;
        }
#else
        auto pfnGetMemoryFdKHR = deviceProc<PFN_vkGetMemoryFdKHR>(m_vkDevice, "vkGetMemoryFdKHR");
        if (!pfnGetMemoryFdKHR)
        {
            cerr << "ERROR: VulkanView: vkGetMemoryFdKHR not found" << endl;
            cleanupSharedImage(slot);
            return nullptr;
        }

        VkMemoryGetFdInfoKHR getFdInfo = {};
        getFdInfo.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
        getFdInfo.memory = m_shared[slot].memory;
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

        if (vkCreateSemaphore(m_vkDevice, &semInfo, nullptr, &m_shared[slot].glReady) != VK_SUCCESS
            || vkCreateSemaphore(m_vkDevice, &semInfo, nullptr, &m_shared[slot].vkReady) != VK_SUCCESS)
        {
            cerr << "ERROR: VulkanView: Failed to create shared semaphores" << endl;
            cleanupSharedImage(slot);
            return nullptr;
        }

        // Export the GL<->Vulkan sync semaphores as external handles.
#ifdef PLATFORM_WINDOWS
        auto pfnGetSemaphoreWin32HandleKHR = deviceProc<PFN_vkGetSemaphoreWin32HandleKHR>(m_vkDevice, "vkGetSemaphoreWin32HandleKHR");
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

        getSemHandleInfo.semaphore = m_shared[slot].glReady;
        if (pfnGetSemaphoreWin32HandleKHR(m_vkDevice, &getSemHandleInfo, &glReadyHandle) != VK_SUCCESS || !glReadyHandle)
        {
            cerr << "ERROR: VulkanView: Failed to get glReady semaphore HANDLE" << endl;
            cleanupSharedImage(slot);
            return nullptr;
        }

        getSemHandleInfo.semaphore = m_shared[slot].vkReady;
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
        auto pfnGetSemaphoreFdKHR = deviceProc<PFN_vkGetSemaphoreFdKHR>(m_vkDevice, "vkGetSemaphoreFdKHR");
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

        getSemFdInfo.semaphore = m_shared[slot].glReady;
        if (pfnGetSemaphoreFdKHR(m_vkDevice, &getSemFdInfo, &glReadyFd) != VK_SUCCESS || glReadyFd < 0)
        {
            cerr << "ERROR: VulkanView: Failed to get glReady semaphore FD" << endl;
            cleanupSharedImage(slot);
            return nullptr;
        }

        getSemFdInfo.semaphore = m_shared[slot].vkReady;
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

        imageBarrier(cb, m_shared[slot].image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 0,
                     VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

        vkEndCommandBuffer(cb);

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cb;

        vkResetFences(m_vkDevice, 1, &m_frameSync[slot].fence);
        VkResult layoutSubmitResult = vkQueueSubmit(m_vkQueue, 1, &submitInfo, m_frameSync[slot].fence);
        if (layoutSubmitResult != VK_SUCCESS)
        {
            if (layoutSubmitResult == VK_ERROR_DEVICE_LOST)
            {
                requestGLFallback();
            }
            cleanupSharedImage(slot);
            return nullptr;
        }
        vkWaitForFences(m_vkDevice, 1, &m_frameSync[slot].fence, VK_TRUE, std::numeric_limits<uint64_t>::max());

        // Signal vkReady initially so GL can start writing to it
        VkSubmitInfo signalInfo = {};
        signalInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        signalInfo.signalSemaphoreCount = 1;
        signalInfo.pSignalSemaphores = &m_shared[slot].vkReady;
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
        m_shared[slot].capacityW = capW;
        m_shared[slot].capacityH = capH;

        return &info;
    }

    //--------------------------------------------------------------------------
    // presentSharedImage
    //--------------------------------------------------------------------------

    void VulkanView::presentSharedImage()
    {
        const uint32_t slot = m_currentFrame;
        const SharedImageInfo& info = m_shared[slot].info;

        if (!m_vkDevice || !m_shared[slot].image || !m_vkSwapchain)
        {
            return;
        }

        // Start-of-frame throttle: wait for this slot's previous frame to finish
        // before reusing its acquire semaphore and per-frame resources. This
        // replaces the old end-of-frame block; with FIFO acquire back-pressure it
        // is what paces the loop to display refresh while still allowing
        // FRAMES_IN_FLIGHT frames outstanding.
        vkWaitForFences(m_vkDevice, 1, &m_frameSync[slot].fence, VK_TRUE, std::numeric_limits<uint64_t>::max());

        // Acquire image
        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(m_vkDevice, m_vkSwapchain, std::numeric_limits<uint64_t>::max(),
                                                m_frameSync[slot].imageAvailable, VK_NULL_HANDLE, &imageIndex);
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
            vkWaitForFences(m_vkDevice, 1, &m_imagesInFlight[imageIndex], VK_TRUE, std::numeric_limits<uint64_t>::max());
        }
        m_imagesInFlight[imageIndex] = m_frameSync[slot].fence;

        // Reset the frame fence only now, right before the submit that re-signals it.
        vkResetFences(m_vkDevice, 1, &m_frameSync[slot].fence);

        VkCommandBuffer cb = m_vkCommandBuffers[imageIndex];
        vkResetCommandBuffer(cb, 0);

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cb, &beginInfo);

        // Transition shared image from COLOR_ATTACHMENT_OPTIMAL to TRANSFER_SRC_OPTIMAL
        imageBarrier(cb, m_shared[slot].image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 0,
                     VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

        // Transition swapchain image to transfer dst
        imageBarrier(cb, m_vkSwapchainImages[imageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0,
                     VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

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
            region.extent = {static_cast<uint32_t>(info.width), static_cast<uint32_t>(info.height), 1};

            vkCmdCopyImage(cb, m_shared[slot].image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_vkSwapchainImages[imageIndex],
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

            vkCmdBlitImage(cb, m_shared[slot].image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_vkSwapchainImages[imageIndex],
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_NEAREST);
        }

        // Transition swapchain image to present
        imageBarrier(cb, m_vkSwapchainImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                     VK_ACCESS_TRANSFER_WRITE_BIT, 0, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

        vkEndCommandBuffer(cb);

        // Submit
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        // Wait for GL to finish writing (glReady) AND swapchain image to be available
        const std::array<VkSemaphore, 2> waitSemaphores = {m_shared[slot].glReady, m_frameSync[slot].imageAvailable};
        const std::array<VkPipelineStageFlags, 2> waitStages = {VK_PIPELINE_STAGE_TRANSFER_BIT,
                                                                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
        submitInfo.pWaitSemaphores = waitSemaphores.data();
        submitInfo.pWaitDstStageMask = waitStages.data();

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cb;

        // Signal the image's renderFinished (present waits on it) AND vkReady (so
        // GL can write the next frame into this slot's shared image).
        const std::array<VkSemaphore, 2> signalSemaphores = {m_vkRenderFinished[imageIndex], m_shared[slot].vkReady};
        submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
        submitInfo.pSignalSemaphores = signalSemaphores.data();

        VkResult submitResult = vkQueueSubmit(m_vkQueue, 1, &submitInfo, m_frameSync[slot].fence);
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
        const std::array<VkSwapchainKHR, 1> swapchains = {m_vkSwapchain};
        presentInfo.swapchainCount = static_cast<uint32_t>(swapchains.size());
        presentInfo.pSwapchains = swapchains.data();
        presentInfo.pImageIndices = &imageIndex;

        VkResult presentResult = vkQueuePresentKHR(m_vkQueue, &presentInfo);
        // Recreate only on OUT_OF_DATE. VK_SUBOPTIMAL_KHR still presents fine and
        // can be reported persistently by some X11/RADV compositors; recreating
        // on it every frame caused a swapchain-recreate loop that starved the Qt
        // event loop (dead input, no fullscreen). Real resizes report OUT_OF_DATE.
        // The submit above is tracked by m_frameSync[slot].fence (waited at the start of
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
        {
            return;
        }

        if (!m_vkSwapchain || m_vkSwapchainExtent.width != static_cast<uint32_t>(w)
            || m_vkSwapchainExtent.height != static_cast<uint32_t>(h))
        {
            // Warm recreate via oldSwapchain (createSwapchain retires the old one).
            if (!createSwapchain())
            {
                return;
            }
        }

        // Start-of-frame throttle (matches presentSharedImage): wait for this
        // slot's previous frame to finish before reusing its staging buffer,
        // acquire semaphore and command resources.
        vkWaitForFences(m_vkDevice, 1, &m_frameSync[slot].fence, VK_TRUE, std::numeric_limits<uint64_t>::max());

        size_t size = w * h * 4;

        // Recreate this slot's staging buffer if needed
        if (size > m_staging[slot].size)
        {
            if (m_staging[slot].buffer)
            {
                vkDestroyBuffer(m_vkDevice, m_staging[slot].buffer, nullptr);
            }
            if (m_staging[slot].memory)
            {
                vkFreeMemory(m_vkDevice, m_staging[slot].memory, nullptr);
            }

            VkBufferCreateInfo bufferInfo = {};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = size;
            bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            vkCreateBuffer(m_vkDevice, &bufferInfo, nullptr, &m_staging[slot].buffer);

            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(m_vkDevice, m_staging[slot].buffer, &memRequirements);

            VkMemoryAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            const std::optional<uint32_t> memoryTypeIndex =
                findMemoryType(m_vkPhysicalDevice, memRequirements.memoryTypeBits,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            if (!memoryTypeIndex)
            {
                cerr << "ERROR: VulkanView: No host-visible memory type for staging buffer" << endl;
                return;
            }
            allocInfo.memoryTypeIndex = *memoryTypeIndex;

            vkAllocateMemory(m_vkDevice, &allocInfo, nullptr, &m_staging[slot].memory);
            vkBindBufferMemory(m_vkDevice, m_staging[slot].buffer, m_staging[slot].memory, 0);

            m_staging[slot].size = size;
        }

        // Copy to staging buffer
        void* data;
        vkMapMemory(m_vkDevice, m_staging[slot].memory, 0, size, 0, &data);
        memcpy(data, pixels, size);
        vkUnmapMemory(m_vkDevice, m_staging[slot].memory);

        // Acquire image
        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(m_vkDevice, m_vkSwapchain, std::numeric_limits<uint64_t>::max(),
                                                m_frameSync[slot].imageAvailable, VK_NULL_HANDLE, &imageIndex);
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
            vkWaitForFences(m_vkDevice, 1, &m_imagesInFlight[imageIndex], VK_TRUE, std::numeric_limits<uint64_t>::max());
        }
        m_imagesInFlight[imageIndex] = m_frameSync[slot].fence;

        vkResetFences(m_vkDevice, 1, &m_frameSync[slot].fence);

        VkCommandBuffer cb = m_vkCommandBuffers[imageIndex];
        vkResetCommandBuffer(cb, 0);

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cb, &beginInfo);

        // Transition image to transfer dst
        imageBarrier(cb, m_vkSwapchainImages[imageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0,
                     VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

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
        region.imageExtent = {static_cast<uint32_t>(w), static_cast<uint32_t>(h), 1};

        vkCmdCopyBufferToImage(cb, m_staging[slot].buffer, m_vkSwapchainImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                               &region);

        // Transition image to present
        imageBarrier(cb, m_vkSwapchainImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                     VK_ACCESS_TRANSFER_WRITE_BIT, 0, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

        vkEndCommandBuffer(cb);

        // Submit
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        const std::array<VkSemaphore, 1> waitSemaphores = {m_frameSync[slot].imageAvailable};
        const std::array<VkPipelineStageFlags, 1> waitStages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
        submitInfo.pWaitSemaphores = waitSemaphores.data();
        submitInfo.pWaitDstStageMask = waitStages.data();
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cb;
        const std::array<VkSemaphore, 1> signalSemaphores = {m_vkRenderFinished[imageIndex]};
        submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
        submitInfo.pSignalSemaphores = signalSemaphores.data();

        VkResult submitResult = vkQueueSubmit(m_vkQueue, 1, &submitInfo, m_frameSync[slot].fence);
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
        presentInfo.pWaitSemaphores = signalSemaphores.data();
        const std::array<VkSwapchainKHR, 1> swapchains = {m_vkSwapchain};
        presentInfo.swapchainCount = static_cast<uint32_t>(swapchains.size());
        presentInfo.pSwapchains = swapchains.data();
        presentInfo.pImageIndices = &imageIndex;

        VkResult presentResult = vkQueuePresentKHR(m_vkQueue, &presentInfo);
        // Recreate only on OUT_OF_DATE. VK_SUBOPTIMAL_KHR still presents fine and
        // can be reported persistently by some X11/RADV compositors; recreating
        // on it every frame caused a swapchain-recreate loop that starved the Qt
        // event loop (dead input, no fullscreen). Real resizes report OUT_OF_DATE.
        // The submit is tracked by m_frameSync[slot].fence (waited at the next use of this
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
                    TwkApp::ActivityChangeEvent aevent("user-inactive", m_videoDevice.get());
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
                TwkApp::ActivityChangeEvent aevent("user-active", m_videoDevice.get());
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
        // Our own native window: only track its Vulkan surface lifetime and
        // let every other event through untouched.
        if (object == windowHandle())
        {
            if (event->type() == QEvent::PlatformSurface
                && static_cast<QPlatformSurfaceEvent*>(event)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed)
            {
                // Destroying a swapchain after its surface is gone crashes in
                // the driver (seen on NVIDIA when closing RV), so release it
                // now. createSwapchain() re-acquires the surface if the native
                // window is recreated.
                cleanupSwapchain();
                m_vkSurface = VK_NULL_HANDLE;
            }
            return false;
        }

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
