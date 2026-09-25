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
        //  eventWidget is the window's container QWidget, used by QTTranslator
        //  for coordinate mapping and mouse grab.
        QTVulkanVideoDevice(TwkApp::VideoModule* module, const std::string& name, VulkanWindow* window, QWidget* eventWidget);
        virtual ~QTVulkanVideoDevice();

        VulkanWindow* vulkanWindow() const { return m_window; }

        QWidget* eventWidget() const { return m_eventWidget; }

        void setEventWidget(QWidget* widget);

        void resetInteropDeviceMatch() const { m_glVulkanDeviceMatch = -1; }

        const QTTranslator& translator() const { return *m_translator; }

        bool hasTranslator() const { return m_translator != nullptr; }

        void setAbsolutePosition(int x, int y);

        //  Drop every GL object imported from the Vulkan side so none outlives
        //  the memory it aliases. syncBuffers() re-imports on the next frame.
        void releaseSharedGLObjects();

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

        //  Readiness probe: the FBO id, or 0 before it exists. Unlike
        //  defaultFBO() it does not create the context; DesktopVideoDevice::
        //  transfer() waits for a non-zero id.
        GLuint fboID() const override;

    private:
        // Ensure the QOpenGLContext + FBO exist and match the current window size.
        // Makes the GL context current and binds the FBO on return.
        void ensureGLContext() const;

        //  The window container owns the window, so Qt can delete it
        //  independently of this device.
        QPointer<VulkanWindow> m_window;
        QWidget* m_eventWidget;
        QTTranslator* m_translator;
        float m_devicePixelRatio{1.0f};
        int m_x{0};
        int m_y{0};
        float m_refresh{-1.0f};
        bool m_isOpen{false};

        mutable QOpenGLContext* m_glContext{nullptr};
        mutable QOffscreenSurface* m_offscreenSurface{nullptr};
        mutable TwkGLF::GLFBO* m_fbo{nullptr};
        mutable GLuint m_fboColorTex{0}; // Texture attached to m_fbo; GLFBO does not own it
        mutable int m_fboWidth{0};
        mutable int m_fboHeight{0};

        // Interop GL objects, indexed by VulkanWindow::currentFrame().
        mutable std::array<GLuint, VulkanWindow::kFramesInFlight> m_glMemoryObject{};
        mutable std::array<GLuint, VulkanWindow::kFramesInFlight> m_glSharedTexture{};
        mutable std::array<GLuint, VulkanWindow::kFramesInFlight> m_glReadySemaphore{};
        mutable std::array<GLuint, VulkanWindow::kFramesInFlight> m_vkReadySemaphore{};
        mutable std::array<GLuint, VulkanWindow::kFramesInFlight> m_drawFbo{};
        mutable std::array<int, VulkanWindow::kFramesInFlight> m_sharedWidth{};
        mutable std::array<int, VulkanWindow::kFramesInFlight> m_sharedHeight{};

        // Last reported present path: -1 none yet, 0 CPU-fallback, 1 GPU-interop.
        mutable int m_loggedPresentPath{-1};
        // -1 until queried, 0 when GL and Vulkan use different/unidentifiable
        // physical devices, 1 when their device UUIDs match.
        mutable int m_glVulkanDeviceMatch{-1};

        // Latched once any GL call on the interop path fails; the device then
        // stays on the CPU path.
        mutable bool m_interopDisabled{false};

        // Drain glGetError(); on error, report which step failed, latch
        // m_interopDisabled and return true. Callers must then release the
        // slot's GL objects and present through the CPU fallback.
        bool interopGLFailed(const char* what) const;

        void cleanupSharedGLObjects(uint32_t slot) const;
        bool glDeviceMatchesVulkan() const;

        // CPU-fallback target: a Y-flipped RGB10_A2 copy that glReadPixels packs
        // directly. Not ringed, since the readback is synchronous.
        mutable GLuint m_cpuFlipFbo{0};
        mutable GLuint m_cpuFlipTex{0};
        mutable int m_cpuFlipWidth{0};
        mutable int m_cpuFlipHeight{0};
        mutable std::vector<uint32_t> m_cpuPackedScratch;

        void ensureCpuFallbackTarget(int w, int h) const;
        void cleanupCpuFallbackTarget() const;

        void presentCpuFallback(int w, int h) const;
    };

} // namespace Rv
