//
//  Copyright (c) 2024 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
#ifndef __TwkMTLF__MetalFBO__h__
#define __TwkMTLF__MetalFBO__h__

#include <cstddef>
#include <functional>
#include <mutex>
#include <condition_variable>

#ifdef __OBJC__
#import <Metal/Metal.h>
#import <Foundation/Foundation.h>
#endif

namespace TwkMTLF
{

    class MetalVideoDevice;

    //
    //  MetalFBO  —  Metal equivalent of TwkGLF::GLFBO
    //
    //  Owns a color MTLTexture (and optionally a depth/stencil texture) and
    //  an MTLRenderPassDescriptor.  The "bind" model differs from GL:
    //
    //    * beginRenderPass(commandBuffer) — starts a render pass and returns an
    //      id<MTLRenderCommandEncoder>.  Callers draw into the encoder.
    //
    //    * endRenderPass(encoder)         — ends the encoder / render pass.
    //
    //    * copyTo(dst, commandBuffer)     — blit copy via MTLBlitCommandEncoder.
    //
    //  Async read-back uses the MTLCommandBuffer completion-handler mechanism
    //  rather than the GL PBO model.
    //
    //  Thread safety: the mutex / condition variable members follow the same
    //  pattern as GLFBO so that transfer threads can synchronise with the
    //  Metal command-buffer completion handler.
    //

    class MetalFBO
    {
    public:
        enum class State
        {
            Renderable,
            Encoding,   // inside a render pass
            Committed,  // command buffer submitted, GPU working
            ReadyToMap, // completion handler fired
            Mapped      // CPU pointer is live
        };

        //
        //  Construct an off-screen render target of the given dimensions and
        //  pixel format.  Pass hasDepth=true to also allocate a
        //  MTLPixelFormatDepth32Float_Stencil8 depth+stencil attachment.
        //
        //  A "default" FBO that wraps the window system drawable can be
        //  constructed via MetalFBO(device*) — mirroring GLFBO(GLVideoDevice*).
        //

#ifdef __OBJC__
        MetalFBO(size_t width, size_t height, MTLPixelFormat colorFormat, id<MTLDevice> device, bool hasDepth = false,
                 void* data = nullptr);
#endif

        explicit MetalFBO(const MetalVideoDevice* device);

        ~MetalFBO();

        // Non-copyable
        MetalFBO(const MetalFBO&) = delete;
        MetalFBO& operator=(const MetalFBO&) = delete;

        // -------------------------------------------------------------------------
        //  Geometry
        // -------------------------------------------------------------------------

        size_t width() const;
        size_t height() const;

        // -------------------------------------------------------------------------
        //  State
        // -------------------------------------------------------------------------

        State state() const;

        // -------------------------------------------------------------------------
        //  Render pass
        // -------------------------------------------------------------------------
        //
        //  beginRenderPass / endRenderPass replace the GL bind/unbind model.
        //  The caller must pass in the command buffer that will record the pass.
        //

#ifdef __OBJC__
        id<MTLRenderCommandEncoder> beginRenderPass(id<MTLCommandBuffer> commandBuffer);
        void endRenderPass(id<MTLRenderCommandEncoder> encoder);

        //
        //  Compatibility aliases matching the GLFBO API surface.
        //  bind() returns the encoder; unbind() ends it.
        //
        id<MTLRenderCommandEncoder> bind(id<MTLCommandBuffer> commandBuffer);
        void unbind(id<MTLRenderCommandEncoder> encoder);
#endif

        // -------------------------------------------------------------------------
        //  Texture access  (replaces GLFBO::colorID())
        // -------------------------------------------------------------------------

#ifdef __OBJC__
        id<MTLTexture> colorTexture() const { return m_colorTexture; }

        id<MTLTexture> depthTexture() const { return m_depthTexture; }

        MTLPixelFormat colorPixelFormat() const { return m_colorFormat; }

        MTLRenderPassDescriptor* renderPassDescriptor() const { return m_renderPassDescriptor; }
#else
        void* colorTexture() const { return m_colorTexture_opaque; }

        void* depthTexture() const { return m_depthTexture_opaque; }

        void* renderPassDescriptor() const { return m_renderPassDescriptor_opaque; }
#endif

        // -------------------------------------------------------------------------
        //  Blit copy  (replaces GLFBO::copyTo())
        // -------------------------------------------------------------------------

#ifdef __OBJC__
        void copyTo(const MetalFBO* dst, id<MTLCommandBuffer> commandBuffer) const;
        void copyRegionTo(const MetalFBO* dst, id<MTLCommandBuffer> commandBuffer, MTLOrigin srcOrigin, MTLSize srcSize,
                          MTLOrigin dstOrigin) const;
#endif

        // -------------------------------------------------------------------------
        //  Async read-back  (replaces GL PBO pattern)
        //
        //  Usage:
        //    1. Call beginAsyncReadBack() to allocate the staging buffer and
        //       attach a completion handler to the provided command buffer.
        //    2. After committing commandBuffer, call waitForReadBack() to block
        //       until the GPU is done.
        //    3. Call mappedContents() to get the CPU pointer.
        //    4. Call unmapBuffer() when done.
        // -------------------------------------------------------------------------

#ifdef __OBJC__
        void beginAsyncReadBack(id<MTLCommandBuffer> commandBuffer);
#endif

        void waitForReadBack() const;

        void* mappedContents() const;

        void unmapBuffer();

        // -------------------------------------------------------------------------
        //  Misc
        // -------------------------------------------------------------------------

        size_t totalSizeInBytes() const { return m_totalSizeInBytes; }

        template <class T> T* data() const { return reinterpret_cast<T*>(m_userData); }

        void setData(void* d) { m_userData = d; }

        std::mutex& mutex() const { return m_mutex; }

        std::condition_variable& readCond() const { return m_readCond; }

        std::string identifier() const;

    private:
        // Shared initialisation used by constructors
#ifdef __OBJC__
        void initColorTexture(id<MTLDevice> device, MTLPixelFormat fmt, size_t width, size_t height);
        void initDepthTexture(id<MTLDevice> device, size_t width, size_t height);
        void buildRenderPassDescriptor();
#endif

    private:
        size_t m_width;
        size_t m_height;

        bool m_ownsTextures;
        const MetalVideoDevice* m_deviceRef; // non-owning; only set for default FBO

        mutable State m_state;
        mutable std::mutex m_mutex;
        mutable std::condition_variable m_readCond;

        void* m_userData;
        size_t m_totalSizeInBytes;

#ifdef __OBJC__
        MTLPixelFormat m_colorFormat;
        id<MTLTexture> m_colorTexture;
        id<MTLTexture> m_depthTexture;
        MTLRenderPassDescriptor* m_renderPassDescriptor;
        id<MTLBuffer> m_stagingBuffer; // for async readback
        mutable id<MTLRenderCommandEncoder> m_activeEncoder;
#else
        int m_colorFormat_opaque;
        void* m_colorTexture_opaque;
        void* m_depthTexture_opaque;
        void* m_renderPassDescriptor_opaque;
        void* m_stagingBuffer_opaque;
        void* m_activeEncoder_opaque;
#endif
    };

    typedef std::vector<const MetalFBO*> ConstMetalFBOVector;
    typedef std::vector<MetalFBO*> MetalFBOVector;

} // namespace TwkMTLF

#endif // __TwkMTLF__MetalFBO__h__
