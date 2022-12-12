//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Linear to Alexa LogC v3
//

vec4 main (const in inputImage in0,
           const in float LogCBlackSignal,
           const in float LogCEncodingOffset,
           const in float LogCEncodingGain,
           const in float LogCGraySignal,
           const in float LogCBlackOffset,
           const in float LogCLinearSlope,
           const in float LogCLinearOffset,
           const in float LogCCutPoint)
{
    vec4 P = in0();
    vec3 xr = (P.rgb - vec3(LogCBlackSignal)) / vec3(LogCGraySignal) + vec3(LogCBlackOffset);
    vec3 nlin = (log(max(xr, vec3(LogCCutPoint))) / log(10.0)) * vec3(LogCEncodingGain) + vec3(LogCEncodingOffset);
    vec3 lin = (xr * vec3(LogCLinearSlope) + vec3(LogCLinearOffset)) * vec3(LogCEncodingGain) + vec3(LogCEncodingOffset);
    bvec3 t = lessThanEqual(xr, vec3(LogCCutPoint));
    return vec4(mix(nlin, lin, vec3(t)), P.a);
}
