//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

vec4 ColorPremult (const in vec4 P)
{
    return vec4(P.rgb * P.aaa, P.a);
}
