//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

vec4 ColorBlendWithConstant(const in vec4 i0, const in vec4 c)
{

    float l = 0.299 * i0.r + 0.587 * i0.g + 0.114 * i0.b;
    float rl = 0.299 * c.r + 0.587 * c.g + 0.114 * c.b;
    float ratio = l / rl;
    vec4 b = vec4(c.rgb * ratio, c.a);
    vec3 color = b.a * b.rgb + (1.0 - b.a) * i0.rgb;
    return vec4(color, i0.a);
}
