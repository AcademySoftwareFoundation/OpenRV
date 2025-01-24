//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkMath__TwkMathQuat__h__
#define __TwkMath__TwkMathQuat__h__
#include <TwkMath/Vec3.h>
#include <TwkMath/Vec4.h>
#include <TwkMath/Mat33.h>
#include <TwkMath/Mat44.h>
#include <TwkMath/Math.h>

namespace TwkMath
{

    //
    //  Quaternion class based largely off of the description you can find
    //  in the MIT HACKMEM documents.
    //
    //  Routines for converting between Quaternions and Matrices can be
    //  found in the Algo class.
    //
    //  One operator (operator~) is defined to mean something different
    //  from its usual meaning (binary complement). For quaternions
    //  operator~ returns the conjugate of the quaternion.
    //
    //  The API is a mixture of the matrix API and the Vec
    //  APIs. Invertion, for example, is invert() and inverted() like
    //  matrix. The magnitude of a quaternion is obtained with magnitude()
    //  and magnitudeSquared() like Vec. Generally, quaternions behave
    //  similarily to vectors in addition and to matrices in
    //  multiplication (i.e. quaternion multiplication is not
    //  commutative).
    //
    //  identity for a Quat<T> is 1. So q * 1 == q;
    //

    template <typename T> class Quat
    {
    public:
        typedef Vec3<T> Vec;
        typedef T Scalar;
        typedef T Radians;
        typedef Quat<T> Q;

        //
        //	The members are a scalar type + the imaginary vector.
        //	Alternately you can access the components of the quaternion
        //	through the data member as a 4 array.
        //

        Scalar s;
        Vec v;

        Quat()
            : s(T(0))
            , v(T(0))
        {
        }

        Quat(Scalar sc)
            : s(sc)
            , v(T(0))
        {
        }

        Quat(Scalar sc, Vec ve)
            : s(sc)
            , v(ve)
        {
        }

        Quat(Scalar sc, Scalar i, Scalar j, Scalar k)
            : s(sc)
            , v(i, j, k)
        {
        }

        template <typename S> Quat(const Mat33<S>&);

        template <typename S>
        Quat(Quat<S> q)
            : s(q.s)
            , v(q.v)
        {
        }

        template <typename S>
        explicit Quat(const Vec4<S>& a)
            : s(a.x)
            , v(a.y, a.z, a.w)
        {
        }

        template <typename S> operator Vec4<S>() const
        {
            return Vec4<S>(s, v.x, v.y, v.z);
        }

        template <typename S> operator Mat33<S>() const;
        template <typename S> operator Mat44<S>() const;

        //
        //	The conjugate operator. Same as the complex conjugate, but the
        //	negation applies to all of the imaginary components.
        //

        Q operator~() const { return Q(s, -v); }

        //
        //	Array style access
        //

        Scalar operator[](size_t i) const { return (&s)[i]; }

        Scalar& operator[](size_t i) { return (&s)[i]; }

        //
        //	An inverted quaternion, which is defined as this:
        //
        //                    2
        //      norm(Q) == |Q|
        //
        //	    1     ~Q          ~Q
        //	    - = -------  =  -------
        //	    Q   ~Q  * Q     norm(Q)
        //
        //	since ~Q * Q == Q * ~Q == mag2(Q);
        //
        //  So! If Q is an orientation -- which means |Q| == 1 then
        //  norm(Q) == 1, then the inverse of the quaternion is just ~Q!
        //  These functions however do *NOT* assume that Q is normalized
        //

        Q inverted() const;
        void invert();

        //
        //	A normalized quaternion is one which has the magnitude of 1:
        //	you can treat it like a 4 dimensional vector.
        //

        Q normalized() const;
        void normalize();

        //
        //	Assignment operators
        //

        template <typename S> Q& operator+=(const Quat<S>&);
        template <typename S> Q& operator-=(const Quat<S>&);
        template <typename S> Q& operator*=(const Quat<S>&);
        template <typename S> Q& operator/=(const Quat<S>&);

        template <typename S> Q& operator*=(S);
        template <typename S> Q& operator/=(S);

        //
        //	Comparison operators
        //

        bool operator==(const Q& q) const { return s == q.s && v == q.v; }

        bool operator!=(const Q& q) const { return s != q.s || v != q.v; }

        //
        //	Some common operations
        //

        void setAxisAngle(const Vec&, Radians);
        Scalar angle() const;
        Vec axis() const;

        void rotateVector(const Vec& from, const Vec& to);
    };

    template <typename T> template <typename S> Quat<T>::Quat(const Mat33<S>& R)
    {
        const S t0 = R(0, 0);
        const S t1 = R(1, 1);
        const S t2 = R(2, 2);
        const S trace = t0 + t1 + t2 + T(1);

        if (trace > S(0))
        {
            S sq = S(.5) / Math<S>::sqrt(trace);
            s = S(.25) / sq;
            v = Vec(R(2, 1) - R(1, 2), R(0, 2) - R(2, 0), R(1, 0) - R(0, 1))
                * sq;
        }
        else
        {
            size_t i = t1 > t0 ? 1 : 0; // find permutation indices
            i = t2 > R(i, i) ? 2 : i;
            const size_t j = (i + 1) % 3;
            const size_t k = (j + 1) % 3;

            S sq = Math<S>::sqrt(R(i, i) - R(j, j) - R(k, k) + T(1));
            v[i] = S(.5) * sq;
            sq = S(.5) / sq;
            s = sq * (R(k, j) - R(j, k));
            v[j] = sq * (R(j, i) + R(i, j));
            v[k] = sq * (R(k, i) + R(i, k));
        }

        normalize();
    }

    template <typename T>
    template <typename S>
    inline Quat<T>& Quat<T>::operator+=(const Quat<S>& a)
    {
        s += a.s;
        v += a.v;
        return *this;
    }

    template <typename T>
    template <typename S>
    inline Quat<T>& Quat<T>::operator-=(const Quat<S>& a)
    {
        s -= a.s;
        v -= a.v;
        return *this;
    }

    template <typename T>
    template <typename S>
    inline Quat<T>& Quat<T>::operator*=(const Quat<S>& a)
    {
        v = dot(v, a.v) + s * a.v + a.s * v + cross(v, a.v);
        s *= a.s;
        return *this;
    }

    template <typename T>
    template <typename S>
    inline Quat<T>&
    // ajg !!! Does this work in win32 !!!
    Quat<T>::operator*=(S r)
    {
        s *= r;
        v *= r;
        return *this;
    }

    template <typename T>
    template <typename S>
    inline Quat<T>&
    // ajg !!! Does this work in win32 !!!
    Quat<T>::operator/=(S r)
    {
        s /= r;
        v /= r;
        return *this;
    }

    template <typename T, typename S>
    inline Quat<T> operator+(const Quat<T>& a, const Quat<S>& b)
    {
        return Quat<T>(a.s + b.s, a.v + b.v);
    }

    template <typename T, typename S>
    inline Quat<T> operator-(const Quat<T>& a, const Quat<S>& b)
    {
        return Quat<T>(a.s - b.s, a.v - b.v);
    }

    template <typename T> inline Quat<T> operator-(const Quat<T>& a)
    {
        return Quat<T>(-a.s, -a.v);
    }

    template <typename T, typename S>
    inline Quat<T> operator*(const Quat<T>& a, const Quat<S>& b)
    {
        return Quat<T>(a.s * b.s - dot(a.v, b.v),
                       a.s * b.v + b.s * a.v + cross(a.v, b.v));
    }

    template <typename T, typename S>
    inline Quat<T> operator*(const Quat<T>& a, S b)
    {
        return Quat<T>(a.s * b, a.v * b);
    }

    template <typename T, typename S>
    inline Vec3<T> operator*(const Quat<T>& q, const Vec3<S> v)
    {
        //
        //  This only works if q is normalized! But presumably it is or
        //  you wouldn't be multiplying a point by it would you!
        //

        return (q * Quat<T>(T(0), v) * ~q).v;
    }

    template <typename T, typename S>
    inline Quat<T> operator/(const Quat<T>& a, S b)
    {
        return Quat<T>(a.s / b, a.v / b);
    }

    template <typename T> inline T magnitudeSquared(const Quat<T>& a)
    {
        return a.s * a.s + dot(a.v, a.v);
    }

    template <typename T> inline T norm(const Quat<T>& a)
    {
        return magnitudeSquared(a);
    }

    template <typename T> inline T magnitude(const Quat<T>& a)
    {
        return Math<T>::sqrt(norm(a));
    }

    template <typename T> inline Quat<T> Quat<T>::inverted() const
    {
        return Quat<T>(~(*this) / norm(*this));
    }

    template <typename T> inline void Quat<T>::invert() { *this = inverted(); }

    template <typename T> inline Quat<T> inverse(const Quat<T>& q)
    {
        return q.inverted();
    }

    template <typename T> inline void Quat<T>::normalize()
    {
        T m = magnitude(*this);
        s /= m;
        v /= m;
    }

    template <typename T> inline Quat<T> Quat<T>::normalized() const
    {
        T m = magnitude(*this);
        return Quat<T>(s / m, v / m);
    }

    template <typename T> inline Quat<T> normalize(const Quat<T>& q)
    {
        return q.normalized();
    }

    template <typename T, typename S>
    inline Quat<T> operator/(const Quat<T>& a, const Quat<S>& b)
    {
        return Quat<T>(a * b.inverted());
    }

    template <typename T>
    template <typename S>
    inline Quat<T>& Quat<T>::operator/=(const Quat<S>& a)
    {
        return *this *= a.inverted();
    }

    template <typename T>
    inline void Quat<T>::setAxisAngle(const typename Quat<T>::Vec& axis,
                                      T radians)
    {
        s = Math<T>::cos(radians / T(2));
        v = axis.normalized() * Math<T>::sin(radians / T(2));
    }

    template <typename T>
    void Quat<T>::rotateVector(const Vec3<T>& from, const Vec3<T>& to)
    {
        Vec3<T> nf = from.normalized();
        Vec3<T> nt = to.normalized();

        T d = dot(nf, nt);

        if (Math<T>::abs(T(1) - d) <= Math<T>::epsilon())
        {
            s = T(1);
            v = Vec3<T>(T(0));
        }
        else if (Math<T>::abs(T(1) + d) <= Math<T>::epsilon())
        {
            Vec3<T> a(T(0));
            a[minorComponent(nf)] = T(1);
            v = cross(cross(a, nf), nf).normalized();
            s = 0;
        }
        else
        {
            // (FROM IMATH: TODO: REPLACE)

            //
            // Use half-angle formulae:
            // cos^2 t = ( 1 + cos (2t) ) / 2
            // w part is cosine of half the rotation angle
            //

            T cost =
                dot(from, to) / Math<T>::sqrt(dot(from, from) * dot(to, to));
            s = Math<T>::sqrt(T(0.5) * (T(1) + cost));

            //
            // sin^2 t = ( 1 - cos (2t) ) / 2
            // Do the normalization of the axis vector at the same time so
            // we only call sqrt once.
            //

            v = cross(from, to);
            v *= Math<T>::sqrt((T(0.5) * (T(1) - cost)) / dot(v, v));
        }
    }

    template <typename T, typename S>
    inline T dot(const Quat<T>& a, const Quat<S>& b)
    {
        return dot(a.v, b.v) + a.s * b.s;
    }

    template <typename T>
    template <typename S>
    inline Quat<T>::operator Mat33<S>() const
    {
        const S xx = v.x * v.x;
        const S yy = v.y * v.y;
        const S zz = v.z * v.z;
        const S xy = v.x * v.y;
        const S xz = v.x * v.z;
        const S yz = v.y * v.z;
        const S sx = v.x * s;
        const S sy = v.y * s;
        const S sz = v.z * s;

        const S a = S(1) - S(2) * (yy + zz);
        const S b = S(2) * (xy + sz);
        const S c = S(2) * (xz - sy);
        const S d = S(2) * (xy - sz);
        const S e = S(1) - S(2) * (zz + xx);
        const S f = S(2) * (yz + sx);
        const S g = S(2) * (xz + sy);
        const S h = S(2) * (yz - sx);
        const S i = S(1) - S(2) * (yy + xx);

        return Mat33<S>(a, d, g, b, e, h, c, f, i);
    }

    template <typename T>
    template <typename S>
    inline Quat<T>::operator Mat44<S>() const
    {
        const S xx = v.x * v.x;
        const S yy = v.y * v.y;
        const S zz = v.z * v.z;
        const S xy = v.x * v.y;
        const S xz = v.x * v.z;
        const S yz = v.y * v.z;
        const S sx = v.x * s;
        const S sy = v.y * s;
        const S sz = v.z * s;

        const S a = S(1) - S(2) * (yy + zz);
        const S b = S(2) * (xy + sz);
        const S c = S(2) * (xz - sy);
        const S d = S(2) * (xy - sz);
        const S e = S(1) - S(2) * (zz + xx);
        const S f = S(2) * (yz + sx);
        const S g = S(2) * (xz + sy);
        const S h = S(2) * (yz - sx);
        const S i = S(1) - S(2) * (yy + xx);

        return Mat44<S>(a, d, g, S(0), b, e, h, S(0), c, f, i, S(0), S(0), S(0),
                        S(0), S(1));
    }

    template <typename T> inline T Quat<T>::angle() const
    {
        return Math<T>::acos(s) * T(2);
    }

    template <typename T> Vec3<T> Quat<T>::axis() const
    {
        return v.normalized();
    }

    template <typename T, typename S>
    Quat<T> slerp(const Quat<T>& q1, const Quat<T>& q2, const S& t)
    {
        // Spherical linear interpolation.
        // Given two quaternions q1 and q2, there
        // are two possible arcs along which one can move,
        // corresponding to alternative starting directions.
        // This routine chooses the shorter arc for the
        // interpolation of q1 and q2, which is more desirable.
        T qdot = dot(q1, q2);
        static const T epsilon = .0001;
        Quat<T> result;
        T s1, s2;

        Quat<T> q1b = q1;

        if (qdot < T(0))
        {
            // If q1 and q2 are more than 90 degrees apart,
            // flip q1 - this gives us the shorter arc.
            qdot = -qdot;
            q1b = -q1b;
        }

        if ((T(1) - qdot) > epsilon)
        {
            T omega = Math<T>::acos(qdot);
            T sinom = Math<T>::sin(omega);
            s1 = Math<T>::sin((S(1) - t) * omega) / sinom;
            s2 = Math<T>::sin(t * omega) / sinom;
        }
        else
        {
            // If q1 and q2 are very close together, do
            // simple linear interpolation:
            s1 = S(1) - t;
            s2 = t;
        }

        return q1b * s1 + q2 * s2;
    }

    typedef Quat<float> Quatf;
    typedef Quat<double> Quatd;
    typedef Quat<half> Quath;

} // namespace TwkMath

#endif // __TwkMath__TwkMathQuat__h__
