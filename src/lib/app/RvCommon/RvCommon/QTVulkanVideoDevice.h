//
//  Copyright (c) 2026 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
#pragma once

#include <TwkGLF/GLVideoDevice.h>
#include <RvCommon/QTTranslator.h>
#include <RvCommon/VulkanWindow.h>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <QtCore/QPointer>

QT_BEGIN_NAMESPACE
class QOpenGLContext;
class QOffscreenSurface;
class QWidget;
QT_END_NAMESPACE

namespace Rv
{
    class VulkanWindow;

    //
    //  QTVulkanVideoDevice
    //
    //  Wraps a VulkanWindow as a TwkGLF::GLVideoDevice so that ImageRenderer's
    //  existing GL rendering pipeline (renderMain, shader cache, etc.) can run
    //  unchanged on the Vulkan presentation path.
    //
    class QTVulkanVideoDevice : public TwkGLF::GLVideoDevice
    {
    public:
        //
        //  The presentation surface is a QWindow (embedded in the widget tree
        //  via createWindowContainer); eventWidget is the container QWidget the
        //  QTTranslator uses for coordinate mapping (height/mapToGlobal) and
        //  mouse grab.
        //
        QTVulkanVideoDevice(TwkApp::VideoModule* module, const std::string& name, VulkanWindow* window, QWidget* eventWidget);
        virtual ~QTVulkanVideoDevice();

        VulkanWindow* vulkanWindow() const { return m_window; }

        QWidget* eventWidget() const { return m_eventWidget; }

        void setEventWidget(QWidget* widget);

        const QTTranslator& translator() const { return *m_translator; }

        bool hasTranslator() const { return m_translator != nullptr; }

        void setAbsolutePosition(int x, int y);

        // VideoDevice API
        void makeCurrent() const override;
        void syncBuffers() const override;
        void redraw() const override;
        void redrawImmediately() const override;
        void clearCaches() const override;

        Resolution resolution() const override;
        Offset offset() const override;
        Timing timing() const override;
        VideoFormat format() const override;

        size_t width() const override;
        size_t height() const override;

        void open(const StringVector& args) override;
        void close() override;
        bool isOpen() const override;

        float devicePixelRatio() const override;

        void setPhysicalDevice(VideoDevice* d) override;

        // GLVideoDevice API
        TwkGLF::GLFBO* defaultFBO() override;
        const TwkGLF::GLFBO* defaultFBO() const override;
        std::string hardwareIdentification() const override;

    private:
        // Ensure the QOpenGLContext + FBO exist and match the current window size.
        // Makes the GL context current and binds the FBO on return.
        void ensureGLContext() const;

        //
        //  Guarded: the window is embedded via QWidget::createWindowContainer(),
        //  which owns it, so Qt can delete it independently of this device (and
        //  of the VulkanView that created both). A QPointer makes the
        //  `if (m_window)` checks below actual liveness checks instead of null
        //  checks.
        //
        QPointer<VulkanWindow> m_window;
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

        // GPU Interop GL objects, ringed per in-flight slot to match VulkanWindow's
        // per-slot Vulkan shared image/semaphores. Indexed by the Vulkan slot for
        // the frame being rendered (VulkanWindow::currentFrame()).
        mutable std::array<GLuint, VulkanWindow::FRAMES_IN_FLIGHT> m_glMemoryObject{};
        mutable std::array<GLuint, VulkanWindow::FRAMES_IN_FLIGHT> m_glSharedTexture{};
        mutable std::array<GLuint, VulkanWindow::FRAMES_IN_FLIGHT> m_glReadySemaphore{};
        mutable std::array<GLuint, VulkanWindow::FRAMES_IN_FLIGHT> m_vkReadySemaphore{};
        mutable std::array<GLuint, VulkanWindow::FRAMES_IN_FLIGHT> m_drawFbo{};
        mutable std::array<int, VulkanWindow::FRAMES_IN_FLIGHT> m_sharedWidth{};
        mutable std::array<int, VulkanWindow::FRAMES_IN_FLIGHT> m_sharedHeight{};

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
