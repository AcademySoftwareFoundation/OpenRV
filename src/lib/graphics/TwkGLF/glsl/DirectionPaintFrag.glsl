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
#define FRAGCOLOR gl_FragColor
#define in varying
#endif

in vec2 TexCoord0;
in float DirectionCoord;

void main() 
{ 
    FRAGCOLOR.rgb = vec3(DirectionCoord);

    vec2  m      = TexCoord0 - vec2(0.5, 0.5);
    float border = 0.05; 
    float radius = 0.5;
    float dist   = radius - sqrt(m.x * m.x + m.y * m.y);
    float t      = 0.0;

    if (dist > border) t = 1.0;
    else if (dist > 0.0) t = dist / border;

    FRAGCOLOR.a = t; 
}
