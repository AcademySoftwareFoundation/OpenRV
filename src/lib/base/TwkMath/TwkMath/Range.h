//******************************************************************************
// Copyright (c) 2001 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkMathRange_h_
#define _TwkMathRange_h_

#include <TwkMath/Math.h>
#include <sys/types.h>
#include <assert.h>
#include <algorithm>

namespace TwkMath
{

    //******************************************************************************
    // RANGE TYPE
    // Contains a min and max value.
    // If min is greater or equal to max, range is considered "empty".
    template <typename T> class Range
    {
    public:
        //**************************************************************************
        // TYPEDEFS
        //**************************************************************************
        typedef T value_type;
        typedef size_t size_type;

        //**************************************************************************
        // DATA
        //**************************************************************************
        T min;
        T max;

        //**************************************************************************
        // CONSTRUCTORS
        //**************************************************************************
        // Default constructor.
        // Default range is empty.
        Range()
            : min(Math<T>::max())
            , max(Math<T>::min())
        {
        }

        // Single value constructor, which sets both min and max to the
        // same value.
        explicit Range(const T& val)
            : min(val)
            , max(val)
        {
        }

        // Two value constructor, which sets min & max directly.
        Range(const T& mn, const T& mx)
            : min(mn)
            , max(mx)
        {
        }

        // Non-similar type copy constructor
        template <typename T2>
        Range(const Range<T2>& copy)
            : min(copy.min)
            , max(copy.max)
        {
        }

        // Same type copy constructor
        Range(const Range<T>& copy)
            : min(copy.min)
            , max(copy.max)
        {
        }

        //**************************************************************************
        // ASSIGNMENT OPERATORS
        //**************************************************************************
        // Single valued assignment - both min & max set equal to val.
        Range<T>& operator=(const T& val);

        // Non-equivalent type copy assignment
        template <typename T2> Range<T>& operator=(const Range<T2>& copy)
        {
            min = copy.min;
            max = copy.max;
            return *this;
        }

        // Equivalent type specialization of above
        Range<T>& operator=(const Range<T>& copy)
        {
            min = copy.min;
            max = copy.max;
            return *this;
        }

        //**************************************************************************
        // DATA ACCESS OPERATORS
        //**************************************************************************
        void set(const T& mn, const T& mx)
        {
            min = mn;
            max = mx;
        }

        //**************************************************************************
        // FUNCTIONS
        //**************************************************************************
        // Size of the range
        T size() const;

        // Center of the range
        T center() const;

        // Is the range empty?
        bool isEmpty() const;

        // Make the range empty
        void makeEmpty();

        // Is the range infinite?
        bool isInfinite() const;

        // Make the range infinite
        void makeInfinite();

        // Extend the range's boundaries to include the value or range.
        void extendBy(const T& v);
        void extendBy(const Range<T>& r);

        // Does this range intersect another value or range?
        bool intersects(const T& v) const;
        bool intersects(const Range<T>& r) const;

        // Find the closest point INSIDE the range, returned by reference.
        // If the point is inside the range, "true" is returned.
        // Otherwise false is returned
        bool closestInteriorPoint(const T& pt, T& ret) const;

        //**************************************************************************
        // ARITHMETIC OPERATORS
        //**************************************************************************
        // Arithmetic operations between ranges and other ranges are not
        // defined. Though the actual +-* / are fairly consistent, it is not
        // possible to globally decide for comparison operators whether to use
        // interval arithmetic & inclusion functions or to compare by element.
        // Therefore, we reserve the "interval" class for interval arithmetic
        // and specifically forbid arithmetic operations between ranges.
        // Arithmetic operations between ranges and scalars applies the
        // operation evenly across min & max, reordering afterwards if
        // necessary.
        Range<T>& operator+=(const T& rhs);
        Range<T>& operator-=(const T& rhs);
        Range<T>& operator*=(const T& rhs);
        Range<T>& operator/=(const T& rhs);
    };

    //*****************************************************************************
    // TYPEDEFS
    //*****************************************************************************
    typedef Range<char> Rangec;
    typedef Range<unsigned char> Rangeuc;
    typedef Range<short> Ranges;
    typedef Range<unsigned short> Rangeus;
    typedef Range<int> Rangei;
    typedef Range<unsigned int> Rangeui;
    typedef Range<long> Rangel;
    typedef Range<unsigned long> Rangeul;
    typedef Range<float> Rangef;
    typedef Range<double> Ranged;

    //******************************************************************************
    // TEMPLATE AND INLINE FUNCTIONS
    //******************************************************************************
    template <typename T> inline Range<T>& Range<T>::operator=(const T& val)
    {
        min = val;
        max = val;
        return *this;
    }

    //******************************************************************************
    template <typename T> inline bool Range<T>::isEmpty() const
    {
        return (bool)(min > max);
    }

    //******************************************************************************
    template <typename T> inline void Range<T>::makeEmpty()
    {
        min = Math<T>::max();
        max = Math<T>::min();
    }

    //******************************************************************************
    template <typename T> inline bool Range<T>::isInfinite() const
    {
        if (min == Math<T>::min() && max == Math<T>::max())
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    //******************************************************************************
    template <typename T> inline void Range<T>::makeInfinite()
    {
        min = Math<T>::min();
        max = Math<T>::max();
    }

    //******************************************************************************
    template <typename T> T Range<T>::size() const
    {
        if (isEmpty())
        {
            return (T)0;
        }
        else if (isInfinite())
        {
            // This is not correct, but
            // necessary to avoid overflow
            // In normal use cases it should be correct enough.
            return Math<T>::max();
        }
        else
        {
            // Technically, this SHOULD be:
            // max - min + Math<T>::epsilon()
            // or max - min + std::numeric_limits<T>::epsilon()
            // However, some while back I changed it to this
            // to fix a problem, and now I can't remember what
            // that problem was. D'oh!
            if (Math<T>::isFloat())
            {
                return max - min;
            }
            else
            {
                return max - min + 1;
            }
        }
    }

    //******************************************************************************
    template <typename T> inline T Range<T>::center() const
    {
        // There's no good way of handling this
        // for empty and infinite ranges.
        if (isEmpty() || isInfinite())
        {
            return (T)0;
        }
        else
        {
            return (min + max) / (T)2;
        }
    }

    //*****************************************************************************
    template <typename T> void Range<T>::extendBy(const T& v)
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
            if (v < min)
            {
                min = v;
            }
            else if (v > max)
            {
                max = v;
            }
        }
    }

    //*****************************************************************************
    template <typename T> void Range<T>::extendBy(const Range<T>& b)
    {
        if (b.isEmpty() || isInfinite())
        {
            // Do nothing
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
        }
        else
        {
            min = STD_MIN(min, b.min);
            max = STD_MAX(max, b.max);
        }
    }

    //*****************************************************************************
    template <typename T> bool Range<T>::intersects(const T& v) const
    {
        // Don't need to check empty or infinite
        // Because the < > checks do so automatically.
        if (v < min || v > max)
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    //*****************************************************************************
    template <typename T> bool Range<T>::intersects(const Range<T>& b) const
    {
        // Empty/Infinite checks necessary, otherwise
        // Empty intersects Infinity returns true.
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
            if (b.max < min || b.min > max)
            {
                return false;
            }
            else
            {
                return true;
            }
        }
    }

    //*****************************************************************************
    template <typename T>
    bool Range<T>::closestInteriorPoint(const T& pt, T& ret) const
    {
        // We don't have to check empty or infinite here
        // because the check below implicitly does so.
        bool interior = true;
        if (pt < min)
        {
            interior = false;
            ret = min;
        }
        else if (pt > max)
        {
            interior = false;
            ret = max;
        }
        else
        {
            ret = pt;
        }
        return interior;
    }

    //******************************************************************************
    template <typename T> inline Range<T>& Range<T>::operator+=(const T& rhs)
    {
        if (!isEmpty() && !isInfinite())
        {
            min += rhs;
            max += rhs;
        }
        return *this;
    }

    //******************************************************************************
    template <typename T> inline Range<T>& Range<T>::operator-=(const T& rhs)
    {
        if (!isEmpty() && !isInfinite())
        {
            min -= rhs;
            max -= rhs;
        }
        return *this;
    }

    //******************************************************************************
    template <typename T> inline Range<T>& Range<T>::operator*=(const T& rhs)
    {
        // Note that multiplication can switch the sense of min & max,
        // hence the swap.
        if (!isEmpty() && !isInfinite())
        {
            min *= rhs;
            max *= rhs;
            if (min > max)
            {
                std::swap(min, max);
            }
        }
        return *this;
    }

    //******************************************************************************
    template <typename T> inline Range<T>& Range<T>::operator/=(const T& rhs)
    {
        // Note that division can switch the sense of min & max,
        // hence the swap.
        if (!isEmpty() && !isInfinite())
        {
            assert(rhs != (T)0);
            min /= rhs;
            max /= rhs;
            if (min > max)
            {
                std::swap(min, max);
            }
        }
        return *this;
    }

    //******************************************************************************
    //******************************************************************************
    // FUNCTIONS WHICH OPERATE ON RANGES
    //******************************************************************************
    //******************************************************************************
    template <typename T> inline T size(const Range<T>& r) { return r.size(); }

    //******************************************************************************
    template <typename T> inline T center(const Range<T>& r)
    {
        return r.center();
    }

    //******************************************************************************
    template <typename T> inline bool isEmpty(const Range<T>& r)
    {
        return r.isEmpty();
    }

    //******************************************************************************
    template <typename T> inline void makeEmpty(Range<T>& r) { r.makeEmpty(); }

    //******************************************************************************
    template <typename T> inline bool isInfinite(const Range<T>& r)
    {
        return r.isInfinite();
    }

    //******************************************************************************
    template <typename T> inline void makeInfinite(Range<T>& r)
    {
        r.makeInfinite();
    }

    //******************************************************************************
    template <typename T> inline void extendBy(Range<T>& r, const T& v)
    {
        r.extendBy(v);
    }

    //******************************************************************************
    template <typename T> inline void extendBy(Range<T>& r, const Range<T>& r2)
    {
        r.extendBy(r2);
    }

    //******************************************************************************
    template <typename T> inline bool intersects(const Range<T>& r, const T& v)
    {
        return r.intersects(v);
    }

    //******************************************************************************
    template <typename T>
    inline bool intersects(const Range<T>& r, const Range<T>& r2)
    {
        return r.intersects(r2);
    }

    //******************************************************************************
    template <typename T>
    inline Range<T> intersection(const Range<T>& a, const Range<T>& b)
    {
        if (a.intersects(b))
        {
            return Range<T>(STD_MAX(a.min, b.min), STD_MIN(a.max, b.max));
        }
        else
        {
            return Range<T>();
        }
    }

    //******************************************************************************
    template <typename T>
    inline bool closestInteriorPoint(const Range<T>& r, const T& pt, T& ret)
    {
        return r.closestInteriorPoint(pt, ret);
    }

    //******************************************************************************
    //******************************************************************************
    // ARITHMETIC OPERATORS
    //******************************************************************************
    //******************************************************************************
    template <typename T>
    inline Range<T> operator+(const T& a, const Range<T>& b)
    {
        if (b.isEmpty() || b.isInfinite())
        {
            return b;
        }
        else
        {
            return Range<T>(a + b.min, a + b.max);
        }
    }

    //******************************************************************************
    template <typename T>
    inline Range<T> operator+(const Range<T>& a, const T& b)
    {
        if (a.isEmpty() || a.isInfinite())
        {
            return a;
        }
        else
        {
            return Range<T>(a.min + b, a.max + b);
        }
    }

    //******************************************************************************
    template <typename T>
    inline Range<T> operator-(const T& a, const Range<T>& b)
    {
        if (b.isEmpty() || b.isInfinite())
        {
            return b;
        }
        else
        {
            return Range<T>(a - b.min, a - b.max);
        }
    }

    //******************************************************************************
    template <typename T>
    inline Range<T> operator-(const Range<T>& a, const T& b)
    {
        if (a.isEmpty() || a.isInfinite())
        {
            return a;
        }
        else
        {
            return Range<T>(a.min - b, a.max - b);
        }
    }

    //******************************************************************************
    template <typename T>
    inline Range<T> operator*(const T& a, const Range<T>& b)
    {
        if (b.isEmpty() || b.isInfinite())
        {
            return b;
        }
        else
        {
            const T abmin = a * b.min;
            const T abmax = a * b.max;
            if (abmin < abmax)
            {
                return Range<T>(abmin, abmax);
            }
            else
            {
                return Range<T>(abmax, abmin);
            }
        }
    }

    //******************************************************************************
    template <typename T>
    inline Range<T> operator*(const Range<T>& a, const T& b)
    {
        if (a.isEmpty() || a.isInfinite())
        {
            return a;
        }
        else
        {
            const T abmin = a.min * b;
            const T abmax = a.max * b;
            if (abmin < abmax)
            {
                return Range<T>(abmin, abmax);
            }
            else
            {
                return Range<T>(abmax, abmin);
            }
        }
    }

    //******************************************************************************
    // This operation doesn't really make any sense, but since
    // there's a consistent way of performing it, I'll put it here anyway
    template <typename T>
    inline Range<T> operator/(const T& a, const Range<T>& b)
    {
        assert(b.min != (T)0);
        assert(b.max != (T)0);
        if (b.isEmpty() || b.isInfinite())
        {
            return b;
        }
        else
        {
            const T abmin = a / b.min;
            const T abmax = a / b.max;
            if (abmin < abmax)
            {
                return Range<T>(abmin, abmax);
            }
            else
            {
                return Range<T>(abmax, abmin);
            }
        }
    }

    //******************************************************************************
    template <typename T>
    inline Range<T> operator/(const Range<T>& a, const T& b)
    {
        assert(b != (T)0);
        if (a.isEmpty() || a.isInfinite())
        {
            return a;
        }
        else
        {
            const T abmin = a.min / b;
            const T abmax = a.max / b;
            if (abmin < abmax)
            {
                return Range<T>(abmin, abmax);
            }
            else
            {
                return Range<T>(abmax, abmin);
            }
        }
    }

    //******************************************************************************
    //******************************************************************************
    // COMPARISON OPERATORS
    //******************************************************************************
    //******************************************************************************
    template <typename T, typename T2>
    inline bool operator==(const Range<T>& a, const Range<T2>& b)
    {
        if (a.isEmpty() || a.isInfinite() || b.isEmpty() || b.isInfinite())
        {
            // Empty and infinite are not actual quantities,
            // but rather concepts, and cannot be equal.
            return false;
        }
        else
        {
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
    template <typename T, typename T2>
    bool operator!=(const Range<T>& a, const Range<T2>& b)
    {
        if (a.isEmpty() || a.isInfinite() || b.isEmpty() || b.isInfinite())
        {
            // Empty and infinite are not actual quantities,
            // but rather concepts, and are always inequal
            return true;
        }
        else
        {
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

#endif
