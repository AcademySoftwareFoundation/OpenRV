//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Linear to Viper Log
//

vec4 main (const in inputImage in0)
{
    vec4 P = in0();
    return vec4(log(P.rgb * vec3(1.0/0.0359824060427)) * vec3(1.0/(log(10.0)*2.048)), P.a);
}
