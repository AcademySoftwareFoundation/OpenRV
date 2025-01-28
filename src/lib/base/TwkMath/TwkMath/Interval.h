//******************************************************************************
// Copyright (c) 2001-2002 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkMathInterval_h_
#define _TwkMathInterval_h_

#include <TwkMath/Math.h>
#include <algorithm>
#include <assert.h>

namespace TwkMath
{

    //******************************************************************************
    // ENUMS
    //******************************************************************************
    // Relational operators on intervals (such as ==, !=, <, etc) produce
    // intervals on the set {false, true}. Rather than express these as
    // intervals, we'll express them as enums defined as:
    // {false, false} => IVL_FALSE
    // {false, true} => IVL_MAYBE
    // {true, true} => IVL_TRUE
    enum BooleanInterval
    {
        IVL_FALSE = 0,
        IVL_TRUE = 1,
        IVL_MAYBE = 2
    };

    //******************************************************************************
    template <typename T> class Interval
    {
    public:
        //**************************************************************************
        // TYPEDEFS
        //**************************************************************************
        typedef T value_type;

        //**************************************************************************
        // DATA MEMBERS
        //**************************************************************************
        union
        {
            T lo;
            T min;
        };

        union
        {
            T hi;
            T max;
        };

        //**************************************************************************
        // PUTTING THIS FUNCTION HERE BECAUSE IT'S USED INLINE IMMEDIATELY BELOW
        // Make the interval valid (min <= max)
        void makeValid()
        {
            if (min > max)
            {
                std::swap(min, max);
            }
        }

        //**************************************************************************
        // CONSTRUCTORS
        //**************************************************************************
        // Default constructor
        // Default interval is zero
        Interval()
            : min((T)0)
            , max((T)0)
        {
        }

        // Single value constructor, which sets both min and max to
        // the same value
        explicit Interval(const T& val)
            : min(val)
            , max(val)
        {
        }

        // Two value constructor, which sets min and max directly
        Interval(const T& mn, const T& mx)
            : min(mn)
            , max(mx)
        {
            makeValid();
        }

        // Non similar type copy constructor
        template <typename T2>
        Interval(const Interval<T2>& copy)
            : min(copy.min)
            , max(copy.max)
        {
            makeValid();
        }

        // Same type copy constructor
        Interval(const Interval<T>& copy)
            : min(copy.min)
            , max(copy.max)
        {
            makeValid();
        }

        //**************************************************************************
        // ASSIGNMENT OPERATORS
        //**************************************************************************
        // Single valued assignment - both min & max set equal to val.
        Interval<T>& operator=(const T& val)
        {
            min = max = val;
            return *this;
        }

        // Non-equivalent type copy assignment
        template <typename T2> Interval<T>& operator=(const Interval<T2>& copy)
        {
            min = copy.min;
            max = copy.max;
            makeValid();
            return *this;
        }

        // Same type copy assignment
        Interval<T>& operator=(const Interval<T>& copy)
        {
            min = copy.min;
            max = copy.max;
            makeValid();
            return *this;
        }

        //**************************************************************************
        // DATA ACCESS OPERATORS
        //**************************************************************************
        void set(const T& val) { min = max = val; }

        void set(const T& mn, const T& mx)
        {
            min = mn;
            max = mx;
            makeValid();
        }

        //**************************************************************************
        // FUNCTIONS
        //**************************************************************************
        // Size of the interval
        T size() const;

        // Center of the interval
        T center() const;

        // Is the interval infinite?
        bool isInfinite() const;

        // make the interval infinite
        void makeInfinite();

        // Is the interval valid? (min <= max)
        bool isValid() const;

        // Does this interval include the value?
        bool intersects(const T& v) const;

        //**************************************************************************
        // ARITHMETIC FUNCTIONS
        //**************************************************************************
        Interval<T>& operator+=(const T& val);
        Interval<T>& operator+=(const Interval<T>& val);
        Interval<T>& operator-=(const T& val);
        Interval<T>& operator-=(const Interval<T>& val);
        Interval<T>& operator*=(const T& val);
        Interval<T>& operator*=(const Interval<T>& val);
        Interval<T>& operator/=(const T& val);
        Interval<T>& operator/=(const Interval<T>& val);
    };

    //*****************************************************************************
    // TYPEDEFS
    //*****************************************************************************
    typedef Interval<float> Ivlf;
    typedef Interval<double> Ivld;

    //******************************************************************************
    // TEMPLATE AND INLINE FUNCTIONS
    //******************************************************************************
    template <typename T> inline bool Interval<T>::isInfinite() const
    {
        return (bool)(min == Math<T>::min() && max == Math<T>::max());
    }

    //******************************************************************************
    template <typename T> inline void Interval<T>::makeInfinite()
    {
        min = Math<T>::min();
        max = Math<T>::max();
    }

    //******************************************************************************
    template <typename T> inline bool Interval<T>::isValid() const
    {
        return (bool)(min <= max);
    }

    //******************************************************************************
    template <typename T> T Interval<T>::size() const
    {
        assert(isValid());

        if (isInfinite())
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
    template <typename T> T Interval<T>::center() const
    {
        assert(isValid());

        // There's no good way of handling this
        // for empty and infinite ranges.
        if (isInfinite())
        {
            return (T)0;
        }
        else
        {
            return (min + max) / (T)2;
        }
    }

    //*****************************************************************************
    template <typename T> inline bool Interval<T>::intersects(const T& v) const
    {
        assert(isValid());

        // Don't need to check empty or infinite
        // Because the < > checks do so automatically.
        return (bool)(v >= min && v <= max);
    }

    //******************************************************************************
    template <typename T> Interval<T>& Interval<T>::operator+=(const T& val)
    {
        assert(isValid());

        if (!isInfinite())
        {
            min += val;
            max += val;
        }
        return *this;
    }

    //******************************************************************************
    template <typename T>
    Interval<T>& Interval<T>::operator+=(const Interval<T>& ivl)
    {
        assert(isValid() && ivl.isValid());

        if (!isInfinite())
        {
            if (ivl.isInfinite())
            {
                makeInfinite();
            }
            else
            {
                min += ivl.min;
                max += ivl.max;
            }
        }
        return *this;
    }

    //******************************************************************************
    template <typename T> Interval<T>& Interval<T>::operator-=(const T& val)
    {
        assert(isValid());

        if (!isInfinite())
        {
            min -= val;
            max -= val;
        }
        return *this;
    }

    //******************************************************************************
    template <typename T>
    Interval<T>& Interval<T>::operator-=(const Interval<T>& ivl)
    {
        assert(isValid() && ivl.isValid());

        if (!isInfinite())
        {
            if (ivl.isInfinite())
            {
                makeInfinite();
            }
            else
            {
                min -= ivl.max;
                max -= ivl.min;
            }
        }
        return *this;
    }

    //******************************************************************************
    template <typename T> Interval<T>& Interval<T>::operator*=(const T& val)
    {
        assert(isValid());

        if (!isInfinite())
        {
            const T av = min * val;
            const T bv = max * val;

            if (av < bv)
            {
                min = av;
                max = bv;
            }
            else
            {
                min = bv;
                max = av;
            }
        }
        return *this;
    }

    //******************************************************************************
    template <typename T>
    Interval<T>& Interval<T>::operator*=(const Interval<T>& ivl)
    {
        assert(isValid() && ivl.isValid());

        if (!isInfinite())
        {
            if (ivl.isInfinite())
            {
                makeInfinite();
            }
            else
            {
                const T ac = min * ivl.min;
                const T ad = min * ivl.max;
                const T bc = max * ivl.min;
                const T bd = max * ivl.max;

                min = std::min(std::min(ac, ad), std::min(bc, bd));
                max = std::max(std::max(ac, ad), std::max(bc, bd));
            }
        }
        return *this;
    }

    //******************************************************************************
    template <typename T> Interval<T>& Interval<T>::operator/=(const T& val)
    {
        assert(isValid());

        if (val == (T)0)
        {
            makeInfinite();
        }
        else if (!isInfinite())
        {
            const T av = min / val;
            const T bv = max / val;
            if (av < bv)
            {
                min = av;
                max = bv;
            }
            else
            {
                min = bv;
                max = av;
            }
        }
        return *this;
    }

    //******************************************************************************
    template <typename T>
    Interval<T>& Interval<T>::operator/=(const Interval<T>& ivl)
    {
        assert(isValid() && ivl.isValid());

        if (ivl.intersects((T)0))
        {
            makeInfinite();
        }
        else if (!isInfinite())
        {
            // Don't need to check if ivl is infinite
            // because if it was, it would intersect (T)0 above.
            const T ac = min / ivl.min;
            const T ad = min / ivl.max;
            const T bc = max / ivl.min;
            const T bd = max / ivl.max;
            min = std::min(std::min(ac, ad), std::min(bc, bd));
            max = std::max(std::max(ac, ad), std::max(bc, bd));
        }
        return *this;
    }

    //******************************************************************************
    //******************************************************************************
    // FUNCTIONS WHICH OPERATE ON INTERVALS
    //******************************************************************************
    //******************************************************************************
    template <typename T> inline T size(const Interval<T>& r)
    {
        return r.size();
    }

    //******************************************************************************
    template <typename T> inline T center(const Interval<T>& r)
    {
        return r.center();
    }

    //******************************************************************************
    template <typename T> inline bool isInfinite(const Interval<T>& r)
    {
        return r.isInfinite();
    }

    //******************************************************************************
    template <typename T> inline void makeInfinite(Interval<T>& r)
    {
        r.makeInfinite();
    }

    //******************************************************************************
    template <typename T> inline bool isValid(const Interval<T>& r)
    {
        return r.isValid();
    }

    //******************************************************************************
    template <typename T> inline void makeValid(Interval<T>& r)
    {
        r.makeValid();
    }

    //******************************************************************************
    template <typename T>
    inline bool intersects(const Interval<T>& r, const T& v)
    {
        return r.intersects(v);
    }

    //******************************************************************************
    //******************************************************************************
    // ARITHMETIC OPERATORS
    //******************************************************************************
    //******************************************************************************
    // ADDITION
    //******************************************************************************
    template <typename T>
    Interval<T> operator+(const T& a, const Interval<T>& b)
    {
        assert(b.isValid());

        if (b.isInfinite())
        {
            return b;
        }
        else
        {
            return Interval<T>(a + b.min, a + b.max);
        }
    }

    //******************************************************************************
    template <typename T>
    Interval<T> operator+(const Interval<T>& a, const T& b)
    {
        assert(a.isValid());

        if (a.isInfinite())
        {
            return a;
        }
        else
        {
            return Interval<T>(a.min + b, a.max + b);
        }
    }

    //******************************************************************************
    template <typename T>
    Interval<T> operator+(const Interval<T>& a, const Interval<T>& b)
    {
        assert(a.isValid() && b.isValid());

        if (a.isInfinite())
        {
            return a;
        }
        else if (b.isInfinite())
        {
            return b;
        }
        else
        {
            return Interval<T>(a.min + b.min, a.max + b.max);
        }
    }

    //******************************************************************************
    // NEGATION
    //******************************************************************************
    template <typename T> Interval<T> operator-(const Interval<T>& a)
    {
        assert(a.isValid());

        if (a.isInfinite())
        {
            return a;
        }
        else
        {
            return Interval<T>(-a.max, -a.min);
        }
    }

    //******************************************************************************
    // SUBTRACTION
    //******************************************************************************
    template <typename T>
    Interval<T> operator-(const T& a, const Interval<T>& b)
    {
        assert(b.isValid());

        if (b.isInfinite())
        {
            return b;
        }
        else
        {
            return Interval<T>(a - b.max, a - b.min);
        }
    }

    //******************************************************************************
    template <typename T>
    Interval<T> operator-(const Interval<T>& a, const T& b)
    {
        assert(a.isValid());

        if (a.isInfinite())
        {
            return a;
        }
        else
        {
            return Interval<T>(a.min - b, a.max - b);
        }
    }

    //******************************************************************************
    template <typename T>
    Interval<T> operator-(const Interval<T>& a, const Interval<T>& b)
    {
        assert(a.isValid() && b.isValid());

        if (a.isInfinite())
        {
            return a;
        }
        else if (b.isInfinite())
        {
            return b;
        }
        else
        {
            return Interval<T>(a.min - b.max, a.max - b.min);
        }
    }

    //******************************************************************************
    // MULTIPLICATION
    //******************************************************************************
    template <typename T>
    Interval<T> operator*(const T& a, const Interval<T>& b)
    {
        assert(b.isValid());

        if (b.isInfinite())
        {
            return b;
        }
        else
        {
            // The constructor for the interval will swap the values
            // if they're backwards.
            return Interval<T>(a * b.min, a * b.max);
        }
    }

    //******************************************************************************
    template <typename T>
    Interval<T> operator*(const Interval<T>& a, const T& b)
    {
        assert(a.isValid());

        if (a.isInfinite())
        {
            return a;
        }
        else
        {
            // The constructor for the interval will swap the values
            // if they're backwards
            return Interval<T>(a.min * b, a.max * b);
        }
    }

    //******************************************************************************
    template <typename T>
    Interval<T> operator*(const Interval<T>& a, const Interval<T>& b)
    {
        assert(a.isValid() && b.isValid());

        if (a.isInfinite())
        {
            return a;
        }
        else if (b.isInfinite())
        {
            return b;
        }
        else
        {
            const T ac = a.min * b.min;
            const T ad = a.min * b.max;
            const T bc = a.max * b.min;
            const T bd = a.max * b.max;

            return Interval<T>(std::min(std::min(ac, ad), std::min(bc, bd)),
                               std::max(std::max(ac, ad), std::max(bc, bd)));
        }
    }

    //******************************************************************************
    // DIVISION
    //******************************************************************************
    template <typename T>
    Interval<T> operator/(const T& a, const Interval<T>& b)
    {
        assert(b.isValid());

        if (b.intersects((T)0))
        {
            Interval<T> ret;
            ret.makeInfinite();
            return ret;
        }
        else
        {
            // Interval constructor will swap these
            // if they're in the wrong order
            return Interval<T>(a / b.min, a / b.max);
        }
    }

    //******************************************************************************
    template <typename T>
    Interval<T> operator/(const Interval<T>& a, const T& b)
    {
        assert(a.isValid());

        if (b == (T)0)
        {
            Interval<T> ret;
            ret.makeInfinite();
            return ret;
        }
        else if (a.isInfinite())
        {
            return a;
        }
        else
        {
            // The constructor for the interval will swap the values
            // if they're backwards
            return Interval<T>(a.min / b, a.max / b);
        }
    }

    //******************************************************************************
    template <typename T>
    Interval<T> operator/(const Interval<T>& a, const Interval<T>& b)
    {
        assert(a.isValid() && b.isValid());

        if (b.intersects((T)0))
        {
            Interval<T> ret;
            ret.makeInfinite();
            return ret;
        }
        else if (a.isInfinite())
        {
            return a;
        }
        else
        {
            const T ac = a.min / b.min;
            const T ad = a.min / b.max;
            const T bc = a.max / b.min;
            const T bd = a.max / b.max;
            return Interval<T>(std::min(std::min(ac, ad), std::min(bc, bd)),
                               std::max(std::max(ac, ad), std::max(bc, bd)));
        }
    }

    //******************************************************************************
    //******************************************************************************
    // COMPARISON OPERATORS
    //******************************************************************************
    //******************************************************************************
    // EQUALS
    //******************************************************************************
    template <typename T>
    BooleanInterval operator==(const T& a, const Interval<T>& b)
    {
        assert(b.isValid());

        if (a == b.min && a == b.max)
        {
            return IVL_TRUE;
        }
        else if (b.intersects(a))
        {
            return IVL_MAYBE;
        }
        else
        {
            return IVL_FALSE;
        }
    }

    //******************************************************************************
    template <typename T>
    BooleanInterval operator==(const Interval<T>& a, const T& b)
    {
        assert(a.isValid());

        if (a.min == b && a.max == b)
        {
            return IVL_TRUE;
        }
        else if (a.intersects(b))
        {
            return IVL_MAYBE;
        }
        else
        {
            return IVL_FALSE;
        }
    }

    //******************************************************************************
    template <typename T>
    BooleanInterval operator==(const Interval<T>& a, const Interval<T>& b)
    {
        assert(a.isValid() && b.isValid());

        if ((a.min == a.max) && (a.min == b.min) && (a.min == b.max))
        {
            // Both intervals are degenerate and equal to the
            // same value
            return IVL_TRUE;
        }
        else if (a.max >= b.min && a.min <= b.max)
        {
            return IVL_MAYBE;
        }
        else
        {
            return IVL_FALSE;
        }
    }

    //******************************************************************************
    inline bool same(BooleanInterval a, BooleanInterval b)
    {
        return ((int)a) == ((int)b);
    }

    //******************************************************************************
    BooleanInterval operator==(BooleanInterval a, BooleanInterval b);

    //******************************************************************************
    // NOT EQUALS
    //******************************************************************************
    template <typename T>
    BooleanInterval operator!=(const T& a, const Interval<T>& b)
    {
        assert(b.isValid());

        if (a == b.min && a == b.max)
        {
            return IVL_FALSE;
        }
        else if (b.intersects(a))
        {
            return IVL_MAYBE;
        }
        else
        {
            return IVL_FALSE;
        }
    }

    //******************************************************************************
    template <typename T>
    BooleanInterval operator!=(const Interval<T>& a, const T& b)
    {
        assert(a.isValid());

        if (a.min == b && a.max == b)
        {
            return IVL_FALSE;
        }
        else if (b.intersects(a))
        {
            return IVL_MAYBE;
        }
        else
        {
            return IVL_FALSE;
        }
    }

    //******************************************************************************
    template <typename T>
    BooleanInterval operator!=(const Interval<T>& a, const Interval<T>& b)
    {
        assert(a.isValid() && b.isValid());

        if ((a.min == a.max) && (a.min == b.min) && (a.min == b.max))
        {
            // Both intervals are degenerate and equal to the
            // same value
            return IVL_FALSE;
        }
        else if (a.max >= b.min && a.min <= b.max)
        {
            return IVL_MAYBE;
        }
        else
        {
            return IVL_TRUE;
        }
    }

    //******************************************************************************
    BooleanInterval operator!=(BooleanInterval a, BooleanInterval b);

    //******************************************************************************
    // LESS THAN
    //******************************************************************************
    template <typename T>
    BooleanInterval operator<(const T& a, const Interval<T>& b)
    {
        assert(b.isValid());

        if (a < b.min)
        {
            return IVL_TRUE;
        }
        else if (a < b.max)
        {
            return IVL_MAYBE;
        }
        else
        {
            return IVL_FALSE;
        }
    }

    //******************************************************************************
    template <typename T>
    BooleanInterval operator<(const Interval<T>& a, const T& b)
    {
        assert(a.isValid());

        if (((T)a.max) < b)
        {
            return IVL_TRUE;
        }
        else if (((T)a.min) < b)
        {
            return IVL_MAYBE;
        }
        else
        {
            return IVL_FALSE;
        }
    }

    //******************************************************************************
    template <typename T>
    BooleanInterval operator<(const Interval<T>& a, const Interval<T>& b)
    {
        assert(a.isValid() && b.isValid());

        if (((T)a.max) < ((T)b.min))
        {
            return IVL_TRUE;
        }
        else if (a.min >= b.max)
        {
            return IVL_FALSE;
        }
        else
        {
            return IVL_MAYBE;
        }
    }

    //******************************************************************************
    // LESS THAN OR EQUALS
    //******************************************************************************
    template <typename T>
    BooleanInterval operator<=(const T& a, const Interval<T>& b)
    {
        assert(b.isValid());

        if (a <= b.min)
        {
            return IVL_TRUE;
        }
        else if (a <= b.max)
        {
            return IVL_MAYBE;
        }
        else
        {
            return IVL_FALSE;
        }
    }

    //******************************************************************************
    template <typename T>
    BooleanInterval operator<=(const Interval<T>& a, const T& b)
    {
        assert(a.isValid());

        if (a.max <= b)
        {
            return IVL_TRUE;
        }
        else if (a.min <= b)
        {
            return IVL_MAYBE;
        }
        else
        {
            return IVL_FALSE;
        }
    }

    //******************************************************************************
    template <typename T>
    BooleanInterval operator<=(const Interval<T>& a, const Interval<T>& b)
    {
        assert(a.isValid() && b.isValid());

        if (a.max <= b.min)
        {
            return IVL_TRUE;
        }
        else if (a.min > b.max)
        {
            return IVL_FALSE;
        }
        else
        {
            return IVL_MAYBE;
        }
    }

    //******************************************************************************
    // GREATER THAN
    //******************************************************************************
    template <typename T>
    BooleanInterval operator>(const T& a, const Interval<T>& b)
    {
        assert(b.isValid());

        if (a > b.max)
        {
            return IVL_TRUE;
        }
        else if (a > b.min)
        {
            return IVL_MAYBE;
        }
        else
        {
            return IVL_FALSE;
        }
    }

    //******************************************************************************
    template <typename T>
    BooleanInterval operator>(const Interval<T>& a, const T& b)
    {
        assert(a.isValid());

        if (a.min > b)
        {
            return IVL_TRUE;
        }
        else if (a.max > b)
        {
            return IVL_MAYBE;
        }
        else
        {
            return IVL_FALSE;
        }
    }

    //******************************************************************************
    template <typename T>
    BooleanInterval operator>(const Interval<T>& a, const Interval<T>& b)
    {
        assert(a.isValid() && b.isValid());

        if (a.min > b.max)
        {
            return IVL_TRUE;
        }
        else if (a.max <= b.min)
        {
            return IVL_FALSE;
        }
        else
        {
            return IVL_MAYBE;
        }
    }

    //******************************************************************************
    // GREATER THAN OR EQUALS
    //******************************************************************************
    template <typename T>
    BooleanInterval operator>=(const T& a, const Interval<T>& b)
    {
        assert(b.isValid());

        if (a >= b.max)
        {
            return IVL_TRUE;
        }
        else if (a >= b.min)
        {
            return IVL_MAYBE;
        }
        else
        {
            return IVL_FALSE;
        }
    }

    //******************************************************************************
    template <typename T>
    BooleanInterval operator>=(const Interval<T>& a, const T& b)
    {
        assert(a.isValid());

        if (a.min >= b)
        {
            return IVL_TRUE;
        }
        else if (a.max >= b)
        {
            return IVL_MAYBE;
        }
        else
        {
            return IVL_FALSE;
        }
    }

    //******************************************************************************
    template <typename T>
    BooleanInterval operator>=(const Interval<T>& a, const Interval<T>& b)
    {
        assert(a.isValid() && b.isValid());

        if (a.min >= b.max)
        {
            return IVL_TRUE;
        }
        else if (((T)a.max) < ((T)b.min))
        {
            return IVL_FALSE;
        }
        else
        {
            return IVL_MAYBE;
        }
    }

    //******************************************************************************
    //******************************************************************************
    // LOGICAL OPERATORS
    //******************************************************************************
    //******************************************************************************
    BooleanInterval operator&&(BooleanInterval a, BooleanInterval b);

    //******************************************************************************
    BooleanInterval operator||(BooleanInterval a, BooleanInterval b);

} // End namespace TwkMath

#endif
