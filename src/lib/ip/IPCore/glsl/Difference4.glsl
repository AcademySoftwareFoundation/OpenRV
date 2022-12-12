//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//glBlendEquation(GL_FUNC_SUBTRACT);
//glBlendFunc(GL_ONE, GL_ONE);

vec4 main(const in vec4 i0, const in vec4 i1, const in vec4 i2, const in vec4 i3)
{
    return vec4(clamp(i0.r-i1.r-i2.r-i3.r, 0.0, 1.0),
                clamp(i0.g-i1.g-i2.g-i3.g, 0.0, 1.0),
                clamp(i0.b-i1.b-i2.b-i3.b, 0.0, 1.0),
                clamp(i0.a-i1.a-i2.a-i3.a, 0.0, 1.0));
}
