//
//  Copyright (c) 2026 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
#pragma once

#include <TwkGLF/GLVideoDevice.h>
#include <RvCommon/QTTranslator.h>
#include <RvCommon/VulkanView.h>
#include <array>
#include <cstdint>
#include <string>
#include <memory>
#include <vector>

QT_BEGIN_NAMESPACE
class QOpenGLContext;
class QOffscreenSurface;
class QWidget;
QT_END_NAMESPACE

namespace Rv
{
    class VulkanView;

    //
    //  QTVulkanVideoDevice
    //
    //  Wraps a VulkanView as a TwkGLF::GLVideoDevice so that ImageRenderer's
    //  existing GL rendering pipeline (renderMain, shader cache, etc.) can run
    //  unchanged on the Vulkan presentation path.
    //
    class QTVulkanVideoDevice : public TwkGLF::GLVideoDevice
    {
    public:
        QTVulkanVideoDevice(TwkApp::VideoModule* module, const std::string& name, VulkanView* view, QWidget* eventWidget);
        virtual ~QTVulkanVideoDevice();

        VulkanView* vulkanView() const { return m_view; }

        QWidget* eventWidget() const { return m_eventWidget; }

        void setEventWidget(QWidget* widget);

        const QTTranslator& translator() const { return *m_translator; }

        bool hasTranslator() const { return m_translator != nullptr; }

        void setAbsolutePosition(int x, int y);

        // VideoDevice API
        virtual void makeCurrent() const override;
        virtual void syncBuffers() const override;
        virtual void redraw() const override;
        virtual void redrawImmediately() const override;
        virtual void clearCaches() const override;

        virtual Resolution resolution() const override;
        virtual Offset offset() const override;
        virtual Timing timing() const override;
        virtual VideoFormat format() const override;

        virtual size_t width() const override;
        virtual size_t height() const override;

        virtual void open(const StringVector& args) override;
        virtual void close() override;
        virtual bool isOpen() const override;

        virtual float devicePixelRatio() const override;

        virtual void setPhysicalDevice(VideoDevice* d) override;

        // GLVideoDevice API
        virtual TwkGLF::GLFBO* defaultFBO() override;
        virtual const TwkGLF::GLFBO* defaultFBO() const override;
        virtual std::string hardwareIdentification() const override;

    private:
        // Ensure the QOpenGLContext + FBO exist and match the current view size.
        // Makes the GL context current and binds the FBO on return.
        void ensureGLContext() const;

        VulkanView* m_view;
        QWidget* m_eventWidget;
        QTTranslator* m_translator;
        float m_devicePixelRatio{1.0f};
        int m_x{0};
        int m_y{0};
        float m_refresh{-1.0f};
        bool m_isOpen{false};

        // Qt GL context + offscreen surface for GL rendering.
        mutable QOpenGLContext* m_glContext{nullptr};
        mutable QOffscreenSurface* m_offscreenSurface{nullptr};
        mutable TwkGLF::GLFBO* m_fbo{nullptr};
        mutable GLuint m_fboColorTex{0}; // Texture attached to m_fbo; GLFBO does not own it
        mutable int m_fboWidth{0};
        mutable int m_fboHeight{0};

        // GPU Interop GL objects, ringed per in-flight slot to match VulkanView's
        // per-slot Vulkan shared image/semaphores. Indexed by the Vulkan slot for
        // the frame being rendered (VulkanView::currentFrame()).
        mutable std::array<GLuint, VulkanView::FRAMES_IN_FLIGHT> m_glMemoryObject{};
        mutable std::array<GLuint, VulkanView::FRAMES_IN_FLIGHT> m_glSharedTexture{};
        mutable std::array<GLuint, VulkanView::FRAMES_IN_FLIGHT> m_glReadySemaphore{};
        mutable std::array<GLuint, VulkanView::FRAMES_IN_FLIGHT> m_vkReadySemaphore{};
        mutable std::array<GLuint, VulkanView::FRAMES_IN_FLIGHT> m_drawFbo{};
        mutable std::array<int, VulkanView::FRAMES_IN_FLIGHT> m_sharedWidth{};
        mutable std::array<int, VulkanView::FRAMES_IN_FLIGHT> m_sharedHeight{};

        void cleanupSharedGLObjects(uint32_t slot) const;

        // CPU-fallback GL state (used only when GPU interop is unavailable or
        // refused). A flipped RGB10_A2 blit target lets GL pack the 10-bit pixels
        // directly with glReadPixels(GL_UNSIGNED_INT_2_10_10_10_REV) and handle the
        // Y flip, eliminating the per-pixel CPU pack loop. The readback format
        // (GL_RGBA vs GL_BGRA) selects the swapchain's channel order
        // (A2B10G10R10 / A2R10G10B10). Not ringed: the fallback is a synchronous
        // readback, so a single reused target is sufficient.
        mutable GLuint m_cpuFlipFbo{0};
        mutable GLuint m_cpuFlipTex{0};
        mutable int m_cpuFlipWidth{0};
        mutable int m_cpuFlipHeight{0};
        mutable std::vector<uint32_t> m_cpuPackedScratch;

        void ensureCpuFallbackTarget(int w, int h) const;
        void cleanupCpuFallbackTarget() const;

        // Pack + present the framebuffer via the CPU fallback (GL-packed RGB10_A2
        // readback). Used when no zero-copy interop path is available.
        void presentCpuFallback(int w, int h) const;
    };

} // namespace Rv
