//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

vec4 ColorOutOfRange (const in vec4 P)
{
    const vec3 grey = vec3(0.5);

    vec3 c = P.rgb;
    vec3 ok1 = vec3(greaterThan(c, vec3(0.0)));
    vec3 big = vec3(greaterThanEqual(c, vec3(1.0)));
    return vec4( grey * ok1 + grey * big, P.a );
}
