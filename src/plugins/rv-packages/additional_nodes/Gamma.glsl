//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Std Gamma Curve.
//

vec4 main (const in inputImage in0, const in vec3 gamma)
{
    vec4 P = in0();
    return vec4(pow(max(P.rgb, vec3(0.0)), gamma), P.a);
}
