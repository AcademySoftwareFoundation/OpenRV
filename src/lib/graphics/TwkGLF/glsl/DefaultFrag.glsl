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

void main() 
{ 
    FRAGCOLOR = uniformColor; 
}

