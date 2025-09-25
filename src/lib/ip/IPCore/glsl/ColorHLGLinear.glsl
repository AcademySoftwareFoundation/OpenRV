//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//
//  Convert from Hybrid Log-Gamma (ARIB STD-B67, BT.2100) to linear
//

vec4 ColorHLGLinear(const in vec4 P)
{
    const float Lw = 1000.0;
    const float a = 0.17883277;
    const float b = 0.28466892;
    const float c = 0.55991073;
    const float threshold = 0.5;

    const float gamma = 1.2;
    const float minLuma = pow(1e-4f, (1.0 / gamma));

    vec3 v = max(vec3(0), P.rgb);
    bvec3 t = lessThanEqual(v, vec3(threshold));

    vec3 eotf = mix(
        (exp((v - c) / a) + b) / 12.0,
        (v * v) / 3.0,
        vec3(t)
    );

    // luma assuming input is Rec.2020 RGB.
    float luma = dot(eotf, vec3(0.2627, 0.6780, 0.0593));
    luma = pow(max(luma, minLuma), gamma - 1.0) * Lw;

    return vec4(eotf * luma * 0.01, P.a);
}