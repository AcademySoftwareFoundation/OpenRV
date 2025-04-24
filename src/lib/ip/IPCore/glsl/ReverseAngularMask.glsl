//
// Copyright (C) 2025 Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

vec4 ReverseAngularMask(const in inputImage in0, const vec2 pivot, const float angleInRadians, const in fragmentPosition fragPos)
{
    // Compute a point that is 1 unit away from the pivot, on the mask line
    vec2 pointOnLine = vec2(pivot.x + cos(angleInRadians), pivot.y + sin(angleInRadians));

    // Determine if the current fragment is on the "left" side of the mask line or not
    bool leftSide = ((pointOnLine.x - pivot.x) * (fragPos.y - pivot.y) - (pointOnLine.y - pivot.y) * (fragPos.x - pivot.x)) > 0;

    if(leftSide) // masked
    {
        // We output premultiplied fragment colors, so all zeros
        return vec4(0.0, 0.0, 0.0, 0.0);
    }
    else // not masked
    {
        return in0();
    }
}
