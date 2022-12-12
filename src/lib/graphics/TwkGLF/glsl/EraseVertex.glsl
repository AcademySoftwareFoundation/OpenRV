//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
#if __VERSION__ >= 150
#version 150
#else
#define in attribute
#define out varying
#endif


uniform float texWidth;
uniform float texHeight;
uniform mat4 projMatrix, modelviewMatrix;
in vec2 in_Position;
in vec2 in_TexCoord0;
out vec2 TexCoord0; // paint stroke texcoord [-1, 1]
out vec2 TexCoord1; // paint stroke texcoord w.r.t. frame

void main() 
{
    gl_Position = projMatrix * modelviewMatrix * vec4(in_Position, 0, 1);
    TexCoord1.x = (gl_Position.x + 1.0) * 0.5 * texWidth;
    TexCoord1.y = (gl_Position.y + 1.0) * 0.5 * texHeight;
    TexCoord0 = in_TexCoord0;
}
