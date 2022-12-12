//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Academy Color Decision List 1.2
//
//  For CDL in ACES Log space
//

vec3 LinearToACESLog (const in vec3 aces)
{
    const vec3 aceslog_unity = vec3(32768.0);
    const vec3 aceslog_xperstop = vec3(2048.0);
    vec3 aces_denorm_trans = vec3(pow( 2.0, -15.0));
    vec3 aces_denorm_fake0 = vec3(pow( 2.0, -16.0));

    bvec3 t0 = lessThan(aces, vec3(0.0));
    bvec3 t1 = lessThan(aces, aces_denorm_trans);
  
    /* compress denorms into 1 stop below last norm stop */
    vec3 c1 = aces_denorm_fake0 + ( aces / 2.0);
    c1 = mix(aces, c1, vec3(t1));

    vec3 acesLog = (log2(c1) * aceslog_xperstop + aceslog_unity);
  
    return mix(acesLog, vec3(0.0), vec3(t0));
}


vec3 ACESLogToLinear (const in vec3 aceslog)
{
    const vec3 aceslog_unity = vec3(32768.0);
    const vec3 aceslog_xperstop = vec3(2048.0);
    vec3 aces_denorm_trans = vec3(pow( 2.0, -15.0));
    vec3 aces_denorm_fake0 = vec3(pow( 2.0, -16.0));

    vec3 aces = pow( vec3(2.0), (aceslog - aceslog_unity) / aceslog_xperstop);
  
    // if (aces < aces_denorm_trans) 
    bvec3 t = lessThan(aces, aces_denorm_trans);

    vec3 c1 = (aces - aces_denorm_fake0) * 2.0;
    return mix(aces, c1, vec3(t));
}

vec4 ColorCDLForACESLog (const in vec4 P, 
                         const in vec3 slope,
                         const in vec3 offset,
                         const in vec3 power,
                         const in float saturation,
                         const in vec3 lumaCoefficients,
                         const in vec3 refLow,  // 12860.643
                         const in vec3 refHigh, // 48742.586
                         const in mat4 toACES,
                         const in mat4 fromACES,
                         const in float minClamp,
                         const in float maxClamp)
{
    vec4 aces = toACES * vec4(P.rgb, 1.0);
    vec3 aceslog = LinearToACESLog(max(aces.rgb, vec3(0.0)));

    vec3 cdl = (aceslog - refLow) / (refHigh - refLow);

    cdl = pow(clamp(cdl * slope + offset, minClamp, maxClamp), power);

    aceslog = refLow + cdl * (refHigh - refLow);

    cdl = ACESLogToLinear(aceslog);

    cdl = cdl * vec3(saturation) +
                    vec3(cdl.r * lumaCoefficients.r +
	                 cdl.g * lumaCoefficients.g +
	                 cdl.b * lumaCoefficients.b) * vec3(1.0 - saturation) ;


    aces = fromACES * vec4(cdl, 1.0);

    return vec4(aces.rgb, P.a);
}


