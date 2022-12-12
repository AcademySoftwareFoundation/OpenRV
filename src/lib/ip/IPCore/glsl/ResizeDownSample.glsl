//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
vec4 ResizeDownSample (const in inputImage in0,
                       const in float scale,
                       const in outputImage win)
{
    if (scale <= 1.0)
        return in0();
    else
    {
        vec4 res = vec4(0.0, 0.0, 0.0, 0.0);
        float i, j;
        for (i = 0.0; i < scale; i+=1.0)
            for (j = 0.0; j < scale; j+=1.0)
                res += in0(vec2(j, i));
        res /= scale * scale;
        return res;
    }
}

