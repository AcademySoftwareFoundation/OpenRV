//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkMath__TwkMathLine__h__
#define __TwkMath__TwkMathLine__h__
#include <TwkMath/Vec3.h>
#include <TwkMath/Vec2.h>
#include <TwkMath/Box.h>
#include <TwkMath/Math.h>

namespace TwkMath
{

    //
    //  class Line<>
    //
    //  Parameterized by a vector (mathematical) type. The line is
    //  parameterized do that u is in the units of the origin and direction of
    //  passed in.
    //

    template <class T> class Line
    {
    public:
        typedef typename T::value_type value_type;
        typedef typename T::value_type Scalar;
        typedef T Point;
        typedef T Dir;

        Dir direction;
        Point origin;

        Line()
            : direction(0)
            , origin(0)
        {
        }

        Line(const Line<T>& l)
            : direction(l.direction)
            , origin(l.origin)
        {
        }

        Line(const Point&, const Dir&);

        Point nearestPointTo(const Point&) const;
        Point nearestPointTo(const Line<T>&) const;
        Scalar nearestUTo(const Line<T>&) const;
        Scalar nearestUTo(const Point&) const;

        bool isParallelTo(const Line<T>&) const;

        Point operator()(float) const;
    };

    template <class T>
    inline Line<T>::Line(const typename Line<T>::Point& p1,
                         const typename Line<T>::Dir& d)
    {
        origin = p1;
        direction = d.normalized();
    }

    template <class T> inline T Line<T>::operator()(float u) const
    {
        return direction * u + origin;
    }

    template <class T>
    inline bool Line<T>::isParallelTo(const Line<T>& line) const
    {
        return Math<Scalar>::abs(dot(line.direction, direction)) == Scalar(1);
    }

    template <class T>
    inline typename T::value_type
    Line<T>::nearestUTo(const typename Line<T>::Point& p) const
    {
        return dot(p - origin, direction);
    }

    template <class T>
    inline T Line<T>::nearestPointTo(const typename Line<T>::Point& p) const
    {
        return nearestUTo(p) * direction + origin;
    }

    template <class T>
    inline typename T::value_type Line<T>::nearestUTo(const Line<T>& line) const
    {
        //
        //	Same algorithm as in ImathLine.h
        //

        T posLpos = origin - line.origin;
        Scalar c = dot(direction, posLpos);
        Scalar a = dot(line.direction, direction);
        Scalar f = dot(line.direction, posLpos);
        Scalar num = c - a * f;
        Scalar denom = a * a - 1;
        Scalar absDenom = ((denom >= 0) ? denom : -denom);

        if (absDenom < 1)
        {
            Scalar absNum = ((num >= 0) ? num : -num);

            if (absNum >= absDenom * Math<Scalar>::max())
            {
                return Scalar(0);
            }
        }

        return (num / denom);
    }

    template <class T>
    inline T Line<T>::nearestPointTo(const Line<T>& line) const
    {
        return origin + direction * nearestUTo(line);
    }

    typedef Line<Vec3f> Line3f;
    typedef Line<Vec2f> Line2f;

} // namespace TwkMath

#endif // __TwkMath__TwkMathLine__h__
