//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
vec4 ColorYCbCrSRGB (const in vec4 P)
{
    float r = P.r + 1.402 * (P.b - 0.5);
    float g = P.r - 0.34414 * (P.g - 0.5) - 0.71414 * (P.b - 0.5);
    float b = P.r + 1.772 * (P.g - 0.5);
    return vec4(r, g, b, P.a);
}
