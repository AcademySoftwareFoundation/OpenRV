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
    //  A QWidget subclass that presents 10-bit Vulkan images on Linux.
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

        //
        //  Vulkan presentation — called by QTVulkanVideoDevice::syncBuffers().
        //
        //  pixels: packed A2B10G10R10 
        //
        void presentPixelData(const void* pixels, int w, int h);

        bool isInitialized() const { return m_initialized; }

    public slots:
        void eventProcessingTimeout();

    protected:
        //  Called once when the widget is first shown.
        void initialize();

        //  Called each time a new frame should be rendered.
        void render();

        void showEvent(QShowEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;

    private:
        bool initVulkan();
        void cleanupVulkan();
        bool createSwapchain();
        void cleanupSwapchain();

        RvDocument* m_doc;
        QTVulkanVideoDevice* m_videoDevice;

        bool m_initialized;
        bool m_firstPaintCompleted;
        bool m_postFirstNonEmptyRender;
        bool m_stopProcessingEvents;
        bool m_userActive;

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
        VkFormat m_vkSwapchainFormat;
        VkExtent2D m_vkSwapchainExtent;
        std::vector<VkImage> m_vkSwapchainImages;
        std::vector<VkCommandBuffer> m_vkCommandBuffers;
        
        VkSemaphore m_vkImageAvailableSemaphore{VK_NULL_HANDLE};
        VkSemaphore m_vkRenderFinishedSemaphore{VK_NULL_HANDLE};
        VkFence m_vkFence{VK_NULL_HANDLE};

        VkBuffer m_vkStagingBuffer{VK_NULL_HANDLE};
        VkDeviceMemory m_vkStagingBufferMemory{VK_NULL_HANDLE};
        size_t m_stagingBufferSize{0};
    };

} // namespace Rv