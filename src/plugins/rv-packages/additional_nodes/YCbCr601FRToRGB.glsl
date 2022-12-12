//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Convert from YCbCr 601 Full Range to RGB 
//
//


vec4 main (const in inputImage in0)
{
    const mat4 m = mat4(
       1.000000e+00,   1.000000e+00,   1.000000e+00,   0.000000e+00,
       0.000000e+00,  -3.441363e-01,   1.772000e+00,   0.000000e+00,
       1.402000e+00,  -7.141362e-01,   0.000000e+00,   0.000000e+00,
      -7.037491e-01,   5.312113e-01,  -8.894745e-01,   1.000000e+00);

    vec4 P = in0();
    vec4 cout = m * vec4(P.rgb, 1.0);
    return vec4( cout.rgb, P.a );
}
