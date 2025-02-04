#ifndef __Mu__Vector__h__
#define __Mu__Vector__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <iostream>
#include <math.h>

namespace Mu
{

    //
    //  template Vector<>
    //
    //  The class implements a vector of some type of a fixed size and
    //  data type. The current implementation is a stand - waiting for the
    //  template loop-unrolling code.
    //
    //  This cannot be a class with a constructor because it must live in
    //  a union. Fortunately, this isn't really a big deal since the
    //  default operator= is exactly what's needed anyway.
    //

    template <typename T, size_t S> struct Vector
    {
        typedef T value_type;

        T data[S];

        T& operator[](int i) { return data[i]; }

        T operator[](int i) const { return data[i]; }

        static size_t dimension() { return S; }
    };

    typedef Vector<float, 4> Vector4f;
    typedef Vector<float, 3> Vector3f;
    typedef Vector<float, 2> Vector2f;
    typedef Vector<double, 4> Vector4d;
    typedef Vector<double, 3> Vector3d;
    typedef Vector<double, 2> Vector2d;
    typedef Vector<int, 4> Vector4i;
    typedef Vector<int, 3> Vector3i;
    typedef Vector<int, 2> Vector2i;
    typedef Vector<unsigned short, 4> Vector4s;
    typedef Vector<unsigned short, 3> Vector3s;
    typedef Vector<unsigned short, 2> Vector2s;
    typedef Vector<unsigned char, 4> Vector4c;
    typedef Vector<unsigned char, 3> Vector3c;
    typedef Vector<unsigned char, 2> Vector2c;

    //----------------------------------------------------------------------

    template <typename T, size_t S>
    inline void assignOp(Vector<T, S>& a, const Vector<T, S>& b)
    {
        for (int i = 0; i < S; i++)
            a[i] = b[i];
    }

    template <typename T, size_t S>
    inline void assignOp(Vector<T, S>& a, const T b)
    {
        for (int i = 0; i < S; i++)
            a[i] = b;
    }

    template <typename T, size_t S>
    inline Vector<T, S> constructVector(const T a)
    {
        Vector<T, S> v;
        v.data[0] = a;
        return v;
    }

    template <typename T, size_t S>
    inline Vector<T, S> constructVector(const T a, const T b)
    {
        Vector<T, S> v;
        v.data[0] = a;
        v.data[1] = b;
        return v;
    }

    template <typename T, size_t S>
    inline Vector<T, S> constructVector(const T a, const T b, const T c)
    {
        Vector<T, S> v;
        v.data[0] = a;
        v.data[1] = b;
        v.data[2] = c;
        return v;
    }

    template <typename T, size_t S>
    inline Vector<T, S> constructVector(const T a, const T b, const T c,
                                        const T d)
    {
        Vector<T, S> v;
        v.data[0] = a;
        v.data[1] = b;
        v.data[2] = c;
        v.data[3] = d;
        return v;
    }

    template <typename T, size_t S>
    std::ostream& operator<<(std::ostream& o, const Vector<T, S>& v)
    {
        o << "<";
        for (int i = 0; i < S; i++)
        {
            if (i)
                o << ", ";
            o << v[i];
        }
        o << ">";
        return o;
    }

    template <typename T, size_t S>
    inline bool operator==(const Vector<T, S>& a, const Vector<T, S>& b)
    {
        for (int i = 0; i < S; i++)
            if (a[i] != b[i])
                return false;
        return true;
    }

    template <typename T, size_t S>
    inline bool operator!=(const Vector<T, S>& a, const Vector<T, S>& b)
    {
        return !(a == b);
    }

    template <typename T, size_t S>
    inline Vector<T, S> operator-(const Vector<T, S>& v)
    {
        Vector<T, S> n;
        for (int i = 0; i < S; i++)
            n[i] = -v[i];
        return n;
    }

    template <typename T, size_t S>
    inline Vector<T, S> operator+(const Vector<T, S>& a, const Vector<T, S>& b)
    {
        Vector<T, S> n;
        for (int i = 0; i < S; i++)
            n[i] = a[i] + b[i];
        return n;
    }

    template <typename T, size_t S>
    inline Vector<T, S> operator-(const Vector<T, S>& a, const Vector<T, S>& b)
    {
        Vector<T, S> n;
        for (int i = 0; i < S; i++)
            n[i] = a[i] - b[i];
        return n;
    }

    template <typename T, size_t S>
    inline Vector<T, S> operator*(const Vector<T, S>& a, const Vector<T, S>& b)
    {
        Vector<T, S> n;
        for (int i = 0; i < S; i++)
            n[i] = a[i] * b[i];
        return n;
    }

    template <typename T, size_t S>
    inline Vector<T, S> operator*(const Vector<T, S>& a, T b)
    {
        Vector<T, S> n;
        for (int i = 0; i < S; i++)
            n[i] = a[i] * b;
        return n;
    }

    template <typename T, size_t S>
    inline Vector<T, S> operator*(T b, const Vector<T, S>& a)
    {
        Vector<T, S> n;
        for (int i = 0; i < S; i++)
            n[i] = a[i] * b;
        return n;
    }

    template <typename T, size_t S>
    inline Vector<T, S> operator/(T b, const Vector<T, S>& a)
    {
        Vector<T, S> n;
        for (int i = 0; i < S; i++)
            n[i] = a[i] / b;
        return n;
    }

    template <typename T, size_t S>
    inline Vector<T, S> operator/(const Vector<T, S>& a, const Vector<T, S>& b)
    {
        Vector<T, S> n;
        for (int i = 0; i < S; i++)
            n[i] = a[i] / b[i];
        return n;
    }

    template <typename T, size_t S>
    inline Vector<T, S> operator/(const Vector<T, S>& a, T s)
    {
        Vector<T, S> v = a;
        for (int i = 0; i < S; i++)
            v[i] /= s;
        return v;
    }

    template <typename T, size_t S>
    inline T dot(const Vector<T, S>& a, const Vector<T, S>& b)
    {
        T s = 0;
        for (int i = 0; i < S; i++)
            s += a[i] * b[i];
        return s;
    }

    template <typename T, size_t S> inline T mag(const Vector<T, S>& a)
    {
        return T(::sqrt(dot(a, a)));
    }

    template <typename T, size_t S>
    inline Vector<T, S> normalize(const Vector<T, S>& a)
    {
        return a / mag(a);
    }

    template <typename T, size_t S>
    inline Vector<T, S> cross(const Vector<T, S>& a, const Vector<T, S>& b)
    {
        return a;
    }

    template <>
    inline Vector<float, 3> cross(const Vector<float, 3>& a,
                                  const Vector<float, 3>& b)
    {
        Vector<float, 3> c;
        c[0] = a[1] * b[2] - a[2] * b[1];
        c[1] = a[2] * b[0] - a[0] * b[2];
        c[2] = a[0] * b[1] - a[1] * b[0];
        return c;
    }

    inline Vector<float, 2> newVector(float x, float y)
    {
        Vector<float, 2> v;
        v[0] = x;
        v[1] = y;
        return v;
    }

    inline Vector<float, 3> newVector(float x, float y, float z)
    {
        Vector<float, 3> v;
        v[0] = x;
        v[1] = y;
        v[2] = z;
        return v;
    }

    inline Vector<float, 4> newVector(float x, float y, float z, float w)
    {
        Vector<float, 4> v;
        v[0] = x;
        v[1] = y;
        v[2] = z;
        v[3] = w;
        return v;
    }

} // namespace Mu

#endif // __Mu__Vector__h__
