//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Convert from RGB to YCbCr 709 Full Range
//
//

vec4 main (const in inputImage in0)
{
    const mat4 m = mat4(
       2.126001e-01,  -1.145722e-01,   5.000000e-01,   0.000000e+00,
       7.152000e-01,  -3.854279e-01,  -4.541529e-01,   0.000000e+00,
       7.220000e-02,   5.000000e-01,  -4.584708e-02,   0.000000e+00,
       0.000000e+00,   5.019608e-01,   5.019608e-01,   1.000000e+00);

    vec4 P = in0();
    vec4 cout = m * vec4(P.rgb, 1.0);
    return vec4( cout.rgb, P.a );
}
