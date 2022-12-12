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
uniform float parityOffset; // 0.0 or 1.0

void main ()
{
    vec4 c0 = texture2DRect(texture0, TexCoord0);
    vec4 c1 = texture2DRect(texture1, TexCoord0);
    FRAGCOLOR = mix(c0, c1, vec4(mod((gl_FragCoord.y + parityOffset), 2.0) < 1.0));
}

