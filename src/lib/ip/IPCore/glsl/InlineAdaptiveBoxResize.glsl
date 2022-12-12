//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
#if __VERSION__ < 150
vec4 InlineAdaptiveBoxResize (const in inputImage in0)
{
    return in0();
}
#else

vec4 InlineAdaptiveBoxResize (const in inputImage in0)
{
    vec2 st = in0.st;
    vec2 d = fwidth(st);
    float l = max(d.x, d.y);
    const float pi = 3.1415926;
    const float pi2 = pi * pi;

//    const float onesixth = 1.0 / 6;
    if (l <= 0.5) 
    {
        const float B = 1.0 / 3;
        const float C = B;
        //mitchell
        //just for visualization purposes
        if (length(st) < 30.0) 
        {
            vec4 c = vec4(clamp(1 - l * 4.0, 0.0, 1.0),
                          clamp(1 - l * 3.0, 0.0, 1.0),
                          clamp(1 - l * 2.0, 0.0, 1.0), 
                          1.0);
            return c;
        }

        // [-2, 2] kernel
        float totalweight = 0;
        float weight = 0;
        vec4 sum = vec4(0.0);
        float step = 0.5;
        for (float i = -1.5; i <= 1.5; i+= step) {
            float lx = 0;
            float absi = abs(i);
            if (absi < 2) {
                if (absi < 1)
                    lx = (12 - 9 * B - 6 * C) * absi * absi * absi 
                        + (-18 + 12 * B + 6 * C) * absi * absi
                        + 6 - 2 * B;
                else
                    lx = (-B - 6 * C) * absi * absi * absi 
                        + (6 * B + 30 * C) * absi * absi
                        + (-12 * B - 48 * C) * absi
                        + 8 * B + 24 * C;
            }
            for (float j = -1.5; j <= 1.5; j+= step) {
                float ly = 0;
                float absj = abs(j);
                if (absj < 2) {
                    if (absj < 1)
                        ly = (12 - 9 * B - 6 * C) * absj * absj * absj 
                            + (-18 + 12 * B + 6 * C) * absj * absj
                            + 6 - 2 * B;
                    else
                        ly = (-B - 6 * C) * absj * absj * absj 
                            + (6 * B + 30 * C) * absj * absj
                            + (-12 * B - 48 * C) * absj
                            + 8 * B + 24 * C;   
                }
                weight = lx * ly;
                vec4 c = in0(vec2(j, i));               
                sum = sum + c * weight;
                totalweight += weight; 

            }
        }
        return sum / totalweight;            

    }
    if (l >= 2.0)
    {
        vec4 c = vec4(0.0);
        //just for visualization purposes
        if (length(st) < 30.0) 
        {
            vec4 c = vec4(clamp(l - 4.0, 0.0, 1.0),
                          clamp(l - 3.0, 0.0, 1.0),
                          clamp(l - 2.0, 0.0, 1.0), 
                          1.0);
            return c;
        }

        float halfl = l * 0.5;
        float step = 1.0 / l; //step is defined as the inverse of
        //the number of gaps for the filter
        //kernel size 
        float totalweight = 0;
        float weight = 0;
        vec4 sum = vec4(0.0);
        float a = int(halfl);
        float ainverse = 1.0 / a;

        for (float i = -halfl + step; i < halfl; i+= step) {
            float lx = 1; 
            if (i != 0)
                lx = a * sin (pi * i) * sin (pi * i * ainverse) / pi2 / i / i;
            for (float j = -halfl + step; j < halfl; j+=step) {
                float ly = 1;
                if (j != 0)
                    ly = a * sin (pi * j) * sin (pi * j * ainverse) / pi2 / j / j;
                weight = lx * ly;
                vec4 c = in0(vec2(j, i));               
                sum = sum + c * weight;
                totalweight += weight; 

            }
        }
        return sum / totalweight;
    }
    //todo
    return in0();
}

#endif
