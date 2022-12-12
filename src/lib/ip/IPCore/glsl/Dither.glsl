//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
// dither shader from pixar 

// for uniformly random input p generate uniform random number on [-1.0,1.0] 
#if __VERSION__ < 150

float rnd(vec2 p)
{ 
    return 1.0 - 2.0*fract(sin(dot(p.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

#else
float rnd(vec2 p)
{ 
    int n = int(p.x * 40.0 + p.y * 6400.0); 
    n = (n << 13) ^ n; 
    return 1.0 - float( (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0; 
}

#endif

vec4 main(const in inputImage in0,
          const in outputImage win,
          const in float scale, // 2^bits - 1, so for 8 bit disp scale == 255.0
          const in float seed)
{ 
    vec2 coord = in0.st / in0.size();

    vec4 texCol = in0();

    vec3 c = texCol.rgb; 
    float a = texCol.a;

    // We need a 2D coordinate to compute a spatial dither. 
    // 
    // If we use only the screen coordinate gl_FragCoord, 
    // we get a result that sticks to the screen and looks like 
    // a dirty window or screen door. 
    // 
    // If we use only the texture coordinate, we get a result 
    // that sticks to the image but looks blocky as we zoom in. 
    // 
    // To ameliorate these unwanted artifacts we combine both 
    // screen and texture coordinates in our computation. 
    // We also scale the texture coordinate to avoid streaking 
    // or blocky artifacts for zoomed-in textures. 
    // 
    // Furthermore, since rnd() is stateless in order to take 
    // advantage of parallel computation (and is really just a 
    // hash function), we use slightly different perturbations 
    // for each of R, G, and B components to effectively make 
    // them randomly different from each other.
    //
    //  UPDATE: the dither shading happens at the end of the display pipeline
    //  where the render target is the same size as the view, so there is no
    //  point in using texture coords _and_ frag coords.  We also want the
    //  dither pattern to change when (1) the view transform changes or (2) the
    //  frame changes or (3) ?, so we accept a "seed" from the calling code
    //  that should incorporate all these factors.  The seed should vary
    //  between 0 and 1.

    vec2 pr = (0.9 + 0.1 * seed) * coord.xy * 1000.1; 
    vec2 pg = (0.9 + 0.1 * seed) * coord.xy * 1000.2; 
    vec2 pb = (0.9 + 0.1 * seed) * coord.xy * 1000.3;

    // Dither to N bits since driver doesn't seem to be doing this correctly. 

    // Note that we don't actually quantize here. Instead, we assume that 
    // the last driver step will be to round to the nearest N bits, so we 
    // simply add the dither bias centered around 0.0 and scaled to 2^N-1.

    return vec4(c.rgb + vec3(rnd(pr), rnd(pg), rnd(pb)) * vec3(0.5) / vec3(scale), a);
}
