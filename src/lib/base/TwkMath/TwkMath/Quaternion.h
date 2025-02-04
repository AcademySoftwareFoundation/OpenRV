//******************************************************************************
// Copyright (c) 2001-2002 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkMathQuaternion_h_
#define _TwkMathQuaternion_h_

#include <TwkMath/Vec3.h>
#include <TwkMath/Mat44.h>
#include <TwkMath/Math.h>
#include <sys/types.h>
#include <assert.h>

#define T0 ((T)0)
#define T1 ((T)1)
#define T2 ((T)2)
#define QUATERNION_NORMALIZATION_THRESHOLD 64

namespace TwkMath
{

    template <typename T> class Quaternion
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
#ifdef COMPILER_GCC2
        T x;
        T y;
        T z;
        T w;
#else
        union
        {
            T q[4];

            struct
            {
                T x;
                T y;
                T z;
                T w;
            };
        };
#endif

        //**************************************************************************
        // CONSTRUCTORS
        //**************************************************************************
        // Default constructor. Initializes to identity.
        Quaternion() { setValue(Vec3<T>(T0, T0, T1), T0); }

        // Array constructor. Explicit so that arrays are not automatically
        // converted into quaternions. In-class because VC++ needs it to be.
        explicit Quaternion(const T* a)
            : x(a[0])
            , y(a[1])
            , z(a[2])
            , w(a[3])
            , m_counter(0)
        {
        }

        // Value by value constructor.
        Quaternion(const T& q0, const T& q1, const T& q2, const T& q3)
            : x(q0)
            , y(q1)
            , z(q2)
            , w(q3)
            , m_counter(0)
        {
        }

        // Matrix constructor. Again, explicit so that matrices are not
        // invisibly converted into quaternions.
        explicit Quaternion(const Mat44<T>& m) { setValue(m); }

        // Axis-angle constructor
        Quaternion(const Vec3<T>& axis, const T& radians)
        {
            setValue(axis, radians);
        }

        // Euler angle constructor
        Quaternion(const Vec3<T>& rotateFrom, const Vec3<T>& rotateTo)
        {
            setValue(rotateFrom, rotateTo);
        }

        // Highly constrained rotation
        Quaternion(const Vec3<T>& fromLook, const Vec3<T>& fromUp,
                   const Vec3<T>& toLook, const Vec3<T>& toUp)
        {
            setValue(fromLook, fromUp, toLook, toUp);
        }

        //**************************************************************************
        // COPY CONSTRUCTORS
        //**************************************************************************
#if 0
    template <typename T2>
    Quaternion( const Quaternion<T2> &copy )
      : x( copy.x ), y( copy.y ), z( copy.z ), w( copy.w ),
        m_counter( copy.m_counter ) {}
#endif

        Quaternion(const Quaternion<T>& copy)
            : x(copy.x)
            , y(copy.y)
            , z(copy.z)
            , w(copy.w)
            , m_counter(copy.m_counter)
        {
        }

        //**************************************************************************
        // ASSIGNMENT OPERATORS
        //**************************************************************************
#if 0
    template <typename T2>
    Quaternion<T> &operator=( const Quaternion<T2> &copy )
    {
        x = copy.x;
        y = copy.y;
        z = copy.z;
        w = copy.w;
        m_counter = copy.m_counter;
        return *this;
    }
#endif

        Quaternion<T>& operator=(const Quaternion<T>& copy)
        {
            x = copy.x;
            y = copy.y;
            z = copy.z;
            w = copy.w;
            m_counter = copy.m_counter;
            return *this;
        }

        //**************************************************************************
        // DATA ACCESS
        //**************************************************************************
        T& operator[](size_type i);
        const T& operator[](size_type i) const;

        void value(T& q0, T& q1, T& q2, T& q3) const
        {
            q0 = x;
            q1 = y;
            q2 = z;
            q3 = w;
        }

        void value(Vec3<T>& axis, T& radians) const;
        void value(Mat44<T>& m) const;

        void setValue(const T& q0, const T& q1, const T& q2, const T& q3)
        {
            x = q0;
            y = q1;
            z = q2;
            w = q3;
            m_counter = 0;
        }

        void setValue(const T* a)
        {
            x = a[0];
            y = a[1];
            z = a[2];
            w = a[2];
            m_counter = 0;
        }

        void setValue(const Mat44<T>& m);
        void setValue(const Vec3<T>& axis, const T& theta);
        void setValue(const Vec3<T>& rotateFrom, const Vec3<T>& rotateTo);
        void setValue(const Vec3<T>& fromLook, const Vec3<T>& fromUp,
                      const Vec3<T>& toLook, const Vec3<T>& toUp);

        //**************************************************************************
        // GEOMETRIC OPERATIONS
        //**************************************************************************
        void makeIdentity();

        void normalize();
        Quaternion<T> normalized() const;

        void conjugate();
        Quaternion<T> conjugated() const;

        void invert();
        Quaternion<T> inverted() const;

        void scaleAngle(const T& scaleFactor);

        //**************************************************************************
        // ARITHMETIC OPERATORS
        //**************************************************************************
        Quaternion<T>& operator*=(const Quaternion<T>& qr);

    protected:
        void counterNormalize();

        // renormalization counter
        unsigned char m_counter;
    };

    //*****************************************************************************
    //*****************************************************************************
    // TYPEDEFS
    //*****************************************************************************
    //*****************************************************************************
    typedef Quaternion<float> Qtnf;
    typedef Quaternion<double> Qtnd;

    //*****************************************************************************
    //*****************************************************************************
    // TEMPLATE AND INLINE FUNCTIONS
    //*****************************************************************************
    //*****************************************************************************
    template <typename T>
    inline T& Quaternion<T>::operator[](Quaternion<T>::size_type i)
    {
        assert(i < 4);
#ifdef COMPILER_GCC2
        return ((T*)this)[i];
#else
        return q[i];
#endif
    }

    //******************************************************************************
    template <typename T>
    inline const T& Quaternion<T>::operator[](Quaternion<T>::size_type i) const
    {
        assert(i < 4);
#ifdef COMPILER_GCC2
        return ((T*)this)[i];
#else
        return q[i];
#endif
    }

    //******************************************************************************
    template <typename T>
    void Quaternion<T>::value(Vec3<T>& axis, T& radians) const
    {
        radians = Math<T>::acos(w) * (T)2;
        if (radians == T0)
        {
            axis = Vec3<T>(T0, T0, T1);
        }
        else
        {
            axis[0] = x;
            axis[1] = y;
            axis[2] = z;
            axis.normalize();
        }
    }

    //******************************************************************************
    template <typename T> void Quaternion<T>::value(Mat44<T>& m) const
    {
        const T norm = (x * x) + (y * y) + (z * z) + (w * w);

        const T s = (norm == T0) ? T0 : (((T)2) / norm);

        const T xs = x * s;
        const T ys = y * s;
        const T zs = z * s;

        const T wx = w * xs;
        const T wy = w * ys;
        const T wz = w * zs;

        const T xx = x * xs;
        const T xy = x * ys;
        const T xz = x * zs;

        const T yy = y * ys;
        const T yz = y * zs;
        const T zz = z * zs;

        m(0, 0) = T1 - (yy + zz);
        m(1, 0) = xy + wz;
        m(2, 0) = xz - wy;

        m(0, 1) = xy - wz;
        m(1, 1) = T1 - (xx + zz);
        m(2, 1) = yz + wx;

        m(0, 2) = xz + wy;
        m(1, 2) = yz - wx;
        m(2, 2) = T1 - (xx + yy);

        m(3, 0) = m(3, 1) = m(3, 2) = m(0, 3) = m(1, 3) = m(2, 3) = T0;
        m(3, 3) = T1;
    }

    //******************************************************************************
    template <typename T> void Quaternion<T>::setValue(const Mat44<T>& m)
    {
        const T trace = m(0, 0) + m(1, 1) + m(2, 2);

        if (trace > T0)
        {
            T s = Math<T>::sqrt(trace + m(3, 3));
            w = s * (T)0.5;
            s = ((T)0.5) / s;

            x = (m(1, 2) - m(2, 1)) * s;
            y = (m(2, 0) - m(0, 2)) * s;
            z = (m(0, 1) - m(1, 0)) * s;
        }
        else
        {
#ifdef COMPILER_GCC2
            T* q = (T*)this;
#endif
            const int nxt[3] = {1, 2, 0};

            int i = 0;
            if (m(1, 1) > m(0, 0))
            {
                i = 1;
            }

            if (m(2, 2) > m(i, i))
            {
                i = 2;
            }

            const int j = nxt[i];
            const int k = nxt[j];

            T s = Math<T>::sqrt((m(i, j) - (m(j, j) + m(k, k))) + T1);

            q[i] = s * (T)0.5;
            s = ((T)0.5) / s;

            w = (m(j, k) - m(k, j)) * s;
            q[j] = (m(i, j) + m(j, i)) * s;
            q[k] = (m(i, k) + m(k, i)) * s;
        }

        m_counter = 0;
    }

    //*************************************************************************
    template <typename T> inline void Quaternion<T>::makeIdentity()
    {
        setValue(Vec3<T>(T0, T0, T1), T0);
    }

    //******************************************************************************
    template <typename T>
    void Quaternion<T>::setValue(const Vec3<T>& axis, const T& theta)
    {
        const T alen2 = axis.magnitudeSquared();

        if (alen2 <= Math<T>::epsilon())
        {
            // axis too small.
            makeIdentity();
        }
        else
        {
            const T newTheta = theta * (T)0.5;
            T sinTheta = Math<T>::sin(newTheta);

            if (alen2 != T1)
            {
                sinTheta /= Math<T>::sqrt(alen2);
            }

            x = sinTheta * axis[0];
            y = sinTheta * axis[1];
            z = sinTheta * axis[2];
            w = Math<T>::cos(newTheta);

            m_counter = 0;
        }
    }

    //*****************************************************************************
    template <typename T>
    void Quaternion<T>::setValue(const Vec3<T>& rotateFrom,
                                 const Vec3<T>& rotateTo)
    {
        Vec3<T> p1(rotateFrom.normalized());

        Vec3<T> p2(rotateTo.normalized());

        const T alpha = dot(p1, p2);

        if (alpha == T1)
        {
            // From and two vectors were parallel and aligned.
            makeIdentity();
        }
        // ensures that the anti-parallel case leads to a positive dot
        else if (alpha == ((T)-1))
        {
            Vec3<T> v;

            if (p1[0] != p1[1] || p1[0] != p1[2])
            {
                v[0] = p1[1];
                v[1] = p1[2];
                v[2] = p1[0];
            }
            else
            {
                v[0] = -p1[0];
                v[1] = p1[1];
                v[2] = p1[2];
            }

            v -= p1 * dot(p1, v);
            v.normalize();

            setValue(v, Math<T>::pi());
        }
        else
        {
            p1 = cross(p1, p2);
            p1.normalize();
            setValue(p1, Math<T>::acos(alpha));
            m_counter = 0;
        }
    }

    //******************************************************************************
    //******************************************************************************
    // GEOMETRIC OPERATORS
    //******************************************************************************
    //******************************************************************************
    template <typename T> void Quaternion<T>::normalize()
    {
        const T len = Math<T>::sqrt((w * w) + (x * x) + (y * y) + (z * z));
        if (len != T0)
        {
            x /= len;
            y /= len;
            z /= len;
            w /= len;
            m_counter = 0;
        }
    }

    //******************************************************************************
    template <typename T> inline Quaternion<T> Quaternion<T>::normalized() const
    {
        Quaternion<T> r(*this);
        r.normalize();
        return r;
    }

    //*****************************************************************************
    template <typename T> inline void Quaternion<T>::conjugate()
    {
        x *= (T)-1;
        y *= (T)-1;
        z *= (T)-1;
    }

    //******************************************************************************
    template <typename T> inline Quaternion<T> Quaternion<T>::conjugated() const
    {
        Quaternion<T> r(*this);
        r.conjugate();
        return r;
    }

    //*****************************************************************************
    template <typename T> inline void Quaternion<T>::invert() { conjugate(); }

    //*****************************************************************************
    template <typename T> inline Quaternion<T> Quaternion<T>::inverted() const
    {
        Quaternion<T> r(*this);
        r.invert();
        return r;
    }

    //*****************************************************************************
    template <typename T>
    inline void Quaternion<T>::scaleAngle(const T& scaleFactor)
    {
        Vec3<T> axis;
        T radians;

        value(axis, radians);
        radians *= scaleFactor;
        setValue(axis, radians);
    }

    //******************************************************************************
    //******************************************************************************
    // ARITHMETIC OPERATORS
    //******************************************************************************
    //******************************************************************************
    template <typename T> inline void Quaternion<T>::counterNormalize()
    {
        if (m_counter > QUATERNION_NORMALIZATION_THRESHOLD)
        {
            normalize();
        }
    }

    //******************************************************************************
    template <typename T>
    Quaternion<T>& Quaternion<T>::operator*=(const Quaternion<T>& qr)
    {
        Quaternion<T> ql(*this);

        w = (ql.w * qr.w) - (ql.x * qr.x) - (ql.y * qr.y) - (ql.z * qr.z);
        x = (ql.w * qr.x) + (ql.x * qr.w) + (ql.y * qr.z) - (ql.z * qr.y);
        y = (ql.w * qr.y) + (ql.y * qr.w) + (ql.z * qr.x) - (ql.x * qr.z);
        z = (ql.w * qr.z) + (ql.z * qr.w) + (ql.x * qr.y) - (ql.y * qr.x);

        m_counter += qr.m_counter;
        ++m_counter;
        counterNormalize();
        return *this;
    }

    //******************************************************************************
    //******************************************************************************
    // FUNCTIONS WHICH OPERATE ON QUATERNIONS
    //******************************************************************************
    //******************************************************************************
    template <typename T>
    inline bool areEquivalent(const Quaternion<T>& q1, const Quaternion<T>& q2,
                              const T& tolerance)
    {
        const T t = Math<T>::sqr(q1.x - q2.x) + Math<T>::sqr(q1.y - q2.y)
                    + Math<T>::sqr(q1.z - q2.z) + Math<T>::sqr(q1.w - q2.w);

        return (t <= tolerance);
    }

    //******************************************************************************
    template <typename T> inline void normalize(Quaternion<T>& q)
    {
        q.normalize();
    }

    //******************************************************************************
    template <typename T>
    inline Quaternion<T> normalized(const Quaternion<T>& q)
    {
        return q.normalized();
    }

    //******************************************************************************
    template <typename T> inline void conjugate(Quaternion<T>& q)
    {
        q.conjugate();
    }

    //******************************************************************************
    template <typename T>
    inline Quaternion<T> conjugated(const Quaternion<T>& q)
    {
        return q.conjugated();
    }

    //******************************************************************************
    template <typename T> inline void invert(Quaternion<T>& q) { q.invert(); }

    //******************************************************************************
    template <typename T> inline Quaternion<T> inverted(const Quaternion<T>& q)
    {
        return q.inverted();
    }

    //******************************************************************************
    template <typename T>
    inline void scaleAngle(Quaternion<T>& q, const T& angScale)
    {
        q.scaleAngle(q);
    }

    //*****************************************************************************
    //*****************************************************************************
    // SLERP FUNCTION
    // This function is the whole reason for having quaternions
    // It smoothly interpolates between two quaterions in an
    // "animation friendly" way.
    //*****************************************************************************
    //*****************************************************************************
    template <typename T>
    Quaternion<T> slerp(const Quaternion<T>& p, const Quaternion<T>& q,
                        const T& alph)
    {
        T cosOmega = (p.x * q.x) + (p.y * q.y) + (p.z * q.z) + (p.w * q.w);

        // if B is on opposite hemisphere from A, use -B instead
        bool bflip;
        if ((bflip = (cosOmega < T0)))
        {
            cosOmega = -cosOmega;
        }

        // complementary interpolation parameter
        T alpha = alph;
        T beta = T1 - alpha;

        if (cosOmega <= T1 - Math<T>::epsilon())
        {
            return p;
        }

        T omega = Math<T>::acos(cosOmega);
        T sinOmega = T1 / Math<T>::sin(omega);

        beta = Math<T>::sin(omega * beta) / sinOmega;
        alpha = Math<T>::sin(omega * alpha) / sinOmega;

        if (bflip)
        {
            alpha = -alpha;
        }

        return Quaternion<T>(
            (beta * p.x) + (alpha * q.x), (beta * p.y) + (alpha * q.y),
            (beta * p.z) + (alpha * q.z), (beta * p.w) + (alpha * q.w));
    }

    //******************************************************************************
    //******************************************************************************
    // ARITHMETIC OPERATORS
    //******************************************************************************
    //******************************************************************************
    // VECTOR MULTIPLICATION
    // Quaternion multiplication with cartesion vector
    // v' = q*v*q(star)
    template <typename T>
    Vec3<T> operator*(const Quaternion<T>& q, const Vec3<T>& v)
    {
        const T vCoef = (q.w * q.w) - (q.x * q.x) - (q.y * q.y) - (q.z * q.z);
        const T uCoef = T2 * ((v[0] * q.x) + (v[1] * q.y) + (v[2] * q.z));
        const T cCoef = T2 * q.w;

        return Vec3<T>((vCoef * v[0]) + (uCoef * q.x)
                           + (cCoef * ((q.y * v[2]) - (q.z * v[1]))),
                       (vCoef * v[1]) + (uCoef * q.y)
                           + (cCoef * ((q.z * v[0]) - (q.x * v[2]))),
                       (vCoef * v[2]) + (uCoef * q.z)
                           + (cCoef * ((q.x * v[1]) - (q.y * v[0]))));
    }

    //******************************************************************************
    // QUATERNION MULTIPLICATION
    template <typename T>
    inline Quaternion<T> operator*(const Quaternion<T>& q1,
                                   const Quaternion<T>& q2)
    {
        Quaternion<T> r(q1);
        r *= q2;
        return r;
    }

    //*****************************************************************************
    template <typename T>
    void Quaternion<T>::setValue(const Vec3<T>& fromLook, const Vec3<T>& fromUp,
                                 const Vec3<T>& toLook, const Vec3<T>& toUp)
    {
        Quaternion<T> rLook(fromLook, toLook);

        Vec3<T> rotatedFromUp(fromUp);
        rotatedFromUp = rLook * rotatedFromUp;

        Quaternion<T> rTwist(rotatedFromUp, toUp);

        *this = rTwist;
        *this *= rLook;
    }

    //******************************************************************************
    //******************************************************************************
    // COMPARISON OPERATORS
    //******************************************************************************
    //******************************************************************************
    template <typename T>
    inline bool operator==(const Quaternion<T>& q1, const Quaternion<T>& q2)
    {
        return ((q1.x == q2.x) && (q1.y == q2.y) && (q1.z == q2.z)
                && (q1.w == q2.w));
    }

    //******************************************************************************
    template <typename T>
    inline bool operator!=(const Quaternion<T>& q1, const Quaternion<T>& q2)
    {
        return ((q1.x != q2.x) || (q1.y != q2.y) || (q1.z != q2.z)
                || (q1.w != q2.w));
    }

} // End namespace TwkMath

#undef T0
#undef T1
#undef T2
#undef QUATERNION_NORMALIZATION_THRESHOLD

#endif
