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

#extension GL_ARB_texture_rectangle : require

uniform vec4 uniformColor;
uniform sampler2DRect texture0; 
in vec2 TexCoord0; // paint stroke texcoord [-1, 1]
in vec2 TexCoord1; // paint stroke texcoord w.r.t. frame

void main() 
{ 
    vec2  d      = fwidth(TexCoord0);
    float l      = max(d.x, d.y);
    float w      = 1.0 / l;
    float border = 2.0 / w;
    vec2  m      = TexCoord0 - vec2(0.5, 0.5);
    float radius = 0.5;
    float dist   = radius - length(m);
    float t      = 0.0;

    if (dist > border)
    {
        t = 1.0;
    }
    else if (dist > 0.0)
    {
        t = smoothstep(0.0, 1.0, dist / border);
    }
    FRAGCOLOR.a = t;

    vec4  back         = texture2DRect(texture0, TexCoord1);
    FRAGCOLOR.rgb      = uniformColor.rgb * back.rgb;
}
