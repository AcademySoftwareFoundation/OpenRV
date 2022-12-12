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

uniform mat4 projMatrix, modelviewMatrix;
in vec2 in_Position;
in vec2 in_TexCoord0;
in vec4 in_UniformColor;
out vec2 TexCoord0;
out vec4 UniformColor;

void main() 
{ 
    gl_Position = projMatrix * modelviewMatrix * vec4(in_Position, 0, 1);
    TexCoord0 = in_TexCoord0;
    UniformColor = in_UniformColor;
}

