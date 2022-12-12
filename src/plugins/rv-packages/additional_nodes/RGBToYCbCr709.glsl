//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Convert from RGB to YCbCr 709
//
//

vec4 main (const in inputImage in0)
{
    const mat4 m = mat4(
       1.825858e-01,  -1.006437e-01,   4.392157e-01,   0.000000e+00,
       6.142306e-01,  -3.385720e-01,  -3.989422e-01,   0.000000e+00,
       6.200706e-02,   4.392157e-01,  -4.027352e-02,   0.000000e+00,
       6.274501e-02,   5.019609e-01,   5.019609e-01,   1.000000e+00);

    vec4 P = in0();
    vec4 cout = m * vec4(P.rgb, 1.0);
    return vec4( cout.rgb, P.a );
}
