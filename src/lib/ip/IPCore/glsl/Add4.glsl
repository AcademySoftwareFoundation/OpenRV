//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//glBlendEquation(GL_FUNC_ADD);
//glBlendFunc(GL_ONE,
//            GL_ONE);

vec4 main(const in vec4 i0, const in vec4 i1, const in vec4 i2, const in vec4 i3)
{
	vec4 e = i0 + i1 + i2 + i3;
    return vec4(clamp(e.r, 0.0, 1.0),
				clamp(e.g, 0.0, 1.0),
				clamp(e.b, 0.0, 1.0),
				clamp(e.a, 0.0, 1.0));

}
