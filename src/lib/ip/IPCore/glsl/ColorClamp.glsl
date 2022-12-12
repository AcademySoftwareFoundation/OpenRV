//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

vec4 ColorClamp (const in vec4 P,
                 const in float minValue,
                 const in float maxValue)
{
    return clamp(P, minValue, maxValue);
}
