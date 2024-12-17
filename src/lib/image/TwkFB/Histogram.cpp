//******************************************************************************
// Copyright (c) 2008 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkFB/Histogram.h>
#include <limits>
#include <half.h>
#include <halfLimits.h>
#include <stdlib.h>

namespace TwkFB
{
    using namespace std;
    using namespace TwkFB;

    template <typename T>
    void floatScanlineHistogram(const T* scanline, size_t len, size_t nchannels,
                                FBHistorgram::iterator hbegin,
                                FBHistorgram::iterator hend, float minRange,
                                float maxRange)
    {
        size_t hsize = hend - hbegin;
        const T* e = scanline + (len * hsize);
        const float range = maxRange - minRange;
        size_t index = 0;

        for (FBHistorgram::iterator i = hbegin; i != hend; ++i, index++)
        {
            ChannelHistogram& h = *i;

            const float mult = float(h.accum.size() - 1);
            size_t* a = &h.accum.front();

            for (const T* p = scanline + index; p < e; p += nchannels)
            {
                const T v = *p;
                const T vs = T((v - minRange) / maxRange);
                const size_t bin = min(size_t(vs * mult), h.accum.size() - 1);
                assert(bin < h.accum.size());
                a[bin]++;

                if (h.max < v)
                    h.max = v;
                if (h.min > v)
                    h.min = v;
            }
        }
    }

    template <typename T>
    void integralScanlineHistogram(const T* scanline, size_t len,
                                   size_t nchannels,
                                   FBHistorgram::iterator hbegin,
                                   FBHistorgram::iterator hend)
    {
        size_t hsize = hend - hbegin;
        const T* e = scanline + (len * hsize);
        const double Tmax = double(numeric_limits<T>::max());
        size_t index = 0;

        for (FBHistorgram::iterator i = hbegin; i != hend; ++i, index++)
        {
            ChannelHistogram& h = *i;

            const size_t bins = h.accum.size();
            const double divisor =
                double(numeric_limits<T>::max()) / double(bins - 1);
            size_t* a = &h.accum.front();

            for (const T* p = scanline + index; p < e; p += nchannels)
            {
                const T v = *p;
                const size_t bin = size_t(double(v) / divisor);
                assert(bin < h.accum.size());
                a[bin]++;

                const float fv = float(double(v) / Tmax);
                if (h.max < fv)
                    h.max = fv;
                if (h.min > fv)
                    h.min = fv;
            }
        }
    }

    template <typename T>
    MinMaxPair histogram(const FrameBuffer* fb, FBHistorgram::iterator& hbegin,
                         FBHistorgram::iterator& hend, float minRange = 0.0,
                         float maxRange = 1.0)
    {
        for (size_t i = 0; i < fb->height(); i++)
        {
            const T* scanline = fb->scanline<T>(i);

            if (numeric_limits<T>::is_integer)
            {
                integralScanlineHistogram(scanline, fb->width(),
                                          fb->numChannels(), hbegin, hend);
            }
            else
            {
                floatScanlineHistogram(scanline, fb->width(), fb->numChannels(),
                                       hbegin, hend, minRange, maxRange);
            }
        }

        MinMaxPair minmax;

        for (FBHistorgram::iterator i = hbegin; i != hend; ++i)
        {
            ChannelHistogram& ch = *i;

            size_t total = 0;

            for (size_t q = 0; q < ch.accum.size(); q++)
            {
                total += ch.accum[q];
            }

            for (size_t q = 0; q < ch.accum.size(); q++)
            {
                ch.histogram[q] = double(ch.accum[q]) / double(total);
            }

            if (i == hbegin)
            {
                minmax = make_pair(ch.min, ch.max);
            }
            else
            {
                minmax.first = min(ch.min, minmax.first);
                minmax.second = max(ch.max, minmax.second);
            }
        }

        return minmax;
    }

    MinMaxPair computeChannelHistogram(const FrameBuffer* fb, FBHistorgram& h,
                                       size_t bins, bool fullRangeOverOne)
    {
        if (fb->isPlanar())
        {
            h.resize(fb->numPlanes());
        }
        else
        {
            h.resize(fb->numChannels());
        }

        for (size_t i = 0; i < h.size(); i++)
        {
            ChannelHistogram& ch = h[i];
            ch.histogram.resize(bins);
            ch.accum.resize(bins);
            ch.min = numeric_limits<float>::max();
            ch.max = numeric_limits<float>::min();
            memset(&ch.accum.front(), 0, sizeof(size_t) * ch.accum.size());
        }

        FBHistorgram::iterator hi = h.begin();
        MinMaxPair minmax = make_pair(numeric_limits<float>::max(),
                                      numeric_limits<float>::min());

        size_t index = 0;

        for (const FrameBuffer* f = fb; f;
             hi += f->numChannels(), f = f->nextPlane())
        {
            FBHistorgram::iterator he = hi + f->numChannels();
            MinMaxPair p;

            for (size_t ch = 0; ch < f->numChannels(); ch++)
            {
                FBHistorgram::iterator hh = hi + ch;
                hh->channelName = f->channelName(ch);
            }

            switch (fb->dataType())
            {
            case FrameBuffer::UCHAR:
                p = histogram<unsigned char>(f, hi, he);
                break;

            case FrameBuffer::USHORT:
                p = histogram<unsigned short>(f, hi, he);
                break;

            case FrameBuffer::HALF:
                p = histogram<half>(f, hi, he);
                break;

            case FrameBuffer::FLOAT:
                p = histogram<float>(f, hi, he);
                break;

            default:
                abort();
            }

            minmax.first = min(minmax.first, p.first);
            minmax.second = max(minmax.second, p.second);
        }

        if (fullRangeOverOne)
        {
            FBHistorgram::iterator hi = h.begin();
            MinMaxPair p;

            for (const FrameBuffer* f = fb; f;
                 hi += f->numChannels(), f = f->nextPlane())
            {
                FBHistorgram::iterator he = hi + f->numChannels();

                switch (fb->dataType())
                {
                case FrameBuffer::HALF:
                    p = histogram<half>(f, hi, he, min(minmax.first, 0.f),
                                        minmax.second);
                    break;

                case FrameBuffer::FLOAT:
                    p = histogram<float>(f, hi, he, min(minmax.first, 0.f),
                                         minmax.second);
                    break;

                default:
                    break;
                }
            }
        }

        return minmax;
    }

} // namespace TwkFB
