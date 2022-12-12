//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Alexa LogC v3 to Linear
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
    float logC_A = 1.0 / LogCEncodingGain;
    float logC_B = - LogCEncodingOffset / LogCEncodingGain;
    float logC_C = LogCGraySignal;
    float logC_D = LogCBlackSignal - LogCBlackOffset * LogCGraySignal;
    float logC_X = LogCGraySignal / (LogCEncodingGain * LogCLinearSlope);
    float logC_Y = - (LogCEncodingOffset + 
        LogCEncodingGain * (LogCLinearSlope * (LogCBlackOffset - LogCBlackSignal / LogCGraySignal)
        + LogCLinearOffset)) * logC_X;

    vec4 P = in0();
    vec3 c    = P.rgb;
    vec3 lin  = c * vec3(logC_X) + vec3(logC_Y);
    vec3 nlin = pow(vec3(10.0), c * vec3(logC_A) + vec3(logC_B)) * vec3(logC_C) + vec3(logC_D);
    bvec3 t = lessThanEqual(c, vec3(LogCCutPoint));
    return vec4(mix(nlin, lin, vec3(t)), P.a);
}
