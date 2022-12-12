//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Premultiplied Over 
//

vec4 main(const in vec4 i0, const in vec4 i1, const in vec4 i2, const in vec4 i3)
{
    // 0 over 1
    float a1 = 1.0 - i0.a;
    vec4 over2 = vec4(i0.rgb + i1.rgb * a1, i0.a + i1.a * a1);

    // then over 2
    float a2 = 1.0 - over2.a;
    vec4 over3 = vec4(over2.rgb + i2.rgb * a2, over2.a + i2.a * a2);

    // then over 3
    float a3 = 1.0 - over3.a;
    return vec4(over3.rgb + i3.rgb * a3, over3.a + i3.a * a3);

}
