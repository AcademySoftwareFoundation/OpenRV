//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#ifndef __base__RVMath__AlexaLogC
#define __base__RVMath__AlexaLogC

#include "RVMath/LogC.h"

namespace RVMath
{

    static constexpr Param ALEXA_LOGC_PARAMS{
        1.0f / 9.0f,              // cutPoint
        400.0f,                   // nominalSpeed
        0.01f,                    // midGreySignal
        16.0f / 4095.0f,          // blackSignal
        95.0f / 1023.0f,          // encodingBlack
        400.0f / 1023.0f,         // encodingOffset
        500.0f / 1023.0f * 0.525f // encodingGain
    };

}

#endif
