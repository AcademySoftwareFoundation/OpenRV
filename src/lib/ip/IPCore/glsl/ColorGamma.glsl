//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

vec4 ColorGamma (const in vec4 P, const in vec3 gamma)
{
    return vec4(pow(max(P.rgb, vec3(0.0)), gamma), P.a);
}
