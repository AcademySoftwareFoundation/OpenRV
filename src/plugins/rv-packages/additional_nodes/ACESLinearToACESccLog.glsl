//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
// This node implement ACEScc color encoding function.
//
// See Section 4.4 "ACEScc" of spec S-2014-03.
//
// It converts ACES linear color values to ACEScc Log
// values.
// First a color space gamut conversion is performed
// that takes the input ACES primaries linear values to ACEScc
// primaries linear space.
// Then an ACEScc log encoding is performed.
//

vec4 main(const in inputImage in0)
{
    // ACES to ACEScc gamut conversion matrix.
    // refer to ACEScc spec S-2014-003.
    //
    const mat3 m33 = mat3(
	1.4514393161, -0.0765537734,  0.0083161484,
       -0.2365107469,  1.1762296998, -0.0060324498,
       -0.2149285693, -0.0996759264,  0.9977163014);

    vec4 P = in0();
    vec3 acescc = m33 * P.rgb;

    bvec3 t0 = lessThan(acescc, vec3(0.0));
    bvec3 t1 = lessThan(acescc, vec3(pow( 2.0, -15.0)));

    vec3 two_neg16 = vec3(pow( 2.0, -16.0));

    vec3 acescc1 = mix(acescc, two_neg16 + acescc * vec3(0.5), vec3(t1));

    vec3 acescc2 = mix(acescc1, two_neg16, vec3(t0));
    
    vec3 acesccLog = (log2(acescc2) + vec3(9.72)) / vec3(17.52);

    //return vec4(acescc, P.a);  // Output without log encoding; for debugging.
    return vec4(acesccLog, P.a);
}

