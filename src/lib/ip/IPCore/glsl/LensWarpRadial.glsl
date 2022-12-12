//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
// Adapted from http://en.wikipedia.org/wiki/Distortion_%28optics%29
// or OpenCV http://opencv.willowgarage.com/documentation/camera_calibration_and_3d_reconstruction.html
// x corrected = x( 1 + k1 * r^2 + k2 * r^4 + k3 * r^6)
// y corrected = y( 1 + k1 * r^2 + k2 * r^4 + k3 * r^6)
// 
// distortion centre, offset is specified within range [-1... 1]
//


vec4 LensWarpRadial (const in inputImage in0,
                     const in float k1,
                     const in float k2,
                     const in float k3,
                     const in float d,
                     const in vec2  offset,
                     const in vec2  f)
{
    vec2 cp = offset - in0.st;        // current pixel from center

    vec2 ncp = cp / f;  	      // normalized current pixel

    float rdest = ncp.x * ncp.x + ncp.y * ncp.y; // radius

    // Radial distortion
    float rsrc  =  ((k3 * rdest + k2) * rdest + k1) * rdest + d; // new radius

    vec2 op = cp - ncp * vec2(rsrc) * f;

    return in0(op);
}

