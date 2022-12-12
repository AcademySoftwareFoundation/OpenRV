//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Viper Log to Linear
//

vec4 ColorViperLogLinear (const in vec4 P)
{
    return vec4(pow(vec3(10.0), min(P.rgb, 1.0) * vec3(2.048)) * vec3(0.0359824060427) , P.a);
}
