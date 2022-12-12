//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

vec4 main (const in inputImage in0,
           const in inputImage in1,
           const in float parityOffset, // 0.0 or 1.0
	   const in outputImage win)
{
    return mix(in0(), in1(), vec4(mod(win.y + parityOffset, 2.0) < 1.0));
}

