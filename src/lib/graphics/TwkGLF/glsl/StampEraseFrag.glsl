//
// Copyright (C) 2026  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#extension GL_ARB_texture_rectangle : require

uniform sampler2D brushTip;
uniform float hardness;
uniform sampler2DRect original;

IN_FRAG vec2 TexCoord0;
IN_FRAG float Opacity;

void main()
{
    float pattern = SAMPLE2D(brushTip, TexCoord0).r;

    vec2 m = TexCoord0 - vec2(0.5);
    float dist = length(m) * 2.0;
    float aa = fwidth(dist);

    float band = max(1.0 - hardness * 0.99, 1.5 * aa);
    float cutoff = 1.0 - band;
    float u = clamp((dist - cutoff) / band, 0.0, 1.0);

    float edge = 1.0 - smoothstep(1.0 - aa, 1.0, dist);
    float radial = exp(-0.5 * (3.5 * u) * (3.5 * u)) * edge;

    float alpha = pattern * radial * Opacity;
    FRAG_COLOR = vec4(texture2DRect(original, gl_FragCoord.xy).rgb, alpha);
}
