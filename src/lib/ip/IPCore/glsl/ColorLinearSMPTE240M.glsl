//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Convert from linear to SMPTE 240M
//

vec4 ColorLinearSMPTE240M (const in vec4 P)
{
    const vec3 q = vec3(0.0228);
    const vec3 a = vec3(0.1115);
    const vec3 b = vec3(4.0);
    const vec3 g = vec3(0.45);

    vec3  c = max(P.rgb, vec3(0.0));
    bvec3 t = lessThanEqual(c, q);
    vec3 c0 = c * b;
    vec3 c1 = (vec3(1.0) + a) * pow(c, g) - a;
    return vec4(mix(c1, c0, vec3(t)), P.a);
}
