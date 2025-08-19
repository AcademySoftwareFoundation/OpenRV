//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//
//  Convert from linear to Hybrid Log-Gamma (ARIB STD-B67, BT.2100)
//

vec4 ColorLinearHLG(const in vec4 P)
{
    const float Lw = 1000.0;
    const float a = 0.17883277;
    const float b = 0.28466892;
    const float c = 0.55991073;
    const float threshold = 1.0 / 12.0;

    const float gamma = 1.2;
    const float minLuma = 1e-4f;

    // linear 1.0 is assumed to be 100 nits, scale so 1.0 == 1 nits
    vec3 L = P.rgb * 100.0;
    // luma assuming input is Rec.2020 RGB.
    float luma = dot(L, vec3(0.2627, 0.6780, 0.0593));
    luma = pow(max(luma, minLuma) / Lw, (1.0 / gamma) - 1.0) / Lw;

    vec3 v = L * luma;
    bvec3 t = lessThanEqual(v, vec3(threshold));
    vec3 oetf = mix(
        a * log(max(12.0 * v - b, 1e-6)) + c,
        sqrt(3.0 * max(v, 0.0)),
        vec3(t)
    );

    return vec4(oetf, P.a);
}