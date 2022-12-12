//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
//glBlendFunc(GL_ONE, GL_ONE);

vec4 main(const in vec4 i0, const in vec4 i1, const in vec4 i2)
{
    return vec4(clamp(i2.r-i1.r-i0.r, 0.0, 1.0),
                clamp(i2.g-i1.g-i0.g, 0.0, 1.0),
                clamp(i2.b-i1.b-i0.b, 0.0, 1.0),
                clamp(i2.a-i1.a-i0.a, 0.0, 1.0));
}
