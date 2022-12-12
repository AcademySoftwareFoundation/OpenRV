//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Convert from linear to ICC style sRGB
//

vec4 ICCLinearSRGB (const in vec4 P,
                    const in vec3 y,
                    const in vec3 a,
                    const in vec3 b,
                    const in vec3 c,
                    const in vec3 d)
{
    vec3  x = max(P.rgb, vec3(0.0));
    bvec3 t = lessThan(c, d);
    vec3 x0 = c * x;
    vec3 x1 = pow(a * x + b, y);
    return vec4(mix(x1, x0, vec3(t)), P.a);
}
