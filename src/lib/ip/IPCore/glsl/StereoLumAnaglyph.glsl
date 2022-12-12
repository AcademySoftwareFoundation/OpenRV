//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
// left eye, red
// right eye, cyan

vec4 main (const in inputImage in0,
           const in inputImage in1,
           const in vec3 lumaCoefficients)
{
    vec4 P0 = in0();
    vec4 P1 = in1();

    float Y0 = lumaCoefficients.r * P0.r + lumaCoefficients.g * P0.g + lumaCoefficients.b * P0.b;
    float Y1 = lumaCoefficients.r * P1.r + lumaCoefficients.g * P1.g + lumaCoefficients.b * P1.b;
 
    return vec4(Y0, Y1, Y1, max(P1.a, P0.a));
}

