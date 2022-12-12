//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Convert from Linear to ACESLog
//
//  From file aces_to_acesLog32f.ctl of the ACES CTL dev code.
//

vec4 ColorLinearACESLog (const in vec4 P)
{
    const vec3 aceslog_unity = vec3(32768.0);
    const vec3 aceslog_xperstop = vec3(2048.0);
    vec3 aces_denorm_trans = vec3(pow( 2.0, -15.0));
    vec3 aces_denorm_fake0 = vec3(pow( 2.0, -16.0));

    vec3 aces = max(P.rgb, vec3(0.0));

    bvec3 t0 = lessThan(aces, vec3(0.0));
    bvec3 t1 = lessThan(aces, aces_denorm_trans);
  
    /* compress denorms into 1 stop below last norm stop */
    vec3 c1 = aces_denorm_fake0 + ( aces / 2.0);
    aces = mix(aces, c1, vec3(t1));

    vec3 acesLog = (log2(aces) * aceslog_xperstop + aceslog_unity) / aceslog_unity;
  
    return vec4(mix(acesLog, vec3(0.0), vec3(t0)), P.a);
}



