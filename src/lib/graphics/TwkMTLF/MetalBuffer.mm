//
//  Copyright (c) 2024 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
#import <TwkMTLF/MetalBuffer.h>

#import <Metal/Metal.h>
#import <Foundation/Foundation.h>

#include <cassert>
#include <stdexcept>

namespace TwkMTLF
{

// ---------------------------------------------------------------------------
//  Helpers
// ---------------------------------------------------------------------------

namespace
{

//  Choose the optimal MTLStorageMode for the direction on the current device.
//
//  Apple Silicon (unified memory architecture):
//    MTLStorageModeShared — CPU and GPU share the same physical memory;
//    no explicit synchronisation is needed.  This is the fast path for both
//    upload (TO_GPU) and readback (FROM_GPU).
//
//  Intel Mac / AMD (discrete GPU):
//    TO_GPU   — MTLStorageModeManaged: CPU writes, GPU reads.  The caller
//               must call didModifyRange() after CPU writes.
//    FROM_GPU — MTLStorageModeShared: placed in system memory so the CPU
//               can read the results without a synchronous copy.  Metal
//               will blit from the private GPU texture into this buffer.
//
MTLStorageMode
chooseStorageMode(id<MTLDevice> device, MetalBuffer::Direction dir)
{
    // MTLDevice.hasUnifiedMemory is available macOS 10.15+.
    if (device.hasUnifiedMemory)
    {
        return MTLStorageModeShared;
    }

    // Discrete GPU.
    if (dir == MetalBuffer::Direction::TO_GPU)
    {
        // Managed lets Metal synchronise between CPU and GPU without a
        // separate staging step.
        return MTLStorageModeManaged;
    }
    else
    {
        // Shared so the CPU can map the result after a GPU blit.
        return MTLStorageModeShared;
    }
}

} // anonymous namespace

// ---------------------------------------------------------------------------
//  Construction / destruction
// ---------------------------------------------------------------------------

MetalBuffer::MetalBuffer(Direction dir,
                         id<MTLDevice> device,
                         size_t byteCount,
                         unsigned int /*alignment*/)
    : m_direction(dir)
    , m_byteCount(byteCount)
    , m_available(true)
    , m_buffer(nil)
    , m_storageMode(MTLStorageModeShared)
{
    @autoreleasepool
    {
        if (!device)
        {
            throw std::invalid_argument("TwkMTLF::MetalBuffer: device is nil");
        }

        if (byteCount == 0)
        {
            throw std::invalid_argument("TwkMTLF::MetalBuffer: byteCount must be > 0");
        }

        m_storageMode = chooseStorageMode(device, dir);

        MTLResourceOptions options = static_cast<MTLResourceOptions>(m_storageMode);
        m_buffer = [device newBufferWithLength:byteCount options:options];

        if (!m_buffer)
        {
            throw std::runtime_error("TwkMTLF::MetalBuffer: "
                                     "newBufferWithLength failed — out of Metal memory?");
        }

        switch (dir)
        {
        case Direction::TO_GPU:
            m_buffer.label = @"TwkMTLF::MetalBuffer::toGPU";
            break;
        case Direction::FROM_GPU:
            m_buffer.label = @"TwkMTLF::MetalBuffer::fromGPU";
            break;
        }
    }
}

MetalBuffer::~MetalBuffer()
{
    m_buffer = nil;
}

// ---------------------------------------------------------------------------
//  Resize
// ---------------------------------------------------------------------------

void MetalBuffer::resize(id<MTLDevice> device, size_t newByteCount)
{
    @autoreleasepool
    {
        if (!device)
        {
            throw std::invalid_argument("TwkMTLF::MetalBuffer::resize: device is nil");
        }

        if (newByteCount == 0)
        {
            throw std::invalid_argument("TwkMTLF::MetalBuffer::resize: "
                                        "newByteCount must be > 0");
        }

        // Release old buffer first so we're not holding two allocations at once.
        m_buffer   = nil;
        m_byteCount = newByteCount;

        m_storageMode = chooseStorageMode(device, m_direction);
        MTLResourceOptions options = static_cast<MTLResourceOptions>(m_storageMode);

        m_buffer = [device newBufferWithLength:newByteCount options:options];
        if (!m_buffer)
        {
            throw std::runtime_error("TwkMTLF::MetalBuffer::resize: "
                                     "newBufferWithLength failed.");
        }
    }
}

// ---------------------------------------------------------------------------
//  Data access
// ---------------------------------------------------------------------------

void* MetalBuffer::contents() const
{
    if (!m_buffer)
        return nullptr;
    return m_buffer.contents;
}

void MetalBuffer::didModifyRange(size_t offset, size_t length) const
{
    if (!m_buffer)
        return;

#if TARGET_OS_OSX
    // didModifyRange is only needed (and only available) on macOS Managed
    // storage mode.  It is a no-op on Shared storage.
    if (m_storageMode == MTLStorageModeManaged)
    {
        [m_buffer didModifyRange:NSMakeRange(offset, length)];
    }
#endif
    (void)offset;
    (void)length;
}

// ---------------------------------------------------------------------------
//  Queries
// ---------------------------------------------------------------------------

bool MetalBuffer::isValid() const
{
    return m_buffer != nil;
}

// ---------------------------------------------------------------------------
//  Pool helper
// ---------------------------------------------------------------------------

MetalBuffer*
MetalBuffer::availableBuffer(MetalBufferVector& pool)
{
    for (MetalBuffer* b : pool)
    {
        if (b && b->isAvailable())
            return b;
    }
    return nullptr;
}

} // namespace TwkMTLF
