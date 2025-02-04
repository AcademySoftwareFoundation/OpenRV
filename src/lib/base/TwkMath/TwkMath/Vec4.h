//******************************************************************************
// Copyright (c) 2001 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkMathVec4_h_
#define _TwkMathVec4_h_

#include <TwkMath/Math.h>
#include <sys/types.h>
#include <assert.h>
#include <half.h>

namespace TwkMath
{

    //******************************************************************************
    template <typename T> class Vec4
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
        // By using an anonymous union we can alias the member data names
        T x;
        T y;
        T z;
        T w;

        //**************************************************************************
        // CONSTRUCTORS
        //**************************************************************************
        // Default Constructor.
        // Intentionally does nothing to avoid unnecessary instructions
        Vec4() {}

        // Single valued constructor. Initializes elements to value.
        // Explicit because we don't want scalars to be automatically
        // turned into vectors, unless the programmer specifically
        // requests it.
        explicit Vec4(const T& v)
            : x(v)
            , y(v)
            , z(v)
            , w(v)
        {
        }

        // Value constructor. Initializes elements directly
        Vec4(const T& vx, const T& vy, const T& vz, const T& vw)
            : x(vx)
            , y(vy)
            , z(vz)
            , w(vw)
        {
        }

        // Array constructor. Initializes elements to array
        // Array must be of the proper size. Explicit
        // to avoid pointers to scalars being turned into vectors
        // automatically.
        explicit Vec4(const T a[])
            : x(a[0])
            , y(a[1])
            , z(a[2])
            , w(a[3])
        {
        }

        // Copy constructor from a Vec4 of a different type
        // Non explicit, so automatic promotion may occur.
        // Still in-class, though, due to VisualC++'s inability
        // to handle out-of-class template-template member functions.
        template <class T2>
        Vec4(const Vec4<T2>& copy)
            : x(T(copy.x))
            , y(T(copy.y))
            , z(T(copy.z))
            , w(T(copy.w))
        {
        }

        // Copy constructor. Copies elements one by one. In class because
        // it is a specialization of the above template-template
        // member function
        explicit Vec4(const Vec4<T>& copy)
            : x(copy.x)
            , y(copy.y)
            , z(copy.z)
            , w(copy.w)
        {
        }

        //**************************************************************************
        // ASSIGNMENT OPERATORS
        //**************************************************************************
        // Single valued assignment. All elements set to value
        Vec4<T>& operator=(const T& value);

        // Copy from a vec2 of any type. Must be in-class due to VisualC++
        // problems.
        template <class T2> Vec4<T>& operator=(const Vec4<T2>& copy)
        {
            x = copy.x;
            y = copy.y;
            z = copy.z;
            w = copy.w;
            return *this;
        }

        // Copy assignment. Elements copied one by one. In-class because it is
        // a specialization of the above in-class function.
        Vec4<T>& operator=(const Vec4<T>& copy)
        {
            x = copy.x;
            y = copy.y;
            z = copy.z;
            w = copy.w;
            return *this;
        }

        //**************************************************************************
        // DIMENSION FUNCTION
        //**************************************************************************
        static size_type dimension() { return 4; }

        //**************************************************************************
        // DATA ACCESS OPERATORS
        //**************************************************************************
        const T& operator[](size_type index) const;
        T& operator[](size_type index);

        void set(const T& vx, const T& vy, const T& vz, const T& vw)
        {
            x = vx;
            y = vy;
            z = vz;
            w = vw;
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
        Vec4<T> normalized() const;

        //**************************************************************************
        // ARITHMETIC OPERATORS
        //**************************************************************************
        Vec4<T>& operator+=(const T& rhs);
        Vec4<T>& operator+=(const Vec4<T>& rhs);

        Vec4<T>& operator-=(const T& rhs);
        Vec4<T>& operator-=(const Vec4<T>& rhs);

        // Multiplication and Divide are COMPONENT-WISE!!!
        Vec4<T>& operator*=(const T& rhs);
        Vec4<T>& operator*=(const Vec4<T>& rhs);

        Vec4<T>& operator/=(const T& rhs);
        Vec4<T>& operator/=(const Vec4<T>& rhs);
    };

    //******************************************************************************
    // TYPEDEFS
    //******************************************************************************
    typedef Vec4<unsigned char> Vec4uc;
    typedef Vec4<char> Vec4c;
    typedef Vec4<unsigned short> Vec4us;
    typedef Vec4<short> Vec4s;
    typedef Vec4<unsigned int> Vec4ui;
    typedef Vec4<int> Vec4i;
    typedef Vec4<unsigned long> Vec4ul;
    typedef Vec4<long> Vec4l;
    typedef Vec4<float> Vec4f;
    typedef Vec4<half> Vec4h;
    typedef Vec4<double> Vec4d;

    //******************************************************************************
    // TEMPLATE AND INLINE FUNCTIONS
    //******************************************************************************
    template <typename T> inline Vec4<T>& Vec4<T>::operator=(const T& value)
    {
        x = value;
        y = value;
        z = value;
        w = value;
        return *this;
    }

    //******************************************************************************
    template <typename T>
    inline const T& Vec4<T>::operator[](typename Vec4<T>::size_type index) const
    {
        assert(index >= 0 && index < 4);
        return ((const T*)this)[index];
    }

    //******************************************************************************
    template <typename T>
    inline T& Vec4<T>::operator[](typename Vec4<T>::size_type index)
    {
        assert(index >= 0 && index < 4);
        return ((T*)this)[index];
    }

    //******************************************************************************
    template <typename T>
    inline typename Vec4<T>::const_iterator Vec4<T>::begin() const
    {
        return static_cast<const_iterator>(&x);
    }

    //******************************************************************************
    template <typename T> inline typename Vec4<T>::iterator Vec4<T>::begin()
    {
        return static_cast<iterator>(&x);
    }

    //******************************************************************************
    template <typename T>
    inline typename Vec4<T>::const_iterator Vec4<T>::end() const
    {
        return static_cast<const_iterator>(&x + 4);
    }

    //******************************************************************************
    template <typename T> inline T Vec4<T>::magnitude() const
    {
        return Math<T>::sqrt((x * x) + (y * y) + (z * z) + (w * w));
    }

    //******************************************************************************
    template <typename T> inline T Vec4<T>::magnitudeSquared() const
    {
        return (x * x) + (y * y) + (z * z) + (w * w);
    }

    //******************************************************************************
    template <typename T> inline void Vec4<T>::normalize()
    {
        const T mag = magnitude();
        if (mag > (T)0)
        {
            x /= mag;
            y /= mag;
            z /= mag;
            w /= mag;
        }
        else
        {
            x = y = z = w = (T)0;
        }
    }

    //******************************************************************************
    template <typename T> inline Vec4<T> Vec4<T>::normalized() const
    {
        const T mag = magnitude();
        if (mag > (T)0)
        {
            return Vec4<T>(x / mag, y / mag, z / mag, w / mag);
        }
        else
        {
            return Vec4<T>((T)0);
        }
    }

    //******************************************************************************
    template <typename T> inline Vec4<T>& Vec4<T>::operator+=(const T& rhs)
    {
        x += rhs;
        y += rhs;
        z += rhs;
        w += rhs;
        return *this;
    }

    //******************************************************************************
    template <typename T>
    inline Vec4<T>& Vec4<T>::operator+=(const Vec4<T>& rhs)
    {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        w += rhs.w;
        return *this;
    }

    //******************************************************************************
    template <typename T> inline Vec4<T>& Vec4<T>::operator-=(const T& rhs)
    {
        x -= rhs;
        y -= rhs;
        z -= rhs;
        w -= rhs;
        return *this;
    }

    //******************************************************************************
    template <typename T>
    inline Vec4<T>& Vec4<T>::operator-=(const Vec4<T>& rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        w -= rhs.w;
        return *this;
    }

    //******************************************************************************
    template <typename T> inline Vec4<T>& Vec4<T>::operator*=(const T& rhs)
    {
        x *= rhs;
        y *= rhs;
        z *= rhs;
        w *= rhs;
        return *this;
    }

    //******************************************************************************
    template <typename T>
    inline Vec4<T>& Vec4<T>::operator*=(const Vec4<T>& rhs)
    {
        x *= rhs.x;
        y *= rhs.y;
        z *= rhs.z;
        w *= rhs.w;
        return *this;
    }

    //******************************************************************************
    template <typename T> inline Vec4<T>& Vec4<T>::operator/=(const T& rhs)
    {
        assert(rhs != (T)0);
        x /= rhs;
        y /= rhs;
        z /= rhs;
        w /= rhs;
        return *this;
    }

    //******************************************************************************
    template <typename T>
    inline Vec4<T>& Vec4<T>::operator/=(const Vec4<T>& rhs)
    {
        assert(rhs.x != (T)0);
        x /= rhs.x;
        assert(rhs.y != (T)0);
        y /= rhs.y;
        assert(rhs.z != (T)0);
        z /= rhs.z;
        assert(rhs.w != (T)0);
        w /= rhs.w;
        return *this;
    };

    //******************************************************************************
    //******************************************************************************
    // FUNCTIONS WHICH OPERATE ON VEC2
    //******************************************************************************
    //******************************************************************************
    template <typename T> inline size_t dimension(Vec4<T>) { return 4; }

    //******************************************************************************
    template <typename T> inline T magnitude(const Vec4<T>& v)
    {
        return v.magnitude();
    }

    //******************************************************************************
    template <typename T> inline T magnitudeSquared(const Vec4<T>& v)
    {
        return v.magnitudeSquared();
    }

    //******************************************************************************
    template <typename T> inline T dot(const Vec4<T>& a, const Vec4<T>& b)
    {
        return (a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.w * b.w);
    }

    //******************************************************************************
    template <typename T> inline Vec4<T> normalize(const Vec4<T>& v)
    {
        return v.normalized();
    }

    //******************************************************************************
    // Absolute value of the angle between two vectors. Never more than PI,
    // never less than zero.
    template <typename T>
    inline T angleBetween(const Vec4<T>& a, const Vec4<T>& b)
    {
        const Vec4<T> an = normalize(a);
        const Vec4<T> bn = normalize(b);
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
    //******************************************************************************
    // GLOBAL ARITHMETIC OPERATORS
    //******************************************************************************
    //******************************************************************************
    // ADDITION
    template <typename T> inline Vec4<T> operator+(const Vec4<T>& a, const T& b)
    {
        return Vec4<T>(a.x + b, a.y + b, a.z + b, a.w + b);
    }

    //******************************************************************************
    template <typename T> inline Vec4<T> operator+(const T& a, const Vec4<T>& b)
    {
        return Vec4<T>(a + b.x, a + b.y, a + b.z, a + b.w);
    }

    //******************************************************************************
    template <typename T>
    inline Vec4<T> operator+(const Vec4<T>& a, const Vec4<T>& b)
    {
        return Vec4<T>(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
    }

    //******************************************************************************
    // NEGATION
    template <typename T> inline Vec4<T> operator-(const Vec4<T>& a)
    {
        return Vec4<T>(-a.x, -a.y, -a.z, -a.w);
    }

    //******************************************************************************
    // SUBTRACTION
    template <typename T> inline Vec4<T> operator-(const Vec4<T>& a, const T& b)
    {
        return Vec4<T>(a.x - b, a.y - b, a.z - b, a.w - b);
    }

    //******************************************************************************
    template <typename T> inline Vec4<T> operator-(const T& a, const Vec4<T>& b)
    {
        return Vec4<T>(a - b.x, a - b.y, a - b.z, a - b.w);
    }

    //******************************************************************************
    template <typename T>
    inline Vec4<T> operator-(const Vec4<T>& a, const Vec4<T>& b)
    {
        return Vec4<T>(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
    }

    //******************************************************************************
    // MULTIPLICATION
    template <typename T> inline Vec4<T> operator*(const Vec4<T>& a, const T& b)
    {
        return Vec4<T>(a.x * b, a.y * b, a.z * b, a.w * b);
    }

    //******************************************************************************
    template <typename T> inline Vec4<T> operator*(const T& a, const Vec4<T>& b)
    {
        return Vec4<T>(a * b.x, a * b.y, a * b.z, a * b.w);
    }

    //******************************************************************************
    template <typename T>
    inline Vec4<T> operator*(const Vec4<T>& a, const Vec4<T>& b)
    {
        return Vec4<T>(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
    }

    //******************************************************************************
    // DIVISION
    template <typename T> inline Vec4<T> operator/(const Vec4<T>& a, const T& b)
    {
        assert(b != (T)0);
        return Vec4<T>(a.x / b, a.y / b, a.z / b, a.w / b);
    }

    //******************************************************************************
    template <typename T> inline Vec4<T> operator/(const T& a, const Vec4<T>& b)
    {
        assert(b.x != (T)0);
        assert(b.y != (T)0);
        assert(b.z != (T)0);
        assert(b.w != (T)0);
        return Vec4<T>(a / b.x, a / b.y, a / b.z, a / b.w);
    }

    //******************************************************************************
    template <typename T>
    inline Vec4<T> operator/(const Vec4<T>& a, const Vec4<T>& b)
    {
        assert(b.x != (T)0);
        assert(b.y != (T)0);
        assert(b.z != (T)0);
        assert(b.w != (T)0);
        return Vec4<T>(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w);
    }

    //******************************************************************************
    //******************************************************************************
    // COMPARISON OPERATORS
    //******************************************************************************
    //******************************************************************************
    template <typename T, typename T2>
    inline bool operator==(const Vec4<T>& a, const Vec4<T2>& b)
    {
        return (a.x == b.x) && (a.y == b.y) && (a.z == b.z) && (a.w == b.w);
    }

    //******************************************************************************
    template <typename T, typename T2>
    inline bool operator!=(const Vec4<T>& a, const Vec4<T2>& b)
    {
        return (a.x != b.x) || (a.y != b.y) || (a.z != b.z) || (a.w != b.w);
    }

} // End namespace TwkMath

#endif
