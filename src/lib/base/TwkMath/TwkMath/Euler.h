//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkMath__TwkMathEuler__h__
#define __TwkMath__TwkMathEuler__h__
#include <TwkMath/Vec3.h>
#include <TwkMath/Mat44.h>
#include <TwkMath/Quat.h>
#include <TwkMath/Math.h>

namespace TwkMath
{

    //
    //  class Euler
    //
    //  This class is based/lifted on/from Ken Shoemake's piece of work in
    //  GGems IV. The different encodings have been made into an enum.
    //

    template <typename T> class Euler : public Vec3<T>
    {
    public:
        typedef Vec3<T> Vec;
        typedef Mat44<T> Mat4;

        enum Parity
        {
            Odd = 0,
            Even = 1
        };

        enum Order
        {
            XYZ = 0x0100,
            XYX = 0x0110,
            XZY = 0x0000,
            XZX = 0x0010,
            YZX = 0x1100,
            YZY = 0x1110,
            YXZ = 0x1000,
            YXY = 0x1010,
            ZXY = 0x2100,
            ZXZ = 0x2110,
            ZYX = 0x2000,
            ZYZ = 0x2010,
            //              ----
            //              ABCD
            //
            //  A = axis number
            //  B = parity (even == 1, odd == 0)
            //  C = repeating (no == 0, yes == 1)
            //  D = 0 == static, 1 == relative
            //
        };

        Euler();
        Euler(const Vec& angles, Order);
        Euler(const Mat4& M, Order);
        Euler(const Euler<T>& euler, Order);

        bool repeating() const { return (m_order & 0x10) == 0x10; }

        int parity() const { return (m_order & 0x100) == 0x100; }

        int relative() const { return (m_order & 0x1) == 0x1; }

        Order order() const { return m_order; }

        void indexOrder(int& i, int& j, int& k) const;

        Mat4 matrix44() const;
        void decompose(const Mat4&, Order);

        operator Mat4() const { return matrix44(); }

        void setXYZVector(const Vec&);
        Vec toXYZVector() const;

        //
        //  FROM IMATH:
        //
        //  Utility methods for getting continuous rotations. None of these
        //  methods change the orientation given by its inputs (or at least
        //  that is the intent).
        //
        //    angleMod() converts an angle to its equivalent in [-PI, PI]
        //
        //    simpleXYZRotation() adjusts xyzRot so that its components differ
        //                        from targetXyzRot by no more than +-PI
        //
        //    nearestRotation() adjusts xyzRot so that its components differ
        //                      from targetXyzRot by as little as possible.
        //                      Note that xyz here really means ijk, because
        //                      the order must be provided.
        //
        //    makeNear() adjusts "this" Euler so that its components differ
        //               from target by as little as possible. This method
        //               might not make sense for Eulers with different order
        //               and it probably doesn't work for repeated axis and
        //               relative orderings (TODO).
        //

        static T angleMod(T angle);
        static void simpleXYZRotation(Vec& xyzRot, const Vec& targetXyzRot);
        static void nearestRotation(Vec& xyzRot, const Vec& targetXyzRot,
                                    Order order = XYZ);

        void makeNear(const Euler<T>& target);

    private:
        Order m_order;
    };

    template <typename T>
    Euler<T>::Euler()
        : Vec(0, 0, 0)
        , m_order(Euler::ZYX)
    {
    }

    template <typename T> Euler<T>::Euler(const Euler<T>& euler, Order order)
    {
        Mat4 M = euler.matrix44();
        decompose(M, order);
    }

    template <typename T>
    Euler<T>::Euler(const Vec3<T>& angles, Order order)
        : Vec(angles)
        , m_order(order)
    {
    }

    template <typename T> Euler<T>::Euler(const Mat44<T>& M, Order order)
    {
        decompose(M, order);
    }

    template <typename T>
    inline void Euler<T>::indexOrder(int& i, int& j, int& k) const
    {
        i = m_order >> 12;
        j = parity() == Even ? (i + 1) % 3 : (i > 0 ? i - 1 : 2);
        k = parity() == Even ? (i > 0 ? i - 1 : 2) : (i + 1) % 3;
    }

    template <typename T> Mat44<T> Euler<T>::matrix44() const
    {
        bool r = relative();
        Vec angles(r ? Vec::z : Vec::x, Vec::y, r ? Vec::x : Vec::z);

        if (parity() == Odd)
        {
            angles *= T(-1);
        }

        T ci = Math<T>::cos(angles.x);
        T cj = Math<T>::cos(angles.y);
        T ch = Math<T>::cos(angles.z);
        T si = Math<T>::sin(angles.x);
        T sj = Math<T>::sin(angles.y);
        T sh = Math<T>::sin(angles.z);
        T cc = ci * ch;
        T cs = ci * sh;
        T sc = si * ch;
        T ss = si * sh;

        if (repeating())
        {
            return Mat4(cj, sj * si, sj * ci, T(0), sj * sh, -cj * ss + cc,
                        -cj * cs - sc, T(0), -sj * ch, cj * sc * cs,
                        cj * cc - ss, T(0), T(0), T(0), T(0), T(1));
        }
        else
        {
            return Mat4(cj * ch, sj * sc - cs, sj * cc + ss, T(0), cj * sh,
                        sj * ss + cc, sj * cs - sc, T(0), -sj, cj * si, cj * ci,
                        T(0), T(0), T(0), T(0), T(1));
        }
    }

    template <typename T>
    void Euler<T>::decompose(const Mat44<T>& M, Euler<T>::Order order)
    {
        m_order = order;
        int i, j, k;
        indexOrder(i, j, k);

        //
        //  Use the same method ILM's Imath does to remove the numeric
        //  problem with gimbal lock: get the first angle, remove it then
        //  get the other angles. This differs from the original Shoemake
        //  code.
        //

        if (repeating())
        {
            Vec::x = Math<T>::atan2(M(i, j), M(i, k));
            Vec axis(0, 0, 0);
            axis[i] = 1;

            Mat44<T> R;
            R.makeRotation(axis, parity() == Odd ? Vec::x : -Vec::x);
            R = M * R;

            T sy = Math<T>::sqrt(R(i, j) * R(i, j) + R(i, k) * R(i, k));
            Vec::y = Math<T>::atan2(sy, R(i, i));
            Vec::z = Math<T>::atan2(R(k, j), R(j, j));
        }
        else
        {
            Vec::x = Math<T>::atan2(M(k, j), M(k, k));
            Vec axis(0, 0, 0);
            axis[i] = 1;

            Mat44<T> R;
            R.makeRotation(axis, parity() == Odd ? Vec::x : -Vec::x);
            R = M * R;

            T cy = Math<T>::sqrt(R(i, i) * R(i, i) + R(j, i) * R(j, i));
            Vec::y = Math<T>::atan2(-R(k, i), cy);
            Vec::z = Math<T>::atan2(-R(i, j), R(j, j));
        }

        if (parity() == Odd)
        {
            (*this) *= T(-1);
        }

        if (relative())
        {
            std::swap(Vec::x, Vec::z);
        }
    }

    template <class T> inline void Euler<T>::setXYZVector(const Vec3<T>& v)
    {
        int i, j, k;
        indexOrder(i, j, k);
        (*this)[i] = v.x;
        (*this)[j] = v.y;
        (*this)[k] = v.z;
    }

    template <class T> inline Vec3<T> Euler<T>::toXYZVector() const
    {
        int i, j, k;
        indexOrder(i, j, k);
        return Vec3<T>((*this)[i], (*this)[j], (*this)[k]);
    }

    template <class T> T Euler<T>::angleMod(T angle)
    {
        angle = Math<T>::mod(T(angle), T(2 * Math<T>::pi()));

        if (angle < -Math<T>::pi())
            angle += 2 * Math<T>::pi();
        if (angle > +Math<T>::pi())
            angle -= 2 * Math<T>::pi();

        return angle;
    }

    template <class T>
    void Euler<T>::simpleXYZRotation(Vec& xyzRot, const Vec& targetXyzRot)
    {
        Vec d = xyzRot - targetXyzRot;
        xyzRot[0] = targetXyzRot[0] + angleMod(d[0]);
        xyzRot[1] = targetXyzRot[1] + angleMod(d[1]);
        xyzRot[2] = targetXyzRot[2] + angleMod(d[2]);
    }

    template <class T>
    void Euler<T>::nearestRotation(Vec& xyzRot, const Vec& targetXyzRot,
                                   Order order)
    {
        int i, j, k;
        Euler<T> e(Vec(0, 0, 0), order);
        e.indexOrder(i, j, k);

        simpleXYZRotation(xyzRot, targetXyzRot);

        Vec otherXyzRot;
        otherXyzRot[i] = Math<T>::pi() + xyzRot[i];
        otherXyzRot[j] = Math<T>::pi() - xyzRot[j];
        otherXyzRot[k] = Math<T>::pi() + xyzRot[k];

        simpleXYZRotation(otherXyzRot, targetXyzRot);

        Vec d = xyzRot - targetXyzRot;
        Vec od = otherXyzRot - targetXyzRot;
        T dMag = dot(d, d);
        T odMag = dot(od, od);

        if (odMag < dMag)
        {
            xyzRot = otherXyzRot;
        }
    }

    template <class T> void Euler<T>::makeNear(const Euler<T>& target)
    {
        Vec xyzRot = toXYZVector();
        Euler<T> targetSameOrder = Euler<T>(target, order());
        Vec targetXyz = targetSameOrder.toXYZVector();

        nearestRotation(xyzRot, targetXyz, order());

        setXYZVector(xyzRot);
    }

} // namespace TwkMath

#endif // __TwkMath__TwkMathEuler__h__
