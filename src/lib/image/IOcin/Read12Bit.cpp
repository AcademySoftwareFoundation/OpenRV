//******************************************************************************
// Copyright (c) 2001-2004 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <IOcin/Read12Bit.h>
#include <TwkFB/Exception.h>
#include <TwkFB/Operations.h>
#include <TwkMath/Iostream.h>
#include <TwkUtil/Interrupt.h>
#include <TwkMath/Color.h>
#include <fstream>
#include <iostream>
#include <stl_ext/string_algo.h>
#include <string>
#include <limits>

namespace TwkFB
{
    using namespace TwkFB;
    using namespace std;
    using namespace TwkUtil;
    using namespace TwkMath;
    using namespace std;

#ifdef _MSC_VER
#define restrict
#else
// Ref.: https://en.wikipedia.org/wiki/Restrict
// C++ does not have standard support for restrict, but many compilers have
// equivalents that usually work in both C++ and C, such as the GCC's and
// Clang's __restrict__, and Visual C++'s __declspec(restrict). In addition,
// __restrict is supported by those three compilers. The exact interpretation of
// these alternative keywords vary by the compiler:
#define restrict __restrict
#endif

    void Read12Bit::planarConfig(FrameBuffer& fb, int w, int h,
                                 FrameBuffer::DataType type)
    {
        vector<string> planeNames(3);
        planeNames[0] = "R";
        planeNames[1] = "G";
        planeNames[2] = "B";
        fb.restructurePlanar(w, h, planeNames, type, FrameBuffer::TOPLEFT);
    }

    void Read12Bit::readRGB8_PLANAR(const string& filename,
                                    const unsigned char* data, FrameBuffer& fb,
                                    int w, int h, size_t maxBytes, bool swap)
    {
        planarConfig(fb, w, h, FrameBuffer::UCHAR);

        FrameBuffer* R = &fb;
        FrameBuffer* G = R->nextPlane();
        FrameBuffer* B = G->nextPlane();

        for (int y = 0; y < h; y++)
        {
            U16* in = (U16*)(data + y * w * sizeof(U16) * 3);
            if (maxBytes && ((char*)in - (char*)data) > maxBytes)
                break;

            U8* restrict r = R->scanline<U8>(y);
            U8* restrict g = G->scanline<U8>(y);
            U8* restrict b = B->scanline<U8>(y);

            if (swap)
            {
                for (U8* restrict end = r + w; r < end; r++, g++, b++)
                {
                    U16 p[3];
                    p[0] = *in;
                    in++;
                    p[1] = *in;
                    in++;
                    p[2] = *in;
                    in++;

                    swapShorts(p, 3);

                    // This should be doing some type of rounding
                    *r = p[0] >> 8;
                    *g = p[1] >> 8;
                    *b = p[2] >> 8;
                }
            }
            else
            {
                for (U8* restrict end = r + w; r < end; r++, g++, b++)
                {
                    // This should be doing some type of rounding
                    *r = *in >> 8;
                    in++;
                    *g = *in >> 8;
                    in++;
                    *b = *in >> 8;
                    in++;
                }
            }
        }
    }

    void Read12Bit::readRGB16_PLANAR(const string& filename,
                                     const unsigned char* data, FrameBuffer& fb,
                                     int w, int h, size_t maxBytes, bool swap)
    {
        planarConfig(fb, w, h, FrameBuffer::USHORT);

        FrameBuffer* R = &fb;
        FrameBuffer* G = R->nextPlane();
        FrameBuffer* B = G->nextPlane();

        for (int y = 0; y < h; y++)
        {
            U16* in = (U16*)(data + y * w * sizeof(U16) * 3);
            if (maxBytes && ((char*)in - (char*)data) > maxBytes)
                break;

            U16* restrict r = R->scanline<U16>(y);
            U16* restrict g = G->scanline<U16>(y);
            U16* restrict b = B->scanline<U16>(y);

            if (swap)
            {
                for (U16* restrict end = r + w; r < end; r++, g++, b++)
                {
                    U16 p[3];
                    p[0] = *in;
                    in++;
                    p[1] = *in;
                    in++;
                    p[2] = *in;
                    in++;

                    swapShorts(p, 3);

                    *r = p[0];
                    *g = p[1];
                    *b = p[2];
                }
            }
            else
            {
                for (U16* restrict end = r + w; r < end; r++, g++, b++)
                {
                    *r = *in;
                    in++;
                    *g = *in;
                    in++;
                    *b = *in;
                    in++;
                }
            }
        }
    }

    void Read12Bit::readNoPaddingRGB16(const string& filename,
                                       const unsigned char* data,
                                       FrameBuffer& fb, int w, int h,
                                       size_t maxBytes, bool swap)
    {
        planarConfig(fb, w, h, FrameBuffer::USHORT);

        unsigned char* data2 = 0;

        if (swap)
        {
            data2 = new unsigned char[maxBytes];
            memcpy(data2, data, maxBytes);
            TwkUtil::swapWords(data2, (maxBytes / 4));
        }

        FrameBuffer* R = &fb;
        FrameBuffer* G = R->nextPlane();
        FrameBuffer* B = G->nextPlane();

        U16* elements[] = {R->pixels<U16>(), G->pixels<U16>(),
                           B->pixels<U16>()};

        //
        //  Loop over the nibbles
        //

        size_t nibbleIndex = 0;
        size_t valueIndex = 0;
        size_t elemIndex = 0;
        size_t iteration = 0;

        for (const unsigned char *p = (swap ? data2 : data), *e = p + maxBytes;
             p < e;)
        {
            const U16 nibble = *p & (nibbleIndex == 0 ? 0x0f : 0xf0);
            if (valueIndex == 0)
                *(elements[elemIndex]) = 0;
            *(elements[elemIndex]) |=
                nibble << (valueIndex * 4 + 4 * (1 - nibbleIndex));

            nibbleIndex ^= 0x1;
            valueIndex = (valueIndex + 1) % 3;

            if (nibbleIndex == 0)
                p++;
            if (valueIndex == 0)
                elements[elemIndex]++;
            elemIndex = (iteration / 3) % 3;
            iteration++;
        }

        delete[] data2;
    }

} //  End namespace TwkFB
