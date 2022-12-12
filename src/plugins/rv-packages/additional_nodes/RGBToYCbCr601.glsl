//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Convert from RGB to YCbCr 601
//
//

vec4 main (const in inputImage in0)
{
    const mat4 m = mat4(
       2.567883e-01,  -1.482229e-01,   4.392157e-01,   0.000000e+00,
       5.041294e-01,  -2.909928e-01,  -3.677883e-01,   0.000000e+00,
       9.790588e-02,   4.392157e-01,  -7.142738e-02,   0.000000e+00,
       6.274512e-02,   5.019608e-01,   5.019608e-01,   1.000000e+00);

    vec4 P = in0();
    vec4 cout = m * vec4(P.rgb, 1.0);
    return vec4( cout.rgb, P.a );
}
