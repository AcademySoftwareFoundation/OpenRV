//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
// Dissolve blend between first two inputs only (ignores i2)

vec4 main(const in vec4 i0, const in vec4 i1, const in vec4 i2, const in float dissolveAmount)
{
    // Blend RGB channels from only i0 and i1 using dissolveAmount, ignore i2
    vec3 rgb = mix(i1.rgb, i0.rgb, dissolveAmount);
    
    // Ignore input alpha channels and output solid alpha of 1.0
    float alpha = 1.0;
    
    return vec4(clamp(rgb.r, 0.0, 1.0),
                clamp(rgb.g, 0.0, 1.0),
                clamp(rgb.b, 0.0, 1.0),
                alpha);
}
