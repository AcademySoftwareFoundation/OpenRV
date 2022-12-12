//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

//
//  Linear to Cineon Log
//
//  NB: refBlack, refWhite are specified in the range [0..1023];
// 

// 0.003333333333 = 0.002/0.6
// 7.85181516711 = 1023.0 * log(10.0) * 0.002/0.6
 
vec4 ColorLinearCineonLog (const in vec4 P,
           const in float refBlack,
           const in float refWhite)
{
    float gain = 1.0 - pow(10.0, (refBlack-refWhite) * 0.003333333333);
    return vec4(vec3(refWhite/1023.0) + log((P.rgb - 1.0) * vec3(gain) + vec3(1.0)) / vec3(7.85181516711), P.a);
}


