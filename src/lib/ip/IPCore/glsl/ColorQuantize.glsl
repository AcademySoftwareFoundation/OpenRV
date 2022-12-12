//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
#if __VERSION__ < 150
#define round(X) floor(X + vec4(0.5))
#endif

vec4 ColorQuantize (const in vec4 P, const in float partitions)
{
    return round(P * partitions) / partitions;
}
