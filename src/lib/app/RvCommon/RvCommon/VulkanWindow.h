//
//  Copyright (c) 2026 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
#pragma once

#include <QtGui/QWindow>
#include <QtCore/QEvent>
#include <QtCore/QTimer>
#include <TwkUtil/Timer.h>

#include <vulkan/vulkan.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

QT_BEGIN_NAMESPACE
class QPlatformWindow;
QT_END_NAMESPACE

namespace Rv
{
    class RvDocument;
    class QTVulkanVideoDevice;

    //
    //  VulkanWindow
    //
    //  The RV viewport as a native Vulkan surface, presenting 10-bit images on
    //  Linux and Windows. IPCore still renders in OpenGL (see
    //  QTVulkanVideoDevice); Vulkan only delivers the final pixels.
    //
    //  Embedded by VulkanView via QWidget::createWindowContainer(), like
    //  GLWindow/GLView, which keeps the QMainWindow off Qt's render-to-texture
    //  composite path.
    //
    class VulkanWindow : public QWindow
    {
        Q_OBJECT

    public:
        typedef TwkUtil::Timer Timer;

        explicit VulkanWindow(RvDocument* doc, bool noResize = true);
        ~VulkanWindow();

        QTVulkanVideoDevice* videoDevice() const { return m_videoDevice; }

        //  Owned by the hosting VulkanView, not by this window.
        void setVideoDevice(QTVulkanVideoDevice* d) { m_videoDevice = d; }

        //  The container QWidget; used for focus and the popup check in render().
        void setEventWidget(QWidget* widget) { m_eventWidget = widget; }

        void stopProcessingEvents();

        bool event(QEvent* event) override;

        bool firstPaintCompleted() const { return m_firstPaintCompleted; }

        void absolutePosition(int& x, int& y) const;

        float devicePixelRatioF() const;

        // A2B10G10R10 or A2R10G10B10 (R/B order differs); UNDEFINED before
        // the swapchain exists.
        VkFormat swapchainFormat() const { return m_vkSwapchainFormat; }

        //  Presentation path taken, in order of preference.
        enum class PresentPath
        {
            Undetermined,
            ZeroCopy,    // GL renders straight into a Vulkan-exported image
            CpuReadback, // GL packs RGB10_A2 to host memory, Vulkan uploads it
            OpenGL       // Vulkan abandoned; RvDocument swaps in GLView
        };

        //  GL<->Vulkan interop configuration, negotiated once per device from
        //  what the driver reports exportable. Both the Vulkan export and the
        //  GL import read it: a tiling or dedicated-allocation mismatch
        //  silently corrupts the image.
        struct InteropConfig
        {
            bool supported{false};
            VkFormat format{VK_FORMAT_A2B10G10R10_UNORM_PACK32};
            VkImageTiling tiling{VK_IMAGE_TILING_LINEAR};
            VkImageUsageFlags usage{0};

            // True when the handle type reports DEDICATED_ONLY. The per-image
            // "prefers dedicated" result lives in
            // SharedImageInfo::dedicatedAllocation.
            bool dedicatedAllocation{false};

            // Raw flags of the winning candidate, for the startup record.
            VkExternalMemoryFeatureFlags externalFeatures{0};

            // Set when an RV_VULKAN_FORCE_* override displaced what the probe
            // would otherwise have chosen; the record reports both values.
            bool tilingOverridden{false};
            bool dedicatedOverridden{false};
            VkImageTiling probedTiling{VK_IMAGE_TILING_LINEAR};
            bool probedDedicated{false};

            // Why no candidate was usable (empty when supported is true).
            std::string rejectReason;
            // Per-candidate probe outcome, one entry per candidate tried.
            std::vector<std::string> candidateLog;
        };

        const InteropConfig& interopConfig() const { return m_interopConfig; }

        // Called by QTVulkanVideoDevice once the first frame establishes the
        // path; emits the startup record.
        void reportPresentPath(PresentPath path, const std::string& reason);
        void reportGLImportState(VkImageTiling tiling, bool dedicated);

        // Lets the GL bridge refuse external-memory interop across GPUs.
        bool physicalDeviceMatchesUUID(const unsigned char* uuid, size_t size) const;

        //  External handles GL imports as a memory object and semaphores:
        //  opaque FDs on Linux, Win32 HANDLEs on Windows (void* to keep
        //  <windows.h> out of this header).
        struct SharedImageInfo
        {
#ifdef PLATFORM_WINDOWS
            void* memoryHandle{nullptr}; // HANDLE; nullptr when unset
            void* glReadySemaphoreHandle{nullptr};
            void* vkReadySemaphoreHandle{nullptr};
#else
            int memoryFd{-1}; // -1 when unset
            int glReadySemaphoreFd{-1};
            int vkReadySemaphoreFd{-1};
#endif
            size_t size{0};
            int width{0};          // used sub-region width presented this frame
            int height{0};         // used sub-region height presented this frame
            int strideWidth{0};    // GL texture width = capacity rowPitch / 4
            int capacityHeight{0}; // allocated image height (>= height); GL texture height
            // GL must import with the matching GL_{OPTIMAL,LINEAR}_TILING_EXT.
            VkImageTiling tiling{VK_IMAGE_TILING_LINEAR};

            // GL must set GL_DEDICATED_MEMORY_OBJECT_EXT to exactly this.
            bool dedicatedAllocation{false};
        };

        // Ring capacity. Runtime depth defaults to 1 for latency;
        // RV_VULKAN_MAX_FRAMES_IN_FLIGHT=2 uses both slots.
        static constexpr uint32_t kFramesInFlight = 2;

        // Ring slot for the current frame; the GL side pairs its objects to it.
        uint32_t currentFrame() const { return m_currentFrame; }

        // A doc-less window is a passive presentation output, presented by its
        // VulkanDesktopVideoDevice. It never drives or blocks the frame loop.
        bool isPassiveOutput() const { return m_doc == nullptr; }

        //  Checked before any GL work. False means skip the frame (a retry is
        //  armed). Always true for the control viewport.
        bool canPresentNow();

        const SharedImageInfo* getSharedImageInfo(int w, int h);
        void presentSharedImage();

        // CPU fallback API (not used when GPU interop is active)
        void presentPixelData(const void* pixels, int w, int h);

        bool isInitialized() const { return m_initialized; }

        //  Whether Vulkan can present a 10-bit format here. Never throws.
        static bool supports10BitPresentation();

    public slots:
        void eventProcessingTimeout();

    protected:
        void initialize();
        void render();

        void exposeEvent(QExposeEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;

    private:
        bool initVulkan();
        void cleanupVulkan();
        void requestBestEffortRetry();
        bool createSwapchain();
        void cleanupSwapchain();

        // Resolves m_interopConfig once per device (not per slot or resize).
        void negotiateInteropConfig();

        // Once per session. Unconditional: needed in logs without -debug gpu.
        void emitPresentationRecord();

        RvDocument* m_doc;
        QTVulkanVideoDevice* m_videoDevice;

        bool m_initialized;

        // Platform window the VkSurfaceKHR was created against; see
        // handleSurfaceLost().
        const QPlatformWindow* m_initializedHandle{nullptr};

        bool m_firstPaintCompleted;
        bool m_postFirstNonEmptyRender;
        bool m_stopProcessingEvents;
        bool m_userActive;

        QWidget* m_eventWidget;

        unsigned int m_lastKey;
        QEvent::Type m_lastKeyType;
        Timer m_activityTimer;
        Timer m_activationTimer;
        //  Forward-progress guard for canPresentNow().
        Timer m_lastPresentTimer;
        QTimer m_eventProcessingTimer;

        VkInstance m_vkInstance{VK_NULL_HANDLE};
        VkSurfaceKHR m_vkSurface{VK_NULL_HANDLE};
        VkPhysicalDevice m_vkPhysicalDevice{VK_NULL_HANDLE};
        VkDevice m_vkDevice{VK_NULL_HANDLE};
        VkQueue m_vkQueue{VK_NULL_HANDLE};
        uint32_t m_queueFamilyIndex{0};
        //  Logged only on change: createSwapchain() runs on every resize.
        VkSurfaceFormatKHR m_loggedSurfaceFormat{};

        bool m_loggedSurfaceFormatList{false};

        bool m_externalInteropSupported{false};
        VkCommandPool m_vkCommandPool{VK_NULL_HANDLE};

        VkSwapchainKHR m_vkSwapchain{VK_NULL_HANDLE};
        VkFormat m_vkSwapchainFormat{VK_FORMAT_UNDEFINED};
        VkExtent2D m_vkSwapchainExtent{};
        std::vector<VkImage> m_vkSwapchainImages;
        std::vector<VkCommandBuffer> m_vkCommandBuffers;

        // Per-in-flight-slot ring (indexed by m_currentFrame).
        std::array<VkSemaphore, kFramesInFlight> m_vkImageAvailableSemaphore{};
        std::array<VkFence, kFramesInFlight> m_vkFence{};
        uint32_t m_currentFrame{0};

        // Per swapchain image. The present-wait semaphore must be per image,
        // not per frame: an image can be re-acquired while its present is still
        // pending. m_imagesInFlight holds the frame fence owning each image.
        std::vector<VkSemaphore> m_vkRenderFinished;
        std::vector<VkFence> m_imagesInFlight;

        // CPU-fallback staging buffer, per slot so a frame never overwrites a
        // buffer an in-flight copy still reads; the slot fence gates reuse.
        std::array<VkBuffer, kFramesInFlight> m_vkStagingBuffer{};
        std::array<VkDeviceMemory, kFramesInFlight> m_vkStagingBufferMemory{};
        std::array<size_t, kFramesInFlight> m_stagingBufferSize{};

        // Shared interop image, per slot.
        std::array<VkImage, kFramesInFlight> m_vkSharedImage{};
        std::array<VkDeviceMemory, kFramesInFlight> m_vkSharedImageMemory{};
        std::array<VkSemaphore, kFramesInFlight> m_vkGlReadySemaphore{};
        std::array<VkSemaphore, kFramesInFlight> m_vkVkReadySemaphore{};
        std::array<SharedImageInfo, kFramesInFlight> m_sharedImageInfo{};

        // Grow-only capacity: a resize within it reuses the export and the GL
        // import.
        std::array<int, kFramesInFlight> m_sharedCapacityW{};
        std::array<int, kFramesInFlight> m_sharedCapacityH{};

        void cleanupSharedImage(uint32_t slot);

        // GL has already signaled glReady and waited vkReady when an acquire
        // fails; this minimal submit keeps the semaphore pair balanced.
        void drainSharedSemaphores(uint32_t slot);

        // Recreate the swapchain after OUT_OF_DATE. SUBOPTIMAL remains usable.
        void handleSwapchainOutOfDate();

        // Tear down and re-initialize after Qt destroyed and recreated the
        // platform window, which invalidates the VkSurfaceKHR.
        void handleSurfaceLost();

        // Must run while the platform window (and the VkSurfaceKHR) is still
        // alive; see the QEvent::PlatformSurface handler in event().
        void releaseVulkanResources();

        // Queue a one-shot switch to GLView; no-op during shutdown.
        void requestGLFallback();

        // False while closing or when the widget has no drawable size.
        bool presentationAllowed() const;

        bool m_glFallbackRequested{false};

        InteropConfig m_interopConfig;
        bool m_interopNegotiated{false};
        bool m_recordEmitted{false};

        PresentPath m_presentPath{PresentPath::Undetermined};
        std::string m_presentPathReason;

        // What GL reported importing, so the record shows both sides.
        bool m_glImportReported{false};
        VkImageTiling m_glImportTiling{VK_IMAGE_TILING_LINEAR};
        bool m_glImportDedicated{false};
    };

} // namespace Rv
