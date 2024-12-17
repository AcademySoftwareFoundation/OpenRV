//******************************************************************************
// Copyright (c) 2001 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkMathIostream_h_
#define _TwkMathIostream_h_

#include <TwkMath/Vec1.h>
#include <TwkMath/Vec2.h>
#include <TwkMath/Vec3.h>
#include <TwkMath/Vec4.h>
#include <TwkMath/Box.h>
#include <TwkMath/Mat22.h>
#include <TwkMath/Mat33.h>
#include <TwkMath/Mat44.h>
#include <TwkMath/Quat.h>
#include <TwkMath/Interval.h>
#include <TwkMath/Plane.h>

namespace TwkMath
{

    //******************************************************************************
    // VEC 1
    //******************************************************************************
    template <typename T>
    inline std::istream& operator>>(std::istream& s, Vec1<T>& v)
    {
        s >> v.x;
        return s;
    }

    //******************************************************************************
    template <> inline std::istream& operator>>(std::istream& s, Vec1<char>& v)
    {
        int vx;
        s >> vx;
        v.x = vx;
        return s;
    }

    //******************************************************************************
    template <>
    inline std::istream& operator>>(std::istream& s, Vec1<unsigned char>& v)
    {
        int vx;
        s >> vx;
        v.x = vx;
        return s;
    }

    //******************************************************************************
    template <typename T>
    inline std::ostream& operator<<(std::ostream& s, const Vec1<T>& v)
    {
        s << "(" << v.x << ")";
        return s;
    }

    //******************************************************************************
    template <>
    inline std::ostream& operator<<(std::ostream& s, const Vec1<char>& v)
    {
        s << "(" << (int)v.x << ")";
        return s;
    }

    //******************************************************************************
    template <>
    inline std::ostream& operator<<(std::ostream& s,
                                    const Vec1<unsigned char>& v)
    {
        s << "(" << (int)v.x << ")";
        return s;
    }

    //******************************************************************************
    // VEC 2
    //******************************************************************************
    template <typename T>
    inline std::istream& operator>>(std::istream& s, Vec2<T>& v)
    {
        s >> v.x;
        s >> v.y;
        return s;
    }

    //******************************************************************************
    template <> inline std::istream& operator>>(std::istream& s, Vec2<char>& v)
    {
        int vx, vy;
        s >> vx;
        s >> vy;
        v.x = vx;
        v.y = vy;
        return s;
    }

    //******************************************************************************
    template <>
    inline std::istream& operator>>(std::istream& s, Vec2<unsigned char>& v)
    {
        int vx, vy;
        s >> vx;
        s >> vy;
        v.x = vx;
        v.y = vy;
        return s;
    }

    //******************************************************************************
    template <typename T>
    inline std::ostream& operator<<(std::ostream& s, const Vec2<T>& v)
    {
        s << "(" << v.x << ", " << v.y << ")";
        return s;
    }

    //******************************************************************************
    template <>
    inline std::ostream& operator<<(std::ostream& s, const Vec2<char>& v)
    {
        s << "(" << (int)v.x << ", " << (int)v.y << ")";
        return s;
    }

    //******************************************************************************
    template <>
    inline std::ostream& operator<<(std::ostream& s,
                                    const Vec2<unsigned char>& v)
    {
        s << "(" << (int)v.x << ", " << (int)v.y << ")";
        return s;
    }

    //******************************************************************************
    // VEC 3
    //******************************************************************************
    template <typename T>
    inline std::istream& operator>>(std::istream& s, Vec3<T>& v)
    {
        s >> v.x;
        s >> v.y;
        s >> v.z;
        return s;
    }

    //******************************************************************************
    template <> inline std::istream& operator>>(std::istream& s, Vec3<char>& v)
    {
        int vx, vy, vz;
        s >> vx;
        s >> vy;
        s >> vz;
        v.x = vx;
        v.y = vy;
        v.z = vz;
        return s;
    }

    //******************************************************************************
    template <>
    inline std::istream& operator>>(std::istream& s, Vec3<unsigned char>& v)
    {
        int vx, vy, vz;
        s >> vx;
        s >> vy;
        s >> vy;
        v.x = vx;
        v.y = vy;
        v.z = vz;
        return s;
    }

    //******************************************************************************
    template <typename T>
    inline std::ostream& operator<<(std::ostream& s, const Vec3<T>& v)
    {
        s << "(" << v.x << ", " << v.y << ", " << v.z << ")";
        return s;
    }

    //******************************************************************************
    template <>
    inline std::ostream& operator<<(std::ostream& s, const Vec3<char>& v)
    {
        s << "(" << (int)v.x << ", " << (int)v.y << ", " << (int)v.z << ")";
        return s;
    }

    //******************************************************************************
    template <>
    inline std::ostream& operator<<(std::ostream& s,
                                    const Vec3<unsigned char>& v)
    {
        s << "(" << (int)v.x << ", " << (int)v.y << ", " << (int)v.z << ")";
        return s;
    }

    //******************************************************************************
    // VEC 4
    //******************************************************************************
    template <typename T>
    inline std::istream& operator>>(std::istream& s, Vec4<T>& v)
    {
        s >> v.x;
        s >> v.y;
        s >> v.z;
        s >> v.w;
        return s;
    }

    //******************************************************************************
    template <> inline std::istream& operator>>(std::istream& s, Vec4<char>& v)
    {
        int vx, vy, vz, vw;
        s >> vx;
        s >> vy;
        s >> vz;
        s >> vw;
        v.x = vx;
        v.y = vy;
        v.z = vz;
        v.w = vw;
        return s;
    }

    //******************************************************************************
    template <>
    inline std::istream& operator>>(std::istream& s, Vec4<unsigned char>& v)
    {
        int vx, vy, vz, vw;
        s >> vx;
        s >> vy;
        s >> vy;
        s >> vw;
        v.x = vx;
        v.y = vy;
        v.z = vz;
        v.w = vw;
        return s;
    }

    //******************************************************************************
    template <typename T>
    inline std::ostream& operator<<(std::ostream& s, const Vec4<T>& v)
    {
        s << "(" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ")";
        return s;
    }

    //******************************************************************************
    template <>
    inline std::ostream& operator<<(std::ostream& s, const Vec4<char>& v)
    {
        s << "(" << (int)v.x << ", " << (int)v.y << ", " << (int)v.z << ", "
          << (int)v.w << ")";
        return s;
    }

    //******************************************************************************
    template <>
    inline std::ostream& operator<<(std::ostream& s,
                                    const Vec4<unsigned char>& v)
    {
        s << "(" << (int)v.x << ", " << (int)v.y << ", " << (int)v.z << ", "
          << (int)v.w << ")";
        return s;
    }

    //******************************************************************************
    // MATRIX 2x2
    //******************************************************************************
    template <typename T>
    inline std::istream& operator>>(std::istream& s, const Mat22<T>& m)
    {
        s >> m.m00;
        s >> m.m01;
        s >> m.m10;
        s >> m.m11;
        return s;
    }

    //******************************************************************************
    template <typename T>
    inline std::ostream& operator<<(std::ostream& s, const Mat22<T>& m)
    {
        s << m.m00 << "\t" << m.m01 << std::endl
          << m.m10 << "\t" << m.m11 << std::endl;
        return s;
    }

    //******************************************************************************
    // MATRIX 3x3
    //******************************************************************************
    template <typename T>
    inline std::istream& operator>>(std::istream& s, const Mat33<T>& m)
    {
        s >> m.m00;
        s >> m.m01;
        s >> m.m02;
        s >> m.m10;
        s >> m.m11;
        s >> m.m12;
        s >> m.m20;
        s >> m.m21;
        s >> m.m22;
        return s;
    }

    //******************************************************************************
    template <typename T>
    inline std::ostream& operator<<(std::ostream& s, const Mat33<T>& m)
    {
        s << m.m00 << "\t" << m.m01 << "\t" << m.m02 << std::endl
          << m.m10 << "\t" << m.m11 << "\t" << m.m12 << std::endl
          << m.m20 << "\t" << m.m21 << "\t" << m.m22 << std::endl;
        return s;
    }

    //******************************************************************************
    // MATRIX 4x4
    //******************************************************************************
    template <typename T>
    inline std::istream& operator>>(std::istream& s, const Mat44<T>& m)
    {
        s >> m.m00;
        s >> m.m01;
        s >> m.m02;
        s >> m.m03;
        s >> m.m10;
        s >> m.m11;
        s >> m.m12;
        s >> m.m13;
        s >> m.m20;
        s >> m.m21;
        s >> m.m22;
        s >> m.m23;
        s >> m.m30;
        s >> m.m31;
        s >> m.m32;
        s >> m.m33;
        return s;
    }

    //******************************************************************************
    template <typename T>
    inline std::ostream& operator<<(std::ostream& s, const Mat44<T>& m)
    {
        s << m.m00 << "\t" << m.m01 << "\t" << m.m02 << "\t" << m.m03
          << std::endl
          << m.m10 << "\t" << m.m11 << "\t" << m.m12 << "\t" << m.m13
          << std::endl
          << m.m20 << "\t" << m.m21 << "\t" << m.m22 << "\t" << m.m23
          << std::endl
          << m.m30 << "\t" << m.m31 << "\t" << m.m32 << "\t" << m.m33
          << std::endl;
        return s;
    }

    //******************************************************************************
    // INTERVAL
    //******************************************************************************
    template <typename T>
    inline std::ostream& operator<<(std::ostream& s, const Interval<T>& ivl)
    {
        s << "(" << ivl.min << ", " << ivl.max << ")";
        return s;
    }

    //******************************************************************************
    template <typename T>
    inline std::ostream& operator<<(std::ostream& s, const Quat<T>& q)
    {
        s << "(" << q.s << ", " << q.v.x << ", " << q.v.y << ", " << q.v.z
          << ")";
        return s;
    }

    //******************************************************************************
    template <typename T>
    inline std::ostream& operator<<(std::ostream& s, const Line<T>& p)
    {
        s << "(" << p.origin << " " << p.direction << "->)";
        return s;
    }

    //******************************************************************************
    template <typename T>
    inline std::ostream& operator<<(std::ostream& s, const Plane<T>& p)
    {
        s << "(" << p.normal.x << "A, " << p.normal.y << "B, " << p.normal.z
          << "C, " << p.distance << "D)";
        return s;
    }

    template <typename T>
    inline std::ostream& operator<<(std::ostream& o, const Box<T>& b)
    {
        o << b.min << " - " << b.max;
        return o;
    }

} // End namespace TwkMath

#endif
