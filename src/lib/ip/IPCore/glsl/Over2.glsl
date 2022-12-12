//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//glBlendEquation(GL_FUNC_ADD);
//glBlendFuncSeparate(GL_ONE,
//                   GL_ONE_MINUS_SRC_ALPHA,
//                   GL_ONE,
//                   GL_ONE_MINUS_SRC_ALPHA);

vec4 main(const in vec4 i0, const in vec4 i1)
{
    // 0 over 1
    float a1 = 1.0 - i0.a;
    vec4 c = vec4(i0.rgb + i1.rgb * a1, i0.a + i1.a * a1);

    return vec4(clamp(c.r, 0.0, 1.0),
                clamp(c.g, 0.0, 1.0),
                clamp(c.b, 0.0, 1.0),
                clamp(c.a, 0.0, 1.0));
}
