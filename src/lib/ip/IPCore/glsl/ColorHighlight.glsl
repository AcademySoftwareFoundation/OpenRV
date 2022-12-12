//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

vec4 ColorHighlight(const in vec4 P,
                    const in vec4 coefficents,
                    const in float highlightPoint)
{
    if (P.r > 1.0) return P;
    if (P.r <= highlightPoint) return P;
    float l = coefficents.r * P.r * P.r * P.r
                + coefficents.g * P.r * P.r
                + coefficents.b * P.r
                + coefficents.a;
    return vec4(min(l, 1.0), P.gba);
}