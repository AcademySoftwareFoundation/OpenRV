//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Academy Color Decision List 1.2
//
//  For CDL in ACES linear space
//

vec4 ColorCDLForACESLinear (const in vec4 P, 
                            const in vec3 slope,
                            const in vec3 offset,
                            const in vec3 power,
                            const in float saturation,
                            const in vec3 lumaCoefficients,
                            const in vec3 refLow,  // 0.001185417175293
                            const in vec3 refHigh, // 222.875
                            const in mat4 toACES,
                            const in mat4 fromACES,
                            const in float minClamp,
                            const in float maxClamp)
{
    vec4 aces = toACES * vec4(P.rgb, 1.0);

    vec3 cdl = (aces.rgb - refLow) / (refHigh - refLow);

    cdl = pow(clamp(cdl * slope + offset, minClamp, maxClamp), power);

    cdl = cdl * vec3(saturation) +
                 vec3(cdl.r * lumaCoefficients.r +
	              cdl.g * lumaCoefficients.g +
	              cdl.b * lumaCoefficients.b) * vec3(1.0 - saturation) ;

    cdl  = refLow + cdl * (refHigh - refLow);

    aces = fromACES * vec4(cdl, 1.0);

    return vec4(aces.rgb, P.a);
}


