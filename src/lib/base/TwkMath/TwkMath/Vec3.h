
// Copyright (c) 2001 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#ifndef _TwkMathVec3_h_
#define _TwkMathVec3_h_

#include <TwkMath/Math.h>
#include <sys/types.h>
#include <half.h>
#include <assert.h>

namespace TwkMath
{

    template <typename T> class Vec3
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

        //**************************************************************************
        // CONSTRUCTORS
        //**************************************************************************
        // Default Constructor.
        // Intentionally does nothing to avoid unnecessary instructions
        Vec3() {}

        // Single valued constructor. Initializes elements to value.
        // Explicit because we don't want scalars to be automatically
        // turned into vectors, unless the programmer specifically
        // requests it.
        explicit Vec3(const T& val)
            : x(val)
            , y(val)
            , z(val)
        {
        }

        // Value constructor. Initializes elements directly
        Vec3(const T& vx, const T& vy, const T& vz)
            : x(vx)
            , y(vy)
            , z(vz)
        {
        }

        // Array constructor. Initializes elements to array
        // Array must be of the proper size. Explicit
        // to avoid pointers to scalars being turned into vectors
        // automatically.
        explicit Vec3(const T a[])
            : x(a[0])
            , y(a[1])
            , z(a[2])
        {
        }

        // Copy constructor from a Vec3 of a different type
        // Non explicit, so automatic promotion may occur.
        // Still in-class, though, due to VisualC++'s inability
        // to handle out-of-class template-template member functions.
        template <class T2>
        Vec3(const Vec3<T2>& copy)
            : x(T(copy.x))
            , y(T(copy.y))
            , z(T(copy.z))
        {
        }

        // Copy constructor. Copies elements one by one. In class because
        // it is a specialization of the above template-template
        // member function
        explicit Vec3(const Vec3<T>& copy)
            : x(copy.x)
            , y(copy.y)
            , z(copy.z)
        {
        }

        //**************************************************************************
        // ASSIGNMENT OPERATORS
        //**************************************************************************
        // Single valued assignment. All elements set to value
        template <typename S> Vec3<T>& operator=(const S& value)
        {
            x = y = z = value;
            return *this;
        }

        // Copy assignment. Elements copied one by one. In-class because it is
        // a specialization of the above in-class function.
        template <typename S> Vec3<T>& operator=(const Vec3<S>& copy)
        {
            x = copy.x;
            y = copy.y;
            z = copy.z;
            return *this;
        }

        //**************************************************************************
        // DIMENSION FUNCTION
        //**************************************************************************
        static size_type dimension() { return 3; }

        //**************************************************************************
        // DATA ACCESS OPERATORS
        //**************************************************************************
        const T& operator[](size_type index) const;
        T& operator[](size_type index);

        void set(const T& vx, const T& vy, const T& vz)
        {
            x = vx;
            y = vy;
            z = vz;
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
        Vec3<T> normalized() const;

        //**************************************************************************
        // ARITHMETIC OPERATORS
        //**************************************************************************
        template <typename S> Vec3<T>& operator+=(const S& rhs);
        template <typename S> Vec3<T>& operator+=(const Vec3<S>& rhs);

        template <typename S> Vec3<T>& operator-=(const S& rhs);
        template <typename S> Vec3<T>& operator-=(const Vec3<S>& rhs);

        // Multiplication and Divide are COMPONENT-WISE!!!
        template <typename S> Vec3<T>& operator*=(const S& rhs);
        template <typename S> Vec3<T>& operator*=(const Vec3<S>& rhs);

        template <typename S> Vec3<T>& operator/=(const S& rhs);
        template <typename S> Vec3<T>& operator/=(const Vec3<S>& rhs);
    };

    // TYPEDEFS

    typedef Vec3<unsigned char> Vec3uc;
    typedef Vec3<char> Vec3c;
    typedef Vec3<unsigned short> Vec3us;
    typedef Vec3<short> Vec3s;
    typedef Vec3<unsigned int> Vec3ui;
    typedef Vec3<int> Vec3i;
    typedef Vec3<unsigned long> Vec3ul;
    typedef Vec3<long> Vec3l;
    typedef Vec3<float> Vec3f;
    typedef Vec3<half> Vec3h;
    typedef Vec3<double> Vec3d;

    template <typename T>
    inline const T& Vec3<T>::operator[](typename Vec3<T>::size_type index) const
    {
        assert(index >= 0 && index < 3);
        return ((const T*)this)[index];
    }

    template <typename T>
    inline T& Vec3<T>::operator[](typename Vec3<T>::size_type index)
    {
        assert(index >= 0 && index < 3);
        return ((T*)this)[index];
    }

    template <typename T>
    inline typename Vec3<T>::const_iterator Vec3<T>::begin() const
    {
        return static_cast<const_iterator>(&x);
    }

    template <typename T> inline typename Vec3<T>::iterator Vec3<T>::begin()
    {
        return static_cast<iterator>(&x);
    }

    template <typename T>
    inline typename Vec3<T>::const_iterator Vec3<T>::end() const
    {
        return static_cast<const_iterator>(&x + 3);
    }

    template <typename T> inline T Vec3<T>::magnitude() const
    {
        return Math<T>::sqrt((x * x) + (y * y) + (z * z));
    }

    template <typename T> inline T Vec3<T>::magnitudeSquared() const
    {
        return (x * x) + (y * y) + (z * z);
    }

    template <typename T> inline void Vec3<T>::normalize()
    {
        const T mag = magnitude();
        if (mag > (T)0)
        {
            x /= mag;
            y /= mag;
            z /= mag;
        }
        else
        {
            x = y = z = (T)0;
        }
    }

    template <typename T> inline Vec3<T> Vec3<T>::normalized() const
    {
        const T mag = magnitude();
        if (mag > (T)0)
        {
            return Vec3<T>(x / mag, y / mag, z / mag);
        }
        else
        {
            return Vec3<T>((T)0);
        }
    }

    template <typename T>
    template <typename S>
    inline Vec3<T>& Vec3<T>::operator+=(const S& rhs)
    {
        x += rhs;
        y += rhs;
        z += rhs;
        return *this;
    }

    template <typename T>
    template <typename S>
    inline Vec3<T>& Vec3<T>::operator+=(const Vec3<S>& rhs)
    {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }

    template <typename T>
    template <typename S>
    inline Vec3<T>& Vec3<T>::operator-=(const S& rhs)
    {
        x -= rhs;
        y -= rhs;
        z -= rhs;
        return *this;
    }

    template <typename T>
    template <typename S>
    inline Vec3<T>& Vec3<T>::operator-=(const Vec3<S>& rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }

    template <typename T>
    template <typename S>
    inline Vec3<T>& Vec3<T>::operator*=(const S& rhs)
    {
        x *= rhs;
        y *= rhs;
        z *= rhs;
        return *this;
    }

    template <typename T>
    template <typename S>
    inline Vec3<T>& Vec3<T>::operator*=(const Vec3<S>& rhs)
    {
        x *= rhs.x;
        y *= rhs.y;
        z *= rhs.z;
        return *this;
    }

    template <typename T>
    template <typename S>
    inline Vec3<T>& Vec3<T>::operator/=(const S& rhs)
    {
        assert(rhs != (T)0);
        x /= rhs;
        y /= rhs;
        z /= rhs;
        return *this;
    }

    template <typename T>
    template <typename S>
    inline Vec3<T>& Vec3<T>::operator/=(const Vec3<S>& rhs)
    {
        assert(rhs.x != (T)0);
        x /= rhs.x;
        assert(rhs.y != (T)0);
        y /= rhs.y;
        assert(rhs.z != (T)0);
        z /= rhs.z;
        return *this;
    };

    // FUNCTIONS WHICH OPERATE ON VEC3

    template <typename T> inline size_t dimension(Vec3<T>) { return 3; }

    template <typename T> inline size_t majorComponent(const Vec3<T>& q)
    {
        Vec3<T> v =
            Vec3<T>(Math<T>::abs(q.x), Math<T>::abs(q.y), Math<T>::abs(q.z));
        if (v.x > v.y)
        {
            return (v.x > v.z) ? 0 : 2;
        }
        else
        {
            return (v.y > v.z) ? 1 : 2;
        }
    }

    template <typename T> inline size_t minorComponent(const Vec3<T>& q)
    {
        Vec3<T> v =
            Vec3<T>(Math<T>::abs(q.x), Math<T>::abs(q.y), Math<T>::abs(q.z));
        if (v.x < v.y)
        {
            return (v.x < v.z) ? 0 : 2;
        }
        else
        {
            return (v.y < v.z) ? 1 : 2;
        }
    }

    template <typename T> inline T magnitude(const Vec3<T>& v)
    {
        return v.magnitude();
    }

    template <typename T> inline T mag(const Vec3<T>& v)
    {
        return v.magnitude();
    }

    template <typename T> inline T magnitudeSquared(const Vec3<T>& v)
    {
        return v.magnitudeSquared();
    }

    template <typename T, typename S>
    inline T dot(const Vec3<T>& a, const Vec3<S>& b)
    {
        return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
    }

    template <typename T> inline Vec3<T> normalize(const Vec3<T>& v)
    {
        return v.normalized();
    }

    template <typename T, typename S>
    inline Vec3<T> cross(const Vec3<T>& a, const Vec3<S>& b)
    {
        return Vec3<T>((a.y * b.z) - (a.z * b.y), (a.z * b.x) - (a.x * b.z),
                       (a.x * b.y) - (a.y * b.x));
    }

    // Absolute value of the angle between two vectors. Never more than PI,
    // never less than zero.
    template <typename T, typename S>
    inline T angleBetween(const Vec3<T>& a, const Vec3<S>& b)
    {
        const Vec3<T> an = normalize(a);
        const Vec3<S> bn = normalize(b);
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

    // GLOBAL ARITHMETIC OPERATORS

    // ADDITION
    template <typename T, typename S>
    inline Vec3<T> operator+(const Vec3<T>& a, const S& b)
    {
        return Vec3<T>(a.x + b, a.y + b, a.z + b);
    }

    template <typename T, typename S>
    inline Vec3<T> operator+(const S& a, const Vec3<T>& b)
    {
        return Vec3<T>(a + b.x, a + b.y, a + b.z);
    }

    template <typename T, typename S>
    inline Vec3<T> operator+(const Vec3<T>& a, const Vec3<S>& b)
    {
        return Vec3<T>(a.x + b.x, a.y + b.y, a.z + b.z);
    }

    // NEGATION
    template <typename T> inline Vec3<T> operator-(const Vec3<T>& a)
    {
        return Vec3<T>(-a.x, -a.y, -a.z);
    }

    // SUBTRACTION
    template <typename T, typename S>
    inline Vec3<T> operator-(const Vec3<T>& a, const S& b)
    {
        return Vec3<T>(a.x - b, a.y - b, a.z - b);
    }

    template <typename T, typename S>
    inline Vec3<T> operator-(const S& a, const Vec3<T>& b)
    {
        return Vec3<T>(a - b.x, a - b.y, a - b.z);
    }

    template <typename T, typename S>
    inline Vec3<T> operator-(const Vec3<T>& a, const Vec3<S>& b)
    {
        return Vec3<T>(a.x - b.x, a.y - b.y, a.z - b.z);
    }

    // MULTIPLICATION
    template <typename T, typename S>
    inline Vec3<T> operator*(const Vec3<T>& a, const S& b)
    {
        return Vec3<T>(a.x * b, a.y * b, a.z * b);
    }

    template <typename T, typename S>
    inline Vec3<T> operator*(const S& a, const Vec3<T>& b)
    {
        return Vec3<T>(a * b.x, a * b.y, a * b.z);
    }

    template <typename T, typename S>
    inline Vec3<T> operator*(const Vec3<T>& a, const Vec3<S>& b)
    {
        return Vec3<T>(a.x * b.x, a.y * b.y, a.z * b.z);
    }

    // DIVISION
    template <typename T, typename S>
    inline Vec3<T> operator/(const Vec3<T>& a, const S& b)
    {
        assert(b != (T)0);
        return Vec3<T>(a.x / b, a.y / b, a.z / b);
    }

    template <typename T, typename S>
    inline Vec3<T> operator/(const S& a, const Vec3<T>& b)
    {
        assert(b.x != (T)0);
        assert(b.y != (T)0);
        assert(b.z != (T)0);
        return Vec3<T>(a / b.x, a / b.y, a / b.z);
    }

    template <typename T, typename S>
    inline Vec3<T> operator/(const Vec3<T>& a, const Vec3<S>& b)
    {
        assert(b.x != (T)0);
        assert(b.y != (T)0);
        assert(b.z != (T)0);
        return Vec3<T>(a.x / b.x, a.y / b.y, a.z / b.z);
    }

    // COMPARISON OPERATORS

    template <typename T, typename T2>
    inline bool operator==(const Vec3<T>& a, const Vec3<T2>& b)
    {
        return (a.x == b.x) && (a.y == b.y) && (a.z == b.z);
    }

    template <typename T, typename T2>
    inline bool operator!=(const Vec3<T>& a, const Vec3<T2>& b)
    {
        return (a.x != b.x) || (a.y != b.y) || (a.z != b.z);
    }

#ifdef COMPILER_GCC2_96
    inline bool operator==(const Vec3<float>& a, const Vec3<float>& b)
    {
        return (a.x == b.x) && (a.y == b.y) && (a.z == b.z);
    }

    inline bool operator!=(const Vec3<float>& a, const Vec3<float>& b)
    {
        return (a.x != b.x) || (a.y != b.y) || (a.z != b.z);
    }
#endif

} // End namespace TwkMath

#endif
