//
// Copyright (C) 2024  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

vec4 Opacity (const in vec4 i0, const in float opacity)
{
    // i0 is premultiplied (ie.: i0.rgb is already multiplied by i0.a)
    return i0 * vec4(opacity);
}
