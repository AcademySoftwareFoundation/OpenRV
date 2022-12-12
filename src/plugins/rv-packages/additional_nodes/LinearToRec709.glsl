//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Convert from linear to Rec709 
//
//  NOTE: this code is manually vectorized.
//

vec4 main (const in inputImage in0)
{
    vec4 P = in0();
    const vec3 q = vec3(0.018);
    const vec3 a0 = vec3(0.099);
    const vec3 a1 = vec3(1.099);
    const vec3 b = vec3(4.5);
    const vec3 g = vec3(0.45);

    vec3  c = max(P.rgb, vec3(0.0));
    bvec3 t = lessThanEqual(c, q);
    vec3 c0 = c * b;
    vec3 c1 = a1 * pow(c, g) - a0;
    return vec4(mix(c1, c0, vec3(t)), P.a);
}
