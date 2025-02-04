//******************************************************************************
// Copyright (c) 2003 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkMath__TwkMathMatrixAlgo2__h__
#define __TwkMath__TwkMathMatrixAlgo2__h__
#include <TwkMath/Mat44.h>
#include <TwkMath/Vec3.h>
#include <TwkMath/Frustum.h>
#include <vector>

//
//  Additional Matrix Algorithms
//

namespace TwkMath
{

    //
    //  A == vector< Vec3<T> > points
    //  B == vector< Vec3<T> > points that are A transfomed
    //

    template <typename T>
    Mat44<T> multiplyAsHomogeneous(const std::vector<Vec3<T>>& A,
                                   const std::vector<Vec3<T>>& B)
    {
        Mat44<T> M;

        for (int a = 0; a < 4; a++)
        {
            for (int b = 0; b < 4; b++)
            {
                T sum = T(0);

                for (int col = 0; col < A.size(); col++)
                {
                    T Ap = a == 3 ? 1.0 : A[col][a];
                    T Bp = b == 3 ? 1.0 : B[col][b];
                    sum += Ap * Bp;
                }

                M(a, b) = sum;
            }
        }

        return M;
    }

    //
    //  Seems to solve for M when:
    //
    //  A = M * B
    //

    template <typename T>
    Mat44<T> LeastSquaredFitTransform(const std::vector<Vec3<T>>& A,
                                      const std::vector<Vec3<T>>& B)
    {
        //       T   -1
        // B A (A  A)   == M
        //

        Mat44<T> BA = multiplyAsHomogeneous(B, A);
        Mat44<T> ATA = multiplyAsHomogeneous(A, A);
        ATA.invert();
        return BA * ATA;
    }

    //
    //  Decompose Orthographic Projection
    //  (Mathematica)
    //

    template <typename T> Frustum<T> DecomposeOrthoProjection(const Mat44<T>& P)
    {
        const T a = P(0, 0);
        const T g = P(0, 3);
        const T c = P(1, 1);
        const T d = P(1, 3);
        const T e = P(2, 2);
        const T h = P(2, 3);

        return Frustum<T>(-((g + T(1)) / a), -((g - T(1)) / a),
                          -((d + T(1)) / c), -((d - T(1)) / c),
                          -((-h - T(1)) / e), -((T(1) - h) / e), true);
    }

} // namespace TwkMath

#endif // __TwkMath__TwkMathMatrixAlgoT(2)__h__
