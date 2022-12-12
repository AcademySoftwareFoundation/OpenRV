//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
// pi has x,y as the point, z as the derivative
vec4 ColorCurve(const in vec4 P,
                const in vec3 p1,
                const in vec3 p2,
                const in vec3 p3,
                const in vec3 p4)
{
    if (P.r > 1.0) return P;
    vec3 tP1, tP2;
    // piece wise cubic
    if (P.x < p2.x)
    {
        tP1 = p1; tP2 = p2;
    }
    else if (P.x < p3.x)
    {
        tP1 = p2; tP2 = p3;
    }
    else
    {
        tP1 = p3; tP2 = p4;
    }
    float h = tP2.x - tP1.x;
    float t = (P.x - tP1.x) / h;
    float t3 = t * t * t;
    float t2 = t * t;
    float l = tP1.y * (2.0 * t3 - 3.0 * t2 + 1.0)
              + h * tP1.z * (t3 - 2.0 * t2 + t)
              + tP2.y * (-2.0 * t3 + 3.0 * t2)
              + h * tP2.z * (t3 - t2);
    return vec4(l, P.gba);
}