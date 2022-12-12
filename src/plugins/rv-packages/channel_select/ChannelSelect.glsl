//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

vec4 ChannelSelect (const in inputImage in0, 
           const in int channel)
{
    vec4 c = in0();

    if      (channel == 0) return vec4(vec3(c.r), c.a);
    else if (channel == 1) return vec4(vec3(c.g), c.a);
    else if (channel == 2) return vec4(vec3(c.b), c.a);
    else if (channel == 3) return vec4(c.a);
    else if (channel == 5) return vec4(vec3(dot(c.rgb, vec3(0.2126, 0.7152, 0.0722))), c.a); // rec709 weights

    return c;
}
