//******************************************************************************
// Copyright (c) 2001-2003 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkMathBspline_h_
#define _TwkMathBspline_h_

namespace TwkMath
{

    //******************************************************************************
    template <typename T>
    inline T quadratic(const T& c0, const T& c1, const T& c2, const T& t)
    {
        return c0 + t * (c1 + t * c2);
    }

    //******************************************************************************
    template <typename T>
    inline T cubic(const T& c0, const T& c1, const T& c2, const T& c3,
                   const T& t)
    {
        return c0 + t * (c1 + t * (c2 + t * c3));
    }

    //******************************************************************************
    template <typename T> inline T bsplineBasis0(const T& t)
    {
        return cubic((T)1, (T)-3, (T)3, (T)-1, t) / (T)6;
    }

    //******************************************************************************
    template <typename T> inline T bsplineBasis1(const T& t)
    {
        return cubic((T)4, (T)0, (T)-6, (T)3, t) / (T)6;
    }

    //******************************************************************************
    template <typename T> inline T bsplineBasis2(const T& t)
    {
        return cubic((T)1, (T)3, (T)3, (T)-3, t) / (T)6;
    }

    //******************************************************************************
    template <typename T> inline T bsplineBasis3(const T& t)
    {
        return (t * t * t) / (T)6;
    }

    //******************************************************************************
    // FIRST DERIVATIVES OF SPLINE FUNCTIONS
    //******************************************************************************
    template <typename T> inline T bsplineBasis0Dt(const T& t)
    {
        return quadratic((T)-3, (T)6, (T)-3, t) / (T)6;
    }

    //******************************************************************************
    template <typename T> inline T bsplineBasis1Dt(const T& t)
    {
        return quadratic((T)0, (T)-12, (T)9, t) / (T)6;
    }

    //******************************************************************************
    template <typename T> inline T bsplineBasis2Dt(const T& t)
    {
        return quadratic((T)3, (T)6, (T)-9, t) / (T)6;
    }

    //******************************************************************************
    template <typename T> inline T bsplineBasis3Dt(const T& t)
    {
        return (t * t) / (T)2;
    }

    //******************************************************************************
    // CURVE FUNCTIONS
    //******************************************************************************
    template <typename CV, typename T>
    inline CV bspline(const CV& v0, const CV& v1, const CV& v2, const CV& v3,
                      const T& t)
    {
        return (v0 * bsplineBasis0(t) + v1 * bsplineBasis1(t)
                + v2 * bsplineBasis2(t) + v3 * bsplineBasis3(t));
    }

    //******************************************************************************
    template <typename CV, typename T>
    inline CV bspline(const CV* v, const T& t)
    {
        return (v[0] * bsplineBasis0(t), v[1] * bsplineBasis1(t),
                v[2] * bsplineBasis2(t), v[3] * bsplineBasis3(t));
    }

    //******************************************************************************
    template <typename CV, typename T>
    inline CV bsplineDt(const CV& v0, const CV& v1, const CV& v2, const CV& v3,
                        const T& t)
    {
        return (v0 * bsplineBasis0Dt(t) + v1 * bsplineBasis1Dt(t)
                + v2 * bsplineBasis2Dt(t) + v3 * bsplineBasis3Dt(t));
    }

    //******************************************************************************
    template <typename CV, typename T>
    inline CV bsplineDt(const CV* v, const T& t)
    {
        return (v[0] * bsplineBasis0Dt(t), v[1] * bsplineBasis1Dt(t),
                v[2] * bsplineBasis2Dt(t), v[3] * bsplineBasis3Dt(t));
    }

    //******************************************************************************
    // SURFACE FUNCTIONS
    //******************************************************************************
    template <typename CV, typename T>
    CV bspline(const CV& v00, const CV& v10, const CV& v20, const CV& v30,
               const CV& v01, const CV& v11, const CV& v21, const CV& v31,
               const CV& v02, const CV& v12, const CV& v22, const CV& v32,
               const CV& v03, const CV& v13, const CV& v23, const CV& v33,
               const T& s, const T& t)
    {
        const T basisS[4] = {bsplineBasis0(s), bsplineBasis1(s),
                             bsplineBasis2(s), bsplineBasis3(s)};

        const T basisT[4] = {bsplineBasis0(t), bsplineBasis1(t),
                             bsplineBasis2(t), bsplineBasis3(t)};

        return (basisT[0]
                * (v00 * basisS[0] + v10 * basisS[1] + v20 * basisS[2]
                   + v30 * basisS[3]))
               +

               (basisT[1]
                * (v01 * basisS[0] + v11 * basisS[1] + v21 * basisS[2]
                   + v31 * basisS[3]))
               +

               (basisT[2]
                * (v02 * basisS[0] + v12 * basisS[1] + v22 * basisS[2]
                   + v32 * basisS[3]))
               +

               (basisT[3]
                * (v03 * basisS[0] + v13 * basisS[1] + v23 * basisS[2]
                   + v33 * basisS[3]));
    }

    //******************************************************************************
    // ARRAY PASSED IN AS T-MAJOR!!!
    template <typename CV, typename T>
    CV bspline(const CV* v, const T& s, const T& t)
    {
        const T basisS[4] = {bsplineBasis0(s), bsplineBasis1(s),
                             bsplineBasis2(s), bsplineBasis3(s)};

        const T basisT[4] = {bsplineBasis0(t), bsplineBasis1(t),
                             bsplineBasis2(t), bsplineBasis3(t)};

        return (basisT[0]
                * (v[0] * basisS[0] + v[1] * basisS[1] + v[2] * basisS[2]
                   + v[3] * basisS[3]))
               +

               (basisT[1]
                * (v[4] * basisS[0] + v[5] * basisS[1] + v[6] * basisS[2]
                   + v[7] * basisS[3]))
               +

               (basisT[2]
                * (v[8] * basisS[0] + v[9] * basisS[1] + v[10] * basisS[2]
                   + v[11] * basisS[3]))
               +

               (basisT[3]
                * (v[12] * basisS[0] + v[13] * basisS[1] + v[14] * basisS[2]
                   + v[15] * basisS[3]));
    }

    //******************************************************************************
    // WITH DERIVATIVES
    template <typename CV, typename T>
    CV bspline(const CV& v00, const CV& v10, const CV& v20, const CV& v30,
               const CV& v01, const CV& v11, const CV& v21, const CV& v31,
               const CV& v02, const CV& v12, const CV& v22, const CV& v32,
               const CV& v03, const CV& v13, const CV& v23, const CV& v33,
               const T& s, const T& t, CV& dPds, CV& dPdt)
    {
        const T basisS[4] = {bsplineBasis0(s), bsplineBasis1(s),
                             bsplineBasis2(s), bsplineBasis3(s)};

        const T basisDs[4] = {bsplineBasis0Dt(s), bsplineBasis1Dt(s),
                              bsplineBasis2Dt(s), bsplineBasis3Dt(s)};

        const T basisT[4] = {bsplineBasis0(t), bsplineBasis1(t),
                             bsplineBasis2(t), bsplineBasis3(t)};

        const T basisDt[4] = {bsplineBasis0Dt(t), bsplineBasis1Dt(t),
                              bsplineBasis2Dt(t), bsplineBasis3Dt(t)};

        dPds = (basisT[0]
                * (v00 * basisDs[0] + v10 * basisDs[1] + v20 * basisDs[2]
                   + v30 * basisDs[3]))
               +

               (basisT[1]
                * (v01 * basisDs[0] + v11 * basisDs[1] + v21 * basisDs[2]
                   + v31 * basisDs[3]))
               +

               (basisT[2]
                * (v02 * basisDs[0] + v12 * basisDs[1] + v22 * basisDs[2]
                   + v32 * basisDs[3]))
               +

               (basisT[3]
                * (v03 * basisDs[0] + v13 * basisDs[1] + v23 * basisDs[2]
                   + v33 * basisDs[3]));

        dPdt = (basisDt[0]
                * (v00 * basisS[0] + v10 * basisS[1] + v20 * basisS[2]
                   + v30 * basisS[3]))
               +

               (basisDt[1]
                * (v01 * basisS[0] + v11 * basisS[1] + v21 * basisS[2]
                   + v31 * basisS[3]))
               +

               (basisDt[2]
                * (v02 * basisS[0] + v12 * basisS[1] + v22 * basisS[2]
                   + v32 * basisS[3]))
               +

               (basisDt[3]
                * (v03 * basisS[0] + v13 * basisS[1] + v23 * basisS[2]
                   + v33 * basisS[3]));

        return (basisT[0]
                * (v00 * basisS[0] + v10 * basisS[1] + v20 * basisS[2]
                   + v30 * basisS[3]))
               +

               (basisT[1]
                * (v01 * basisS[0] + v11 * basisS[1] + v21 * basisS[2]
                   + v31 * basisS[3]))
               +

               (basisT[2]
                * (v02 * basisS[0] + v12 * basisS[1] + v22 * basisS[2]
                   + v32 * basisS[3]))
               +

               (basisT[3]
                * (v03 * basisS[0] + v13 * basisS[1] + v23 * basisS[2]
                   + v33 * basisS[3]));
    }

    //******************************************************************************
    // ARRAY PASSED IN T-MAJOR ORDER, WITH DERIVATIVES
    template <typename CV, typename T>
    CV bspline(const CV* v, const T& s, const T& t, CV& dPds, CV& dPdt)
    {
        const T basisS[4] = {bsplineBasis0(s), bsplineBasis1(s),
                             bsplineBasis2(s), bsplineBasis3(s)};

        const T basisDs[4] = {bsplineBasis0Dt(s), bsplineBasis1Dt(s),
                              bsplineBasis2Dt(s), bsplineBasis3Dt(s)};

        const T basisT[4] = {bsplineBasis0(t), bsplineBasis1(t),
                             bsplineBasis2(t), bsplineBasis3(t)};

        const T basisDt[4] = {bsplineBasis0Dt(t), bsplineBasis1Dt(t),
                              bsplineBasis2Dt(t), bsplineBasis3Dt(t)};

        dPds = (basisT[0]
                * (v[0] * basisDs[0] + v[1] * basisDs[1] + v[2] * basisDs[2]
                   + v[3] * basisDs[3]))
               +

               (basisT[1]
                * (v[4] * basisDs[0] + v[5] * basisDs[1] + v[6] * basisDs[2]
                   + v[7] * basisDs[3]))
               +

               (basisT[2]
                * (v[8] * basisDs[0] + v[9] * basisDs[1] + v[10] * basisDs[2]
                   + v[11] * basisDs[3]))
               +

               (basisT[3]
                * (v[12] * basisDs[0] + v[13] * basisDs[1] + v[14] * basisDs[2]
                   + v[15] * basisDs[3]));

        dPdt = (basisDt[0]
                * (v[0] * basisS[0] + v[1] * basisS[1] + v[2] * basisS[2]
                   + v[3] * basisS[3]))
               +

               (basisDt[1]
                * (v[4] * basisS[0] + v[5] * basisS[1] + v[6] * basisS[2]
                   + v[7] * basisS[3]))
               +

               (basisDt[2]
                * (v[8] * basisS[0] + v[9] * basisS[1] + v[10] * basisS[2]
                   + v[11] * basisS[3]))
               +

               (basisDt[3]
                * (v[12] * basisS[0] + v[13] * basisS[1] + v[14] * basisS[2]
                   + v[15] * basisS[3]));

        return (basisT[0]
                * (v[0] * basisS[0] + v[1] * basisS[1] + v[2] * basisS[2]
                   + v[3] * basisS[3]))
               +

               (basisT[1]
                * (v[4] * basisS[0] + v[5] * basisS[1] + v[6] * basisS[2]
                   + v[7] * basisS[3]))
               +

               (basisT[2]
                * (v[8] * basisS[0] + v[9] * basisS[1] + v[10] * basisS[2]
                   + v[11] * basisS[3]))
               +

               (basisT[3]
                * (v[12] * basisS[0] + v[13] * basisS[1] + v[14] * basisS[2]
                   + v[15] * basisS[3]));
    }

} // End namespace TwkMath

#endif
