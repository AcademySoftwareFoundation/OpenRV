//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Red Log to Linear
//
//  f(x) = (10^(2*x) - 1)/(10^2 - 1); x >= 0
//  g(x) = -f(abs(x)); x < 0
// 

vec4 ColorRedLogLinear (const in vec4 P)
{
    bvec3 t = lessThan(P.rgb, vec3(0.0));
    vec3 c = vec3(2.0) * abs(P.rgb);

    // Case if pixel values are positive in value.
    vec3 pos_C = pow(vec3(10.0), c) - vec3(1.0);

    // Case if pixel values are negative in value.
    vec3 neg_C = vec3(-1.0) * pos_C;

    return vec4(mix(pos_C, neg_C, vec3(t)) / vec3(99.0), P.a);
}


