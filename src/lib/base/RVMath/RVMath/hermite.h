//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#ifndef __base__RVMath__HERMITE
#define __base__RVMath__HERMITE

#include <cmath>

namespace RVMath
{
    void hermite(float x, float x1, float x2, float* w)
    {
        const float d = x2 - x1;
        const float s = (x - x1) / d;
        const float s2 = 1 - s;

        const float powSInt = pow(s2, 2.0f);
        const float powS = pow(s, 2.0f);

        w[0] = (1.0f + 2.0f * s) * powSInt;
        w[1] = (3.0f - 2.0f * s) * powS;
        w[2] = d * s * powSInt;
        w[3] = -d * s2 * powS;
    }
} // namespace RVMath

#endif
