//******************************************************************************
// Copyright (c) 2001-2003 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkMathBoxSpec_h_
#define _TwkMathBoxSpec_h_

namespace TwkMath
{

    //******************************************************************************
    // THIS FILE CONTAINS SPECIALIZATIONS FOR THE BOX CLASS.
    // IT IS INCLUDED BY TWK_MATH_BOX.H, so NEVER INCLUDE IT YOURSELF!

    //******************************************************************************
    // TEMPLATE AND INLINE FUNCTIONS
    //******************************************************************************

    //******************************************************************************
    //******************************************************************************
    // VEC2I
    //******************************************************************************
    //******************************************************************************
    template <> inline bool Box<Vec2i>::isEmpty() const
    {
        return (min.x > max.x || min.y > max.y);
    }

    //******************************************************************************
    template <> inline bool Box<Vec2i>::isInfinite() const
    {
        static const int mini = Math<int>::min();
        static const int maxi = Math<int>::max();
        return (min.x == mini && min.y == mini && max.x == maxi
                && max.y == maxi);
    }

    //******************************************************************************
    template <> inline size_t Box<Vec2i>::majorAxis() const
    {
        int v0 = Math<int>::abs(max.x - min.x);
        int v1 = Math<int>::abs(max.y - min.y);
        return v0 > v1 ? 0 : 1;
    }

    //******************************************************************************
    template <> Vec2i Box<Vec2i>::size() const;

    //******************************************************************************
    template <> int Box<Vec2i>::size(Box<Vec2i>::size_type i) const;

    //******************************************************************************
    template <> void Box<Vec2i>::extendBy(const Vec2i& v);

    //******************************************************************************
    template <> void Box<Vec2i>::extendBy(const Box<Vec2i>& b);

    //******************************************************************************
    template <> inline bool Box<Vec2i>::intersects(const Vec2i& v) const
    {
        return (v.x >= min.x && v.x <= max.x && v.y >= min.y && v.y <= max.y);
    }

    //******************************************************************************
    template <> bool Box<Vec2i>::intersects(const Vec2i& p, int radius) const;

    //******************************************************************************
    template <>
    bool Box<Vec2i>::intersects(const Vec2i& origin, const Vec2i& dir) const;

    //******************************************************************************
    template <> bool Box<Vec2i>::intersects(const Box<Vec2i>& b) const;

    //******************************************************************************
    template <>
    bool Box<Vec2i>::closestInteriorPoint(const Vec2i& pt, Vec2i& ret) const;

    //******************************************************************************
    template <> Box<Vec2i>& Box<Vec2i>::operator*=(const Vec2i& rhs);

    //******************************************************************************
    template <> Box<Vec2i>& Box<Vec2i>::operator/=(const Vec2i& rhs);

    //******************************************************************************
    // template <>
    inline Box<Vec2i> intersection(const Box<Vec2i>& a, const Box<Vec2i>& b)
    {
        if (a.intersects(b))
        {
            Vec2i min;
            Vec2i max;

            // X
            min.x = std::max(a.min.x, b.min.x);
            max.x = std::min(a.max.x, b.max.x);

            // Y
            min.y = std::max(a.min.y, b.min.y);
            max.y = std::min(a.max.y, b.max.y);

            return Box<Vec2i>(min, max);
        }
        else
        {
            return Box<Vec2i>();
        }
    }

    //******************************************************************************
    //******************************************************************************
    // VEC2F
    //******************************************************************************
    //******************************************************************************
    template <> inline bool Box<Vec2f>::isEmpty() const
    {
        return (min.x > max.x || min.y > max.y);
    }

    //******************************************************************************
    template <> inline bool Box<Vec2f>::isInfinite() const
    {
        static const float minf = Math<float>::min();
        static const float maxf = Math<float>::max();
        return (min.x == minf && min.y == minf && max.x == maxf
                && max.y == maxf);
    }

    //******************************************************************************
    template <> inline size_t Box<Vec2f>::majorAxis() const
    {
        float v0 = Math<float>::abs(max.x - min.x);
        float v1 = Math<float>::abs(max.y - min.y);
        return v0 > v1 ? 0 : 1;
    }

    //******************************************************************************
    template <> Vec2f Box<Vec2f>::size() const;

    //******************************************************************************
    template <> float Box<Vec2f>::size(Box<Vec2f>::size_type i) const;

    //******************************************************************************
    template <> void Box<Vec2f>::extendBy(const Vec2f& v);

    //******************************************************************************
    template <> void Box<Vec2f>::extendBy(const Box<Vec2f>& b);

    //******************************************************************************
    template <> inline bool Box<Vec2f>::intersects(const Vec2f& v) const
    {
        return (v.x >= min.x && v.x <= max.x && v.y >= min.y && v.y <= max.y);
    }

    //******************************************************************************
    template <> bool Box<Vec2f>::intersects(const Vec2f& p, float radius) const;

    //******************************************************************************
    template <>
    bool Box<Vec2f>::intersects(const Vec2f& origin, const Vec2f& dir) const;

    //******************************************************************************
    template <> bool Box<Vec2f>::intersects(const Box<Vec2f>& b) const;

    //******************************************************************************
    template <>
    bool Box<Vec2f>::closestInteriorPoint(const Vec2f& pt, Vec2f& ret) const;

    //******************************************************************************
    template <> Box<Vec2f>& Box<Vec2f>::operator*=(const Vec2f& rhs);

    //******************************************************************************
    template <> Box<Vec2f>& Box<Vec2f>::operator/=(const Vec2f& rhs);

    //******************************************************************************
    // template <>
    inline Box<Vec2f> intersection(const Box<Vec2f>& a, const Box<Vec2f>& b)
    {
        if (a.intersects(b))
        {
            Vec2f min;
            Vec2f max;

            // X
            min.x = std::max(a.min.x, b.min.x);
            max.x = std::min(a.max.x, b.max.x);

            // Y
            min.y = std::max(a.min.y, b.min.y);
            max.y = std::min(a.max.y, b.max.y);

            return Box<Vec2f>(min, max);
        }
        else
        {
            return Box<Vec2f>();
        }
    }

    //******************************************************************************
    //******************************************************************************
    // VEC2D
    //******************************************************************************
    //******************************************************************************
    template <> inline bool Box<Vec2d>::isEmpty() const
    {
        return (min.x > max.x || min.y > max.y);
    }

    //******************************************************************************
    template <> inline bool Box<Vec2d>::isInfinite() const
    {
        static const double minf = Math<double>::min();
        static const double maxf = Math<double>::max();
        return (min.x == minf && min.y == minf && max.x == maxf
                && max.y == maxf);
    }

    //******************************************************************************
    template <> inline size_t Box<Vec2d>::majorAxis() const
    {
        double v0 = Math<double>::abs(max.x - min.x);
        double v1 = Math<double>::abs(max.y - min.y);
        return v0 > v1 ? 0 : 1;
    }

    //******************************************************************************
    template <> Vec2d Box<Vec2d>::size() const;

    //******************************************************************************
    template <> double Box<Vec2d>::size(Box<Vec2d>::size_type i) const;

    //******************************************************************************
    template <> void Box<Vec2d>::extendBy(const Vec2d& v);

    //******************************************************************************
    template <> void Box<Vec2d>::extendBy(const Box<Vec2d>& b);

    //******************************************************************************
    template <> inline bool Box<Vec2d>::intersects(const Vec2d& v) const
    {
        return (v.x >= min.x && v.x <= max.x && v.y >= min.y && v.y <= max.y);
    }

    //******************************************************************************
    template <>
    bool Box<Vec2d>::intersects(const Vec2d& p, double radius) const;

    //******************************************************************************
    template <>
    bool Box<Vec2d>::intersects(const Vec2d& origin, const Vec2d& dir) const;

    //******************************************************************************
    template <> bool Box<Vec2d>::intersects(const Box<Vec2d>& b) const;

    //******************************************************************************
    template <>
    bool Box<Vec2d>::closestInteriorPoint(const Vec2d& pt, Vec2d& ret) const;

    //******************************************************************************
    template <> Box<Vec2d>& Box<Vec2d>::operator*=(const Vec2d& rhs);

    //******************************************************************************
    template <> Box<Vec2d>& Box<Vec2d>::operator/=(const Vec2d& rhs);

    //******************************************************************************
    // template <>
    inline Box<Vec2d> intersection(const Box<Vec2d>& a, const Box<Vec2d>& b)
    {
        if (a.intersects(b))
        {
            Vec2d min;
            Vec2d max;

            // X
            min.x = std::max(a.min.x, b.min.x);
            max.x = std::min(a.max.x, b.max.x);

            // Y
            min.y = std::max(a.min.y, b.min.y);
            max.y = std::min(a.max.y, b.max.y);

            return Box<Vec2d>(min, max);
        }
        else
        {
            return Box<Vec2d>();
        }
    }

    //******************************************************************************
    //******************************************************************************
    // VEC3I
    //******************************************************************************
    //******************************************************************************
    template <> inline bool Box<Vec3i>::isEmpty() const
    {
        return (min.x > max.x || min.y > max.y || min.z > max.z);
    }

    //******************************************************************************
    template <> inline bool Box<Vec3i>::isInfinite() const
    {
        static const int mini = Math<int>::min();
        static const int maxi = Math<int>::max();
        return (min.x == mini && min.y == mini && min.z == mini && max.x == maxi
                && max.y == maxi && max.z == maxi);
    }

    //******************************************************************************
    template <> inline size_t Box<Vec3i>::majorAxis() const
    {
        int v0 = Math<int>::abs(max.x - min.x);
        int v1 = Math<int>::abs(max.y - min.y);
        int v2 = Math<int>::abs(max.z - min.z);
        return v0 > v1 ? (v0 > v2 ? 0 : 2) : (v1 > v2 ? 1 : 2);
    }

    //******************************************************************************
    template <> Vec3i Box<Vec3i>::size() const;

    //******************************************************************************
    template <> int Box<Vec3i>::size(Box<Vec3i>::size_type i) const;

    //******************************************************************************
    template <> void Box<Vec3i>::extendBy(const Vec3i& v);

    //******************************************************************************
    template <> void Box<Vec3i>::extendBy(const Box<Vec3i>& b);

    //******************************************************************************
    template <> inline bool Box<Vec3i>::intersects(const Vec3i& v) const
    {
        return (v.x >= min.x && v.x <= max.x && v.y >= min.y && v.y <= max.y
                && v.z >= min.z && v.z <= max.z);
    }

    //******************************************************************************
    template <> bool Box<Vec3i>::intersects(const Vec3i& p, int radius) const;

    //******************************************************************************
    template <>
    bool Box<Vec3i>::intersects(const Vec3i& origin, const Vec3i& dir) const;

    //******************************************************************************
    template <> bool Box<Vec3i>::intersects(const Box<Vec3i>& b) const;

    //******************************************************************************
    template <>
    bool Box<Vec3i>::closestInteriorPoint(const Vec3i& pt, Vec3i& ret) const;

    //******************************************************************************
    template <> Box<Vec3i>& Box<Vec3i>::operator*=(const Vec3i& rhs);

    //******************************************************************************
    template <> Box<Vec3i>& Box<Vec3i>::operator/=(const Vec3i& rhs);

    //******************************************************************************
    // template <>
    inline Box<Vec3i> intersection(const Box<Vec3i>& a, const Box<Vec3i>& b)
    {
        if (a.intersects(b))
        {
            Vec3i min;
            Vec3i max;

            // X
            min.x = std::max(a.min.x, b.min.x);
            max.x = std::min(a.max.x, b.max.x);

            // Y
            min.y = std::max(a.min.y, b.min.y);
            max.y = std::min(a.max.y, b.max.y);

            // Z
            min.z = std::max(a.min.z, b.min.z);
            max.z = std::min(a.max.z, b.max.z);

            return Box<Vec3i>(min, max);
        }
        else
        {
            return Box<Vec3i>();
        }
    }

    //******************************************************************************
    //******************************************************************************
    // VEC3F
    //******************************************************************************
    //******************************************************************************
    template <> inline bool Box<Vec3f>::isEmpty() const
    {
        return (min.x > max.x || min.y > max.y || min.z > max.z);
    }

    //******************************************************************************
    template <> inline bool Box<Vec3f>::isInfinite() const
    {
        static const float minf = Math<float>::min();
        static const float maxf = Math<float>::max();
        return (min.x == minf && min.y == minf && min.z == minf && max.x == maxf
                && max.y == maxf && max.z == maxf);
    }

    //******************************************************************************
    template <> inline size_t Box<Vec3f>::majorAxis() const
    {
        float v0 = Math<float>::abs(max.x - min.x);
        float v1 = Math<float>::abs(max.y - min.y);
        float v2 = Math<float>::abs(max.z - min.z);
        return v0 > v1 ? (v0 > v2 ? 0 : 2) : (v1 > v2 ? 1 : 2);
    }

    //******************************************************************************
    template <> Vec3f Box<Vec3f>::size() const;

    //******************************************************************************
    template <> float Box<Vec3f>::size(Box<Vec3f>::size_type i) const;

    //******************************************************************************
    template <> void Box<Vec3f>::extendBy(const Vec3f& v);

    //******************************************************************************
    template <> void Box<Vec3f>::extendBy(const Box<Vec3f>& b);

    //******************************************************************************
    template <> inline bool Box<Vec3f>::intersects(const Vec3f& v) const
    {
        return (v.x >= min.x && v.x <= max.x && v.y >= min.y && v.y <= max.y
                && v.z >= min.z && v.z <= max.z);
    }

    //******************************************************************************
    template <> bool Box<Vec3f>::intersects(const Vec3f& p, float radius) const;

    //******************************************************************************
    template <>
    bool Box<Vec3f>::intersects(const Vec3f& origin, const Vec3f& dir) const;

    //******************************************************************************
    template <> bool Box<Vec3f>::intersects(const Box<Vec3f>& b) const;

    //******************************************************************************
    template <>
    bool Box<Vec3f>::closestInteriorPoint(const Vec3f& pt, Vec3f& ret) const;

    //******************************************************************************
    template <> Box<Vec3f>& Box<Vec3f>::operator*=(const Vec3f& rhs);

    //******************************************************************************
    template <> Box<Vec3f>& Box<Vec3f>::operator/=(const Vec3f& rhs);

    //******************************************************************************
    // template <>
    inline Box<Vec3f> intersection(const Box<Vec3f>& a, const Box<Vec3f>& b)
    {
        if (a.intersects(b))
        {
            Vec3f min;
            Vec3f max;

            // X
            min.x = std::max(a.min.x, b.min.x);
            max.x = std::min(a.max.x, b.max.x);

            // Y
            min.y = std::max(a.min.y, b.min.y);
            max.y = std::min(a.max.y, b.max.y);

            // Z
            min.z = std::max(a.min.z, b.min.z);
            max.z = std::min(a.max.z, b.max.z);

            return Box<Vec3f>(min, max);
        }
        else
        {
            return Box<Vec3f>();
        }
    }

    //******************************************************************************
    //******************************************************************************
    // VEC3D
    //******************************************************************************
    //******************************************************************************
    template <> inline bool Box<Vec3d>::isEmpty() const
    {
        return (min.x > max.x || min.y > max.y || min.z > max.z);
    }

    //******************************************************************************
    template <> inline bool Box<Vec3d>::isInfinite() const
    {
        static const double minf = Math<double>::min();
        static const double maxf = Math<double>::max();
        return (min.x == minf && min.y == minf && min.z == minf && max.x == maxf
                && max.y == maxf && max.z == maxf);
    }

    //******************************************************************************
    template <> inline size_t Box<Vec3d>::majorAxis() const
    {
        double v0 = Math<double>::abs(max.x - min.x);
        double v1 = Math<double>::abs(max.y - min.y);
        double v2 = Math<double>::abs(max.z - min.z);
        return v0 > v1 ? (v0 > v2 ? 0 : 2) : (v1 > v2 ? 1 : 2);
    }

    //******************************************************************************
    template <> Vec3d Box<Vec3d>::size() const;

    //******************************************************************************
    template <> double Box<Vec3d>::size(Box<Vec3d>::size_type i) const;

    //******************************************************************************
    template <> void Box<Vec3d>::extendBy(const Vec3d& v);

    //******************************************************************************
    template <> void Box<Vec3d>::extendBy(const Box<Vec3d>& b);

    //******************************************************************************
    template <> inline bool Box<Vec3d>::intersects(const Vec3d& v) const
    {
        return (v.x >= min.x && v.x <= max.x && v.y >= min.y && v.y <= max.y
                && v.z >= min.z && v.z <= max.z);
    }

    //******************************************************************************
    template <>
    bool Box<Vec3d>::intersects(const Vec3d& p, double radius) const;

    //******************************************************************************
    template <>
    bool Box<Vec3d>::intersects(const Vec3d& origin, const Vec3d& dir) const;

    //******************************************************************************
    template <> bool Box<Vec3d>::intersects(const Box<Vec3d>& b) const;

    //******************************************************************************
    template <>
    bool Box<Vec3d>::closestInteriorPoint(const Vec3d& pt, Vec3d& ret) const;

    //******************************************************************************
    template <> Box<Vec3d>& Box<Vec3d>::operator*=(const Vec3d& rhs);

    //******************************************************************************
    template <> Box<Vec3d>& Box<Vec3d>::operator/=(const Vec3d& rhs);

    //******************************************************************************
    // template <>
    inline Box<Vec3d> intersection(const Box<Vec3d>& a, const Box<Vec3d>& b)
    {
        if (a.intersects(b))
        {
            Vec3d min;
            Vec3d max;

            // X
            min.x = std::max(a.min.x, b.min.x);
            max.x = std::min(a.max.x, b.max.x);

            // Y
            min.y = std::max(a.min.y, b.min.y);
            max.y = std::min(a.max.y, b.max.y);

            // Z
            min.z = std::max(a.min.z, b.min.z);
            max.z = std::min(a.max.z, b.max.z);

            return Box<Vec3d>(min, max);
        }
        else
        {
            return Box<Vec3d>();
        }
    }

} // End namespace TwkMath

#endif
