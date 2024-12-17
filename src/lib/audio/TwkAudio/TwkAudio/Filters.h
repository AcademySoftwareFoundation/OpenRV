//
//  Copyright (c) 2008 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __TwkAudio__Filters__h__
#define __TwkAudio__Filters__h__
#include <iostream>
#include <TwkAudio/Audio.h>
#include <TwkAudio/dll_defs.h>

namespace TwkAudio
{

    TWKAUDIO_EXPORT void lowPassFilter(AudioBuffer& in, AudioBuffer& prev,
                                       AudioBuffer& out, float alpha,
                                       bool isBackwards);

} // namespace TwkAudio

#endif // __TwkAudio__Filters__h__
