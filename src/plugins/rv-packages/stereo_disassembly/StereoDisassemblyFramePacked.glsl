//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

vec4 main (const in inputImage in0, const in int stereoEye, const in int topBottom, const in int swapEyes)
{
    vec2 sz = in0.size();
    float pa = sz.x/sz.y;

    //
    //  Top & Bottom
    //
    //  XXX This assumes an UPPER LEFT native pixel coordinate origin.
    //

    vec2 topOff  = vec2(sz.x/4.0-in0.s/2.0, -in0.t/2.0);
    vec2 botOff  = vec2(sz.x/4.0-in0.s/2.0, sz.y/2.0 - in0.t/2.0);

    //
    //  Side by Side
    //

    vec2 lOff = vec2(-sz.x/4.0, 0.0);
    vec2 rOff = vec2( sz.x/4.0, 0.0);

    //
    //  Define left/right image
    //

    vec2 leftEyeOff  = mix(lOff, topOff, vec2(topBottom));
    vec2 rightEyeOff = mix(rOff, botOff, vec2(topBottom));

    //
    //  Pick result image according to stereoEye
    //

    vec2 swappedLeftEyeOff  = mix(leftEyeOff, rightEyeOff, vec2(swapEyes));
    vec2 swappedRightEyeOff = mix(rightEyeOff, leftEyeOff, vec2(swapEyes));

    return in0(mix(swappedLeftEyeOff, swappedRightEyeOff, vec2(stereoEye)));
}
