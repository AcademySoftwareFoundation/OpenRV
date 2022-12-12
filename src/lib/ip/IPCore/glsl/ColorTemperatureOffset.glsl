//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

vec4 ColorTemperatureOffset(const in vec4 i0, const in vec4 c)
{
    float l = 0.299 * i0.r + 0.587 * i0.g + 0.114 * i0.b;
    return i0 + vec4(c.rgb * c.a * l, 0);
}
