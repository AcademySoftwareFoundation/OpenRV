//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//glBlendEquation(GL_FUNC_ADD);
//glBlendFunc(GL_ONE,
//            GL_ONE);
vec4 main(const in vec4 i0, const in vec4 i1)
{
	vec4 c = i0 + i1;
	return vec4(clamp(c.r, 0.0, 1.0),
				clamp(c.g, 0.0, 1.0),
				clamp(c.b, 0.0, 1.0),
				clamp(c.a, 0.0, 1.0));
}
