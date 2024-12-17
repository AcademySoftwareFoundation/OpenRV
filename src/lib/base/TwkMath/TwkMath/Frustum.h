//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkMath__TwkMathFrustum__h__
#define __TwkMath__TwkMathFrustum__h__
#include <TwkMath/Math.h>
#include <TwkMath/Vec2.h>
#include <TwkMath/Vec3.h>
#include <TwkMath/Mat44.h>
#include <TwkMath/Exc.h>
#include <algorithm>
#include <limits>

namespace TwkMath
{

    //
    //  class Frustum
    //
    //  The Frustum object represents a geometric interpretation of a 4x4
    //  projection matrix. The Frustum can be converted into a projection
    //  matrix in which case the projection will be "facing down -Z".
    //
    //  The API is meant to reminiscent of IRIS GL's methods for setting
    //  projections.
    //

    template <typename T> class Frustum
    {
    public:
        //
        //  Types
        //

        typedef T value_type;
        typedef Vec2<T> vec2;
        typedef Vec3<T> vec3;
        typedef Mat44<T> matrix_type;

        //
        //  Constructors
        //

        Frustum() { window(-1, 1, -1, 1, .1, 1000.0); }

        Frustum(value_type left, value_type right, value_type bottom,
                value_type top, value_type nearPlane, value_type farPlane,
                bool ortho = false);

        Frustum(value_type fovy, value_type aspect, value_type nearPlane,
                value_type farPlane);

        //
        //  Member access
        //

        // ajg - dude */
        value_type nearPlane() const { return m_near; }

        value_type farPlane() const { return m_far; }

        value_type left() const { return m_left; }

        value_type right() const { return m_right; }

        value_type bottom() const { return m_bottom; }

        value_type top() const { return m_top; }

        bool ortho() const { return m_ortho; }

        //
        //  Dervied values
        //
        //  matrix() -- returns a projection matrix.
        //

        value_type fovy() const;
        value_type fovx() const;
        value_type rmanFov() const; // as rib expects (but in radians)
        value_type aspect() const;
        matrix_type matrix() const;

        //
        //  Projections
        //
        //  window -- set the projection using all the available window
        //  parameters. The window parameters (other than far) are
        //  relative to the near plane.
        //

        void window(value_type left, value_type right, value_type bottom,
                    value_type top, value_type nearPlane, value_type farPlane,
                    bool ortho = false);

        //
        //  perspective -- just like the GL call. Field of view is in
        //  "y". Fov is in radians.
        //

        void perspective(value_type fovyRadians, value_type aspect,
                         value_type nearPlane, value_type farPlane);

        //
        //  RIBPerspective -- just like a RenderMan RIB projection include
        //  the weird FOV thing they do.
        //

        void RIBperspective(value_type RIBfovRadians, value_type aspect,
                            value_type nearPlane, value_type farPlane);

        //
        //  Modifying existing frustums.
        //
        //  crop -- creates a new frustum with the specified window
        //  parameters. The passed in crop parameters are in normalized
        //  device coordinates relative to the existing Frustum. [0.0, 1.0]
        //

        Frustum<T> crop(value_type left, value_type right, value_type bottom,
                        value_type top) const;

        //
        //  Set values (while keeping the frustum geometry the same). Some
        //  functions will modify other parts of the frustum in order to keep
        //  the frustum geometry consistant.
        //

        // AJG - near...  far!
        void nearPlane(value_type newNear);

        void farPlane(value_type newFar) { m_far = newFar; }

        void ortho(bool b) { m_ortho = b; }

        void adjustAspectHorizontally(value_type newAspect);

        //
        //  Value conversion
        //

        vec2 fromScreen(const vec2&);
        vec2 toScreen(const vec2&);

    private:
        bool sanityCheck();

    private:
        value_type m_left;
        value_type m_right;
        value_type m_bottom;
        value_type m_top;
        value_type m_near;
        value_type m_far;
        bool m_ortho;
    };

    template <typename T>
    inline Frustum<T>::Frustum(T left, T right, T bottom, T top, T nearPlane,
                               T farPlane, bool ortho)
    {
        window(left, right, bottom, top, nearPlane, farPlane, ortho);
    }

    template <typename T>
    inline Frustum<T>::Frustum(T fovy, T aspect, T nearPlane, T farPlane)
    {
        perspective(fovy, aspect, nearPlane, farPlane);
    }

    /* ajg - nearplane, farplane */
    template <typename T>
    inline void Frustum<T>::window(T left, T right, T bottom, T top,
                                   T nearPlane, T farPlane, bool ortho)
    {
        m_left = left;
        m_right = right;
        m_bottom = bottom;
        m_top = top;
        m_near = nearPlane;
        m_far = farPlane;
        m_ortho = ortho;

        if (!sanityCheck())
        {
            throw BadFrustumExc();
        }
    }

    template <typename T>
    inline void Frustum<T>::perspective(T fovy, T aspect, T nearPlane,
                                        T farPlane)
    {
        m_top = nearPlane * Math<T>::tan(fovy / T(2));
        m_bottom = -m_top;
        m_right = (m_top - m_bottom) * aspect / T(2);
        m_left = -m_right;
        m_near = nearPlane;
        m_far = farPlane;
        m_ortho = false;

        if (!sanityCheck())
        {
            throw BadFrustumExc();
        }
    }

    template <typename T>
    inline void Frustum<T>::RIBperspective(T fov, T aspect, T nearPlane,
                                           T farPlane)
    {
        perspective(fov, aspect, nearPlane, farPlane);

        if (aspect < 1.0)
        {
            std::swap(m_top, m_right);
            std::swap(m_bottom, m_left);
        }
    }

    template <typename T> bool Frustum<T>::sanityCheck()
    {
        T w = m_right - m_left;
        T h = m_top - m_bottom;
        T d = m_far - m_near;

#if 0
    return  Math<T>::abs(w) > std::limits<T>::epsilon() &&
            Math<T>::abs(h) > std::limits<T>::epsilon() &&
            Math<T>::abs(d) > std::limits<T>::epsilon();
#endif
        return true;
    }

    template <typename T> inline T Frustum<T>::fovy() const
    {
        return Math<T>::atan2(m_top, m_near) - Math<T>::atan2(m_bottom, m_near);
    }

    template <typename T> inline T Frustum<T>::fovx() const
    {
        return Math<T>::atan2(m_right, m_near) - Math<T>::atan2(m_left, m_near);
    }

    template <typename T> inline T Frustum<T>::rmanFov() const
    {
        return (aspect() > 1.0) ? fovy() : fovx();
    }

    template <typename T> T Frustum<T>::aspect() const
    {
        T w = m_right - m_left;
        T h = m_top - m_bottom;

        return w != T(0) ? (w / h) : 0;
    }

    template <typename T> void Frustum<T>::adjustAspectHorizontally(T newAspect)
    {
        // T w = m_right - m_left;
        // float a = (((newAspect * w) / aspect()) - w) / 2.0;
        // m_right += a;
        // m_left  -= a;

        float f = fovy();
        perspective(fovy(), newAspect, m_near, m_far);
    }

    template <typename T> Vec2<T> Frustum<T>::fromScreen(const Vec2<T>& v)
    {
        const float w = m_right - m_left;
        const float h = m_top - m_bottom;

        return vec2(w * (T(1) + v.x) / T(2) + m_left,
                    h * (T(1) + v.y) / T(2) + m_right);
    }

    template <typename T> Vec2<T> Frustum<T>::toScreen(const Vec2<T>& v)
    {
        // Assumes center of projection in the center of the 'image'
        const float copX = (m_right - m_left) / 2.0f;
        const float copY = (m_top - m_bottom) / 2.0f;

        return vec2((v.x - copX) / copX, (v.y - copY) / copY);
    }

    template <typename T>
    Frustum<T> Frustum<T>::crop(T left, T right, T bottom, T top) const
    {
        T w = m_right - m_left;
        T h = m_top - m_bottom;
        left = w * left + m_left;
        right = w * right + m_left;
        bottom = h * bottom + m_bottom;
        top = h * top + m_bottom;
        return Frustum<T>(left, right, bottom, top, m_near, m_far, m_ortho);
    }

    /* ajg - nearPlane damnit! */
    template <typename T> void Frustum<T>::nearPlane(T newNear)
    {
        if (m_ortho)
        {
            m_near = newNear;
        }
        else
        {
            T u = newNear / m_near;

            window(m_left * u, m_right * u, m_bottom * u, m_top * u, newNear,
                   m_far, false);
        }
    }

    template <typename T> Mat44<T> Frustum<T>::matrix() const
    {
        const T rml = m_right - m_left;
        const T rpl = m_right + m_left;
        const T tmb = m_top - m_bottom;
        const T tpb = m_top + m_bottom;
        const T fpn = m_far + m_near;
        const T fmn = m_far - m_near;

        if (m_ortho)
        {
            const T A = T(2) / rml;
            const T B = T(2) / tmb;
            const T C = T(-2) / fmn;

            return matrix_type(A, 0, 0, -rpl / rml, 0, B, 0, -tpb / tmb, 0, 0,
                               C, -fpn / fmn, 0, 0, 0, T(1));
        }
        else
        {
            const T n2 = m_near * T(2);

            return matrix_type(n2 / rml, 0, rpl / rml, 0, 0, n2 / tmb,
                               tpb / tmb, 0, 0, 0, -fpn / fmn,
                               (-n2 * m_far) / fmn, 0, 0, T(-1), 0);
        }
    }

    typedef Frustum<float> Frustumf;
    typedef Frustum<double> Frustumd;

} // namespace TwkMath

#endif // __TwkMath__TwkMathFrustum__h__
