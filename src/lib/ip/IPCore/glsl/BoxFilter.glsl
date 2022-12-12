//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

//filter size is (2*size+1) by (2*size+1)

vec4 BoxFilter (const in inputImage in0, const in float size)
{
    if (size == 0.0) return in0();
    
    vec2 st = in0.st;
    vec4 outColor = vec4(0.0, 0.0, 0.0, 0.0);
    
    float i, j;
    for (i = -size; i <= size; i+=1.0)
    {
        for (j = -size; j <= size; j+=1.0)
        {
            outColor += in0(vec2(j, i));
        }
    }
    
    outColor /= (2.0 * size + 1.0) * (2.0 * size + 1.0);

    return outColor;
}
