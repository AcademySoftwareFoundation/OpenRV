//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
// left eye, red
// right eye, cyan

vec4 main (const in inputImage in0, const in inputImage in1)
{
    vec4 P0 = in0();
    vec4 P1 = in1();
    return vec4(P0.r, P1.g, P1.b, max(P1.a, P0.a));
}

