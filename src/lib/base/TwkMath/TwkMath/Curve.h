//******************************************************************************
// Copyright (c) 2001-2002 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkMathCurve_h_
#define _TwkMathCurve_h_

#include <TwkMath/Math.h>
#include <TwkMath/Function.h>
#include <TwkMath/Vec2.h>
#include <TwkMath/Vec3.h>
#include <assert.h>

namespace TwkMath
{

    //******************************************************************************
    // The math behind a hermite curve is this:
    // You have a curve which you assume to be of the form
    // v(t) = c3 * t^3 + c2 * t^2 + c1 * t + c0
    // it's derivative (slope) is then:
    // m(t) = 3 c3 t^2 + 2 c2 t + c1
    // You start with four curve values vn1, v0, v1 & v2 which represent
    // v(-1), v(0), v(1) & v(2) respectively.
    // From these you infer m(0) = m0 = ( v1 - vn1 ) / 2
    //                      m(1) = m1 = ( v2 - v0 ) / 2
    // You now have four unknowns ( c0, c1, c2 & c3 )
    // and four equations:
    // v(0) = c0 = v0;
    // v(1) = c3 + c2 + c1 + c0 = v1;
    // m(0) = c1 = m0;
    // m(1) = 3 c3 + 2 c2 + c1 = m1;
    // Solve these equations for c0, c1, c2 & c3 to get below
    template <class VAL, class INTERP>
    inline VAL hermite(const VAL& vn1, const VAL& v0, const VAL& v1,
                       const VAL& v2, const INTERP& t)
    {
        static const INTERP i2 = (INTERP)2;
        static const INTERP i3 = (INTERP)3;

        const VAL m0 = (v1 - vn1) / i2;
        const VAL m1 = (v2 - v0) / i2;

        const VAL c0 = v0;
        const VAL c1 = m0;

        const VAL c2 = (i3 * (v1 - c0)) - (i2 * c1) - m1;
        const VAL c3 = v1 - c2 - c1 - c0;

        return ((c3 * t + c2) * t + c1) * t + c0;
    }

    //******************************************************************************
    // A version of hermite for only three points at t values 0, 1, 2
    template <class VAL, class INTERP>
    inline VAL hermiteBegin(const VAL& v0, const VAL& v1, const VAL& v2,
                            const INTERP& t)
    {
        static const INTERP i2 = (INTERP)2;
        static const INTERP i3 = (INTERP)3;

        const VAL m0 = (v1 - v0);
        const VAL m1 = (v2 - v0) / i2;

        const VAL c0 = v0;
        const VAL c1 = m0;

        const VAL c2 = (i3 * (v1 - c0)) - (i2 * c1) - m1;
        const VAL c3 = v1 - c2 - c1 - c0;

        return ((c3 * t + c2) * t + c1) * t + c0;
    }

    //******************************************************************************
    // A version of hermite for only three points at t values -1, 0, 1
    template <class VAL, class INTERP>
    inline VAL hermiteEnd(const VAL& vn1, const VAL& v0, const VAL& v1,
                          const INTERP& t)
    {
        static const INTERP i2 = (INTERP)2;
        static const INTERP i3 = (INTERP)3;

        const VAL m0 = (v1 - vn1) / i2;
        const VAL m1 = (v1 - v0);

        const VAL c0 = v0;
        const VAL c1 = m0;

        const VAL c2 = (i3 * (v1 - c0)) - (i2 * c1) - m1;
        const VAL c3 = v1 - c2 - c1 - c0;

        return ((c3 * t + c2) * t + c1) * t + c0;
    }

    //******************************************************************************
    // Treat a list of points as a piecewise hermite curve
    template <class VAL, class INTERP>
    VAL hermite(const VAL* vals, int numVals, const INTERP& t)
    {
        if (numVals == 1)
        {
            return vals[0];
        }
        else if (numVals == 2)
        {
            return lerp(vals[0], vals[1], t);
        }
        else if (t <= (INTERP)1)
        {
            return hermiteBegin(vals[0], vals[1], vals[2], t);
        }
        else if (t >= ((INTERP)(numVals - 2)))
        {
            return hermiteEnd(vals[numVals - 3], vals[numVals - 2],
                              vals[numVals - 1], t - (numVals - 2));
        }
        else
        {
            int i = Math<INTERP>::floor(t);
            return hermite(vals[i - 1], vals[i], vals[i + 1], vals[i + 2],
                           t - (INTERP)i);
        }
    }

    template <class T>
    Vec2<T> bezierCurve2D(const Vec2<T>& p0, const Vec2<T>& p1,
                          const Vec2<T>& p2, const Vec2<T>& p3, const T t)
    {
        const T oneMinusT = T(1) - t;
        const T oneMinusT2 = oneMinusT * oneMinusT;
        const T oneMinusT3 = oneMinusT2 * oneMinusT;
        const T t2 = t * t;
        const T t3 = t2 * t;

        return p0 * oneMinusT3 + (T(3) * oneMinusT2 * t) * p1
               + (T(3) * oneMinusT * t2) * p2 + t3 * p3;
    }

    template <class T>
    Vec2<T> bezierSamplingTangent2D(const Vec2<T>& p0, const Vec2<T>& p1,
                                    const Vec2<T>& p2)
    {
        const Vec2<T> s0 = p1 - p0;
        const Vec2<T> s1 = p1 - p2;
        const T m0 = magnitude(s0);
        const T m1 = magnitude(s1);
        const Vec2<T> n0 = s0 / m0;
        const Vec2<T> n1 = s1 / m1;
        const T l = std::min(m0, m1) / T(2);

        if (Math<T>::abs(T(1) - dot(n0, -n1)) <= Math<T>::epsilon())
        {
            //
            //  Co-linear
            //

            return n0 * l;
        }
        else
        {
            const Vec2<T> norm = normalize(n0 + n1);
            const Vec3<T> s03 = Vec3<T>(s0.x, s0.y, 0);
            const Vec3<T> s13 = Vec3<T>(s1.x, s1.y, 0);
            const T z = cross(s03, s13).z > T(0) ? T(1) : T(-1);
            const Vec3<T> c =
                cross(Vec3<T>(0, 0, z), Vec3<T>(norm.x, norm.y, 0));

            return -Vec2<T>(c.x, c.y) * l;
        }
    }

} // End namespace TwkMath

#endif
