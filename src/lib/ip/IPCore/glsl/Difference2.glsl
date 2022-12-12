//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//glBlendEquation(GL_FUNC_SUBTRACT);
//glBlendFunc(GL_ONE, GL_ONE);

vec4 main(const in vec4 i0, const in vec4 i1)
{
    return vec4(clamp(i0.r-i1.r, 0.0, 1.0),
                clamp(i0.g-i1.g, 0.0, 1.0),
                clamp(i0.b-i1.b, 0.0, 1.0),
                clamp(i0.a-i1.a, 0.0, 1.0));
}
