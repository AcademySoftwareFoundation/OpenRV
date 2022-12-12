//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Convert from linear to sRGB 
//

vec4 main (const in inputImage in0)
{
    vec4 P = in0();
    const vec3 q = vec3(0.0031308);
    const vec3 a0 = vec3(0.055);
    const vec3 a1 = vec3(1.055);
    const vec3 b = vec3(12.92);
    const vec3 g = vec3(1.0 / 2.4);

    vec3  c = max(P.rgb, vec3(0.0));
    bvec3 t = lessThanEqual(c, q);
    vec3 c0 = c * b;
    vec3 c1 = a1 * pow(c, g) - a0;
    return vec4(mix(c1, c0, vec3(t)), P.a);
}
