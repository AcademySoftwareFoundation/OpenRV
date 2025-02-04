//******************************************************************************
// Copyright (c) 2001-2004 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <IOcin/Read16Bit.h>
#include <TwkFB/Exception.h>
#include <TwkFB/Operations.h>
#include <TwkMath/Iostream.h>
#include <TwkUtil/Timer.h>
#include <TwkUtil/ByteSwap.h>
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

    void Read16Bit::planarConfig(FrameBuffer& fb, int w, int h,
                                 FrameBuffer::DataType type)
    {
        vector<string> planeNames(3);
        planeNames[0] = "R";
        planeNames[1] = "G";
        planeNames[2] = "B";
        fb.restructurePlanar(w, h, planeNames, type, FrameBuffer::TOPLEFT);
    }

    void Read16Bit::readRGB16(const string& filename, const unsigned char* data,
                              FrameBuffer& fb, int w, int h, size_t maxBytes,
                              bool swap, bool useRaw,
                              unsigned char* deletePointer)
    {
        fb.restructure(
            w, h, 0, 3, FrameBuffer::USHORT, useRaw ? (unsigned char*)data : 0,
            0, FrameBuffer::TOPLEFT, true, 0, 0, useRaw ? deletePointer : 0);

        if (useRaw)
        {
            // cout << "READ16: use raw" << endl;

            if (swap)
            {
                Timer t;
                t.start();
                swapShorts(fb.pixels<unsigned short>(), w * h * 3);
                // cout << "READ16: swapped in " << t.elapsed() << endl;
            }
        }
        else
        {
            int ch = fb.numChannels();

            for (int y = 0; y < h; y++)
            {
                Pixel* in = (Pixel*)(data + y * w * sizeof(U16) * ch);
                if (maxBytes && ((char*)in - (char*)data) > maxBytes)
                    break;
                U16* out = fb.scanline<U16>(y);

                memcpy(out, in, w * sizeof(U16) * ch);
                if (swap)
                    swapShorts(out, w * ch);
            }
        }
    }

    void Read16Bit::readRGB16_PLANAR(const string& filename,
                                     const unsigned char* data, FrameBuffer& fb,
                                     int w, int h, size_t maxBytes, bool swap)
    {
        planarConfig(fb, w, h, FrameBuffer::USHORT);

        FrameBuffer* R = &fb;
        FrameBuffer* G = R->nextPlane();
        FrameBuffer* B = G->nextPlane();

        int ch = fb.numPlanes();

        for (int y = 0; y < h; y++)
        {
            Pixel* in = (Pixel*)(data + y * w * sizeof(U16) * ch);
            if (maxBytes && ((char*)in - (char*)data) > maxBytes)
                break;

            U16* restrict r = R->scanline<U16>(y);
            U16* restrict g = G->scanline<U16>(y);
            U16* restrict b = B->scanline<U16>(y);

            for (U16* restrict end = r + w; r < end; r++, g++, b++)
            {
                *r = *in;
                in++;
                *g = *in;
                in++;
                *b = *in;
                in++;
            }

            if (swap)
            {
                swapShorts(R->scanline<U16>(y), w);
                swapShorts(G->scanline<U16>(y), w);
                swapShorts(B->scanline<U16>(y), w);
            }
        }
    }

    void Read16Bit::readRGBA16(const string& filename,
                               const unsigned char* data, FrameBuffer& fb,
                               int w, int h, size_t maxBytes, bool alpha,
                               bool swap, bool useRaw,
                               unsigned char* deletePointer)
    {
        fb.restructure(
            w, h, 0, 4, FrameBuffer::USHORT, useRaw ? (unsigned char*)data : 0,
            0, FrameBuffer::TOPLEFT, true, 0, 0, useRaw ? deletePointer : 0);

        int ch = fb.numChannels();

        if (alpha)
        {
            if (useRaw)
            {
                // cout << "READ16: use raw" << endl;

                if (swap)
                {
                    Timer t;
                    t.start();
                    swapShorts(fb.pixels<unsigned short>(), w * h * 4);
                    // cout << "READ16: swapped in " << t.elapsed() << endl;
                }
            }
            else
            {
                for (int y = 0; y < h; y++)
                {
                    Pixel* in = (Pixel*)(data + y * w * sizeof(U16) * ch);
                    if (maxBytes && ((char*)in - (char*)data) > maxBytes)
                        break;
                    U16* out = fb.scanline<U16>(y);

                    memcpy(out, in, w * sizeof(U16) * ch);
                    if (swap)
                        swapShorts(out, w * ch);
                }
            }
        }
    }

} //  End namespace TwkFB
