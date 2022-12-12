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
 
vec4 ColorCineonLogLinear (const in vec4 P,
                           const in float cinBlack,
                           const in float cinWhiteBlackDiff)
{
    // Compute normalized Cineon 10bit to linear.
    return vec4(max((pow(vec3(10.0), min(P.rgb, vec3(1.0)) * vec3(NORMALISED_TF)) - vec3(cinBlack)), vec3(0.0)) / vec3(cinWhiteBlackDiff), P.a );
}


