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
#include <string>
#include <vector>

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

        bool event(QEvent* event) override;
        bool eventFilter(QObject* object, QEvent* event) override;

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
        //  Presentation path taken this session. RV prefers ZeroCopy, degrades
        //  to CpuReadback (slower, still 10-bit), and only then to OpenGL
        //  (which forgoes 10-bit). See emitPresentationRecord().
        //
        enum class PresentPath
        {
            Undetermined,
            ZeroCopy,    // GL renders straight into a Vulkan-exported image
            CpuReadback, // GL packs RGB10_A2 to host memory, Vulkan uploads it
            OpenGL       // Vulkan abandoned; RvDocument swaps in GLView
        };

        //  Resolved GL<->Vulkan interop configuration. Negotiated once per
        //  device from what the driver reports exportable, never from GPU
        //  vendor identity or host platform. Both the Vulkan export and the GL
        //  import read their settings from this one struct so the two sides
        //  cannot disagree -- a disagreement about tiling or dedicated
        //  allocation corrupts the image rather than raising an error.
        struct InteropConfig
        {
            bool supported{false};
            VkFormat format{VK_FORMAT_A2B10G10R10_UNORM_PACK32};
            VkImageTiling tiling{VK_IMAGE_TILING_LINEAR};
            VkImageUsageFlags usage{0};

            // Probe-time floor for dedicated allocation: true when the handle
            // type reports DEDICATED_ONLY, which is a hard requirement. The
            // softer "prefers dedicated" signal belongs to a concrete image
            // rather than to the format, so it is read per-image from
            // VkMemoryDedicatedRequirements at allocation time and recorded in
            // SharedImageInfo::dedicatedAllocation, which is what the GL side
            // mirrors.
            bool dedicatedAllocation{false};

            // Raw VkExternalMemoryFeatureFlags the winning candidate reported,
            // so a log read by someone without the machine can tell whether
            // dedicated allocation was required by the handle type or merely
            // preferred by the image.
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

        // Record the path actually taken, and the GL side's view of the shared
        // image, then emit the one-per-session startup record. Called by
        // QTVulkanVideoDevice once the first frame establishes which path ran.
        void reportPresentPath(PresentPath path, const std::string& reason);
        void reportGLImportState(VkImageTiling tiling, bool dedicated);

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

            // The negotiated tiling this image was actually created with. GL
            // must import with the matching GL_{OPTIMAL,LINEAR}_TILING_EXT:
            // importing OPTIMAL-tiled memory as LINEAR yields an image whose
            // large-scale structure survives but whose pixels are scrambled
            // within each tile.
            VkImageTiling tiling{VK_IMAGE_TILING_LINEAR};

            // Whether the export used a dedicated allocation. GL must set
            // GL_DEDICATED_MEMORY_OBJECT_EXT to exactly this before
            // glTexStorageMem2DEXT; a mismatch corrupts the image.
            bool dedicatedAllocation{false};
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

        // Probe the driver for an exportable shared-image configuration and
        // resolve m_interopConfig. Runs exactly once per device, at device
        // creation -- not per shared-image slot and not again on resize.
        void negotiateInteropConfig();

        // Emit the one-per-session startup record describing the negotiated
        // configuration and the path taken. Unconditional: it must not be
        // gated on ImageRenderer::debugGpu(), because Windows/NVIDIA is
        // verified by QA against a build, and a log that needs a debug flag
        // set in advance costs a whole verification round.
        void emitPresentationRecord();

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

        // Negotiated interop configuration and the state behind the startup
        // record. m_interopNegotiated guards the once-per-device probe;
        // m_recordEmitted guards the once-per-session record.
        InteropConfig m_interopConfig;
        bool m_interopNegotiated{false};
        bool m_recordEmitted{false};

        PresentPath m_presentPath{PresentPath::Undetermined};
        std::string m_presentPathReason;
        VkColorSpaceKHR m_vkSwapchainColorSpace{VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

        // What the GL side reported importing, so the record can show the two
        // sides agreeing (or not) rather than only what Vulkan intended.
        bool m_glImportReported{false};
        VkImageTiling m_glImportTiling{VK_IMAGE_TILING_LINEAR};
        bool m_glImportDedicated{false};
    };

} // namespace Rv
