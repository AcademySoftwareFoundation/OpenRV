//******************************************************************************
// Copyright (c) 2001-2003 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkMathBoxAlgo_h_
#define _TwkMathBoxAlgo_h_

#include <TwkMath/Box.h>
#include <TwkMath/Mat44.h>
#include <algorithm>
#include <TwkMath/Exc.h>

namespace TwkMath
{

    //******************************************************************************
    template <typename T>
    Box<Vec3<T>> transform(const Mat44<T>& M, const Box<Vec3<T>>& box)
    {
        Box<Vec3<T>> newBox;
        typedef Vec3<T> Vec;

        if (box.isInfinite())
        {
            return box;
        }

        if (!box.isEmpty())
        {
            if (M.isAffine())
            {
                for (size_t i = 0; i < 3; ++i)
                {
                    const T t = M(i, 3);
                    newBox.min[i] = t;
                    newBox.max[i] = t;

                    for (size_t q = 0; q < 3; ++q)
                    {
                        const T miq = M(i, q);
                        const T a = miq * box.min[q];
                        const T b = miq * box.max[q];

                        if (a < b)
                        {
                            newBox.min[i] += a;
                            newBox.max[i] += b;
                        }
                        else
                        {
                            newBox.min[i] += b;
                            newBox.max[i] += a;
                        }
                    }
                }
            }
            else
            {
                //	General brute-force transform
                newBox.extendBy(M * Vec(box.min.x, box.min.y, box.min.z));
                newBox.extendBy(M * Vec(box.min.x, box.min.y, box.max.z));
                newBox.extendBy(M * Vec(box.min.x, box.max.y, box.min.z));
                newBox.extendBy(M * Vec(box.min.x, box.max.y, box.max.z));
                newBox.extendBy(M * Vec(box.max.x, box.min.y, box.min.z));
                newBox.extendBy(M * Vec(box.max.x, box.min.y, box.max.z));
                newBox.extendBy(M * Vec(box.max.x, box.max.y, box.min.z));
                newBox.extendBy(M * Vec(box.max.x, box.max.y, box.max.z));
            }
        }

        return newBox;
    }

    //******************************************************************************
    template <typename T>
    inline Box<Vec3<T>> operator*(const Mat44<T>& M, const Box<Vec3<T>>& box)
    {
        return transform(M, box);
    }

    //******************************************************************************
    // Util stuff for functions below
    namespace
    {

        enum Quadrant
        {
            LEFT,
            MIDDLE,
            RIGHT
        };

    } // End anonymous namespace

    //******************************************************************************
    // Fast Ray-Box Intersection
    // by Andrew Woo
    // from "Graphics Gems", Academic Press, 1990
    template <class VEC, class T>
    bool boxRayIntersection(const TwkMath::Box<VEC>& box, const VEC& origin,
                            const VEC& dir, T& resultT, VEC& result)
    {
        static const size_t DIM = VEC::dimension();

        const VEC& minB = box.min;
        const VEC& maxB = box.max;

        bool inside = true;
        Quadrant quadrant[DIM];
        size_t whichPlane;
        T maxT[DIM];
        T candidatePlane[DIM];

        // Find candidate planes; this loop can be avoided if
        // rays cast all from the eye(assume perpsective view)
        for (size_t i = 0; i < DIM; ++i)
        {
            if (origin[i] < minB[i])
            {
                quadrant[i] = LEFT;
                candidatePlane[i] = minB[i];
                inside = false;
            }
            else if (origin[i] > maxB[i])
            {
                quadrant[i] = RIGHT;
                candidatePlane[i] = maxB[i];
                inside = false;
            }
            else
            {
                quadrant[i] = MIDDLE;
            }
        }

        // Ray origin inside bounding box
        if (inside)
        {
            result = origin;
            resultT = ((T)0);
            return true;
        }

        // Calculate T distances to candidate planes
        for (size_t i = 0; i < DIM; ++i)
        {
            if (quadrant[i] != MIDDLE && dir[i] != ((T)0))
            {
                maxT[i] = (candidatePlane[i] - origin[i]) / dir[i];
            }
            else
            {
                maxT[i] = ((T)-1);
            }
        }

        // Get largest of the maxT's for final choice of intersection
        // Loops from 1 because whichPlane is initialized to zero.
        whichPlane = 0;
        for (size_t i = 1; i < DIM; ++i)
        {
            if (maxT[whichPlane] < maxT[i])
            {
                whichPlane = i;
            }
        }

        // Check final candidate actually inside box
        if (maxT[whichPlane] < ((T)0))
        {
            return false;
        }

        for (size_t i = 0; i < DIM; ++i)
        {
            if (whichPlane != i)
            {
                result[i] = origin[i] + (maxT[whichPlane] * dir[i]);
                if (result[i] < minB[i] || result[i] > maxB[i])
                {
                    return false;
                }
            }
            else
            {
                result[i] = candidatePlane[i];
            }
        }

        // Ray hits box.
        resultT = maxT[whichPlane];
        return true;
    }

    //******************************************************************************
    // Intersection vs unterminated ray without result
    template <class VEC>
    inline bool boxRayIntersect(const Box<VEC>& box, const VEC& origin,
                                const VEC& dir)
    {
        static VEC dummy;
        static typename VEC::value_type dummyT;
        return boxRayIntersection(box, origin, dir, dummyT, dummy);
    }

    //******************************************************************************
    // Intersection of a box with a terminated ray
    // It's from Andrew Woo's Graphics Gem.
    template <class VEC, typename T>
    inline bool boxSegmentIntersection(const Box<VEC>& box, const VEC& origin,
                                       const VEC& dir, const T& tMin,
                                       const T& tMax, T& resultT, VEC& result)
    {
        static VEC dummy;
        static T dummyT;
        if (!boxRayIntersection(box, origin, dir, dummyT, dummy))
        {
            return false;
        }
        if (dummyT < tMin || dummyT > tMax)
        {
            return false;
        }
        resultT = dummyT;
        result = dummy;
        return true;
    }

    //******************************************************************************
    // Intersection of a box with a terminated ray
    // It's from Andrew Woo's Graphics Gem.
    template <class VEC, class T>
    inline bool boxSegmentIntersect(const Box<VEC>& box, const VEC& origin,
                                    const VEC& dir, const T& tMin,
                                    const T& tMax)
    {
        static VEC dummy;
        static T dummyT;
        return boxSegmentIntersection(box, origin, dir, tMin, tMax, dummyT,
                                      dummy);
    }

} // End namespace TwkMath

#endif // _TwkMathBoxAlgo_h_
