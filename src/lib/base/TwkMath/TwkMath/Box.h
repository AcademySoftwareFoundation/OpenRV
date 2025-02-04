//******************************************************************************
// Copyright (c) 2001 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkMathBox_h_
#define _TwkMathBox_h_

#include <TwkMath/Vec2.h>
#include <TwkMath/Vec3.h>
#include <TwkMath/Vec4.h>
#include <TwkMath/Math.h>
#include <sys/types.h>
#include <assert.h>

namespace TwkMath
{

    //******************************************************************************
    // CLASS Box
    // Similar to a range, but from a vector to a vector rather
    // than from a scalar to a scalar.
    template <class VEC> class Box
    {
    public:
        //**************************************************************************
        // TYPEDEFS
        //**************************************************************************
        typedef VEC vector_type;
        typedef typename VEC::value_type value_type;
        typedef size_t size_type;

        //**************************************************************************
        // DATA
        //**************************************************************************
        VEC min;
        VEC max;

        //**************************************************************************
        // CONSTRUCTORS
        //**************************************************************************
        // Default constructor.
        // Default box is empty.
        Box()
            : min(Math<value_type>::max())
            , max(Math<value_type>::min())
        {
        }

        // Single value constructor, which sets all values of min and max to
        // the value
        explicit Box(const value_type& val)
            : min(val)
            , max(val)
        {
        }

        // Single vector constructor, which sets min & max to that vector.
        explicit Box(const VEC& vec)
            : min(vec)
            , max(vec)
        {
        }

        // Two vector constructor, which sets min & max directly
        Box(const VEC& mn, const VEC& mx)
            : min(mn)
            , max(mx)
        {
        }

        // Non-similar type copy constructor
        template <class VEC2>
        Box(const Box<VEC2>& copy)
            : min(VEC2(copy.min))
            , max(VEC2(copy.max))
        {
        }

        // Same type copy constructor
        explicit Box(const Box<VEC>& copy)
            : min(copy.min)
            , max(copy.max)
        {
        }

        //**************************************************************************
        // ASSIGNMENT OPERATORS
        //**************************************************************************
        // Single value assignment - both min & max set equal to val.
        Box<VEC>& operator=(const value_type& val);

        // Single vector assignment - both min & max set equal to vec.
        Box<VEC>& operator=(const VEC& vec);

        // Non-equivalent type copy assignment
        template <typename VEC2> Box<VEC>& operator=(const Box<VEC2>& copy)
        {
            min = copy.min;
            max = copy.max;
            return *this;
        }

        // Equivalent type specialization of above
        Box<VEC>& operator=(const Box<VEC>& copy)
        {
            min = copy.min;
            max = copy.max;
            return *this;
        }

        //**************************************************************************
        // DIMENSION FUNCTION
        //**************************************************************************
        static size_type dimension() { return VEC::dimension(); }

        //**************************************************************************
        // DATA ACCESS OPERATORS
        //**************************************************************************
        void set(const VEC& mn, const VEC& mx)
        {
            min = mn;
            max = mx;
        }

        //**************************************************************************
        // FUNCTIONS
        //**************************************************************************
        // Size of the box
        VEC size() const;

        // Size of a particular dimension
        value_type size(size_type i) const;

        // Center of the box
        VEC center() const;

        // Returns dimension of greatest size
        size_t majorAxis() const;

        // Returns dimension of smallest size
        size_t minorAxis() const;

        // Center along a particular dimension
        value_type center(size_type i) const;

        // Is the box empty?
        bool isEmpty() const;

        // Make the box empty.
        void makeEmpty();

        // Is the box infinite?
        bool isInfinite() const;

        // Make the box infinite
        void makeInfinite();

        // Extend the box's boundaries to include the vector or box.
        void extendBy(const VEC& v);
        void extendBy(const Box<VEC>& b);

        // Does this box intersect another vector, box, or sphere?
        bool intersects(const VEC& v) const;
        bool intersects(const Box<VEC>& b) const;
        bool intersects(const VEC& center, value_type radius) const;
        bool intersects(const VEC& origin, const VEC& dir) const;

        // Find the closest point INSIDE the box, returned by reference.
        // If the point is inside the box, "true" is returned.
        // Otherwise, false is returned.
        bool closestInteriorPoint(const VEC& pt, VEC& ret) const;

        //**************************************************************************
        // ARITHMETIC OPERATORS
        //**************************************************************************
        // Arithmetic operations between boxes and other boxes are not defined.
        // Operations between boxes and vectors apply the operation evenly
        // across min & max, reordering afterwards if necessary.
        Box<VEC>& operator+=(const value_type& rhs);
        Box<VEC>& operator+=(const VEC& rhs);
        Box<VEC>& operator-=(const value_type& rhs);
        Box<VEC>& operator-=(const VEC& rhs);
        Box<VEC>& operator*=(const value_type& rhs);
        Box<VEC>& operator*=(const VEC& rhs);
        Box<VEC>& operator/=(const value_type& rhs);
        Box<VEC>& operator/=(const VEC& rhs);
    };

    //******************************************************************************
    // TYPEDEFS
    //******************************************************************************
    typedef Box<Vec2<char>> Box2c;
    typedef Box<Vec2<unsigned char>> Box2uc;
    typedef Box<Vec2<short>> Box2s;
    typedef Box<Vec2<unsigned short>> Box2us;
    typedef Box<Vec2<int>> Box2i;
    typedef Box<Vec2<unsigned int>> Box2ui;
    typedef Box<Vec2<long>> Box2l;
    typedef Box<Vec2<unsigned long>> Box2ul;
    typedef Box<Vec2<float>> Box2f;
    typedef Box<Vec2<double>> Box2d;

    typedef Box<Vec3<char>> Box3c;
    typedef Box<Vec3<unsigned char>> Box3uc;
    typedef Box<Vec3<short>> Box3s;
    typedef Box<Vec3<unsigned short>> Box3us;
    typedef Box<Vec3<int>> Box3i;
    typedef Box<Vec3<unsigned int>> Box3ui;
    typedef Box<Vec3<long>> Box3l;
    typedef Box<Vec3<unsigned long>> Box3ul;
    typedef Box<Vec3<float>> Box3f;
    typedef Box<Vec3<double>> Box3d;

    typedef Box<Vec4<char>> Box4c;
    typedef Box<Vec4<unsigned char>> Box4uc;
    typedef Box<Vec4<short>> Box4s;
    typedef Box<Vec4<unsigned short>> Box4us;
    typedef Box<Vec4<int>> Box4i;
    typedef Box<Vec4<unsigned int>> Box4ui;
    typedef Box<Vec4<long>> Box4l;
    typedef Box<Vec4<unsigned long>> Box4ul;
    typedef Box<Vec4<float>> Box4f;
    typedef Box<Vec4<double>> Box4d;

    //******************************************************************************
    // TEMPLATE AND INLINE FUNCTIONS
    //******************************************************************************
    template <class VEC>
    inline Box<VEC>&
    Box<VEC>::operator=(const typename Box<VEC>::value_type& val)
    {
        min = val;
        max = val;
        return *this;
    }

    //******************************************************************************
    template <class VEC> inline Box<VEC>& Box<VEC>::operator=(const VEC& vec)
    {
        min = vec;
        max = vec;
        return *this;
    }

    //******************************************************************************
    template <class VEC> inline bool Box<VEC>::isEmpty() const
    {
        for (size_type i = 0; i < dimension(); ++i)
        {
            if (min[i] > max[i])
            {
                return true;
            }
        }
        return false;
    }

    //******************************************************************************
    template <class VEC> inline void Box<VEC>::makeEmpty()
    {
        min = Math<value_type>::max();
        max = Math<value_type>::min();
    }

    //******************************************************************************
    template <class VEC> inline bool Box<VEC>::isInfinite() const
    {
        for (size_type i = 0; i < dimension(); ++i)
        {
            if (min[i] != Math<value_type>::min()
                || max[i] != Math<value_type>::max())
            {
                return false;
            }
        }
        return true;
    }

    //******************************************************************************
    template <class VEC> inline void Box<VEC>::makeInfinite()
    {
        min = Math<value_type>::min();
        max = Math<value_type>::max();
    }

    //******************************************************************************
    template <class VEC> size_t Box<VEC>::majorAxis() const
    {
        VEC d = max - min;
        size_t dim = 0;
        value_type v = Math<value_type>::abs(d[0]);

        for (size_t i = 1; i < dimension(); i++)
        {
            value_type t = Math<value_type>::abs(d[i]);
            if (t > v)
            {
                v = t;
                dim = i;
            }
        }

        return dim;
    }

    //******************************************************************************
    template <class VEC> size_t Box<VEC>::minorAxis() const
    {
        VEC d = max - min;
        size_t dim = 0;
        value_type v = Math<value_type>::abs(d[0]);

        for (size_t i = 1; i < dimension(); i++)
        {
            value_type t = Math<value_type>::abs(d[i]);
            if (t < v)
            {
                v = t;
                dim = i;
            }
        }

        return dim;
    }

    //******************************************************************************
    template <class VEC> VEC Box<VEC>::size() const
    {
        if (isEmpty())
        {
            return VEC((value_type)0);
        }
        else if (isInfinite())
        {
            // This is not correct, but
            // necessary to avoid overflow.
            // In normal use cases is should be close enough.
            return VEC(Math<value_type>::max());
        }
        else
        {
            if (Math<value_type>::isFloat())
            {
                return max - min;
            }
            else
            {
                return max - min + (value_type)1;
            }
        }
    }

    //******************************************************************************
    template <class VEC>
    typename Box<VEC>::value_type
    Box<VEC>::size(typename Box<VEC>::size_type i) const
    {
        if (min[i] > max[i])
        {
            // Empty
            return (value_type)0;
        }
        else if (min[i] == Math<value_type>::min()
                 && max[i] == Math<value_type>::max())
        {
            // infinite
            return Math<value_type>::max();
        }
        else
        {
            if (Math<value_type>::isFloat())
            {
                return max[i] - min[i];
            }
            else
            {
                return max[i] - min[i] + (value_type)1;
            }
        }
    }

    //******************************************************************************
    template <class VEC> inline VEC Box<VEC>::center() const
    {
        // There's no good way of handling this
        // for empty and infinite ranges.
        if (isEmpty() || isInfinite())
        {
            return VEC((value_type)0);
        }
        else
        {
            return (min + max) / (value_type)2;
        }
    }

    //******************************************************************************
    template <class VEC>
    inline typename Box<VEC>::value_type
    Box<VEC>::center(typename Box<VEC>::size_type i) const
    {
        if ((min[i] > max[i])
            || (min[i] == Math<value_type>::min()
                && max[i] == Math<value_type>::max()))
        {
            return (value_type)0;
        }
        else
        {
            return (min[i] + max[i]) / (value_type)2;
        }
    }

    //******************************************************************************
    template <class VEC> void Box<VEC>::extendBy(const VEC& v)
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
            for (size_type i = 0; i < dimension(); ++i)
            {
                if (v[i] < min[i])
                {
                    min[i] = v[i];
                }
                else if (v[i] > max[i])
                {
                    max[i] = v[i];
                }
            }
        }
    }

    //******************************************************************************
    template <class VEC> void Box<VEC>::extendBy(const Box<VEC>& b)
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
            for (size_type i = 0; i < dimension(); ++i)
            {
                min[i] = STD_MIN(min[i], b.min[i]);
                max[i] = STD_MAX(max[i], b.max[i]);
            }
        }
    }

    //******************************************************************************
    template <class VEC> bool Box<VEC>::intersects(const VEC& v) const
    {
        for (size_type i = 0; i < dimension(); ++i)
        {
            if (v[i] < min[i] || v[i] > max[i])
            {
                return false;
            }
        }
        return true;
    }

    template <> inline bool Box<Vec3f>::intersects(const Vec3f& v) const
    {
        return !(v[0] < min[0] || v[0] > max[0] || v[1] < min[1]
                 || v[1] > max[1] || v[2] < min[2] || v[2] > max[2]);
    }

    //******************************************************************************
    template <class VEC>
    bool Box<VEC>::intersects(const VEC& p,
                              typename Box<VEC>::value_type radius) const
    {
        //
        //	Ripped off from graphics gems with improvements
        //

        value_type d = 0;

        for (size_t i = 0; i < dimension(); i++)
        {
            value_type pv = p[i];
            value_type minv = min[i];
            value_type maxv = max[i];

            if (pv < minv)
            {
                value_type s = pv - minv;
                d += s * s;
            }
            else if (pv > maxv)
            {
                value_type s = pv - maxv;
                d += s * s;
            }
        }

        return d <= (radius * radius);
    }

    //******************************************************************************
    template <class VEC>
    bool Box<VEC>::intersects(const VEC& origin, const VEC& dir) const
    {
        //
        //	Ripped off of GGEMS
        //

        const value_type LEFT = value_type(-1);
        const value_type MIDDLE = value_type(0);
        const value_type RIGHT = value_type(1);

        bool inside = true;
        VEC quadrant;
        VEC maxT;
        VEC candidatePlane;

        for (int i = 0; i < dimension(); i++)
        {
            if (origin[i] < min[i])
            {
                quadrant[i] = LEFT;
                candidatePlane[i] = min[i];
                inside = false;
            }
            else if (origin[i] > max[i])
            {
                quadrant[i] = RIGHT;
                candidatePlane[i] = max[i];
                inside = false;
            }
            else
            {
                quadrant[i] = MIDDLE;
            }
        }

        if (inside)
            return true;

        for (int i = 0; i < dimension(); i++)
        {
            if (quadrant[i] != MIDDLE && dir[i] != value_type(0.))
            {
                maxT[i] = (candidatePlane[i] - origin[i]) / dir[i];
            }
            else
            {
                maxT[i] = -1.;
            }
        }

        int whichPlane = 0;

        for (int i = 1; i < dimension(); i++)
        {
            if (maxT[whichPlane] < maxT[i])
            {
                whichPlane = i;
            }
        }

        if (maxT[whichPlane] < 0.)
            return false;

        for (int i = 0; i < dimension(); i++)
        {
            if (whichPlane != i)
            {
                value_type x = origin[i] + maxT[whichPlane] * dir[i];
                if (x < min[i] || x > max[i])
                    return false;
            }
        }

        return true;
    }

    //******************************************************************************
    template <class VEC> bool Box<VEC>::intersects(const Box<VEC>& b) const
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
            for (size_type i = 0; i < dimension(); ++i)
            {
                if (b.max[i] < min[i] || b.min[i] > max[i])
                {
                    return false;
                }
            }
            return true;
        }
    }

    //******************************************************************************
    template <class VEC>
    bool Box<VEC>::closestInteriorPoint(const VEC& pt, VEC& ret) const
    {
        // Don't have to check empty or infinite here
        // because the check below implicitly does so.
        bool interior = true;
        for (size_type i = 0; i < dimension(); ++i)
        {
            if (pt[i] < min[i])
            {
                interior = false;
                ret[i] = min[i];
            }
            else if (pt[i] > max[i])
            {
                interior = false;
                ret[i] = max[i];
            }
            else
            {
                ret[i] = pt[i];
            }
        }
        return interior;
    }

    //******************************************************************************
    template <class VEC>
    Box<VEC>& Box<VEC>::operator+=(const typename Box<VEC>::value_type& rhs)
    {
        if (!isEmpty() && !isInfinite())
        {
            min += rhs;
            max += rhs;
        }
        return *this;
    }

    //******************************************************************************
    template <class VEC> Box<VEC>& Box<VEC>::operator+=(const VEC& rhs)
    {
        if (!isEmpty() && !isInfinite())
        {
            min += rhs;
            max += rhs;
        }
        return *this;
    }

    //******************************************************************************
    template <class VEC>
    Box<VEC>& Box<VEC>::operator-=(const typename Box<VEC>::value_type& rhs)
    {
        if (!isEmpty() && !isInfinite())
        {
            min -= rhs;
            max -= rhs;
        }
        return *this;
    }

    //******************************************************************************
    template <class VEC> Box<VEC>& Box<VEC>::operator-=(const VEC& rhs)
    {
        if (!isEmpty() && !isInfinite())
        {
            min -= rhs;
            max -= rhs;
        }
        return *this;
    }

    //******************************************************************************
    template <class VEC>
    Box<VEC>& Box<VEC>::operator*=(const typename Box<VEC>::value_type& rhs)
    {
        if (!isEmpty() && !isInfinite())
        {
            min *= rhs;
            max *= rhs;
            if (rhs < (value_type)0)
            {
                // Multiplication by rhs will switch the
                // sense, because rhs is negative.
                std::swap(min, max);
            }
        }
        return *this;
    }

    //******************************************************************************
    template <class VEC> Box<VEC>& Box<VEC>::operator*=(const VEC& rhs)
    {
        if (!isEmpty() && !isInfinite())
        {
            for (size_type i = 0; i < dimension(); ++i)
            {
                min[i] *= rhs[i];
                max[i] *= rhs[i];
                if (min[i] > max[i])
                {
                    std::swap(min[i], max[i]);
                }
            }
        }
        return *this;
    }

    //******************************************************************************
    template <class VEC>
    Box<VEC>& Box<VEC>::operator/=(const typename Box<VEC>::value_type& rhs)
    {
        assert(rhs != 0);
        if (!isEmpty() && !isInfinite())
        {
            min /= rhs;
            max /= rhs;
            if (rhs < (value_type)0)
            {
                // Multiplication by rhs will switch the
                // sense, because rhs is negative.
                std::swap(min, max);
            }
        }
        return *this;
    }

    //******************************************************************************
    template <class VEC> Box<VEC>& Box<VEC>::operator/=(const VEC& rhs)
    {
        if (!isEmpty() && !isInfinite())
        {
            for (size_type i = 0; i < dimension(); ++i)
            {
                assert(rhs[i] != 0);
                min[i] /= rhs[i];
                max[i] /= rhs[i];
                if (min[i] > max[i])
                {
                    std::swap(min[i], max[i]);
                }
            }
        }
        return *this;
    }

    //******************************************************************************
    //******************************************************************************
    // FUNCTIONS WHICH OPERATE ON BOXES
    //******************************************************************************
    //******************************************************************************
    template <class VEC>
    inline typename Box<VEC>::size_type dimension(const Box<VEC>& b)
    {
        return VEC::dimension();
    }

    //******************************************************************************
    template <class VEC> inline VEC size(const Box<VEC>& b) { return b.size(); }

    //******************************************************************************
    template <class VEC>
    inline typename Box<VEC>::value_type size(const Box<VEC>& b,
                                              typename Box<VEC>::size_type i)
    {
        return b.size(i);
    }

    //******************************************************************************
    template <class VEC> inline VEC center(const Box<VEC>& b)
    {
        return b.center();
    }

    //******************************************************************************
    template <class VEC>
    inline typename Box<VEC>::value_type center(const Box<VEC>& b,
                                                typename Box<VEC>::size_type i)
    {
        return b.center(i);
    }

    //******************************************************************************
    template <class VEC> inline bool isEmpty(const Box<VEC>& b)
    {
        return b.isEmpty();
    }

    //******************************************************************************
    template <class VEC> inline void makeEmpty(Box<VEC>& b) { b.makeEmpty(); }

    //******************************************************************************
    template <class VEC> inline bool isInfinite(const Box<VEC>& b)
    {
        return b.isInfinite();
    }

    //******************************************************************************
    template <class VEC> inline void makeInfinite(Box<VEC>& b)
    {
        b.makeInfinite();
    }

    //******************************************************************************
    template <class VEC> inline void extendBy(Box<VEC>& b, const VEC& v)
    {
        b.extendBy(v);
    }

    //******************************************************************************
    template <class VEC> inline void extendBy(Box<VEC>& b, const Box<VEC>& b2)
    {
        b.extendBy(b2);
    }

    //******************************************************************************
    template <class VEC> inline bool intersects(const Box<VEC>& b, const VEC& v)
    {
        return b.intersects(v);
    }

    //******************************************************************************
    template <class VEC>
    inline bool intersects(const Box<VEC>& b, const Box<VEC>& b2)
    {
        return b.intersects(b2);
    }

    //******************************************************************************
    template <class VEC>
    inline Box<VEC> intersection(const Box<VEC>& a, const Box<VEC>& b)
    {
        if (a.intersects(b))
        {
            VEC min;
            VEC max;
            for (typename Box<VEC>::size_type i = 0; i < Box<VEC>::dimension();
                 ++i)
            {
                min[i] = STD_MAX(a.min[i], b.min[i]);
                max[i] = STD_MIN(a.max[i], b.max[i]);
            }
            return Box<VEC>(min, max);
        }
        else
        {
            return Box<VEC>();
        }
    }

    //******************************************************************************
    template <class VEC>
    inline bool closestInteriorPoint(const Box<VEC>& b, const VEC& pt, VEC& ret)
    {
        return b.closestInteriorPoint(pt, ret);
    }

    //******************************************************************************
    //******************************************************************************
    // ARITHMETIC OPERATORS
    //******************************************************************************
    //******************************************************************************
    template <class VEC>
    Box<VEC> operator+(const typename Box<VEC>::value_type& a,
                       const Box<VEC>& b)
    {
        if (b.isEmpty() || b.isInfinite())
        {
            return b;
        }
        else
        {
            return Box<VEC>(a + b.min, a + b.max);
        }
    }

    //******************************************************************************
    template <class VEC> Box<VEC> operator+(const VEC& a, const Box<VEC>& b)
    {
        if (b.isEmpty() || b.isInfinite())
        {
            return b;
        }
        else
        {
            return Box<VEC>(a + b.min, a + b.max);
        }
    }

    //******************************************************************************
    template <class VEC>
    Box<VEC> operator+(const Box<VEC>& a,
                       const typename Box<VEC>::value_type& b)
    {
        if (a.isEmpty() || a.isInfinite())
        {
            return a;
        }
        else
        {
            return Box<VEC>(a.min + b, a.max + b);
        }
    }

    //******************************************************************************
    template <class VEC> Box<VEC> operator+(const Box<VEC>& a, const VEC& b)
    {
        if (a.isEmpty() || a.isInfinite())
        {
            return a;
        }
        else
        {
            return Box<VEC>(a.min + b, a.max + b);
        }
    }

    //******************************************************************************
    template <class VEC>
    Box<VEC> operator-(const typename Box<VEC>::value_type& a,
                       const Box<VEC>& b)
    {
        if (b.isEmpty() || b.isInfinite())
        {
            return b;
        }
        else
        {
            return Box<VEC>(a - b.min, a - b.max);
        }
    }

    //******************************************************************************
    template <class VEC> Box<VEC> operator-(const VEC& a, const Box<VEC>& b)
    {
        if (b.isEmpty() || b.isInfinite())
        {
            return b;
        }
        else
        {
            return Box<VEC>(a - b.min, a - b.max);
        }
    }

    //******************************************************************************
    template <class VEC>
    Box<VEC> operator-(const Box<VEC>& a,
                       const typename Box<VEC>::value_type& b)
    {
        if (a.isEmpty() || a.isInfinite())
        {
            return a;
        }
        else
        {
            return Box<VEC>(a.min - b, a.max - b);
        }
    }

    //******************************************************************************
    template <class VEC> Box<VEC> operator-(const Box<VEC>& a, const VEC& b)
    {
        if (a.isEmpty() || a.isInfinite())
        {
            return a;
        }
        else
        {
            return Box<VEC>(a.min - b, a.max - b);
        }
    }

    //******************************************************************************
    template <class VEC>
    inline Box<VEC> operator*(const typename Box<VEC>::value_type& a,
                              const Box<VEC>& b)
    {
        Box<VEC> c(b);
        return (c *= a);
    }

    //******************************************************************************
    template <class VEC>
    inline Box<VEC> operator*(const VEC& a, const Box<VEC>& b)
    {
        Box<VEC> c(b);
        return (c *= a);
    }

    //******************************************************************************
    template <class VEC>
    inline Box<VEC> operator*(const Box<VEC>& a,
                              const typename Box<VEC>::value_type& b)
    {
        Box<VEC> c(a);
        return (c *= b);
    }

    //******************************************************************************
    template <class VEC>
    inline Box<VEC> operator*(const Box<VEC>& a, const VEC& b)
    {
        Box<VEC> c(a);
        return (c *= b);
    }

    //******************************************************************************
    template <class VEC>
    inline Box<VEC> operator/(const typename Box<VEC>::value_type& a,
                              const Box<VEC>& b)
    {
        Box<VEC> c(b);
        return (c /= a);
    }

    //******************************************************************************
    template <class VEC>
    inline Box<VEC> operator/(const VEC& a, const Box<VEC>& b)
    {
        Box<VEC> c(b);
        return (c /= a);
    }

    //******************************************************************************
    template <class VEC>
    inline Box<VEC> operator/(const Box<VEC>& a,
                              const typename Box<VEC>::value_type& b)
    {
        Box<VEC> c(a);
        return (c /= b);
    }

    //******************************************************************************
    template <class VEC>
    inline Box<VEC> operator/(const Box<VEC>& a, const VEC& b)
    {
        Box<VEC> c(a);
        return (c /= b);
    }

    //******************************************************************************
    //******************************************************************************
    // COMPARISON OPERATORS
    //******************************************************************************
    //******************************************************************************
    template <class VEC, class VEC2>
    bool operator==(const Box<VEC>& a, const Box<VEC2>& b)
    {
        if (a.isEmpty() || a.isInfinite() || b.isEmpty() || b.isInfinite())
        {
            // These can never be equal.
            return false;
        }
        else
        {
            // These comparison operators provide a compile-time
            // check that VEC & VEC2 are comparable (have the same dimension..)
            if (a.min == b.min && a.max == b.max)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
    }

    //******************************************************************************
    template <class VEC, class VEC2>
    bool operator!=(const Box<VEC>& a, const Box<VEC2>& b)
    {
        if (a.isEmpty() || a.isInfinite() || b.isEmpty() || b.isInfinite())
        {
            // Empty and infinite are concepts, not quantities,
            // and thus are always inequal.
            return true;
        }
        else
        {
            // These comparison operators provide a compile-time
            // check that VEC & VEC2 are comparable (same dimension()...)
            if (a.min != b.min || a.max != b.max)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
    }

} // End namespace TwkMath

#if 0
//******************************************************************************
//******************************************************************************
// INCLUDED: SPECIALIZATIONS FOR COMMON TYPES!
//******************************************************************************
//******************************************************************************
#include <TwkMath/BoxSpec.h>
#endif

#endif
