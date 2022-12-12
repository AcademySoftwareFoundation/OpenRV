//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
vec4 ColorLinearGray (const in vec4 P)
{
    float y = 0.299 * P.r + 0.587 * P.g + 0.114 * P.b;
    return vec4(y, y, y, P.a);
}
