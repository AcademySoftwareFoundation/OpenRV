//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  NOTE: this is multiplying a 4D coordinate by a 4x4 matrix.  This
//  is probably not what you want. See ColorMatrix
//

vec4 ColorMatrix4D (const in vec4 P, const in mat4 M)
{
    return M * P;
}
