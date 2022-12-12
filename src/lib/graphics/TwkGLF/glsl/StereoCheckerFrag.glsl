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

uniform sampler2DRect texture0;
uniform sampler2DRect texture1;
in vec2 TexCoord0;
uniform vec2 parityOffset;

void main ()
{
    vec4 c0    = texture2DRect(texture0, TexCoord0); 
    vec4 c1    = texture2DRect(texture1, TexCoord0); 
    vec4 yeven = vec4(mod(gl_FragCoord.y + parityOffset.y, 2.0) < 1.0);
    vec4 xeven = vec4(mod(gl_FragCoord.x + parityOffset.x, 2.0) < 1.0);
    vec4 sc0   = mix(c0, c1, yeven);
    vec4 sc1   = mix(c1, c0, yeven);
    FRAGCOLOR  = mix(sc0, sc1, xeven);
}
