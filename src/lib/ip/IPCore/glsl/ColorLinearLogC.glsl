//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  This one may need some optimization. I'm guessing that simplifying
//  the equations towards MAD form and precomputing the constant parts
//  outside the shader would help. This is what was done for the logc
//  to linear shader
//
//		 x - pbs
//	3:  xr = ------- + bo
//		   gs
//
//	2:  y = log  (xr) eg + eo
//		   10
//
//	1:  y = (xr ls + lo) eg + eo
//  
//  NB: cutoff = (cutoff_from_spec - vec3(pbs)) / vec3(gs) + vec3(bo);
//  e.g. for EI=800 cutoff_from_spec=0.010591 (from Alexa V3 spec).
//

vec4 ColorLinearLogC (const in vec4 P,
                      const in float pbs,
                      const in float eo,
                      const in float eg,
                      const in float gs,
                      const in float bo,
                      const in float ls,
                      const in float lo,
                      const in float cutoff)
{
    vec3 c = P.rgb;
    vec3 xr = (c - vec3(pbs)) / vec3(gs) + vec3(bo);
    vec3 nlin = (log(max(xr, vec3(cutoff)))/log(10.0)) * vec3(eg) + vec3(eo);
    vec3 lin = (xr * vec3(ls) + vec3(lo)) * vec3(eg) + vec3(eo);
    bvec3 t = lessThanEqual(xr, vec3(cutoff));
    return vec4(mix(nlin, lin, vec3(t)), P.a);
}

