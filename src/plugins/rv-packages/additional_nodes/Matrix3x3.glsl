//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
// This is 3x3 matrix operation.
//

vec4 main(const in inputImage in0, const in mat3 m33)
{
    vec4 P = in0();
    return vec4(m33 * P.rgb, P.a);
}

