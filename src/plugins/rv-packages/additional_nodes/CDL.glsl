//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Academy Color Decision List 1.2
//

vec4 main (const in inputImage in0, 
           const in vec3 slope,
           const in vec3 offset,
           const in vec3 power,
           const in float saturation,
           const in vec3 lumaCoefficients,
           const in float minClamp,
           const in float maxClamp)
{
    vec4 P = in0();
    vec3 cdl = pow(clamp(P.rgb * slope + offset, minClamp, maxClamp), power);
    vec3 c =   cdl * vec3(saturation) +
               vec3(cdl.r * lumaCoefficients.r +
	            cdl.g * lumaCoefficients.g +
	            cdl.b * lumaCoefficients.b) * vec3(1.0 - saturation) ;

    return vec4(clamp(c, minClamp, maxClamp), P.a);
}


