//*****************************************************************************/
//
// Filename: FastMemcpy.C
//
// Copyright (c) 2019 Autodesk, Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//*****************************************************************************/

//==============================================================================
// EXTERNAL DECLARATIONS
//==============================================================================

#include <TwkFB/FastMemcpy.h>

#include <TwkFB/TwkFBThreadPool.h>
#include <TwkUtil/sgcHop.h>

#include <cstring> // for memcpy()
#if defined(_USE_SIMD_MEMCPY)
#include <mmintrin.h>
#include <xmmintrin.h>
#endif
#include <cstdio>
#include <algorithm>

//==============================================================================
// Special memcpy implementations leveraging SIMD/REPMOVSB
//==============================================================================

// Note that multiple implemetations can be enabled (defined) by design.
// PxlMemcpy() is responsible for selecting the best implementation at run time
// and will also make sure that a specific implementation is supported by the
// host processor before leveraging it.
//
// There is a performance and an integrity test available in the PPIP tests
// suite that validates those special memcpy implementations below and also
// benchmarks their performances against the standard memcpy.
//

//==============================================================================
// CONSTANTS
//==============================================================================

#define small_memcpy(dest, src, n) memcpy((dest), (src), (n))
#define MIN_LEN 128
#define MIN_LEN_MP 4096
#define SSE_REGISTER_SIZE 16

//==============================================================================
// REP MOVSB MEMCPY IMPLEMENTATION
//==============================================================================

#if defined(_USE_REP_MOVSB_MEMCPY)

void* FastMemcpy_REPMOVSB(void* PXLRESTRICT outBuf,
                          const void* PXLRESTRICT inBuf, size_t n)
{
    void* origOutBuf = outBuf;

    asm volatile( // assembler template
        "rep movsb"
        // output operands
        : "=D"(outBuf), "=S"(inBuf), "=c"(n)
        // input operands
        : "0"(outBuf), "1"(inBuf), "2"(n)
        // list of clobbered registers
        : "memory");

    return origOutBuf;
}

#endif // #if defined(_USE_REP_MOVSB_MEMCPY)

//==============================================================================
// SIMD MEMCPY IMPLEMENTATION
//==============================================================================

#if defined(_USE_SIMD_MEMCPY)

static inline bool is_aligned(const void* PXLRESTRICT ptr, size_t byte_count)
{
    return (reinterpret_cast<ptrdiff_t>(ptr) % byte_count) == 0;
}

void* FastMemcpy_SIMD(void* PXLRESTRICT outBuf, const void* PXLRESTRICT inBuf,
                      size_t n)
{
    // Fall back to the standard version of memcpy if the size does not meet the
    // mininum length criteria
    if (n < MIN_LEN)
    {
        return small_memcpy(outBuf, inBuf, n);
    }

    const unsigned char* PXLRESTRICT src =
        static_cast<const unsigned char * PXLRESTRICT>(inBuf);
    unsigned char* PXLRESTRICT dst =
        static_cast<unsigned char * PXLRESTRICT>(outBuf);

    // memcpy until dst is aligned with simd
    if (!is_aligned(dst, SSE_REGISTER_SIZE))
    {
        size_t delta = SSE_REGISTER_SIZE
                       - (reinterpret_cast<ptrdiff_t>(dst) % SSE_REGISTER_SIZE);
        small_memcpy(dst, src, delta);
        src += delta;
        dst += delta;
        n -= delta;
    }

    // memcpy 4 sse registers (128 bits) at a time
    const size_t step = 4 * SSE_REGISTER_SIZE;

    // At this point the dest is aligned but not necessarily the source
    if (is_aligned(src, SSE_REGISTER_SIZE))
    {
        for (; n >= step; n -= step)
        {
            _mm_prefetch(reinterpret_cast<const char * PXLRESTRICT>(src) + 1024,
                         _MM_HINT_T0);
            __m128 xmm0 = _mm_load_ps((float*)(src + 0 * SSE_REGISTER_SIZE));
            __m128 xmm1 = _mm_load_ps((float*)(src + 1 * SSE_REGISTER_SIZE));
            __m128 xmm2 = _mm_load_ps((float*)(src + 2 * SSE_REGISTER_SIZE));
            __m128 xmm3 = _mm_load_ps((float*)(src + 3 * SSE_REGISTER_SIZE));

            _mm_store_ps((float*)(dst + 0 * SSE_REGISTER_SIZE), xmm0);
            _mm_store_ps((float*)(dst + 1 * SSE_REGISTER_SIZE), xmm1);
            _mm_store_ps((float*)(dst + 2 * SSE_REGISTER_SIZE), xmm2);
            _mm_store_ps((float*)(dst + 3 * SSE_REGISTER_SIZE), xmm3);

            src += step;
            dst += step;
        }
    }
    else
    {
        for (; n >= step; n -= step)
        {
            __m128 xmm0 = _mm_loadu_ps((float*)(src + 0 * SSE_REGISTER_SIZE));
            __m128 xmm1 = _mm_loadu_ps((float*)(src + 1 * SSE_REGISTER_SIZE));
            __m128 xmm2 = _mm_loadu_ps((float*)(src + 2 * SSE_REGISTER_SIZE));
            __m128 xmm3 = _mm_loadu_ps((float*)(src + 3 * SSE_REGISTER_SIZE));

            _mm_store_ps((float*)(dst + 0 * SSE_REGISTER_SIZE), xmm0);
            _mm_store_ps((float*)(dst + 1 * SSE_REGISTER_SIZE), xmm1);
            _mm_store_ps((float*)(dst + 2 * SSE_REGISTER_SIZE), xmm2);
            _mm_store_ps((float*)(dst + 3 * SSE_REGISTER_SIZE), xmm3);

            src += step;
            dst += step;
        }
    }

    // Take care of any left over bytes to copy
    if (n != 0)
    {
        small_memcpy(dst, src, n);
    }

    return outBuf;
}

#endif // #if defined(_USE_SIMD_MEMCPY)

//==============================================================================
// MEMCPY IMPLEMENTATION SELECTOR
//==============================================================================

void* FastMemcpy(void* FASTMEMCPYRESTRICT outBuf,
                 const void* FASTMEMCPYRESTRICT inBuf, size_t n)
{
    // On the latest processors, starting with the Haswell architecture, the
    // standard memcpy implementation is faster than both our REPMOVSB and SIMD
    // custom implementation so we'll simply defer to the standard memcpy
    // implementation.
    // Haswell was the first micro-architecture with avx2 support so we'll use
    // this feature to detect a newer processor.
#if defined(_SSEUTIL_IS_AVAILABLE)
    static const bool avx2Supported =
        SSEUtil::isSupported(SSEUtil::CPU_FEATURE_AVX2);
    if (avx2Supported)
    {
        return memcpy(outBuf, inBuf, n);
    }
#endif

    // See note below for an explanation of the !defined(LINUX) condition
#if defined(_USE_REP_MOVSB_MEMCPY) && !defined(LINUX)
    static const bool enhancedRepMovSBSupported =
        SSEUtil::isSupported(SSEUtil::CPU_FEATURE_ERMSB);
    if (enhancedRepMovSBSupported)
    {
        return PxlMemcpy_REPMOVSB(outBuf, inBuf, n);
    }
#endif

#if defined(_USE_SIMD_MEMCPY)
    return PxlMemcpy_SIMD(outBuf, inBuf, n);
#else
    return memcpy(outBuf, inBuf, n);
#endif
}

//------------------------------------------------------------------------------
// This is a special implementation of the same memcpy selector intended for
// multi-threading operations.
// Rationale: With the introduction of the new REP MOVSB memcpy implementation,
// we measured an anomaly on Z820 systems with ivy bridge processors which are
// supporting the new REP MOVSB processor optimization: we are getting better
// performance optimization when using the REP MOVSB version for multi-threading
// processing (PxlMemcpy_MP) and the SIMD for single threading processing
// (PxlMemcpy). This routine was created to get the best of both worlds.
// Note that this anomaly was NOT found on Mac systems.
//
void* FastMemcpyForMP(void* FASTMEMCPYRESTRICT outBuf,
                      const void* FASTMEMCPYRESTRICT inBuf, size_t n)
{
    // On the latest processors, starting with the Haswell architecture, the
    // standard memcpy implementation is faster than both our REPMOVSB and SIMD
    // custom implementation so we'll simply defer to the standard memcpy
    // implementation.
    // Haswell was the first micro-architecture with avx2 support so we'll use
    // this feature to detect a newer processor.
#if defined(_SSEUTIL_IS_AVAILABLE)
    static const bool avx2Supported =
        SSEUtil::isSupported(SSEUtil::CPU_FEATURE_AVX2);
    if (avx2Supported)
    {
        return memcpy(outBuf, inBuf, n);
    }
#endif

#if defined(_USE_REP_MOVSB_MEMCPY)
    static const bool enhancedRepMovSBSupported =
        SSEUtil::isSupported(SSEUtil::CPU_FEATURE_ERMSB);
    if (enhancedRepMovSBSupported)
    {
        return PxlMemcpy_REPMOVSB(outBuf, inBuf, n);
    }
#endif

#if defined(_USE_SIMD_MEMCPY)
    return FastMemcpy_SIMD(outBuf, inBuf, n);
#else
    return memcpy(outBuf, inBuf, n);
#endif
}

using namespace ILMTHREAD_NAMESPACE;

class MemcpyTask : public Task
{
public:
    MemcpyTask(TaskGroup* group, void* FASTMEMCPYRESTRICT outBuf,
               const void* FASTMEMCPYRESTRICT inBuf, size_t n)
        : Task(group)
        , _outBuf(outBuf)
        , _inBuf(inBuf)
        , _n(n)
    {
    }

    virtual ~MemcpyTask() {}

    virtual void execute() { FastMemcpy(_outBuf, _inBuf, _n); }

    void* FASTMEMCPYRESTRICT _outBuf;
    const void* FASTMEMCPYRESTRICT _inBuf;
    size_t _n;
};

void* FastMemcpy_MP(void* FASTMEMCPYRESTRICT outBuf,
                    const void* FASTMEMCPYRESTRICT inBuf, size_t n)
{
    static bool use_standard_memcpy = getenv("RV_USE_STD_MEMCPY");
    if (use_standard_memcpy)
    {
        HOP_PROF("memcpy()");
        return memcpy(outBuf, inBuf, n);
    }

    HOP_PROF_FUNC();

    const size_t kMaxChunkSize = 16 * 1024 * 1024;

    uint8_t* currOutBuf = static_cast<uint8_t*>(outBuf);
    const uint8_t* currInBuf = static_cast<const uint8_t*>(inBuf);

    TaskGroup taskGroup;

    while (n > 0)
    {
        if (n > kMaxChunkSize)
        {
            TwkFB::ThreadPool::addTask(new MemcpyTask(
                &taskGroup, currOutBuf, currInBuf, kMaxChunkSize));
            n -= kMaxChunkSize;
            currInBuf += kMaxChunkSize;
            currOutBuf += kMaxChunkSize;
        }
        else
        {
            TwkFB::ThreadPool::addTask(
                new MemcpyTask(&taskGroup, currOutBuf, currInBuf, n));
            n = 0;
        }
    }

    return outBuf;
}

//------------------------------------------------------------------------------
//
//  In order to do this with a shader and have it be efficient we have to
//  write the shader to completely pack the subsampled data with no
//  scanline padding. Until we have that this will suffice.
//
//  NOTE: this is discarding every other chroma sample instead of
//  interpolating. Not the best way to do this.
//

void subsample422_8bit_UYVY(size_t width, size_t height,
                            const uint8_t* FASTMEMCPYRESTRICT inBuf,
                            uint8_t* FASTMEMCPYRESTRICT outBuf)
{
    uint8_t* FASTMEMCPYRESTRICT p1 = outBuf;

    if (width % 6 == 0)
    {
        for (size_t row = 0; row < height; row++)
        {
            for (const uint8_t *FASTMEMCPYRESTRICT
                     p0 = inBuf + row * 3 * width,
                     *FASTMEMCPYRESTRICT e = p0 + 3 * width;
                 p0 < e; p0 += 6, p1 += 4)
            {
                //
                //  The comment out parts do the interpolation, but this
                //  can be done as a small blur after the YUV
                //  conversion. That saves a few microseconds
                //

                // p1[0] = (int(p0[1]) + int(p0[4])) >> 1;
                p1[0] = p0[1];
                p1[1] = p0[0];
                // p1[2] = (int(p0[2]) + int(p0[5])) >> 1;
                p1[2] = p0[2];
                p1[3] = p0[3];
            }
        }
    }
    else
    {
        for (size_t row = 0; row < height; row++)
        {
            size_t count = 0;

            for (const uint8_t *FASTMEMCPYRESTRICT
                     p0 = inBuf + row * 3 * width,
                     *FASTMEMCPYRESTRICT e = p0 + 3 * width;
                 p0 < e; p0 += 3, count++)
            {
                *p1 = p0[count % 2 + 1];
                p1++;
                *p1 = *p0;
                p1++;
            }
        }
    }
}

//------------------------------------------------------------------------------
//
class Subsample422_8bit_UYVY_Task : public Task
{
public:
    Subsample422_8bit_UYVY_Task(TaskGroup* group, size_t width, size_t height,
                                const uint8_t* FASTMEMCPYRESTRICT inBuf,
                                uint8_t* FASTMEMCPYRESTRICT outBuf)
        : Task(group)
        , _width(width)
        , _height(height)
        , _inBuf(inBuf)
        , _outBuf(outBuf)
    {
    }

    virtual ~Subsample422_8bit_UYVY_Task() {}

    virtual void execute()
    {
        subsample422_8bit_UYVY(_width, _height, _inBuf, _outBuf);
    }

    const size_t _width;
    const size_t _height;
    const uint8_t* FASTMEMCPYRESTRICT _inBuf;
    uint8_t* FASTMEMCPYRESTRICT _outBuf;
};

//------------------------------------------------------------------------------
//
void subsample422_8bit_UYVY_MP(size_t width, size_t height,
                               const uint8_t* FASTMEMCPYRESTRICT inBuf,
                               uint8_t* FASTMEMCPYRESTRICT outBuf)
{
    static bool use_standard_memcpy = getenv("RV_USE_STD_MEMCPY");
    if (use_standard_memcpy)
    {
        HOP_PROF("subsample422_8bit_UYVY()");
        subsample422_8bit_UYVY(width, height, inBuf, outBuf);
        return;
    }

    HOP_PROF_FUNC();

    const size_t taskHeight = height / TwkFB::ThreadPool::getNumThreads();
    const size_t inBufStride = width * 3;
    const size_t outBufStride = width * 2;

    size_t curY = 0;

    TaskGroup taskGroup;

    while (curY < height)
    {
        const uint8_t* FASTMEMCPYRESTRICT curInBuf = inBuf + curY * inBufStride;
        uint8_t* FASTMEMCPYRESTRICT curOutBuf = outBuf + curY * outBufStride;
        const size_t curHeight = std::min(taskHeight, height - curY);
        TwkFB::ThreadPool::addTask(new Subsample422_8bit_UYVY_Task(
            &taskGroup, width, curHeight, curInBuf, curOutBuf));
        curY += curHeight;
    }
}

//
//  In order to do this with a shader and have it be efficient we have to
//  write the shader to completely pack the subsampled data with no
//  scanline padding. Until we have that this will suffice.
//
//  NOTE: this is discarding every other chroma sample instead of
//  interpolating. Not the best way to do this.
//

#define R10MASK 0x3FF00000
#define G10MASK 0xFFC00
#define B10MASK 0x3FF

#define REVERSE(x) \
    (((x << 20) & R10MASK) | (x & G10MASK) | ((x >> 20) & B10MASK));

//------------------------------------------------------------------------------
//
void subsample422_10bit(size_t width, size_t height,
                        const uint32_t* FASTMEMCPYRESTRICT inBuf,
                        uint32_t* FASTMEMCPYRESTRICT outBuf, size_t inBufStride,
                        size_t outBufStride)
{
    //
    //  NOTE: 2_10_10_10_INT_REV is *backwards* eventhough its GL_RGB
    //  (hence the REV). So Y is the lowest sig bits.
    //

    for (size_t row = 0; row < height; row++)
    {
        uint32_t* FASTMEMCPYRESTRICT p1 =
            outBuf + row * outBufStride / sizeof(uint32_t);

        for (const uint32_t *FASTMEMCPYRESTRICT
                 p0 = inBuf + row * inBufStride / sizeof(uint32_t),
                 *FASTMEMCPYRESTRICT e =
                     p0 + inBufStride / sizeof(uint32_t) - (width % 6);
             p0 < e; p0 += 6)
        {
            uint32_t A = *p0;
            uint32_t B = p0[1];
            uint32_t C = p0[2];
            uint32_t D = p0[3];
            uint32_t E = p0[4];
            uint32_t F = p0[5];

            *p1 = (A & R10MASK) | ((A << 10) & G10MASK) | ((A >> 10) & B10MASK);
            p1++;
            *p1 = ((C << 20) & R10MASK) | (B & G10MASK) | (B & B10MASK);
            p1++;
            *p1 = ((C << 10) & R10MASK) | ((D << 10) & G10MASK)
                  | ((B >> 20) & B10MASK);
            p1++;
            *p1 = ((F << 20) & R10MASK) | ((D >> 10) & G10MASK) | (E & B10MASK);
            p1++;
        }
    }
}

//------------------------------------------------------------------------------
//
class Subsample422_10bit_Task : public Task
{
public:
    Subsample422_10bit_Task(TaskGroup* group, size_t width, size_t height,
                            const uint32_t* FASTMEMCPYRESTRICT inBuf,
                            uint32_t* FASTMEMCPYRESTRICT outBuf,
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

    virtual ~Subsample422_10bit_Task() {}

    virtual void execute()
    {
        subsample422_10bit(_width, _height, _inBuf, _outBuf, _inBufStride,
                           _outBufStride);
    }

    const size_t _width;
    const size_t _height;
    const uint32_t* FASTMEMCPYRESTRICT _inBuf;
    uint32_t* FASTMEMCPYRESTRICT _outBuf;
    size_t _inBufStride;
    size_t _outBufStride;
};

//------------------------------------------------------------------------------
//
void subsample422_10bit_MP(size_t width, size_t height,
                           const uint32_t* FASTMEMCPYRESTRICT inBuf,
                           uint32_t* FASTMEMCPYRESTRICT outBuf,
                           size_t inBufStride, size_t outBufStride)
{
    static bool use_standard_memcpy = getenv("RV_USE_STD_MEMCPY");
    if (use_standard_memcpy)
    {
        HOP_PROF("subsample422_10bit()");
        subsample422_10bit(width, height, inBuf, outBuf, inBufStride,
                           outBufStride);
        return;
    }

    HOP_PROF_FUNC();

    const size_t taskHeight = height / TwkFB::ThreadPool::getNumThreads();
    size_t curY = 0;

    TaskGroup taskGroup;

    while (curY < height)
    {
        const uint32_t* FASTMEMCPYRESTRICT curInBuf =
            inBuf + curY * inBufStride / sizeof(uint32_t);
        uint32_t* FASTMEMCPYRESTRICT curOutBuf =
            outBuf + curY * outBufStride / sizeof(uint32_t);
        const size_t curHeight = std::min(taskHeight, height - curY);
        TwkFB::ThreadPool::addTask(
            new Subsample422_10bit_Task(&taskGroup, width, curHeight, curInBuf,
                                        curOutBuf, inBufStride, outBufStride));
        curY += curHeight;
    }
}

//------------------------------------------------------------------------------
//
void swap_bytes_32bit(size_t width, size_t height,
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
            const uint32_t a = *p0;
            *p1 = ((a & 0x000000FF) << 24) | ((a & 0x0000FF00) << 8)
                  | ((a & 0x00FF0000) >> 8) | ((a & 0xFF000000) >> 24);
            p1++;
        }
    }
}

//------------------------------------------------------------------------------
//
class Swap_bytes_32bit_Task : public Task
{
public:
    Swap_bytes_32bit_Task(TaskGroup* group, size_t width, size_t height,
                          const uint32_t* FASTMEMCPYRESTRICT inBuf,
                          uint32_t* FASTMEMCPYRESTRICT outBuf)
        : Task(group)
        , _width(width)
        , _height(height)
        , _inBuf(inBuf)
        , _outBuf(outBuf)
    {
    }

    virtual ~Swap_bytes_32bit_Task() {}

    virtual void execute()
    {
        swap_bytes_32bit(_width, _height, _inBuf, _outBuf);
    }

    const size_t _width;
    const size_t _height;
    const uint32_t* FASTMEMCPYRESTRICT _inBuf;
    uint32_t* FASTMEMCPYRESTRICT _outBuf;
};

//------------------------------------------------------------------------------
//
void swap_bytes_32bit_MP(size_t width, size_t height,
                         const uint32_t* FASTMEMCPYRESTRICT inBuf,
                         uint32_t* FASTMEMCPYRESTRICT outBuf)
{
    static bool use_standard_memcpy = getenv("RV_USE_STD_MEMCPY");
    if (use_standard_memcpy)
    {
        HOP_PROF("swap_bytes_32bit()");
        swap_bytes_32bit(width, height, inBuf, outBuf);
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
        TwkFB::ThreadPool::addTask(new Swap_bytes_32bit_Task(
            &taskGroup, width, curHeight, curInBuf, curOutBuf));
        curY += curHeight;
    }
}
