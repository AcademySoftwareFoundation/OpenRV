//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Convert from YCgCo to RGB 
//
//


vec4 main (const in inputImage in0)
{
    const vec4 offset = vec4(0.0, 0.5, 0.5, 0.0);
    const mat4 m = mat4(1.0, 1.0, 1.0, 0,
                        -1.0, 1.0, -1.0, 0,
                        1.0, 0.0, -1.0, 0,
                        0, 0, 0, 1.0);

    return ( m * (in0() - offset));
}
