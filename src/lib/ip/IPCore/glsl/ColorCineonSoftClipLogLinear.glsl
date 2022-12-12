//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

//
//  Cineon Log to Linear
//
// 

// 1023 = 10bit scaling value;
// Equals 3.41.
#define NORMALISED_TF  (1023.0 * 0.002/0.6)
 
vec4 ColorCineonSoftClipLogLinear (const in vec4 P,
                                   const in float cinBlack,
                                   const in float cinWhiteBlackDiff,
                                   const in float cinSoftClip,
                                   const in float cinBreakpoint,
                                   const in float cinKneeGain,
                                   const in float cinKneeOffset)
{
    vec3 nP = min(P.rgb, 1.0);
    vec3 breakpoint = vec3(cinBreakpoint);

    // Compute normalized Cineon 10bit to linear between refBlack and breakpoint
    vec3 c = max((pow(vec3(10.0), nP * vec3(NORMALISED_TF)) - vec3(cinBlack)), vec3(0.0)) / vec3(cinWhiteBlackDiff);

    // Compute normalized Cineon 10bit to linear between breakpoint and 1.0; softclip zone.
    vec3 cSoftClipped = pow(abs(nP - breakpoint) * vec3(1023.0), vec3(cinSoftClip)) * vec3(cinKneeGain) + vec3(cinKneeOffset); 

    // 
    // Test for values above breakpoint.
    //
    bvec3 useSoftClip = greaterThanEqual(nP, breakpoint);

    return vec4(mix(c, cSoftClipped, vec3(useSoftClip)), P.a );
}


