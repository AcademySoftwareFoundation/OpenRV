//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  in0 is the input image, ST is the new mapping. The filter width is
//  derived from the incoming and outgoing ST filter widths.
//

vec4 InlineBoxResize (const in inputImage in0, const in vec2 invScale)
{
    vec2 st = in0.st;

    vec2 doff = invScale - vec2(1.0);
    float l = max(invScale.x, invScale.y);

    if (l > 1.0)
    {
        vec4 c = vec4(0.0);

        // if (length(st) < 30.0) 
        // {
        //     vec4 c = vec4(l - 1.0,
        //                   clamp(l - 2.0, 0.0, 1.0),
        //                   clamp(l - 3.0, 0.0, 1.0), 
        //                   1.0);
        //     return c;
        // }

        if (l > 3.0) doff *= 0.25;

        vec4 c_pm = in0(doff * vec2(1.0, -1.0));
        vec4 c_0m = in0(doff * vec2(0.0, -1.0));
        vec4 c_mm = in0(doff * vec2(-1.0, -1.0));

        vec4 c_p0 = in0(doff * vec2(1.0, 0.0));
        vec4 c_00 = in0(doff * vec2(0.0, 0.0));
        vec4 c_m0 = in0(doff * vec2(-1.0, 0.0));

        vec4 c_pp = in0(doff * vec2(1.0, 1.0));
        vec4 c_0p = in0(doff * vec2(0.0, 1.0));
        vec4 c_mp = in0(doff * vec2(-1.0, 1.0));

        if (l <= 3.0)
        {
            return (c_pm + c_mm + c_pp + c_mp + c_0m + c_p0 + c_m0 + c_0p + c_00) / 9.0;
        }

        vec4 c_2p2m = in0(doff * vec2(2.0, -2.0));
        vec4 c_p2m  = in0(doff * vec2(1.0, -2.0));
        vec4 c_02m  = in0(doff * vec2(0.0, -2.0));
        vec4 c_m2m  = in0(doff * vec2(-1.0, -2.0));
        vec4 c_2m2m = in0(doff * vec2(-2.0, -2.0));

        vec4 c_2pm = in0(doff * vec2(2.0, -1.0));
        vec4 c_2mm = in0(doff * vec2(-2.0, -1.0));

        vec4 c_2p0 = in0(doff * vec2(2.0, 0.0));
        vec4 c_2m0 = in0(doff * vec2(-2.0, 0.0));

        vec4 c_2pp = in0(doff * vec2(2.0, 1.0));
        vec4 c_2mp = in0(doff * vec2(-2.0, 1.0));

        vec4 c_2p2p = in0(doff * vec2(2.0, 2.0));
        vec4 c_p2p  = in0(doff * vec2(1.0, 2.0));
        vec4 c_02p  = in0(doff * vec2(0.0, 2.0));
        vec4 c_m2p  = in0(doff * vec2(-1.0, 2.0));
        vec4 c_2m2p = in0(doff * vec2(-2.0, 2.0));

        return (c_2p2m + c_2p2p + c_2m2p + c_2m2m + c_p2m + c_m2m + c_m2p + c_p2p +
                c_2mm + c_2mp + c_2pm + c_pp + c_02m + c_02p + c_2m0 + c_2p0 +
                c_pm + c_mm + c_pp + c_mp + c_0m + c_p0 + c_m0 + c_0p + c_00) / 25.0;
    }

    return in0();
}
