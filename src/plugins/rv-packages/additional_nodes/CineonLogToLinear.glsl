//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

//
//  Cineon Log to Linear
//
//  NB: refBlack, refWhite and softClip are specified in the range [0..1023];
// 

// 0.003333333333 = 0.002/0.6
 
vec4 main (const in inputImage in0,
           const in float refBlack,
           const in float refWhite,
           const in float softClip)
{
    vec4 P = in0();
    vec3 nP = min(P.rgb,1.0) * vec3(1023.0);
    float black = pow(10.0, refBlack * 0.003333333333);
    float whiteBlackDiff = pow(10.0, refWhite * 0.003333333333) - black;
    // Compute normalized Cineon 10bit to linear between refBlack and breakpoint
    vec3 c = max((pow(vec3(10.0), nP * vec3(0.003333333333)) - vec3(black)), vec3(0.0)) / vec3(whiteBlackDiff);

    if (softClip != 0.0)
    {
        float breakpoint = refWhite - softClip;
        float kneeOffset = (pow(10.0, breakpoint * 0.003333333333) - black) / whiteBlackDiff;
        float kneeGain = (1.0 - kneeOffset)/pow(5.0*softClip,softClip*0.01);

        // Compute normalized Cineon 10bit to linear between breakpoint and 1.0; softclip zone.
        vec3 cSoftClipped = pow(abs(nP - vec3(breakpoint)), vec3(softClip*0.01)) * vec3(kneeGain) + vec3(kneeOffset); 

        // 
        // Test for values above breakpoint.
        //
        bvec3 useSoftClip = greaterThanEqual(nP, vec3(breakpoint));

        return vec4(mix(c, cSoftClipped, vec3(useSoftClip)), P.a);
    }
    else
    {
        return vec4(c, P.a );
    }
}


