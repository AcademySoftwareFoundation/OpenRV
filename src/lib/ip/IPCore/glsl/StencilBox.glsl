//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
// 
// takes a 'stencil box' equivalent
// anything out of the box returns vec4(0)
// boxX has the min and max x value
// boxY has the min and max y value
//
vec4 main (const in inputImage in0,
           const in vec2 boxX,
           const in vec2 boxY)
{
    vec2 sz = in0.size();
    vec2 st = in0.st;
    if (st.x < (boxX[0] * sz.x) ||
        st.x > (boxX[1] * sz.x) || 
        st.y < (boxY[0] * sz.y) ||
        st.y > (boxY[1] * sz.y))
    {
        discard; 
    }
    return in0();
}

