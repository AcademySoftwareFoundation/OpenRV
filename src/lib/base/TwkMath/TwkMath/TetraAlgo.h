//******************************************************************************
// Copyright (c) 2001-2003 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkMathTetraAlgo_h_
#define _TwkMathTetraAlgo_h_

#include <TwkMath/Vec3.h>
#include <TwkMath/Math.h>
#include <TwkMath/Random.h>
#include <TwkMath/Function.h>
#include <TwkMath/PlaneAlgo.h>
#include <assert.h>

namespace TwkMath
{

    template <typename T>
    inline T volumeOfTetra(const Vec3<T>& va, const Vec3<T>& vb,
                           const Vec3<T>& vc, const Vec3<T>& vd)
    {
        return Math<T>::abs(areaOfTriangle(va, vb, vc)
                            * elevation(va, vb, vc, vd) / T(3));
    }

    template <typename T>
    inline bool insideTetra(const Vec3<T>& a, const Vec3<T>& b,
                            const Vec3<T>& c, const Vec3<T>& d,
                            const Vec3<T>& p)
    {
        return (elevation(d, c, b, p) <= T(0) && elevation(c, d, a, p) <= T(0)
                && elevation(b, a, d, p) <= T(0)
                && elevation(a, b, c, p) <= T(0));
    }

    //
    // From: http://www.acm.org/jgt/papers/RocchiniCignoni00
    //
    // This paper proposes a simple and efficient technique to generate
    // uniformly random points in a tetrahedron. The technique generates
    // random points in a cube and folds the cube into the barycentric
    // space of the tetrahedron in a way that preserves uniformity.
    //

    template <typename T, class RANDGEN>
    Vec3<T> randomPointInTetra(const Vec3<T>& v0, const Vec3<T>& v1,
                               const Vec3<T>& v2, const Vec3<T>& v3,
                               RANDGEN& randGen)
    {
        T s = (T)(randGen.nextFloat());
        T t = (T)(randGen.nextFloat());
        T u = (T)(randGen.nextFloat());
        if (s + t > T(1))
        {
            // Cut and fold the cube into a prism
            s = T(1) - s;
            t = T(1) - t;
        }
        if (t + u > T(1))
        {
            // Cut and fold the prism into a tetrahedron
            T tmp = u;
            u = T(1) - s - t;
            t = T(1) - tmp;
        }
        else if (s + t + u > T(1))
        {
            T tmp = u;
            u = s + t + u - T(1);
            s = T(1) - t - tmp;
        }

        // a, s, t, u are the barycentric coordinates
        // of the random point.
        T a = T(1) - s - t - u;

        // Return
        // Vec3<T> ret = v0*a + v1*s + v2*t + v3*u;
        Vec3<T> ret =
            v0 + ((T)0.999) * (s * (v1 - v0) + t * (v2 - v0) + u * (v3 - v0));

        // CJH: too restrictive. Roundoff error tends
        // to make this fail, even for valid points.
        // assert( insideTetra( v0, v1, v2, v3, ret ) );
        return ret;
    }

} // End namespace TwkMath

#endif
