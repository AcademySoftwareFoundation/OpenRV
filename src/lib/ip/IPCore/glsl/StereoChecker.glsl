//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

vec4 main (const in inputImage in0,
           const in inputImage in1,
           const in float parityOffsetX,
           const in float parityOffsetY,
	   const in outputImage win)
{
    vec4 c0    = in0();
    vec4 c1    = in1();
    vec4 yeven = vec4(mod(win.y + parityOffsetY, 2.0) < 1.0);
    vec4 xeven = vec4(mod(win.x + parityOffsetX, 2.0) < 1.0);
    vec4 sc0   = mix(c0, c1, yeven);
    vec4 sc1   = mix(c1, c0, yeven);
    return mix(sc0, sc1, xeven);
}

