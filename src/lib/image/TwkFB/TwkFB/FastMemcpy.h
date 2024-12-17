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
    TWKFB_EXPORT void* FastMemcpy(void* FASTMEMCPYRESTRICT outBuf,
                                  const void* FASTMEMCPYRESTRICT inBuf,
                                  size_t n);
    TWKFB_EXPORT void* FastMemcpy_MP(void* FASTMEMCPYRESTRICT outBuf,
                                     const void* FASTMEMCPYRESTRICT inBuf,
                                     size_t n);

    TWKFB_EXPORT void
    subsample422_8bit_UYVY(size_t width, size_t height,
                           const uint8_t* FASTMEMCPYRESTRICT inBuf,
                           uint8_t* FASTMEMCPYRESTRICT outBuf);
    TWKFB_EXPORT void
    subsample422_8bit_UYVY_MP(size_t width, size_t height,
                              const uint8_t* FASTMEMCPYRESTRICT inBuf,
                              uint8_t* FASTMEMCPYRESTRICT outBuf);

    /// @brief Converts packed YUV-444 10-bits to packed YUV-422 10-bits.
    ///
    /// @param width The number of pixels per row.
    /// @param height The number of rows.
    /// @param inBuf The input buffer.
    /// @param outBuf The output buffer.
    /// @param inBufStride The stride of the output buffer in bytes.
    /// @param outBufStride The stride of the input buffer in bytes.
    TWKFB_EXPORT void
    subsample422_10bit(size_t width, size_t height,
                       const uint32_t* FASTMEMCPYRESTRICT inBuf,
                       uint32_t* FASTMEMCPYRESTRICT outBuf, size_t inBufStride,
                       size_t outBufStride);

    /// @brief Converts packed YUV-444 10-bits to packed YUV-422 10-bits
    /// multithreaded
    ///
    /// @param width The number of pixels per row.
    /// @param height The number of rows.
    /// @param inBuf The input buffer.
    /// @param outBuf The output buffer.
    /// @param inBufStride The stride of the output buffer in bytes.
    /// @param outBufStride The stride of the input buffer in bytes.
    TWKFB_EXPORT void
    subsample422_10bit_MP(size_t width, size_t height,
                          const uint32_t* FASTMEMCPYRESTRICT inBuf,
                          uint32_t* FASTMEMCPYRESTRICT outBuf,
                          size_t inBufStride, size_t outBufStride);

    TWKFB_EXPORT void swap_bytes_32bit(size_t width, size_t height,
                                       const uint32_t* FASTMEMCPYRESTRICT inBuf,
                                       uint32_t* FASTMEMCPYRESTRICT outBuf);
    TWKFB_EXPORT void
    swap_bytes_32bit_MP(size_t width, size_t height,
                        const uint32_t* FASTMEMCPYRESTRICT inBuf,
                        uint32_t* FASTMEMCPYRESTRICT outBuf);

#ifdef __cplusplus
}
#endif

#endif //__TwkFB__FastMemcpy__h__
