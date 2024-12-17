//******************************************************************************
// Copyright (c) 2008 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkAudio__Mix__h__
#define __TwkAudio__Mix__h__
#include <TwkAudio/Audio.h>
#include <TwkMath/Mat22.h>
#include <TwkAudio/dll_defs.h>

namespace TwkAudio
{

    /// Mix input buffer channels into output buffer channels

    ///
    /// Takes any channel count input buffer, any channel count output
    /// buffer, the volume of the left channel balance, the volume of the
    /// right channel balance, and a bool which determines wether to mix
    /// with the existing output samples.
    ///
    /// in and out sample sizes must match.
    ///

    TWKAUDIO_EXPORT void mixChannels(const AudioBuffer& in, AudioBuffer& out,
                                     const float lVolume, const float rVolume,
                                     const bool compose);

} // namespace TwkAudio

#endif // __TwkAudio__Mix__h__
