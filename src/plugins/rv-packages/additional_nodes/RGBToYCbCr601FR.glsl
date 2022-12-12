//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Convert from RGB to YCbCr 601 Full Range
//
//

vec4 main (const in inputImage in0)
{
    const mat4 m = mat4(
       2.990000e-01,  -1.687359e-01,   5.000001e-01,   0.000000e+00,
       5.870001e-01,  -3.312642e-01,  -4.186876e-01,   0.000000e+00,
       1.140000e-01,   5.000000e-01,  -8.131242e-02,   0.000000e+00,
       0.000000e+00,   5.019608e-01,   5.019608e-01,   1.000000e+00);

    vec4 P = in0();
    vec4 cout = m * vec4(P.rgb, 1.0);
    return vec4( cout.rgb, P.a );
}
