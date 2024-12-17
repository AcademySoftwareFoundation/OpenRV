//
//  Copyright (c) 2009 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __TwkPaint__Paint__h__
#define __TwkPaint__Paint__h__
#include <half.h>
#include <TwkMath/Vec2.h>
#include <TwkMath/Vec3.h>
#include <TwkMath/Color.h>
#include <iostream>
#include <vector>

//
//  This library should only contain geometric aspects of painting
//  paths/shapes in 2D. Rendering should be done by another
//  library. (e.g TwkGLPaint).
//

namespace TwkPaint
{

    typedef float BasePrecision;

    typedef BasePrecision Scalar;
    typedef TwkMath::Vec2<Scalar> Vec;
    typedef TwkMath::Vec3<Scalar> Vec3;
    typedef TwkMath::Vec3<size_t> IndexTriangle;
    typedef Vec Point;
    typedef TwkMath::Col4f Color;
    typedef std::vector<Point> PointArray;
    typedef std::vector<Vec> VecArray;
    typedef std::vector<Scalar> ScalarArray;
    typedef std::vector<Color> ColorArray;
    typedef std::vector<size_t> IndexArray;
    typedef std::vector<IndexTriangle> IndexTriangleArray;

} // namespace TwkPaint

#endif // __TwkPaint__Paint__h__
