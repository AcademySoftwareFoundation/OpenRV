//
//  Copyright (c) 2008 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __TwkAudio__ScaleTime__h__
#define __TwkAudio__ScaleTime__h__
#include <iostream>
#include <TwkAudio/Audio.h>
#include <TwkAudio/dll_defs.h>

namespace TwkAudio
{

    TWKAUDIO_EXPORT void scaleTime(const AudioBuffer& in, AudioBuffer& out);

} // namespace TwkAudio

#endif // __TwkAudio__ScaleTime__h__
