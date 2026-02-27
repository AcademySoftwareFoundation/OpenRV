//
//  Copyright (c) 2024 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
#import <TwkMTLF/MetalVideoDevice.h>
#import <TwkMTLF/MetalFBO.h>

#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>
#import <Foundation/Foundation.h>

#include <cassert>
#include <stdexcept>

namespace TwkMTLF
{

using namespace TwkApp;

// ---------------------------------------------------------------------------
//  Construction / destruction
// ---------------------------------------------------------------------------

MetalVideoDevice::MetalVideoDevice(VideoModule* module,
                                   const std::string& name,
                                   unsigned int capabilities)
    : VideoDevice(module, name, capabilities)
    , m_fbo(nullptr)
    , m_device(nil)
    , m_commandQueue(nil)
    , m_currentCommandBuffer(nil)
{
    @autoreleasepool
    {
        m_device = MTLCreateSystemDefaultDevice();
        if (!m_device)
        {
            throw std::runtime_error("TwkMTLF::MetalVideoDevice: "
                                     "MTLCreateSystemDefaultDevice() returned nil — "
                                     "Metal is not available on this machine.");
        }
        initCommandQueue();
    }
}

MetalVideoDevice::~MetalVideoDevice()
{
    // Ensure any in-flight command buffer is finished before we tear down.
    if (m_currentCommandBuffer)
    {
        [m_currentCommandBuffer waitUntilCompleted];
        m_currentCommandBuffer = nil;
    }

    delete m_fbo;
    m_fbo = nullptr;

    // ARC releases m_commandQueue and m_device automatically.
    m_commandQueue = nil;
    m_device       = nil;
}

// ---------------------------------------------------------------------------
//  Internal initialisation helpers
// ---------------------------------------------------------------------------

void MetalVideoDevice::initCommandQueue()
{
    assert(m_device != nil);
    m_commandQueue = [m_device newCommandQueue];
    if (!m_commandQueue)
    {
        throw std::runtime_error("TwkMTLF::MetalVideoDevice: "
                                 "Failed to create MTLCommandQueue.");
    }
    m_commandQueue.label = @"TwkMTLF::MetalVideoDevice::commandQueue";
}

// ---------------------------------------------------------------------------
//  TwkApp::VideoDevice API
// ---------------------------------------------------------------------------

VideoDevice::Resolution MetalVideoDevice::resolution() const
{
    return Resolution(width(), height(), 1.0f, 1.0f);
}

VideoDevice::Offset MetalVideoDevice::offset() const
{
    return Offset(0, 0);
}

VideoDevice::Timing MetalVideoDevice::timing() const
{
    return Timing(0.0f);
}

VideoDevice::VideoFormat MetalVideoDevice::format() const
{
    return VideoFormat(width(), height(), 1.0f, 1.0f, 0.0f,
                       hardwareIdentification());
}

void MetalVideoDevice::open(const StringVector& /*args*/) {}

void MetalVideoDevice::close() {}

bool MetalVideoDevice::isOpen() const { return m_device != nil; }

void MetalVideoDevice::clearCaches() const
{
    // No GL texture caches to clear; subclasses may override to invalidate
    // pipeline-state caches or argument-buffer caches.
}

// ---------------------------------------------------------------------------
//  Frame lifecycle
// ---------------------------------------------------------------------------

void MetalVideoDevice::beginFrame() const
{
    @autoreleasepool
    {
        assert(m_commandQueue != nil);

        // If the previous frame's buffer was never committed (e.g. the caller
        // forgot to call endFrame), commit it now to avoid a resource leak.
        if (m_currentCommandBuffer &&
            m_currentCommandBuffer.status == MTLCommandBufferStatusNotEnqueued)
        {
            [m_currentCommandBuffer commit];
        }

        m_currentCommandBuffer = [m_commandQueue commandBuffer];
        if (!m_currentCommandBuffer)
        {
            throw std::runtime_error("TwkMTLF::MetalVideoDevice::beginFrame: "
                                     "Failed to create MTLCommandBuffer.");
        }
        m_currentCommandBuffer.label = @"TwkMTLF::frame";
    }
}

void MetalVideoDevice::endFrame() const
{
    if (!m_currentCommandBuffer)
        return;

    @autoreleasepool
    {
        [m_currentCommandBuffer commit];
        m_currentCommandBuffer = nil;
    }
}

void MetalVideoDevice::makeCurrent() const
{
    // Compatibility shim: makeCurrent() starts a new frame.
    beginFrame();
}

void MetalVideoDevice::syncBuffers() const
{
    endFrame();
}

void MetalVideoDevice::redraw() const {}

void MetalVideoDevice::redrawImmediately() const {}

// ---------------------------------------------------------------------------
//  Default FBO
// ---------------------------------------------------------------------------

MetalFBO* MetalVideoDevice::defaultFBO()
{
    if (!m_fbo)
        m_fbo = new MetalFBO(this);
    return m_fbo;
}

const MetalFBO* MetalVideoDevice::defaultFBO() const
{
    if (!m_fbo)
        m_fbo = new MetalFBO(this);
    return m_fbo;
}

// ---------------------------------------------------------------------------
//  Worker device factory
// ---------------------------------------------------------------------------

MetalVideoDevice* MetalVideoDevice::newSharedContextWorkerDevice() const
{
    // Metal devices and command queues are naturally thread-safe and shareable.
    // Subclasses that need a dedicated command queue for a worker thread should
    // override this method.
    return nullptr;
}

// ---------------------------------------------------------------------------
//  Pixel format mapping
// ---------------------------------------------------------------------------

MTLPixelFormat
MetalVideoDevice::metalPixelFormatFromDataFormat(InternalDataFormat fmt)
{
    switch (fmt)
    {
    case VideoDevice::RGB8:
        // Metal has no 3-channel formats; pack into RGBA8.
        return MTLPixelFormatRGBA8Unorm;
    case VideoDevice::RGBA8:
        return MTLPixelFormatRGBA8Unorm;
    case VideoDevice::BGRA8:
        return MTLPixelFormatBGRA8Unorm;
    case VideoDevice::RGB16:
        return MTLPixelFormatRGBA16Unorm;  // promoted to 4-channel
    case VideoDevice::RGBA16:
        return MTLPixelFormatRGBA16Unorm;
    case VideoDevice::RGB10X2:
        return MTLPixelFormatRGB10A2Uint;
    case VideoDevice::RGB10X2Rev:
        return MTLPixelFormatBGR10A2Unorm;
    case VideoDevice::RGB16F:
        return MTLPixelFormatRGBA16Float;  // promoted to 4-channel
    case VideoDevice::RGBA16F:
        return MTLPixelFormatRGBA16Float;
    case VideoDevice::RGB32F:
        return MTLPixelFormatRGBA32Float;  // promoted to 4-channel
    case VideoDevice::RGBA32F:
        return MTLPixelFormatRGBA32Float;

    // YCbCr / 4:2:2 formats — Metal handles these via YCbCr conversion
    // descriptors on Apple Silicon.  Return a reasonable RGBA fallback here;
    // callers that need proper YCbCr rendering should handle these explicitly.
    case VideoDevice::CbY0CrY1_8_422:
    case VideoDevice::Y0CbY1Cr_8_422:
    case VideoDevice::Y1CbY0Cr_8_422:
        return MTLPixelFormatRGBA8Unorm;
    case VideoDevice::YCrCb_AJA_10_422:
    case VideoDevice::YCrCb_BM_10_422:
    case VideoDevice::YCbCr_P216_16_422:
        return MTLPixelFormatRGBA16Float;

    default:
        return MTLPixelFormatInvalid;
    }
}

} // namespace TwkMTLF
