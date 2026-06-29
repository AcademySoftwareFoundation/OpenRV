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

        // Format of the active swapchain image (e.g. VK_FORMAT_A2B10G10R10_UNORM_PACK32).
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
            void* memoryHandle;             // HANDLE; nullptr when unset
            void* glReadySemaphoreHandle;
            void* vkReadySemaphoreHandle;
#else
            int memoryFd;                   // -1 when unset
            int glReadySemaphoreFd;
            int vkReadySemaphoreFd;
#endif
            size_t size;
            int width;
            int height;
            int strideWidth;
        };

        const SharedImageInfo* getSharedImageInfo(int w, int h);
        void presentSharedImage();

        // CPU fallback API (not used when GPU interop is active)
        void presentPixelData(const void* pixels, int w, int h);

        bool isInitialized() const { return m_initialized; }

        //
        //  Surface-independent probe for whether this machine's Vulkan can
        //  present A2B10G10R10 (the format used by GL_RGB10_A2 interop). Used at
        //  RvDocument construction time to decide whether a 10-bit display
        //  request should route to the Vulkan path or fall back to OpenGL.
        //  Creates a throwaway QVulkanInstance and queries format support; it
        //  does not require a window/surface and never throws — returns false
        //  if Vulkan is unavailable for any reason.
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

        VkSemaphore m_vkImageAvailableSemaphore{VK_NULL_HANDLE};
        VkSemaphore m_vkRenderFinishedSemaphore{VK_NULL_HANDLE};
        VkFence m_vkFence{VK_NULL_HANDLE};

        VkBuffer m_vkStagingBuffer{VK_NULL_HANDLE};
        VkDeviceMemory m_vkStagingBufferMemory{VK_NULL_HANDLE};
        size_t m_stagingBufferSize{0};

        // Shared Image for GPU Interop
        VkImage m_vkSharedImage{VK_NULL_HANDLE};
        VkDeviceMemory m_vkSharedImageMemory{VK_NULL_HANDLE};
        VkSemaphore m_vkGlReadySemaphore{VK_NULL_HANDLE};
        VkSemaphore m_vkVkReadySemaphore{VK_NULL_HANDLE};
#ifdef PLATFORM_WINDOWS
        SharedImageInfo m_sharedImageInfo{nullptr, nullptr, nullptr, 0, 0, 0, 0};
#else
        SharedImageInfo m_sharedImageInfo{-1, -1, -1, 0, 0, 0, 0};
#endif

        void cleanupSharedImage();

        // Recreate swapchain (and shared image) after OUT_OF_DATE / SUBOPTIMAL.
        void handleSwapchainOutOfDate();

        // Queue a one-shot switch to GLView; no-op during shutdown.
        void requestGLFallback();

        // False while closing or when the widget has no drawable size.
        bool presentationAllowed() const;

        bool m_glFallbackRequested{false};
    };

} // namespace Rv
