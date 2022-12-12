//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Convert from sRGB to linear 
//  This and Rec709 could be done using the "4 value model" but this is
//  fine now.
//

vec4 ColorSRGBLinear (const in vec4 P)
{
    const vec3 a = vec3(0.055);
    const vec3 b = vec3(12.92);
    const vec3 p = vec3(0.04045);
    const vec3 g = vec3(2.4);

    vec3  c = max(P.rgb, vec3(0.0));
    bvec3 t = lessThanEqual(c, p);
    vec3 c0 = c / b;
    vec3 c1 = pow((c + a) / (vec3(1.0) + a), g);
    return vec4(mix(c1, c0, vec3(t)), P.a);
}
