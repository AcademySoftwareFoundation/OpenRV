//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

// Note: This is a "light" version of the std ColorPremult
//       shader in so far that it does NOT premultiply the
//       RGB channel values if the alpha channel is zero.
//       
vec4 ColorPremultLight(const in vec4 P)
{
    return P.a > 0.0 ?  vec4(P.rgb * P.aaa, P.a) : P;
}
