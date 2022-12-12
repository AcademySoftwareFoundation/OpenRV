//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Convert from YCbCr 601 to RGB 
//
//

vec4 main (const in inputImage in0)
{
    const mat4 m = mat4(
       1.164384e+00,   1.164384e+00,   1.164384e+00,   0.000000e+00,
       0.000000e+00,  -3.917623e-01,   2.017232e+00,   0.000000e+00,
       1.596027e+00,  -8.129675e-01,   0.000000e+00,   0.000000e+00,
      -8.742022e-01,   5.316678e-01,  -1.085631e+00,   1.000000e+00);

    vec4 P = in0();
    vec4 cout = m * vec4(P.rgb, 1.0);
    return vec4( cout.rgb, P.a );
}
