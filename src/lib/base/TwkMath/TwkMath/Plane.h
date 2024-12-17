//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkMath__TwkMathPlane__h__
#define __TwkMath__TwkMathPlane__h__
#include <TwkMath/Vec3.h>
#include <TwkMath/Line.h>
#include <TwkMath/Math.h>
#include <algorithm>

namespace TwkMath
{

    //
    //  class Plane<>
    //
    //  Parameterized by scalar type. Interacts with the Line class.
    //

    template <typename T> class Plane
    {
    public:
        typedef Vec3<T> Point;
        typedef Vec3<T> Vec;
        typedef Vec3<T> UnitVec;
        typedef Line<Vec> LineT;

        UnitVec normal; // AKA: Ax + By + Cz + D = 0
        T distance;

        Plane() {}

        Plane(const Point&, const Vec&);
        Plane(const Point&, const Point&, const Point&);

        void set(const Point&, const Vec&);
        void set(Point, Point, Point);

        bool parallel(const LineT&) const;
        bool parallel(const Plane<T>&) const;

        Point reflect(const Point&) const;

        T distanceTo(const Point&) const;

        Point nearestPointTo(const Point&) const;

        //
        //	Check for parallel before calling these
        //

        T intersectionU(const LineT&) const;
        Point intersection(const LineT&) const;
        LineT intersection(const Plane<T>&) const;

    private:
        bool less(const Point& a, const Point& b);
    };

    template <typename T>
    inline bool Plane<T>::less(const Point& a, const Point& b)
    {
        return a.z < b.z
               || (a.z == b.z && (a.y < b.y || (a.y == b.y && a.x < b.x)));
    }

    template <typename T>
    inline void Plane<T>::set(const typename Plane<T>::Point& p,
                              const typename Plane<T>::Vec& v)
    {
        normal = v.normalized();
        distance = dot(p, normal);
    }

    template <typename T>
    inline Plane<T>::Plane(const typename Plane<T>::Point& p,
                           const typename Plane<T>::Vec& v)
    {
        set(p, v);
    }

    template <typename T>
    void Plane<T>::set(typename Plane<T>::Point a, typename Plane<T>::Point b,
                       typename Plane<T>::Point c)
    {
        //
        //  Make sure the same points produce the same plane precisly
        //  regardless of input order.
        //

        bool flip = false;

        if (less(b, a))
        {
            std::swap(a, b);
            flip = !flip;
        }

        if (less(c, a))
        {
            std::swap(a, c);
            flip = !flip;
        }

        if (less(c, b))
        {
            std::swap(b, c);
            flip = !flip;
        }

        typename Plane<T>::Vec v1 = b - a;
        typename Plane<T>::Vec v2 = c - b;

        if (flip)
        {
            normal = cross(v2, v1).normalized();
        }
        else
        {
            normal = cross(v1, v2).normalized();
        }

        distance = dot(a, normal);
    }

    template <typename T>
    inline Plane<T>::Plane(const typename Plane<T>::Point& a,
                           const typename Plane<T>::Point& b,
                           const typename Plane<T>::Point& c)
    {
        set(a, b, c);
    }

    template <typename T>
    inline T Plane<T>::distanceTo(const typename Plane<T>::Point& p) const
    {
        return dot((p - normal * distance), normal);
    }

    template <typename T>
    typename Plane<T>::Point
    Plane<T>::reflect(const typename Plane<T>::Point& p) const
    {
        return -2.0 * distanceTo(p) * normal + p;
    }

    template <typename T>
    inline typename Plane<T>::Point
    Plane<T>::nearestPointTo(const typename Plane<T>::Point& p) const
    {
        return -distanceTo(p) * normal + p;
    }

    template <typename T>
    inline T Plane<T>::intersectionU(const typename Plane<T>::LineT& line) const
    {
        const T d0 = dot(normal, line.direction);
        const T d1 = dot(normal, line.origin);
        return (distance - d1) / d0;
    }

    template <typename T>
    inline bool Plane<T>::parallel(const typename Plane<T>::LineT& line) const
    {
        return dot(normal, line.direction) == T(0);
    }

    template <typename T>
    inline bool Plane<T>::parallel(const Plane<T>& plane) const
    {
        //
        //	There's no good epsilon to test for here. So just assume that
        //	unless the normals are really bit identical, they're not
        //	parallel.
        //

        return normal == plane.normaml;
    }

    template <typename T>
    inline typename Plane<T>::Point
    Plane<T>::intersection(const typename Plane<T>::LineT& line) const
    {
        const T u = intersectionU(line);
        return line(u);
    }

    template <typename T>
    inline typename Plane<T>::LineT
    Plane<T>::intersection(const Plane<T>& plane) const
    {
        const T r = dot(plane.normal, normal);
        const T d = T(-1) / (r * r);
        const T c0 = (distance - r * plane.distance) * d;
        const T c1 = (plane.distance - r * distance) * d;
        return LineT(c0 * normal + c1 * plane.normal,
                     cross(plane.normal, normal));
    }

    template <typename T>
    inline bool operator==(const Plane<T>& a, const Plane<T>& b)
    {
        if (a.normal != b.normal)
            return false;
        const T r = a.distance - b.distance;
        if (Math<T>::abs(r) < Math<T>::epsilon())
            return true;
        return false;
    }

    typedef Plane<float> Planef;
    typedef Plane<double> Planed;

} // namespace TwkMath

#endif // __TwkMath__TwkMathPlane__h__
