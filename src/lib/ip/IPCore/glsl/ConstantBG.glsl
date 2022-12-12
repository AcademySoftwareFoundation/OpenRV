//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

vec4 main(const in inputImage in0,
          const in outputImage outImage,
          const in vec3 bgColor)
{
    vec4  c   = in0();
    vec3  rgb = c.rgb;
    float a   = c.a;
    vec3  bg  = bgColor;

    return vec4(rgb * a + (1.0 - a) * bg, a);
}
