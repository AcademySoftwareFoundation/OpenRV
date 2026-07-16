//*****************************************************************************/
//
// Filename: FastMemcpy.h
//
// Copyright (c) 2019 Autodesk, Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//*****************************************************************************/

#ifndef __TwkFB__FastMemcpy__h__
#define __TwkFB__FastMemcpy__h__

#include <TwkFB/dll_defs.h>

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h> // for size_t
#include <stdint.h> // for uint32_t

#define FASTMEMCPYRESTRICT __restrict

    //
    // FastMemcpy is functionally equivalent to memcpy
    // However it has been optimized for speed for image size memory transfers.
    // The multi-core FastMemcpy_MP version is also available and provides a
    // boost of performances by leveraging all the memory channels available on
    // the host CPU.
    //
    TWKFB_EXPORT void* FastMemcpy(void* FASTMEMCPYRESTRICT outBuf, const void* FASTMEMCPYRESTRICT inBuf, size_t n);
    TWKFB_EXPORT void* FastMemcpy_MP(void* FASTMEMCPYRESTRICT outBuf, const void* FASTMEMCPYRESTRICT inBuf, size_t n);

    TWKFB_EXPORT void subsample422_8bit_UYVY(size_t width, size_t height, const uint8_t* FASTMEMCPYRESTRICT inBuf,
                                             uint8_t* FASTMEMCPYRESTRICT outBuf);
    TWKFB_EXPORT void subsample422_8bit_UYVY_MP(size_t width, size_t height, const uint8_t* FASTMEMCPYRESTRICT inBuf,
                                                uint8_t* FASTMEMCPYRESTRICT outBuf);

    /// @brief Converts packed YUV-444 10-bits to packed YUV-422 10-bits.
    ///
    /// @param width The number of pixels per row.
    /// @param height The number of rows.
    /// @param inBuf The input buffer.
    /// @param outBuf The output buffer.
    /// @param inBufStride The stride of the output buffer in bytes.
    /// @param outBufStride The stride of the input buffer in bytes.
    TWKFB_EXPORT void subsample422_10bit(size_t width, size_t height, const uint32_t* FASTMEMCPYRESTRICT inBuf,
                                         uint32_t* FASTMEMCPYRESTRICT outBuf, size_t inBufStride, size_t outBufStride);

    /// @brief Converts packed YUV-444 10-bits to packed YUV-422 10-bits
    /// multithreaded
    ///
    /// @param width The number of pixels per row.
    /// @param height The number of rows.
    /// @param inBuf The input buffer.
    /// @param outBuf The output buffer.
    /// @param inBufStride The stride of the output buffer in bytes.
    /// @param outBufStride The stride of the input buffer in bytes.
    TWKFB_EXPORT void subsample422_10bit_MP(size_t width, size_t height, const uint32_t* FASTMEMCPYRESTRICT inBuf,
                                            uint32_t* FASTMEMCPYRESTRICT outBuf, size_t inBufStride, size_t outBufStride);

    TWKFB_EXPORT void swap_bytes_32bit(size_t width, size_t height, const uint32_t* FASTMEMCPYRESTRICT inBuf,
                                       uint32_t* FASTMEMCPYRESTRICT outBuf);
    TWKFB_EXPORT void swap_bytes_32bit_MP(size_t width, size_t height, const uint32_t* FASTMEMCPYRESTRICT inBuf,
                                          uint32_t* FASTMEMCPYRESTRICT outBuf);

    /// @brief Expand packed 3-component RGB to packed 4-component RGBA by
    /// inserting an opaque alpha, for 16-bit components (half float).
    ///
    /// GPUs have no fast DMA path for 3-component float uploads (GL_RGB16F);
    /// the driver expands RGB->RGBA per pixel on upload. Doing the expansion on
    /// the CPU here lets the texture use the native GL_RGBA16F transfer path.
    ///
    /// @param width Pixels per row.
    /// @param height Number of rows.
    /// @param inBuf Source RGB buffer.
    /// @param inRowBytes Source scanline stride in bytes (>= width*3*2).
    /// @param outBuf Destination RGBA buffer (tightly packed, width*4*2 bytes/row).
    /// @param alpha Raw 16-bit alpha value to insert (0x3C00 == half 1.0).
    TWKFB_EXPORT void expand_rgb_to_rgba_16bit(size_t width, size_t height, const uint8_t* FASTMEMCPYRESTRICT inBuf, size_t inRowBytes,
                                               uint16_t* FASTMEMCPYRESTRICT outBuf, uint16_t alpha);
    TWKFB_EXPORT void expand_rgb_to_rgba_16bit_MP(size_t width, size_t height, const uint8_t* FASTMEMCPYRESTRICT inBuf, size_t inRowBytes,
                                                  uint16_t* FASTMEMCPYRESTRICT outBuf, uint16_t alpha);

    /// @brief Expand packed 3-component RGB to packed 4-component RGBA for
    /// 32-bit components (full float). @see expand_rgb_to_rgba_16bit.
    ///
    /// @param alpha Raw 32-bit alpha value to insert (0x3F800000 == 1.0f).
    TWKFB_EXPORT void expand_rgb_to_rgba_32bit(size_t width, size_t height, const uint8_t* FASTMEMCPYRESTRICT inBuf, size_t inRowBytes,
                                               uint32_t* FASTMEMCPYRESTRICT outBuf, uint32_t alpha);
    TWKFB_EXPORT void expand_rgb_to_rgba_32bit_MP(size_t width, size_t height, const uint8_t* FASTMEMCPYRESTRICT inBuf, size_t inRowBytes,
                                                  uint32_t* FASTMEMCPYRESTRICT outBuf, uint32_t alpha);

#ifdef __cplusplus
}
#endif

#endif //__TwkFB__FastMemcpy__h__
