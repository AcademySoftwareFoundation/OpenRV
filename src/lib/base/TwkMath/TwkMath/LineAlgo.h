//******************************************************************************
// Copyright (c) 2001-2003 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkMathLineAlgo_h_
#define _TwkMathLineAlgo_h_

#include <TwkMath/Vec2.h>
#include <algorithm>

namespace TwkMath
{

    //******************************************************************************
    // Function to see if two line segments intersect.
    // Line segments a-b, c-d
    // This function is exact.
    template <typename T>
    bool segmentsIntersect(const Vec2<T>& a, const Vec2<T>& b, const Vec2<T>& c,
                           const Vec2<T>& d)
    {
        // Make swappable
        const Vec2<T>* p[4] = {&a, &b, &c, &d};

        // make sure a & b are sorted
        if (b.y < a.y || (b.y == a.y && b.x < a.x))
        {
            std::swap(p[0], p[1]);
        }

        // make sure c & d are sorted
        if (d.y < c.y || (d.y == c.y && d.x < c.x))
        {
            std::swap(p[2], p[3]);
        }

        // Alias
        const Vec2<T>& A = *(p[0]);
        const Vec2<T>& B = *(p[1]);
        const Vec2<T>& C = *(p[2]);
        const Vec2<T>& D = *(p[3]);

        // Okay, points are sorted properly.
        T denom = ((B.x - A.x) * (D.y - C.y) - (B.y - A.y) * (D.x - C.x));
        if (denom == ((T)0))
        {
            return false;
        }

        T r = ((A.y - C.y) * (D.x - C.x) - (A.x - C.x) * (D.y - C.y)) / denom;
        if (r < ((T)0) || r > ((T)1))
        {
            return false;
        }

        T s = ((A.y - C.y) * (B.x - A.x) - (A.x - C.x) * (B.y - A.y)) / denom;
        if (s < ((T)0) || s > ((T)1))
        {
            return false;
        }

        return true;
    }

    //******************************************************************************
    // does a line cast from point P in the X direction hit the line segment
    // defined by A-B? This function is exact - will always return the same
    // answer for the same inputs in any order.
    template <typename T>
    bool testPointVsSegment(const Vec2<T>& a, const Vec2<T>& b,
                            const Vec2<T>& P)
    {
        const Vec2<T>* p[2] = {&a, &b};

        if (b.y < a.y || (b.y == a.y && b.x < a.x))
        {
            std::swap(p[0], p[1]);
        }

        const Vec2<T>& A = *(p[0]);
        const Vec2<T>& B = *(p[1]);

        bool say = A.y >= P.y;
        bool sby = B.y >= P.y;

        if (say == sby)
        {
            return false;
        }
        else
        {
            bool sax = A.x > P.x;
            bool sbx = B.x > P.x;
            if (sax && sbx)
            {
                return true;
            }
            else if (sax || sbx)
            {
                T xint = A.x + ((B.x - A.x) * (P.y - A.y) / (B.y - A.y));
                return xint >= P.x;
            }
            else
            {
                return false;
            }
        }

        return false;
    }

} // End namespace TwkMath

#endif
