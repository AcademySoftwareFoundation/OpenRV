//
//  Copyright (c) 2026 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
#ifndef __RvCommon__QTVulkanVideoDevice__h__
#define __RvCommon__QTVulkanVideoDevice__h__

#include <TwkGLF/GLVideoDevice.h>
#include <RvCommon/QTTranslator.h>
#include <RvCommon/VulkanWindow.h>
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
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
        ~QTVulkanVideoDevice() override;

        QTVulkanVideoDevice(const QTVulkanVideoDevice&) = delete;
        QTVulkanVideoDevice& operator=(const QTVulkanVideoDevice&) = delete;

        void resetInteropDeviceMatch() const { m_glVulkanDeviceMatch.reset(); }

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

        void setPhysicalDevice(VideoDevice* device) override;

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

        // Logical window pixels to device pixels, rounded. Requires m_window.
        int toDevicePixels(int logical) const;

        //  The window container owns the window, so Qt can delete it
        //  independently of this device.
        QPointer<VulkanWindow> m_window;
        std::unique_ptr<QTTranslator> m_translator;
        float m_devicePixelRatio{1.0f};
        int m_x{0};
        int m_y{0};
        std::optional<float> m_refresh;

        mutable std::unique_ptr<QOpenGLContext> m_glContext;
        mutable std::unique_ptr<QOffscreenSurface> m_offscreenSurface;
        mutable std::unique_ptr<TwkGLF::GLFBO> m_fbo;
        mutable GLuint m_fboColorTex{0}; // Texture attached to m_fbo; GLFBO does not own it
        mutable int m_fboWidth{0};
        mutable int m_fboHeight{0};

        // Interop GL objects, indexed by VulkanWindow::currentFrame().
        struct SharedGLObjects
        {
            GLuint memoryObject{0};
            GLuint texture{0};
            GLuint glReadySemaphore{0};
            GLuint vkReadySemaphore{0};
            GLuint drawFbo{0};

            // Imported capacity, not the used size.
            int width{0};
            int height{0};
        };

        mutable std::array<SharedGLObjects, VulkanWindow::kFramesInFlight> m_glShared{};

        // Last logged present path (true for GPU-interop); empty until the first.
        mutable std::optional<bool> m_loggedPresentPath;
        // Empty until queried; true when the GL and Vulkan device UUIDs match.
        mutable std::optional<bool> m_glVulkanDeviceMatch;

        // Latched once any GL call on the interop path fails; the device then
        // stays on the CPU path.
        mutable bool m_interopDisabled{false};

        // Drain glGetError(); on error, report which step failed, latch
        // m_interopDisabled and return true. Callers must then release the
        // slot's GL objects and present through the CPU fallback.
        bool interopGLFailed(const char* what) const;

        void cleanupSharedGLObjects(uint32_t slot) const;
        void cleanupAllSharedGLObjects() const;
        bool glDeviceMatchesVulkan() const;

        // CPU-fallback target: a Y-flipped RGB10_A2 copy that glReadPixels packs
        // directly. Not ringed, since the readback is synchronous.
        mutable GLuint m_cpuFlipFbo{0};
        mutable GLuint m_cpuFlipTex{0};
        mutable int m_cpuFlipWidth{0};
        mutable int m_cpuFlipHeight{0};
        mutable std::vector<uint32_t> m_cpuPackedScratch;

        void ensureCpuFallbackTarget(int targetWidth, int targetHeight) const;
        void cleanupCpuFallbackTarget() const;

        void presentCpuFallback(int frameWidth, int frameHeight) const;
    };

} // namespace Rv

#endif // __RvCommon__QTVulkanVideoDevice__h__
