//*****************************************************************************/
// Copyright (c) 2019 Autodesk, Inc.
// All rights reserved.
// 
// SPDX-License-Identifier: Apache-2.0
//
//*****************************************************************************/

#ifndef __TwkFB__FastConversion__h__
#define __TwkFB__FastConversion__h__

#include <TwkFB/dll_defs.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>  // for size_t
#include <stdint.h>  // for uint32_t

#define FASTMEMCPYRESTRICT __restrict

// Converts ABGR10 to RGBA10
// From ABGR10:
// M                                                             L
// A1A0B9B8B7B6B5B4B3B2B1B0G9G8G7G6G5G4G3G2G1G0R9R8R7R6R5R4R3R2R1R0
// To RGBA10:
// M                                                             L
// R9R8R7R6R5R4R3R2R1R0G9G8G7G6G5G4G3G2G1G0B9B8B7B6B5B4B3B2B1B0A1A0
TWKFB_EXPORT void convert_ABGR10_to_RGBA10( size_t width, size_t height, const uint32_t*FASTMEMCPYRESTRICT inBuf, uint32_t*FASTMEMCPYRESTRICT outBuf );
TWKFB_EXPORT void convert_ABGR10_to_RGBA10_MP( size_t width, size_t height, const uint32_t*FASTMEMCPYRESTRICT inBuf, uint32_t*FASTMEMCPYRESTRICT outBuf );

// Converts RGB16 to RGB12
//
/// @brief Converts packed RGB 16 bits per component to packed RGB 12 bits per component.
///
/// From RGB16 where each component is 4 bits and each boundary is 16 bits:
/// M                                     L
/// B0 B0 B0 XX | G0 G0 G0 XX | R0 R0 R0 XX 
/// B1 B1 B1 XX | G1 G1 G1 XX | R1 R1 R1 XX 
/// B2 B2 B2 XX | G2 G2 G2 XX | R2 R2 R2 XX 
/// B3 B3 B3 XX | G3 G3 G3 XX | R3 R3 R3 XX 
/// To RGBA12 where each component is 4 bits and each boundary is 16 bits:
/// M                                     L
/// R1 R1 R1 B0 | B0 B0 G0 G0 | G0 R0 R0 R0
/// G2 G2 G2 R2 | R2 R2 B1 B1 | B1 G1 G1 G1
/// B3 B3 B3 G3 | G3 G3 R3 R3 | R3 B2 B2 B2
///
/// @param width The number of pixels per row.
/// @param height The number of rows.
/// @param inBuf The input buffer.
/// @param outBuf The output buffer.
/// @param inBufStride The stride of the output buffer in bytes.
/// @param outBufStride The stride of the input buffer in bytes.
TWKFB_EXPORT void convert_RGB16_to_RGB12(    size_t width, 
                                size_t height, 
                                const void* FASTMEMCPYRESTRICT inBuf, 
                                void* FASTMEMCPYRESTRICT outBuf, 
                                size_t inBufStride, 
                                size_t outBufStride );
TWKFB_EXPORT void convert_RGB16_to_RGB12_MP( size_t width, 
                                size_t height, 
                                const void* FASTMEMCPYRESTRICT inBuf, 
                                void* FASTMEMCPYRESTRICT outBuf, 
                                size_t inBufStride, 
                                size_t outBufStride );

/// @brief Converts packed UYVY 10-bits to planar YUV 16-bits.
///
/// @param width The number of bytes per row.
/// @param height The number of rows.
/// @param inBuf The input buffer.
/// @param outY The output channel buffer of the Y component.
/// @param outCb The output channel buffer of the Cb component.
/// @param outCr The output channel buffer of the Cr component.
/// @param strideY The stride of the output channel buffer of the Y component in bytes.
/// @param strideCb The stride of the output channel buffer of the Cb component in bytes.
/// @param strideCr The stride of the output channel buffer of the Cr component in bytes.
TWKFB_EXPORT void packedUYVY10_to_planarYUV16( size_t width, size_t height, const uint32_t* FASTMEMCPYRESTRICT inBuf, uint16_t* FASTMEMCPYRESTRICT outY, uint16_t* FASTMEMCPYRESTRICT outCb, uint16_t* FASTMEMCPYRESTRICT outCr, size_t strideY, size_t strideCb, size_t strideCr );
TWKFB_EXPORT void packedUYVY10_to_planarYUV16_MP( size_t width, size_t height, const uint32_t* FASTMEMCPYRESTRICT inBuf, uint16_t* FASTMEMCPYRESTRICT outY, uint16_t* FASTMEMCPYRESTRICT outCb, uint16_t* FASTMEMCPYRESTRICT outCr, size_t strideY, size_t strideCb, size_t strideCr );

/// @brief Converts packed UYVY 16-bits to planar YUV 16-bits.
///
/// @param width The number of bytes per row.
/// @param height The number of rows.
/// @param inBuf The input buffer.
/// @param outY The output channel buffer of the Y component.
/// @param outCb The output channel buffer of the Cb component.
/// @param outCr The output channel buffer of the Cr component.
/// @param strideY The stride of the output channel buffer of the Y component in bytes.
/// @param strideCb The stride of the output channel buffer of the Cb component in bytes.
/// @param strideCr The stride of the output channel buffer of the Cr component in bytes.
TWKFB_EXPORT void packedUYVY16_to_planarYUV16( size_t width, size_t height, const uint16_t* FASTMEMCPYRESTRICT inBuf, uint16_t* FASTMEMCPYRESTRICT outY, uint16_t* FASTMEMCPYRESTRICT outCb, uint16_t* FASTMEMCPYRESTRICT outCr, size_t strideY, size_t strideCb, size_t strideCr );
TWKFB_EXPORT void packedUYVY16_to_planarYUV16_MP( size_t width, size_t height, const uint16_t* FASTMEMCPYRESTRICT inBuf, uint16_t* FASTMEMCPYRESTRICT outY, uint16_t* FASTMEMCPYRESTRICT outCb, uint16_t* FASTMEMCPYRESTRICT outCr, size_t strideY, size_t strideCb, size_t strideCr );

/// @brief Converts packed UVYA 16-bits to planar YUVA 16-bits.
///
/// @param width The number of bytes per row.
/// @param height The number of rows.
/// @param inBuf The input buffer.
/// @param outY The output channel buffer of the Y component.
/// @param outCb The output channel buffer of the Cb component.
/// @param outCr The output channel buffer of the Cr component.
/// @param outA The output channel buffer of the A component.
/// @param strideY The stride of the output channel buffer of the Y component in bytes.
/// @param strideCb The stride of the output channel buffer of the Cb component in bytes.
/// @param strideCr The stride of the output channel buffer of the Cr component in bytes.
/// @param strideA The stride of the output channel buffer of the A component in bytes.
TWKFB_EXPORT void packedUVYA16_to_planarYUVA16( size_t width, size_t height, const uint64_t* FASTMEMCPYRESTRICT inBuf, uint16_t* FASTMEMCPYRESTRICT outY, uint16_t* FASTMEMCPYRESTRICT outCb, uint16_t* FASTMEMCPYRESTRICT outCr, uint16_t* FASTMEMCPYRESTRICT outA, size_t strideY, size_t strideCb, size_t strideCr, size_t strideA );
TWKFB_EXPORT void packedUVYA16_to_planarYUVA16_MP( size_t width, size_t height, const uint64_t* FASTMEMCPYRESTRICT inBuf, uint16_t* FASTMEMCPYRESTRICT outY, uint16_t* FASTMEMCPYRESTRICT outCb, uint16_t* FASTMEMCPYRESTRICT outCr, uint16_t* FASTMEMCPYRESTRICT outA, size_t strideY, size_t strideCb, size_t strideCr, size_t strideA );

/// @brief Converts packed YUV-444 10-bits to semi-planar YUV-422 16-bits (P216)
///
/// @param width The number of pixels per row.
/// @param height The number of rows.
/// @param inBuf The input buffer.
/// @param outBufY The output buffer Y.
/// @param outBufCbCr The output buffer CbCr
/// @param inBufStride The stride of the output buffer in bytes.
/// @param outBufStride The stride of the input buffer in bytes.
TWKFB_EXPORT void packedYUV444_10bits_to_P216( size_t width, size_t height, const uint32_t*FASTMEMCPYRESTRICT inBuf, 
    uint16_t*FASTMEMCPYRESTRICT outBufY, uint16_t*FASTMEMCPYRESTRICT outBufCbC, 
    size_t inBufStride, size_t outBufStride, bool flip );
TWKFB_EXPORT void packedYUV444_10bits_to_P216_MP( size_t width, size_t height, const uint32_t*FASTMEMCPYRESTRICT inBuf, 
    uint16_t*FASTMEMCPYRESTRICT outBufY, uint16_t*FASTMEMCPYRESTRICT outBufCbC, 
    size_t inBufStride, size_t outBufStride, bool flip );

/// @brief Converts packed BGRA 64-bits BE to packed ABGR 64-bits LE.
///
/// @param width The number of bytes per row.
/// @param height The number of rows.
/// @param inBuf The input buffer.
/// @param outBuf The output buffer.
/// @param outStride The stride of the output buffer in bytes.
TWKFB_EXPORT void packedBGRA64BE_to_packedABGR64LE( size_t width, size_t height, const uint64_t* FASTMEMCPYRESTRICT inBuf, uint64_t* FASTMEMCPYRESTRICT outBuf, size_t outStride );
TWKFB_EXPORT void packedBGRA64BE_to_packedABGR64LE_MP( size_t width, size_t height, const uint64_t* FASTMEMCPYRESTRICT inBuf, uint64_t* FASTMEMCPYRESTRICT outBuf, size_t outStride );

#ifdef __cplusplus
}
#endif

#endif //__TwkFB__FastConversion__h__
