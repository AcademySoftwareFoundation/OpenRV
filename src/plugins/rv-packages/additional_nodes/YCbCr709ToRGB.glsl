//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Convert from YCbCr 709 to RGB 
//
//

vec4 main (const in inputImage in0)
{
    const mat4 m = mat4(
       1.164384e+00,   1.164384e+00,   1.164384e+00,   0.000000e+00,
       0.000000e+00,  -2.132486e-01,   2.112402e+00,   0.000000e+00,
       1.792741e+00,  -5.329093e-01,   0.000000e+00,   0.000000e+00,
      -9.729452e-01,   3.014827e-01,  -1.133402e+00,   1.000000e+00);

    vec4 P = in0();
    vec4 cout = m * vec4(P.rgb, 1.0);
    return vec4( cout.rgb, P.a );
}
