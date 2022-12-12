//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  This is a simple test of the 

use math;
use math_util;

Point       := vector float[3];
Vec         := vector float[3];
Color       := vector float[3];
dt          := 0.041666666666;
ColorTemp   := (float, Color);

global ColorTemp[] bbColors = 
{
    { 0.0,   Color(0, 0, 0) },
    { 0.37,  Color(0.937, 0, 0) },
    { 0.54,  Color(1.0, 0.392, 0) },
    { 0.705, Color(1.0, 0.639, 0.146) },
    { 0.87,  Color(1.0, 0.835, 0.612) },
    { 1.0,   Color(1.0, 0.981, 0.943) },
    { 1.0,   Color(1.0, 0.981, 0.943) }
};

\: blackBodyColor (Color; float u)
{
    Color rcol;

    for (int i=0, s=bbColors.size() - 1; i < s; i++)
    {
        let bb1 = bbColors[i+1],
            u1  = bb1._0;

        if (u <= u1)
        {
            let bb0 = bbColors[i],
                u0  = bb0._0,
                c0  = bb0._1,
                c1  = bb1._1;

            return lerp(c0, c1, linstep(u0, u1, u));
        }
    }

    return Color(0);
}

blackBodyColor(.5);
