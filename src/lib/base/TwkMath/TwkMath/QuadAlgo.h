//-*****************************************************************************
// Copyright (c) 2001-2004 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//-*****************************************************************************

#ifndef _TwkMath_TwkMathQuadAlgo_h_
#define _TwkMath_TwkMathQuadAlgo_h_

#include <TwkMath/Vec2.h>
#include <TwkMath/Vec3.h>
#include <TwkMath/Function.h>
#include <TwkMath/LineAlgo.h>
#include <TwkMath/Mat44.h>
#include <TwkMath/Box.h>
#include <vector>

//-*****************************************************************************
// Handy.
#define T0 ((T)0)
#define T1 ((T)1)
#define T2 ((T)2)
#define T3 ((T)3)
#define T4 ((T)4)

namespace TwkMath
{

    //-*****************************************************************************
    //-*****************************************************************************
    // HANDY UTILITY (use for simple bilinear interpolation of quad)
    //-*****************************************************************************
    //-*****************************************************************************
    template <class VEC, class VEC2>
    inline VEC quadMap(const VEC& p00, const VEC& p10, const VEC& p11,
                       const VEC& p01, VEC2& uv)
    {
        return lerp(lerp(p00, p10, uv[0]), lerp(p01, p11, uv[0]), uv[1]);
    }

    //-*****************************************************************************
    //-*****************************************************************************
    // STRUCT FOR BOOL AND VEC2 PAIR
    //-*****************************************************************************
    //-*****************************************************************************
    template <typename T> struct BoolAndVec2
    {
        BoolAndVec2()
            : status(false)
            , uv(((T)0))
        {
        }

        BoolAndVec2(bool tf)
            : status(tf)
            , uv(((T)0))
        {
        }

        BoolAndVec2(bool tf, const Vec2<T>& v2)
            : status(tf)
            , uv(v2)
        {
        }

        bool status;
        Vec2<T> uv;
    };

    //-*****************************************************************************
    //-*****************************************************************************
    // CLASS TO USE FOR INVERTING A UV->ST QUAD
    //-*****************************************************************************
    //-*****************************************************************************
    template <typename T> class UvStQuadInverter
    {
    public:
        UvStQuadInverter(const Vec2<T>& p00, const Vec2<T>& p10,
                         const Vec2<T>& p11, const Vec2<T>& p01);

        Vec2<T> map(const Vec2<T>& uv) const;

        BoolAndVec2<T> inverseMap(const Vec2<T>& st) const;

        Vec2<T> closestInverseMap(const Vec2<T>& st) const;

        // This version assumes that the sts and uvs
        // vectors are both the same size.
        void inverseMap(const std::vector<Vec2<T>>& sts,
                        std::vector<BoolAndVec2<T>>& uvs) const;

        void closestInverseMap(const std::vector<Vec2<T>>& sts,
                               std::vector<Vec2<T>>& uvs) const;

    protected:
        // The uv -> st equation
        // s = Sk[0] + Sk[1] * u + Sk[2] * v + Sk[3] * uv;
        // t = Tk[0] + Tk[1] * u + Tk[2] * v + Tk[3] * uv;
        T m_Sk[4];
        T m_Tk[4];

        // Bounds in 2d.
        Box<Vec2<T>> m_bounds;

        // Stuff used for inverse mapping.
        Vec2<T> m_p00;
        Vec2<T> m_p10;
        Vec2<T> m_p11;
        Vec2<T> m_p01;

        Vec2<T> m_pa;
        Vec2<T> m_pb;
        Vec2<T> m_pc;
        // pd == m_p00

        T m_Du0;
        T m_Du1;
        T m_Du2;

        T m_Dv0;
        T m_Dv1;
        T m_Dv2;
    };

    //-*****************************************************************************
    //-*****************************************************************************
    // FUNCTIONS FOR MAKING A 3D INVERSION
    //-*****************************************************************************
    //-*****************************************************************************
    template <typename T>
    BoolAndVec2<T> quadInverseMap(const Vec2<T>& p00, const Vec2<T>& p10,
                                  const Vec2<T>& p11, const Vec2<T>& p01,
                                  const Vec2<T>& p);

    //-*****************************************************************************
    template <typename T>
    void quadInverseMap(const Vec2<T>& p00, const Vec2<T>& p10,
                        const Vec2<T>& p11, const Vec2<T>& p01,
                        const std::vector<Vec2<T>>& p,
                        std::vector<BoolAndVec2<T>>& uvs);

    //-*****************************************************************************
    template <typename T>
    BoolAndVec2<T> quadInverseMap(const Vec3<T>& p00, const Vec3<T>& p10,
                                  const Vec3<T>& p11, const Vec3<T>& p01,
                                  const Vec3<T>& p);

    //-*****************************************************************************
    template <typename T>
    void quadInverseMap(const Vec3<T>& p00, const Vec3<T>& p10,
                        const Vec3<T>& p11, const Vec3<T>& p01,
                        const std::vector<Vec3<T>>& p,
                        std::vector<BoolAndVec2<T>>& uvs);

    //-*****************************************************************************
    //-*****************************************************************************
    //-*****************************************************************************
    template <typename T>
    Vec2<T> quadClosestInverseMap(const Vec2<T>& p00, const Vec2<T>& p10,
                                  const Vec2<T>& p11, const Vec2<T>& p01,
                                  const Vec2<T>& p);

    //-*****************************************************************************
    template <typename T>
    void quadClosestInverseMap(const Vec2<T>& p00, const Vec2<T>& p10,
                               const Vec2<T>& p11, const Vec2<T>& p01,
                               const std::vector<Vec2<T>>& p,
                               std::vector<Vec2<T>>& uvs);

    //-*****************************************************************************
    template <typename T>
    Vec2<T> quadClosestInverseMap(const Vec3<T>& p00, const Vec3<T>& p10,
                                  const Vec3<T>& p11, const Vec3<T>& p01,
                                  const Vec3<T>& p);

    //-*****************************************************************************
    template <typename T>
    void quadClosestInverseMap(const Vec3<T>& p00, const Vec3<T>& p10,
                               const Vec3<T>& p11, const Vec3<T>& p01,
                               const std::vector<Vec3<T>>& p,
                               std::vector<Vec2<T>>& uvs);

    //-*****************************************************************************
    //-*****************************************************************************
    // HANDY TYPEDEFS
    //-*****************************************************************************
    //-*****************************************************************************
    typedef BoolAndVec2<float> BoolAndVec2f;
    typedef BoolAndVec2<double> BoolAndVec2d;

    typedef UvStQuadInverter<float> UvStQuadInverterf;
    typedef UvStQuadInverter<double> UvStQuadInverterd;

    //-*****************************************************************************
    //-*****************************************************************************
    // TEMPLATE AND INLINE FUNCTIONS
    //-*****************************************************************************
    //-*****************************************************************************

    //-*****************************************************************************
    template <typename T>
    inline Vec2<T> UvStQuadInverter<T>::map(const Vec2<T>& uv) const
    {
        const T uXv = uv.x * uv.y;
        return Vec2<T>(
            m_Sk[0] + (m_Sk[1] * uv.x) + (m_Sk[2] * uv.y) + (m_Sk[3] * uXv),
            m_Tk[0] + (m_Tk[1] * uv.x) + (m_Tk[2] * uv.y) + (m_Tk[3] * uXv));
    }

    //-*****************************************************************************
    template <typename T>
    UvStQuadInverter<T>::UvStQuadInverter(const Vec2<T>& p00,
                                          const Vec2<T>& p10,
                                          const Vec2<T>& p11,
                                          const Vec2<T>& p01)
    {
        // Figure out the Sk and Tk coefficients correspondingly.
        const T s0 = p00[0];
        const T s1 = p10[0];
        const T s2 = p11[0];
        const T s3 = p01[0];
        m_Sk[0] = s0;
        m_Sk[1] = s1 - s0;
        m_Sk[2] = s3 - s0;
        m_Sk[3] = (s2 - s3) - (s1 - s0);

        const T t0 = p00[1];
        const T t1 = p10[1];
        const T t2 = p11[1];
        const T t3 = p01[1];
        m_Tk[0] = t0;
        m_Tk[1] = t1 - t0;
        m_Tk[2] = t3 - t0;
        m_Tk[3] = (t2 - t3) - (t1 - t0);

        // Compute the bounds
        m_bounds.makeEmpty();
        m_bounds.min.x = std::min(std::min(s0, s1), std::min(s2, s3));
        m_bounds.max.x = std::max(std::max(s0, s1), std::max(s2, s3));
        m_bounds.min.y = std::min(std::min(t0, t1), std::min(t2, t3));
        m_bounds.max.y = std::max(std::max(t0, t1), std::max(t2, t3));

        static Vec2<T> v00(T0, T0);
        static Vec2<T> v10(T1, T0);
        static Vec2<T> v11(T1, T1);
        static Vec2<T> v01(T0, T1);

        m_p00 = Vec2<T>(map(v00));
        m_p10 = Vec2<T>(map(v10));
        m_p11 = Vec2<T>(map(v11));
        m_p01 = Vec2<T>(map(v01));

        m_pa = m_p00 - m_p10 + m_p11 - m_p01;
        m_pb = m_p10 - m_p00;
        m_pc = m_p01 - m_p00;

        Vec2<T> Na(m_pa.y, -m_pa.x);
        Vec2<T> Nb(m_pb.y, -m_pb.x);
        Vec2<T> Nc(m_pc.y, -m_pc.x);

        m_Du0 = dot(Nc, m_p00);
        m_Du1 = dot(Na, m_p00) + dot(Nc, m_pb);
        m_Du2 = dot(Na, m_pb);

        m_Dv0 = dot(Nb, m_p00);
        m_Dv1 = dot(Na, m_p00) + dot(Nb, m_pc);
        m_Dv2 = dot(Na, m_pc);
    }

    //******************************************************************************
    template <typename T>
    BoolAndVec2<T> UvStQuadInverter<T>::inverseMap(const Vec2<T>& st) const
    {
        if (m_bounds.intersects(st))
        {
            // Is the point inside this UvQuad?
            size_t NC = 0;
            NC += (size_t)testPointVsSegment(m_p00, m_p10, st);
            NC += (size_t)testPointVsSegment(m_p10, m_p11, st);
            NC += (size_t)testPointVsSegment(m_p11, m_p01, st);
            NC += (size_t)testPointVsSegment(m_p01, m_p00, st);
            if (NC == 0 || NC == 2 || NC == 4)
            {
                return BoolAndVec2<T>(false);
            }

            // Don't cache these.
            Vec2<T> Na(m_pa.y, -m_pa.x);
            Vec2<T> Nb(m_pb.y, -m_pb.x);
            Vec2<T> Nc(m_pc.y, -m_pc.x);

            // Dont cache these.
            bool parallelU = bool(Math<T>::abs(m_Du2) < Math<T>::epsilon());
            bool parallelV = bool(Math<T>::abs(m_Dv2) < Math<T>::epsilon());

            // Project
            T Bu = m_Du1 - dot(st, Na);
            T Cu = m_Du0 - dot(st, Nc);
            T u = -T1;

            if (parallelU)
            {
                if (Math<T>::abs(Bu) < Math<T>::epsilon())
                {
                    // Default answer
                    u = -T1;
                }
                else
                {
                    u = -Cu / Bu;
                }
            }
            else
            {
                T discrim = (Bu * Bu) - (T4 * m_Du2 * Cu);
                if (discrim < T0)
                {
                    // This is a default answer
                    u = -Bu / (T2 * m_Du2);
                }
                else if (discrim == T0)
                {
                    u = -Bu / (T2 * m_Du2);
                }
                else
                {
                    discrim = sqrtf(discrim);
                    u = (-Bu + discrim) / (T2 * m_Du2);
                    if (u < T0 || u > T1)
                    {
                        u = (-Bu - discrim) / (T2 * m_Du2);
                    }
                }
            }

            T Bv = m_Dv1 - dot(st, Na);
            T Cv = m_Dv0 - dot(st, Nb);
            T v = -T1;

            if (parallelV)
            {
                if (Math<T>::abs(Bv) < Math<T>::epsilon())
                {
                    v = T1;
                }
                else
                {
                    v = -Cv / Bv;
                }
            }
            else
            {
                T discrim = (Bv * Bv) - (T4 * m_Dv2 * Cv);
                if (discrim < T0)
                {
                    // Default answer
                    v = -Bv / (T2 * m_Dv2);
                }
                else if (discrim == T0)
                {
                    v = -Bv / (T2 * m_Dv2);
                }
                else
                {
                    discrim = sqrtf(discrim);
                    v = (-Bv + discrim) / (T2 * m_Dv2);
                    if (v < T0 || v > T1)
                    {
                        v = (-Bv - discrim) / (T2 * m_Dv2);
                    }
                }
            }

            // Check bounds of u, v
            return BoolAndVec2<T>(true,
                                  Vec2<T>(clamp(u, T0, T1), clamp(v, T0, T1)));
        }
        else
        {
            return BoolAndVec2<T>(false);
        }
    }

    //******************************************************************************
    template <typename T>
    Vec2<T> UvStQuadInverter<T>::closestInverseMap(const Vec2<T>& st) const
    {
        // Get the closest point in the bounds...
        Vec2<T> in = st;
        m_bounds.closestInteriorPoint(st, in);

        // Don't cache these.
        Vec2<T> Na(m_pa.y, -m_pa.x);
        Vec2<T> Nb(m_pb.y, -m_pb.x);
        Vec2<T> Nc(m_pc.y, -m_pc.x);

        // Dont cache these.
        bool parallelU = bool(Math<T>::abs(m_Du2) < Math<T>::epsilon());
        bool parallelV = bool(Math<T>::abs(m_Dv2) < Math<T>::epsilon());

        // Project
        T Bu = m_Du1 - dot(st, Na);
        T Cu = m_Du0 - dot(st, Nc);
        T u = -T1;

        if (parallelU)
        {
            if (Math<T>::abs(Bu) < Math<T>::epsilon())
            {
                // Default answer
                u = -T1;
            }
            else
            {
                u = -Cu / Bu;
            }
        }
        else
        {
            T discrim = (Bu * Bu) - (T4 * m_Du2 * Cu);
            if (discrim < T0)
            {
                // This is a default answer
                u = -Bu / (T2 * m_Du2);
            }
            else if (discrim == T0)
            {
                u = -Bu / (T2 * m_Du2);
            }
            else
            {
                discrim = sqrtf(discrim);
                u = (-Bu + discrim) / (T2 * m_Du2);
                if (u < T0 || u > T1)
                {
                    u = (-Bu - discrim) / (T2 * m_Du2);
                }
            }
        }

        T Bv = m_Dv1 - dot(st, Na);
        T Cv = m_Dv0 - dot(st, Nb);
        T v = -T1;

        if (parallelV)
        {
            if (Math<T>::abs(Bv) < Math<T>::epsilon())
            {
                v = T1;
            }
            else
            {
                v = -Cv / Bv;
            }
        }
        else
        {
            T discrim = (Bv * Bv) - (T4 * m_Dv2 * Cv);
            if (discrim < T0)
            {
                // Default answer
                v = -Bv / (T2 * m_Dv2);
            }
            else if (discrim == T0)
            {
                v = -Bv / (T2 * m_Dv2);
            }
            else
            {
                discrim = sqrtf(discrim);
                v = (-Bv + discrim) / (T2 * m_Dv2);
                if (v < T0 || v > T1)
                {
                    v = (-Bv - discrim) / (T2 * m_Dv2);
                }
            }
        }

        // Check bounds of u, v
        return Vec2<T>(clamp(u, T0, T1), clamp(v, T0, T1));
    }

    //-*****************************************************************************
    template <typename T>
    inline void
    UvStQuadInverter<T>::inverseMap(const std::vector<Vec2<T>>& sts,
                                    std::vector<BoolAndVec2<T>>& uvs) const
    {
        for (size_t i = 0, s = sts.size(); i < s; ++i)
        {
            uvs[i] = inverseMap(sts[i]);
        }
    }

    //-*****************************************************************************
    template <typename T>
    inline void
    UvStQuadInverter<T>::closestInverseMap(const std::vector<Vec2<T>>& sts,
                                           std::vector<Vec2<T>>& uvs) const
    {
        for (size_t i = 0, s = sts.size(); i < s; ++i)
        {
            uvs[i] = closestInverseMap(sts[i]);
        }
    }

    //-*****************************************************************************
    //-*****************************************************************************
    // HANDY FUNCTIONS (IMPLEMENTATION)
    //-*****************************************************************************
    //-*****************************************************************************
    template <typename T>
    inline BoolAndVec2<T> quadInverseMap(const Vec2<T>& p00, const Vec2<T>& p10,
                                         const Vec2<T>& p11, const Vec2<T>& p01,
                                         const Vec2<T>& p)
    {
        UvStQuadInverter<T> uvqi(p00, p10, p11, p01);
        return uvqi.inverseMap(p);
    }

    //-*****************************************************************************
    template <typename T>
    inline void quadInverseMap(const Vec2<T>& p00, const Vec2<T>& p10,
                               const Vec2<T>& p11, const Vec2<T>& p01,
                               const std::vector<Vec2<T>>& p,
                               std::vector<BoolAndVec2<T>>& uvs)
    {
        UvStQuadInverter<T> uvqi(p00, p10, p11, p01);
        uvqi.inverseMap(p, uvs);
    }

    //-*****************************************************************************
    template <typename T>
    void quadInverseMap(const Vec3<T>& p00, const Vec3<T>& p10,
                        const Vec3<T>& p11, const Vec3<T>& p01,
                        const std::vector<Vec3<T>>& p,
                        std::vector<BoolAndVec2<T>>& uvs)
    {
        Vec3<T> v0 = p10 - p00;
        v0.normalize();
        Vec3<T> v1 = p01 - p00;
        Vec3<T> n = cross(v0, v1);
        n.normalize();
        v1 = cross(n, v0);
        v1.normalize();

        Mat44<T> worldToPlane(v0[0], v0[1], v0[2], -dot(v0, p00), v1[0], v1[1],
                              v1[2], -dot(v1, p00), n[0], n[1], n[2],
                              -dot(n, p00), T0, T0, T0, T1);

        Vec3<T> proj;
        proj = worldToPlane.transform(p00);
        Vec2<T> st00(proj[0], proj[1]);
        proj = worldToPlane.transform(p10);
        Vec2<T> st10(proj[0], proj[1]);
        proj = worldToPlane.transform(p11);
        Vec2<T> st11(proj[0], proj[1]);
        proj = worldToPlane.transform(p01);
        Vec2<T> st01(proj[0], proj[1]);

        UvStQuadInverter<T> uvqi(st00, st10, st11, st01);

        std::vector<Vec2<T>> sts(p.size());
        for (size_t i = 0, s = sts.size(); i < s; ++i)
        {
            Vec3<T> proj = worldToPlane.transform(p[i]);
            sts[i].set(proj[0], proj[1]);
        }

        uvqi.inverseMap(sts, uvs);
    }

    //-*****************************************************************************
    template <typename T>
    inline BoolAndVec2<T> quadInverseMap(const Vec3<T>& p00, const Vec3<T>& p10,
                                         const Vec3<T>& p11, const Vec3<T>& p01,
                                         const Vec3<T>& p)
    {
        std::vector<Vec3<T>> pvec(1);
        pvec[0] = p;
        std::vector<BoolAndVec2<T>> uvs(1);
        quadInverseMap(p00, p10, p11, p01, pvec, uvs);
        return uvs[0];
    }

    //-*****************************************************************************
    //-*****************************************************************************
    // CLOSEST HANDY FUNCTIONS (IMPLEMENTATION)
    //-*****************************************************************************
    //-*****************************************************************************
    template <typename T>
    inline Vec2<T> quadClosestInverseMap(const Vec2<T>& p00, const Vec2<T>& p10,
                                         const Vec2<T>& p11, const Vec2<T>& p01,
                                         const Vec2<T>& p)
    {
        UvStQuadInverter<T> uvqi(p00, p10, p11, p01);
        return uvqi.closestInverseMap(p);
    }

    //-*****************************************************************************
    template <typename T>
    inline void quadClosestInverseMap(const Vec2<T>& p00, const Vec2<T>& p10,
                                      const Vec2<T>& p11, const Vec2<T>& p01,
                                      const std::vector<Vec2<T>>& p,
                                      std::vector<Vec2<T>>& uvs)
    {
        UvStQuadInverter<T> uvqi(p00, p10, p11, p01);
        uvqi.closestInverseMap(p, uvs);
    }

    //-*****************************************************************************
    template <typename T>
    void quadClosestInverseMap(const Vec3<T>& p00, const Vec3<T>& p10,
                               const Vec3<T>& p11, const Vec3<T>& p01,
                               const std::vector<Vec3<T>>& p,
                               std::vector<Vec2<T>>& uvs)
    {
        Vec3<T> v0 = p10 - p00;
        v0.normalize();
        Vec3<T> v1 = p01 - p00;
        Vec3<T> n = cross(v0, v1);
        n.normalize();
        v1 = cross(n, v0);
        v1.normalize();

        Mat44<T> worldToPlane(v0[0], v0[1], v0[2], -dot(v0, p00), v1[0], v1[1],
                              v1[2], -dot(v1, p00), n[0], n[1], n[2],
                              -dot(n, p00), T0, T0, T0, T1);

        Vec3<T> proj;
        proj = worldToPlane.transform(p00);
        Vec2<T> st00(proj[0], proj[1]);
        proj = worldToPlane.transform(p10);
        Vec2<T> st10(proj[0], proj[1]);
        proj = worldToPlane.transform(p11);
        Vec2<T> st11(proj[0], proj[1]);
        proj = worldToPlane.transform(p01);
        Vec2<T> st01(proj[0], proj[1]);

        UvStQuadInverter<T> uvqi(st00, st10, st11, st01);

        std::vector<Vec2<T>> sts(p.size());
        for (size_t i = 0, s = sts.size(); i < s; ++i)
        {
            Vec3<T> proj = worldToPlane.transform(p[i]);
            sts[i].set(proj[0], proj[1]);
        }

        uvqi.closestInverseMap(sts, uvs);
    }

    //-*****************************************************************************
    template <typename T>
    inline Vec2<T> quadClosestInverseMap(const Vec3<T>& p00, const Vec3<T>& p10,
                                         const Vec3<T>& p11, const Vec3<T>& p01,
                                         const Vec3<T>& p)
    {
        std::vector<Vec3<T>> pvec(1);
        pvec[0] = p;
        std::vector<Vec2<T>> uvs(1);
        quadClosestInverseMap(p00, p10, p11, p01, pvec, uvs);
        return uvs[0];
    }

} // End namespace TwkMath

#undef T0
#undef T1
#undef T2
#undef T3
#undef T4

#endif
