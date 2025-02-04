//******************************************************************************
// Copyright (c) 2001 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkMathVec2_h_
#define _TwkMathVec2_h_

#include <TwkMath/Math.h>
#include <sys/types.h>
#include <assert.h>
#include <half.h>

namespace TwkMath
{

    //******************************************************************************
    template <typename T> class Vec2
    {
    public:
        //**************************************************************************
        // TYPEDEFS
        //**************************************************************************
        typedef T value_type;
        typedef size_t size_type;
        typedef T* iterator;
        typedef const T* const_iterator;

        //**************************************************************************
        // DATA MEMBERS
        //**************************************************************************
        T x;
        T y;

        //**************************************************************************
        // CONSTRUCTORS
        //**************************************************************************
        // Default Constructor.
        // Intentionally does nothing to avoid unnecessary instructions
        Vec2() {}

        // Single valued constructor. Initializes elements to value.
        // Explicit because we don't want scalars to be automatically
        // turned into vectors, unless the programmer specifically
        // requests it.
        explicit Vec2(const T& val)
            : x(val)
            , y(val)
        {
        }

        // Value constructor. Initializes elements directly
        Vec2(const T& vx, const T& vy)
            : x(vx)
            , y(vy)
        {
        }

        // Array constructor. Initializes elements to array
        // Array must be of the proper size. Explicit
        // to avoid pointers to scalars being turned into vectors
        // automatically.
        explicit Vec2(const T a[])
            : x(a[0])
            , y(a[1])
        {
        }

        // Copy constructor from a Vec2 of a different type
        // Non explicit, so automatic promotion may occur.
        // Still in-class, though, due to VisualC++'s inability
        // to handle out-of-class template-template member functions.
        template <class T2>
        Vec2(const Vec2<T2>& copy)
            : x(T(copy.x))
            , y(T(copy.y))
        {
        }

        // Copy constructor. Copies elements one by one. In class because
        // it is a specialization of the above template-template
        // member function
        explicit Vec2(const Vec2<T>& copy)
            : x(copy.x)
            , y(copy.y)
        {
        }

        //**************************************************************************
        // ASSIGNMENT OPERATORS
        //**************************************************************************
        // Single valued assignment. All elements set to value
        Vec2<T>& operator=(const T& value);

        // Copy from a vec2 of any type. Must be in-class due to VisualC++
        // problems.
        template <class T2> Vec2<T>& operator=(const Vec2<T2>& copy)
        {
            x = copy.x;
            y = copy.y;
            return *this;
        }

        // Copy assignment. Elements copied one by one. In-class because it is
        // a specialization of the above in-class function.
        Vec2<T>& operator=(const Vec2<T>& copy)
        {
            x = copy.x;
            y = copy.y;
            return *this;
        }

        //**************************************************************************
        // DIMENSION FUNCTION
        //**************************************************************************
        static size_type dimension() { return 2; }

        //**************************************************************************
        // DATA ACCESS OPERATORS
        //**************************************************************************
        const T& operator[](size_type index) const;
        T& operator[](size_type index);

        void set(const T& vx, const T& vy)
        {
            x = vx;
            y = vy;
        }

        //**************************************************************************
        // ITERATORS
        //**************************************************************************
        const_iterator begin() const;
        iterator begin();
        const_iterator end() const;

        //**************************************************************************
        // GEOMETRIC FUNCTIONS
        //**************************************************************************
        // Return the magnitude of the vector
        T magnitude() const;

        // Return the magnitude squared of the vector.
        // Same as dot( *this, *this ), but more easily optimized
        T magnitudeSquared() const;

        // Normalize this vector
        void normalize();

        // Return a copy of this vector, normalized
        Vec2<T> normalized() const;

        //**************************************************************************
        // ARITHMETIC OPERATORS
        //**************************************************************************
        Vec2<T>& operator+=(const T& rhs);
        Vec2<T>& operator+=(const Vec2<T>& rhs);

        Vec2<T>& operator-=(const T& rhs);
        Vec2<T>& operator-=(const Vec2<T>& rhs);

        // Multiplication and Divide are COMPONENT-WISE!!!
        Vec2<T>& operator*=(const T& rhs);
        Vec2<T>& operator*=(const Vec2<T>& rhs);

        Vec2<T>& operator/=(const T& rhs);
        Vec2<T>& operator/=(const Vec2<T>& rhs);
    };

    //******************************************************************************
    // TYPEDEFS
    //******************************************************************************
    typedef Vec2<unsigned char> Vec2uc;
    typedef Vec2<char> Vec2c;
    typedef Vec2<unsigned short> Vec2us;
    typedef Vec2<short> Vec2s;
    typedef Vec2<unsigned int> Vec2ui;
    typedef Vec2<int> Vec2i;
    typedef Vec2<unsigned long> Vec2ul;
    typedef Vec2<long> Vec2l;
    typedef Vec2<float> Vec2f;
    typedef Vec2<half> Vec2h;
    typedef Vec2<double> Vec2d;

    //******************************************************************************
    // TEMPLATE AND INLINE FUNCTIONS
    //******************************************************************************
    template <typename T> inline Vec2<T>& Vec2<T>::operator=(const T& value)
    {
        x = value;
        y = value;
        return *this;
    }

    //******************************************************************************
    template <typename T>
    inline const T& Vec2<T>::operator[](typename Vec2<T>::size_type index) const
    {
        assert(index >= 0 && index < 2);
        return ((const T*)this)[index];
    }

    //******************************************************************************
    template <typename T>
    inline T& Vec2<T>::operator[](typename Vec2<T>::size_type index)
    {
        assert(index >= 0 && index < 2);
        return ((T*)this)[index];
    }

    //******************************************************************************
    template <typename T>
    inline typename Vec2<T>::const_iterator Vec2<T>::begin() const
    {
        return static_cast<const_iterator>(&x);
    }

    //******************************************************************************
    template <typename T> inline typename Vec2<T>::iterator Vec2<T>::begin()
    {
        return static_cast<iterator>(&x);
    }

    //******************************************************************************
    template <typename T>
    inline typename Vec2<T>::const_iterator Vec2<T>::end() const
    {
        return static_cast<const_iterator>(&x + 2);
    }

    //******************************************************************************
    template <typename T> inline T Vec2<T>::magnitude() const
    {
        return Math<T>::sqrt((x * x) + (y * y));
    }

    //******************************************************************************
    template <typename T> inline T Vec2<T>::magnitudeSquared() const
    {
        return (x * x) + (y * y);
    }

    //******************************************************************************
    template <typename T> inline void Vec2<T>::normalize()
    {
        const T mag = magnitude();
        if (mag > (T)0)
        {
            x /= mag;
            y /= mag;
        }
        else
        {
            x = y = (T)0;
        }
    }

    //******************************************************************************
    template <typename T> inline Vec2<T> Vec2<T>::normalized() const
    {
        const T mag = magnitude();
        if (mag > (T)0)
        {
            return Vec2<T>(x / mag, y / mag);
        }
        else
        {
            return Vec2<T>((T)0);
        }
    }

    //******************************************************************************
    template <typename T> inline Vec2<T>& Vec2<T>::operator+=(const T& rhs)
    {
        x += rhs;
        y += rhs;
        return *this;
    }

    //******************************************************************************
    template <typename T>
    inline Vec2<T>& Vec2<T>::operator+=(const Vec2<T>& rhs)
    {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    //******************************************************************************
    template <typename T> inline Vec2<T>& Vec2<T>::operator-=(const T& rhs)
    {
        x -= rhs;
        y -= rhs;
        return *this;
    }

    //******************************************************************************
    template <typename T>
    inline Vec2<T>& Vec2<T>::operator-=(const Vec2<T>& rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    //******************************************************************************
    template <typename T> inline Vec2<T>& Vec2<T>::operator*=(const T& rhs)
    {
        x *= rhs;
        y *= rhs;
        return *this;
    }

    //******************************************************************************
    template <typename T>
    inline Vec2<T>& Vec2<T>::operator*=(const Vec2<T>& rhs)
    {
        x *= rhs.x;
        y *= rhs.y;
        return *this;
    }

    //******************************************************************************
    template <typename T> inline Vec2<T>& Vec2<T>::operator/=(const T& rhs)
    {
        assert(rhs != (T)0);
        x /= rhs;
        y /= rhs;
        return *this;
    }

    //******************************************************************************
    template <typename T>
    inline Vec2<T>& Vec2<T>::operator/=(const Vec2<T>& rhs)
    {
        assert(rhs.x != (T)0);
        x /= rhs.x;
        assert(rhs.y != (T)0);
        y /= rhs.y;
        return *this;
    };

    //******************************************************************************
    //******************************************************************************
    // FUNCTIONS WHICH OPERATE ON VEC2
    //******************************************************************************
    //******************************************************************************
    template <typename T> inline size_t dimension(Vec2<T>) { return 2; }

    //******************************************************************************
    template <typename T> inline size_t majorComponent(const Vec2<T>& v)
    {
        return v.x > v.y ? 0 : 1;
    }

    //******************************************************************************
    template <typename T> inline size_t minorComponent(const Vec2<T>& v)
    {
        return v.x < v.y ? 0 : 1;
    }

    //******************************************************************************
    template <typename T> inline T magnitude(const Vec2<T>& v)
    {
        return v.magnitude();
    }

    //******************************************************************************
    template <typename T> inline T magnitudeSquared(const Vec2<T>& v)
    {
        return v.magnitudeSquared();
    }

    //******************************************************************************
    template <typename T> inline T dot(const Vec2<T>& a, const Vec2<T>& b)
    {
        return (a.x * b.x) + (a.y * b.y);
    }

    //******************************************************************************
    // Perpdot is the dot of B with the counterclockwise (left) perpendicular
    // of A.
    template <typename T> inline T perpDot(const Vec2<T>& a, const Vec2<T>& b)
    {
        return (-a.y * b.x) + (a.x * b.y);
    }

    //******************************************************************************
    template <typename T> inline Vec2<T> normalize(const Vec2<T>& v)
    {
        return v.normalized();
    }

    //******************************************************************************
    // Two dimensional cross product returns a scalar.
    template <typename T> inline T cross(const Vec2<T>& a, const Vec2<T>& b)
    {
        return (a.x * b.y) - (a.y * b.x);
    }

    template <typename T> inline Vec2<T> crossUp(const Vec2<T>& a)
    {
        return Vec2<T>(a.y, -a.x);
    }

    template <typename T> inline Vec2<T> crossDown(const Vec2<T>& a)
    {
        return Vec2<T>(-a.y, a.x);
    }

    //******************************************************************************
    // Absolute value of the angle between two vectors. Never more than PI,
    // never less than zero.
    template <typename T>
    inline T angleBetween(const Vec2<T>& a, const Vec2<T>& b)
    {
        const Vec2<T> an = normalize(a);
        const Vec2<T> bn = normalize(b);
        T d = dot(an, bn);
        //
        //  Even after normalizing above, the dot() can produce a value > or <
        //  1, which will give a NAN from acos(), so trim the dot to the
        //  accepted input range for acos.
        //
        if (d < T(-1))
            d = T(-1);
        if (d > T(1))
            d = T(1);
        return Math<T>::acos(d);
    }

    //******************************************************************************
    // Turn angle - this is the angle required to rotate from A to B - it's
    // between -PI to PI
    template <typename T>
    inline T turnAngle(const Vec2<T>& p0, const Vec2<T>& p1, const Vec2<T>& p2)
    {
        Vec2<T> a = p1 - p0;
        Vec2<T> b = p2 - p1;
        return Math<T>::atan2(perpDot(a, b), dot(a, b));
    }

    //******************************************************************************
    //******************************************************************************
    // GLOBAL ARITHMETIC OPERATORS
    //******************************************************************************
    //******************************************************************************
    // ADDITION
    template <typename T> inline Vec2<T> operator+(const Vec2<T>& a, const T& b)
    {
        return Vec2<T>(a.x + b, a.y + b);
    }

    //******************************************************************************
    template <typename T> inline Vec2<T> operator+(const T& a, const Vec2<T>& b)
    {
        return Vec2<T>(a + b.x, a + b.y);
    }

    //******************************************************************************
    template <typename T>
    inline Vec2<T> operator+(const Vec2<T>& a, const Vec2<T>& b)
    {
        return Vec2<T>(a.x + b.x, a.y + b.y);
    }

    //******************************************************************************
    // NEGATION
    template <typename T> inline Vec2<T> operator-(const Vec2<T>& a)
    {
        return Vec2<T>(-a.x, -a.y);
    }

    //******************************************************************************
    // SUBTRACTION
    template <typename T> inline Vec2<T> operator-(const Vec2<T>& a, const T& b)
    {
        return Vec2<T>(a.x - b, a.y - b);
    }

    //******************************************************************************
    template <typename T> inline Vec2<T> operator-(const T& a, const Vec2<T>& b)
    {
        return Vec2<T>(a - b.x, a - b.y);
    }

    //******************************************************************************
    template <typename T>
    inline Vec2<T> operator-(const Vec2<T>& a, const Vec2<T>& b)
    {
        return Vec2<T>(a.x - b.x, a.y - b.y);
    }

    //******************************************************************************
    // MULTIPLICATION
    template <typename T> inline Vec2<T> operator*(const Vec2<T>& a, const T& b)
    {
        return Vec2<T>(a.x * b, a.y * b);
    }

    //******************************************************************************
    template <typename T> inline Vec2<T> operator*(const T& a, const Vec2<T>& b)
    {
        return Vec2<T>(a * b.x, a * b.y);
    }

    //******************************************************************************
    template <typename T>
    inline Vec2<T> operator*(const Vec2<T>& a, const Vec2<T>& b)
    {
        return Vec2<T>(a.x * b.x, a.y * b.y);
    }

    //******************************************************************************
    // DIVISION
    template <typename T> inline Vec2<T> operator/(const Vec2<T>& a, const T& b)
    {
        assert(b != (T)0);
        return Vec2<T>(a.x / b, a.y / b);
    }

    //******************************************************************************
    template <typename T> inline Vec2<T> operator/(const T& a, const Vec2<T>& b)
    {
        assert(b.x != (T)0);
        assert(b.y != (T)0);
        return Vec2<T>(a / b.x, a / b.y);
    }

    //******************************************************************************
    template <typename T>
    inline Vec2<T> operator/(const Vec2<T>& a, const Vec2<T>& b)
    {
        assert(b.x != (T)0);
        assert(b.y != (T)0);
        return Vec2<T>(a.x / b.x, a.y / b.y);
    }

    //******************************************************************************
    //******************************************************************************
    // COMPARISON OPERATORS
    //******************************************************************************
    //******************************************************************************
    template <typename T, typename T2>
    inline bool operator==(const Vec2<T>& a, const Vec2<T2>& b)
    {
        return (a.x == b.x) && (a.y == b.y);
    }

    //******************************************************************************
    template <typename T, typename T2>
    inline bool operator!=(const Vec2<T>& a, const Vec2<T2>& b)
    {
        return (a.x != b.x) || (a.y != b.y);
    }

#ifdef COMPILER_GCC2_96
    inline bool operator==(const Vec2<int>& a, const Vec2<int>& b)
    {
        return (a.x == b.x) && (a.y == b.y);
    }

    //******************************************************************************
    inline bool operator!=(const Vec2<int>& a, const Vec2<int>& b)
    {
        return (a.x != b.x) || (a.y != b.y);
    }

#endif

} // End namespace TwkMath

#endif
