//*****************************************************************************/
// Copyright (c) 2019 Autodesk, Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//*****************************************************************************/

#include <TwkFB/FastConversion.h>

#include <TwkFB/TwkFBThreadPool.h>
#include <TwkUtil/sgcHop.h>

#include <algorithm>

using namespace ILMTHREAD_NAMESPACE;

//------------------------------------------------------------------------------
//
void convert_ABGR10_to_RGBA10(size_t width, size_t height,
                              const uint32_t* FASTMEMCPYRESTRICT inBuf,
                              uint32_t* FASTMEMCPYRESTRICT outBuf)
{
    uint32_t* FASTMEMCPYRESTRICT p1 = outBuf;

    for (size_t row = 0; row < height; row++)
    {
        for (const uint32_t *FASTMEMCPYRESTRICT
                 p0 = inBuf + row * width,
                 *FASTMEMCPYRESTRICT e = p0 + width;
             p0 < e; p0++)
        {
            // Converts ABGR10 to RGBA10
            // From ABGR10:
            // M                                                             L
            // A1A0B9B8B7B6B5B4B3B2B1B0G9G8G7G6G5G4G3G2G1G0R9R8R7R6R5R4R3R2R1R0
            // To RGBA10:
            // M                                                             L
            // R9R8R7R6R5R4R3R2R1R0G9G8G7G6G5G4G3G2G1G0B9B8B7B6B5B4B3B2B1B0A1A0
            const uint32_t a = *p0;
            *p1 = (a >> 30) |                /* alpha */
                  ((a & 0x3FF00000) >> 18) | /* blue */
                  ((a & 0x000FFC00) << 2) |  /* green */
                  (a << 22);                 /* red */
            p1++;
        }
    }
}

//------------------------------------------------------------------------------
//
class Convert_ABGR10_to_RGBA10_MP_Task : public Task
{
public:
    Convert_ABGR10_to_RGBA10_MP_Task(TaskGroup* group, size_t width,
                                     size_t height,
                                     const uint32_t* FASTMEMCPYRESTRICT inBuf,
                                     uint32_t* FASTMEMCPYRESTRICT outBuf)
        : Task(group)
        , _width(width)
        , _height(height)
        , _inBuf(inBuf)
        , _outBuf(outBuf)
    {
    }

    virtual ~Convert_ABGR10_to_RGBA10_MP_Task() {}

    virtual void execute()
    {
        convert_ABGR10_to_RGBA10(_width, _height, _inBuf, _outBuf);
    }

    const size_t _width;
    const size_t _height;
    const uint32_t* FASTMEMCPYRESTRICT _inBuf;
    uint32_t* FASTMEMCPYRESTRICT _outBuf;
};

//------------------------------------------------------------------------------
//
void convert_ABGR10_to_RGBA10_MP(size_t width, size_t height,
                                 const uint32_t* FASTMEMCPYRESTRICT inBuf,
                                 uint32_t* FASTMEMCPYRESTRICT outBuf)
{
    static bool use_standard_memcpy = getenv("RV_USE_STD_MEMCPY");
    if (use_standard_memcpy)
    {
        HOP_PROF("convert_ABGR10_to_RGBA10()");
        convert_ABGR10_to_RGBA10(width, height, inBuf, outBuf);
        return;
    }

    HOP_PROF_FUNC();

    const size_t taskHeight = height / TwkFB::ThreadPool::getNumThreads();
    const size_t bufStride = width;

    size_t curY = 0;

    TaskGroup taskGroup;

    while (curY < height)
    {
        const uint32_t* FASTMEMCPYRESTRICT curInBuf = inBuf + curY * bufStride;
        uint32_t* FASTMEMCPYRESTRICT curOutBuf = outBuf + curY * bufStride;
        const size_t curHeight = std::min(taskHeight, height - curY);
        TwkFB::ThreadPool::addTask(new Convert_ABGR10_to_RGBA10_MP_Task(
            &taskGroup, width, curHeight, curInBuf, curOutBuf));
        curY += curHeight;
    }
}

//------------------------------------------------------------------------------
//
void convert_RGB16_to_RGB12(size_t width, size_t height,
                            const void* FASTMEMCPYRESTRICT inBuf,
                            void* FASTMEMCPYRESTRICT outBuf, size_t inBufStride,
                            size_t outBufStride)
{
    const size_t src_uint16_per_row = inBufStride / sizeof(uint16_t);
    const size_t dst_uint16_per_row = outBufStride / sizeof(uint16_t);

    const uint16_t* FASTMEMCPYRESTRICT src =
        reinterpret_cast<const uint16_t*>(inBuf);
    uint16_t* FASTMEMCPYRESTRICT dst = reinterpret_cast<uint16_t*>(outBuf);

    // Convert :
    // From RGB16 where each component is 4 bits and each boundary is 16 bits:
    // M                                     L
    // B0 B0 B0 XX | G0 G0 G0 XX | R0 R0 R0 XX
    // B1 B1 B1 XX | G1 G1 G1 XX | R1 R1 R1 XX
    // B2 B2 B2 XX | G2 G2 G2 XX | R2 R2 R2 XX
    // B3 B3 B3 XX | G3 G3 G3 XX | R3 R3 R3 XX
    // To RGBA12 where each component is 4 bits and each boundary is 16 bits:
    // M                                     L
    // R1 R1 R1 B0 | B0 B0 G0 G0 | G0 R0 R0 R0
    // G2 G2 G2 R2 | R2 R2 B1 B1 | B1 G1 G1 G1
    // B3 B3 B3 G3 | G3 G3 R3 R3 | R3 B2 B2 B2

    for (size_t y = 0; y < height; y++)
    {
        for (size_t x = 0; x < width; x += 4)
        {
            const uint16_t r0 = src[x * 3 + 0] >> 4;
            const uint16_t g0 = src[x * 3 + 1] >> 4;
            const uint16_t b0 = src[x * 3 + 2] >> 4;

            const uint16_t r1 = src[x * 3 + 3] >> 4;
            const uint16_t g1 = src[x * 3 + 4] >> 4;
            const uint16_t b1 = src[x * 3 + 5] >> 4;

            const uint16_t r2 = src[x * 3 + 6] >> 4;
            const uint16_t g2 = src[x * 3 + 7] >> 4;
            const uint16_t b2 = src[x * 3 + 8] >> 4;

            const uint16_t r3 = src[x * 3 + 9] >> 4;
            const uint16_t g3 = src[x * 3 + 10] >> 4;
            const uint16_t b3 = src[x * 3 + 11] >> 4;

            dst[x * 9 / 4 + 0] = ((g0 & 0xF) << 12) | r0;
            dst[x * 9 / 4 + 1] = ((b0 & 0xFF) << 8) | ((g0 & 0xFF0) >> 4);
            dst[x * 9 / 4 + 2] = (r1 << 4) | ((b0 & 0xF00) >> 8);
            ;

            dst[x * 9 / 4 + 3] = ((b1 & 0xF) << 12) | g1;
            dst[x * 9 / 4 + 4] = ((r2 & 0xFF) << 8) | ((b1 & 0xFF0) >> 4);
            dst[x * 9 / 4 + 5] = (g2 << 4) | ((r2 & 0xF00) >> 8);
            ;

            dst[x * 9 / 4 + 6] = ((r3 & 0xF) << 12) | b2;
            dst[x * 9 / 4 + 7] = ((g3 & 0xFF) << 8) | ((r3 & 0xFF0) >> 4);
            dst[x * 9 / 4 + 8] = (b3 << 4) | ((g3 & 0xF00) >> 8);
            ;
        }

        src += src_uint16_per_row;
        dst += dst_uint16_per_row;
    }
}

//------------------------------------------------------------------------------
//
class Convert_RGB16_to_RGB12_MP_Task : public Task
{
public:
    Convert_RGB16_to_RGB12_MP_Task(TaskGroup* group, size_t width,
                                   size_t height,
                                   const void* FASTMEMCPYRESTRICT inBuf,
                                   void* FASTMEMCPYRESTRICT outBuf,
                                   size_t inBufStride, size_t outBufStride)
        : Task(group)
        , _width(width)
        , _height(height)
        , _inBuf(inBuf)
        , _outBuf(outBuf)
        , _inBufStride(inBufStride)
        , _outBufStride(outBufStride)
    {
    }

    virtual ~Convert_RGB16_to_RGB12_MP_Task() {}

    virtual void execute()
    {
        convert_RGB16_to_RGB12(_width, _height, _inBuf, _outBuf, _inBufStride,
                               _outBufStride);
    }

    const size_t _width;
    const size_t _height;
    const void* FASTMEMCPYRESTRICT _inBuf;
    void* FASTMEMCPYRESTRICT _outBuf;
    const size_t _inBufStride;
    const size_t _outBufStride;
};

//------------------------------------------------------------------------------
//
void convert_RGB16_to_RGB12_MP(size_t width, size_t height,
                               const void* FASTMEMCPYRESTRICT inBuf,
                               void* FASTMEMCPYRESTRICT outBuf,
                               size_t inBufStride, size_t outBufStride)
{
    HOP_PROF_FUNC();

    const size_t taskHeight = height / TwkFB::ThreadPool::getNumThreads();

    size_t curY = 0;

    TaskGroup taskGroup;

    while (curY < height)
    {
        const void* FASTMEMCPYRESTRICT curInBuf =
            reinterpret_cast<const uint8_t*>(inBuf) + curY * inBufStride;
        void* FASTMEMCPYRESTRICT curOutBuf =
            reinterpret_cast<uint8_t*>(outBuf) + curY * outBufStride;
        const size_t curHeight = std::min(taskHeight, height - curY);
        TwkFB::ThreadPool::addTask(new Convert_RGB16_to_RGB12_MP_Task(
            &taskGroup, width, curHeight, curInBuf, curOutBuf, inBufStride,
            outBufStride));
        curY += curHeight;
    }
}

//------------------------------------------------------------------------------
// The following packedUYVY10_to_planarYUV16 functions can be used to pass
// from packed UYVY with 10 bits per component to planar YUV with 16 bits
// per component.
// See v210 at http://developer.apple.com/library/mac/technotes/tn2162/
//
void packedUYVY10_to_planarYUV16(size_t width, size_t height,
                                 const uint32_t* FASTMEMCPYRESTRICT inBuf,
                                 uint16_t* FASTMEMCPYRESTRICT outY,
                                 uint16_t* FASTMEMCPYRESTRICT outCb,
                                 uint16_t* FASTMEMCPYRESTRICT outCr,
                                 size_t strideY, size_t strideCb,
                                 size_t strideCr)
{
    const size_t nbPixelGroups = width / 16;
    uint16_t* FASTMEMCPYRESTRICT startOutY = outY;
    uint16_t* FASTMEMCPYRESTRICT startOutCb = outCb;
    uint16_t* FASTMEMCPYRESTRICT startOutCr = outCr;
    for (size_t i = 0; i < height; ++i)
    {
        outY = startOutY + i * strideY / 2;
        outCb = startOutCb + i * strideCb / 2;
        outCr = startOutCr + i * strideCr / 2;
        for (size_t j = 0; j < nbPixelGroups; ++j)
        {
            *outCb = (*inBuf) & 0x000003FF;
            ++outCb;
            *outY = ((*inBuf) & 0x000FFC00) >> 10;
            ++outY;
            *outCr = ((*inBuf) & 0x3FF00000) >> 20;
            ++outCr;
            ++inBuf;

            *outY = (*inBuf) & 0x000003FF;
            ++outY;
            *outCb = ((*inBuf) & 0x000FFC00) >> 10;
            ++outCb;
            *outY = ((*inBuf) & 0x3FF00000) >> 20;
            ++outY;
            ++inBuf;

            *outCr = (*inBuf) & 0x000003FF;
            ++outCr;
            *outY = ((*inBuf) & 0x000FFC00) >> 10;
            ++outY;
            *outCb = ((*inBuf) & 0x3FF00000) >> 20;
            ++outCb;
            ++inBuf;

            *outY = (*inBuf) & 0x000003FF;
            ++outY;
            *outCr = ((*inBuf) & 0x000FFC00) >> 10;
            ++outCr;
            *outY = ((*inBuf) & 0x3FF00000) >> 20;
            ++outY;
            ++inBuf;
        }
    }
}

//------------------------------------------------------------------------------
//
class PackedUYVY10_to_planarYUV16_Task : public Task
{
public:
    PackedUYVY10_to_planarYUV16_Task(TaskGroup* group, size_t width,
                                     size_t height,
                                     const uint32_t* FASTMEMCPYRESTRICT inBuf,
                                     uint16_t* FASTMEMCPYRESTRICT outY,
                                     uint16_t* FASTMEMCPYRESTRICT outCb,
                                     uint16_t* FASTMEMCPYRESTRICT outCr,
                                     size_t strideY, size_t strideCb,
                                     size_t strideCr)
        : Task(group)
        , _width(width)
        , _height(height)
        , _inBuf(inBuf)
        , _outY(outY)
        , _outCb(outCb)
        , _outCr(outCr)
        , _strideY(strideY)
        , _strideCb(strideCb)
        , _strideCr(strideCr)
    {
    }

    virtual ~PackedUYVY10_to_planarYUV16_Task() {}

    virtual void execute()
    {
        packedUYVY10_to_planarYUV16(_width, _height, _inBuf, _outY, _outCb,
                                    _outCr, _strideY, _strideCb, _strideCr);
    }

private:
    const size_t _width;
    const size_t _height;
    const uint32_t* FASTMEMCPYRESTRICT _inBuf;
    uint16_t* FASTMEMCPYRESTRICT _outY;
    uint16_t* FASTMEMCPYRESTRICT _outCb;
    uint16_t* FASTMEMCPYRESTRICT _outCr;
    const size_t _strideY;
    const size_t _strideCb;
    const size_t _strideCr;
};

//------------------------------------------------------------------------------
//
void packedUYVY10_to_planarYUV16_MP(size_t width, size_t height,
                                    const uint32_t* FASTMEMCPYRESTRICT inBuf,
                                    uint16_t* FASTMEMCPYRESTRICT outY,
                                    uint16_t* FASTMEMCPYRESTRICT outCb,
                                    uint16_t* FASTMEMCPYRESTRICT outCr,
                                    size_t strideY, size_t strideCb,
                                    size_t strideCr)
{
    static bool use_standard_memcpy = getenv("RV_USE_STD_MEMCPY");
    if (use_standard_memcpy)
    {
        HOP_PROF("packedUYVY10_to_planarYUV16()");
        packedUYVY10_to_planarYUV16(width, height, inBuf, outY, outCb, outCr,
                                    strideY, strideCb, strideCr);
        return;
    }

    HOP_PROF_FUNC();

    const size_t taskHeight = height / TwkFB::ThreadPool::getNumThreads();
    const size_t bufStride = width / 4;

    size_t curY = 0;

    TaskGroup taskGroup;

    while (curY < height)
    {
        // Need to divide the strides (in bytes) by 2 since incrementing a
        // uint16_t pointer by 1 is moving 2 bytes forward.
        const uint32_t* FASTMEMCPYRESTRICT curInBuf = inBuf + curY * bufStride;
        uint16_t* FASTMEMCPYRESTRICT curOutY = outY + curY * strideY / 2;
        uint16_t* FASTMEMCPYRESTRICT curOutCb = outCb + curY * strideCb / 2;
        uint16_t* FASTMEMCPYRESTRICT curOutCr = outCr + curY * strideCr / 2;
        const size_t curHeight = std::min(taskHeight, height - curY);
        TwkFB::ThreadPool::addTask(new PackedUYVY10_to_planarYUV16_Task(
            &taskGroup, width, curHeight, curInBuf, curOutY, curOutCb, curOutCr,
            strideY, strideCb, strideCr));
        curY += curHeight;
    }
}

//------------------------------------------------------------------------------
// The following packedUYVY16_to_planarYUV16 functions can be used to pass
// from packed UYVY with 16 bits per component to planar YUV with 16 bits
// per component.
// See v216 at http://developer.apple.com/library/mac/technotes/tn2162/
//
void packedUYVY16_to_planarYUV16(size_t width, size_t height,
                                 const uint16_t* FASTMEMCPYRESTRICT inBuf,
                                 uint16_t* FASTMEMCPYRESTRICT outY,
                                 uint16_t* FASTMEMCPYRESTRICT outCb,
                                 uint16_t* FASTMEMCPYRESTRICT outCr,
                                 size_t strideY, size_t strideCb,
                                 size_t strideCr)
{
    const size_t nbPixelGroups = width / 8;
    uint16_t* FASTMEMCPYRESTRICT startOutY = outY;
    uint16_t* FASTMEMCPYRESTRICT startOutCb = outCb;
    uint16_t* FASTMEMCPYRESTRICT startOutCr = outCr;
    for (size_t i = 0; i < height; ++i)
    {
        outY = startOutY + i * strideY / 2;
        outCb = startOutCb + i * strideCb / 2;
        outCr = startOutCr + i * strideCr / 2;
        for (size_t j = 0; j < nbPixelGroups; ++j)
        {
            *outCb = *inBuf;
            ++outCb;
            ++inBuf;
            *outY = *inBuf;
            ++outY;
            ++inBuf;
            *outCr = *inBuf;
            ++outCr;
            ++inBuf;
            *outY = *inBuf;
            ++outY;
            ++inBuf;
        }
    }
}

//------------------------------------------------------------------------------
//
class PackedUYVY16_to_planarYUV16_Task : public Task
{
public:
    PackedUYVY16_to_planarYUV16_Task(TaskGroup* group, size_t width,
                                     size_t height,
                                     const uint16_t* FASTMEMCPYRESTRICT inBuf,
                                     uint16_t* FASTMEMCPYRESTRICT outY,
                                     uint16_t* FASTMEMCPYRESTRICT outCb,
                                     uint16_t* FASTMEMCPYRESTRICT outCr,
                                     size_t strideY, size_t strideCb,
                                     size_t strideCr)
        : Task(group)
        , _width(width)
        , _height(height)
        , _inBuf(inBuf)
        , _outY(outY)
        , _outCb(outCb)
        , _outCr(outCr)
        , _strideY(strideY)
        , _strideCb(strideCb)
        , _strideCr(strideCr)
    {
    }

    virtual ~PackedUYVY16_to_planarYUV16_Task() {}

    virtual void execute()
    {
        packedUYVY16_to_planarYUV16(_width, _height, _inBuf, _outY, _outCb,
                                    _outCr, _strideY, _strideCb, _strideCr);
    }

private:
    const size_t _width;
    const size_t _height;
    const uint16_t* FASTMEMCPYRESTRICT _inBuf;
    uint16_t* FASTMEMCPYRESTRICT _outY;
    uint16_t* FASTMEMCPYRESTRICT _outCb;
    uint16_t* FASTMEMCPYRESTRICT _outCr;
    const size_t _strideY;
    const size_t _strideCb;
    const size_t _strideCr;
};

void packedUYVY16_to_planarYUV16_MP(size_t width, size_t height,
                                    const uint16_t* FASTMEMCPYRESTRICT inBuf,
                                    uint16_t* FASTMEMCPYRESTRICT outY,
                                    uint16_t* FASTMEMCPYRESTRICT outCb,
                                    uint16_t* FASTMEMCPYRESTRICT outCr,
                                    size_t strideY, size_t strideCb,
                                    size_t strideCr)
{
    static bool use_standard_memcpy = getenv("RV_USE_STD_MEMCPY");
    if (use_standard_memcpy)
    {
        HOP_PROF("packedUYVY16_to_planarYUV16()");
        packedUYVY16_to_planarYUV16(width, height, inBuf, outY, outCb, outCr,
                                    strideY, strideCb, strideCr);
        return;
    }

    HOP_PROF_FUNC();

    const size_t taskHeight = height / TwkFB::ThreadPool::getNumThreads();
    const size_t bufStride = width / 2;

    size_t curY = 0;

    TaskGroup taskGroup;

    while (curY < height)
    {
        // Need to divide the strides (in bytes) by 2 since incrementing a
        // uint16_t pointer by 1 is moving 2 bytes forward.
        const uint16_t* FASTMEMCPYRESTRICT curInBuf = inBuf + curY * bufStride;
        uint16_t* FASTMEMCPYRESTRICT curOutY = outY + curY * strideY / 2;
        uint16_t* FASTMEMCPYRESTRICT curOutCb = outCb + curY * strideCb / 2;
        uint16_t* FASTMEMCPYRESTRICT curOutCr = outCr + curY * strideCr / 2;
        const size_t curHeight = std::min(taskHeight, height - curY);
        TwkFB::ThreadPool::addTask(new PackedUYVY16_to_planarYUV16_Task(
            &taskGroup, width, curHeight, curInBuf, curOutY, curOutCb, curOutCr,
            strideY, strideCb, strideCr));
        curY += curHeight;
    }
}

//------------------------------------------------------------------------------
// The following packedUVYA16_to_planarYUVA16 functions can be used to pass
// from packed UVYA 16 bits per component to planar YUVA with 16 bits per
// component.
//
void packedUVYA16_to_planarYUVA16(
    size_t width, size_t height, const uint64_t* FASTMEMCPYRESTRICT inBuf,
    uint16_t* FASTMEMCPYRESTRICT outY, uint16_t* FASTMEMCPYRESTRICT outCb,
    uint16_t* FASTMEMCPYRESTRICT outCr, uint16_t* FASTMEMCPYRESTRICT outA,
    size_t strideY, size_t strideCb, size_t strideCr, size_t strideA)
{
    const size_t nbPixels = width / 8;
    uint16_t* FASTMEMCPYRESTRICT startOutY = outY;
    uint16_t* FASTMEMCPYRESTRICT startOutCb = outCb;
    uint16_t* FASTMEMCPYRESTRICT startOutCr = outCr;
    uint16_t* FASTMEMCPYRESTRICT startOutA = outA;
    for (size_t i = 0; i < height; ++i)
    {
        outY = startOutY + i * strideY / 2;
        outCb = startOutCb + i * strideCb / 2;
        outCr = startOutCr + i * strideCr / 2;
        outA = startOutA + i * strideA / 2;
        for (size_t i = 0; i < nbPixels; ++i)
        {
            *outCr = (*inBuf & 0xFFFF000000000000) >> 48;
            ++outCr;
            *outCb = (*inBuf & 0x0000FFFF00000000) >> 32;
            ++outCb;
            *outY = (*inBuf & 0x00000000FFFF0000) >> 16;
            ++outY;
            *outA = (*inBuf & 0x000000000000FFFF);
            ++outA;
            ++inBuf;
        }
    }
}

//------------------------------------------------------------------------------
//
class PackedUVYA16_to_planarYUVA16_Task : public Task
{
public:
    PackedUVYA16_to_planarYUVA16_Task(
        TaskGroup* group, size_t width, size_t height,
        const uint64_t* FASTMEMCPYRESTRICT inBuf,
        uint16_t* FASTMEMCPYRESTRICT outY, uint16_t* FASTMEMCPYRESTRICT outCb,
        uint16_t* FASTMEMCPYRESTRICT outCr, uint16_t* FASTMEMCPYRESTRICT outA,
        size_t strideY, size_t strideCb, size_t strideCr, size_t strideA)
        : Task(group)
        , _width(width)
        , _height(height)
        , _inBuf(inBuf)
        , _outY(outY)
        , _outCb(outCb)
        , _outCr(outCr)
        , _outA(outA)
        , _strideY(strideY)
        , _strideCb(strideCb)
        , _strideCr(strideCr)
        , _strideA(strideA)
    {
    }

    virtual ~PackedUVYA16_to_planarYUVA16_Task() {}

    virtual void execute()
    {
        packedUVYA16_to_planarYUVA16(_width, _height, _inBuf, _outY, _outCb,
                                     _outCr, _outA, _strideY, _strideCb,
                                     _strideCr, _strideA);
    }

private:
    const size_t _width;
    const size_t _height;
    const uint64_t* FASTMEMCPYRESTRICT _inBuf;
    uint16_t* FASTMEMCPYRESTRICT _outY;
    uint16_t* FASTMEMCPYRESTRICT _outCb;
    uint16_t* FASTMEMCPYRESTRICT _outCr;
    uint16_t* FASTMEMCPYRESTRICT _outA;
    const size_t _strideY;
    const size_t _strideCb;
    const size_t _strideCr;
    const size_t _strideA;
};

void packedUVYA16_to_planarYUVA16_MP(
    size_t width, size_t height, const uint64_t* FASTMEMCPYRESTRICT inBuf,
    uint16_t* FASTMEMCPYRESTRICT outY, uint16_t* FASTMEMCPYRESTRICT outCb,
    uint16_t* FASTMEMCPYRESTRICT outCr, uint16_t* FASTMEMCPYRESTRICT outA,
    size_t strideY, size_t strideCb, size_t strideCr, size_t strideA)
{
    static bool use_standard_memcpy = getenv("RV_USE_STD_MEMCPY");
    if (use_standard_memcpy)
    {
        HOP_PROF("packedUVYA16_to_planarYUVA16()");
        packedUVYA16_to_planarYUVA16(width, height, inBuf, outY, outCb, outCr,
                                     outA, strideY, strideCb, strideCr,
                                     strideA);
        return;
    }

    HOP_PROF_FUNC();

    const size_t taskHeight = height / TwkFB::ThreadPool::getNumThreads();
    const size_t bufStride = width / 8;

    size_t curY = 0;

    TaskGroup taskGroup;

    while (curY < height)
    {
        // Need to divide the strides (in bytes) by 2 since incrementing a
        // uint16_t pointer by 1 is moving 2 bytes forward.
        const uint64_t* FASTMEMCPYRESTRICT curInBuf = inBuf + curY * bufStride;
        uint16_t* FASTMEMCPYRESTRICT curOutY = outY + curY * strideY / 2;
        uint16_t* FASTMEMCPYRESTRICT curOutCb = outCb + curY * strideCb / 2;
        uint16_t* FASTMEMCPYRESTRICT curOutCr = outCr + curY * strideCr / 2;
        uint16_t* FASTMEMCPYRESTRICT curOutA = outA + curY * strideA / 2;
        const size_t curHeight = std::min(taskHeight, height - curY);
        TwkFB::ThreadPool::addTask(new PackedUVYA16_to_planarYUVA16_Task(
            &taskGroup, width, curHeight, curInBuf, curOutY, curOutCb, curOutCr,
            curOutA, strideY, strideCb, strideCr, strideA));
        curY += curHeight;
    }
}

//------------------------------------------------------------------------------
//
static inline float normalizeY10(const uint32_t in_val)
{
    const float scale = 1.0f / (940.0f - 64.0f);
    const float offset = -64.0f / (940.0f - 64.0f);

    return in_val * scale + offset;
}

//------------------------------------------------------------------------------
//
static inline uint16_t clampAndRoundToUInt16(const float val)
{
    if (val <= 0.0f)
    {
        return 0;
    }
    else if (val >= 65535.0f)
    {
        return 65535;
    }
    else
    {
        // Here was assume that val is always positive for rounding
        return (uint16_t)(val + 0.5f);
    }
}

//------------------------------------------------------------------------------
//
static inline uint16_t denormalizeY16(const float in_val)
{
    const float scale = (60160.0f - 4096.0f);
    const float offset = 4096.0f;

    return clampAndRoundToUInt16(in_val * scale + offset);
}

//------------------------------------------------------------------------------
//
static inline float normalizeC10(const uint32_t in_val)
{
    const float scale = 1.0f / (960.0f - 64.0f);
    const float offset = -512.0f / (960.0f - 64.0f);

    return in_val * scale + offset;
}

//------------------------------------------------------------------------------
//
static inline uint16_t denormalizeC16(const float in_val)
{
    const float scale = 61440.0f - 4096.0f;
    const float offset = 32768.0f;

    return clampAndRoundToUInt16(in_val * scale + offset);
}

//------------------------------------------------------------------------------
// The following packedYUV444_10bits_to_P216 functions can be used to pass
// from packed YUV444 with 10 bits per component to to semi-planar YUV-422
// 16-bits per component (P216)
//
#define Y10MASK 0x000003FF
#define U10MASK 0x000FFC00
#define V10MASK 0x3FF00000

void packedYUV444_10bits_to_P216(size_t width, size_t height,
                                 const uint32_t* FASTMEMCPYRESTRICT inBuf,
                                 uint16_t* FASTMEMCPYRESTRICT outBufY,
                                 uint16_t* FASTMEMCPYRESTRICT outBufCbCy,
                                 size_t inBufStride, size_t outBufStride,
                                 bool flip)
{
    const size_t nbPixelsPerLoop = 2;
    const size_t nbPixelGroups = width / nbPixelsPerLoop;
    for (size_t y = 0; y < height; ++y)
    {
        size_t input_y = flip ? (height - 1 - y) : y;
        const uint32_t* inYUV = reinterpret_cast<const uint32_t*>(
            reinterpret_cast<const uint8_t*>(inBuf) + input_y * inBufStride);
        uint16_t* outY = reinterpret_cast<uint16_t*>(
            reinterpret_cast<uint8_t*>(outBufY) + y * outBufStride);
        uint16_t* outCbCr = reinterpret_cast<uint16_t*>(
            reinterpret_cast<uint8_t*>(outBufCbCy) + y * outBufStride);
        for (size_t i = 0; i < nbPixelGroups; ++i)
        {
            *outY = denormalizeY16(normalizeY10((*inYUV) & Y10MASK));
            ++outY;
            *outCbCr = denormalizeC16(normalizeC10(((*inYUV) & U10MASK) >> 10));
            ++outCbCr;
            *outCbCr = denormalizeC16(normalizeC10(((*inYUV) & V10MASK) >> 20));
            ++outCbCr;
            ++inYUV;

            *outY = denormalizeY16(normalizeY10((*inYUV) & Y10MASK));
            ++outY;
            ++inYUV;
        }
    }
}

//------------------------------------------------------------------------------
//
class PackedYUV444_10bits_to_P216_Task : public Task
{
public:
    PackedYUV444_10bits_to_P216_Task(TaskGroup* group, size_t width,
                                     size_t height,
                                     const uint32_t* FASTMEMCPYRESTRICT inBuf,
                                     uint16_t* FASTMEMCPYRESTRICT outBufY,
                                     uint16_t* FASTMEMCPYRESTRICT outBufCbCr,
                                     size_t inBufStride, size_t outBufStride,
                                     bool flip)
        : Task(group)
        , _width(width)
        , _height(height)
        , _inBuf(inBuf)
        , _outBufY(outBufY)
        , _outBufCbCr(outBufCbCr)
        , _inBufStride(inBufStride)
        , _outBufStride(outBufStride)
        , _flip(flip)
    {
    }

    virtual ~PackedYUV444_10bits_to_P216_Task() {}

    virtual void execute()
    {
        packedYUV444_10bits_to_P216(_width, _height, _inBuf, _outBufY,
                                    _outBufCbCr, _inBufStride, _outBufStride,
                                    _flip);
    }

private:
    const size_t _width;
    const size_t _height;
    const uint32_t* FASTMEMCPYRESTRICT _inBuf;
    uint16_t* FASTMEMCPYRESTRICT _outBufY;
    uint16_t* FASTMEMCPYRESTRICT _outBufCbCr;
    const size_t _inBufStride;
    const size_t _outBufStride;
    bool _flip;
};

//------------------------------------------------------------------------------
//
void packedYUV444_10bits_to_P216_MP(size_t width, size_t height,
                                    const uint32_t* FASTMEMCPYRESTRICT inBuf,
                                    uint16_t* FASTMEMCPYRESTRICT outBufY,
                                    uint16_t* FASTMEMCPYRESTRICT outBufCbCr,
                                    size_t inBufStride, size_t outBufStride,
                                    bool flip)
{
    static bool use_standard_memcpy = getenv("RV_USE_STD_MEMCPY");
    if (use_standard_memcpy)
    {
        HOP_PROF("packedYUV444_10bits_to_P216()");
        packedYUV444_10bits_to_P216(width, height, inBuf, outBufY, outBufCbCr,
                                    inBufStride, outBufStride, flip);
        return;
    }

    HOP_PROF_FUNC();

    const size_t taskHeight = height / TwkFB::ThreadPool::getNumThreads();

    size_t curY = 0;

    TaskGroup taskGroup;

    while (curY < height)
    {
        const size_t curHeight = std::min(taskHeight, height - curY);
        size_t input_y = flip ? (height - curHeight - curY) : curY;
        const uint32_t* FASTMEMCPYRESTRICT curInBuf =
            reinterpret_cast<const uint32_t*>(
                reinterpret_cast<const uint8_t*>(inBuf)
                + input_y * inBufStride);
        uint16_t* curOutY = reinterpret_cast<uint16_t*>(
            reinterpret_cast<uint8_t*>(outBufY) + curY * outBufStride);
        uint16_t* curOutCbCr = reinterpret_cast<uint16_t*>(
            reinterpret_cast<uint8_t*>(outBufCbCr) + curY * outBufStride);
        TwkFB::ThreadPool::addTask(new PackedYUV444_10bits_to_P216_Task(
            &taskGroup, width, curHeight, curInBuf, curOutY, curOutCbCr,
            inBufStride, outBufStride, flip));
        curY += curHeight;
    }
}

//------------------------------------------------------------------------------
// The following packedBGRA64_to_packedABGR64 functions can be used to pass
// from packed BGRA with 16 bits BE per component to packed ABGR with 16 bits LE
// per component.
//
void packedBGRA64BE_to_packedABGR64LE(size_t inStride, size_t height,
                                      const uint64_t* FASTMEMCPYRESTRICT inBuf,
                                      uint64_t* FASTMEMCPYRESTRICT outBuf,
                                      size_t outStride)
{
    // Big endian to little endiant
    const size_t nbPixelsPerRow = std::min(inStride, outStride) / 8;
    for (size_t line = 0; line < height; line++)
    {
        for (size_t i = 0; i < nbPixelsPerRow; ++i)
        {
            *(outBuf + i) = ((*(inBuf + i) & 0xFF00000000000000) >> 24)
                            | ((*(inBuf + i) & 0x00FF000000000000) >> 8)
                            | ((*(inBuf + i) & 0x0000FF0000000000) >> 24)
                            | ((*(inBuf + i) & 0x000000FF00000000) >> 8)
                            | ((*(inBuf + i) & 0x00000000FF000000) >> 24)
                            | ((*(inBuf + i) & 0x0000000000FF0000) >> 8)
                            | ((*(inBuf + i) & 0x000000000000FF00) << 40)
                            | ((*(inBuf + i) & 0x00000000000000FF) << 56);
        }
        inBuf += (inStride / 8);
        outBuf += (outStride / 8);
    }
}

//------------------------------------------------------------------------------
//
class PackedBGRA64BE_to_packedABGR64LETask : public Task
{
public:
    PackedBGRA64BE_to_packedABGR64LETask(
        TaskGroup* group, size_t width, size_t height,
        const uint64_t* FASTMEMCPYRESTRICT inBuf,
        uint64_t* FASTMEMCPYRESTRICT outBuf, size_t outStride)
        : Task(group)
        , _width(width)
        , _height(height)
        , _inBuf(inBuf)
        , _outBuf(outBuf)
        , _outStride(outStride)
    {
    }

    virtual ~PackedBGRA64BE_to_packedABGR64LETask() {}

    virtual void execute()
    {
        packedBGRA64BE_to_packedABGR64LE(_width, _height, _inBuf, _outBuf,
                                         _outStride);
    }

private:
    const size_t _width;
    const size_t _height;
    const uint64_t* FASTMEMCPYRESTRICT _inBuf;
    uint64_t* FASTMEMCPYRESTRICT _outBuf;
    const size_t _outStride;
};

void packedBGRA64BE_to_packedABGR64LE_MP(
    size_t width, size_t height, const uint64_t* FASTMEMCPYRESTRICT inBuf,
    uint64_t* FASTMEMCPYRESTRICT outBuf, size_t outStride)
{
    static bool use_standard_memcpy = getenv("RV_USE_STD_MEMCPY");
    if (use_standard_memcpy)
    {
        HOP_PROF("packedBGRA64BE_to_packedABGR64LE()");
        packedBGRA64BE_to_packedABGR64LE(width, height, inBuf, outBuf,
                                         outStride);
        return;
    }

    HOP_PROF_FUNC();

    const size_t taskHeight = height / TwkFB::ThreadPool::getNumThreads();
    const size_t bufStride = width / 8;

    size_t curY = 0;

    TaskGroup taskGroup;

    while (curY < height)
    {
        // Need to divide the strides (in bytes) by 8 since incrementing a
        // uint64_t pointer by 1 is moving 8 bytes forward.
        const uint64_t* FASTMEMCPYRESTRICT curInBuf = inBuf + curY * bufStride;
        uint64_t* FASTMEMCPYRESTRICT curOutBuf = outBuf + curY * outStride / 8;
        const size_t curHeight = std::min(taskHeight, height - curY);
        TwkFB::ThreadPool::addTask(new PackedBGRA64BE_to_packedABGR64LETask(
            &taskGroup, width, curHeight, curInBuf, curOutBuf, outStride));
        curY += curHeight;
    }
}
