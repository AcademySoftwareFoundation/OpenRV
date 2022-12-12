//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
vec4 ColorSRGBYCbCr (const in vec4 P)
{
    float y = 0.299 * P.r + 0.587 * P.g + 0.114 * P.b;
    float cb = 0.5 - 0.168736 * P.r - 0.331264 * P.g + 0.5 * P.b;
    float cr = 0.5 + 0.5 * P.r - 0.418688 * P.g - 0.081312 * P.b;
    return vec4(y, cb, cr, P.a);
}
