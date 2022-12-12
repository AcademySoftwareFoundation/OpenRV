//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
#if __VERSION__ >= 150
#version 150
out vec4 FragColor;
#define FRAGCOLOR FragColor
#else
#define in varying
#define FRAGCOLOR gl_FragColor
#endif

uniform sampler2D texture0;
in vec2 TexCoord0;

void main() 
{ 
    FRAGCOLOR = texture2D(texture0, TexCoord0); 
}

