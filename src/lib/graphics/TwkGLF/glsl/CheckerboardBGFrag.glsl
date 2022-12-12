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
void main()
{
    vec2  st  = TexCoord0 - vec2(0.5);
    bvec2 f   = equal(mod(floor(st / 20.0), 2.0), vec2(0.0));
    bvec2 f2  = equal(f, f.yx);
    vec3  bg0 = vec3(0.20);
    vec3  bg1 = vec3(0.15);
    vec3  m   = vec3(f2, f2.x);
    vec3  bg  = mix(bg0, bg1, m);

    FRAGCOLOR = vec4(bg, 0.0); 
}
