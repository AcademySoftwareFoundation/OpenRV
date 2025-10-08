//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//
//  Convert from SMPTE 2084 (PQ) to linear
//

vec4 ColorSMPTE2084Linear(const in vec4 P)
{
    // PQ constants
    const vec3 m1 = vec3(0.25 * 2610.0 / 4096);
    const vec3 m2 = vec3(128.0 * 2523.0 / 4096);
    const vec3 c2 = vec3(32.0 * 2413.0 / 4096);
    const vec3 c3 = vec3(32.0 * 2392.0 / 4096);
    const vec3 c1 = c3 - c2 + vec3(1.0);

    vec3 x = max(P.rgb, vec3(0.0));
    vec3 xp = pow(x, 1.0 / m2);
    vec3 num = max(xp - c1, vec3(0.0));
    vec3 denom = c2 - c3 * xp;
    vec3 nits = pow(num / denom, 1.0 / m1);

    // Output scale is 1.0 = 10000 nits, we map it to make 1.0 = 100 nits.
    vec3 linear = 100.0 * nits;
    return vec4(linear, P.a);
}