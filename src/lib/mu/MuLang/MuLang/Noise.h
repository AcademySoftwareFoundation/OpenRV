#ifndef __MuLang__Noise__h__
#define __MuLang__Noise__h__

//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

namespace Mu
{

    float noise1(float x);
    float noiseAndGrad1(float x, float& gradX);
    float noise2(const float v[2]);
    float noiseAndGrad2(const float v[2], float grad[2]);
    float noise3(const float v[3]);
    float noiseAndGrad3(const float v[3], float grad[3]);

} // namespace Mu

#endif // __MuLang__Noise__h__
