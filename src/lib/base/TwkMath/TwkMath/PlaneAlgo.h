//******************************************************************************
// Copyright (c) 2001-2003 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkMathPlaneAlgo_h_
#define _TwkMathPlaneAlgo_h_

#include <TwkMath/Vec3.h>
#include <assert.h>

namespace TwkMath
{

    //******************************************************************************
    namespace
    {

        template <typename T>
        inline bool compare(const Vec3<T>* a, const Vec3<T>* b)
        {
            return (a->z < b->z
                    || (a->z == b->z
                        && (a->y < b->y || (a->y == b->y && a->x < b->x))));
        }

    } // End unnamed namespace

    //******************************************************************************
    // Elevation of a point above a plane defined by three points.
    template <typename T>
    T elevation(const Vec3<T>& v0, const Vec3<T>& v1, const Vec3<T>& v2,
                const Vec3<T>& p)
    {
        bool flip = false;
        const Vec3<T>* vp[3] = {&v0, &v1, &v2};
        if (compare(vp[1], vp[0]))
        {
            std::swap(vp[0], vp[1]);
            flip = !flip;
        }

        if (compare(vp[2], vp[0]))
        {
            std::swap(vp[0], vp[2]);
            flip = !flip;
        }

        if (compare(vp[2], vp[1]))
        {
            std::swap(vp[1], vp[2]);
            flip = !flip;
        }

        assert(!compare(vp[1], vp[0]));
        assert(!compare(vp[2], vp[0]));
        assert(!compare(vp[2], vp[1]));

        Vec3<T> du = (*(vp[1])) - (*(vp[0]));
        Vec3<T> dv = (*(vp[2])) - (*(vp[0]));
        Vec3<T> N = cross(du, dv);
        N.normalize();

        Vec3<T> dp = p - (*(vp[0]));
        return flip ? -dot(dp, N) : dot(dp, N);
    }

    //******************************************************************************
    template <typename T>
    inline bool abovePlane(const Vec3<T>& v0, const Vec3<T>& v1,
                           const Vec3<T>& v2, const Vec3<T>& p)
    {
        return (elevation(v0, v1, v2, p) > ((T)0));
    }

    //******************************************************************************
    template <typename T>
    inline bool aboveOrOnPlane(const Vec3<T>& v0, const Vec3<T>& v1,
                               const Vec3<T>& v2, const Vec3<T>& p)
    {
        return (elevation(v0, v1, v2, p) >= ((T)0));
    }

    //******************************************************************************
    template <typename T>
    inline bool belowPlane(const Vec3<T>& v0, const Vec3<T>& v1,
                           const Vec3<T>& v2, const Vec3<T>& p)
    {
        return (elevation(v0, v1, v2, p) < ((T)0));
    }

    //******************************************************************************
    template <typename T>
    inline bool belowOrOnPlane(const Vec3<T>& v0, const Vec3<T>& v1,
                               const Vec3<T>& v2, const Vec3<T>& p)
    {
        return (elevation(v0, v1, v2, p) <= ((T)0));
    }

    //******************************************************************************
    template <typename T>
    inline bool onPlane(const Vec3<T>& v0, const Vec3<T>& v1, const Vec3<T>& v2,
                        const Vec3<T>& p)
    {
        return (elevation(v0, v1, v2, p) == ((T)0));
    }

} // End namespace TwkMath

#endif
