//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Academy Color Decision List 1.1
//

vec4 ColorCDL (const in vec4 P, 
               const in vec3 slope,
               const in vec3 offset,
               const in vec3 power)
{
    return vec4(pow(clamp(P.rgb * slope + offset, 0.0, 1.0), power), P.a);
}
