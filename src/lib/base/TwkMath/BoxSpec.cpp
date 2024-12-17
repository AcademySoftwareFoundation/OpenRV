//******************************************************************************
// Copyright (c) 2001-2003 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkMath/Box.h>

namespace TwkMath
{

    //******************************************************************************
    //******************************************************************************
    // VEC2I
    //******************************************************************************
    //******************************************************************************

    //******************************************************************************
    template <> Vec2i Box<Vec2i>::size() const
    {
        if (isEmpty())
        {
            return Vec2i(0);
        }
        else if (isInfinite())
        {
            // This is not correct, but
            // necessary to avoid overflow.
            // In normal use cases is should be close enough.
            return Vec2i(Math<int>::max());
        }
        else
        {
            return max - min + 1;
        }
    }

    //******************************************************************************
    template <> int Box<Vec2i>::size(Box<Vec2i>::size_type i) const
    {
        if (min[i] > max[i])
        {
            // Empty
            return 0;
        }
        else if (min[i] == Math<int>::min() && max[i] == Math<int>::max())
        {
            // infinite
            return Math<int>::max();
        }
        else
        {
            return max[i] - min[i] + 1;
        }
    }

    //******************************************************************************
    template <> void Box<Vec2i>::extendBy(const Vec2i& v)
    {
        if (isInfinite())
        {
            return;
        }
        else if (isEmpty())
        {
            min = max = v;
        }
        else
        {
            if (v.x < min.x)
            {
                min.x = v.x;
            }
            else if (v.x > max.x)
            {
                max.x = v.x;
            }

            if (v.y < min.y)
            {
                min.y = v.y;
            }
            else if (v.y > max.y)
            {
                max.y = v.y;
            }
        }
    }

    //******************************************************************************
    template <> void Box<Vec2i>::extendBy(const Box<Vec2i>& b)
    {
        if (b.isEmpty() || isInfinite())
        {
            return;
        }
        else if (b.isInfinite())
        {
            makeInfinite();
            return;
        }
        else if (isEmpty())
        {
            min = b.min;
            max = b.max;
            return;
        }
        else
        {
            if (b.min.x < min.x)
            {
                min.x = b.min.x;
            }
            else if (b.max.x > max.x)
            {
                max.x = b.max.x;
            }

            if (b.min.y < min.y)
            {
                min.y = b.min.y;
            }
            else if (b.max.y > max.y)
            {
                max.y = b.max.y;
            }
        }
    }

    //******************************************************************************
    template <> bool Box<Vec2i>::intersects(const Vec2i& p, int radius) const
    {
        //	Ripped off from graphics gems with improvements
        int d = 0;

        // X
        if (p.x < min.x)
        {
            int s = p.x - min.x;
            d += s * s;
        }
        else if (p.x > max.x)
        {
            int s = p.x - max.x;
            d += s * s;
        }

        // Y
        if (p.y < min.y)
        {
            int s = p.y - min.y;
            d += s * s;
        }
        else if (p.y > max.y)
        {
            int s = p.y - max.y;
            d += s * s;
        }

        return d <= (radius * radius);
    }

    //******************************************************************************
    template <>
    bool Box<Vec2i>::intersects(const Vec2i& origin, const Vec2i& dir) const
    {
        // Ripped off of GGEMS
        static const int LEFT = -1;
        static const int MIDDLE = 0;
        static const int RIGHT = 1;

        bool inside = true;
        Vec2i quadrant;
        Vec2i maxT;
        Vec2i candidatePlane;

        // X
        if (origin.x < min.x)
        {
            quadrant.x = LEFT;
            candidatePlane.x = min.x;
            inside = false;
        }
        else if (origin.x > max.x)
        {
            quadrant.x = RIGHT;
            candidatePlane.x = max.x;
            inside = false;
        }
        else
        {
            quadrant.x = MIDDLE;
        }

        // Y
        if (origin.y < min.y)
        {
            quadrant.y = LEFT;
            candidatePlane.y = min.y;
            inside = false;
        }
        else if (origin.y > max.y)
        {
            quadrant.y = RIGHT;
            candidatePlane.y = max.y;
            inside = false;
        }
        else
        {
            quadrant.y = MIDDLE;
        }

        if (inside)
        {
            return true;
        }

        // X
        if (quadrant.x != MIDDLE && dir.x != 0)
        {
            maxT.x = (candidatePlane.x - origin.x) / dir.x;
        }
        else
        {
            maxT.x = -1;
        }

        // Y
        if (quadrant.y != MIDDLE && dir.y != 0)
        {
            maxT.y = (candidatePlane.y - origin.y) / dir.y;
        }
        else
        {
            maxT.y = -1;
        }

        int whichPlane = maxT.x > maxT.y ? 0 : 1;

        if (maxT[whichPlane] < 0)
        {
            return false;
        }

        // X
        if (whichPlane != 0)
        {
            int x = origin.x + maxT.y * dir.x;

            if (x < min.x || x > max.x)
            {
                return false;
            }
        }
        else
        {
            int y = origin.y + maxT.x * dir.y;

            if (y < min.y || y > max.y)
            {
                return false;
            }
        }

        return true;
    }

    //******************************************************************************
    template <> bool Box<Vec2i>::intersects(const Box<Vec2i>& b) const
    {
        // DO need to check empty or infinity here,
        // otherwise empty.intersects.infinity = true.
        if (isEmpty() || b.isEmpty())
        {
            return false;
        }
        else if (isInfinite() || b.isInfinite())
        {
            return true;
        }
        else
        {
            if (b.max.x < min.x || b.min.x > max.x)
            {
                return false;
            }
            if (b.max.y < min.y || b.min.y > max.y)
            {
                return false;
            }
            return true;
        }
    }

    //******************************************************************************
    template <>
    bool Box<Vec2i>::closestInteriorPoint(const Vec2i& pt, Vec2i& ret) const
    {
        // Don't have to check empty or infinite here
        // because the check below implicitly does so.
        bool interior = true;

        // X
        if (pt.x < min.x)
        {
            interior = false;
            ret.x = min.x;
        }
        else if (pt.x > max.x)
        {
            interior = false;
            ret.x = max.x;
        }
        else
        {
            ret.x = pt.x;
        }

        // Y
        if (pt.y < min.y)
        {
            interior = false;
            ret.y = min.y;
        }
        else if (pt.y > max.y)
        {
            interior = false;
            ret.y = max.y;
        }
        else
        {
            ret.y = pt.y;
        }

        return interior;
    }

    //******************************************************************************
    template <> Box<Vec2i>& Box<Vec2i>::operator*=(const Vec2i& rhs)
    {
        if (!isEmpty() && !isInfinite())
        {
            // X
            min.x *= rhs.x;
            max.x *= rhs.x;
            if (min.x > max.x)
            {
                std::swap(min.x, max.x);
            }

            // Y
            min.y *= rhs.y;
            max.y *= rhs.y;
            if (min.y > max.y)
            {
                std::swap(min.y, max.y);
            }
        }
        return *this;
    }

    //******************************************************************************
    template <> Box<Vec2i>& Box<Vec2i>::operator/=(const Vec2i& rhs)
    {
        if (!isEmpty() && !isInfinite())
        {
            // X
            assert(rhs.x != 0);
            min.x /= rhs.x;
            max.x /= rhs.x;
            if (min.x > max.x)
            {
                std::swap(min.x, max.x);
            }

            // Y
            assert(rhs.y != 0);
            min.y /= rhs.y;
            max.y /= rhs.y;
            if (min.y > max.y)
            {
                std::swap(min.y, max.y);
            }
        }
        return *this;
    }

    //******************************************************************************
    //******************************************************************************
    // VEC2F
    //******************************************************************************
    //******************************************************************************

    //******************************************************************************
    template <> Vec2f Box<Vec2f>::size() const
    {
        if (isEmpty())
        {
            return Vec2f(0.0f);
        }
        else if (isInfinite())
        {
            // This is not correct, but
            // necessary to avoid overflow.
            // In normal use cases is should be close enough.
            return Vec2f(Math<float>::max());
        }
        else
        {
            return max - min;
        }
    }

    //******************************************************************************
    template <> float Box<Vec2f>::size(Box<Vec2f>::size_type i) const
    {
        if (min[i] > max[i])
        {
            // Empty
            return 0.0f;
        }
        else if (min[i] == Math<float>::min() && max[i] == Math<float>::max())
        {
            // infinite
            return Math<float>::max();
        }
        else
        {
            return max[i] - min[i];
        }
    }

    //******************************************************************************
    template <> void Box<Vec2f>::extendBy(const Vec2f& v)
    {
        if (isInfinite())
        {
            return;
        }
        else if (isEmpty())
        {
            min = max = v;
        }
        else
        {
            if (v.x < min.x)
            {
                min.x = v.x;
            }
            else if (v.x > max.x)
            {
                max.x = v.x;
            }

            if (v.y < min.y)
            {
                min.y = v.y;
            }
            else if (v.y > max.y)
            {
                max.y = v.y;
            }
        }
    }

    //******************************************************************************
    template <> void Box<Vec2f>::extendBy(const Box<Vec2f>& b)
    {
        if (b.isEmpty() || isInfinite())
        {
            return;
        }
        else if (b.isInfinite())
        {
            makeInfinite();
            return;
        }
        else if (isEmpty())
        {
            min = b.min;
            max = b.max;
            return;
        }
        else
        {
            if (b.min.x < min.x)
            {
                min.x = b.min.x;
            }
            else if (b.max.x > max.x)
            {
                max.x = b.max.x;
            }

            if (b.min.y < min.y)
            {
                min.y = b.min.y;
            }
            else if (b.max.y > max.y)
            {
                max.y = b.max.y;
            }
        }
    }

    //******************************************************************************
    template <> bool Box<Vec2f>::intersects(const Vec2f& p, float radius) const
    {
        //	Ripped off from graphics gems with improvements
        float d = 0;

        // X
        if (p.x < min.x)
        {
            float s = p.x - min.x;
            d += s * s;
        }
        else if (p.x > max.x)
        {
            float s = p.x - max.x;
            d += s * s;
        }

        // Y
        if (p.y < min.y)
        {
            float s = p.y - min.y;
            d += s * s;
        }
        else if (p.y > max.y)
        {
            float s = p.y - max.y;
            d += s * s;
        }

        return d <= (radius * radius);
    }

    //******************************************************************************
    template <>
    bool Box<Vec2f>::intersects(const Vec2f& origin, const Vec2f& dir) const
    {
        // Ripped off of GGEMS
        static const float LEFT = -1.0f;
        static const float MIDDLE = 0.0f;
        static const float RIGHT = 1.0f;

        bool inside = true;
        Vec2f quadrant;
        Vec2f maxT;
        Vec2f candidatePlane;

        // X
        if (origin.x < min.x)
        {
            quadrant.x = LEFT;
            candidatePlane.x = min.x;
            inside = false;
        }
        else if (origin.x > max.x)
        {
            quadrant.x = RIGHT;
            candidatePlane.x = max.x;
            inside = false;
        }
        else
        {
            quadrant.x = MIDDLE;
        }

        // Y
        if (origin.y < min.y)
        {
            quadrant.y = LEFT;
            candidatePlane.y = min.y;
            inside = false;
        }
        else if (origin.y > max.y)
        {
            quadrant.y = RIGHT;
            candidatePlane.y = max.y;
            inside = false;
        }
        else
        {
            quadrant.y = MIDDLE;
        }

        if (inside)
        {
            return true;
        }

        // X
        if (quadrant.x != MIDDLE && dir.x != 0.0f)
        {
            maxT.x = (candidatePlane.x - origin.x) / dir.x;
        }
        else
        {
            maxT.x = -1.0f;
        }

        // Y
        if (quadrant.y != MIDDLE && dir.y != 0.0f)
        {
            maxT.y = (candidatePlane.y - origin.y) / dir.y;
        }
        else
        {
            maxT.y = -1.0f;
        }

        int whichPlane = maxT.x > maxT.y ? 0 : 1;

        if (maxT[whichPlane] < 0)
        {
            return false;
        }

        // X
        if (whichPlane != 0)
        {
            float x = origin.x + maxT.y * dir.x;

            if (x < min.x || x > max.x)
            {
                return false;
            }
        }
        else
        {
            float y = origin.y + maxT.x * dir.y;

            if (y < min.y || y > max.y)
            {
                return false;
            }
        }

        return true;
    }

    //******************************************************************************
    template <> bool Box<Vec2f>::intersects(const Box<Vec2f>& b) const
    {
        // DO need to check empty or infinity here,
        // otherwise empty.intersects.infinity = true.
        if (isEmpty() || b.isEmpty())
        {
            return false;
        }
        else if (isInfinite() || b.isInfinite())
        {
            return true;
        }
        else
        {
            if (b.max.x < min.x || b.min.x > max.x)
            {
                return false;
            }
            if (b.max.y < min.y || b.min.y > max.y)
            {
                return false;
            }
            return true;
        }
    }

    //******************************************************************************
    template <>
    bool Box<Vec2f>::closestInteriorPoint(const Vec2f& pt, Vec2f& ret) const
    {
        // Don't have to check empty or infinite here
        // because the check below implicitly does so.
        bool interior = true;

        // X
        if (pt.x < min.x)
        {
            interior = false;
            ret.x = min.x;
        }
        else if (pt.x > max.x)
        {
            interior = false;
            ret.x = max.x;
        }
        else
        {
            ret.x = pt.x;
        }

        // Y
        if (pt.y < min.y)
        {
            interior = false;
            ret.y = min.y;
        }
        else if (pt.y > max.y)
        {
            interior = false;
            ret.y = max.y;
        }
        else
        {
            ret.y = pt.y;
        }

        return interior;
    }

    //******************************************************************************
    template <> Box<Vec2f>& Box<Vec2f>::operator*=(const Vec2f& rhs)
    {
        if (!isEmpty() && !isInfinite())
        {
            // X
            min.x *= rhs.x;
            max.x *= rhs.x;
            if (min.x > max.x)
            {
                std::swap(min.x, max.x);
            }

            // Y
            min.y *= rhs.y;
            max.y *= rhs.y;
            if (min.y > max.y)
            {
                std::swap(min.y, max.y);
            }
        }
        return *this;
    }

    //******************************************************************************
    template <> Box<Vec2f>& Box<Vec2f>::operator/=(const Vec2f& rhs)
    {
        if (!isEmpty() && !isInfinite())
        {
            // X
            assert(rhs.x != 0.0f);
            min.x /= rhs.x;
            max.x /= rhs.x;
            if (min.x > max.x)
            {
                std::swap(min.x, max.x);
            }

            // Y
            assert(rhs.y != 0.0f);
            min.y /= rhs.y;
            max.y /= rhs.y;
            if (min.y > max.y)
            {
                std::swap(min.y, max.y);
            }
        }
        return *this;
    }

    //******************************************************************************
    //******************************************************************************
    // VEC2D
    //******************************************************************************
    //******************************************************************************

    //******************************************************************************
    template <> Vec2d Box<Vec2d>::size() const
    {
        if (isEmpty())
        {
            return Vec2d(0.0);
        }
        else if (isInfinite())
        {
            // This is not correct, but
            // necessary to avoid overflow.
            // In normal use cases is should be close enough.
            return Vec2d(Math<double>::max());
        }
        else
        {
            return max - min;
        }
    }

    //******************************************************************************
    template <> double Box<Vec2d>::size(Box<Vec2d>::size_type i) const
    {
        if (min[i] > max[i])
        {
            // Empty
            return 0.0;
        }
        else if (min[i] == Math<double>::min() && max[i] == Math<double>::max())
        {
            // infinite
            return Math<double>::max();
        }
        else
        {
            return max[i] - min[i];
        }
    }

    //******************************************************************************
    template <> void Box<Vec2d>::extendBy(const Vec2d& v)
    {
        if (isInfinite())
        {
            return;
        }
        else if (isEmpty())
        {
            min = max = v;
        }
        else
        {
            if (v.x < min.x)
            {
                min.x = v.x;
            }
            else if (v.x > max.x)
            {
                max.x = v.x;
            }

            if (v.y < min.y)
            {
                min.y = v.y;
            }
            else if (v.y > max.y)
            {
                max.y = v.y;
            }
        }
    }

    //******************************************************************************
    template <> void Box<Vec2d>::extendBy(const Box<Vec2d>& b)
    {
        if (b.isEmpty() || isInfinite())
        {
            return;
        }
        else if (b.isInfinite())
        {
            makeInfinite();
            return;
        }
        else if (isEmpty())
        {
            min = b.min;
            max = b.max;
            return;
        }
        else
        {
            if (b.min.x < min.x)
            {
                min.x = b.min.x;
            }
            else if (b.max.x > max.x)
            {
                max.x = b.max.x;
            }

            if (b.min.y < min.y)
            {
                min.y = b.min.y;
            }
            else if (b.max.y > max.y)
            {
                max.y = b.max.y;
            }
        }
    }

    //******************************************************************************
    template <> bool Box<Vec2d>::intersects(const Vec2d& p, double radius) const
    {
        //	Ripped off from graphics gems with improvements
        double d = 0;

        // X
        if (p.x < min.x)
        {
            double s = p.x - min.x;
            d += s * s;
        }
        else if (p.x > max.x)
        {
            double s = p.x - max.x;
            d += s * s;
        }

        // Y
        if (p.y < min.y)
        {
            double s = p.y - min.y;
            d += s * s;
        }
        else if (p.y > max.y)
        {
            double s = p.y - max.y;
            d += s * s;
        }

        return d <= (radius * radius);
    }

    //******************************************************************************
    template <>
    bool Box<Vec2d>::intersects(const Vec2d& origin, const Vec2d& dir) const
    {
        // Ripped off of GGEMS
        static const double LEFT = -1.0;
        static const double MIDDLE = 0.0;
        static const double RIGHT = 1.0;

        bool inside = true;
        Vec2d quadrant;
        Vec2d maxT;
        Vec2d candidatePlane;

        // X
        if (origin.x < min.x)
        {
            quadrant.x = LEFT;
            candidatePlane.x = min.x;
            inside = false;
        }
        else if (origin.x > max.x)
        {
            quadrant.x = RIGHT;
            candidatePlane.x = max.x;
            inside = false;
        }
        else
        {
            quadrant.x = MIDDLE;
        }

        // Y
        if (origin.y < min.y)
        {
            quadrant.y = LEFT;
            candidatePlane.y = min.y;
            inside = false;
        }
        else if (origin.y > max.y)
        {
            quadrant.y = RIGHT;
            candidatePlane.y = max.y;
            inside = false;
        }
        else
        {
            quadrant.y = MIDDLE;
        }

        if (inside)
        {
            return true;
        }

        // X
        if (quadrant.x != MIDDLE && dir.x != 0.0)
        {
            maxT.x = (candidatePlane.x - origin.x) / dir.x;
        }
        else
        {
            maxT.x = -1.0;
        }

        // Y
        if (quadrant.y != MIDDLE && dir.y != 0.0)
        {
            maxT.y = (candidatePlane.y - origin.y) / dir.y;
        }
        else
        {
            maxT.y = -1.0;
        }

        int whichPlane = maxT.x > maxT.y ? 0 : 1;

        if (maxT[whichPlane] < 0)
        {
            return false;
        }

        // X
        if (whichPlane != 0)
        {
            double x = origin.x + maxT.y * dir.x;

            if (x < min.x || x > max.x)
            {
                return false;
            }
        }
        else
        {
            double y = origin.y + maxT.x * dir.y;

            if (y < min.y || y > max.y)
            {
                return false;
            }
        }

        return true;
    }

    //******************************************************************************
    template <> bool Box<Vec2d>::intersects(const Box<Vec2d>& b) const
    {
        // DO need to check empty or infinity here,
        // otherwise empty.intersects.infinity = true.
        if (isEmpty() || b.isEmpty())
        {
            return false;
        }
        else if (isInfinite() || b.isInfinite())
        {
            return true;
        }
        else
        {
            if (b.max.x < min.x || b.min.x > max.x)
            {
                return false;
            }
            if (b.max.y < min.y || b.min.y > max.y)
            {
                return false;
            }
            return true;
        }
    }

    //******************************************************************************
    template <>
    bool Box<Vec2d>::closestInteriorPoint(const Vec2d& pt, Vec2d& ret) const
    {
        // Don't have to check empty or infinite here
        // because the check below implicitly does so.
        bool interior = true;

        // X
        if (pt.x < min.x)
        {
            interior = false;
            ret.x = min.x;
        }
        else if (pt.x > max.x)
        {
            interior = false;
            ret.x = max.x;
        }
        else
        {
            ret.x = pt.x;
        }

        // Y
        if (pt.y < min.y)
        {
            interior = false;
            ret.y = min.y;
        }
        else if (pt.y > max.y)
        {
            interior = false;
            ret.y = max.y;
        }
        else
        {
            ret.y = pt.y;
        }

        return interior;
    }

    //******************************************************************************
    template <> Box<Vec2d>& Box<Vec2d>::operator*=(const Vec2d& rhs)
    {
        if (!isEmpty() && !isInfinite())
        {
            // X
            min.x *= rhs.x;
            max.x *= rhs.x;
            if (min.x > max.x)
            {
                std::swap(min.x, max.x);
            }

            // Y
            min.y *= rhs.y;
            max.y *= rhs.y;
            if (min.y > max.y)
            {
                std::swap(min.y, max.y);
            }
        }
        return *this;
    }

    //******************************************************************************
    template <> Box<Vec2d>& Box<Vec2d>::operator/=(const Vec2d& rhs)
    {
        if (!isEmpty() && !isInfinite())
        {
            // X
            assert(rhs.x != 0.0);
            min.x /= rhs.x;
            max.x /= rhs.x;
            if (min.x > max.x)
            {
                std::swap(min.x, max.x);
            }

            // Y
            assert(rhs.y != 0.0);
            min.y /= rhs.y;
            max.y /= rhs.y;
            if (min.y > max.y)
            {
                std::swap(min.y, max.y);
            }
        }
        return *this;
    }

    //******************************************************************************
    //******************************************************************************
    // VEC3I
    //******************************************************************************
    //******************************************************************************

    //******************************************************************************
    template <> Vec3i Box<Vec3i>::size() const
    {
        if (isEmpty())
        {
            return Vec3i(0);
        }
        else if (isInfinite())
        {
            // This is not correct, but
            // necessary to avoid overflow.
            // In normal use cases is should be close enough.
            return Vec3i(Math<int>::max());
        }
        else
        {
            return max - min + 1;
        }
    }

    //******************************************************************************
    template <> int Box<Vec3i>::size(Box<Vec3i>::size_type i) const
    {
        if (min[i] > max[i])
        {
            // Empty
            return 0;
        }
        else if (min[i] == Math<int>::min() && max[i] == Math<int>::max())
        {
            // infinite
            return Math<int>::max();
        }
        else
        {
            return max[i] - min[i] + 1;
        }
    }

    //******************************************************************************
    template <> void Box<Vec3i>::extendBy(const Vec3i& v)
    {
        if (isInfinite())
        {
            return;
        }
        else if (isEmpty())
        {
            min = max = v;
        }
        else
        {
            if (v.x < min.x)
            {
                min.x = v.x;
            }
            else if (v.x > max.x)
            {
                max.x = v.x;
            }

            if (v.y < min.y)
            {
                min.y = v.y;
            }
            else if (v.y > max.y)
            {
                max.y = v.y;
            }

            if (v.z < min.z)
            {
                min.z = v.z;
            }
            else if (v.z > max.z)
            {
                max.z = v.z;
            }
        }
    }

    //******************************************************************************
    template <> void Box<Vec3i>::extendBy(const Box<Vec3i>& b)
    {
        if (b.isEmpty() || isInfinite())
        {
            return;
        }
        else if (b.isInfinite())
        {
            makeInfinite();
            return;
        }
        else if (isEmpty())
        {
            min = b.min;
            max = b.max;
            return;
        }
        else
        {
            if (b.min.x < min.x)
            {
                min.x = b.min.x;
            }
            else if (b.max.x > max.x)
            {
                max.x = b.max.x;
            }

            if (b.min.y < min.y)
            {
                min.y = b.min.y;
            }
            else if (b.max.y > max.y)
            {
                max.y = b.max.y;
            }

            if (b.min.z < min.z)
            {
                min.z = b.min.z;
            }
            else if (b.max.z > max.z)
            {
                max.z = b.max.z;
            }
        }
    }

    //******************************************************************************
    template <> bool Box<Vec3i>::intersects(const Vec3i& p, int radius) const
    {
        //	Ripped off from graphics gems with improvements
        int d = 0;

        // X
        if (p.x < min.x)
        {
            int s = p.x - min.x;
            d += s * s;
        }
        else if (p.x > max.x)
        {
            int s = p.x - max.x;
            d += s * s;
        }

        // Y
        if (p.y < min.y)
        {
            int s = p.y - min.y;
            d += s * s;
        }
        else if (p.y > max.y)
        {
            int s = p.y - max.y;
            d += s * s;
        }

        // Z
        if (p.z < min.z)
        {
            int s = p.z - min.z;
            d += s * s;
        }
        else if (p.z > max.z)
        {
            int s = p.z - max.z;
            d += s * s;
        }

        return d <= (radius * radius);
    }

    //******************************************************************************
    template <>
    bool Box<Vec3i>::intersects(const Vec3i& origin, const Vec3i& dir) const
    {
        // Ripped off of GGEMS
        static const int LEFT = -1;
        static const int MIDDLE = 0;
        static const int RIGHT = 1;

        bool inside = true;
        Vec3i quadrant;
        Vec3i maxT;
        Vec3i candidatePlane;

        // X
        if (origin.x < min.x)
        {
            quadrant.x = LEFT;
            candidatePlane.x = min.x;
            inside = false;
        }
        else if (origin.x > max.x)
        {
            quadrant.x = RIGHT;
            candidatePlane.x = max.x;
            inside = false;
        }
        else
        {
            quadrant.x = MIDDLE;
        }

        // Y
        if (origin.y < min.y)
        {
            quadrant.y = LEFT;
            candidatePlane.y = min.y;
            inside = false;
        }
        else if (origin.y > max.y)
        {
            quadrant.y = RIGHT;
            candidatePlane.y = max.y;
            inside = false;
        }
        else
        {
            quadrant.y = MIDDLE;
        }

        // Z
        if (origin.z < min.z)
        {
            quadrant.z = LEFT;
            candidatePlane.z = min.z;
            inside = false;
        }
        else if (origin.z > max.z)
        {
            quadrant.z = RIGHT;
            candidatePlane.z = max.z;
            inside = false;
        }
        else
        {
            quadrant.z = MIDDLE;
        }

        if (inside)
        {
            return true;
        }

        // X
        if (quadrant.x != MIDDLE && dir.x != 0)
        {
            maxT.x = (candidatePlane.x - origin.x) / dir.x;
        }
        else
        {
            maxT.x = -1;
        }

        // Y
        if (quadrant.y != MIDDLE && dir.y != 0)
        {
            maxT.y = (candidatePlane.y - origin.y) / dir.y;
        }
        else
        {
            maxT.y = -1;
        }

        // Z
        if (quadrant.z != MIDDLE && dir.z != 0)
        {
            maxT.z = (candidatePlane.z - origin.z) / dir.z;
        }
        else
        {
            maxT.z = -1;
        }

        int whichPlane = maxT.x > maxT.y ? (maxT.x > maxT.z ? 0 : 2)
                                         : (maxT.y > maxT.z ? 1 : 2);

        if (maxT[whichPlane] < 0)
        {
            return false;
        }

        // X
        if (whichPlane != 0)
        {
            int x = origin.x + maxT[whichPlane] * dir.x;

            if (x < min.x || x > max.x)
            {
                return false;
            }
        }
        else if (whichPlane != 1)
        {
            int y = origin.y + maxT[whichPlane] * dir.y;

            if (y < min.y || y > max.y)
            {
                return false;
            }
        }
        else
        {
            int z = origin.z + maxT[whichPlane] * dir.z;

            if (z < min.z || z > max.z)
            {
                return false;
            }
        }

        return true;
    }

    //******************************************************************************
    template <> bool Box<Vec3i>::intersects(const Box<Vec3i>& b) const
    {
        // DO need to check empty or infinity here,
        // otherwise empty.intersects.infinity = true.
        if (isEmpty() || b.isEmpty())
        {
            return false;
        }
        else if (isInfinite() || b.isInfinite())
        {
            return true;
        }
        else
        {
            if (b.max.x < min.x || b.min.x > max.x)
            {
                return false;
            }
            if (b.max.y < min.y || b.min.y > max.y)
            {
                return false;
            }
            if (b.max.z < min.z || b.min.z > max.z)
            {
                return false;
            }
            return true;
        }
    }

    //******************************************************************************
    template <>
    bool Box<Vec3i>::closestInteriorPoint(const Vec3i& pt, Vec3i& ret) const
    {
        // Don't have to check empty or infinite here
        // because the check below implicitly does so.
        bool interior = true;

        // X
        if (pt.x < min.x)
        {
            interior = false;
            ret.x = min.x;
        }
        else if (pt.x > max.x)
        {
            interior = false;
            ret.x = max.x;
        }
        else
        {
            ret.x = pt.x;
        }

        // Y
        if (pt.y < min.y)
        {
            interior = false;
            ret.y = min.y;
        }
        else if (pt.y > max.y)
        {
            interior = false;
            ret.y = max.y;
        }
        else
        {
            ret.y = pt.y;
        }

        // Z
        if (pt.z < min.z)
        {
            interior = false;
            ret.z = min.z;
        }
        else if (pt.z > max.z)
        {
            interior = false;
            ret.z = max.z;
        }
        else
        {
            ret.z = pt.z;
        }

        return interior;
    }

    //******************************************************************************
    template <> Box<Vec3i>& Box<Vec3i>::operator*=(const Vec3i& rhs)
    {
        if (!isEmpty() && !isInfinite())
        {
            // X
            min.x *= rhs.x;
            max.x *= rhs.x;
            if (min.x > max.x)
            {
                std::swap(min.x, max.x);
            }

            // Y
            min.y *= rhs.y;
            max.y *= rhs.y;
            if (min.y > max.y)
            {
                std::swap(min.y, max.y);
            }

            // Z
            min.z *= rhs.z;
            max.z *= rhs.z;
            if (min.z > max.z)
            {
                std::swap(min.z, max.z);
            }
        }
        return *this;
    }

    //******************************************************************************
    template <> Box<Vec3i>& Box<Vec3i>::operator/=(const Vec3i& rhs)
    {
        if (!isEmpty() && !isInfinite())
        {
            // X
            assert(rhs.x != 0);
            min.x /= rhs.x;
            max.x /= rhs.x;
            if (min.x > max.x)
            {
                std::swap(min.x, max.x);
            }

            // Y
            assert(rhs.y != 0);
            min.y /= rhs.y;
            max.y /= rhs.y;
            if (min.y > max.y)
            {
                std::swap(min.y, max.y);
            }

            // Z
            assert(rhs.z != 0);
            min.z /= rhs.z;
            max.z /= rhs.z;
            if (min.z > max.z)
            {
                std::swap(min.z, max.z);
            }
        }
        return *this;
    }

    //******************************************************************************
    //******************************************************************************
    // VEC3F
    //******************************************************************************
    //******************************************************************************

    //******************************************************************************
    template <> Vec3f Box<Vec3f>::size() const
    {
        if (isEmpty())
        {
            return Vec3f(0.0f);
        }
        else if (isInfinite())
        {
            // This is not correct, but
            // necessary to avoid overflow.
            // In normal use cases is should be close enough.
            return Vec3f(Math<float>::max());
        }
        else
        {
            return max - min;
        }
    }

    //******************************************************************************
    template <> float Box<Vec3f>::size(Box<Vec3f>::size_type i) const
    {
        if (min[i] > max[i])
        {
            // Empty
            return 0.0f;
        }
        else if (min[i] == Math<float>::min() && max[i] == Math<float>::max())
        {
            // infinite
            return Math<float>::max();
        }
        else
        {
            return max[i] - min[i];
        }
    }

    //******************************************************************************
    template <> void Box<Vec3f>::extendBy(const Vec3f& v)
    {
        if (isInfinite())
        {
            return;
        }
        else if (isEmpty())
        {
            min = max = v;
        }
        else
        {
            if (v.x < min.x)
            {
                min.x = v.x;
            }
            else if (v.x > max.x)
            {
                max.x = v.x;
            }

            if (v.y < min.y)
            {
                min.y = v.y;
            }
            else if (v.y > max.y)
            {
                max.y = v.y;
            }

            if (v.z < min.z)
            {
                min.z = v.z;
            }
            else if (v.z > max.z)
            {
                max.z = v.z;
            }
        }
    }

    //******************************************************************************
    template <> void Box<Vec3f>::extendBy(const Box<Vec3f>& b)
    {
        if (b.isEmpty() || isInfinite())
        {
            return;
        }
        else if (b.isInfinite())
        {
            makeInfinite();
            return;
        }
        else if (isEmpty())
        {
            min = b.min;
            max = b.max;
            return;
        }
        else
        {
            if (b.min.x < min.x)
            {
                min.x = b.min.x;
            }
            else if (b.max.x > max.x)
            {
                max.x = b.max.x;
            }

            if (b.min.y < min.y)
            {
                min.y = b.min.y;
            }
            else if (b.max.y > max.y)
            {
                max.y = b.max.y;
            }

            if (b.min.z < min.z)
            {
                min.z = b.min.z;
            }
            else if (b.max.z > max.z)
            {
                max.z = b.max.z;
            }
        }
    }

    //******************************************************************************
    template <> bool Box<Vec3f>::intersects(const Vec3f& p, float radius) const
    {
        //	Ripped off from graphics gems with improvements
        float d = 0;

        // X
        if (p.x < min.x)
        {
            float s = p.x - min.x;
            d += s * s;
        }
        else if (p.x > max.x)
        {
            float s = p.x - max.x;
            d += s * s;
        }

        // Y
        if (p.y < min.y)
        {
            float s = p.y - min.y;
            d += s * s;
        }
        else if (p.y > max.y)
        {
            float s = p.y - max.y;
            d += s * s;
        }

        // Z
        if (p.z < min.z)
        {
            float s = p.z - min.z;
            d += s * s;
        }
        else if (p.z > max.z)
        {
            float s = p.z - max.z;
            d += s * s;
        }

        return d <= (radius * radius);
    }

    //******************************************************************************
    template <>
    bool Box<Vec3f>::intersects(const Vec3f& origin, const Vec3f& dir) const
    {
        // Ripped off of GGEMS
        static const float LEFT = -1.0f;
        static const float MIDDLE = 0.0f;
        static const float RIGHT = 1.0f;

        bool inside = true;
        Vec3f quadrant;
        Vec3f maxT;
        Vec3f candidatePlane;

        // X
        if (origin.x < min.x)
        {
            quadrant.x = LEFT;
            candidatePlane.x = min.x;
            inside = false;
        }
        else if (origin.x > max.x)
        {
            quadrant.x = RIGHT;
            candidatePlane.x = max.x;
            inside = false;
        }
        else
        {
            quadrant.x = MIDDLE;
        }

        // Y
        if (origin.y < min.y)
        {
            quadrant.y = LEFT;
            candidatePlane.y = min.y;
            inside = false;
        }
        else if (origin.y > max.y)
        {
            quadrant.y = RIGHT;
            candidatePlane.y = max.y;
            inside = false;
        }
        else
        {
            quadrant.y = MIDDLE;
        }

        // Z
        if (origin.z < min.z)
        {
            quadrant.z = LEFT;
            candidatePlane.z = min.z;
            inside = false;
        }
        else if (origin.z > max.z)
        {
            quadrant.z = RIGHT;
            candidatePlane.z = max.z;
            inside = false;
        }
        else
        {
            quadrant.z = MIDDLE;
        }

        if (inside)
        {
            return true;
        }

        // X
        if (quadrant.x != MIDDLE && dir.x != 0.0f)
        {
            maxT.x = (candidatePlane.x - origin.x) / dir.x;
        }
        else
        {
            maxT.x = -1.0f;
        }

        // Y
        if (quadrant.y != MIDDLE && dir.y != 0.0f)
        {
            maxT.y = (candidatePlane.y - origin.y) / dir.y;
        }
        else
        {
            maxT.y = -1.0f;
        }

        // Z
        if (quadrant.z != MIDDLE && dir.z != 0.0f)
        {
            maxT.z = (candidatePlane.z - origin.z) / dir.z;
        }
        else
        {
            maxT.z = -1.0f;
        }

        int whichPlane = maxT.x > maxT.y ? (maxT.x > maxT.z ? 0 : 2)
                                         : (maxT.y > maxT.z ? 1 : 2);

        if (maxT[whichPlane] < 0.0f)
        {
            return false;
        }

        // X
        if (whichPlane != 0)
        {
            float x = origin.x + maxT[whichPlane] * dir.x;

            if (x < min.x || x > max.x)
            {
                return false;
            }
        }
        else if (whichPlane != 1)
        {
            float y = origin.y + maxT[whichPlane] * dir.y;

            if (y < min.y || y > max.y)
            {
                return false;
            }
        }
        else
        {
            float z = origin.z + maxT[whichPlane] * dir.z;

            if (z < min.z || z > max.z)
            {
                return false;
            }
        }

        return true;
    }

    //******************************************************************************
    template <> bool Box<Vec3f>::intersects(const Box<Vec3f>& b) const
    {
        // DO need to check empty or infinity here,
        // otherwise empty.intersects.infinity = true.
        if (isEmpty() || b.isEmpty())
        {
            return false;
        }
        else if (isInfinite() || b.isInfinite())
        {
            return true;
        }
        else
        {
            if (b.max.x < min.x || b.min.x > max.x)
            {
                return false;
            }
            if (b.max.y < min.y || b.min.y > max.y)
            {
                return false;
            }
            if (b.max.z < min.z || b.min.z > max.z)
            {
                return false;
            }
            return true;
        }
    }

    //******************************************************************************
    template <>
    bool Box<Vec3f>::closestInteriorPoint(const Vec3f& pt, Vec3f& ret) const
    {
        // Don't have to check empty or infinite here
        // because the check below implicitly does so.
        bool interior = true;

        // X
        if (pt.x < min.x)
        {
            interior = false;
            ret.x = min.x;
        }
        else if (pt.x > max.x)
        {
            interior = false;
            ret.x = max.x;
        }
        else
        {
            ret.x = pt.x;
        }

        // Y
        if (pt.y < min.y)
        {
            interior = false;
            ret.y = min.y;
        }
        else if (pt.y > max.y)
        {
            interior = false;
            ret.y = max.y;
        }
        else
        {
            ret.y = pt.y;
        }

        // Z
        if (pt.z < min.z)
        {
            interior = false;
            ret.z = min.z;
        }
        else if (pt.z > max.z)
        {
            interior = false;
            ret.z = max.z;
        }
        else
        {
            ret.z = pt.z;
        }

        return interior;
    }

    //******************************************************************************
    template <> Box<Vec3f>& Box<Vec3f>::operator*=(const Vec3f& rhs)
    {
        if (!isEmpty() && !isInfinite())
        {
            // X
            min.x *= rhs.x;
            max.x *= rhs.x;
            if (min.x > max.x)
            {
                std::swap(min.x, max.x);
            }

            // Y
            min.y *= rhs.y;
            max.y *= rhs.y;
            if (min.y > max.y)
            {
                std::swap(min.y, max.y);
            }

            // Z
            min.z *= rhs.z;
            max.z *= rhs.z;
            if (min.z > max.z)
            {
                std::swap(min.z, max.z);
            }
        }
        return *this;
    }

    //******************************************************************************
    template <> Box<Vec3f>& Box<Vec3f>::operator/=(const Vec3f& rhs)
    {
        if (!isEmpty() && !isInfinite())
        {
            // X
            assert(rhs.x != 0.0f);
            min.x /= rhs.x;
            max.x /= rhs.x;
            if (min.x > max.x)
            {
                std::swap(min.x, max.x);
            }

            // Y
            assert(rhs.y != 0.0f);
            min.y /= rhs.y;
            max.y /= rhs.y;
            if (min.y > max.y)
            {
                std::swap(min.y, max.y);
            }

            // Z
            assert(rhs.z != 0.0f);
            min.z /= rhs.z;
            max.z /= rhs.z;
            if (min.z > max.z)
            {
                std::swap(min.z, max.z);
            }
        }
        return *this;
    }

    //******************************************************************************
    //******************************************************************************
    // VEC3D
    //******************************************************************************
    //******************************************************************************

    //******************************************************************************
    template <> Vec3d Box<Vec3d>::size() const
    {
        if (isEmpty())
        {
            return Vec3d(0.0);
        }
        else if (isInfinite())
        {
            // This is not correct, but
            // necessary to avoid overflow.
            // In normal use cases is should be close enough.
            return Vec3d(Math<double>::max());
        }
        else
        {
            return max - min;
        }
    }

    //******************************************************************************
    template <> double Box<Vec3d>::size(Box<Vec3d>::size_type i) const
    {
        if (min[i] > max[i])
        {
            // Empty
            return 0.0;
        }
        else if (min[i] == Math<double>::min() && max[i] == Math<double>::max())
        {
            // infinite
            return Math<double>::max();
        }
        else
        {
            return max[i] - min[i];
        }
    }

    //******************************************************************************
    template <> void Box<Vec3d>::extendBy(const Vec3d& v)
    {
        if (isInfinite())
        {
            return;
        }
        else if (isEmpty())
        {
            min = max = v;
        }
        else
        {
            if (v.x < min.x)
            {
                min.x = v.x;
            }
            else if (v.x > max.x)
            {
                max.x = v.x;
            }

            if (v.y < min.y)
            {
                min.y = v.y;
            }
            else if (v.y > max.y)
            {
                max.y = v.y;
            }

            if (v.z < min.z)
            {
                min.z = v.z;
            }
            else if (v.z > max.z)
            {
                max.z = v.z;
            }
        }
    }

    //******************************************************************************
    template <> void Box<Vec3d>::extendBy(const Box<Vec3d>& b)
    {
        if (b.isEmpty() || isInfinite())
        {
            return;
        }
        else if (b.isInfinite())
        {
            makeInfinite();
            return;
        }
        else if (isEmpty())
        {
            min = b.min;
            max = b.max;
            return;
        }
        else
        {
            if (b.min.x < min.x)
            {
                min.x = b.min.x;
            }
            else if (b.max.x > max.x)
            {
                max.x = b.max.x;
            }

            if (b.min.y < min.y)
            {
                min.y = b.min.y;
            }
            else if (b.max.y > max.y)
            {
                max.y = b.max.y;
            }

            if (b.min.z < min.z)
            {
                min.z = b.min.z;
            }
            else if (b.max.z > max.z)
            {
                max.z = b.max.z;
            }
        }
    }

    //******************************************************************************
    template <> bool Box<Vec3d>::intersects(const Vec3d& p, double radius) const
    {
        //	Ripped off from graphics gems with improvements
        double d = 0;

        // X
        if (p.x < min.x)
        {
            double s = p.x - min.x;
            d += s * s;
        }
        else if (p.x > max.x)
        {
            double s = p.x - max.x;
            d += s * s;
        }

        // Y
        if (p.y < min.y)
        {
            double s = p.y - min.y;
            d += s * s;
        }
        else if (p.y > max.y)
        {
            double s = p.y - max.y;
            d += s * s;
        }

        // Z
        if (p.z < min.z)
        {
            double s = p.z - min.z;
            d += s * s;
        }
        else if (p.z > max.z)
        {
            double s = p.z - max.z;
            d += s * s;
        }

        return d <= (radius * radius);
    }

    //******************************************************************************
    template <>
    bool Box<Vec3d>::intersects(const Vec3d& origin, const Vec3d& dir) const
    {
        // Ripped off of GGEMS
        static const double LEFT = -1.0;
        static const double MIDDLE = 0.0;
        static const double RIGHT = 1.0;

        bool inside = true;
        Vec3d quadrant;
        Vec3d maxT;
        Vec3d candidatePlane;

        // X
        if (origin.x < min.x)
        {
            quadrant.x = LEFT;
            candidatePlane.x = min.x;
            inside = false;
        }
        else if (origin.x > max.x)
        {
            quadrant.x = RIGHT;
            candidatePlane.x = max.x;
            inside = false;
        }
        else
        {
            quadrant.x = MIDDLE;
        }

        // Y
        if (origin.y < min.y)
        {
            quadrant.y = LEFT;
            candidatePlane.y = min.y;
            inside = false;
        }
        else if (origin.y > max.y)
        {
            quadrant.y = RIGHT;
            candidatePlane.y = max.y;
            inside = false;
        }
        else
        {
            quadrant.y = MIDDLE;
        }

        // Z
        if (origin.z < min.z)
        {
            quadrant.z = LEFT;
            candidatePlane.z = min.z;
            inside = false;
        }
        else if (origin.z > max.z)
        {
            quadrant.z = RIGHT;
            candidatePlane.z = max.z;
            inside = false;
        }
        else
        {
            quadrant.z = MIDDLE;
        }

        if (inside)
        {
            return true;
        }

        // X
        if (quadrant.x != MIDDLE && dir.x != 0.0)
        {
            maxT.x = (candidatePlane.x - origin.x) / dir.x;
        }
        else
        {
            maxT.x = -1.0;
        }

        // Y
        if (quadrant.y != MIDDLE && dir.y != 0.0)
        {
            maxT.y = (candidatePlane.y - origin.y) / dir.y;
        }
        else
        {
            maxT.y = -1.0;
        }

        // Z
        if (quadrant.z != MIDDLE && dir.z != 0.0)
        {
            maxT.z = (candidatePlane.z - origin.z) / dir.z;
        }
        else
        {
            maxT.z = -1.0;
        }

        int whichPlane = maxT.x > maxT.y ? (maxT.x > maxT.z ? 0 : 2)
                                         : (maxT.y > maxT.z ? 1 : 2);

        if (maxT[whichPlane] < 0.0)
        {
            return false;
        }

        // X
        if (whichPlane != 0)
        {
            double x = origin.x + maxT[whichPlane] * dir.x;

            if (x < min.x || x > max.x)
            {
                return false;
            }
        }
        else if (whichPlane != 1)
        {
            double y = origin.y + maxT[whichPlane] * dir.y;

            if (y < min.y || y > max.y)
            {
                return false;
            }
        }
        else
        {
            double z = origin.z + maxT[whichPlane] * dir.z;

            if (z < min.z || z > max.z)
            {
                return false;
            }
        }

        return true;
    }

    //******************************************************************************
    template <> bool Box<Vec3d>::intersects(const Box<Vec3d>& b) const
    {
        // DO need to check empty or infinity here,
        // otherwise empty.intersects.infinity = true.
        if (isEmpty() || b.isEmpty())
        {
            return false;
        }
        else if (isInfinite() || b.isInfinite())
        {
            return true;
        }
        else
        {
            if (b.max.x < min.x || b.min.x > max.x)
            {
                return false;
            }
            if (b.max.y < min.y || b.min.y > max.y)
            {
                return false;
            }
            if (b.max.z < min.z || b.min.z > max.z)
            {
                return false;
            }
            return true;
        }
    }

    //******************************************************************************
    template <>
    bool Box<Vec3d>::closestInteriorPoint(const Vec3d& pt, Vec3d& ret) const
    {
        // Don't have to check empty or infinite here
        // because the check below implicitly does so.
        bool interior = true;

        // X
        if (pt.x < min.x)
        {
            interior = false;
            ret.x = min.x;
        }
        else if (pt.x > max.x)
        {
            interior = false;
            ret.x = max.x;
        }
        else
        {
            ret.x = pt.x;
        }

        // Y
        if (pt.y < min.y)
        {
            interior = false;
            ret.y = min.y;
        }
        else if (pt.y > max.y)
        {
            interior = false;
            ret.y = max.y;
        }
        else
        {
            ret.y = pt.y;
        }

        // Z
        if (pt.z < min.z)
        {
            interior = false;
            ret.z = min.z;
        }
        else if (pt.z > max.z)
        {
            interior = false;
            ret.z = max.z;
        }
        else
        {
            ret.z = pt.z;
        }

        return interior;
    }

    //******************************************************************************
    template <> Box<Vec3d>& Box<Vec3d>::operator*=(const Vec3d& rhs)
    {
        if (!isEmpty() && !isInfinite())
        {
            // X
            min.x *= rhs.x;
            max.x *= rhs.x;
            if (min.x > max.x)
            {
                std::swap(min.x, max.x);
            }

            // Y
            min.y *= rhs.y;
            max.y *= rhs.y;
            if (min.y > max.y)
            {
                std::swap(min.y, max.y);
            }

            // Z
            min.z *= rhs.z;
            max.z *= rhs.z;
            if (min.z > max.z)
            {
                std::swap(min.z, max.z);
            }
        }
        return *this;
    }

    //******************************************************************************
    template <> Box<Vec3d>& Box<Vec3d>::operator/=(const Vec3d& rhs)
    {
        if (!isEmpty() && !isInfinite())
        {
            // X
            assert(rhs.x != 0.0);
            min.x /= rhs.x;
            max.x /= rhs.x;
            if (min.x > max.x)
            {
                std::swap(min.x, max.x);
            }

            // Y
            assert(rhs.y != 0.0);
            min.y /= rhs.y;
            max.y /= rhs.y;
            if (min.y > max.y)
            {
                std::swap(min.y, max.y);
            }

            // Z
            assert(rhs.z != 0.0);
            min.z /= rhs.z;
            max.z /= rhs.z;
            if (min.z > max.z)
            {
                std::swap(min.z, max.z);
            }
        }
        return *this;
    }

} // End namespace TwkMath
