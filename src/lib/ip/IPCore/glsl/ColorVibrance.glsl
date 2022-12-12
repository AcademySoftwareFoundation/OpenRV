//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

vec4 ColorVibrance(const in vec4 P,
                   const in vec3 rec,
                   const in float saturation)
{
    // compute this P's saturation
    float maxV = max(P.r, max(P.g, P.b));
    float minV = min(P.r, min(P.g, P.b));
    float ps;
    float epsilon = 0.0001;
    if (abs(maxV) < epsilon) ps = 0.0;
    else ps = (maxV - minV) / maxV;
    
    // luminance
    float l = 0.299 * P.r + 0.587 * P.g + 0.114 * P.b;
    float ls = min(l, 0.25) / 0.25;

    float s = (saturation - 1.0) * (1.0 - ps) + 1.0;
    s = (s - 1.0) * ls + 1.0;
    
    float rw709 = rec.r;
    float gw709 = rec.g;
    float bw709 = rec.b;
    
    float a = (1.0 - s) * rw709 + s;
    float b = (1.0 - s) * rw709;
    float c = (1.0 - s) * rw709;
    float d = (1.0 - s) * gw709;
    float e = (1.0 - s) * gw709 + s;
    float f = (1.0 - s) * gw709;
    float g = (1.0 - s) * bw709;
    float h = (1.0 - s) * bw709;
    float i = (1.0 - s) * bw709 + s;
    
    vec3 outC;
    outC.r = a * P.r + d * P.g + g * P.b;
    outC.g = b * P.r + e * P.g + h * P.b;
    outC.b = c * P.r + f * P.g + i * P.b;
    
    return vec4(outC, P.a);
}