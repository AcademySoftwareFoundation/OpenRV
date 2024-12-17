//******************************************************************************
// Copyright (c) 2001-2003 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkMathPca22_h_
#define _TwkMathPca22_h_

#include <TwkMath/Vec2.h>
#include <TwkMath/Math.h>
#include <TwkMath/Function.h>
#include <algorithm>
#include <vector>
#include <iostream>

#define T0 ((T)0)
#define T1 ((T)1)
#define T2 ((T)2)
#define T3 ((T)3)
#define T4 ((T)4)
#define T_PI ((T)M_PI)
// #define ZERO_TOLERANCE ((T)1.0e-12)
#define ZERO_TOLERANCE (Math<T>::epsilon())
#define IS_ZERO(A) (zeroTol((A), ZERO_TOLERANCE))
#define ARE_EQUAL(A, B) (equalTol((A), (B), ZERO_TOLERANCE))

namespace TwkMath
{

    //******************************************************************************
    // FORCE ORTHOGONAL
    template <typename T> void forceOrthogonal(Vec2<T> vecs[])
    {
        // Keep 0 as is.
        vecs[0].normalize();

        // Get 1.
        Vec2<T> tmp(-vecs[0].y, vecs[0].x);
        if (dot(tmp, vecs[1]) < 0)
        {
            vecs[1] = -tmp;
        }
        else
        {
            vecs[1] = tmp;
        }
        vecs[1].normalize();
    }

    //******************************************************************************
    // Solve for quad roots. Returns the number of roots found
    template <typename T> size_t realQuadraticRoots(T a0, T a1, T roots[])
    {
        // ( -b +- sqrt( b^2 - 4ac ) ) / 2a
        T discrim = (a1 * a1) - (T4 * a0);
        if (discrim < T0)
        {
            return 0;
        }
        else if (discrim == T0)
        {
            roots[0] = -a1 / T2;
            return 1;
        }

        discrim = Math<T>::sqrt(discrim);
        roots[0] = (-a1 - discrim) / T2;
        roots[1] = (-a1 + discrim) / T2;
        return 2;
    }

    //******************************************************************************
    // Finding an eigenvector for a given symmetric matrix
    template <typename T> Vec2<T> findUnitEigenVector2(T A, T B, T C, T Y)
    {
        // Check degenerate:
        if (ARE_EQUAL(A, Y))
        {
            return Vec2<T>(T1, T0);
        }
        else if (ARE_EQUAL(C, Y))
        {
            return Vec2<T>(T0, T1);
        }

        return Vec2<T>(B / (Y - A), T1).normalized();
    }

    //******************************************************************************
    // Eigen analysis of a 2x2
    //
    // | a1 a2 |           | (A-Y)   B  |
    // | b1 b2 |           |   B  (C-Y) |
    //
    // a1 b2 - b1 a2
    //
    // a1 b2
    // -b1 a2
    //
    // (A-Y)(C-Y)
    // -B^2
    //
    // Y^2 - (A+C)Y + AC - B^2 = 0
    //
    // (1)Y^2
    // (-A-C)Y
    // (AC - B^2 )
    //
    // Use quadratic eqn to solve.
    //
    // For these arguments,
    // A = sum( ( xi - xmean )^2 )
    // B = sum( ( xi - xmean ) * ( yi - ymean ) )
    // C = sum( ( yi - ymean )^2 )
    template <typename T>
    void eigenAnalysisOf2x2CovarianceMatrix(T A, T B, T C, Vec2<T> vecs[],
                                            T vals[])
    {
        // See if it's already diagonal.
        if (IS_ZERO(B))
        {
            // It's diagonal. All done.
            if (A > C)
            {
                vecs[0].set(T1, T0);
                vals[0] = A;
                vecs[1].set(T0, T1);
                vals[1] = C;
            }
            else
            {
                vecs[0].set(T0, T1);
                vals[0] = C;
                vecs[1].set(T1, T0);
                vals[1] = A;
            }
            return;
        }

        // Get quadratic roots
        T a1 = (-A - C);
        T a0 = ((A * C) - (B * B));
        T roots[2];
        size_t numRoots = realQuadraticRoots(a0, a1, roots);

        if (numRoots == 0)
        {
            // Weird. This should not happen
            // Pretend diagonal and quit
#if DEBUG
            std::cerr << "WARNING: zero roots of characteristic eqn of 2x2"
                      << std::endl
                      << "covariance matrix. That's not supposed to happen"
                      << std::endl;
#endif
            if (A > C)
            {
                vecs[0].set(T1, T0);
                vals[0] = A;
                vecs[1].set(T0, T1);
                vals[1] = C;
            }
            else
            {
                vecs[0].set(T0, T1);
                vals[0] = C;
                vecs[1].set(T1, T0);
                vals[1] = A;
            }
            return;
        }
        else if (numRoots == 1)
        {
            // Means eigenvalues are the same.
            vecs[0] = findUnitEigenVector2(A, B, C, roots[0]);
            vecs[1].set(-vecs[0].y, vecs[0].x);

            forceOrthogonal(vecs);

            vals[0] = vals[1] = roots[0];
            return;
        }

        // Two distinct eigenvalues
        if (roots[0] > roots[1])
        {
            vecs[0] = findUnitEigenVector2(A, B, C, roots[0]);
            vals[0] = roots[0];
            vecs[1].set(-vecs[0].y, vecs[0].x);

            forceOrthogonal(vecs);

            vals[1] = roots[1];
        }
        else
        {
            vecs[0] = findUnitEigenVector2(A, B, C, roots[1]);
            vals[0] = roots[1];
            vecs[1].set(-vecs[0].y, vecs[0].x);

            forceOrthogonal(vecs);

            vals[1] = roots[0];
        }
    }

    //******************************************************************************
    // Find the principal components of a 3d point distribution.
    template <class ITER, class VEC, typename TA, typename TB>
    void principalComponentAnalysis2(ITER begin, ITER end, VEC& mean,
                                     VEC components[], TA vals[], TB dummy)
    {
        size_t numVerts = (size_t)(end - begin);

        // Check for degenerate case.
        if (numVerts == 0)
        {
            mean = ((TA)0);
            components[0].set(((TA)1), ((TA)0));
            components[1].set(((TA)0), ((TA)1));
            vals[0] = vals[1] = ((TA)1);
            return;
        }

        // Get the mean
        Vec2<TB> meanD(((TB)0));
        for (ITER viter = begin; viter != end; ++viter)
        {
            meanD += Vec2<TB>((*viter));
        }
        meanD /= TB(numVerts);

        // Get covariance matrix
        TB mxx = ((TB)0);
        TB mxy = ((TB)0);
        TB myy = ((TB)0);
        for (ITER viter = begin; viter != end; ++viter)
        {
            TB vx = TB((*viter).x) - meanD.x;
            TB vy = TB((*viter).y) - meanD.y;

            mxx += vx * vx;
            mxy += vx * vy;
            myy += vy * vy;
        }
        mxx /= TB(numVerts);
        mxy /= TB(numVerts);
        myy /= TB(numVerts);

        // Get eigenvectors/values
        Vec2<TB> evecs[2];
        TB evals[2];
        eigenAnalysisOf2x2CovarianceMatrix(mxx, mxy, myy, evecs, evals);

        // Put values in
        mean.set(meanD.x, meanD.y);
        components[0].set(evecs[0].x, evecs[0].y);
        components[1].set(evecs[1].x, evecs[1].y);
        vals[0] = evals[0];
        vals[1] = evals[1];
    }

} // End namespace TwkMath

#undef T0
#undef T1
#undef T2
#undef T3
#undef T4
#undef T_PI
#undef ZERO_TOLERANCE
#undef IS_ZERO
#undef ARE_EQUAL

#endif
