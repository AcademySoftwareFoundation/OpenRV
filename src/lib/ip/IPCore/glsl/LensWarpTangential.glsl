//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
// Adapted from http://en.wikipedia.org/wiki/Distortion_%28optics%29
// or OpenCV http://opencv.willowgarage.com/documentation/camera_calibration_and_3d_reconstruction.html
// x corrected tan = 2p2*x*y + p1(r^2 + 2x^2)]
// y corrected tan = p2*(r^2 + 2*y^2) + 2 * p1 * x * y
//
// distortion centre, offset is specified within range [-1... 1]
//

vec4 LensWarpTangential (const in inputImage in0,
                         const in float p1,
                         const in float p2,
                         const in vec2  offset,
                         const in vec2  f)
{
    vec2 cp = offset - in0.st;        // current pixel from center

    vec2 ncp = cp / f;  	      // normalized current pixel

    float ncp_x2 = ncp.x * ncp.x;
    float ncp_y2 = ncp.y * ncp.y;
    float ncp_xy = ncp.x * ncp.y;

    // Tangential distortion
    float xtan = 2.0 * p2 * (ncp_xy) + p1 * (ncp_y2 + 3.0 * ncp_x2);
    float ytan = p2 * (ncp_x2 + 3.0 * ncp_y2) + 2.0 * p1 * ncp_xy;

    vec2 op = cp - f * (ncp  + vec2(xtan, ytan));

    return in0(op);
}

