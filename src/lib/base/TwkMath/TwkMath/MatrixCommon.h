//******************************************************************************
// Copyright (c) 2001-2002 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkMathMatCommon_h_
#define _TwkMathMatCommon_h_

#include <TwkMath/Exc.h>

//******************************************************************************
// Functions which all of the matrix classes use.

#define T0 ((T)0)
#define T1 ((T)1)

namespace TwkMath
{

    //******************************************************************************
    template <typename T>
    inline T determinant2x2(const T m00, const T m01, const T m10, const T m11)
    {
        return (m00 * m11) - (m01 * m10);
    }

    //******************************************************************************
    template <typename T>
    inline T determinant3x3(const T m00, const T m01, const T m02, const T m10,
                            const T m11, const T m12, const T m20, const T m21,
                            const T m22)
    {
        return (m00 * m11 * m22) + (m01 * m12 * m20) + (m02 * m10 * m21)
               - (m02 * m11 * m20) - (m01 * m10 * m22) - (m00 * m12 * m21);
    }

    //******************************************************************************
    template <typename T>
    inline T determinant4x4(const T m00, const T m01, const T m02, const T m03,
                            const T m10, const T m11, const T m12, const T m13,
                            const T m20, const T m21, const T m22, const T m23,
                            const T m30, const T m31, const T m32, const T m33)
    {
        // Get cofactors..
        const T A00 =
            determinant3x3(m11, m12, m13, m21, m22, m23, m31, m32, m33);
        const T A01 =
            -T1 * determinant3x3(m10, m12, m13, m20, m22, m23, m30, m32, m33);
        const T A02 =
            determinant3x3(m10, m11, m13, m20, m21, m23, m30, m31, m33);
        const T A03 =
            -T1 * determinant3x3(m10, m11, m12, m20, m21, m22, m30, m31, m32);

        // Determinant! Whoo hoo!
        return (m00 * A00) + (m01 * A01) + (m02 * A02) + (m03 * A03);
    }

    //******************************************************************************
    template <typename T> void invert2x2(T& m00, T& m01, T& m10, T& m11)
    {
        const T det = determinant2x2(m00, m01, m10, m11);
        if (det == T0)
        {
            throw SingularMatrixExc();
        }

        // Swap m00 and m11, divide by det.
        const T tmp = m00;
        m00 = m11 / det;
        m11 = tmp / det;

        // Negate m01 and m10, also divide by det
        m01 /= -det;
        m10 /= -det;
    }

    //******************************************************************************
    template <typename T>
    void invert3x3(T& m00, T& m01, T& m02, T& m10, T& m11, T& m12, T& m20,
                   T& m21, T& m22)
    {
        // Get determinant
        const T det =
            determinant3x3(m00, m01, m02, m10, m11, m12, m20, m21, m22);
        if (det == T0)
        {
            throw SingularMatrixExc();
        }

        const T i00 = determinant2x2(m11, m12, m21, m22) / det;
        const T i10 = -determinant2x2(m10, m12, m20, m22) / det;
        const T i20 = determinant2x2(m10, m11, m20, m21) / det;

        const T i01 = -determinant2x2(m01, m02, m21, m22) / det;
        const T i11 = determinant2x2(m00, m02, m20, m22) / det;
        const T i21 = -determinant2x2(m00, m01, m20, m21) / det;

        const T i02 = determinant2x2(m01, m02, m11, m12) / det;
        const T i12 = -determinant2x2(m00, m02, m10, m12) / det;
        const T i22 = determinant2x2(m00, m01, m10, m11) / det;

        m00 = i00;
        m01 = i01;
        m02 = i02;

        m10 = i10;
        m11 = i11;
        m12 = i12;

        m20 = i20;
        m21 = i21;
        m22 = i22;
    }

} // End namespace TwkMath

#undef T0
#undef T1

#endif
