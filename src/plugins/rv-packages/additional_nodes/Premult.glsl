//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
// This is premult operation.
//

vec4 main(const in inputImage in0)
{ 
    vec4 P = in0();
    return vec4 (P.rgb * P.aaa, P.a);
}

