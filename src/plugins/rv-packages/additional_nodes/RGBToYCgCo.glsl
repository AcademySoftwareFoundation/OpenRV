//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Convert from RGB to YCgCo
//
//

vec4 main (const in inputImage in0)
{
    const mat4 m = mat4(0.25, -0.25, 0.5, 0.0,
                        0.5, 0.5, 0.0, 0.0,
                        0.25, -0.25, -0.5, 0.0,
                        0.0, 0.0, 0.0, 1.0);
    const vec4 offset = vec4(0, 0.5, 0.5, 0.0);

    return (offset + m * in0());

}
