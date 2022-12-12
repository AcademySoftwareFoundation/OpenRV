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

uniform vec4 uniformColor;
in vec2 TexCoord0;

void main() 
{
    vec2  m      = TexCoord0 - vec2(0.5, 0.5);
    float mag    = sqrt(m.x * m.x + m.y * m.y);
    float radius = 0.5;
    float bot    = 0.01831563888; /* exp(-4.0) */
    float ratio  = mag * 2.0 / radius;
    float t      = 0.0;

    if (ratio <= 2.0) t = exp(-ratio*ratio)-bot*ratio*0.5;
    FRAGCOLOR = uniformColor * vec4(1, 1, 1, t); 
}

