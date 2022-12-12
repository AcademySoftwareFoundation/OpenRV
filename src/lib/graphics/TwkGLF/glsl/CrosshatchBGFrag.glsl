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
    vec2 st = TexCoord0 - vec2(0.5);
    bool f = mod(floor(st.x - st.y), 8.0) == 0.0 ||
             mod(floor(st.x + st.y), 8.0) == 0.0;
    vec3 bg1 = vec3(0.25);
    vec3 bg0 = vec3(0.18);
    vec3 bg = mix(bg0, bg1, vec3(f));

    FRAGCOLOR = vec4(bg, 0.0);
}
