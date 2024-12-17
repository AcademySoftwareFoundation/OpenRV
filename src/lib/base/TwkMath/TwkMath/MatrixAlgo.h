//******************************************************************************
// Copyright (c) 2001-2002 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkMathMatrixAlgo_h_
#define _TwkMathMatrixAlgo_h_

#include <sys/types.h>
#include <string.h>
#include <algorithm>
#include <TwkMath/Math.h>
#include <TwkMath/Exc.h>

#define T0 ((T)0)
#define T1 ((T)1)

namespace TwkMath
{

    //******************************************************************************
    template <typename T, class MAT, size_t N> class gaussjInvert
    {
    public:
        gaussjInvert() {}

        static void doit(MAT& mat);
    };

    //******************************************************************************
    template <typename T, class MAT, size_t N>
    void gaussjInvert<T, MAT, N>::doit(MAT& mat)
    {
        if (mat[0][0] != mat[0][0])
            throw SingularMatrixExc();

        int a, b, c, d, e;
        int irow, icol;
        int indxc[N];
        int indxr[N];
        int pivotIndex[N];

        memset((void*)indxc, 0, N * sizeof(int));
        memset((void*)indxr, 0, N * sizeof(int));
        memset((void*)pivotIndex, 0, N * sizeof(int));

        // This is the main loop over the columns to be reduced.
        for (a = 0; a < N; ++a)
        {
            T big = T0;

            // This is the outer loop of the search for a pivot
            // element.
            for (b = 0; b < N; ++b)
            {
                if (pivotIndex[b] != T1)
                {
                    for (c = 0; c < N; ++c)
                    {
                        if (pivotIndex[c] == T0)
                        {
                            T abc = Math<T>::abs(mat[b][c]);
                            if (abc >= big)
                            {
                                big = abc;
                                irow = b;
                                icol = c;
                            }
                        }
                        else if (pivotIndex[c] > T1)
                        {
                            throw SingularMatrixExc();
                        }
                    }
                }
            }

            ++pivotIndex[icol];

            // We now have the pivot element, so we exchange rows, if
            // needed, to put the pivot element on the diagonal. The
            // columns are not physically interchanged, only relabeled:
            // indxc[a], the column of the ith pivot element, is the ith
            // column that is reduced, while
            // indxr[a] is the row in which that pivot element was
            // originally located. If indxr[a] != indxc[a], there is an
            // implied column interchange. With this form of bookkeeping,
            // the solution b's will end up in the correct order, and
            // the inverse matrix will be scrambled by columns.
            if (irow != icol)
            {
                for (d = 0; d < N; ++d)
                {
                    std::swap(mat[irow][d], mat[icol][d]);
                }
            }

            // We are now ready to divide the pivot row by the pivot
            // element, located at irow and icol;
            indxr[a] = irow;
            indxc[a] = icol;
            if (mat[icol][icol] == T0)
            {
                throw SingularMatrixExc();
            }
            T pivinv = T1 / (mat[icol][icol]);
            mat[icol][icol] = T1;
            for (d = 0; d < N; ++d)
            {
                mat[icol][d] *= pivinv;
            }

            // Next we reduce the rows,
            // except for the pivot one, of course.
            for (e = 0; e < N; ++e)
            {
                if (e != icol)
                {
                    T dum = mat[e][icol];
                    mat[e][icol] = T0;
                    for (d = 0; d < N; ++d)
                    {
                        mat[e][d] -= mat[icol][d] * dum;
                    }
                }
            }
        }

        // This is the end of the main loop over columns of the reduction.
        // It only remains to unscrmable the solution in view of the column
        // interchanges. We do this by interchaning pairs of columns in
        // the reverse order that the permutation was built up.
        for (d = N - 1; d >= 0; --d)
        {
            if (indxr[d] != indxc[d])
            {
                for (c = 0; c < N; ++c)
                {
                    std::swap(mat[c][indxr[d]], mat[c][indxc[d]]);
                }
            }
        }
    }

} // End namespace TwkMath

#undef T0
#undef T1
#endif
