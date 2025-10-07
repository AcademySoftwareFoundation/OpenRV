//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//
//  Convert from linear to SMPTE 2084 (PQ)
//

vec4 ColorLinearSMPTE2084(const in vec4 P)
{
    // PQ constants
    const vec3 m1 = vec3(0.25 * 2610.0 / 4096);
    const vec3 m2 = vec3(128.0 * 2523.0 / 4096);
    const vec3 c2 = vec3(32.0 * 2413.0 / 4096);
    const vec3 c3 = vec3(32.0 * 2392.0 / 4096);
    const vec3 c1 = c3 - c2 + vec3(1.0);

    // Input is in nits/100, convert to [0,1], where 1 is 10000 nits.
    vec3 L = max(P.rgb * 0.01, vec3(0.0));
    vec3 y = pow(L, m1);
    vec3 num = c1 + c2 * y;
    vec3 denom = vec3(1.0) + c3 * y;
    vec3 pq = pow(num / denom, m2);

    return vec4(pq, P.a);
}