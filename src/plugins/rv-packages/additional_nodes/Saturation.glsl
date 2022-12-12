//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Saturation.
//  Default Rec709 lumaCoeffs.
//

vec4 main (const in inputImage in0, 
           const in float saturation,
           const in vec3 lumaCoefficients,
           const in float minClamp,
           const in float maxClamp)
{
    vec4 P = in0();
    vec3 c =   P.rgb * vec3(saturation) +
               vec3(P.r * lumaCoefficients.r +
	            P.g * lumaCoefficients.g +
	            P.b * lumaCoefficients.b) * vec3(1.0 - saturation) ;

    return vec4(clamp(c, minClamp, maxClamp), P.a);
}


