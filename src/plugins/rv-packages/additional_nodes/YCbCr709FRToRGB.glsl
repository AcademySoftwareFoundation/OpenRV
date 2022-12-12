//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Convert from YCbCr 709 Full Range to RGB 
//
//

vec4 main (const in inputImage in0)
{
    const mat4 m = mat4(
       1.000000e+00,   1.000000e+00,   1.000000e+00,   0.000000e+00,
       0.000000e+00,  -1.873243e-01,   1.855600e+00,   0.000000e+00,
       1.574800e+00,  -4.681243e-01,   0.000000e+00,   0.000000e+00,
      -7.904879e-01,   3.290095e-01,  -9.314385e-01,   1.000000e+00);

    vec4 P = in0();
    vec4 cout = m * vec4(P.rgb, 1.0);
    return vec4( cout.rgb, P.a );
}
