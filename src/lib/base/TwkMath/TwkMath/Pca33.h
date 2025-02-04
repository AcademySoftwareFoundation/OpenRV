//******************************************************************************
// Copyright (c) 2001-2003 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkMathPca33_h_
#define _TwkMathPca33_h_

#include <TwkMath/Vec3.h>
#include <TwkMath/Math.h>
#include <TwkMath/Pca22.h>
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
    template <typename T> void forceOrthogonal(Vec3<T> vecs[])
    {
        // Keep 0 as is.
        vecs[0].normalize();

        // Get 2.
        Vec3<T> tmp = cross(vecs[0], vecs[1]);
        if (dot(tmp, vecs[2]) < 0)
        {
            vecs[2] = -tmp;
        }
        else
        {
            vecs[2] = tmp;
        }
        vecs[2].normalize();

        // Get 1.
        tmp = cross(vecs[2], vecs[0]);
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
    // Solve for cubic roots. we know there will be some because symmetric
    // matrices always have real eigenvectors.
    //
    // z^3 + a2 z^2 + a1 z + a0 = 0
    //
    // Q = ( 3 a1 - a2^2 ) / 9
    // R = ( 9 a2 a1 - 27 a0 - 2 a2^3 ) / 54
    //
    // D = ( Q^3 + R^2 ) -> D is the "polynomial discriminant"
    // if D > 0, one real root, two complex ones (complex conjugates)
    // if D == 0, one real root, two others that equal each other
    // if D < 0, three distinct real roots.
    //
    // Assuming D <= 0 (it has to be for our matrix)
    //
    // Theta = acos( R / sqrt( -Q^3 ) )
    //
    // root1 = 2 sqrt( -Q ) cos( Theta / 3 ) - a2/3
    // root2 = 2 sqrt( -Q ) cos( ( Theta + 2pi ) / 3 ) - a2/3
    // root3 = 2 sqrt( -Q ) cos( ( Theta + 4pi ) / 3 ) - a2/3
    //
    //******************************************************************************

    //******************************************************************************
    // Returns the number of real cubic roots encountered: 1, 2 or 3.
    // It is assumed that cubic has been normalized such that the coefficient
    // of the cubed term is 1.
    template <typename T> size_t realCubicRoots(T a0, T a1, T a2, T roots[])
    {
        T Q = ((T3 * a1) - (a2 * a2)) / ((T)9);
        T R = ((((T)9) * a2 * a1) - (((T)27) * a0) - (T2 * a2 * a2 * a2))
              / ((T)54);

        T D = (Q * Q * Q) + (R * R);

        T a2Div3 = a2 / T3;

        if (D > T0)
        {
            // One real root.
            T sqrtD = Math<T>::sqrt(D);
            T S = Math<T>::cbrt(R + sqrtD);
            T Ta = Math<T>::cbrt(R - sqrtD);
            roots[0] = S + Ta - a2Div3;
            return 1;
        }
        else if (D == T0)
        {
            if (R != T0 || Q != T0)
            {
                // If D == 0,
                // Q^3 == -(R^2)
                // therefore,
                // -(Q^3) == R^2
                // therefore,
                // acos( R / sqrt( -(Q^3)) ) = acos( R / sqrt(R^2) )
                //                           = acos( R/R ) = acos( 1 ) = 0
                static const T cosTwoThirdsPi = Math<T>::cos(T2 * T_PI / T3);

                // z1 = 2 sqrt( -Q ) - a2/3
                // z2,z3 = 2 sqrt( -Q ) cos(2pi/3) - a2/3
                roots[0] = T2 * Math<T>::sqrt(-Q);
                roots[1] = (roots[0] * cosTwoThirdsPi) - a2Div3;
                roots[0] -= a2Div3;
                return 2;
            }
            else
            {
                roots[0] = roots[1] = -a2Div3;
                return 1;
            }
        }

        // D < 0
        T Theta = Math<T>::acos(R / Math<T>::sqrt(-(Q * Q * Q)));
        roots[0] = roots[1] = roots[2] = T2 * Math<T>::sqrt(-Q);
        roots[0] = (roots[0] * Math<T>::cos(Theta / T3)) - a2Div3;
        roots[1] = (roots[1] * Math<T>::cos((Theta + T2 * T_PI) / T3)) - a2Div3;
        roots[2] = (roots[2] * Math<T>::cos((Theta + T4 * T_PI) / T3)) - a2Div3;

        return 3;
    }

    //******************************************************************************
    // Finding an eigenvector for a given symmetric matrix
    template <typename T>
    Vec3<T> findUnitEigenVector(T A, T B, T C, T D, T E, T F, T Y)
    {
        // The solution if x == 1 for the eigenvector:
        //
        // Let M = [ B      C ]
        //         [ (D-Y)  E ]
        //
        // Let V = [ (Y-A) ]
        //         [  -B   ]
        //
        // (y,z) = inv( M ) * V;
        //
        // So, we can check the discriminant of M to see if x should actually
        // be set to zero.
        //
        // If the discriminant of M is very close to zero, then x is actually
        // supposed to be really close to zero too, so setting it to one is
        // numerically not good.
        //
        // Then, set y == 1
        //
        // Let M = [ (A-Y)  C ]
        //         [ B      E ]
        //
        // Let V = [  -B   ]
        //         [ (Y-D) ]
        //
        // (x,z) = inv( M ) * V

        T a = B;
        T b = C;
        T c = D - Y;
        T d = E;
        T vy = Y - A;
        T vz = -B;

        T discrim = (a * d) - (c * b);

        if (IS_ZERO(discrim))
        {
            // Set y == 1.
            a = A - Y;
            b = C;
            c = B;
            d = E;
            T vx = -B;
            vz = Y - D;

            discrim = (a * d) - (c * b);

            if (IS_ZERO(discrim))
            {
                // This can only happen if the matrix was ALREADY
                // diagonalized.
                T dx = Math<T>::abs(A - Y);
                T dy = Math<T>::abs(D - Y);
                T dz = Math<T>::abs(F - Y);

                if (dx < dy)
                {
                    if (dx < dz)
                    {
                        return Vec3<T>(T1, T0, T0);
                    }
                    else
                    {
                        return Vec3<T>(T0, T0, T1);
                    }
                }
                else
                {
                    if (dy < dz)
                    {
                        return Vec3<T>(T0, T1, T0);
                    }
                    else
                    {
                        return Vec3<T>(T0, T0, T1);
                    }
                }
            }

            std::swap(a, d);
            a /= discrim;
            b /= -discrim;
            c /= -discrim;
            d /= discrim;

            return Vec3<T>(a * vx + b * vz, T1, c * vx + d * vz).normalized();
        }

        std::swap(a, d);
        a /= discrim;
        b /= -discrim;
        c /= -discrim;
        d /= discrim;

        return Vec3<T>(T1, a * vy + b * vz, c * vy + d * vz).normalized();
    }

    //******************************************************************************
    // Get eigenvalues
    //
    // | a1  a2  a3 |    | (A-Y)  B   C  |
    // | b1  b2  b3 |    | B   (D-Y)  E  |
    // | c1  c2  c3 |    | C    E  (F-Y) |
    //
    // a1 b2 c3 - a1 b3 c2 - a2 b1 c3 + a2 b3 c1 + a3 b1 c2 - a3 b2 c1
    //
    // a1 b2 c3
    // -a1 b3 c2
    // -a2 b1 c3
    // a2 b3 c1
    // a3 b1 c2
    // -a3 b2 c1
    //
    // (A-Y) (D-Y) (F-Y)
    // -(A-Y) E E
    // -B B (F-Y)
    // B E C
    // C B E
    // -C (D-Y) C
    //
    // (-1)Y^3 + (A+D+F)Y^2 + (-AD -AF -DF)Y + (ADF)
    // (-A E^2) + (E^2)Y
    // (-F B^2) + (B^2)Y
    // (BCE)
    // (BCE)
    // (-D C^2) + (C^2)Y
    //
    // (-1)Y^3
    // (A+D+F)Y^2
    // (-AD -AF -DF + E^2 + B^2 + C^2)Y
    // (ADF - AE^2 - FB^2 - DC^2 + 2BCE)
    //
    // Flip it...
    //
    // Y^3
    // (-A-D-F)Y^2
    // (AD + AF + DF -E^2 -B^2 -C^2)Y
    // (AE^2 + FB^2 + DC^2 -ADF -2BCE)
    //
    // a3 = 1
    // a2 = (-A-D-F)
    // a1 = (AD + AF + DF -E^2 -B^2 -C^2)
    // a0 = (AE^2 FB^2 DC^2 -ADF -2BCE)
    //
    // For these arguments,
    // A = sum( ( xi - xMean )^2 )
    // B = sum( ( xi - xMean ) * ( yi - yMean ) )
    // C = sum( ( xi - xMean ) * ( zi - zMean ) )
    // D = sum( ( yi - yMean )^2 )
    // E = sum( ( yi - yMean ) * ( zi - zMean ) )
    // F = sum( ( zi - zMean )^2 )
    template <typename T>
    void eigenAnalysisOf3x3CovarianceMatrix(T A, T B, T C, T D, T E, T F,
                                            Vec3<T> vecs[], T vals[])
    {
        // See if we can do a 2x2
        Vec2<T> vecs2[2];
        T vals2[2];
        if (IS_ZERO(B) && IS_ZERO(C))
        {
            // If xy and xz are both zero, it means the x direction is
            // already independent
            vals[0] = A;
            vecs[0].set(T1, T0, T0);

            // Get the 2x2 eigens
            eigenAnalysisOf2x2CovarianceMatrix(D, E, F, vecs2, vals2);

            vals[1] = vals2[0];
            vecs[1].set(T0, vecs2[0][0], vecs2[0][1]);

            vals[2] = vals2[1];
            vecs[2].set(T0, vecs2[1][0], vecs2[1][1]);

            // Sort and return
            if (vals[0] < vals[1])
            {
                std::swap(vals[0], vals[1]);
                std::swap(vecs[0], vecs[1]);
            }
            if (vals[0] < vals[2])
            {
                std::swap(vals[0], vals[2]);
                std::swap(vecs[0], vecs[2]);
            }
            if (vals[1] < vals[2])
            {
                std::swap(vals[1], vals[2]);
                std::swap(vecs[1], vecs[2]);
            }

            assert(vals[0] >= vals[1]);
            assert(vals[1] >= vals[2]);

            forceOrthogonal(vecs);

            return;
        }
        else if (IS_ZERO(B) && IS_ZERO(E))
        {
            // If xy and yz are both zero, it means the y direction is
            // already independent
            vals[0] = D;
            vecs[0].set(T0, T1, T0);

            // Get the 2x2 eigens
            eigenAnalysisOf2x2CovarianceMatrix(A, C, F, vecs2, vals2);

            vals[1] = vals2[0];
            vecs[1].set(vecs2[0][0], T0, vecs2[0][1]);

            vals[2] = vals2[1];
            vecs[2].set(vecs2[1][0], T0, vecs2[1][1]);

            // Sort and return
            if (vals[0] < vals[1])
            {
                std::swap(vals[0], vals[1]);
                std::swap(vecs[0], vecs[1]);
            }
            if (vals[0] < vals[2])
            {
                std::swap(vals[0], vals[2]);
                std::swap(vecs[0], vecs[2]);
            }
            if (vals[1] < vals[2])
            {
                std::swap(vals[1], vals[2]);
                std::swap(vecs[1], vecs[2]);
            }

            assert(vals[0] >= vals[1]);
            assert(vals[1] >= vals[2]);

            forceOrthogonal(vecs);

            return;
        }
        else if (IS_ZERO(C) && IS_ZERO(E))
        {
            // If xz and yz are both zero, it means the z direction is
            // already independent
            vals[0] = F;
            vecs[0].set(T0, T0, T1);

            // Get the 2x2 eigens
            eigenAnalysisOf2x2CovarianceMatrix(A, B, D, vecs2, vals2);

            vals[1] = vals2[0];
            vecs[1].set(vecs2[0][0], vecs2[0][1], T0);

            vals[2] = vals2[1];
            vecs[2].set(vecs2[1][0], vecs2[1][1], T0);

            // Sort and return
            if (vals[0] < vals[1])
            {
                std::swap(vals[0], vals[1]);
                std::swap(vecs[0], vecs[1]);
            }
            if (vals[0] < vals[2])
            {
                std::swap(vals[0], vals[2]);
                std::swap(vecs[0], vecs[2]);
            }
            if (vals[1] < vals[2])
            {
                std::swap(vals[1], vals[2]);
                std::swap(vecs[1], vecs[2]);
            }

            assert(vals[0] >= vals[1]);
            assert(vals[1] >= vals[2]);

            forceOrthogonal(vecs);

            return;
        }

        T a2 = (-A - D - F);
        T a1 = ((A * D) + (A * F) + (D * F) - (E * E) - (B * B) - (C * C));
        T a0 = ((A * E * E) + (F * B * B) + (D * C * C) - (A * D * F)
                - (T2 * B * C * E));
        T roots[3] = {T0, T0, T0};
        size_t numRoots = realCubicRoots(a0, a1, a2, roots);

        if (numRoots == 1)
        {
            // Hmm - only one real eigen value - this means that the matrix
            // is diagonal and all the diagonal values are equal.
            vecs[0].set(T1, T0, T0);
            vecs[1].set(T0, T1, T0);
            vecs[2].set(T0, T0, T1);
            vals[0] = vals[1] = vals[2] = roots[0];

            assert(vals[0] >= vals[1]);
            assert(vals[1] >= vals[2]);

            // No need to force orthogonal

            return;
        }
        else if (numRoots == 2)
        {
            // Two eigen values - this means that there's one axis around
            // which the distribution represented by the covariance matrix
            // is rotationally invariant.
            // The second root here is the T one.
            if (roots[0] >= roots[1])
            {
                // They're in order. Note the forced orthagonality.
                vecs[0] = findUnitEigenVector(A, B, C, D, E, F, roots[0]);
                vecs[1] = findUnitEigenVector(A, B, C, D, E, F, roots[1]);
                vecs[2] = cross(vecs[0], vecs[1]);

                forceOrthogonal(vecs);

                vals[0] = roots[0];
                vals[1] = vals[2] = roots[1];
            }
            else
            {
                vecs[0] = findUnitEigenVector(A, B, C, D, E, F, roots[1]);
                vecs[2] = findUnitEigenVector(A, B, C, D, E, F, roots[0]);
                vecs[1] = cross(vecs[2], vecs[0]);

                forceOrthogonal(vecs);

                vals[0] = vals[1] = roots[1];
                vals[2] = roots[0];
            }

            assert(vals[0] >= vals[1]);
            assert(vals[1] >= vals[2]);

            return;
        }

        // Sort eigenvalues
        if (roots[0] < roots[1])
        {
            std::swap(roots[0], roots[1]);
        }
        if (roots[0] < roots[2])
        {
            std::swap(roots[0], roots[2]);
        }
        if (roots[1] < roots[2])
        {
            std::swap(roots[1], roots[2]);
        }

        vecs[0] = findUnitEigenVector(A, B, C, D, E, F, roots[0]);
        vecs[1] = findUnitEigenVector(A, B, C, D, E, F, roots[1]);
        vecs[2] = cross(vecs[0], vecs[1]);

        forceOrthogonal(vecs);

        vals[0] = roots[0];
        vals[1] = roots[1];
        vals[2] = roots[2];

        assert(vals[0] >= vals[1]);
        assert(vals[1] >= vals[2]);

        // All done.
    }

    //******************************************************************************
    // Find the principal components of a 3d point distribution.
    template <class ITER, class VEC, typename TA, typename TB>
    void principalComponentAnalysis3(ITER begin, ITER end, VEC& mean,
                                     VEC components[], TA vals[], TB dummy)
    {
        size_t numVerts = (size_t)(end - begin);

        // Check for degenerate case.
        if (numVerts == 0)
        {
            mean = ((TA)0);
            components[0].set(((TA)1), ((TA)0), ((TA)0));
            components[1].set(((TA)0), ((TA)1), ((TA)0));
            components[2].set(((TA)0), ((TA)0), ((TA)1));
            vals[0] = vals[1] = vals[2] = ((TA)1);
            return;
        }

        // Get the mean
        Vec3<TB> meanD(((TB)0));
        for (ITER viter = begin; viter != end; ++viter)
        {
            meanD += Vec3<TB>((*viter));
        }
        meanD /= TB(numVerts);

        // Get covariance matrix
        TB mxx = ((TB)0);
        TB mxy = ((TB)0);
        TB mxz = ((TB)0);
        TB myy = ((TB)0);
        TB myz = ((TB)0);
        TB mzz = ((TB)0);
        for (ITER viter = begin; viter != end; ++viter)
        {
            TB vx = TB((*viter).x) - meanD.x;
            TB vy = TB((*viter).y) - meanD.y;
            TB vz = TB((*viter).z) - meanD.z;

            mxx += vx * vx;
            mxy += vx * vy;
            mxz += vx * vz;
            myy += vy * vy;
            myz += vy * vz;
            mzz += vz * vz;
        }
        mxx /= TB(numVerts);
        mxy /= TB(numVerts);
        mxz /= TB(numVerts);
        myy /= TB(numVerts);
        myz /= TB(numVerts);
        mzz /= TB(numVerts);

        // Get eigenvectors/values
        Vec3<TB> evecs[3];
        TB evals[3];
        eigenAnalysisOf3x3CovarianceMatrix(mxx, mxy, mxz, myy, myz, mzz, evecs,
                                           evals);

        // Put values in
        mean.set(meanD.x, meanD.y, meanD.z);
        components[0].set(evecs[0].x, evecs[0].y, evecs[0].z);
        components[1].set(evecs[1].x, evecs[1].y, evecs[1].z);
        components[2].set(evecs[2].x, evecs[2].y, evecs[2].z);
        vals[0] = evals[0];
        vals[1] = evals[1];
        vals[2] = evals[2];
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
