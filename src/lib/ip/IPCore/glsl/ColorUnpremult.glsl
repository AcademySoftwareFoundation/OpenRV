//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

//
//  NOTE: this one is interesting. NVIDIA's compiler/card does not
//  like this:
//
//        return vec4( mix(P.rgb / P.a, 
//                         P.rgb, 
//                         vec3(bvec3(P.a == 0.0))),
//                     P.a );
//
//  because (P.rgb / P.a) can be NaN. I think its rejecting the entire
//  fragment when this happens. On AMD mac it works fine.
//

vec4 ColorUnpremult (const in vec4 P)
{
    return P.a > 0.0 ? vec4(P.rgb / P.a, P.a) : P;
}
