//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

//
//  NOTE: this is multiplying a homogeneous color coordinate by a 4x4
//  matrix. The alpha is restored afterwards so alpha is never touched (or
//  used). If you want an actual 4D non-homogenous matrix use
//  ColorMatrix4D).
//

vec4 ColorMatrix (const in vec4 P, const in mat4 M)
{
    vec4 c = M * vec4(P.rgb, 1);
    return vec4(c.rgb, P.a);
}
