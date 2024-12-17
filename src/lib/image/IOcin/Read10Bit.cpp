//******************************************************************************
// Copyright (c) 2001-2004 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <IOcin/Read10Bit.h>
#include <TwkFB/Exception.h>
#include <TwkFB/Operations.h>
#include <TwkMath/Iostream.h>
#include <TwkMath/Color.h>
#include <TwkUtil/Timer.h>
#include <TwkUtil/ByteSwap.h>
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

    typedef FrameBuffer::Pixel10 Pixel10;
    typedef FrameBuffer::Pixel10Rev Pixel10Rev;

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

    void Read10Bit::planarConfig(FrameBuffer& fb, int w, int h,
                                 FrameBuffer::DataType type)
    {
        //
        //  NOTE: we're defaulting to TOPLEFT as that's the most common DPX
        //  orientation.
        //

        vector<string> planeNames(3);
        planeNames[0] = "R";
        planeNames[1] = "G";
        planeNames[2] = "B";
        fb.restructurePlanar(w, h, planeNames, type, FrameBuffer::TOPLEFT);
    }

    void Read10Bit::readRGBA8(const string& filename, const unsigned char* data,
                              FrameBuffer& fb, int w, int h, size_t maxBytes,
                              bool alpha, bool swap)
    {
        fb.restructure(w, h, 0, 4, FrameBuffer::UCHAR, 0, 0,
                       FrameBuffer::TOPLEFT, true);

        if (alpha)
        {
            Pixel* in = (Pixel*)(data);
            size_t inindex = 0;

            const size_t nwords = (w * 4) / 3;
            const size_t rem = (w * 4) % 3;
            const size_t totalWords = (nwords + (rem ? 1 : 0)) * h;
            const bool truncatedScanline =
                (maxBytes / 4 == nwords * h) && rem != 0;
            const size_t brem = (w * h * 4) % 3;
            const size_t bwords = (w * h * 4) / 3 * 4 + (brem ? 4 : 0);
            const bool runonScanline = bwords == maxBytes && rem != 0;
            size_t ss = fb.scanlineSize();

            if (truncatedScanline && runonScanline)
            {
                cout << "WARNING: " << filename
                     << " is both truncated and run-on" << endl;
            }

            if (truncatedScanline)
            {
                cout << "INFO: Reading as a truncated scanline file" << endl;
                cout << "INFO: " << filename << " is not following the DPX spec"
                     << endl;
                ss -= rem;
            }
            else if (runonScanline)
            {
                cout << "INFO: Read as a run-on scanline file" << endl;
                cout << "INFO: " << filename << " is not following the DPX spec"
                     << endl;
            }

            for (int y = 0; y < h; y++)
            {
                if (maxBytes && ((char*)in - (char*)data) > maxBytes)
                    break;
                U8* start = fb.scanline<U8>(y);

                for (U8 *out = start, *end = start + ss; out < end; out++)
                {
                    Pixel p;

                    if (swap)
                    {
                        const U32 v = in->pixelWord;

                        p.pixelWord = ((v & 0xff000000) >> 24)
                                      | ((v & 0x00ff0000) >> 8)
                                      | ((v & 0x0000ff00) << 8)
                                      | ((v & 0x000000ff) << 24);
                    }
                    else
                    {
                        p = *in;
                    }

                    switch (inindex)
                    {
                    case 0:
                        *out = U8(p.red == 0x3ff ? p.red_most
                                                 : ((p.red + 1) >> 2));
                        break;
                    case 1:
                        *out = U8(p.green == 0x3ff ? p.green_most
                                                   : ((p.green + 1) >> 2));
                        break;
                    case 2:
                        *out = U8(p.blue == 0x3ff ? p.blue_most
                                                  : ((p.blue + 1) >> 2));
                        break;
                    }

                    inindex = (inindex + 1) % 3;

                    if (inindex == 0)
                    {
                        in++;
                        if (maxBytes && ((char*)in - (char*)data) > maxBytes)
                            break;
                    }
                }

                if (inindex != 0 && !runonScanline)
                {
                    //
                    //  As far as I can tell, this is correct. Both v1 and v2
                    //  DPX specs say that once you get to the end of a
                    //  scanline, the remaining data in the final word will be
                    //  zero and hence the padding is "broken". The next
                    //  scanline will start on the next word with the first
                    //  datum being reset.
                    //
                    //  Here's the relevant passage from v1 spec:
                    //
                    //      Any bits in the last word of a scan line left over
                    //      will be filled with zeroes. That is to say that the
                    //      packing is broken on scan-line boundaries.
                    //
                    //  Unforunately, it looks like both nuke and shake will
                    //  write images where this is not true. You usually don't
                    //  see this problem because the resolutions that people
                    //  care about (* 4) are usually divisible by 3 so this
                    //  doesn't even happen. In their case it looks like
                    //  instead of dropping the remaining bits, they drop the
                    //  remaining data from the end of the scanline. So they
                    //  just don't have a "last word of a scan line".
                    //  E.g. (400 * 4) / 3 => 533.33333 so there's one last
                    //  datum in the last word. It looks like nuke and shake
                    //  drop this datum (so they're missing the last alpha
                    //  value).
                    //
                    //  In constrast, the ARRISCAN111 for example just keeps
                    //  packing more data and ignores the scanline break all
                    //  together. We include a hack here to allow the reader to
                    //  optionally ignore the scanline break for this output.
                    //

                    in++;
                    inindex = 0;
                }
            }
        }
        else
        {
            if (swap)
            {
                for (int y = 0; y < h; y++)
                {
                    Pixel* in = (Pixel*)(data + y * w * sizeof(U32));
                    Pixel* lim = (Pixel*)(data + (y + 1) * w * sizeof(U32));
                    if (maxBytes && ((char*)lim - (char*)data) > maxBytes)
                        break;
                    U8* out = fb.scanline<U8>(y);

                    for (U8* end = out + w * 4; out < end; in++)
                    {
                        Pixel p;
                        const U32 v = in->pixelWord;

                        p.pixelWord = ((v & 0xff000000) >> 24)
                                      | ((v & 0x00ff0000) >> 8)
                                      | ((v & 0x0000ff00) << 8)
                                      | ((v & 0x000000ff) << 24);

                        // const U8 rv = p.red_most;
                        // const U8 gv = p.green_most;
                        // const U8 bv = p.blue_most;
                        const U8 rv =
                            p.red == 0x3ff ? p.red_most : ((p.red + 1) >> 2);
                        const U8 gv = p.green == 0x3ff ? p.green_most
                                                       : ((p.green + 1) >> 2);
                        const U8 bv =
                            p.blue == 0x3ff ? p.blue_most : ((p.blue + 1) >> 2);

                        *out = rv;
                        out++;
                        *out = gv;
                        out++;
                        *out = bv;
                        out++;
                        *out = 255;
                        out++;
                    }
                }
            }
            else
            {
                for (int y = 0; y < h; y++)
                {
                    Pixel* in = (Pixel*)(data + y * w * sizeof(U32));
                    Pixel* lim = (Pixel*)(data + (y + 1) * w * sizeof(U32));
                    if (maxBytes && ((char*)lim - (char*)data) > maxBytes)
                        break;
                    U8* out = fb.scanline<U8>(y);

                    for (U8* end = out + w * 4; out < end; in++)
                    {
                        Pixel p = *in;

                        // const U8 rv = p.red_most;
                        // const U8 gv = p.green_most;
                        // const U8 bv = p.blue_most;
                        const U8 rv =
                            p.red == 0x3ff ? p.red_most : ((p.red + 1) >> 2);
                        const U8 gv = p.green == 0x3ff ? p.green_most
                                                       : ((p.green + 1) >> 2);
                        const U8 bv =
                            p.blue == 0x3ff ? p.blue_most : ((p.blue + 1) >> 2);

                        *out = rv;
                        out++;
                        *out = gv;
                        out++;
                        *out = bv;
                        out++;
                        *out = 255;
                        out++;
                    }
                }
            }
        }
    }

    void Read10Bit::readRGB8_PLANAR(const string& filename,
                                    const unsigned char* data, FrameBuffer& fb,
                                    int w, int h, size_t maxBytes, bool swap)
    {
        planarConfig(fb, w, h, FrameBuffer::UCHAR);

        FrameBuffer* R = &fb;
        FrameBuffer* G = R->nextPlane();
        FrameBuffer* B = G->nextPlane();

        const float slop = 1.0 / 1023.0 / 2.0;

        if (swap)
        {
            for (int y = 0; y < h; y++)
            {
                Pixel* in = (Pixel*)(data + y * w * sizeof(U32));
                Pixel* lim = (Pixel*)(data + (y + 1) * w * sizeof(U32));
                if (maxBytes && ((char*)lim - (char*)data) > maxBytes)
                    break;
                U8* out = fb.scanline<U8>(y);

                U8* restrict r = R->scanline<U8>(y);
                U8* restrict g = G->scanline<U8>(y);
                U8* restrict b = B->scanline<U8>(y);

                for (U8* restrict end = r + w; r < end; r++, g++, b++, in++)
                {
                    Pixel p;
                    const U32 v = in->pixelWord;

                    p.pixelWord =
                        ((v & 0xff000000) >> 24) | ((v & 0x00ff0000) >> 8)
                        | ((v & 0x0000ff00) << 8) | ((v & 0x000000ff) << 24);

                    // const U8 rv = U8((float(p.red) / 1023.0f + slop) *
                    // 255.0f); const U8 gv = U8((float(p.green) / 1023.0f +
                    // slop) * 255.0f); const U8 bv = U8((float(p.blue) /
                    // 1023.0f + slop) * 255.0f);

                    const U8 rv =
                        p.red == 0x3ff ? p.red_most : ((p.red + 1) >> 2);
                    const U8 gv =
                        p.green == 0x3ff ? p.green_most : ((p.green + 1) >> 2);
                    const U8 bv =
                        p.blue == 0x3ff ? p.blue_most : ((p.blue + 1) >> 2);

                    // const U8 rv = p.red_most;
                    // const U8 gv = p.green_most;
                    // const U8 bv = p.blue_most;

                    *r = rv;
                    *g = gv;
                    *b = bv;
                }
            }
        }
        else
        {

            for (int y = 0; y < h; y++)
            {
                Pixel* in = (Pixel*)(data + y * w * sizeof(U32));
                Pixel* lim = (Pixel*)(data + (y + 1) * w * sizeof(U32));
                if (maxBytes && ((char*)lim - (char*)data) > maxBytes)
                    break;
                U8* out = fb.scanline<U8>(y);

                U8* restrict r = R->scanline<U8>(y);
                U8* restrict g = G->scanline<U8>(y);
                U8* restrict b = B->scanline<U8>(y);

                for (U8* restrict end = r + w; r < end; r++, g++, b++, in++)
                {
                    Pixel p = *in;

                    // const U8 rv = U8((float(p.red) / 1023.0f + slop) *
                    // 255.0f); const U8 gv = U8((float(p.green) / 1023.0f +
                    // slop) * 255.0f); const U8 bv = U8((float(p.blue) /
                    // 1023.0f + slop) * 255.0f); const U8 rv = p.red_most;
                    // const U8 gv = p.green_most;
                    // const U8 bv = p.blue_most;
                    const U8 rv =
                        p.red == 0x3ff ? p.red_most : ((p.red + 1) >> 2);
                    const U8 gv =
                        p.green == 0x3ff ? p.green_most : ((p.green + 1) >> 2);
                    const U8 bv =
                        p.blue == 0x3ff ? p.blue_most : ((p.blue + 1) >> 2);

                    *r = rv;
                    *g = gv;
                    *b = bv;
                }
            }
        }
    }

    void Read10Bit::readRGB8(const string& filename, const unsigned char* data,
                             FrameBuffer& fb, int w, int h, size_t maxBytes,
                             bool swap)
    {
        fb.restructure(w, h, 0, 3, FrameBuffer::UCHAR, 0, 0,
                       FrameBuffer::TOPLEFT, true);

        for (int y = 0; y < h; y++)
        {
            Pixel* in = (Pixel*)(data + y * w * sizeof(U32));
            Pixel* lim = (Pixel*)(data + (y + 1) * w * sizeof(U32));
            if (maxBytes && ((char*)lim - (char*)data) > maxBytes)
                break;
            U8* out = fb.scanline<U8>(y);

            if (swap)
            {
                for (U8* end = out + w * 3; out < end; in++)
                {
                    Pixel p;
                    const U32 v = in->pixelWord;

                    p.pixelWord =
                        ((v & 0xff000000) >> 24) | ((v & 0x00ff0000) >> 8)
                        | ((v & 0x0000ff00) << 8) | ((v & 0x000000ff) << 24);

                    const U8 rv =
                        p.red == 0x3ff ? p.red_most : ((p.red + 1) >> 2);
                    const U8 gv =
                        p.green == 0x3ff ? p.green_most : ((p.green + 1) >> 2);
                    const U8 bv =
                        p.blue == 0x3ff ? p.blue_most : ((p.blue + 1) >> 2);

                    *out = rv;
                    out++;
                    *out = gv;
                    out++;
                    *out = bv;
                    out++;
                }
            }
            else
            {
                for (U8* end = out + w * 3; out < end; in++)
                {
                    Pixel p = *in;

                    const U8 rv =
                        p.red == 0x3ff ? p.red_most : ((p.red + 1) >> 2);
                    const U8 gv =
                        p.green == 0x3ff ? p.green_most : ((p.green + 1) >> 2);
                    const U8 bv =
                        p.blue == 0x3ff ? p.blue_most : ((p.blue + 1) >> 2);

                    *out = rv;
                    out++;
                    *out = gv;
                    out++;
                    *out = bv;
                    out++;
                }
            }
        }
    }

    void Read10Bit::readRGB16(const string& filename, const unsigned char* data,
                              FrameBuffer& fb, int w, int h, size_t maxBytes,
                              bool swap)
    {
        fb.restructure(w, h, 0, 3, FrameBuffer::USHORT, 0, 0,
                       FrameBuffer::TOPLEFT, true);

        for (int y = 0; y < h; y++)
        {
            Pixel* in = (Pixel*)(data + y * w * sizeof(U32));
            Pixel* lim = (Pixel*)(data + (y + 1) * w * sizeof(U32));
            if (maxBytes && ((char*)lim - (char*)data) > maxBytes)
                break;
            U16* out = fb.scanline<U16>(y);

            if (swap)
            {
                for (U16* end = out + w * 3; out < end; in++)
                {
                    Pixel p;
                    const U32 v = in->pixelWord;

                    p.pixelWord =
                        ((v & 0xff000000) >> 24) | ((v & 0x00ff0000) >> 8)
                        | ((v & 0x0000ff00) << 8) | ((v & 0x000000ff) << 24);

                    //
                    //  We used to make 16bit values from 10bit values by just
                    //  shifting (16bit = 10bit << 6), which mapps the 10bit
                    //  values onto the "lattice points" within the 16bit range
                    //  separated by 2^6.  But these values are "really"
                    //  supposed to represent the floating range 0.0-1.0, so
                    //  only hitting these lattice points is actually wrong. The
                    //  mappping is correct for 0.0, but the error increases as
                    //  we approach 1.0.  In particular:
                    //
                    //  "10bit 1.0" == 1111111111 is mapped to 1111111111000000
                    //  != 1111111111111111 = "16bit 1.0"
                    //
                    //  This would be fine if we were going to just shift back
                    //  at some point, but these 16bit values really _will_ be
                    //  converted to floating point during rendering, and
                    //  rounding errors etc mean that even a 10bit "pass
                    //  through" with 16bit textures may result in errors large
                    //  enough that the output 10bit value will differ from the
                    //  input by 1 code value.
                    //
                    //  So we now convert from 10bit to 16bit by "passing
                    //  through" a floating point value, to ensure that 0->0 and
                    //  1->1 (and presumably everything inbetween is optimally
                    //  mapped.
                    //

                    const U16 rv = U16((double(65535l * p.red) / 1023.0l));
                    const U16 gv = U16((double(65535l * p.green) / 1023.0l));
                    const U16 bv = U16((double(65535l * p.blue) / 1023.0l));

                    *out = rv;
                    out++;
                    *out = gv;
                    out++;
                    *out = bv;
                    out++;
                }
            }
            else
            {
                for (U16* end = out + w * 3; out < end; in++)
                {
                    Pixel p = *in;

                    //
                    //  See above comment
                    //

                    const U16 rv = U16((double(65535l * p.red) / 1023.0l));
                    const U16 gv = U16((double(65535l * p.green) / 1023.0l));
                    const U16 bv = U16((double(65535l * p.blue) / 1023.0l));

                    *out = rv;
                    out++;
                    *out = gv;
                    out++;
                    *out = bv;
                    out++;
                }
            }
        }
    }

    void Read10Bit::readRGBA16(const string& filename,
                               const unsigned char* data, FrameBuffer& fb,
                               int w, int h, size_t maxBytes, bool alpha,
                               bool swap)
    {
        fb.restructure(w, h, 0, 4, FrameBuffer::USHORT, 0, 0,
                       FrameBuffer::TOPLEFT, true);

        if (alpha)
        {
            Pixel* in = (Pixel*)(data);
            size_t inindex = 0;

            const size_t nwords = (w * 4) / 3;
            const size_t rem = (w * 4) % 3;
            const size_t totalWords = (nwords + (rem ? 1 : 0)) * h;
            const bool truncatedScanline =
                (maxBytes / 4 == nwords * h) && rem != 0;
            const size_t brem = (w * h * 4) % 3;
            const size_t bwords = (w * h * 4) / 3 * 4 + (brem ? 4 : 0);
            const bool runonScanline = bwords == maxBytes && rem != 0;
            size_t ss = fb.width();

            if (truncatedScanline && runonScanline)
            {
                cout << "WARNING: " << filename
                     << " is both truncated and run-on" << endl;
            }

            if (truncatedScanline)
            {
                cout << "INFO: Reading as a truncated scanline file" << endl;
                cout << "INFO: " << filename << " is not following the DPX spec"
                     << endl;
                ss -= rem;
            }
            else if (runonScanline)
            {
                cout << "INFO: Read as a run-on scanline file" << endl;
                cout << "INFO: " << filename << " is not following the DPX spec"
                     << endl;
            }

            for (int y = 0; y < h; y++)
            {
                if (maxBytes && ((char*)in - (char*)data) > maxBytes)
                    break;
                U16* start = fb.scanline<U16>(y);

                for (U16 *out = start, *end = start + 4 * ss; out < end; out++)
                {
                    Pixel p;

                    if (swap)
                    {
                        const U32 v = in->pixelWord;

                        p.pixelWord = ((v & 0xff000000) >> 24)
                                      | ((v & 0x00ff0000) >> 8)
                                      | ((v & 0x0000ff00) << 8)
                                      | ((v & 0x000000ff) << 24);
                    }
                    else
                    {
                        p = *in;
                    }

                    //
                    //  See big comment in readRGB16() above.
                    //

                    switch (inindex)
                    {
                    case 0:
                        *out = U16((double(65535l * p.red) / 1023.0l));
                        break;
                    case 1:
                        *out = U16((double(65535l * p.green) / 1023.0l));
                        break;
                    case 2:
                        *out = U16((double(65535l * p.blue) / 1023.0l));
                        break;
                    }

                    inindex = (inindex + 1) % 3;

                    if (inindex == 0)
                    {
                        in++;
                        if (maxBytes && ((char*)in - (char*)data) > maxBytes)
                            break;
                    }
                }

                if (inindex != 0 && !runonScanline)
                {
                    //
                    //  As far as I can tell, this is correct. Both v1 and v2
                    //  DPX specs say that once you get to the end of a
                    //  scanline, the remaining data in the final word will be
                    //  zero and hence the padding is "broken". The next
                    //  scanline will start on the next word with the first
                    //  datum being reset.
                    //
                    //  Here's the relevant passage from v1 spec:
                    //
                    //      Any bits in the last word of a scan line left over
                    //      will be filled with zeroes. That is to say that the
                    //      packing is broken on scan-line boundaries.
                    //
                    //  Unforunately, it looks like both nuke and shake will
                    //  write images where this is not true. You usually don't
                    //  see this problem because the resolutions that people
                    //  care about (* 4) are usually divisible by 3 so this
                    //  doesn't even happen. In their case it looks like
                    //  instead of dropping the remaining bits, they drop the
                    //  remaining data from the end of the scanline. So they
                    //  just don't have a "last word of a scan line".
                    //  E.g. (400 * 4) / 3 => 533.33333 so there's one last
                    //  datum in the last word. It looks like nuke and shake
                    //  drop this datum (so they're missing the last alpha
                    //  value).
                    //
                    //  In constrast, the ARRISCAN111 for example just keeps
                    //  packing more data and ignores the scanline break all
                    //  together. We include a hack here to allow the reader to
                    //  optionally ignore the scanline break for this output.
                    //

                    in++;
                    inindex = 0;
                }
            }
        }
        else
        {
            for (int y = 0; y < h; y++)
            {
                Pixel* in = (Pixel*)(data + y * w * sizeof(U32));
                Pixel* lim = (Pixel*)(data + (y + 1) * w * sizeof(U32));
                if (maxBytes && ((char*)lim - (char*)data) > maxBytes)
                    break;
                U16* out = fb.scanline<U16>(y);

                if (swap)
                {
                    for (U16* end = out + w * 4; out < end; in++)
                    {
                        Pixel p;
                        const U32 v = in->pixelWord;

                        p.pixelWord = ((v & 0xff000000) >> 24)
                                      | ((v & 0x00ff0000) >> 8)
                                      | ((v & 0x0000ff00) << 8)
                                      | ((v & 0x000000ff) << 24);

                        //
                        //  See big comment in readRGB16() above.
                        //

                        const U16 rv = U16((double(65535l * p.red) / 1023.0l));
                        const U16 gv =
                            U16((double(65535l * p.green) / 1023.0l));
                        const U16 bv = U16((double(65535l * p.blue) / 1023.0l));

                        *out = rv;
                        out++;
                        *out = gv;
                        out++;
                        *out = bv;
                        out++;
                        *out = numeric_limits<U16>::max();
                        out++;
                    }
                }
                else
                {
                    for (U16* end = out + w * 4; out < end; in++)
                    {
                        Pixel p = *in;

                        //
                        //  See big comment in readRGB16() above.
                        //

                        const U16 rv = U16((double(65535l * p.red) / 1023.0l));
                        const U16 gv =
                            U16((double(65535l * p.green) / 1023.0l));
                        const U16 bv = U16((double(65535l * p.blue) / 1023.0l));

                        *out = rv;
                        out++;
                        *out = gv;
                        out++;
                        *out = bv;
                        out++;
                        *out = numeric_limits<U16>::max();
                        out++;
                    }
                }
            }
        }
    }

    void Read10Bit::readRGB16_PLANAR(const string& filename,
                                     const unsigned char* data, FrameBuffer& fb,
                                     int w, int h, size_t maxBytes, bool swap)
    {
        planarConfig(fb, w, h, FrameBuffer::USHORT);

        FrameBuffer* R = &fb;
        FrameBuffer* G = R->nextPlane();
        FrameBuffer* B = G->nextPlane();

        for (int y = 0; y < h; y++)
        {
            Pixel* in = (Pixel*)(data + y * w * sizeof(U32));
            Pixel* lim = (Pixel*)(data + (y + 1) * w * sizeof(U32));
            if (maxBytes && ((char*)lim - (char*)data) > maxBytes)
                break;

            U16* restrict r = R->scanline<U16>(y);
            U16* restrict g = G->scanline<U16>(y);
            U16* restrict b = B->scanline<U16>(y);

            if (swap)
            {
                for (U16* restrict end = r + w; r < end; r++, g++, b++, in++)
                {
                    Pixel p;
                    const U32 v = in->pixelWord;

                    p.pixelWord =
                        ((v & 0xff000000) >> 24) | ((v & 0x00ff0000) >> 8)
                        | ((v & 0x0000ff00) << 8) | ((v & 0x000000ff) << 24);

                    //
                    //  See big comment in readRGB16() above.
                    //

                    const U16 rv = U16((double(65535l * p.red) / 1023.0l));
                    const U16 gv = U16((double(65535l * p.green) / 1023.0l));
                    const U16 bv = U16((double(65535l * p.blue) / 1023.0l));

                    *r = rv;
                    *g = gv;
                    *b = bv;
                }
            }
            else
            {
                for (U16* restrict end = r + w; r < end; r++, g++, b++, in++)
                {
                    Pixel p = *in;

                    //
                    //  See big comment in readRGB16() above.
                    //

                    const U16 rv = U16((double(65535l * p.red) / 1023.0l));
                    const U16 gv = U16((double(65535l * p.green) / 1023.0l));
                    const U16 bv = U16((double(65535l * p.blue) / 1023.0l));

                    *r = rv;
                    *g = gv;
                    *b = bv;
                }
            }
        }
    }

    void Read10Bit::readRGB10_A2(const string& filename,
                                 const unsigned char* data, FrameBuffer& fb,
                                 int w, int h, size_t maxBytes, bool swap,
                                 bool useRaw, unsigned char* deletePointer)
    {
        fb.restructure(w, h, 0, 1, FrameBuffer::PACKED_R10_G10_B10_X2,
                       useRaw ? (unsigned char*)data : 0, 0,
                       FrameBuffer::TOPLEFT, true, 0, 0,
                       useRaw ? deletePointer : 0);
        //
        //  Scarf it all at once.
        //

        if (!useRaw)
        {
            if (maxBytes < fb.allocSize())
            {
                memcpy(fb.pixels<char>(), data, maxBytes);
            }
            else
            {
                memcpy(fb.pixels<char>(), data, sizeof(Pixel) * w * h);
            }
        }

        // if (useRaw) cout << "READ10: used raw" << endl;

        if (swap)
        {
            Timer t;
            t.start();
            TwkUtil::swapWords(fb.pixels<char>(), w * h);
            // cout << "READ10: swapped time = "
            //<< t.elapsed()
            //<< endl;
        }
    }

    void Read10Bit::readA2_BGR10(const string& filename,
                                 const unsigned char* data, FrameBuffer& fb,
                                 int w, int h, size_t maxBytes, bool swap)
    {
        fb.restructure(w, h, 0, 1, FrameBuffer::PACKED_X2_B10_G10_R10, 0, 0,
                       FrameBuffer::TOPLEFT, true, 0, 0);

        for (int y = 0; y < h; y++)
        {
            Pixel* in = (Pixel*)(data + y * w * sizeof(U32));
            Pixel* lim = (Pixel*)(data + (y + 1) * w * sizeof(U32));
            if (maxBytes && ((char*)lim - (char*)data) > maxBytes)
                break;
            Pixel10Rev* out = fb.scanline<Pixel10Rev>(y);

            if (swap)
            {
                for (Pixel10Rev* end = out + w; out < end; in++)
                {
                    Pixel p;
                    const U32 v = in->pixelWord;

                    p.pixelWord =
                        ((v & 0xff000000) >> 24) | ((v & 0x00ff0000) >> 8)
                        | ((v & 0x0000ff00) << 8) | ((v & 0x000000ff) << 24);

                    out->red = p.red;
                    out->green = p.green;
                    out->blue = p.blue;
                    out++;
                }
            }
            else
            {
                for (Pixel10Rev* end = out + w; out < end; in++)
                {
                    out->red = in->red;
                    out->green = in->green;
                    out->blue = in->blue;
                    out++;
                }
            }
        }
    }

    void Read10Bit::readYCrYCb8_422_PLANAR(const std::string& filename,
                                           const unsigned char* data,
                                           TwkFB::FrameBuffer& fb, int w, int h,
                                           size_t maxBytes, bool swap)
    {
        vector<string> planeNames(3);
        vector<int> xsamplings(3);
        vector<int> ysamplings(3);

        planeNames[0] = "Y";
        planeNames[1] = "U";
        planeNames[2] = "V";

        xsamplings[0] = 1;
        xsamplings[1] = 2;
        xsamplings[2] = 2;
        ysamplings[0] = 1;
        ysamplings[1] = 1;
        ysamplings[2] = 1;

        fb.restructurePlanar(w, h, xsamplings, ysamplings, planeNames,
                             FrameBuffer::UCHAR, FrameBuffer::TOPLEFT, 1);

        FrameBuffer* Y = &fb;
        FrameBuffer* U = Y->nextPlane();
        FrameBuffer* V = U->nextPlane();

        Pixel* in = (Pixel*)(data);
        size_t inindex = 0;

        // const size_t nwords = (w * 4) / 3;
        // const size_t rem    = (w * 4) % 3;
        // const size_t totalWords = (nwords + (rem ? 1 : 0)) * h;
        // const bool truncatedScanline = (maxBytes / 4 == nwords * h) && rem !=
        // 0;
        const bool truncatedScanline = false;
        // const size_t brem = (w * h * 4) % 3;
        // const size_t bwords = (w * h * 4) / 3 * 4 + (brem ? 4 : 0);
        // const bool runonScanline = bwords == maxBytes && rem != 0;
        const bool runonScanline = false;
        size_t ss = fb.scanlineSize();

        size_t outindex = 0;

        for (int y = 0; y < h; y++)
        {
            if (maxBytes && ((char*)in - (char*)data) > maxBytes)
                break;

            U8* start = Y->scanline<U8>(y);
            U8* outY = Y->scanline<U8>(y);
            U8* outU = U->scanline<U8>(y);
            U8* outV = V->scanline<U8>(y);

            for (U8* end = start + ss; outY < end;)
            {
                Pixel p;

                if (swap)
                {
                    const U32 v = in->pixelWord;

                    p.pixelWord =
                        ((v & 0xff000000) >> 24) | ((v & 0x00ff0000) >> 8)
                        | ((v & 0x0000ff00) << 8) | ((v & 0x000000ff) << 24);
                }
                else
                {
                    p = *in;
                }

                U8 value;

                switch (inindex)
                {
                case 0:
                    value = U8(p.red >> 2);
                    break;
                case 1:
                    value = U8(p.green >> 2);
                    break;
                case 2:
                    value = U8(p.blue >> 2);
                    break;
                }

                inindex = (inindex + 1) % 3;

                if (inindex == 0)
                {
                    in++;
                    if (maxBytes && ((char*)in - (char*)data) > maxBytes)
                        break;
                }

                switch (outindex)
                {
                case 2:
                    *outV = value;
                    outV++;
                    break;
                case 0:
                    *outU = value;
                    outU++;
                    break;
                case 1:
                case 3:
                    *outY = value;
                    outY++;
                    break;
                }

                outindex = (outindex + 1) % 4;
            }

            if (inindex != 0 && !runonScanline)
            {
                //
                //  As far as I can tell, this is correct. Both v1 and v2
                //  DPX specs say that once you get to the end of a
                //  scanline, the remaining data in the final word will be
                //  zero and hence the padding is "broken". The next
                //  scanline will start on the next word with the first
                //  datum being reset.
                //
                //  Here's the relevant passage from v1 spec:
                //
                //      Any bits in the last word of a scan line left over
                //      will be filled with zeroes. That is to say that the
                //      packing is broken on scan-line boundaries.
                //
                //  Unforunately, it looks like both nuke and shake will
                //  write images where this is not true. You usually don't
                //  see this problem because the resolutions that people
                //  care about (* 4) are usually divisible by 3 so this
                //  doesn't even happen. In their case it looks like
                //  instead of dropping the remaining bits, they drop the
                //  remaining data from the end of the scanline. So they
                //  just don't have a "last word of a scan line".
                //  E.g. (400 * 4) / 3 => 533.33333 so there's one last
                //  datum in the last word. It looks like nuke and shake
                //  drop this datum (so they're missing the last alpha
                //  value).
                //
                //  In constrast, the ARRISCAN111 for example just keeps
                //  packing more data and ignores the scanline break all
                //  together. We include a hack here to allow the reader to
                //  optionally ignore the scanline break for this output.
                //

                in++;
                inindex = 0;
            }
        }
    }

} //  End namespace TwkFB
