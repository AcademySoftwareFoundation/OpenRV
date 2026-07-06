//
//  Copyright (c) 2026 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
#pragma once

#include <QWidget>
#include <QtCore/QSize>
#include <QtCore/QEvent>
#include <QtCore/QTimer>
#include <TwkUtil/Timer.h>

#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>

namespace Rv
{
    class RvDocument;
    class QTVulkanVideoDevice;

    //
    //  VulkanView
    //
    //  A QWidget subclass that presents 10-bit Vulkan images on Linux and Windows.
    //  All IPCore image processing runs in OpenGL via a separate
    //  QOpenGLContext+QOffscreenSurface; the Vulkan path is used only for
    //  final 10-bit pixel delivery to avoid 8-bit GLX visual truncation.
    //
    class VulkanView : public QWidget
    {
        Q_OBJECT

    public:
        typedef TwkUtil::Timer Timer;

        explicit VulkanView(RvDocument* doc, QWidget* parent = nullptr, bool noResize = true);
        ~VulkanView();

        QTVulkanVideoDevice* videoDevice() const { return m_videoDevice; }

        void setEventWidget(QWidget* widget);

        void stopProcessingEvents();

        virtual bool event(QEvent* event) override;
        virtual bool eventFilter(QObject* object, QEvent* event) override;

        bool firstPaintCompleted() const { return m_firstPaintCompleted; }

        void setContentSize(int w, int h) { m_csize = QSize(w, h); }

        void setMinimumContentSize(int w, int h) { m_msize = QSize(w, h); }

        QSize sizeHint() const override { return m_csize; }

        QSize minimumSizeHint() const override { return m_msize; }

        void absolutePosition(int& x, int& y) const;

        float devicePixelRatio() const;

        // Format of the active swapchain image: VK_FORMAT_A2B10G10R10_UNORM_PACK32
        // or VK_FORMAT_A2R10G10B10_UNORM_PACK32 (the two differ in R/B order).
        // VK_FORMAT_UNDEFINED before the swapchain is created.
        VkFormat swapchainFormat() const { return m_vkSwapchainFormat; }

        //
        //  Vulkan presentation — called by QTVulkanVideoDevice::syncBuffers().
        //

        //  GPU Interop API
        //
        //  External handles to a Vulkan device-memory block (and its
        //  GL<->Vulkan sync semaphores) that GL imports as a memory
        //  object + semaphores. On Linux these are opaque file
        //  descriptors; on Windows they are Win32 HANDLEs. Stored as
        //  void* in the header to keep <windows.h> out of public Qt
        //  includes; the .cpp casts to HANDLE.
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
        };

        // Number of frames the present path keeps in flight. Per-frame Vulkan
        // sync objects and the GL<->Vulkan shared resources are stored in rings
        // of this size and indexed by currentFrame(). 2 pipelines the present so
        // a frame's GL work + submit can begin before the prior present retires;
        // the throttle is FIFO acquire back-pressure + the start-of-frame fence
        // wait (no per-frame end-of-frame block).
        static constexpr uint32_t FRAMES_IN_FLIGHT = 2;

        // Index of the in-flight ring slot the next/current frame uses. The GL
        // side (QTVulkanVideoDevice) reads this to pair its own ring objects with
        // the Vulkan slot for the frame being rendered.
        uint32_t currentFrame() const { return m_currentFrame; }

        const SharedImageInfo* getSharedImageInfo(int w, int h);
        void presentSharedImage();

        // CPU fallback API (not used when GPU interop is active)
        void presentPixelData(const void* pixels, int w, int h);

        bool isInitialized() const { return m_initialized; }

        //
        //  Probe for whether this machine's Vulkan can present a 10-bit format
        //  (A2B10G10R10 or A2R10G10B10). Used at RvDocument construction time to
        //  decide whether a 10-bit display request should route to the Vulkan
        //  path or fall back to OpenGL. Creates a throwaway QVulkanInstance +
        //  dummy surface and queries the advertised surface formats; it never
        //  throws — returns false if Vulkan is unavailable for any reason.
        //
        static bool supports10BitPresentation();

    public slots:
        void eventProcessingTimeout();

    protected:
        //  Called once when the widget is first shown.
        void initialize();

        //  Called each time a new frame should be rendered.
        void render();

        void showEvent(QShowEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

        QPaintEngine* paintEngine() const override { return nullptr; }

    private:
        bool initVulkan();
        void cleanupVulkan();
        bool createSwapchain();
        void cleanupSwapchain();

        // Post a coalesced UpdateRequest: at most one render is queued at a time,
        // so a burst of resize events collapses to a single render at the latest
        // size instead of one heavy swapchain recreate per event.
        void requestUpdate();

        RvDocument* m_doc;
        QTVulkanVideoDevice* m_videoDevice;

        bool m_initialized;
        bool m_firstPaintCompleted;
        bool m_postFirstNonEmptyRender;
        bool m_stopProcessingEvents;
        bool m_userActive;
        bool m_updatePending{false};

        QSize m_csize;
        QSize m_msize;
        QWidget* m_eventWidget;

        unsigned int m_lastKey;
        QEvent::Type m_lastKeyType;
        Timer m_activityTimer;
        Timer m_activationTimer;
        QTimer m_eventProcessingTimer;

        // Vulkan state
        VkInstance m_vkInstance{VK_NULL_HANDLE};
        VkSurfaceKHR m_vkSurface{VK_NULL_HANDLE};
        VkPhysicalDevice m_vkPhysicalDevice{VK_NULL_HANDLE};
        VkDevice m_vkDevice{VK_NULL_HANDLE};
        VkQueue m_vkQueue{VK_NULL_HANDLE};
        uint32_t m_queueFamilyIndex{0};
        VkCommandPool m_vkCommandPool{VK_NULL_HANDLE};

        VkSwapchainKHR m_vkSwapchain{VK_NULL_HANDLE};
        VkFormat m_vkSwapchainFormat{VK_FORMAT_UNDEFINED};
        VkExtent2D m_vkSwapchainExtent{};
        std::vector<VkImage> m_vkSwapchainImages;
        std::vector<VkCommandBuffer> m_vkCommandBuffers;

        // Per-in-flight-slot ring (indexed by m_currentFrame).
        std::array<VkSemaphore, FRAMES_IN_FLIGHT> m_vkImageAvailableSemaphore{};
        std::array<VkFence, FRAMES_IN_FLIGHT> m_vkFence{};
        uint32_t m_currentFrame{0};

        // Per-swapchain-image (indexed by imageIndex, sized to the swapchain
        // image count, (re)built in createSwapchain / freed in cleanupSwapchain).
        // The present-wait semaphore MUST be tied to the image, not the frame:
        // with 2 frames in flight the same image can be re-acquired while its
        // prior present is still pending, and reusing a per-frame semaphore there
        // trips the present-semaphore-reuse validation error. m_imagesInFlight
        // records which frame fence currently owns each image so a re-acquired
        // in-flight image is waited on before reuse.
        std::vector<VkSemaphore> m_vkRenderFinished;
        std::vector<VkFence> m_imagesInFlight;

        // CPU-fallback staging buffer, ringed per in-flight slot: the frame maps
        // and overwrites it before acquiring, so with the per-frame block removed
        // it must not alias a buffer whose copy from a still-in-flight frame is
        // pending. The slot's frame fence (waited at frame start) gates reuse.
        std::array<VkBuffer, FRAMES_IN_FLIGHT> m_vkStagingBuffer{};
        std::array<VkDeviceMemory, FRAMES_IN_FLIGHT> m_vkStagingBufferMemory{};
        std::array<size_t, FRAMES_IN_FLIGHT> m_stagingBufferSize{};

        // Shared Image for GPU Interop, ringed per in-flight slot (indexed by
        // m_currentFrame). SharedImageInfo's default member initializers give the
        // correct unset state (FDs/handles = -1/nullptr), so value-initializing
        // the array is safe.
        std::array<VkImage, FRAMES_IN_FLIGHT> m_vkSharedImage{};
        std::array<VkDeviceMemory, FRAMES_IN_FLIGHT> m_vkSharedImageMemory{};
        std::array<VkSemaphore, FRAMES_IN_FLIGHT> m_vkGlReadySemaphore{};
        std::array<VkSemaphore, FRAMES_IN_FLIGHT> m_vkVkReadySemaphore{};
        std::array<SharedImageInfo, FRAMES_IN_FLIGHT> m_sharedImageInfo{};

        // Grow-only allocated capacity of each slot's shared image. A resize
        // within capacity reuses the existing allocation/export (no rebuild, no
        // FD re-export, no GL re-import); the image is only reallocated when the
        // request exceeds capacity, at which point capacity grows to the
        // componentwise max of the request and the screen size (monotonic).
        std::array<int, FRAMES_IN_FLIGHT> m_sharedCapacityW{};
        std::array<int, FRAMES_IN_FLIGHT> m_sharedCapacityH{};

        void cleanupSharedImage(uint32_t slot);

        // Rebalance a slot's glReady/vkReady binary-semaphore pair when a frame is
        // aborted at acquire time. The GL side (syncBuffers) has already signaled
        // glReady[slot] and waited vkReady[slot] before the acquire result is
        // known; if the frame returns without its normal submit, this issues a
        // minimal submit that waits glReady[slot] and signals vkReady[slot] so the
        // pair cannot desync across the skipped frame.
        void drainSharedSemaphores(uint32_t slot);

        // Recreate swapchain (and shared image) after OUT_OF_DATE / SUBOPTIMAL.
        void handleSwapchainOutOfDate();

        // Queue a one-shot switch to GLView; no-op during shutdown.
        void requestGLFallback();

        // False while closing or when the widget has no drawable size.
        bool presentationAllowed() const;

        bool m_glFallbackRequested{false};
    };

} // namespace Rv
