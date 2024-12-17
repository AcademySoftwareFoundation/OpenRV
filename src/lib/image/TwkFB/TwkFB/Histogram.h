//******************************************************************************
// Copyright (c) 2008 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkFB__Histogram__h__
#define __TwkFB__Histogram__h__
#include <TwkFB/dll_defs.h>
#include <TwkFB/FrameBuffer.h>

namespace TwkFB
{

    struct TWKFB_EXPORT ChannelHistogram
    {
        size_t index;
        float min;
        float max;

        std::string channelName;
        std::vector<float> histogram;
        std::vector<size_t> accum;
    };

    typedef std::vector<ChannelHistogram> FBHistorgram;

    /// Compute a per-channel histogram of the FB

    ///
    /// computeChannelHistogram computes a per-channel histogram of the
    /// FB. The returned pair is the overall min, max values. If
    /// fullRangeOverOne is true and the fb has float data, the histogram
    /// will be transformed to hold all of the values in the fb. If the fb
    /// has negative values the min bin will represent the lowest value,
    /// otherwise, the lowest bin will be 0.
    ///

    typedef std::pair<float, float> MinMaxPair;

    TWKFB_EXPORT MinMaxPair
    computeChannelHistogram(const FrameBuffer* fb, FBHistorgram& output,
                            size_t bins = 100, bool fullRangeOverOne = false);

} // namespace TwkFB

#endif // __TwkFB__Histogram__h__
