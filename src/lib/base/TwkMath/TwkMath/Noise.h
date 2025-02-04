//******************************************************************************
// Copyright (c) 2001-2002 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkMathNoise_h_
#define _TwkMathNoise_h_

#include <TwkMath/Vec2.h>
#include <TwkMath/Vec3.h>
#include <TwkMath/Vec4.h>

namespace TwkMath
{

    //*****************************************************************************
    // These noise functions return
    // perlin noise centered around zero
    // The range never quite reaches -1, 1
    //*****************************************************************************

    float noise(float v);
    float noise(const Vec2f& v);
    float noise(const Vec3f& v);
    float noise(float x, float y, float z, float w);

    inline float noise(const Vec4f& v) { return noise(v.x, v.y, v.z, v.w); }

    inline float noise(const Vec3f& v, float e)
    {
        return noise(v.x, v.y, v.z, e);
    }

    // With gradients
    float noiseAndGrad(float v, float& grad);
    float noiseAndGrad(const Vec2f& v, Vec2f& grad);
    float noiseAndGrad(const Vec3f& v, Vec3f& grad);

} // End namespace TwkMath

#endif
