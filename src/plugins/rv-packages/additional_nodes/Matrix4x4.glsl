//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
// This is 4x4 matrix operation.
//

vec4 main(const in inputImage in0, const in mat4 m44)
{
    return m44 * in0();
}

