//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Convert from ACESLog to linear 
//
//  From file acesLog32f_to_aces.ctl of the ACES CTL dev code.
//

vec4 ColorACESLogLinear (const in vec4 P)
{
    const vec3 aceslog_unity = vec3(32768.0);
    const vec3 aceslog_xperstop = vec3(2048.0);
    vec3 aces_denorm_trans = vec3(pow( 2.0, -15.0));
    vec3 aces_denorm_fake0 = vec3(pow( 2.0, -16.0));

    vec3 aces = pow( vec3(2.0), (P.rgb * aceslog_unity - aceslog_unity) / aceslog_xperstop);
  
    // if (aces < aces_denorm_trans) 
    bvec3 t = lessThan(aces, aces_denorm_trans);

    vec3 c1 = (aces - aces_denorm_fake0) * 2.0;
    return vec4(mix(aces, c1, vec3(t)), P.a);
}

