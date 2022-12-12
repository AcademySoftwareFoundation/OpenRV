//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
// See publication
// Lens Distortion in 3DE4, Science-D-Visions December 11, 2013
// for description of model.
//
// Matches plugin 'tde4_ldp_anamorphic_deg_6' 
//


vec4 LensWarp3DE4AnamorphicDegree6 (
	const in inputImage in0,
	const in vec2 c02,
	const in vec2 c22,
	const in vec2 c04,
	const in vec2 c24,
	const in vec2 c44,
	const in vec2 c06,
	const in vec2 c26,
	const in vec2 c46,
	const in vec2 c66,
	const in vec2 offset,
	const in vec2 f)
{
    vec2 cp = offset - in0.st;        // current pixel from center

    vec2 ncp = cp / f;  	      // normalized current pixel

    vec2 rdest = vec2(ncp.x * ncp.x + ncp.y * ncp.y); // sqr radius

    float phi = atan(ncp.y, ncp.x);
    vec2 cos2phi = vec2(cos(2.0 * phi));
    vec2 cos4phi = vec2(cos(4.0 * phi));
    vec2 cos6phi = vec2(cos(6.0 * phi));

    // Radial distortion
    vec2 rsrc = 1.0 + (((c06 + c26*cos2phi + c46*cos4phi + c66*cos6phi) * rdest 
                    +   (c04 + c24*cos2phi + c44*cos4phi)) * rdest 
		    +   (c02 + c22*cos2phi)) * rdest;

    vec2 op = cp - ncp * rsrc * f;

    return in0(op);
}

