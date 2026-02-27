//
//  Copyright (c) 2024 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
#import <TwkMTLF/MetalFBO.h>
#import <TwkMTLF/MetalVideoDevice.h>

#import <Metal/Metal.h>
#import <Foundation/Foundation.h>

#include <cassert>
#include <sstream>
#include <stdexcept>

namespace TwkMTLF
{

// ---------------------------------------------------------------------------
//  Off-screen render-target constructor
// ---------------------------------------------------------------------------

MetalFBO::MetalFBO(size_t width, size_t height,
                   MTLPixelFormat colorFormat,
                   id<MTLDevice> device,
                   bool hasDepth,
                   void* data)
    : m_width(width)
    , m_height(height)
    , m_ownsTextures(true)
    , m_deviceRef(nullptr)
    , m_state(State::Renderable)
    , m_userData(data)
    , m_totalSizeInBytes(0)
    , m_colorFormat(colorFormat)
    , m_colorTexture(nil)
    , m_depthTexture(nil)
    , m_renderPassDescriptor(nil)
    , m_stagingBuffer(nil)
    , m_activeEncoder(nil)
{
    @autoreleasepool
    {
        if (!device)
        {
            throw std::invalid_argument("TwkMTLF::MetalFBO: device is nil");
        }

        initColorTexture(device, colorFormat, width, height);

        if (hasDepth)
        {
            initDepthTexture(device, width, height);
        }

        buildRenderPassDescriptor();
    }
}

// ---------------------------------------------------------------------------
//  Default FBO constructor (wraps a MetalVideoDevice's drawable)
// ---------------------------------------------------------------------------

MetalFBO::MetalFBO(const MetalVideoDevice* device)
    : m_width(0)
    , m_height(0)
    , m_ownsTextures(false)
    , m_deviceRef(device)
    , m_state(State::Renderable)
    , m_userData(nullptr)
    , m_totalSizeInBytes(0)
    , m_colorFormat(MTLPixelFormatBGRA8Unorm)
    , m_colorTexture(nil)
    , m_depthTexture(nil)
    , m_renderPassDescriptor(nil)
    , m_stagingBuffer(nil)
    , m_activeEncoder(nil)
{
    // The dimensions and drawable texture are resolved at render time via
    // the device's CAMetalLayer.  This constructor mirrors GLFBO(GLVideoDevice*)
    // which also leaves m_width/m_height at 0 until query time.
    m_renderPassDescriptor = [MTLRenderPassDescriptor new];
}

// ---------------------------------------------------------------------------
//  Destructor
// ---------------------------------------------------------------------------

MetalFBO::~MetalFBO()
{
    // End any open encoder before destroying the object.
    if (m_activeEncoder)
    {
        [m_activeEncoder endEncoding];
        m_activeEncoder = nil;
    }

    m_stagingBuffer       = nil;
    m_renderPassDescriptor = nil;

    if (m_ownsTextures)
    {
        m_colorTexture = nil;
        m_depthTexture = nil;
    }
}

// ---------------------------------------------------------------------------
//  Geometry
// ---------------------------------------------------------------------------

size_t MetalFBO::width() const
{
    if (m_colorTexture)
        return m_colorTexture.width;
    if (m_deviceRef)
        return m_deviceRef->width();
    return m_width;
}

size_t MetalFBO::height() const
{
    if (m_colorTexture)
        return m_colorTexture.height;
    if (m_deviceRef)
        return m_deviceRef->height();
    return m_height;
}

// ---------------------------------------------------------------------------
//  State
// ---------------------------------------------------------------------------

MetalFBO::State MetalFBO::state() const
{
    return m_state;
}

// ---------------------------------------------------------------------------
//  Render pass
// ---------------------------------------------------------------------------

id<MTLRenderCommandEncoder>
MetalFBO::beginRenderPass(id<MTLCommandBuffer> commandBuffer)
{
    assert(commandBuffer != nil);
    assert(m_state == State::Renderable);
    assert(m_renderPassDescriptor != nil);

    @autoreleasepool
    {
        // For the default (window-system) FBO the caller must have already
        // set m_renderPassDescriptor's color attachment texture to the
        // current drawable texture before calling this method.
        m_activeEncoder =
            [commandBuffer renderCommandEncoderWithDescriptor:m_renderPassDescriptor];

        if (!m_activeEncoder)
        {
            throw std::runtime_error("TwkMTLF::MetalFBO::beginRenderPass: "
                                     "renderCommandEncoderWithDescriptor returned nil.");
        }

        m_activeEncoder.label = @"TwkMTLF::MetalFBO::renderEncoder";
        m_state = State::Encoding;
    }

    return m_activeEncoder;
}

void MetalFBO::endRenderPass(id<MTLRenderCommandEncoder> encoder)
{
    if (!encoder)
        return;

    [encoder endEncoding];

    if (m_activeEncoder == encoder)
    {
        m_activeEncoder = nil;
    }

    m_state = State::Renderable;
}

// ---------------------------------------------------------------------------
//  Compatibility aliases (GL bind/unbind naming)
// ---------------------------------------------------------------------------

id<MTLRenderCommandEncoder>
MetalFBO::bind(id<MTLCommandBuffer> commandBuffer)
{
    return beginRenderPass(commandBuffer);
}

void MetalFBO::unbind(id<MTLRenderCommandEncoder> encoder)
{
    endRenderPass(encoder);
}

// ---------------------------------------------------------------------------
//  Blit copy
// ---------------------------------------------------------------------------

void MetalFBO::copyTo(const MetalFBO* dst,
                      id<MTLCommandBuffer> commandBuffer) const
{
    assert(dst != nullptr);
    assert(commandBuffer != nil);
    assert(m_colorTexture != nil);
    assert(dst->m_colorTexture != nil);

    @autoreleasepool
    {
        id<MTLBlitCommandEncoder> blit =
            [commandBuffer blitCommandEncoder];
        blit.label = @"TwkMTLF::MetalFBO::copyTo";

        [blit copyFromTexture:m_colorTexture
                  sourceSlice:0
                  sourceLevel:0
                 sourceOrigin:MTLOriginMake(0, 0, 0)
                   sourceSize:MTLSizeMake(width(), height(), 1)
                    toTexture:dst->m_colorTexture
             destinationSlice:0
             destinationLevel:0
            destinationOrigin:MTLOriginMake(0, 0, 0)];

        [blit endEncoding];
    }
}

void MetalFBO::copyRegionTo(const MetalFBO* dst,
                             id<MTLCommandBuffer> commandBuffer,
                             MTLOrigin srcOrigin, MTLSize srcSize,
                             MTLOrigin dstOrigin) const
{
    assert(dst != nullptr);
    assert(commandBuffer != nil);
    assert(m_colorTexture != nil);
    assert(dst->m_colorTexture != nil);

    @autoreleasepool
    {
        id<MTLBlitCommandEncoder> blit =
            [commandBuffer blitCommandEncoder];
        blit.label = @"TwkMTLF::MetalFBO::copyRegionTo";

        [blit copyFromTexture:m_colorTexture
                  sourceSlice:0
                  sourceLevel:0
                 sourceOrigin:srcOrigin
                   sourceSize:srcSize
                    toTexture:dst->m_colorTexture
             destinationSlice:0
             destinationLevel:0
            destinationOrigin:dstOrigin];

        [blit endEncoding];
    }
}

// ---------------------------------------------------------------------------
//  Async read-back
// ---------------------------------------------------------------------------

void MetalFBO::beginAsyncReadBack(id<MTLCommandBuffer> commandBuffer)
{
    assert(commandBuffer != nil);
    assert(m_colorTexture != nil);

    @autoreleasepool
    {
        const size_t bytesPerRow  = m_colorTexture.width * 4; // assume 4 bytes/pixel
        const size_t bufferSize   = bytesPerRow * m_colorTexture.height;

        if (!m_stagingBuffer || m_stagingBuffer.length < bufferSize)
        {
            id<MTLDevice> dev = m_colorTexture.device;
            m_stagingBuffer =
                [dev newBufferWithLength:bufferSize
                                 options:MTLResourceStorageModeShared];

            if (!m_stagingBuffer)
            {
                throw std::runtime_error("TwkMTLF::MetalFBO::beginAsyncReadBack: "
                                         "Failed to allocate staging buffer.");
            }
        }

        // Blit from the render-target texture into the staging buffer.
        id<MTLBlitCommandEncoder> blit = [commandBuffer blitCommandEncoder];
        blit.label = @"TwkMTLF::MetalFBO::asyncReadBack";

        [blit copyFromTexture:m_colorTexture
                  sourceSlice:0
                  sourceLevel:0
                 sourceOrigin:MTLOriginMake(0, 0, 0)
                   sourceSize:MTLSizeMake(m_colorTexture.width,
                                          m_colorTexture.height, 1)
                     toBuffer:m_stagingBuffer
            destinationOffset:0
       destinationBytesPerRow:bytesPerRow
     destinationBytesPerImage:bufferSize];

        [blit endEncoding];

        m_state = State::Committed;

        // Wake any waiting threads when the GPU finishes.
        __block std::condition_variable* condPtr = &m_readCond;
        __block std::mutex*              mutPtr  = &m_mutex;
        __block State*                   statePtr = &m_state;

        [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> /*buf*/) {
            std::unique_lock<std::mutex> lock(*mutPtr);
            *statePtr = State::ReadyToMap;
            condPtr->notify_all();
        }];
    }
}

void MetalFBO::waitForReadBack() const
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_readCond.wait(lock, [this] {
        return m_state == State::ReadyToMap || m_state == State::Mapped;
    });
}

void* MetalFBO::mappedContents() const
{
    if (!m_stagingBuffer)
        return nullptr;

    m_state = State::Mapped;
    return m_stagingBuffer.contents;
}

void MetalFBO::unmapBuffer()
{
    // On Shared storage there is no explicit unmap; just transition state.
    m_state = State::Renderable;
}

// ---------------------------------------------------------------------------
//  Identifier
// ---------------------------------------------------------------------------

std::string MetalFBO::identifier() const
{
    std::ostringstream oss;
    oss << "MetalFBO(" << width() << "x" << height() << ")";
    return oss.str();
}

// ---------------------------------------------------------------------------
//  Private helpers
// ---------------------------------------------------------------------------

void MetalFBO::initColorTexture(id<MTLDevice> device,
                                 MTLPixelFormat fmt,
                                 size_t w, size_t h)
{
    MTLTextureDescriptor* desc =
        [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:fmt
                                                           width:w
                                                          height:h
                                                       mipmapped:NO];

    desc.usage          = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    desc.storageMode    = MTLStorageModePrivate;
    desc.resourceOptions = MTLResourceStorageModePrivate;

    m_colorTexture = [device newTextureWithDescriptor:desc];
    if (!m_colorTexture)
    {
        throw std::runtime_error("TwkMTLF::MetalFBO: Failed to create color texture.");
    }
    m_colorTexture.label = @"TwkMTLF::MetalFBO::colorTexture";

    // bytes per pixel * w * h (approximate — depends on format)
    m_totalSizeInBytes += w * h * 4; // conservative 4 bytes/pixel estimate
}

void MetalFBO::initDepthTexture(id<MTLDevice> device,
                                 size_t w, size_t h)
{
    MTLTextureDescriptor* desc =
        [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float_Stencil8
                                                           width:w
                                                          height:h
                                                       mipmapped:NO];

    desc.usage       = MTLTextureUsageRenderTarget;
    desc.storageMode = MTLStorageModePrivate;

    m_depthTexture = [device newTextureWithDescriptor:desc];
    if (!m_depthTexture)
    {
        throw std::runtime_error("TwkMTLF::MetalFBO: Failed to create depth/stencil texture.");
    }
    m_depthTexture.label = @"TwkMTLF::MetalFBO::depthTexture";
}

void MetalFBO::buildRenderPassDescriptor()
{
    m_renderPassDescriptor = [MTLRenderPassDescriptor new];

    if (m_colorTexture)
    {
        m_renderPassDescriptor.colorAttachments[0].texture     = m_colorTexture;
        m_renderPassDescriptor.colorAttachments[0].loadAction  = MTLLoadActionClear;
        m_renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
        m_renderPassDescriptor.colorAttachments[0].clearColor  =
            MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
    }

    if (m_depthTexture)
    {
        m_renderPassDescriptor.depthAttachment.texture     = m_depthTexture;
        m_renderPassDescriptor.depthAttachment.loadAction  = MTLLoadActionClear;
        m_renderPassDescriptor.depthAttachment.storeAction = MTLStoreActionDontCare;
        m_renderPassDescriptor.depthAttachment.clearDepth  = 1.0;

        m_renderPassDescriptor.stencilAttachment.texture     = m_depthTexture;
        m_renderPassDescriptor.stencilAttachment.loadAction  = MTLLoadActionClear;
        m_renderPassDescriptor.stencilAttachment.storeAction = MTLStoreActionDontCare;
        m_renderPassDescriptor.stencilAttachment.clearStencil = 0;
    }
}

} // namespace TwkMTLF
