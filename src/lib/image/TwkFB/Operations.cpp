//******************************************************************************
// Copyright (c) 2004 Tweak Inc.
// All rights reserved.
// 
// SPDX-License-Identifier: Apache-2.0
// 
//******************************************************************************

#include <TwkFB/Operations.h>
#include <TwkFB/FrameBuffer.h>

#include <ImfRgbaYca.h>
#include <ImfChromaticities.h>
#include <ImathMatrix.h>
#include <ImathVec.h>
#include <RVMath/AlexaLogC.h>
#include <TwkMath/Math.h>
#include <TwkMath/Vec4.h>
#include <TwkMath/Vec3.h>
#include <TwkMath/Vec2.h>
#include <TwkMath/Mat44.h>
#include <TwkMath/MatrixColor.h>
#include <TwkMath/Iostream.h>
#include <TwkMath/Function.h>

#include <assert.h>
#include <algorithm>
#include <iostream>
#include <half.h>
#include <limits>
#include <string.h>


namespace TwkFB {
using namespace std;
using namespace TwkMath;
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint32;
typedef FrameBuffer::Pixel10 Pixel10;
typedef FrameBuffer::Pixel10Rev Pixel10Rev;


inline Imath::V2f convert(Vec2f v) { return Imath::V2f(v.x, v.y); }
inline Imath::V3f convert(Vec3f v) { return Imath::V3f(v.x, v.y, v.z); }
inline Vec2f convert(Imath::V2f v) { return Vec2f(v.x, v.y); }
inline Vec3f convert(Imath::V3f v) { return Vec3f(v.x, v.y, v.z); }

//
//  When converting assuming truncation. In order to accommodate
//  non-primitive float types which may not have math rounding
//  functions defined, we'll use this brain-dead method of centering
//

namespace {
template <typename F, typename I>
double
roundingSlop()
{
    const int bits = numeric_limits<I>::digits;
    const double slop = 1.0 / double(int(::pow(2.0, double(bits))) - 1);
    return F(slop / 2.0);
}

template<> double roundingSlop<double, unsigned char>() { return 1.0 / 255.0 / 2.0; }
template<> double roundingSlop<float, unsigned char>() { return 1.0 / 255.0 / 2.0; }
template<> double roundingSlop<half, unsigned char>() { return 1.0 / 255.0 / 2.0; }
template<> double roundingSlop<double, unsigned short>() { return 1.0 / 65535.0 / 2.0; }
template<> double roundingSlop<float, unsigned short>() { return 1.0 / 65535.0 / 2.0; }
template<> double roundingSlop<half, unsigned short>() { return 1.0 / 65535.0 / 2.0; }

template<> double roundingSlop<double, unsigned int>() { return 1.0 / 65535.0 / 2.0; }
template<> double roundingSlop<float, unsigned int>() { return 1.0 / 65535.0 / 2.0; }
template<> double roundingSlop<half, unsigned int>() { return 1.0 / 65535.0 / 2.0; }

template <typename A, typename B> A roundingMask() { return 0; }
template<> unsigned short roundingMask<unsigned short, unsigned char>() { return 0xff; }
template<> unsigned int roundingMask<unsigned int, unsigned char>() { return 0xffffff; }

template <typename A, typename B> A roundingCenter() { return 0; }
template<> unsigned short roundingCenter<unsigned short, unsigned char>() { return 0xff / 2; }
template<> unsigned int roundingCenter<unsigned int, unsigned char>() { return 0xffffff / 2; }

//
//  Specialized copy functions. These are scanline based in case the
//  fb has scanline padding (presumably for alignment) at some point
//  in the future. Conversion performance is a bottleneck for image
//  and sequence viewing so optimization here is a good thing.
//

template <typename A, typename B>
void
copyFloatingToFloating(const FrameBuffer* a, FrameBuffer* b)
{
    const size_t w    = a->width();
    const size_t h    = a->height();
    const size_t d    = a->depth() == 0 ? 1 : a->depth();
    const A*     in   = a->pixels<A>();
    const A*     end  = in + a->numChannels() * w * h * d;
    B*           out  = b->pixels<B>();

    for (;in < end; in++, out++)
    {
        *out = B(*in);
    }
}

template <typename A, typename B>
void
copyIntegralToIntegral(const FrameBuffer* a, FrameBuffer* b)
{
    const size_t adigits = numeric_limits<A>::digits;
    const size_t bdigits = numeric_limits<B>::digits;
    const size_t w       = a->width();
    const size_t h       = a->height();
    const size_t d       = a->depth() == 0 ? 1 : a->depth();
    const A*     in      = a->pixels<A>();
    const A*     end     = in + a->numChannels() * w * h * d;
    B*           out     = b->pixels<B>();

    if (adigits > bdigits)
    {
        // For the UINT 32bit case, just pass through the value.
        const unsigned int bits = ((adigits == 32)?0:adigits - bdigits);

        for (;in < end; in++, out++)
        {
            *out = *in >> bits;
        }
    }
    else
    {
        const A mask = roundingMask<A,B>();
        const A center = roundingCenter<A,B>();
        const A m = numeric_limits<A>::max();

        for (;in < end; in++, out++)
        {
            A v = *in;
            if ((v & mask) > center && v != m) v++;
            *out = v;
        }
    }
}

template <typename I, typename F>
void
copyIntegralToFloating(const FrameBuffer* a, FrameBuffer* b)
{
    const size_t w   = a->width();
    const size_t h   = a->height();
    const size_t d   = a->depth() == 0 ? 1 : a->depth();
    const I*     in  = a->pixels<I>();
    const I*     end = in + a->numChannels() * w * h * d;
    F*           out = b->pixels<F>();

    // For the UINT 32bit case, just pass through the value.
    if (numeric_limits<I>::digits == 32)
    {
        for (;in < end; in++, out++)
        {
            *out = float(*in);
        }
    }
    else
    {
        for (;in < end; in++, out++)
        {
            *out = float(double(*in) / double(numeric_limits<I>::max()));
        }
    }
}

template <typename F, typename I>
void
copyFloatingToIntegral(const FrameBuffer* a, FrameBuffer* b)
{
    const double  slop = roundingSlop<F,I>();
    const size_t w   = a->width();
    const size_t h   = a->height();
    const size_t d   = a->depth() == 0 ? 1 : a->depth();
    const F*     in  = a->pixels<F>();
    const F*     end = in + a->numChannels() * w * h * d;
    I*           out = b->pixels<I>();

    // For the UINT 32bit case, just pass through the value.
    if (numeric_limits<I>::digits == 32)
    {
        for (;in < end; in++, out++)
        {
            *out = I(*in);
        }
    }
    else
    {
        for (;in < end; in++, out++)
        {
            *out = I(clamp(double(*in) + slop, 0.0, 1.0) * numeric_limits<I>::max());
        }
    }
}


template <typename F,typename P>
void
copyIntegral10BitToFloating(const FrameBuffer* a, FrameBuffer* b)
{
    const size_t   w   = a->width();
    const size_t   h   = a->height();
    const size_t   d   = a->depth() == 0 ? 1 : a->depth();
    const F*       in  = a->pixels<F>();
    const F*       end = in + w * h * d;
    P*             out = b->pixels<P>();

    for (;in < end; in++)
    {
        F p = *in;
        const P rv = p.red / 1023.0;
        const P gv = p.green / 1023.0;
        const P bv = p.blue / 1023.0;

        *out = rv; out++;
        *out = gv; out++;
        *out = bv; out++;
    }
}

template <typename F, typename P>
void
copyFloatingToIntegral10Bit(const FrameBuffer* a, FrameBuffer* b)
{
    const double  slop = 1.0 / 1023.0 / 2.0;
    const size_t w   = a->width();
    const size_t h   = a->height();
    const size_t d   = a->depth() == 0 ? 1 : a->depth();
    const F*     in  = a->pixels<F>();
    const F*     end = in + a->numChannels() * w * h * d;
    P*           out = b->pixels<P>();

    for (;in < end; out++)
    {
        out->red   = clamp(double(*in) + slop, 0.0, 1.0) * 1023.0; in++;
        out->green = clamp(double(*in) + slop, 0.0, 1.0) * 1023.0; in++;
        out->blue  = clamp(double(*in) + slop, 0.0, 1.0) * 1023.0; in++;
    }
}

template <typename B, typename P>
void
copyIntegral10BitToIntegral(const FrameBuffer* a, FrameBuffer* b)
{
    const size_t adigits = 10;
    const size_t bdigits = numeric_limits<P>::digits;
    const size_t w       = a->width();
    const size_t h       = a->height();
    const size_t d       = a->depth() == 0 ? 1 : a->depth();
    const B*     in      = a->pixels<B>();
    const B*     end     = in + w * h * d;
    P*           out     = b->pixels<P>();

    const int bits = ((bdigits < adigits) ? 0 : ((int) bdigits - (int) adigits));

    for (;in < end; in++)
    {
        *out = in->red   << bits; out++;
        *out = in->green << bits; out++;
        *out = in->blue  << bits; out++;
    }
}


template <>
void
copyIntegral10BitToIntegral<unsigned char, Pixel10>(const FrameBuffer* a, FrameBuffer* b)
{
    const size_t   w   = a->width();
    const size_t   h   = a->height();
    const size_t   d   = a->depth() == 0 ? 1 : a->depth();
    const Pixel10* in  = a->pixels<Pixel10>();
    const Pixel10* end = in + w * h * d;
    unsigned char* out = b->pixels<unsigned char>();

    for (;in < end; in++)
    {
        *out = in->red == 0x3ff ? (in->red >> 2) : ((in->red + 1) >> 2); out++;
        *out = in->green == 0x3ff ? (in->green >> 2) : ((in->green + 1) >> 2); out++;
        *out = in->blue == 0x3ff ? (in->blue >> 2) : ((in->blue + 1) >> 2); out++;
    }
}

template <>
void
copyIntegral10BitToIntegral<unsigned char, Pixel10Rev>(const FrameBuffer* a, FrameBuffer* b)
{
    const size_t      w   = a->width();
    const size_t      h   = a->height();
    const size_t      d   = a->depth() == 0 ? 1 : a->depth();
    const Pixel10Rev* in  = a->pixels<Pixel10Rev>();
    const Pixel10Rev* end = in + w * h * d;
    unsigned char*    out = b->pixels<unsigned char>();

    for (;in < end; in++)
    {
        *out = in->red == 0x3ff ? (in->red >> 2) : ((in->red + 1) >> 2); out++;
        *out = in->green == 0x3ff ? (in->green >> 2) : ((in->green + 1) >> 2); out++;
        *out = in->blue == 0x3ff ? (in->blue >> 2) : ((in->blue + 1) >> 2); out++;
    }
}

}

//
//  General copy
//

void
copyPlane(const FrameBuffer* a, FrameBuffer* b)
{
    assert(a->width() == b->width() &&
           a->height() == b->height());

    int numColorsA = (  a->dataType() == FrameBuffer::PACKED_R10_G10_B10_X2 ||
                        a->dataType() == FrameBuffer::PACKED_X2_B10_G10_R10 ||
                        a->dataType() == FrameBuffer::PACKED_Y8_Cb8_Y8_Cr8 ||
                        a->dataType() == FrameBuffer::PACKED_Cb8_Y8_Cr8_Y8 )  ?  3 : a->numChannels();

    int numColorsB = (  b->dataType() == FrameBuffer::PACKED_R10_G10_B10_X2 ||
                        a->dataType() == FrameBuffer::PACKED_X2_B10_G10_R10 ||
                        b->dataType() == FrameBuffer::PACKED_Y8_Cb8_Y8_Cr8 ||
                        b->dataType() == FrameBuffer::PACKED_Cb8_Y8_Cr8_Y8 )  ?  3 : a->numChannels();

    assert(numColorsA == numColorsB);

    bool fallback = false;

    if (a->dataType() == b->dataType())
    {
        memcpy(b->pixels<unsigned char>(),
               a->pixels<unsigned char>(),
               a->dataSize());
    }
    else if (b->dataType() == FrameBuffer::UCHAR)
    {
        if (a->dataType() == FrameBuffer::HALF)
        {
            copyFloatingToIntegral<half,uchar>(a,b);
        }
        else if (a->dataType() == FrameBuffer::FLOAT)
        {
            copyFloatingToIntegral<float,uchar>(a,b);
        }
        else if (a->dataType() == FrameBuffer::USHORT)
        {
            copyIntegralToIntegral<ushort,uchar>(a,b);
        }
        else if (a->dataType() == FrameBuffer::UINT)
        {
            copyIntegralToIntegral<uint32,uchar>(a,b);
        }
        else if (a->dataType() == FrameBuffer::PACKED_R10_G10_B10_X2)
        {
            copyIntegral10BitToIntegral<Pixel10,uchar>(a,b);
        }
        else if (a->dataType() == FrameBuffer::PACKED_X2_B10_G10_R10)
        {
            copyIntegral10BitToIntegral<Pixel10Rev,uchar>(a,b);
        }
        else
        {
            fallback = true;
        }
    }
    else if (b->dataType() == FrameBuffer::USHORT)
    {
        if (a->dataType() == FrameBuffer::HALF)
        {
            copyFloatingToIntegral<half,ushort>(a,b);
        }
        else if (a->dataType() == FrameBuffer::FLOAT)
        {
            copyFloatingToIntegral<float,ushort>(a,b);
        }
        else if (a->dataType() == FrameBuffer::USHORT)
        {
            copyIntegralToIntegral<ushort,ushort>(a,b);
        }
        else if (a->dataType() == FrameBuffer::PACKED_R10_G10_B10_X2)
        {
            copyIntegral10BitToIntegral<Pixel10,ushort>(a,b);
        }
        else if (a->dataType() == FrameBuffer::PACKED_X2_B10_G10_R10)
        {
            copyIntegral10BitToIntegral<Pixel10Rev,ushort>(a,b);
        }
        else if (a->dataType() == FrameBuffer::UINT)
        {
            copyIntegralToIntegral<uint32,ushort>(a,b);
        }
        else
        {
            fallback = true;
        }
    }
    else if (b->dataType() == FrameBuffer::UINT)
    {
        if (a->dataType() == FrameBuffer::HALF)
        {
            copyFloatingToIntegral<half,uint32>(a,b);
        }
        else if (a->dataType() == FrameBuffer::FLOAT)
        {
            copyFloatingToIntegral<float,uint32>(a,b);
        }
        else if (a->dataType() == FrameBuffer::USHORT)
        {
            copyIntegralToIntegral<ushort,uint32>(a,b);
        }
        else if (a->dataType() == FrameBuffer::PACKED_R10_G10_B10_X2)
        {
            copyIntegral10BitToIntegral<Pixel10,uint32>(a,b);
        }
        else if (a->dataType() == FrameBuffer::PACKED_X2_B10_G10_R10)
        {
            copyIntegral10BitToIntegral<Pixel10,uint32>(a,b);
        }
        else if (a->dataType() == FrameBuffer::UINT)
        {
            copyIntegralToIntegral<uint32,uint32>(a,b);
        }
        else
        {
            fallback = true;
        }
    }
    else if (b->dataType() == FrameBuffer::HALF)
    {
        if (a->dataType() == FrameBuffer::UCHAR)
        {
            copyIntegralToFloating<uchar,half>(a,b);
        }
        else if (a->dataType() == FrameBuffer::USHORT)
        {
            copyIntegralToFloating<ushort,half>(a,b);
        }
        else if (a->dataType() == FrameBuffer::FLOAT)
        {
            copyFloatingToFloating<float,half>(a,b);
        }
        else if (a->dataType() == FrameBuffer::PACKED_R10_G10_B10_X2)
        {
            copyIntegral10BitToFloating<Pixel10,half>(a,b);
        }
        else if (a->dataType() == FrameBuffer::PACKED_X2_B10_G10_R10)
        {
            copyIntegral10BitToFloating<Pixel10Rev,half>(a,b);
        }
        else if (a->dataType() == FrameBuffer::UINT)
        {
            copyIntegralToFloating<uint32,half>(a,b);
        }
        else
        {
            fallback = true;
        }
    }
    else if (b->dataType() == FrameBuffer::FLOAT)
    {
        if (a->dataType() == FrameBuffer::HALF)
        {
            copyFloatingToFloating<half,float>(a, b);
        }
        else if (a->dataType() == FrameBuffer::UCHAR)
        {
            copyIntegralToFloating<uchar,float>(a, b);
        }
        else if (a->dataType() == FrameBuffer::USHORT)
        {
            copyIntegralToFloating<ushort,float>(a, b);
        }
        else if (a->dataType() == FrameBuffer::PACKED_R10_G10_B10_X2)
        {
            copyIntegral10BitToFloating<Pixel10,float>(a,b);
        }
        else if (a->dataType() == FrameBuffer::PACKED_X2_B10_G10_R10)
        {
            copyIntegral10BitToFloating<Pixel10Rev,float>(a,b);
        }
        else if (a->dataType() == FrameBuffer::UINT)
        {
            copyIntegralToFloating<uint32,float>(a, b);
        }
        else
        {
            fallback = true;
        }
    }
    else if (b->dataType() == FrameBuffer::PACKED_R10_G10_B10_X2)
    {
        if (a->dataType() == FrameBuffer::HALF)
        {
            copyFloatingToIntegral10Bit<half,Pixel10>(a, b);
        }
        else if (a->dataType() == FrameBuffer::FLOAT)
        {
            copyFloatingToIntegral10Bit<float,Pixel10>(a, b);
        }
        else
        {
            fallback = true;
        }
    }
    else if (b->dataType() == FrameBuffer::PACKED_X2_B10_G10_R10)
    {
        if (a->dataType() == FrameBuffer::HALF)
        {
            copyFloatingToIntegral10Bit<half,Pixel10Rev>(a, b);
        }
        else if (a->dataType() == FrameBuffer::FLOAT)
        {
            copyFloatingToIntegral10Bit<float,Pixel10Rev>(a, b);
        }
        else
        {
            fallback = true;
        }
    }
    else
    {
        fallback = true;
    }

    if (fallback)
    {
        if (a->numChannels() == 3)
        {
            for (int y = 0; y < b->height(); y++)
            {
                for (int x = 0; x < b->width(); x++)
                {
                    float p[4];
                    a->getPixel4f(x, y, p);
                    b->setPixel3f(p[0], p[1], p[2], x, y);
                }
            }
        }
        else if (a->numChannels() == 4)
        {
            for (int y = 0; y < b->height(); y++)
            {
                for (int x = 0; x < b->width(); x++)
                {
                    float p[4];
                    a->getPixel4f(x, y, p);
                    b->setPixel4f(p[0], p[1], p[2], p[3], x, y);
                }
            }
        }
        else
        {
            abort();
        }
    }

    if (a != b)
    {
        a->copyAttributesTo(b);
        if (b->isRootPlane()) b->setIdentifier(a->identifier());
        b->setOrientation(a->orientation());
    }
}


FrameBuffer*
copyConvertYRYBYtoYUV(const FrameBuffer* from,
                      FrameBuffer::DataType type)
{
    if (from->isPlanar())
    {
        const FrameBuffer* inY  = from;
        const FrameBuffer* inRY = inY->nextPlane();
        const FrameBuffer* inBY = inRY->nextPlane();

        const float ux = float(inY->width()) / float(inRY->width());
        const float uy = float(inY->height()) / float(inRY->height());
        const float vx = float(inY->width()) / float(inBY->width());
        const float vy = float(inY->height()) / float(inBY->height());

        FrameBuffer* m = mergePlanes(from);
        FrameBuffer* rgb = convertToLinearRGB709(m); delete m;
        FrameBuffer* rgbi = copyConvert(rgb, type); delete rgb;
        convertRGBtoYUV(rgbi, rgbi);
        FrameBufferVector planes = split(rgbi); delete rgbi;

        if (ux != 1.0 || uy != 1.0)
        {
            FrameBuffer* fb = new FrameBuffer(inRY->width(),
                                              inRY->height(),
                                              1, type);
            fb->setChannelName(0, "U");
            fb->setOrientation(inRY->orientation());

            nearestNeighborResize(planes[1], fb);
            delete planes[1];
            planes[1] = fb;
        }

        if (vx != 1.0 || vy != 1.0)
        {
            FrameBuffer* fb = new FrameBuffer(inBY->width(),
                                              inBY->height(),
                                              1, type);
            fb->setChannelName(0, "V");
            fb->setOrientation(inBY->orientation());

            nearestNeighborResize(planes[2], fb);
            delete planes[2];
            planes[2] = fb;
        }

        for (int i=1; i < planes.size(); i++)
        {
            planes[0]->appendPlane(planes[i]);
        }

        return planes[0];
    }
    else
    {
        const FrameBuffer* inY  = from;
        const FrameBuffer* inRY = inY->nextPlane();
        const FrameBuffer* inBY = inRY->nextPlane();

        FrameBuffer* rgb = convertToLinearRGB709(from);
        FrameBuffer* rgbi = copyConvert(rgb, type); delete rgb;
        convertRGBtoYUV(rgbi, rgbi);
        return rgbi;
    }

    return 0;
}


void
copy(const FrameBuffer* a, FrameBuffer* b)
{
    assert(a && b);
    assert(a->numPlanes() == b->numPlanes());

    do
    {
        copyPlane(a, b);
        a = a->nextPlane();
        b = b->nextPlane();
    }
    while (a);
}

namespace {

template <typename A, typename B>
void
applyFloatingToFloating(const FrameBuffer* a, FrameBuffer* b,
                        ColorTransformFunc F,
                        void *data)
{
    unsigned int n    = a->width() * a->numChannels();
    unsigned int nrow = a->height();

    vector<float> scanline(n);

    for (int row = 0; row < nrow; row++)
    {
        const A* ap = a->FrameBuffer::scanline<A>(row);
        B* bp = b->FrameBuffer::scanline<B>(row);

        for (float* fp = &scanline.front(), *ep = fp + n;
             fp < ep;
             fp++, ap++)
        {
            *fp = float(*ap);
        }

        F(&scanline.front(),
          &scanline.front(),
          a->numChannels(),
          a->width(), data);

        for (float* fp = &scanline.front(), *ep = fp + n;
             fp < ep;
             fp++, bp++)
        {
            *bp = B(*fp);
        }
    }
}

template<typename P>
void
applyIntegral10ToIntegral10(const FrameBuffer* a, FrameBuffer* b,
                            ColorTransformFunc F,
                            void* data)

{
    unsigned int n    = a->width() * 3;
    unsigned int nrow = a->height();

    vector<float> scanline(n);
    float slop = 1.0 / 1023.0 / 2.0;

    for (int row = 0; row < nrow; row++)
    {
        const P* ap = a->FrameBuffer::scanline<P>(row);
        P* bp = b->FrameBuffer::scanline<P>(row);

        for (float* fp = &scanline.front(), *ep = fp + n;
             fp < ep;
             ap++)
        {
            *fp = ap->red / 1023.0f; fp++;
            *fp = ap->green / 1023.0f; fp++;
            *fp = ap->blue / 1023.0f; fp++;
        }

        F(&scanline.front(), &scanline.front(), 3, a->width(), data);

        for (float* fp = &scanline.front(), *ep = fp + n;
             fp < ep;
             bp++)
        {
            P p;
            p.red   = int(clamp(*fp + slop, 0.0f, 1.0f) * 1023.0f); fp++;
            p.green = int(clamp(*fp + slop, 0.0f, 1.0f) * 1023.0f); fp++;
            p.blue  = int(clamp(*fp + slop, 0.0f, 1.0f) * 1023.0f); fp++;

            *bp = p;
        }
    }
}

template <typename A, typename B>
void
applyIntegralToIntegral(const FrameBuffer* a, FrameBuffer* b,
                        ColorTransformFunc F,
                        void* data)

{
    unsigned int n    = a->width() * a->numChannels();
    unsigned int nrow = a->height();
    int bits = numeric_limits<A>::digits - numeric_limits<B>::digits;

    float adiv = pow(2.0, double(numeric_limits<A>::digits)) - 1.0;
    float bmult = pow(2.0, double(numeric_limits<B>::digits)) - 1.0;

    vector<float> scanline(n);

    for (int row = 0; row < nrow; row++)
    {
        const A* ap = a->FrameBuffer::scanline<A>(row);
        B* bp = b->FrameBuffer::scanline<B>(row);

        for (float* fp = &scanline.front(), *ep = fp + n;
             fp < ep;
             fp++, ap++)
        {
            *fp = float(*ap) / adiv;
        }

        F(&scanline.front(),
          &scanline.front(),
          a->numChannels(),
          a->width(), data);

        for (float* fp = &scanline.front(), *ep = fp + n;
             fp < ep;
             fp++, bp++)
        {
            *bp = B(clamp(*fp, 0.0f, 1.0f) * bmult);
        }
    }
}

template <typename I, typename F>
void
applyIntegralToFloating(const FrameBuffer* a, FrameBuffer* b,
                        ColorTransformFunc C,
                        void* data)
{
    unsigned int n    = a->width() * a->numChannels();
    unsigned int nrow = a->height();

    float adiv = pow(2.0, double(numeric_limits<I>::digits)) - 1.0;
    vector<float> scanline(n);

    for (int row = 0; row < nrow; row++)
    {
        const I* ap = a->FrameBuffer::scanline<I>(row);
        F* bp = b->FrameBuffer::scanline<F>(row);

        for (float* fp = &scanline.front(), *ep = fp + n;
             fp < ep;
             fp++, ap++)
        {
            *fp = float(*ap) / adiv;
        }

        if (numeric_limits<float>::digits == numeric_limits<F>::digits)
        {
            C(&scanline.front(),
              &scanline.front(),
              a->numChannels(),
              a->width(), data);
        }
        else
        {
            C(&scanline.front(),
              &scanline.front(),
              a->numChannels(),
              a->width(), data);

            for (float* fp = &scanline.front(), *ep = fp + n;
                 fp < ep;
                 fp++, bp++)
            {
                *bp = F(*fp);
            }
        }
    }
}

template <typename F, typename I>
void
applyFloatingToIntegral(const FrameBuffer* a, FrameBuffer* b,
                        ColorTransformFunc C,
                        void* data)
{
    unsigned int n    = a->width() * a->numChannels();
    unsigned int nrow = a->height();
    float bmult = pow(2.0, double(numeric_limits<I>::digits)) - 1.0;
    vector<float> scanline(n);

    for (int row = 0; row < nrow; row++)
    {
        const F* ap = a->FrameBuffer::scanline<F>(row);
        I*       bp = b->FrameBuffer::scanline<I>(row);
        float*   fp = &scanline.front();

        for (const F* ep = ap + n; ap < ep; ap++, fp++)
        {
            *fp = *ap;
        }

        C(fp, fp, a->numChannels(), a->width(), data);

        for (float* ep = fp + n; fp < ep; fp++, bp++)
        {
            F v = *fp;
            if (v > 1.0f) v = 1.0f;
            else if (v < 0.0f) v = 0.0f;

            *bp = I(clamp(*fp, 0.0f, 1.0f) * bmult + 0.49);
        }
    }
}

}

void applyTransform(const FrameBuffer* a,
                    FrameBuffer* b,
                    ColorTransformFunc F,
                    void* data)
{
    assert(a->width() == b->width() &&
           a->height() == b->height());

    assert(a->numChannels() == b->numChannels());

    bool fallback = false;

    if (!a->isPlanar() && !b->isPlanar())
    {
        if (b->dataType() == FrameBuffer::UCHAR)
        {
            if (a->dataType() == FrameBuffer::UCHAR)
            {
                applyIntegralToIntegral<uchar,uchar>(a,b,F,data);
            }
            else if (a->dataType() == FrameBuffer::HALF)
            {
                applyFloatingToIntegral<half,uchar>(a,b,F,data);
            }
            else if (a->dataType() == FrameBuffer::FLOAT)
            {
                applyFloatingToIntegral<float,uchar>(a,b,F,data);
            }
            else if (a->dataType() == FrameBuffer::USHORT)
            {
                applyIntegralToIntegral<ushort,uchar>(a,b,F,data);
            }
            else if (a->dataType() == FrameBuffer::UINT)
            {
                applyIntegralToIntegral<uint32,uchar>(a,b,F,data);
            }
            else
            {
                fallback = true;
            }
        }
        else if (b->dataType() == FrameBuffer::USHORT)
        {
            if (a->dataType() == FrameBuffer::UCHAR)
            {
                applyIntegralToIntegral<uchar,ushort>(a,b,F,data);
            }
            else if (a->dataType() == FrameBuffer::HALF)
            {
                applyFloatingToIntegral<half,ushort>(a,b,F,data);
            }
            else if (a->dataType() == FrameBuffer::FLOAT)
            {
                applyFloatingToIntegral<float,ushort>(a,b,F,data);
            }
            else if (a->dataType() == FrameBuffer::USHORT)
            {
                applyIntegralToIntegral<ushort,ushort>(a,b,F,data);
            }
            else if (a->dataType() == FrameBuffer::UINT)
            {
                applyIntegralToIntegral<uint32,ushort>(a,b,F,data);
            }
            else
            {
                fallback = true;
            }
        }
        else if (b->dataType() == FrameBuffer::UINT)
        {
            if (a->dataType() == FrameBuffer::UCHAR)
            {
                applyIntegralToIntegral<uchar,uint32>(a,b,F,data);
            }
            else if (a->dataType() == FrameBuffer::HALF)
            {
                applyFloatingToIntegral<half,uint32>(a,b,F,data);
            }
            else if (a->dataType() == FrameBuffer::FLOAT)
            {
                applyFloatingToIntegral<float,uint32>(a,b,F,data);
            }
            else if (a->dataType() == FrameBuffer::USHORT)
            {
                applyIntegralToIntegral<ushort,uint32>(a,b,F,data);
            }
            else if (a->dataType() == FrameBuffer::UINT)
            {
                applyIntegralToIntegral<uint32,uint32>(a,b,F,data);
            }
            else
            {
                fallback = true;
            }
        }
        else if (b->dataType() == FrameBuffer::HALF)
        {
            if (a->dataType() == FrameBuffer::UCHAR)
            {
                applyIntegralToFloating<uchar,half>(a,b,F,data);
            }
            else if (a->dataType() == FrameBuffer::HALF)
            {
                applyFloatingToFloating<half,half>(a,b,F,data);
            }
            else if (a->dataType() == FrameBuffer::USHORT)
            {
                applyIntegralToFloating<ushort,half>(a,b,F,data);
            }
            else if (a->dataType() == FrameBuffer::FLOAT)
            {
                applyFloatingToFloating<float,half>(a,b,F,data);
            }
            else if (a->dataType() == FrameBuffer::UINT)
            {
                applyIntegralToFloating<uint32,half>(a,b,F,data);
            }
            else
            {
                fallback = true;
            }
        }
        else if (b->dataType() == FrameBuffer::FLOAT)
        {
            if (a->dataType() == FrameBuffer::HALF)
            {
                applyFloatingToFloating<half,float>(a, b,F,data);
            }
            else if (a->dataType() == FrameBuffer::FLOAT)
            {
                applyFloatingToFloating<float,float>(a, b,F,data);
            }
            else if (a->dataType() == FrameBuffer::UCHAR)
            {
                applyIntegralToFloating<uchar,float>(a, b,F,data);
            }
            else if (a->dataType() == FrameBuffer::USHORT)
            {
                applyIntegralToFloating<ushort,float>(a, b,F,data);
            }
            else if (a->dataType() == FrameBuffer::UINT)
            {
                applyIntegralToFloating<uint32,float>(a, b,F,data);
            }
        }
        else if (b->dataType() == FrameBuffer::PACKED_R10_G10_B10_X2)
        {
            if (a->dataType() == FrameBuffer::PACKED_R10_G10_B10_X2)
            {
                applyIntegral10ToIntegral10<Pixel10>(a, b, F, data);
            }
            else
            {
                fallback = true;
            }
        }
        else if (b->dataType() == FrameBuffer::PACKED_X2_B10_G10_R10)
        {
            if (a->dataType() == FrameBuffer::PACKED_X2_B10_G10_R10)
            {
                applyIntegral10ToIntegral10<Pixel10Rev>(a, b, F, data);
            }
            else
            {
                fallback = true;
            }
        }
        else
        {
            fallback = true;
        }
    }
    else
    {
        fallback = true;
    }

    if (fallback)
    {
        if (a->numChannels() <= 3)
        {
            for (int y = 0; y < b->height(); y++)
            {
                for (int x = 0; x < b->width(); x++)
                {
                    float p[4];
                    a->getPixel4f(x, y, p);
                    F(p, p, 3, 1, data);
                    b->setPixel3f(p[0], p[1], p[2], x, y);
                }
            }
        }
        else if (a->numChannels() == 4)
        {
            for (int y = 0; y < b->height(); y++)
            {
                for (int x = 0; x < b->width(); x++)
                {
                    float p[4];
                    a->getPixel4f(x, y, p);
                    F(p, p, 4, 1, data);
                    b->setPixel4f(p[0], p[1], p[2], p[3], x, y);
                }
            }
        }
        else
        {
            abort();
        }
    }
}

//----------------------------------------------------------------------
//
//  Transform functions (passed to applyTransform)
//

namespace {
const double cinblack = pow(10.0, 95.0 * 0.002 / 0.6);
const double cinwhite = pow(10.0, 685.0 * 0.002 / 0.6);
const double cinwbdiff = cinwhite - cinblack;
const double cinbwdiff = cinblack - cinwhite;
}

void
logLinearTransform(const float* inPixels,
                   float* outPixels,
                   int channels,
                   int nelements,
                   void* data) // << bool vector indicating which channels to do
{
    const float* end = inPixels + (nelements * channels);
    bool* channelMask = reinterpret_cast<bool*>(data);
    int count = 0;


    for (const float* p = inPixels; p < end; p++, outPixels++, count++)
    {
        if (channelMask[count % channels])
        {
            *outPixels = (Math<double>::pow(10, *p * 3.41) - cinblack) / cinwbdiff;
        }
    }
}

void
linearLogTransform(const float* inPixels,
                   float* outPixels,
                   int channels,
                   int nelements,
                   void* data) // << bool array indicating which channels to do
{
    const float* end = inPixels + (nelements * channels);
    bool* channelMask = reinterpret_cast<bool*>(data);
    int count = 0;

    for (const float* p = inPixels; p < end; p++, outPixels++, count++)
    {
        if (channelMask[count % channels])
        {
            *outPixels = Math<double>::log10((cinblack / cinwbdiff + *p) * cinwbdiff) / 3.41;
        }
    }
}

void
logCToLinearTransform(const float* inPixels,
                     float* outPixels,
                     int channels,
                     int nelements,
                     void* data) // << float vector params
{
    const float* end = inPixels + (nelements * channels);
    LogCTransformParams* params = reinterpret_cast<LogCTransformParams*>(data);
    bool* channelMask = params->chmap;
    int count = 0;

    const float pbs    = params->LogCBlackSignal;       // ColorSpace::LogCBlackSignal()
    const float eo     = params->LogCEncodingOffset;    // ColorSpace::LogCEncodingOffset()
    const float eg     = params->LogCEncodingGain;      // ColorSpace::LogCEncodingGain()
    const float gs     = params->LogCGraySignal;        // ColorSpace::LogCGraySignal()
    const float bo     = params->LogCBlackOffset;       // ColorSpace::LogCBlackOffset()
    const float ls     = params->LogCLinearSlope;       // ColorSpace::LogCLinearSlope()
    const float lo     = params->LogCLinearOffset;      // ColorSpace::LogCLinearOffset()
    const float cutoff = params->LogCLinearCutPoint;    // ColorSpace::LogCLinearCutPoint()

    const float A      = 1.0f / eg;
    const float B      = -eo / eg;
    const float C      = gs;
    const float D      = pbs - bo * gs;
    const float X      = gs / (eg * ls);
    const float Y      = -(eo + eg*(ls*(bo - pbs / gs) + lo)) * X;

    for (const float* p = inPixels; p < end; p++, outPixels++, count++)
    {
        if (channelMask[count % channels])
        {
            const float v = *p;

            if (v <= cutoff)
            {
                *outPixels = v * X + Y;
            }
            else
            {
                *outPixels = Math<double>::pow(10, v * A + B) * C + D;
            }
        }
    }
}

void
linearToLogCTransform(const float* inPixels,
                      float* outPixels,
                      int channels,
                      int nelements,
                      void* data) // << float vector params
{
    const float* end = inPixels + (nelements * channels);
    LogCTransformParams* params = reinterpret_cast<LogCTransformParams*>(data);
    bool* channelMask = params->chmap;
    int count = 0;

    const float pbs    = params->LogCBlackSignal;       // ColorSpace::LogCBlackSignal()
    const float eo     = params->LogCEncodingOffset;    // ColorSpace::LogCEncodingOffset()
    const float eg     = params->LogCEncodingGain;      // ColorSpace::LogCEncodingGain()
    const float gs     = params->LogCGraySignal;        // ColorSpace::LogCGraySignal()
    const float bo     = params->LogCBlackOffset;       // ColorSpace::LogCBlackOffset()
    const float cutoff = params->LogCCutPoint;          // ColorSpace::LogCCutPoint()

    const float ls_eg = params->LogCLinearSlope * eg;
    const float lo_eg_eo = params->LogCLinearOffset * eg + eo;

    for (const float* p = inPixels; p < end; p++, outPixels++, count++)
    {
        if (channelMask[count % channels])
        {
            const float xr = bo + (std::max(*p, 0.0f) - pbs) / gs;

            if (xr <= cutoff)
            {
                *outPixels = xr * ls_eg + lo_eg_eo;;
            }
            else
            {
                *outPixels = Math<double>::log10(xr) * eg + eo;
            }
        }
    }
}


void
redLogLinearTransform(const float* inPixels,
                   float* outPixels,
                   int channels,
                   int nelements,
                   void* data) // << bool vector indicating which channels to do
{
    const float* end = inPixels + (nelements * channels);
    bool* channelMask = reinterpret_cast<bool*>(data);
    int count = 0;


    for (const float* p = inPixels; p < end; p++, outPixels++, count++)
    {
        if (channelMask[count % channels])
        {
            if (*p < 0)
            {
                *outPixels =  (1.0 - Math<double>::pow(10.0, 2.0 * Math<double>::abs(*p))) / 99.0;
            }
            else
            {
                *outPixels =  (Math<double>::pow(10.0, 2.0 * (*p)) - 1.0) / 99.0;
            }
        }
    }
}

void
linearRedLogTransform(const float* inPixels,
                   float* outPixels,
                   int channels,
                   int nelements,
                   void* data) // << bool array indicating which channels to do
{
    const float* end = inPixels + (nelements * channels);
    bool* channelMask = reinterpret_cast<bool*>(data);
    int count = 0;

    for (const float* p = inPixels; p < end; p++, outPixels++, count++)
    {
        if (channelMask[count % channels])
        {
            if (*p < 0)
            {
                *outPixels =  -0.5 * Math<double>::log10(99.0 *  Math<double>::abs(*p) + 1.0);

            }
            else
            {
                *outPixels =  0.5 * Math<double>::log10(99.0 * (*p) + 1.0);
            }
        }
    }
}


void
linearColorTransform(const float* in,
                     float* out,
                     int channels,   // has to be 3
                     int nelements,
                     void* data) // << Mat44f*
{
    Mat44f M = *reinterpret_cast<Mat44f*>(data);

    switch (channels)
    {
      case 3:
          {
              const Vec3f* inPixels  = reinterpret_cast<const Vec3f*>(in);
              Vec3f*       outPixels = reinterpret_cast<Vec3f*>(out);

              const Vec3f* end = inPixels + nelements;

              for (const Vec3f* p = inPixels; p < end; p++, outPixels++)
              {
                  *outPixels = M * *p;
              }
              break;
          }
      case 4: // assume 4th is alpha and should not be transformed
          {
              const Vec4f* inPixels  = reinterpret_cast<const Vec4f*>(in);
              Vec4f*       outPixels = reinterpret_cast<Vec4f*>(out);

              const Vec4f* end = inPixels + nelements;

              for (const Vec4f* p = inPixels; p < end; p++, outPixels++)
              {
                  Vec3f t = M * Vec3f(p->x, p->y, p->z);
                  *outPixels = Vec4f(t.x, t.y, t.z, p->w);
              }

              break;
          }
      case 2: // assume Y A
          {
              const Vec2f* inPixels  = reinterpret_cast<const Vec2f*>(in);
              Vec2f*       outPixels = reinterpret_cast<Vec2f*>(out);

              const Vec2f* end = inPixels + nelements;

              for (const Vec2f* p = inPixels; p < end; p++, outPixels++)
              {
                  Vec3f t = M * Vec3f(p->x, 0, 0);
                  *outPixels = Vec2f(t.x, p->y);
              }

              break;
          }
      case 1:
          {
              const float* end = in + nelements;

              for (; in < end; in++, out++)
              {
                  float y = *in;
                  *out = (M * Vec3f(y)).x;
              }

              break;
          }
    }
}

void
yryby2rgbColorTransform(const float* in,
                        float* out,
                        int channels,   // has to be 3
                        int nelements,
                        void* data) // Vec3f* Yw
{
    if (channels == 3)
    {
        const Vec3f* inPixels  = reinterpret_cast<const Vec3f*>(in);
        Vec3f*       outPixels = reinterpret_cast<Vec3f*>(out);
        const Vec3f* end       = inPixels + nelements;
        const Vec3f  Yw        = *reinterpret_cast<Vec3f*>(data);

        for (const Vec3f* p = inPixels; p < end; p++, outPixels++)
        {
            if (p->y == 0.0f && p->z == 0.0f)
            {
                //
                //  Set to luminance
                //

                *outPixels = Vec3f(p->x);
            }
            else
            {
                const float Y = p->x;
                const float r = (p->y + 1.0f) * Y;
                const float b = (p->z + 1.0f) * Y;
                const float g = (Y - r * Yw.x - b * Yw.z) / Yw.y;

                *outPixels = Vec3f(r, g, b);
            }
        }
    }
    else if (channels == 4)
    {
        const Vec4f* inPixels  = reinterpret_cast<const Vec4f*>(in);
        Vec4f*       outPixels = reinterpret_cast<Vec4f*>(out);
        const Vec4f* end       = inPixels + nelements;
        const Vec4f  Yw        = *reinterpret_cast<Vec4f*>(data);

        for (const Vec4f* p = inPixels; p < end; p++, outPixels++)
        {
            if (p->y == 0.0f && p->z == 0.0f)
            {
                //
                //  Set to luminance
                //

                *outPixels = Vec4f(p->x, p->x, p->x, p->w);
            }
            else
            {
                const float Y = p->x;
                const float r = (p->y + 1.0f) * Y;
                const float b = (p->z + 1.0f) * Y;
                const float g = (Y - r * Yw.x - b * Yw.z) / Yw.y;
                const float a = p->w;

                *outPixels = Vec4f(r, g, b, a);
            }
        }
    }
}

void
rgb2yrybyColorTransform(const float* in,
                        float* out,
                        int channels,   // has to be 3 or 4
                        int nelements,
                        void* data) // Vec3f* Yw
{
    if (channels == 3)
    {
        const Vec3f* inPixels  = reinterpret_cast<const Vec3f*>(in);
        Vec3f*       outPixels = reinterpret_cast<Vec3f*>(out);
        const Vec3f* end       = inPixels + nelements;
        const Vec3f  Yw        = *reinterpret_cast<Vec3f*>(data);

        for (const Vec3f* p = inPixels; p < end; p++, outPixels++)
        {
            const float r = p->x < 0.0f ? 0.0f : p->x;
            const float g = p->y < 0.0f ? 0.0f : p->y;
            const float b = p->z < 0.0f ? 0.0f : p->z;
            const float Y = r * Yw.x + g * Yw.y + b * Yw.z;

            if (r == g && r == b)
            {
                //
                //  Use luminance. This handles the case of Y == 0 as
                //  well (divide by 0)
                //

                *outPixels = Vec3f(Y, 0.0f, 0.0f);
            }
            else
            {
                *outPixels = Vec3f(Y, (r - Y) / Y, (b - Y) / Y);
            }
        }
    }
    else if (channels == 4)
    {
        const Vec4f* inPixels  = reinterpret_cast<const Vec4f*>(in);
        Vec4f*       outPixels = reinterpret_cast<Vec4f*>(out);
        const Vec4f* end       = inPixels + nelements;
        const Vec4f  Yw        = *reinterpret_cast<Vec4f*>(data);

        for (const Vec4f* p = inPixels; p < end; p++, outPixels++)
        {
            const float r = p->x < 0.0f ? 0.0f : p->x;
            const float g = p->y < 0.0f ? 0.0f : p->y;
            const float b = p->z < 0.0f ? 0.0f : p->z;
            const float a = p->w < 0.0f ? 0.0f : p->w;

            const float Y = r * Yw.x + g * Yw.y + b * Yw.z;

            if (r == g && r == b)
            {
                //
                //  Use luminance. This handles the case of Y == 0 as
                //  well (divide by 0)
                //

                *outPixels = Vec4f(Y, 0.0f, 0.0f, a);
            }
            else
            {
                *outPixels = Vec4f(Y, (r - Y) / Y, (b - Y) / Y, a);
            }
        }
    }
}

void
premultTransform(const float* inPixels,
                 float* outPixels,
                 int channels,
                 int nelements,
                 void* data) // nothing
{
    assert(channels == 4 || channels == 2);

    const float* end = inPixels + (nelements * channels);
    bool* channelMask = reinterpret_cast<bool*>(data);
    int count = 0;

    if (channels == 4)
    {
        for (const float* p = inPixels; p < end; p+=4, outPixels+=4)
        {
            const float r = p[0];
            const float g = p[1];
            const float b = p[2];
            const float a = p[3];

            outPixels[0] = r * a;
            outPixels[1] = g * a;
            outPixels[2] = b * a;
        }
    }
    else
    {
        for (const float* p = inPixels; p < end; p+=2, outPixels+=2)
        {
            outPixels[0] = p[0] * p[1];
        }
    }
}

void
unpremultTransform(const float* inPixels,
                   float* outPixels,
                   int channels,
                   int nelements,
                   void* data) // nothing
{
    assert(channels == 4 || channels == 2);

    const float* end = inPixels + (nelements * channels);
    bool* channelMask = reinterpret_cast<bool*>(data);
    int count = 0;

    if (channels == 4)
    {
        for (const float* p = inPixels; p < end; p+=4, outPixels+=4)
        {
            const float r = p[0];
            const float g = p[1];
            const float b = p[2];
            const float a = p[3];

            outPixels[0] = a != 0.0 ? r / a : 1.0;
            outPixels[1] = a != 0.0 ? g / a : 1.0;
            outPixels[2] = a != 0.0 ? b / a : 1.0;
        }
    }
    else
    {
        for (const float* p = inPixels; p < end; p+=2, outPixels+=2)
        {
            outPixels[0] = p[1] != 0.0 ? p[0] / p[1] : 1.0;
        }
    }
}


void
floatChromaToIntegralTransform(const float* inPixels,
                               float* outPixels,
                               int channels,
                               int nelements,
                               void* data) // nothing
{
    assert(channels != 2);

    const float* end = inPixels + (nelements * channels);
    bool* channelMask = reinterpret_cast<bool*>(data);
    int count = 0;

    if (channels == 1)
    {
        for (const float* p = inPixels; p < end; p+=1, outPixels+=1)
        {
            *outPixels = max(0.0, min(*p + 0.5, 1.0));
        }
    }
    else
    {
        for (const float* p = inPixels;
             p < end;
             p+=channels, outPixels+=channels)
        {
            outPixels[1] = max(0.0, min(p[1] + 0.5, 1.0));
            outPixels[2] = max(0.0, min(p[2] + 0.5, 1.0));
        }
    }
}


void
gammaTransform(const float* inPixels,
               float* outPixels,
               int channels,
               int nelements,
               void* data) // float* -> gamma values
{
    const float* end = inPixels + (nelements * channels);
    bool channelMask[4] = {true, true, true, false};
    size_t count = 0;
    const float* ingammas = reinterpret_cast<float*>(data);
    float gammas[4] = {1.0f / ingammas[0], 1.0f / ingammas[1], 1.0f / ingammas[2], 1.0f};

    for (const float* p = inPixels; p < end; p++, outPixels++, count++)
    {
        //
        //  If the number of channels is odd (no alpha) or the current
        //  p is not the last channel
        //

        if (channelMask[count % channels])
        {
            *outPixels = TwkMath::Math<double>::pow(*p, gammas[count % channels]);
        }
    }
}

void
powerTransform(const float* inPixels,
               float* outPixels,
               int channels,
               int nelements,
               void* data) // float* -> gamma values
{
    const float* end = inPixels + (nelements * channels);
    bool channelMask[4] = {true, true, true, false};
    size_t count = 0;
    const float* powers = reinterpret_cast<float*>(data);

    for (const float* p = inPixels; p < end; p++, outPixels++, count++)
    {
        //
        //  If the number of channels is odd (no alpha) or the current
        //  p is not the last channel
        //

        if (channelMask[count % channels])
        {
            *outPixels = TwkMath::Math<double>::pow(*p, powers[count % channels]);
        }
    }
}

void
sRGBtoLinearTransform (const float* inPixels,
                       float* outPixels,
                       int channels,
                       int nelements,
                       void* data) // nothing
{
    const float* end = inPixels + (nelements * channels);
    bool* channelMask = reinterpret_cast<bool*>(data);
    size_t count = 0;

    for (const float* p = inPixels; p < end; p++, outPixels++, count++)
    {
        //
        //  If the number of channels is odd (no alpha) or the current
        //  p is not the last channel
        //

        if (channelMask[count % channels])
        {
            const double c = *p;

            if (c <= 0.04045)
            {
                *outPixels = c / 12.92;
            }
            else
            {
                *outPixels = pow((c + 0.055) / (1.0 + 0.055), 2.4);
            }
        }
    }
}

void
linearToSRGBTransform(const float* inPixels,
                      float* outPixels,
                      int channels,
                      int nelements,
                      void* data) // nothing
{
    const float* end = inPixels + (nelements * channels);
    bool* channelMask = reinterpret_cast<bool*>(data);
    size_t count = 0;

    for (const float* p = inPixels; p < end; p++, outPixels++, count++)
    {
        //
        //  If the number of channels is odd (no alpha) or the current
        //  p is not the last channel
        //

        if (channelMask[count % channels])
        {
            const double c = *p;

            if (c <= 0.0031308)
            {
                *outPixels = c * 12.92;
            }
            else
            {
                *outPixels = (1.0 + 0.055) * ::pow(c, 1.0 / 2.4) - 0.055;
            }
        }
    }
}

void
Rec709toLinearTransform (const float* inPixels,
                         float* outPixels,
                         int channels,
                         int nelements,
                         void* data) // nothing
{
    const float* end = inPixels + (nelements * channels);
    bool* channelMask = reinterpret_cast<bool*>(data);
    size_t count = 0;

    for (const float* p = inPixels; p < end; p++, outPixels++, count++)
    {
        //
        //  If the number of channels is odd (no alpha) or the current
        //  p is not the last channel
        //

        if (channelMask[count % channels])
        {
            const double c = *p;

            if (c <= 0.081)
            {
                *outPixels = c / 4.5;
            }
            else
            {
                *outPixels = pow((c + 0.099) / 1.099, 1.0/.45);
            }
        }
    }
}

void
linearToRec709Transform(const float* inPixels,
                        float* outPixels,
                        int channels,
                        int nelements,
                        void* data) // nothing
{
    const float* end = inPixels + (nelements * channels);
    bool* channelMask = reinterpret_cast<bool*>(data);
    size_t count = 0;

    for (const float* p = inPixels; p < end; p++, outPixels++, count++)
    {
        //
        //  If the number of channels is odd (no alpha) or the current
        //  p is not the last channel
        //

        if (channelMask[count % channels])
        {
            const double c = *p;

            if (c <= 0.018)
            {
                *outPixels = c * 4.5;
            }
            else
            {
                *outPixels = 1.099 * pow(c, 0.45) - 0.099;
            }
        }
    }
}

void
channelLUTTransform(const float* inPixels,
                    float* outPixels,
                    int channels,
                    int nelements,
                    void* data)
{
    //
    //  FB is expected to be width x 1 in size and 3 channels.
    //

    FrameBuffer* fb        = reinterpret_cast<FrameBuffer*>(data);
    const size_t width     = fb->width();
    const int    wi        = width - 1;
    const float  wf        = float(wi);
    const size_t uchannels = !(channels % 2) ? channels - 1 : channels;
    float*       o         = outPixels;

    for (const float* p = inPixels, *e = p + (nelements * channels);
         p < e;
         o+=channels, p+=channels)
    {
        for (size_t q = 0; q < channels; q++)
        {
            if (q < uchannels)
            {
                float pixel0[4];
                float pixel1[4];

                const float f0 = clamp(p[q], 0.0f, 1.0f) * wf;
                const int   x0 = int(f0);
                const int   x1 = x0 == wi ? x0 : x0 + 1;

                fb->getPixel4f(x0, 0, pixel0);
                fb->getPixel4f(x1, 0, pixel1);

                o[q] = lerp(pixel0[q], pixel1[q], f0 - float(x0));
            }
            else
            {
                o[q] = p[q];
            }
        }
    }
}

void
luminanceLUTTransform(const float* inPixels,
                      float* outPixels,
                      int channels,
                      int nelements,
                      void* data)
{
    //
    //  FB is expected to be width x 1 in size and 3 channels.
    //

    FrameBuffer* fb        = reinterpret_cast<FrameBuffer*>(data);
    const size_t width     = fb->width();
    const int    wi        = width - 1;
    const float  wf        = float(wi);
    const size_t uchannels = !(channels % 2) ? channels - 1 : channels;
    float*       o         = outPixels;

    Mat44f M = Rec709FullRangeRGBToYUV8<float>();
    const float rw709 = M.m00;
    const float gw709 = M.m01;
    const float bw709 = M.m02;

    Mat44f L = Mat44f(rw709,  gw709,   bw709,   0,
                      rw709,  gw709,   bw709,   0,
                      rw709,  gw709,   bw709,   0,
                      0,     0,      0,      1);

    if (channels >= 3)
    {
        for (const float* p = inPixels, *e = p + (nelements * channels);
             p < e;
             o+=channels, p+=channels)
        {
            float l = (L * Vec3f(p[0], p[1], p[2])).x;

            float pixel0[4];
            float pixel1[4];

            const float f0 = clamp(l, 0.0f, 1.0f) * wf;
            const int   x0 = int(f0);
            const int   x1 = x0 == wi ? x0 : x0 + 1;

            fb->getPixel4f(x0, 0, pixel0);
            fb->getPixel4f(x1, 0, pixel1);

            Vec3f c0(pixel0[0], pixel0[1], pixel0[2]);
            Vec3f c1(pixel1[0], pixel1[1], pixel1[2]);

            Vec3f c = lerp(c0, c1, f0 - float(x0));

            o[0] = c.x;
            o[1] = c.y;
            o[2] = c.z;
        }
    }
}

void
pixel3DLUTTransform(const float* inPixels,
                    float* outPixels,
                    int channels,
                    int nelements,
                    void* data)
{
    //
    //  FB is expected to be 3 channels float
    //  The input should be either 3 or 4 channels.
    //

    FrameBuffer* infb = reinterpret_cast<FrameBuffer*>(data);
    FrameBuffer* fb = infb->dataType() == FrameBuffer::FLOAT ? infb : copyConvert(infb, FrameBuffer::FLOAT);
    const size_t xs = fb->width();
    const size_t ys = fb->height();
    const size_t zs = fb->depth();
    const size_t xl = xs - 1;
    const size_t yl = ys - 1;
    const size_t zl = zs - 1;

    const Vec3f* lut = fb->pixels<Vec3f>();

    if (channels >= 3)
    {
        float* o = outPixels;

        for (const float* p = inPixels, *e = p + nelements * channels;
             p < e;
             o+=channels, p+=channels)
        {
            //
            //  find the corners
            //

            Vec3f ip = Vec3f(clamp(p[0], 0.0f, 1.0f),
                             clamp(p[1], 0.0f, 1.0f),
                             clamp(p[2], 0.0f, 1.0f));

            Vec3f vn = ip * Vec3f(xl, yl, zl);

            const size_t x0 = size_t(vn.x);
            const size_t y0 = size_t(vn.y);
            const size_t z0 = size_t(vn.z);
            const size_t x1 = x0 == xl ? xl : x0 + 1;
            const size_t y1 = y0 == yl ? yl : y0 + 1;
            const size_t z1 = z0 == zl ? zl : z0 + 1;

            Vec3f corners[2][2][2];

            corners[0][0][0] = lut[xs * ys * z0 + xs * y0 + x0];
            corners[0][0][1] = lut[xs * ys * z0 + xs * y0 + x1];
            corners[0][1][0] = lut[xs * ys * z0 + xs * y1 + x0];
            corners[0][1][1] = lut[xs * ys * z0 + xs * y1 + x1];
            corners[1][0][0] = lut[xs * ys * z1 + xs * y0 + x0];
            corners[1][0][1] = lut[xs * ys * z1 + xs * y0 + x1];
            corners[1][1][0] = lut[xs * ys * z1 + xs * y1 + x0];
            corners[1][1][1] = lut[xs * ys * z1 + xs * y1 + x1];

            float t[3] = {vn.x - float(x0),
                          vn.y - float(y0),
                          vn.z - float(z0)};

            Vec3f op = trilinear(corners, t);

            o[0] = op.x;
            o[1] = op.y;
            o[2] = op.z;
        }
    }

    if (fb != infb) delete fb;
}

//----------------------------------------------------------------------
//
//    This converts the funky apple packed YUV 4:2:2 format into 3
//    channel RGB in REC_709 linear space.
//
namespace {

template <class T>
void convertYUVS(const FrameBuffer* infb,
                 FrameBuffer* outfb,
                 double cmax,
                 bool reverse)
{
    Mat44d C;

    if (infb->primaryColorSpace() == ColorSpace::Rec709())
    {
        C = Rec709FullRangeYUVToRGB8<float>();
    }
    else
    {
        C = Rec601FullRangeYUVToRGB8<float>();
    }

    //
    //  Mult in scale to final data type
    //

    Mat44d B(cmax, 0, 0, 0,
             0, cmax, 0, 0,
             0, 0, cmax, 0,
             0, 0, 0,    1);

    Mat44d A = B * C;

#ifndef restrict
#define restrict
#endif

    for (int y = 0; y < infb->height(); y++)
    {
        const unsigned short* row = infb->scanline<unsigned short>(y);
        T* out = outfb->scanline<T>(y);

        if (reverse)
        {
            for (const unsigned short* restrict p = row, *e = row + infb->width();
                 p < e;)
            {
                const double y0 = double(*p >> 8);
                const double u = double(*p & 0xff);
                p++;
                const double y1 = double(*p >> 8);
                const double v = double(*p & 0xff);
                p++;

                const Vec3d rgb0 = A * Vec3d(y0, u, v);
                const Vec3d rgb1 = A * Vec3d(y1, u, v);

                *out = (T)clamp(rgb0.x, 0.0, cmax); out++;
                *out = (T)clamp(rgb0.y, 0.0, cmax); out++;
                *out = (T)clamp(rgb0.z, 0.0, cmax); out++;
                *out = (T)clamp(rgb1.x, 0.0, cmax); out++;
                *out = (T)clamp(rgb1.y, 0.0, cmax); out++;
                *out = (T)clamp(rgb1.z, 0.0, cmax); out++;
            }
        }
        else
        {
            for (const unsigned short* restrict p = row, *e = row + infb->width();
                 p < e;)
            {
                const double u = double(*p >> 8);
                const double y0 = double(*p & 0xff);
                p++;
                const double v = double(*p >> 8);
                const double y1 = double(*p & 0xff);
                p++;

                const Vec3d rgb0 = A * Vec3d(y0, u, v);
                const Vec3d rgb1 = A * Vec3d(y1, u, v);

                *out = (T)clamp(rgb0.x, 0.0, cmax); out++;
                *out = (T)clamp(rgb0.y, 0.0, cmax); out++;
                *out = (T)clamp(rgb0.z, 0.0, cmax); out++;
                *out = (T)clamp(rgb1.x, 0.0, cmax); out++;
                *out = (T)clamp(rgb1.y, 0.0, cmax); out++;
                *out = (T)clamp(rgb1.z, 0.0, cmax); out++;
            }
        }
    }
}

FrameBuffer*
copyConvertPackedYUVS(const FrameBuffer* f, FrameBuffer::DataType d)
{
    FrameBuffer::StringVector names;
    names.push_back("R");
    names.push_back("G");
    names.push_back("B");

    FrameBuffer::DataType nd = d;

    if (d != FrameBuffer::UCHAR &&
        d != FrameBuffer::HALF &&
        d != FrameBuffer::FLOAT &&
        d != FrameBuffer::USHORT &&
        d != FrameBuffer::UINT)
    {
        nd = FrameBuffer::UCHAR;
    }

    FrameBuffer* fnew = new FrameBuffer(f->coordinateType(),
                                        f->width(), f->height(), f->depth(),
                                        3, nd,
                                        0,
                                        &names,
                                        f->orientation(),
                                        true);

    typedef unsigned char byte;
    bool reverse = f->dataType() == FrameBuffer::PACKED_Y8_Cb8_Y8_Cr8;

    switch (nd)
    {
      default:
      case FrameBuffer::UCHAR:
          convertYUVS<byte>(f, fnew, 255.0, reverse);
          break;

      case FrameBuffer::USHORT:
          convertYUVS<unsigned short>(f, fnew, 65335.0, reverse);
          break;

      case FrameBuffer::UINT:
          convertYUVS<unsigned int>(f, fnew, 65335.0, reverse);
          break;

      case FrameBuffer::HALF:
          convertYUVS<byte>(f, fnew, 1.0, reverse);
          break;

      case FrameBuffer::FLOAT:
          convertYUVS<float>(f, fnew, 1.0, reverse);
          break;
    }

    fnew->setPrimaryColorSpace(ColorSpace::Rec709());
    fnew->setTransferFunction(ColorSpace::Linear());

    if (d != nd)
    {
        FrameBuffer* fnew2 = copyConvert(fnew, d);
        delete fnew;
        return fnew2;
    }

    fnew->setUncrop(f);

    return fnew;
}

}

//----------------------------------------------------------------------

template <typename P>
FrameBuffer*
copyConvertPacked10Bit(const FrameBuffer* f, FrameBuffer::DataType d)
{
    FrameBuffer::StringVector names;
    names.push_back("R");
    names.push_back("G");
    names.push_back("B");

    FrameBuffer* fnew = new FrameBuffer(f->coordinateType(),
                                        f->width(), f->height(), f->depth(),
                                        3, d,
                                        0,
                                        &names,
                                        f->orientation(),
                                        true);

    switch (d)
    {
      case FrameBuffer::HALF:
          copyIntegral10BitToFloating<P, half>(f, fnew);
          break;
      case FrameBuffer::FLOAT:
          copyIntegral10BitToFloating<P, float>(f, fnew);
          break;
      case FrameBuffer::DOUBLE:
          copyIntegral10BitToFloating<P, double>(f, fnew);
          break;
      case FrameBuffer::UCHAR:
          copyIntegral10BitToIntegral<P, unsigned char>(f, fnew);
          break;
      case FrameBuffer::USHORT:
          copyIntegral10BitToIntegral<P, unsigned short>(f, fnew);
          break;
      case FrameBuffer::UINT:
          copyIntegral10BitToIntegral<P, unsigned int>(f, fnew);
          break;
      default:
          abort();
    }

    return fnew;
}

//----------------------------------------------------------------------

FrameBuffer*
copyConvertPlane(const FrameBuffer* f, FrameBuffer::DataType d)
{
    if (f->dataType() == FrameBuffer::PACKED_Cb8_Y8_Cr8_Y8 ||
        f->dataType() == FrameBuffer::PACKED_Y8_Cb8_Y8_Cr8)
    {
        return copyConvertPackedYUVS(f, d);
    }
    else if (f->dataType() == FrameBuffer::PACKED_R10_G10_B10_X2)
    {
        return copyConvertPacked10Bit<Pixel10>(f, d);
    }
    else if (f->dataType() == FrameBuffer::PACKED_X2_B10_G10_R10)
    {
        return copyConvertPacked10Bit<Pixel10Rev>(f, d);
    }
    else
    {
        FrameBuffer::StringVector names = f->channelNames();
        FrameBuffer* fnew = new FrameBuffer(f->coordinateType(),
                                            f->width(), f->height(), f->depth(),
                                            f->numChannels(), d,
                                            0,
                                            &names,
                                            f->orientation(),
                                            true);

        copyPlane(f, fnew);
        return fnew;
    }
}

FrameBuffer*
copyConvert(const FrameBuffer* from, FrameBuffer::DataType d)
{
    FrameBuffer* result = 0;

    for (const FrameBuffer* f = from->firstPlane(); f; f = f->nextPlane())
    {
        if (result) result->appendPlane(copyConvertPlane(f, d));
        else result = copyConvertPlane(f, d);
    }

    result->setUncrop(from);

    return result;
}

void
resample(const FrameBuffer* a, FrameBuffer* b)
{
    for (;a && b; a = a->nextPlane(), b = b->nextPlane())
    {
        assert(a->width() == b->width() &&
               a->height() == b->height());

        assert(a->numChannels() == b->numChannels());

        if (a->dataType() == b->dataType())
        {
            memcpy(b->pixels<unsigned char>(),
                   a->pixels<unsigned char>(),
                   a->scanlineSize() * a->height());
        }
        else
        {
            for (int y = 0; y < b->height(); y++)
            {
                for (int x = 0; x < b->width(); x++)
                {
                    float p[4];
                    a->getPixel4f(x, y, p);
                    b->setPixel4f(p[0], p[1], p[2], p[3], x, y);
                }
            }
        }
    }
}

namespace {

unsigned char*
getscanline(FrameBuffer* fb, int y)
{
    return fb->scanline<unsigned char>(y);
}

const unsigned char*
getscanline(const FrameBuffer* fb, int y)
{
    return fb->scanline<const unsigned char>(y);
}

template <typename T>
void
normalizeRange(FrameBuffer* fb,
               bool discardmax,
               const T maxval,
               bool invert)
{
    T mn = numeric_limits<T>::max();
    T mx = -mn;

    const unsigned int nc = fb->numChannels();
    const bool skipLast = nc == 4 || nc == 2;

    //
    //  Find min/max values, possibly ignore alpha
    //  possibly ignore max value (shadow maps)
    //

    for (int y = 0; y < fb->height(); y++)
    {
        //unsigned char* b = fb->scanline<unsigned char>(y);
        unsigned char* b = getscanline(fb, y); // 3.2 compiler bug requires this
        const unsigned char* e = b + fb->scanlineSize();
        unsigned int count = 1; // 1 pulls computation out of mod below

        for (const T* p = reinterpret_cast<const T*>(b);
             p < reinterpret_cast<const T*>(e);
             p++, count++)
        {
            if (skipLast && (count % nc) == 0) continue;
            if (discardmax && *p >= maxval) continue;
            if (mx < *p) mx = *p;
            if (mn > *p) mn = *p;
        }
    }

    //
    //  Apply the normalization to the data
    //

    const T r = mx - mn;

    for (int y = 0; y < fb->height(); y++)
    {
        //unsigned char* b = fb->scanline<unsigned char>(y);
        unsigned char* b = getscanline(fb, y);  // 3.2 compiler bug requires this
        unsigned char* e = b + fb->scanlineSize();
        unsigned int count = 1; // 1 pulls computation out of mod below

        for (T* p = reinterpret_cast<T*>(b);
             p < reinterpret_cast<const T*>(e);
             p++, count++)
        {
            if (skipLast && (count % nc) == 0) continue;

            if (discardmax && *p >= maxval)
            {
                *p = T(0);
            }
            else if (invert)
            {
                *p = T(1.0) - T(double(*p) / double(r) - double(mn) / double(r));
            }
            else
            {
                *p = T(double(*p) / double(r) - double(mn) / double(r));
            }
        }
    }

    fb->newAttribute("NormalizedMax", mx);
    fb->newAttribute("NormalizedMin", mn);
}

}

void
normalize(FrameBuffer* fb, bool discardmax, bool invert)
{
    switch (fb->dataType())
    {
      case FrameBuffer::HALF:
          normalizeRange<half>(fb, discardmax, 1e6, invert);
          break;
      case FrameBuffer::FLOAT:
          // these maxvals are hardcoded from entropy's shadow maps
          // is the number different for prman?
          normalizeRange<float>(fb, discardmax, 1e10, invert);
          break;
      case FrameBuffer::DOUBLE:
          normalizeRange<double>(fb, discardmax, 1e10, invert);
          break;
      default:
          abort();
    }
}

void
nearestNeighborResize(const FrameBuffer* a, FrameBuffer* b)
{
    assert(a->numChannels() == b->numChannels());

    for (;a && b; a = a->nextPlane(), b = b->nextPlane())
    {
        for (int by = 0; by < b->height(); by++)
        {
            const float ndcy = float(by) / float(b->height() - 1);
            const float ay   = (ndcy > 1.0f ? 1.0f : ndcy) * float(a->height() - 1);

            for (int bx = 0; bx < b->width(); bx++)
            {
                const float ndcx = float(bx) / float(b->width() - 1);
                const float ax   = (ndcx > 1.0f ? 1.0f : ndcx) * float(a->width() - 1);

                const int iax = int(ax);
                const int iay = int(ay);

                float p[4];
                a->getPixel4f(iax, iay, p);
                b->setPixel4f(p[0], p[1], p[2], p[3], bx, by);
            }
        }
    }
}

void
scaledTransfer(const float* inpixel, float* outpixel, void* data)
{
    float scale = *reinterpret_cast<float*>(data);
    outpixel[0] += inpixel[0] * scale;
    outpixel[1] += inpixel[1] * scale;
    outpixel[2] += inpixel[2] * scale;
    outpixel[3] += inpixel[3] * scale;
}

void
minTransfer(const float* inpixel, float* outpixel, void* data)
{
    outpixel[0] = std::min(outpixel[0], inpixel[0]);
    outpixel[1] = std::min(outpixel[1], inpixel[1]);
    outpixel[2] = std::min(outpixel[2], inpixel[2]);
    outpixel[3] = std::min(outpixel[3], inpixel[3]);
}

void
maxTransfer(const float* inpixel, float* outpixel, void* data)
{
    outpixel[0] = std::max(outpixel[0], inpixel[0]);
    outpixel[1] = std::max(outpixel[1], inpixel[1]);
    outpixel[2] = std::max(outpixel[2], inpixel[2]);
    outpixel[3] = std::max(outpixel[3], inpixel[3]);
}

void
transfer(const FrameBuffer* a, FrameBuffer* b, TransferFunc F, void* data)
{
    assert(a->numChannels() == b->numChannels());

    for (;a && b; a = a->nextPlane(), b = b->nextPlane())
    {
        const float xdiv = float(a->width()) / float(b->width());
        const float ydiv = float(a->height()) / float(b->height());
        assert(xdiv == ydiv && xdiv == float(int(xdiv)));

        for (int ay = 0; ay < a->height(); ay++)
        {
            const float ndcy = float(ay) / float(a->height() - 1);
            const float by   = (ndcy > 1.0f ? 1.0f : ndcy) * float(b->height() - 1);
            const int   iby  = int(by);

            for (int ax = 0; ax < a->width(); ax++)
            {
                const float ndcx = float(ax) / float(a->width() - 1);
                const float bx   = (ndcx > 1.0f ? 1.0f : ndcx) * float(b->width() - 1);
                const int   ibx  = int(bx);

                float inpixel[4], outpixel[4];
                a->getPixel4f(ax, ay, inpixel);
                b->getPixel4f(ibx, iby, outpixel);

                F(inpixel, outpixel, data);

                b->setPixel4f(outpixel[0], outpixel[1],
                              outpixel[2], outpixel[3],
                              ibx, iby);
            }
        }
    }
}

FrameBuffer*
channelMapToPlanar(const FrameBuffer* in, vector<string> newMapping)
{
    FrameBufferVector planes;

    if (!in->isPlanar())
    {
        FrameBufferVector fbs;
        fbs = split(in);

        for (size_t q = 0; q < newMapping.size(); q++)
        {
            for (size_t i=0; i < fbs.size(); i++)
            {
                if (newMapping[q] == fbs[i]->channelName(0))
                {
                    planes.push_back(fbs[i]);
                    break;
                }
            }
        }
    }
    else
    {
        for (int i=0; i < newMapping.size(); i++)
        {
            for (const FrameBuffer* f = in->firstPlane(); f; f = f->nextPlane())
            {
                if (newMapping[i] == f->channelName(0))
                {
                    planes.push_back(f->copyPlane());
                    break;
                }
            }
        }
    }

    if (planes.empty()) return 0;

    FrameBuffer* fb = planes.front();

    for (int i=1; i < planes.size(); i++)
    {
        if (planes[i]->numPlanes() != 1 || planes[i] == fb)
        {
            //
            //  There's a repeat in the map
            //

            fb->appendPlane(planes[i]->copy());
        }
        else
        {
            fb->appendPlane(planes[i]);
        }
    }

    fb->setUncrop(in);

    return fb;
}


FrameBuffer*
channelMap(const FrameBuffer* in, const vector<string>& newMapping)
{
    size_t n = newMapping.size();
    vector<int> cindex(newMapping.size());
    const FrameBuffer* in1 = in;

    if (in->isPlanar())
    {
        in = mergePlanes(in);
    }

    FrameBuffer* out = new FrameBuffer(in->width(),
                                       in->height(),
                                       n,
                                       in->dataType(),
                                       0,
                                       &newMapping,
                                       in->orientation());

    for (int i=0; i < n; i++)
    {
        const string& name = newMapping[i];

        for (int q=0; q < in->numChannels(); q++)
        {
            if (name == in->channelName(q))
            {
                cindex[i] = q;
                continue;
            }
        }
    }

    bool sameIndices = true;
    for (int i=0; i < cindex.size(); i++) if (cindex[i] != i) sameIndices = false;
    size_t b = out->bytesPerChannel();

    if (sameIndices && out->numChannels() < in->numChannels())
    {
        //
        //  In this case we're just deleting channels off the end. This
        //  happens a LOT during file output (e.g. no alpha)
        //

        const size_t chunkSize = out->pixelSize();
        const size_t pixelSize = in->pixelSize();
        const unsigned char* inp  = in->pixels<unsigned char>();
        unsigned char* outp = out->pixels<unsigned char>();

        for (const unsigned char* endp = inp + in->planeSize();
             inp < endp;
             inp += pixelSize,
             outp += chunkSize)
        {
            memcpy(outp, inp, chunkSize);
        }
    }
    else
    {
        for (int y = 0; y < in->height(); y++)
        {
            for (int x = 0; x < in->width(); x++)
            {
                unsigned char* p0 = &(in->pixel<unsigned char>(x,y));
                unsigned char* p1 = &(out->pixel<unsigned char>(x,y));

                for (int q = 0; q < cindex.size(); q++)
                {
                    memcpy(p1 + q * b, p0 + cindex[q] * b, b);
                }
            }
        }
    }

    if (in1 != in) delete (FrameBuffer*)in;
    out->setUncrop(in1);

    return out;
}

void applyGamma(const FrameBuffer* a, FrameBuffer* b, float gamma)
{
    assert(a->width() == b->width() &&
           a->height() == b->height());

    for (int y = 0; y < b->height(); y++)
    {
        for (int x = 0; x < b->width(); x++)
        {
            float p[4];
            a->getPixel4f(x, y, p);

            p[0] = std::max(0.0f, p[0]);
            p[1] = std::max(0.0f, p[1]);
            p[2] = std::max(0.0f, p[2]);

            p[0] = TwkMath::Math<double>::pow(p[0], 1.0/(double)gamma);
            p[1] = TwkMath::Math<double>::pow(p[1], 1.0/(double)gamma);
            p[2] = TwkMath::Math<double>::pow(p[2], 1.0/(double)gamma);

            b->setPixel4f(p[0], p[1], p[2], p[3], x, y);
        }
    }
}

// We'll discuss what 'applyGamma' means, but for now, this is silly :)
void linearizeFromGamma(const FrameBuffer* a, FrameBuffer* b, float fromGamma)
{
    applyGamma(a, b, fromGamma);
}

template <class T>
void
minMaxInteger(const FrameBuffer* fb,
              std::vector<float>& mins,
              std::vector<float>& maxs)
{
    mins.resize(fb->numChannels());
    maxs.resize(fb->numChannels());

    const size_t nrow         = fb->height();
    const size_t nchannels    = fb->numChannels();
    const size_t scanlineSize = fb->scanlineSize() / sizeof(T);
    const T maxValue          = numeric_limits<T>::max();

    fill(mins.begin(), mins.end(), numeric_limits<float>::max());
    fill(maxs.begin(), maxs.end(), numeric_limits<float>::min());

    for (int row = 0; row < nrow; row++)
    {
        const T* begin = reinterpret_cast<const T*>(getscanline(fb, row));
        const T* end   = begin + scanlineSize;

        for (int ch = 0; ch < nchannels; ch++)
        {
            float cmax = numeric_limits<float>::min();
            float cmin = numeric_limits<float>::max();

            for (const T* p = begin + ch; p < end; p+=nchannels)
            {
                float v = double(*p) / double(maxValue);
                if (v > cmax) cmax = v;
                if (v < cmin) cmin = v;
            }

            if (mins[ch] > cmin) mins[ch] = cmin;
            if (maxs[ch] < cmax) maxs[ch] = cmax;
        }
    }
}

template <class T>
void
minMaxFloat(const FrameBuffer* fb,
            std::vector<float> mins,
            std::vector<float> maxs)
{
    mins.resize(fb->numChannels());
    maxs.resize(fb->numChannels());

    const size_t nrow         = fb->height();
    const size_t nchannels    = fb->numChannels();
    const size_t scanlineSize = fb->scanlineSize() / sizeof(T);
    const T maxValue          = numeric_limits<T>::max();

    fill(mins.begin(), mins.end(), numeric_limits<float>::max());
    fill(maxs.begin(), maxs.end(), numeric_limits<float>::min());

    for (int row = 0; row < nrow; row++)
    {
        const T* begin = reinterpret_cast<const T*>(getscanline(fb, row));
        const T* end   = begin + scanlineSize;

        for (int ch = 0; ch < nchannels; ch++)
        {
            float cmax = numeric_limits<float>::min();
            float cmin = numeric_limits<float>::max();

            for (const T* p = begin + ch; p < end; p+=nchannels)
            {
                float v = *p;
                if (v > cmax) cmax = v;
                if (v < cmin) cmin = v;
            }

            if (mins[ch] > cmin) mins[ch] = cmin;
            if (maxs[ch] < cmax) maxs[ch] = cmax;
        }
    }
}

void minMax(const FrameBuffer* fb,
            std::vector<float>& minValues,
            std::vector<float>& maxValues)
{
    switch (fb->dataType())
    {
      case FrameBuffer::UCHAR:
          minMaxInteger<unsigned char>(fb, minValues, maxValues);
          break;
      case FrameBuffer::USHORT:
          minMaxInteger<unsigned short>(fb, minValues, maxValues);
          break;
      case FrameBuffer::UINT:
          minMaxInteger<unsigned int>(fb, minValues, maxValues);
          break;
      case FrameBuffer::FLOAT:
          minMaxFloat<float>(fb, minValues, maxValues);
          break;
      case FrameBuffer::HALF:
          minMaxFloat<half>(fb, minValues, maxValues);
          break;
      case FrameBuffer::DOUBLE:
          minMaxFloat<double>(fb, minValues, maxValues);
          break;
      default:
          break;
    }
}

void flip(FrameBuffer *flipMe)
{
    for (FrameBuffer* fb = flipMe->firstPlane(); fb; fb = fb->nextPlane())
    {
        int ymax = fb->height() - 1;
        vector<unsigned char> temp(fb->scanlineSize());

        for (int y = 0; y < (ymax+1)/2; y++)
        {
            memcpy(&temp.front(),
                   fb->scanline<unsigned char>(y),
                   fb->scanlineSize());

            memcpy(fb->scanline<unsigned char>(y),
                   fb->scanline<unsigned char>(ymax - y),
                   fb->scanlineSize());

            memcpy(fb->scanline<unsigned char>(ymax - y),
                   &temp.front(),
                   fb->scanlineSize());
        }
    }
}

void
flop(FrameBuffer *flopMe)
{
    for (FrameBuffer* fb = flopMe->firstPlane(); fb; fb = fb->nextPlane())
    {
        for (int y = 0; y < fb->height(); y++)
        {
            for (int x = 0; x < fb->width()/2; x++)
            {
                // SLOOOOOOW!
                float pLeft[4], pRight[4];
                fb->getPixel4f(x, y, pLeft);
                fb->getPixel4f(fb->width()-1-x, y, pRight);

                fb->setPixel4f(pRight[0], pRight[1], pRight[2], pRight[3], x, y);
                fb->setPixel4f(pLeft[0],  pLeft[1],  pLeft[2],  pLeft[3],  fb->width()-1-x, y);
            }
        }
    }
}

FrameBufferVector
split(const FrameBuffer* fb)
{
    FrameBufferVector fbs;

    if (fb->isYA2C2Planar())
    {
        const bool alpha = fb->hasAttribute("AlphaType") &&
                           fb->attribute<string>("AlphaType") != "None";


        FrameBuffer YA(fb->coordinateType(),
                       fb->width(),
                       fb->height(),
                       0,
                       2,
                       fb->dataType(),
                       (unsigned char*)fb->pixels<unsigned char>(),
                       &fb->channelNames(),
                       fb->orientation(),
                       false,
                       0);

        FrameBufferVector fbs0 = split(&YA);

        const FrameBuffer* fbc = fb->nextPlane();

        FrameBuffer C(fbc->coordinateType(),
                      fbc->width(),
                      fbc->height(),
                       0,
                       2,
                       fbc->dataType(),
                       (unsigned char*)fbc->pixels<unsigned char>(),
                       &fbc->channelNames(),
                       fbc->orientation(),
                       false,
                       0);

        FrameBufferVector fbs1 = split(&C);
        fbs0.push_back(fbs1[0]);
        fbs0.push_back(fbs1[1]);
        return fbs0;
    }
    else if (fb->isPlanar())
    {
        for (const FrameBuffer* f = fb->firstPlane(); f; f = f->nextPlane())
        {
            FrameBuffer* p = new FrameBuffer(f->coordinateType(),
                                             f->width(),
                                             f->height(),
                                             f->depth(),
                                             1,
                                             f->dataType(),
                                             0,
                                             &f->channelNames(),
                                             f->orientation(),
                                             true);

            copyPlane(f, p);
            fbs.push_back(p);
        }
    }
    else
    {
        for (int i=0; i < fb->numChannels(); i++)
        {
            FrameBuffer::StringVector channels;
            channels.push_back(fb->channelName(i));

            FrameBuffer* cfb = new FrameBuffer(fb->coordinateType(),
                                               fb->width(),
                                               fb->height(),
                                               fb->depth(),
                                               1,
                                               fb->dataType(),
                                               0,
                                               &channels,
                                               fb->orientation(),
                                               true);
            fbs.push_back(cfb);

            const unsigned char* src = fb->begin<unsigned char>()
                + fb->bytesPerChannel() * i;
            unsigned char* dst = cfb->begin<unsigned char>();
            const unsigned char* end = fb->begin<unsigned char>() + fb->dataSize();
            size_t srcSize = fb->pixelSize();
            size_t dstSize = cfb->pixelSize();

            for (;src < end; src += srcSize, dst += dstSize)
            {
                memcpy(dst, src, dstSize);
            }
        }
    }

    if (fb->uncrop())
    {
        for (size_t i = 0; i < fbs.size(); i++)
        {
            fbs[i]->setUncrop(fb);
        }
    }

    return fbs;
}


FrameBuffer*
merge(const FrameBufferVector& fbs)
{
    FrameBuffer* a = fbs.front();
    FrameBuffer::StringVector channels;

    for (int i=0; i < fbs.size(); i++)
    {
        assert(fbs[i]->numChannels() == 1);
        assert(fbs[i]->width() == fbs.front()->width());
        assert(fbs[i]->height() == fbs.front()->height());
        assert(fbs[i]->depth() == fbs.front()->depth());
        assert(fbs[i]->dataType() == fbs.front()->dataType());
        channels.push_back(fbs[i]->channelName(0));
    }

    FrameBuffer* fb = new FrameBuffer(a->coordinateType(),
                                      a->width(),
                                      a->height(),
                                      a->depth(),
                                      fbs.size(),
                                      a->dataType(),
                                      0, &channels,
                                      a->orientation(),
                                      true);

    for (int i=0; i < fb->numChannels(); i++)
    {
        FrameBuffer* cfb = fbs[i];
        unsigned char* dst = fb->begin<unsigned char>()
            + fb->bytesPerChannel() * i;
        const unsigned char* src = cfb->begin<unsigned char>();
        const unsigned char* end = fb->begin<unsigned char>() + fb->dataSize();
        size_t dstSize = fb->pixelSize();
        size_t srcSize = cfb->pixelSize();

        for (;dst < end; src += srcSize, dst += dstSize)
        {
            memcpy(dst, src, srcSize);
        }
    }

    fb->setUncrop(fbs.front());

    return fb;
}

FrameBuffer*
mergePlanes(const FrameBuffer* fb)
{
    FrameBuffer::StringVector channels;
    if (!fb->nextPlane()) return 0;

    for (const FrameBuffer* f = fb; f; f = f->nextPlane())
    {
        assert(f->numChannels() == 1);
        assert(f->dataType() == fb->dataType());
        channels.push_back(f->channelName(0));
    }

    int nchannels = channels.size();

    //
    // XXX The following FrameBuffer construction assumes the first plane has
    // the largest resolution of all of the planes
    //
    FrameBuffer* nfb = new FrameBuffer(fb->coordinateType(),
                                       fb->width(), fb->height(), fb->depth(),
                                       nchannels, fb->dataType(),
                                       0, &channels, fb->orientation(),
                                       true);

    nfb->setUncrop(fb);

    //
    //  Any relevant colormetric attributes should be on the "first"
    //  plane.
    //

    fb->copyAttributesTo(nfb);
    if (nfb->isRootPlane()) nfb->setIdentifier(fb->identifier());

    int channel = 0;

    for (const FrameBuffer* f = fb; f; f = f->nextPlane(), channel++)
    {
        size_t xdiv = nfb->width() / f->width();
        size_t ydiv = nfb->height() / f->height();

        for (size_t y = 0; y < nfb->height(); y++)
        {
            size_t yf = y / ydiv;
            if (yf >= f->height()) yf = f->height() - 1;

            switch (fb->dataType())
            {
              case FrameBuffer::UCHAR:
                  {
                      const unsigned char* pscanline = f->scanline<unsigned char>(yf);
                      unsigned char* scanline = nfb->scanline<unsigned char>(y);
                      for (size_t x = 0; x < nfb->width(); x++)
                      {
                          size_t xf = x / xdiv;
                          scanline[x*nchannels+channel] = pscanline[xf];
                      }
                  }
                  break;
              case FrameBuffer::USHORT:
                  {
                      const unsigned short* pscanline = f->scanline<unsigned short>(yf);
                      unsigned short* scanline = nfb->scanline<unsigned short>(y);
                      for (size_t x = 0; x < nfb->width(); x++)
                      {
                          size_t xf = x / xdiv;
                          scanline[x*nchannels+channel] = pscanline[xf];
                      }
                  }
                  break;
              case FrameBuffer::UINT:
                  {
                      const unsigned int* pscanline = f->scanline<unsigned int>(yf);
                      unsigned int* scanline = nfb->scanline<unsigned int>(y);
                      for (size_t x = 0; x < nfb->width(); x++)
                      {
                          size_t xf = x / xdiv;
                          scanline[x*nchannels+channel] = pscanline[xf];
                      }
                  }
                  break;
              case FrameBuffer::HALF:
                  {
                      const half* pscanline = f->scanline<half>(yf);
                      half* scanline = nfb->scanline<half>(y);
                      for (size_t x = 0; x < nfb->width(); x++)
                      {
                          size_t xf = x / xdiv;
                          scanline[x*nchannels+channel] = pscanline[xf];
                      }
                  }
                  break;
              case FrameBuffer::FLOAT:
                  {
                      const float* pscanline = f->scanline<float>(yf);
                      float* scanline = nfb->scanline<float>(y);
                      for (size_t x = 0; x < nfb->width(); x++)
                      {
                          size_t xf = x / xdiv;
                          scanline[x*nchannels+channel] = pscanline[xf];
                      }
                  }
                  break;
              case FrameBuffer::DOUBLE:
                  {
                      const double* pscanline = f->scanline<double>(yf);
                      double* scanline = nfb->scanline<double>(y);
                      for (size_t x = 0; x < nfb->width(); x++)
                      {
                          size_t xf = x / xdiv;
                          scanline[x*nchannels+channel] = pscanline[xf];
                      }
                  }
                  break;
              default:
                  abort();
            }

        }
    }

    return nfb;
}


//----------------------------------------------------------------------


//
//  Convert from c0 to c1
//

namespace {

Imath::M44f
primaryConvert(const Imf::Chromaticities& c0,
               const Imf::Chromaticities& c1,
               const Imath::V2f c0Neutral,
               const Imath::V2f c1Neutral,
               bool adapt)
{
    Imath::M44f A;

    // pinched from ImfAcesFile's ACES color conversion

    if (adapt)
    {
        static const Imath::M44f B
            ( 0.895100, -0.750200,  0.038900,  0.000000,
              0.266400,  1.713500, -0.068500,  0.000000,
              -0.161400,  0.036700,  1.029600,  0.000000,
              0.000000,  0.000000,  0.000000,  1.000000  );

        static const Imath::M44f BI
	    (0.986993,  0.432305, -0.008529,  0.000000,
             -0.147054,  0.518360,  0.040043,  0.000000,
	     0.159963,  0.049291,  0.968487,  0.000000,
	     0.000000,  0.000000,  0.000000,  1.000000);

        float ix = c0Neutral.x;
        float iy = c0Neutral.y;
        Imath::V3f inN(ix / iy, 1, (1 - ix - iy) / iy);

        float ox = c1Neutral.x;
        float oy = c1Neutral.y;
        Imath::V3f outN(ox / oy, 1, (1 - ox - oy) / oy);

        Imath::V3f ratio((outN * B) / (inN * B));

        Imath::M44f R(ratio[0], 0,        0,        0,
                      0,        ratio[1], 0,        0,
                      0,        0,        ratio[2], 0,
                      0,        0,        0,        1);

        A = B * R * BI;
    }

    Imath::M44f m0 = Imf::RGBtoXYZ(c0, 1.0);
    Imath::M44f m1 = Imf::XYZtoRGB(c1, 1.0);
    return m0 * A * m1;
}

}

void
colorSpaceConversionMatrix(const float* i,  // in chromaticies (8 floats)
                           const float* o,  // out chromaticies
                           const float* ni, // in neutral (2 floats)
                           const float* no, // out neutral
                           bool adapt,      // neutral adaptation
                           float* M)        // return 4x4 matrix
{
    if (!memcmp(i, o, sizeof(float)*8) &&
        (!adapt || !memcmp(ni, no, sizeof(float)*2)))
    {
        // We hv identical in's and out's for chromaticities and neutral
        // so return identity matrix without precision loss.
        // By doing so further on in the code logic
        // tests like C != M44f() will be false and this saves
        // rendering computation since there is no colorspace conversion to do.
        Imath::M44f C;
        memcpy(M, &C, sizeof(float)*16);
    }
    else
    {
        Imf::Chromaticities c0(Imath::V2f(i[0], i[1]),
                               Imath::V2f(i[2], i[3]),
                               Imath::V2f(i[4], i[5]),
                               Imath::V2f(i[6], i[7]));

        Imf::Chromaticities c1(Imath::V2f(o[0], o[1]),
                               Imath::V2f(o[2], o[3]),
                               Imath::V2f(o[4], o[5]),
                               Imath::V2f(o[6], o[7]));

        Imath::V2f n0(ni[0], ni[1]);
        Imath::V2f n1(no[0], no[1]);

        Imath::M44f C = primaryConvert(c0, c1, n0, n1, adapt);
        C.transpose();
        memcpy(M, &C, sizeof(float)*16);
    }
}

void
rec709Matrix(const FrameBuffer* fb, float M[16], bool adapt)
{
    Vec2f white, red, green, blue, neutral;
    bool rec709 = false;

    if (fb->hasPrimaries())
    {
        white = fb->attribute<Vec2f>(ColorSpace::WhitePrimary());
        red   = fb->attribute<Vec2f>(ColorSpace::RedPrimary());
        green = fb->attribute<Vec2f>(ColorSpace::GreenPrimary());
        blue  = fb->attribute<Vec2f>(ColorSpace::BluePrimary());
    }
    else
    {
        white = Vec2f(0.3127f, 0.3290f);
        red   = Vec2f(0.6400f, 0.3300f);
        green = Vec2f(0.3000f, 0.6000f);
        blue  = Vec2f(0.1500f, 0.0600f);
        rec709 = true;
    }

    neutral = white;

    if (fb->hasAdoptedNeutral())
    {
        neutral = fb->attribute<Vec2f>(ColorSpace::AdoptedNeutral());
    }


    Imf::Chromaticities chr(convert(red),
                            convert(green),
                            convert(blue),
                            convert(white));

    Imf::Chromaticities chr709;

    if (chr.red == chr709.red &&
        chr.green == chr709.green &&
        chr.blue == chr709.blue  &&
        chr.white == chr709.white)
    {
        Imath::M44f C;
        memcpy(M, &C, sizeof(float)*16);
        return;
    }

    Imath::M44f C = primaryConvert(chr, Imf::Chromaticities(),
                                   convert(neutral),
                                   Imf::Chromaticities().white,
                                   adapt);

    if (fb->primaryColorSpace() == ColorSpace::Rec601())
    {
        //
        // * 1.16438356164 - 0.0730593607303
        //
        C *= Imath::M44f(1.16438356164,    0, 0, 0,
                         0,                1, 0, 0,
                         0,                0, 1, 0,
                         -0.0730593607303, 0, 0, 1);
    }

    C.transpose();
    memcpy(M, &C, sizeof(float)*16);
}

void
acesMatrix(const FrameBuffer* fb, float M[16], bool adapt)
{
    Vec2f white, red, green, blue, neutral;

    if (fb->hasPrimaries())
    {
        white = fb->attribute<Vec2f>(ColorSpace::WhitePrimary());
        red   = fb->attribute<Vec2f>(ColorSpace::RedPrimary());
        green = fb->attribute<Vec2f>(ColorSpace::GreenPrimary());
        blue  = fb->attribute<Vec2f>(ColorSpace::BluePrimary());
    }
    else
    {
        white = Vec2f(0.3127f, 0.3290f);
        red   = Vec2f(0.6400f, 0.3300f);
        green = Vec2f(0.3000f, 0.6000f);
        blue  = Vec2f(0.1500f, 0.0600f);
    }

    neutral = white;

    if (fb->hasAdoptedNeutral())
    {
        neutral = fb->attribute<Vec2f>(ColorSpace::AdoptedNeutral());
    }

    Imf::Chromaticities chr(convert(red),
                            convert(green),
                            convert(blue),
                            convert(white));

    Imf::Chromaticities aces(Imath::V2f(0.73470,  0.26530),
                             Imath::V2f(0.00000,  1.00000),
                             Imath::V2f(0.00010, -0.07700),
                             Imath::V2f(0.32168,  0.33767));

    Imath::M44f C = primaryConvert(chr, aces,
                                   convert(neutral), aces.white,
                                   adapt);

    if (fb->primaryColorSpace() == ColorSpace::Rec601())
    {
        //
        // * 1.16438356164 - 0.0730593607303
        //
        C *= Imath::M44f(1.16438356164,    0, 0, 0,
                         0,                1, 0, 0,
                         0,                0, 1, 0,
                         -0.0730593607303, 0, 0, 1);
    }

    C.transpose();
    memcpy(M, &C, sizeof(float)*16);
}

void
yrybyYweights(const FrameBuffer* fb, float& rw, float& gw, float& bw)
{
    Vec2f white, red, green, blue;

    if (fb->hasAttribute(ColorSpace::WhitePrimary()))
    {
        white = fb->attribute<Vec2f>(ColorSpace::WhitePrimary());
        red   = fb->attribute<Vec2f>(ColorSpace::RedPrimary());
        green = fb->attribute<Vec2f>(ColorSpace::GreenPrimary());
        blue  = fb->attribute<Vec2f>(ColorSpace::BluePrimary());
    }
    else
    {
        white = Vec2f(0.3127f, 0.3290f);
        red   = Vec2f(0.6400f, 0.3300f);
        green = Vec2f(0.3000f, 0.6000f);
        blue  = Vec2f(0.1500f, 0.0600f);
    }

    Imf::Chromaticities chr(convert(red),
                            convert(green),
                            convert(blue),
                            convert(white));

    Imath::V3f w = Imf::RgbaYca::computeYw(chr);

    rw = w.x;
    gw = w.y;
    bw = w.z;
}

namespace {
void removeChromaticityAttrs(FrameBuffer* fb)
{
    static vector<string> attrs;
    if (attrs.empty())
    {
        attrs.push_back (ColorSpace::WhitePrimary());
        attrs.push_back (ColorSpace::RedPrimary());
        attrs.push_back (ColorSpace::GreenPrimary());
        attrs.push_back (ColorSpace::BluePrimary());
        attrs.push_back (ColorSpace::AdoptedNeutral());
    }

    for (int i=0; i < attrs.size(); i++)
    {
        if (const FBAttribute* a = fb->findAttribute(attrs[i]))
        {
            fb->deleteAttribute(a);
        }
    }
}

}

//
//  These YUV->RGB matrices are generated from a utility in our src tree
//  called yuvconvert.
//  Please note: If you change of these, please update their
//  use in the custom GLSL nodes (see additional_nodes)
//  accordingly.
//
void
getYUVtoRGBMatrix (Mat44f &M,
                   const string& fb_conversion,
                   const string& fb_range,
                   unsigned int bits)
{
    if (fb_conversion == ColorSpace::Rec709() &&
        (fb_range == "None" || fb_range == ColorSpace::VideoRange()) )
    {
        switch (bits)
        {
          default:
          case 8: M = Rec709VideoRangeYUVToRGB8<float>(); break;
          case 10: M = Rec709VideoRangeYUVToRGB10<float>(); break;
          case 16: M = Rec709VideoRangeYUVToRGB16<float>(); break;
        }
    }
    else if ((fb_conversion == ColorSpace::Rec601() &&
              (fb_range == "None" || fb_range == ColorSpace::VideoRange()) ) ||
             fb_conversion == "None")
    {
      switch (bits)
      {
        default:
        case 8: M = Rec601VideoRangeYUVToRGB8<float>(); break;
        case 10: M = Rec601VideoRangeYUVToRGB10<float>(); break;
        case 16: M = Rec601VideoRangeYUVToRGB16<float>(); break;
      }
    }
    else if (fb_conversion == ColorSpace::Rec601() &&
             fb_range == ColorSpace::FullRange())
    {
        switch (bits)
        {
          default:
          case 8: M = Rec601FullRangeYUVToRGB8<float>(); break;
          case 10: M = Rec601FullRangeYUVToRGB10<float>(); break;
          case 16: M = Rec601FullRangeYUVToRGB16<float>(); break;
        }
    }
    else if (fb_conversion == ColorSpace::Rec709() &&
             fb_range == ColorSpace::FullRange())
    {
        switch (bits)
        {
          default:
          case 8: M = Rec709FullRangeYUVToRGB8<float>(); break;
          case 10: M = Rec709FullRangeYUVToRGB10<float>(); break;
          case 16: M = Rec709FullRangeYUVToRGB16<float>(); break;
        }
    }
}

//
//  These RGB->YUV matrices are generated from a utility in our src tree
//  called yuvconvert.
//  Please note: If you change of these, please update their
//  use in the custom GLSL nodes (see additional_nodes)
//  accordingly.
//
void
getRGBtoYUVMatrix (Mat44f &M,
                   const string& fb_conversion,
                   const string& fb_range,
                   unsigned int bits)
{
    if (( fb_conversion == ColorSpace::Rec601() &&
          (fb_range == "None" || fb_range == ColorSpace::VideoRange()) ) ||
         fb_conversion == "None")
    {
        switch (bits)
        {
          default:
          case 8: M = Rec601VideoRangeRGBToYUV8<float>(); break;
          case 10: M = Rec601VideoRangeRGBToYUV10<float>(); break;
          case 16: M = Rec601VideoRangeRGBToYUV16<float>(); break;
        }
    }
    else if ( fb_conversion == ColorSpace::Rec709() &&
              (fb_range == "None" || fb_range == ColorSpace::VideoRange()) )
    {
        switch (bits)
        {
          default:
          case 8: M = Rec709VideoRangeRGBToYUV8<float>(); break;
          case 10: M = Rec709VideoRangeRGBToYUV10<float>(); break;
          case 16: M = Rec709VideoRangeRGBToYUV16<float>(); break;
        }
    }
    else if (fb_conversion == ColorSpace::Rec601() &&
             fb_range == ColorSpace::FullRange())
    {
        switch (bits)
        {
          default:
          case 8: M = Rec601FullRangeRGBToYUV8<float>(); break;
          case 10: M = Rec601FullRangeRGBToYUV10<float>(); break;
          case 16: M = Rec601FullRangeRGBToYUV16<float>(); break;
        }
    }
    else if (fb_conversion == ColorSpace::Rec709() &&
             fb_range == ColorSpace::FullRange())
    {
        switch (bits)
        {
          default:
          case 8: M = Rec709FullRangeRGBToYUV8<float>(); break;
          case 10: M = Rec709FullRangeRGBToYUV10<float>(); break;
          case 16: M = Rec709FullRangeRGBToYUV16<float>(); break;
        }
    }
}


TwkMath::Mat44f
YUVtoRGBMatrix (const FrameBuffer* fb)
{
    unsigned int bits = 8;

    switch (fb->dataType())
    {
      default:
          bits = 8;
          break;
      case FrameBuffer::PACKED_R10_G10_B10_X2:
      case FrameBuffer::PACKED_X2_B10_G10_R10:
          bits = 10;
          break;
      case FrameBuffer::USHORT:
      case FrameBuffer::HALF:
      case FrameBuffer::FLOAT:
      case FrameBuffer::UINT:
          bits = 16;
          break;
    }

    Mat44f m;
    getYUVtoRGBMatrix(m, fb->conversion(), fb->range(), bits);
    return m;
}

TwkMath::Mat44f
RGBtoYUVMatrix (const FrameBuffer* fb)
{
    unsigned int bits = 8;

    switch (fb->dataType())
    {
      default:
          bits = 8;
          break;
      case FrameBuffer::PACKED_R10_G10_B10_X2:
      case FrameBuffer::PACKED_X2_B10_G10_R10:
          bits = 10;
          break;
      case FrameBuffer::USHORT:
      case FrameBuffer::HALF:
      case FrameBuffer::FLOAT:
      case FrameBuffer::UINT:
          bits = 16;
          break;
    }

    string c = fb->conversion();
    string r = fb->range();

    Mat44f M;
    getRGBtoYUVMatrix(M, c, r, bits);
    return M;
}

void
convertYUVtoRGB(const FrameBuffer* a, FrameBuffer* b)
{
    //
    //  THIS IS ITU.BT-601/709 Y'CbCr  -> R'G'B'
    //
    Mat44f M;

    if (a->hasConversion())
    {
        getYUVtoRGBMatrix(M, a->conversion(), a->range(), 8);
    }

    if (a->primaryColorSpace() == ColorSpace::Rec601() ||
        a->primaryColorSpace() == ColorSpace::Generic())
    {
        getYUVtoRGBMatrix(M, ColorSpace::Rec601(), ColorSpace::FullRange(), 8);
    }
    else if (a->primaryColorSpace() == ColorSpace::Rec709())
    {
        getYUVtoRGBMatrix(M, ColorSpace::Rec709(), ColorSpace::FullRange(), 8);
    }

    applyTransform(a, b, linearColorTransform, &M);
    b->setChannelName(0, "R");
    b->setChannelName(1, "G");
    b->setChannelName(2, "B");
    a->copyAttributesTo(b);
    b->setPrimaryColorSpace(ColorSpace::Rec709());
    b->setTransferFunction(ColorSpace::Linear());
    removeChromaticityAttrs(b);
}

void
convertRGBtoYUV(const FrameBuffer* a, FrameBuffer* b)
{
    //
    //  THIS IS R'G'B' -> ITU.BT-601/709 Y'CbCr
    //

    Mat44f M;

    unsigned int bits = 8;
    if (b->dataType() == FrameBuffer::USHORT) bits=16;
    if (b->dataType() == FrameBuffer::PACKED_X2_B10_G10_R10 ||
        b->dataType() == FrameBuffer::PACKED_R10_G10_B10_X2) bits=10;

    if (b->primaryColorSpace() == ColorSpace::Rec601() ||
        b->primaryColorSpace() == ColorSpace::Generic())
    {
        getRGBtoYUVMatrix(M, ColorSpace::Rec601(), ColorSpace::FullRange(), bits);
    }
    else if (b->primaryColorSpace() == ColorSpace::Rec709())
    {
        getRGBtoYUVMatrix(M, ColorSpace::Rec709(), ColorSpace::FullRange(), bits);
    }

    assert(a->numChannels() == 3 || a->numChannels() == 4);
    assert(!a->isYUV() && !a->isYRYBY());
    applyTransform(a, b, linearColorTransform, &M);
    b->setChannelName(0, "Y");
    b->setChannelName(1, "U");
    b->setChannelName(2, "V");
    a->copyAttributesTo(b);
    removeChromaticityAttrs(b);
}

void
convertRGBtoYRYBY(const FrameBuffer* a, FrameBuffer* b)
{
    Vec3f Yw;
    assert(a->numChannels() == 3 || a->numChannels() == 4);
    assert(a->numChannels() == b->numChannels());
    yrybyYweights(a, Yw.x, Yw.y, Yw.z);

    if (b->dataType() == FrameBuffer::UCHAR ||
        b->dataType() == FrameBuffer::USHORT ||
        b->dataType() == FrameBuffer::UINT)
    {
        TWK_THROW_EXC_STREAM("convertRGBtoYRYBY requires floating point output type, got "
                             << b->dataType() << " instead");
    }

    applyTransform(a, b, rgb2yrybyColorTransform, &Yw);
    b->setChannelName(0, "Y");
    b->setChannelName(1, "RY");
    b->setChannelName(2, "BY");
    // leave chromaticities
    a->copyAttributesTo(b);
}

FrameBuffer*
convertToLinearRGB709(const FrameBuffer* fb)
{
    size_t nchannels = fb->numChannels();
    bool yuv    = fb->isYUV();
    bool yryby  = fb->isYRYBY();
    FrameBuffer* nfb = 0;

    if (fb->isYA2C2Planar())
    {
        FrameBufferVector fbs = split(fb);
        nfb = merge(fbs);
        for (size_t i =0; i < fbs.size(); i++) delete fbs[i];

        Vec3f Yw;
        float M[16];
        yrybyYweights(fb, Yw.x, Yw.y, Yw.z);
        applyTransform(nfb, nfb, yryby2rgbColorTransform, &Yw);

        if (fb->hasPrimaries())
        {
            rec709Matrix(fb, M);
            applyTransform(nfb, nfb, linearColorTransform, M);
            removeChromaticityAttrs(nfb);
        }

        nfb->setChannelName(0, "R");
        nfb->setChannelName(1, "G");
        nfb->setChannelName(2, "B");
    }

    if (fb->dataType() >= FrameBuffer::PACKED_R10_G10_B10_X2)
    {
        nfb = copyConvert(fb, FrameBuffer::USHORT);
    }
    else if (yuv || yryby)
    {
        nfb = fb->copy();

        if (yuv)
        {
            convertYUVtoRGB(nfb, nfb);
        }
        else
        {
            Vec3f Yw;
            float M[16];
            yrybyYweights(fb, Yw.x, Yw.y, Yw.z);
            applyTransform(nfb, nfb, yryby2rgbColorTransform, &Yw);

            if (fb->hasPrimaries())
            {
                rec709Matrix(fb, M);
                applyTransform(nfb, nfb, linearColorTransform, M);
                removeChromaticityAttrs(nfb);
            }

            nfb->setChannelName(0, "R");
            nfb->setChannelName(1, "G");
            nfb->setChannelName(2, "B");
        }
    }
    else if (fb->isRGBPlanar())
    {
        nfb = mergePlanes(fb);
        float M[16];
        rec709Matrix(fb, M);
        applyTransform(nfb, nfb, linearColorTransform, M);
        removeChromaticityAttrs(nfb);
    }
    else if (fb->isYUVPlanar())
    {
        nfb = mergePlanes(fb);
        convertYUVtoRGB(nfb, nfb);
    }
    else
    {
        float M[16];
        rec709Matrix(fb, M);
        nfb = fb->copy();
        applyTransform(nfb, nfb, linearColorTransform, M);
        removeChromaticityAttrs(nfb);
    }

    if (fb->hasAttribute("PixelAspectRatio"))
    {
        nfb->setPixelAspectRatio(fb->pixelAspectRatio());
    }

    return nfb;
}

void
linearRGBA709pixelValue(const FrameBuffer* fb, int x, int y, float *p)
{
    p[3] = 1.0;

    bool YRYBYp = fb->isYRYBYPlanar();
    bool YUVp = fb->isYUVPlanar();
    bool RGBp = fb->isRGBPlanar();

    if (YRYBYp || YUVp || RGBp)
    {
        int i = 0;
        for (const FrameBuffer* f = fb->firstPlane(); f; f = f->nextPlane())
        {
            const float xm = float(f->width()) / float(fb->width());
            const float ym = float(f->height()) / float(fb->height());

            float xi = xm * float(x);
            float yi = ym * float(y);

            if (xi >= f->width())  xi = f->width() - 1;
            if (yi >= f->height()) yi = f->height() - 1;

            float t[4];
            f->getPixel4f(int(xi), int(yi), t);
            p[i++] = t[0];
        }
    }
    else
    {
        fb->getPixel4f(x, y, p);
    }

    float &r = p[0];
    float &g = p[1];
    float &b = p[2];

    if (fb->isYUV() || YUVp ||
        fb->dataType() == FrameBuffer::PACKED_Cb8_Y8_Cr8_Y8 ||
        fb->dataType() == FrameBuffer::PACKED_Y8_Cb8_Y8_Cr8)
    {
        const float r0 = r;
        const float g0 = g;
        const float b0 = b;

        Mat44f M;

        getYUVtoRGBMatrix(M, fb->conversion(), fb->range(), 8);

        r = r0 * M.m00 +  g0 * M.m01 + M.m02 * b0 + M.m03;
        g = r0 * M.m10 +  g0 * M.m11 + M.m12 * b0 + M.m13;
        b = r0 * M.m20 +  g0 * M.m21 + M.m22 * b0 + M.m23;
    }
    else if (fb->isYRYBY() || YRYBYp)
    {
        const float r0 = r;
        const float g0 = g;
        const float b0 = b;

        Vec3f Yw;
        yrybyYweights(fb, Yw.x, Yw.y, Yw.z);

        const float Y = r0;
        r = (g0 + 1.0f) * Y;
        b = (b0 + 1.0f) * Y;
        g = (Y - r * Yw.x - b * Yw.z) / Yw.y;
    }

    if (fb->hasPrimaries())
    {
        Mat44f M;
        rec709Matrix(fb, (float*)&M);
        Vec3f c(r,g,b);
        c = M * c;
        r = c.x;
        g = c.y;
        b = c.z;
    }
}


void
getLogCCurveParams(LogCTransformParams& params,
                   const FrameBuffer* fb,
                   float ei)
{
    if (fb && fb->hasLogCParameters() && ei != 0.0f)
    {
        params.LogCBlackSignal     = fb->attribute<float>(TwkFB::ColorSpace::LogCBlackSignal());
        params.LogCEncodingOffset  = fb->attribute<float>(TwkFB::ColorSpace::LogCEncodingOffset());
        params.LogCEncodingGain    = fb->attribute<float>(TwkFB::ColorSpace::LogCEncodingGain());
        params.LogCGraySignal      = fb->attribute<float>(TwkFB::ColorSpace::LogCGraySignal());
        params.LogCBlackOffset     = fb->attribute<float>(TwkFB::ColorSpace::LogCBlackOffset());
        params.LogCLinearSlope     = fb->attribute<float>(TwkFB::ColorSpace::LogCLinearSlope());
        params.LogCLinearOffset    = fb->attribute<float>(TwkFB::ColorSpace::LogCLinearOffset());
        params.LogCLinearCutPoint  = fb->attribute<float>(TwkFB::ColorSpace::LogCLinearCutPoint());
        params.LogCCutPoint        = fb->attribute<float>(TwkFB::ColorSpace::LogCCutPoint());
    }
    else
    {
        //
        //  NOTE:
        //  To reproduce the results ARRI gets you need to set pbs to 0 and gs to .18
        //  This makes black 0 and grey .18 on output.
        //
        if (ei == 0.0f) ei = 800.0f;

        RVMath::LogC logC(RVMath::ALEXA_LOGC_PARAMS, ei); 

        params.LogCBlackSignal = 0.0f;
        params.LogCEncodingOffset = logC.getEncodingOffset();
        params.LogCEncodingGain = logC.getEncodingGain();
        params.LogCGraySignal = .18f;
        params.LogCBlackOffset = logC.getBlackOffset();
        params.LogCLinearSlope = logC.getLinearSlope();
        params.LogCLinearOffset = logC.getLinearOffset();
        params.LogCLinearCutPoint = 
            (RVMath::ALEXA_LOGC_PARAMS.cutPoint * params.LogCLinearSlope + params.LogCLinearOffset) *
            params.LogCEncodingGain + params.LogCEncodingOffset;

        params.LogCCutPoint = RVMath::ALEXA_LOGC_PARAMS.cutPoint;
    }
}

namespace {
bool*
RGBYChannelMask(const FrameBuffer* a)
{
    bool* channelMask = new bool[a->numChannels()];

    for (int i=0; i < a->numChannels(); i++)
    {
        const string& name = a->channelName(i);

        channelMask[i] = name == "R" ||
                         name == "G" ||
                         name == "B" ||
                         name == "Y";
    }

    return channelMask;
}


bool*
RGBChannelMask(const FrameBuffer* a)
{
    bool* channelMask = new bool[a->numChannels()];

    for (int i=0; i < a->numChannels(); i++)
    {
        const string& name = a->channelName(i);

        channelMask[i] = name == "R" ||
                         name == "G" ||
                         name == "B";
    }

    return channelMask;
}

void
convertWithChannelMask(const FrameBuffer* a,
                       FrameBuffer* b,
                       ColorTransformFunc F)
{
    bool* channelMask = RGBYChannelMask(a);
    applyTransform(a, b, F, channelMask);
    delete[] channelMask;
}

void
convertWithLogCTransformParams(const FrameBuffer* a,
                               FrameBuffer* b,
                               float ei,
                               ColorTransformFunc F)
{
    bool* channelMask = RGBYChannelMask(a);

    LogCTransformParams params;
    params.chmap = channelMask;
    getLogCCurveParams(params, a, ei);

    applyTransform(a, b, F, &params);

    // If ei != 0 then we need to set the new LogC
    // params on the target fb.
    if (ei != 0.0f)
    {
        b->newAttribute<float>(ColorSpace::LogCCutPoint(), params.LogCCutPoint);
        b->newAttribute<float>(ColorSpace::LogCLinearCutPoint(), params.LogCLinearCutPoint);
        b->newAttribute<float>(ColorSpace::LogCLinearOffset(), params.LogCLinearOffset);
        b->newAttribute<float>(ColorSpace::LogCLinearSlope(), params.LogCLinearSlope);
        b->newAttribute<float>(ColorSpace::LogCBlackOffset(), params.LogCBlackOffset);
        b->newAttribute<float>(ColorSpace::LogCGraySignal(), params.LogCGraySignal);
        b->newAttribute<float>(ColorSpace::LogCEncodingGain(), params.LogCEncodingGain);
        b->newAttribute<float>(ColorSpace::LogCEncodingOffset(), params.LogCEncodingOffset);
        b->newAttribute<float>(ColorSpace::LogCBlackSignal(), params.LogCBlackSignal);
    }

    delete[] channelMask;
}

}

void
convertLogToLinear(const FrameBuffer* a, FrameBuffer* b)
{
    convertWithChannelMask(a, b, logLinearTransform);
}

void
convertLinearToLog(const FrameBuffer* a, FrameBuffer* b)
{
    convertWithChannelMask(a, b, linearLogTransform);
}

void
convertRedLogToLinear(const FrameBuffer* a, FrameBuffer* b)
{
    convertWithChannelMask(a, b, redLogLinearTransform);
}

void
convertLinearToRedLog(const FrameBuffer* a, FrameBuffer* b)
{
    convertWithChannelMask(a, b, linearRedLogTransform);
}

void
convertSRGBToLinear(const FrameBuffer* a, FrameBuffer* b)
{
    convertWithChannelMask(a, b, sRGBtoLinearTransform);
}

void
convertLinearToSRGB(const FrameBuffer* a, FrameBuffer* b)
{
    convertWithChannelMask(a, b, linearToSRGBTransform);
}

void
convertLinearToRec709(const FrameBuffer* a, FrameBuffer* b)
{
    convertWithChannelMask(a, b, linearToRec709Transform);
}


void
convertLogCToLinear(const FrameBuffer* a, FrameBuffer* b, float ei)
{
    convertWithLogCTransformParams(a, b, ei, logCToLinearTransform);
}

void
convertLinearToLogC(const FrameBuffer* a, FrameBuffer* b, float ei)
{
    convertWithLogCTransformParams(a, b, ei, linearToLogCTransform);
}

void
premult(const FrameBuffer* a, FrameBuffer* b)
{
    convertWithChannelMask(a, b, premultTransform);
}

void
unpremult(const FrameBuffer* a, FrameBuffer* b)
{
    convertWithChannelMask(a, b, unpremultTransform);
}

void
orientationMatrix(const FrameBuffer* fb, bool normalized, float* m)
{
    Mat44f& M = *(Mat44f*)m;
    const float w = normalized ? 1 : fb->width();
    const float h = normalized ? 1 : fb->height();

    switch (fb->orientation())
    {
      default:
          M = Mat44f();
          break;
      case FrameBuffer::TOPLEFT:
          M = Mat44f(1,  0, 0, 0,
                     0, -1, 0, h,
                     0,  0, 1, 0,
                     0,  0, 0, 1);
          break;
      case FrameBuffer::TOPRIGHT:
          M = Mat44f(-1,  0, 0, w,
                      0, -1, 0, h,
                      0,  0, 1, 0,
                      0,  0, 0, 1);
          break;
      case FrameBuffer::BOTTOMRIGHT:
          M = Mat44f(-1, 0, 0, w,
                      0, 1, 0, 0,
                      0, 0, 1, 0,
                      0, 0, 0, 1);
          break;
    }
}

FrameBuffer*
cropPlane(const FrameBuffer* fb, int x0, int y0, int x1, int y1)
{
    if (x1 < x0 || y1 < y0 ||
        x0 < 0 || x0 >= fb->width() ||
        x1 < 0 || x1 >= fb->width() ||
        y0 < 0 || y0 >= fb->height() ||
        y1 < 0 || y1 >= fb->height() )
    {
        TWK_THROW_EXC_STREAM("bad parameter(s) to crop "
                             << x0 << " " << y0 << " "
                             << x1 << " " << y1
                             << ", fb " << fb->width()
                             << " " << fb->height());
    }

    FrameBuffer* crop = new FrameBuffer(fb->coordinateType(),
                                        x1 - x0 + 1,
                                        y1 - y0 + 1,
                                        fb->depth(),
                                        fb->numChannels(),
                                        fb->dataType(),
                                        0,
                                        &fb->channelNames(),
                                        fb->orientation(),
                                        true);

    for (int row = y0; row <= y1; row++)
    {
        const unsigned char* src = fb->scanline<unsigned char>(row) + x0 * fb->pixelSize();
        unsigned char* dst = crop->scanline<unsigned char>(row - y0);
        memcpy(dst, src, crop->scanlineSize());
    }

    return crop;
}

FrameBuffer*
crop(const FrameBuffer* fb, int x0, int y0, int x1, int y1)
{
    FrameBuffer* nfb = 0;

    //
    //  Check requested crop numbers now, so that error message is in space
    //  that user is using.
    //

    string errStr = "";

         if (x1 < x0)                      errStr = "x1 < x0";
    else if (y1 < y0)                      errStr = "y1 < y0";
    else if (x0 < 0 || x0 >= fb->width())  errStr = "x0 out-of-bounds";
    else if (x1 < 0 || x1 >= fb->width())  errStr = "x1 out-of-bounds";
    else if (y0 < 0 || y0 >= fb->height()) errStr = "y0 out-of-bounds";
    else if (y1 < 0 || y1 >= fb->height()) errStr = "y1 out-of-bounds";

    if (!errStr.empty())
    {
        TWK_THROW_EXC_STREAM("bad parameter(s) to crop "
                             << x0 << " " << y0 << " "
                             << x1 << " " << y1
                             << ", fb size " << fb->width()
                             << " " << fb->height() <<
                             " (" << errStr << ")");
    }

    //
    //  Crop numbers are always considered to be consistant with
    //  a FrameBuffer::NATURAL orientation, but the actual crop
    //  operations expect numbers in the native orientation of this
    //  framebuffer, so we convert from one to the other here.
    //

    if (fb->orientation() == FrameBuffer::TOPRIGHT ||
        fb->orientation() == FrameBuffer::BOTTOMRIGHT)
    {
        int x0orig = x0;
        x0 = fb->width() -1 - x1;
        x1 = fb->width() -1 - x0orig;
    }

    if (fb->orientation() == FrameBuffer::TOPLEFT ||
        fb->orientation() == FrameBuffer::TOPRIGHT)
    {
        int y0orig = y0;
        y0 = fb->height() -1 - y1;
        y1 = fb->height() -1 - y0orig;
    }

    if (fb->dataType() >= FrameBuffer::PACKED_R10_G10_B10_X2)
    {
        FrameBuffer* tfb = 0;
        float nx0 = x0 / 2 * 2;
        float nx1 = x1 / 2 * 2 + 1;
        if (nx1 >= fb->width()) nx1 = fb->width() - 1;
        tfb = cropPlane(fb, nx0, y0, nx1, y1);
        y0 = 0;
        y1 = tfb->height()-1;
	//
	//  Start in one pixel if nx0 is one less
	//
        x0 = x0 - nx0;
	//
	//  Come back from end one extra pixel if nx1 is one more
	//
        x1 = tfb->width() - 1 - (nx1 - x1);
	//
	//  Come back from end one extra pixel if nx1 is one more
	//
        nfb = convertToLinearRGB709(tfb);
        delete tfb;
        fb->copyAttributesTo(nfb);
        if (nfb->isRootPlane()) nfb->setIdentifier(fb->identifier());
        fb = nfb;
    }

    FrameBuffer* cfb = cropPlane(fb, x0, y0, x1, y1);

    const double w   = double(fb->width());
    const double h   = double(fb->height());
    const double fx0 = double(x0) / w;
    const double fx1 = double(x1) / w;
    const double fy0 = double(y0) / h;
    const double fy1 = double(y1) / h;

    for (const FrameBuffer* f = fb->nextPlane(); f; f = f->nextPlane())
    {
        const double nw  = double(f->width());
        const double nh  = double(f->height());
        const int    nx0 = int(fx0 * nw);
        const int    nx1 = int(fx1 * nw);
        const int    ny0 = int(fy0 * nh);
        const int    ny1 = int(fy1 * nh);

        cfb->appendPlane(cropPlane(f, nx0, ny0, nx1, ny1));
    }

    delete nfb;
    return cfb;
}

FrameBuffer*
cropWithUncrop(const FrameBuffer* fb, int x0, int y0, int x1, int y1)
{
    FrameBuffer* cfb = crop(fb, x0, y0, x1, y1);
    cfb->setUncrop(fb->width(), fb->height(), x0, y0);
    return cfb;
}

void
cropInto(const FrameBuffer* infb,
         FrameBuffer* outfb,
         int x0, int y0,
         int x1, int y1)
{
    const double w     = double(infb->width());
    const double h     = double(infb->height());
    //
    //  Note that x* y* are _inclusive_ So we want e.g. fy1 to be 1.0 when
    //  y1==(h-1).
    //
    const double fx0   = double(x0) / (w - 1);
    const double fx1   = double(x1) / (w - 1);
    const double fy0   = double(y0) / (h - 1);
    const double fy1   = double(y1) / (h - 1);
    const double cropW = x1 - x0 + 1;
    const double cropH = y1 - y0 + 1;

    const FrameBuffer* in = infb;
    FrameBuffer* out = outfb;

    for (;in; in = in->nextPlane(), out = out->nextPlane())
    {
        bool append = false;
        if (!out) { out = new FrameBuffer(); append = true; }

        const double xsampling = double(in->width()) / w;
        const double ysampling = double(in->height()) / h;
        const int    nw        = int(cropW * xsampling + 0.5);
        const int    nh        = int(cropH * ysampling + 0.5);

        out->restructure(nw,
                         nh,
                         in->depth(),
                         in->numChannels(),
                         in->dataType(),
                         NULL,
                         &in->channelNames(),
                         in->orientation());

        for (int row = int(fy0 * ysampling * h + 0.499), nrow = 0;
             row < int(fy1 * ysampling * h + 0.499) && nrow < out->height();
             row++, nrow++)
        {
            const unsigned char* src = in->scanline<unsigned char>(row) + size_t(fx0 * xsampling * in->width()) * in->pixelSize();
            unsigned char* dst = out->scanline<unsigned char>(nrow);
            memcpy(dst, src, out->scanlineSize());
        }

        if (append) outfb->appendPlane(out);
    }
}

} // TwkFB
