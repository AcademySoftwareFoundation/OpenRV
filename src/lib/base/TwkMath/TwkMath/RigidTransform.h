//******************************************************************************
// Copyright (c) 2003 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkMath__TwkMathRigidTransform__h__
#define __TwkMath__TwkMathRigidTransform__h__
#include <TwkMath/Quat.h>
#include <TwkMath/Mat44.h>
#include <TwkMath/Mat33.h>
#include <TwkMath/Vec3.h>

namespace TwkMath
{

    //
    //  template class RigidTransform
    //
    //  This transform is a sub-set of all 4x4 linear transformations that
    //  only incorporates scale translation and rotation. The scales can
    //  be non-uniform (so its not *really* a rigid transform). Shear and
    //  projection is not allowed. The class stores the parameters in the
    //  form of two vectors and a quaternion. The transform is always
    //  composed by multiplying like this:
    //
    //      T * R * S
    //
    //  if T, R, and S are translation, rotation, and scale matrices
    //  respectively.
    //
    //  This class is most useful in the context of interpolating
    //  matrices. Since we're storing the parameters only, the size is
    //  smaller than a 4x4, and interpolation is done using interpolation
    //  of the parameters.
    //
    //  There's a lerp() function at the end of the file.
    //

    template <typename T> class RigidTransform
    {
    public:
        //
        //  Types
        //

        typedef T value_type;
        typedef Vec3<T> V;
        typedef Quat<T> Q;
        typedef Mat44<T> Mat;
        typedef RigidTransform<T> ThisType;

        //
        //  Public Members
        //

        V translation;
        Q orientation;
        V scale;

        //
        //  Constructors
        //

        RigidTransform()
            : translation(T(0))
            , orientation()
            , scale(T(1))
        {
        }

        RigidTransform(const Mat&);

        RigidTransform(const V& t, const Q& r, const V& s)
            : translation(t)
            , orientation(r)
            , scale(s)
        {
        }

        void set(const Mat&);

        //
        //  Casts
        //

        operator Mat() const;

        void operator=(const Mat44<T>& M) { set(M); }
    };

    template <typename T> RigidTransform<T>::RigidTransform(const Mat44<T>& M)
    {
        set(M);
    }

    template <typename T> void RigidTransform<T>::set(const Mat44<T>& M)
    {
        //
        //  Remove the translation
        //

        translation = Vec3<T>(M(0, 3), M(1, 3), M(2, 3));

        //
        //  Get the scales by pulling them out of the columns.
        //

        Vec3<T> i = Vec3<T>(M(0, 0), M(1, 0), M(2, 0));
        Vec3<T> j = Vec3<T>(M(0, 1), M(1, 1), M(2, 1));
        Vec3<T> k = Vec3<T>(M(0, 2), M(1, 2), M(2, 2));

        const T si = magnitude(i);
        const T sj = magnitude(j);
        const T sk = magnitude(k);

        scale.x = si;
        scale.y = sj;
        scale.z = sk;

        //
        //  Have the quat class to decompose the rotation matrix. We don't
        //  need to check for any special conditions this way.
        //

        if (si != T(0))
            i /= si;
        if (sj != T(0))
            j /= sj;
        if (sk != T(0))
            k /= sk;

        orientation = Mat33<T>(i.x, j.x, k.x, i.y, j.y, k.y, i.z, j.z, k.z);
    }

    template <typename T> inline RigidTransform<T>::operator Mat44<T>() const
    {
        //
        //  This code is duplicated from Quat for efficiency (the
        //  translation and scale are computed at the same time instead of
        //  copying data around).
        //

        V v = orientation.v;
        T s = orientation.s;

        const float xx = v.x * v.x;
        const float yy = v.y * v.y;
        const float zz = v.z * v.z;
        const float xy = v.x * v.y;
        const float xz = v.x * v.z;
        const float yz = v.y * v.z;
        const float sx = v.x * s;
        const float sy = v.y * s;
        const float sz = v.z * s;

        const float a = T(1) - T(2) * (yy + zz);
        const float b = T(2) * (xy + sz);
        const float c = T(2) * (xz - sy);
        const float d = T(2) * (xy - sz);
        const float e = T(1) - T(2) * (zz + xx);
        const float f = T(2) * (yz + sx);
        const float g = T(2) * (xz + sy);
        const float h = T(2) * (yz - sx);
        const float i = T(1) - T(2) * (yy + xx);

        return Mat44<T>(a * scale.x, d * scale.y, g * scale.z, translation.x,
                        b * scale.x, e * scale.y, h * scale.z, translation.y,
                        c * scale.x, f * scale.y, i * scale.z, translation.z,
                        T(0), T(0), T(0), T(1));
    }

    template <typename T, typename InterpT>
    inline RigidTransform<T> lerp(const RigidTransform<T>& a,
                                  const RigidTransform<T>& b, const InterpT u)
    {
        const InterpT u0 = InterpT(1) - u;
        return RigidTransform<T>(a.translation * u0 + b.translation * u,
                                 slerp(a.orientation, b.orientation, u),
                                 a.scale * u0 + b.scale * u);
    }

    template <typename T>
    inline RigidTransform<T> inverse(const RigidTransform<T>& r)
    {
        return RigidTransform<T>(-r.translation, ~r.orientation,
                                 1.0f / r.scale);
    }

    template <typename T>
    inline Vec3<T> operator*(const RigidTransform<T>& r, const Vec3<T>& v)
    {
        return r.orientation * (r.scale * v) + r.translation;
    }

    typedef RigidTransform<float> RigidTransformf;
    typedef RigidTransform<double> RigidTransformd;

} // namespace TwkMath

#endif // __TwkMath__TwkMathRigidTransform__h__
