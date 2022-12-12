//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

vec4 ColorLogCLinear (const in vec4 P,
                      const in float logC_A,
                      const in float logC_B,
                      const in float logC_C,
                      const in float logC_D,
                      const in float logC_X,
                      const in float logC_Y,
                      const in float logC_cutoff)
{
    vec3 c    = P.rgb;
    vec3 lin  = c * vec3(logC_X) + vec3(logC_Y);
    vec3 nlin = pow(vec3(10.0), c * vec3(logC_A) + vec3(logC_B)) * vec3(logC_C) + vec3(logC_D);
    bvec3 t = lessThanEqual(c, vec3(logC_cutoff));
    return vec4(mix(nlin, lin, vec3(t)), P.a);
}
