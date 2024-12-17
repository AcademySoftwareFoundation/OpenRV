//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkAudio__Interlace__h__
#define __TwkAudio__Interlace__h__
#include <vector>
#include <TwkAudio/Audio.h>
#include <TwkAudio/dll_defs.h>

namespace TwkAudio
{

    TWKAUDIO_EXPORT void deinterlace(const SampleVector& inbuffer,
                                     int nchannels,
                                     std::vector<SampleVector>& outbuffers);

    TWKAUDIO_EXPORT void deinterlace(const float* inbuffer, size_t nsamples,
                                     int nchannels,
                                     std::vector<SampleVector>& outbuffers);

    TWKAUDIO_EXPORT void interlace(const std::vector<SampleVector>& inbuffers,
                                   SampleVector& outbuffer);

    TWKAUDIO_EXPORT void interlace(const std::vector<SampleVector>& inbuffers,
                                   float* out, size_t start = 0,
                                   size_t maxSize = 0);

} // namespace TwkAudio

#endif // __TwkAudio__Interlace__h__
