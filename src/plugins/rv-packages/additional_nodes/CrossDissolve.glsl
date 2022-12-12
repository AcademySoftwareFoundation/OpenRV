//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Cross Dissolve between (pixels)
//

vec4 main (const in inputImage in0, 
           const in inputImage in1,
           const in float startFrame,
           const in float numFrames,
           const in float frame)
{
    //
    //  Make sure that first visible effect is on startFrame, and last visible
    //  effect is on startFrame + numFrames - 1, so that effect is on screen
    //  for numFrames.
    //
    
    float t = (frame - startFrame + 1.0) / (numFrames + 1.0);
    return mix(in0(), in1(), vec4(clamp(t, 0.0, 1.0)));
}

