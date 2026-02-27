//
//  Copyright (c) 2024 Autodesk, Inc. All Rights Reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
#ifndef __TwkMTLF__MetalBuffer__h__
#define __TwkMTLF__MetalBuffer__h__

#include <cstddef>
#include <vector>

#ifdef __OBJC__
#import <Metal/Metal.h>
#endif

namespace TwkMTLF
{

    //
    //  MetalBuffer  —  Metal equivalent of TwkGLF::GLVBO + GLPixelBufferObject
    //
    //  Wraps an id<MTLBuffer> allocated with MTLResourceStorageModeShared on
    //  Apple Silicon (unified memory, zero-copy CPU/GPU access) or
    //  MTLResourceStorageModeManaged on Intel Macs (requires explicit sync).
    //
    //  Direction semantics match GLPixelBufferObject:
    //    TO_GPU   — CPU writes data, GPU reads (vertex/uniform upload).
    //    FROM_GPU — GPU writes data, CPU reads (async pixel readback).
    //
    //  Usage:
    //    MetalBuffer buf(MetalBuffer::TO_GPU, device, byteCount);
    //    memcpy(buf.contents(), srcData, byteCount);
    //    // ... bind to render encoder as vertex buffer ...
    //

    class MetalBuffer
    {
    public:
        enum class Direction
        {
            TO_GPU,  // CPU → GPU  (vertex, uniform, texture upload)
            FROM_GPU // GPU → CPU  (pixel readback)
        };

        // -------------------------------------------------------------------------
        //  Construction / destruction
        // -------------------------------------------------------------------------

#ifdef __OBJC__
        MetalBuffer(Direction dir, id<MTLDevice> device, size_t byteCount, unsigned int alignment = 1);
#endif

        ~MetalBuffer();

        // Non-copyable
        MetalBuffer(const MetalBuffer&) = delete;
        MetalBuffer& operator=(const MetalBuffer&) = delete;

        // -------------------------------------------------------------------------
        //  Resize — reallocates the underlying MTLBuffer.
        //  Any previously obtained contents() pointer is invalidated.
        // -------------------------------------------------------------------------

#ifdef __OBJC__
        void resize(id<MTLDevice> device, size_t newByteCount);
#endif

        // -------------------------------------------------------------------------
        //  Data access
        // -------------------------------------------------------------------------

        //  Direct CPU pointer — valid as long as the buffer is live.
        //  On Shared-storage devices this is zero-copy.
        //  On Managed devices, call didModifyRange() after CPU writes before
        //  submitting to the GPU.

        void* contents() const;

        //  On Managed storage (Intel Mac), inform Metal that the CPU has written
        //  to the given byte range so it can sync to GPU-side memory.
        void didModifyRange(size_t offset, size_t length) const;

        // -------------------------------------------------------------------------
        //  Queries
        // -------------------------------------------------------------------------

        size_t size() const { return m_byteCount; }

        bool isValid() const;

        Direction direction() const { return m_direction; }

        //  True when not currently bound to any encoder / command buffer.
        bool isAvailable() const { return m_available; }

        void makeAvailable() { m_available = true; }

        void makeUnavailable() { m_available = false; }

        // -------------------------------------------------------------------------
        //  Metal object accessor
        // -------------------------------------------------------------------------

#ifdef __OBJC__
        id<MTLBuffer> buffer() const { return m_buffer; }
#else
        void* buffer() const { return m_buffer_opaque; }
#endif

        // -------------------------------------------------------------------------
        //  Pool helper — same pattern as GLVBO::availableVBO
        // -------------------------------------------------------------------------

        using MetalBufferVector = std::vector<MetalBuffer*>;
        static MetalBuffer* availableBuffer(MetalBufferVector& pool);

    private:
        Direction m_direction;
        size_t m_byteCount;
        bool m_available;

#ifdef __OBJC__
        id<MTLBuffer> m_buffer;
        MTLStorageMode m_storageMode;
#else
        void* m_buffer_opaque;
        int m_storageMode_opaque;
#endif
    };

} // namespace TwkMTLF

#endif // __TwkMTLF__MetalBuffer__h__
