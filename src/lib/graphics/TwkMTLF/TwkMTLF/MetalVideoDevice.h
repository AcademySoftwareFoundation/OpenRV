//
//  Copyright (c) 2024 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
#ifndef __TwkMTLF__MetalVideoDevice__h__
#define __TwkMTLF__MetalVideoDevice__h__

#include <string>
#include <TwkApp/VideoDevice.h>

#ifdef __OBJC__
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>
#endif

namespace TwkApp
{
    class VideoModule;
}

namespace TwkFB
{
    class FrameBuffer;
}

namespace TwkMTLF
{

    class MetalFBO;

    //
    //  MetalVideoDevice
    //
    //  A TwkApp::VideoDevice backed by Apple Metal.  Subclasses must supply
    //  a concrete MTLDevice (usually obtained from MTLCreateSystemDefaultDevice()).
    //
    //  Frame lifecycle:
    //    beginFrame()   — acquires a new MTLCommandBuffer for the current frame
    //    endFrame()     — commits the command buffer (replaces GL syncBuffers)
    //    currentCommandBuffer() — returns the in-flight command buffer
    //
    //  The class mirrors the GLVideoDevice interface so that higher-level code
    //  can be ported with minimal friction.
    //

    class MetalVideoDevice : public TwkApp::VideoDevice
    {
    public:
        MetalVideoDevice(TwkApp::VideoModule* module, const std::string& name, unsigned int capabilities);

        virtual ~MetalVideoDevice();

        //
        //  TwkApp::VideoDevice API — minimal overrides required by the base.
        //  Subclasses that own a real window/layer should further override these.
        //

        virtual TwkApp::VideoDevice::Resolution resolution() const override;
        virtual TwkApp::VideoDevice::Offset offset() const override;
        virtual TwkApp::VideoDevice::Timing timing() const override;
        virtual TwkApp::VideoDevice::VideoFormat format() const override;

        virtual void open(const StringVector& args) override;
        virtual void close() override;
        virtual bool isOpen() const override;

        virtual void clearCaches() const override;

        //
        //  Metal frame lifecycle — replaces makeCurrent/syncBuffers.
        //
        //  beginFrame() must be called once per frame before issuing any
        //  Metal rendering commands.  endFrame() commits and releases the
        //  command buffer.  Both are virtual so subclasses that manage a
        //  CAMetalLayer drawable can override them.
        //

        virtual void beginFrame() const;
        virtual void endFrame() const;

        //
        //  Compatibility shim: makeCurrent() calls beginFrame() so existing
        //  call sites compile without changes.
        //

        virtual void makeCurrent() const;

        //
        //  syncBuffers() calls endFrame() — matches the base class virtual.
        //

        virtual void syncBuffers() const override;

        virtual void redraw() const;
        virtual void redrawImmediately() const;

        //
        //  Default FBO (Metal render target).  The device owns the returned object.
        //

        virtual MetalFBO* defaultFBO();
        virtual const MetalFBO* defaultFBO() const;

        //
        //  Metal object accessors.
        //
        //  These return opaque void* when compiled from plain C++ so that headers
        //  including this file from non-.mm translation units still compile.
        //

#ifdef __OBJC__
        id<MTLDevice> device() const { return m_device; }

        id<MTLCommandQueue> commandQueue() const { return m_commandQueue; }

        id<MTLCommandBuffer> currentCommandBuffer() const { return m_currentCommandBuffer; }
#else
        void* device() const { return m_device_opaque; }

        void* commandQueue() const { return m_commandQueue_opaque; }

        void* currentCommandBuffer() const { return m_currentCommandBuffer_opaque; }
#endif

        //
        //  Utility: map TwkApp::VideoDevice::InternalDataFormat → MTLPixelFormat.
        //  Returns MTLPixelFormatInvalid (0) if there is no direct equivalent.
        //

#ifdef __OBJC__
        static MTLPixelFormat metalPixelFormatFromDataFormat(InternalDataFormat fmt);
#endif

        //
        //  Worker/shared-context device factory (Metal has no notion of shared
        //  GL contexts; on Metal the MTLDevice and MTLCommandQueue are naturally
        //  sharable).  Returns nullptr by default; subclasses may override.
        //

        virtual MetalVideoDevice* newSharedContextWorkerDevice() const;

    protected:
        //
        //  Called by the constructor after setting m_device.  Creates
        //  m_commandQueue.  Subclasses that set m_device themselves should call
        //  this before any rendering.
        //

        void initCommandQueue();

    protected:
        mutable MetalFBO* m_fbo;

#ifdef __OBJC__
        id<MTLDevice> m_device;
        id<MTLCommandQueue> m_commandQueue;
        mutable id<MTLCommandBuffer> m_currentCommandBuffer;
#else
        void* m_device_opaque;
        void* m_commandQueue_opaque;
        mutable void* m_currentCommandBuffer_opaque;
#endif
    };

} // namespace TwkMTLF

#endif // __TwkMTLF__MetalVideoDevice__h__
