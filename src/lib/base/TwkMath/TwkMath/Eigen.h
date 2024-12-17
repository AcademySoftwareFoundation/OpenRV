//******************************************************************************
// Copyright (c) 2001-2002 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkMathEigen_h_
#define _TwkMathEigen_h_

#include <TwkMath/Math.h>
#include <TwkMath/Function.h>
#include <TwkMath/Exc.h>
#include <algorithm>

namespace TwkMath
{

    //******************************************************************************
    //******************************************************************************
    // MEAN OF SOME SAMPLES
    //******************************************************************************
    //******************************************************************************

    //******************************************************************************
    // Function to produce a uniformly weighted sample mean based on a
    // population of samples. This version assumes that dimensions must be
    // iterated over individually.
    template <class ARRAY2D, class ARRAY1D, class T>
    inline void sampleMean(const ARRAY2D& samples, int numSamples,
                           int dimension, ARRAY1D& mean, T dummy)
    {
        // Set mean to zero.
        int dim;
        for (dim = 0; dim < dimension; dim++)
        {
            mean[dim] = (T)0;
        }

        // Accumulate samples into the mean.
        for (int samp = 0; samp < numSamples; samp++)
        {
            for (dim = 0; dim < dimension; dim++)
            {
                mean[dim] += samples[samp][dim];
            }
        }

        // Normalize the mean
        for (dim = 0; dim < dimension; dim++)
        {
            mean[dim] /= (T)numSamples;
        }
    }

    //******************************************************************************
    // Function to produce a uniformly weighted sample mean based on a
    // population of samples. This version assumes that samples
    // have arithmetic operations built in, and can be assigned to
    // scalars.
    template <class ARRAY2D, class ARRAY1D, class T>
    inline void sampleMean(const ARRAY2D& samples, int numSamples,
                           ARRAY1D& mean, T dummy)
    {
        // Set mean to zero.
        mean = (T)0;

        // Accumulate samples into the mean.
        for (int samp = 0; samp < numSamples; samp++)
        {
            mean += samples[samp];
        }

        // Normalize the mean
        mean /= (T)numSamples;
    }

    //******************************************************************************
    //******************************************************************************
    // COVARIANCE MATRIX OF SOME SAMPLES
    //******************************************************************************
    //******************************************************************************

    //******************************************************************************
    // Function to produce a uniformly weighted covariance matrix based on a
    // population of samples and their given mean.
    template <class ARRAY2D_1, class ARRAY2D_2, class ARRAY1D, class T>
    void sampleCovarianceMatrix(const ARRAY2D_1& samples, int numSamples,
                                int dimension, const ARRAY1D& mean,
                                ARRAY2D_2& covarMat, T dummy)
    {
        // Counter variables
        int samp;
        int row;
        int col;

        // Initialize upper triangle of covariance matrix to zero
        for (row = 0; row < dimension; row++)
        {
            for (col = row; col < dimension; col++)
            {
                covarMat[row][col] = (T)0;
            }
        }

        // Accumulate covariances into upper triangle of matrix.
        for (samp = 0; samp < numSamples; samp++)
        {
            // Only calculate upper triangle,
            // since covariance matrix is symmetric
            for (row = 0; row < dimension; row++)
            {
                for (col = row; col < dimension; col++)
                {
                    covarMat[row][col] +=
                        ((T)(samples[samp][row] - mean[row]))
                        * ((T)(samples[samp][col] - mean[col]));
                }
            }
        }

        // Fill in lower triangle of matrix and normalize
        // covariances at the same time.
        for (row = 0; row < dimension; row++)
        {
            for (col = row; col < dimension; col++)
            {
                // row,col is defined, but unnormalized.
                // col,row is not.
                covarMat[col][row] = covarMat[row][col] =
                    covarMat[row][col] / (T)numSamples;
            }
        }
    }

    //******************************************************************************
    // Unnamed namespace used for helper functions that will
    // remain scoped to THIS FILE ONLY, even if this file is included
    // in another file.
    namespace
    {

        //******************************************************************************
        template <class T> inline T safeHypot(const T& a, const T& b)
        {
            T absa = Math<T>::abs(a);
            T absb = Math<T>::abs(b);

            if (absa > absb)
            {
                std::swap(absa, absb);
            }

            if (absb == (T)0)
            {
                return (T)0;
            }
            else
            {
                return absb * Math<T>::sqrt(((T)1) + Math<T>::sqr(absa / absb));
            }
        }

        //******************************************************************************
        template <class T>
        inline bool relativeUnderflowTest(const T& testVal, const T& anorm)
        {
            // The original numerical recipies had this statement,
            // which really makes no sense to me:
            // if ( (float)(fabs(testVal)+anorm) == anorm )
            // Anorm is defined to be non-negative.
            // Is this just some weird way of checking for a certain
            // kind of roundoff error?
            const float tv = (float)testVal;
            const float an = (float)anorm;
            const double added = Math<double>::abs((double)tv) + (double)an;
            return (bool)(((float)added) == an);
        }

        //******************************************************************************
        template <class T>
        inline T makeSameSign(const T& val, const T& desiredSign)
        {
            if (desiredSign > 0)
            {
                return Math<T>::abs(val);
            }
            else
            {
                return -Math<T>::abs(val);
            }
        }

    } // End unnamed namespace

    //******************************************************************************
    //******************************************************************************
    // SINGULAR VALUE DECOMPOSITION
    // SVD methods are based on the following theorem of linear algebra: Any MxN
    // matrix A whose number of rows M is greater than or equal to its number
    // of columns N, can be written as the product of an MxN column-orthogonal
    // matrix U, an NxN diagonal matrix W with positive or zero elements
    // (the singular values), and the transpose of an NxN orthogonal matrix V.
    // A = U * W * (V^T)
    //
    // SVD is the method of choice for solving most linear least-squares
    // problems. Furthermore, it is a good choice for finding the inverse and
    // eigenvectors/eigenvalues of a covariance matrix when it is not known
    // whether the data of the covariance matrix had good variance in each
    // of its components.
    //
    // A column-orthogonal matrix is one in which the dot product of the vector
    // representing any column of the matrix with another vector representing
    // any OTHER column of the matrix is equal to zero. If the a column is
    // dotted with itself, the product is not constrained.
    // An orthogonal matrix has the same property on its both rows and columns
    // that a column-orthogonal matrix has on its columns.
    //
    // Given a matrix A of size numRows x numColumns, this routine computes
    // its singular value decomposition, A = U * W * (V^T). The matrix U
    // replaces A on output. The diagonal matrix of singular values W is output
    // as a vector W[1..n]. The matrix V (not the transpose V^T) is output as
    // V[1...n][1...n]
    //
    // Though this routine is based on the numerical recipies implementation,
    // some of the variable names have been changed for readability and the
    // iteration goes from 0 to dim-1, rather than 1 to dim.
    // The algorithm is so complicated to explain that even Numerical Recipies
    // does not explain it, but rather describes a test program to verify
    // the routine and points the reader to a separate linear algebra text.
    // Therefore, I won't even begin to try to understand or explain it.
    //
    // With regard to template parameters:
    // ARRAY2D is any type which can be indexed like this: instance[i][j]
    // to produce a value of type T & or const T &.
    // T is simply the data type. Most often float or double, though
    // could conceivably be a long double or a half.
    // The dummy parameter exists so that we can generically support any
    // ARRAY1D type that fits the bill.
    // Otherwise, we'd have to declare w as "T *", thus losing this generality.
    //
    // NOTE!!! This function is overkill when A is symmetric positive definite.
    //******************************************************************************
    template <class ARRAY2D_1, class ARRAY2D_2, class ARRAY1D, class T>
    void singularValueDecomposition(ARRAY2D_1& A, int M, int N, ARRAY1D& W,
                                    ARRAY2D_2& V, T dummy)
    {
        // Declare constants
        static const int MAX_SVD_ITERS = 30;

        // Declare variables that will be used.
        int i, its, j, p, k, q, nm;
        T anorm, c, f, g, h, s, scale, x, y, z;

        // Create the vector rv1;
        T* rv1 = new T[N];

        //**************************************************************************
        // Householder reduction to bidiagonal form.
        g = scale = anorm = (T)0;
        for (i = 0; i < N; i++)
        {
            q = i + 1;
            rv1[i] = scale * g;
            g = s = scale = (T)0;

            // Technically, this check is not necessary,
            // as all of the blocks internally check the same
            // test, but it prevents that test from being performed
            // over and over.
            if (i < M)
            {
                for (k = i; k < M; k++)
                {
                    scale += Math<T>::abs(A[k][i]);
                }

                if (scale != (T)0)
                {
                    for (k = i; k < M; k++)
                    {
                        A[k][i] /= scale;
                        s += A[k][i] * A[k][i];
                    }
                    f = A[i][i];
                    g = -makeSameSign(Math<T>::sqrt(s), f);
                    h = (f * g) - s;
                    A[i][i] = f - g;

                    for (j = q; j < N; j++)
                    {
                        s = (T)0;
                        for (k = i; k < M; k++)
                        {
                            s += A[k][i] * A[k][j];
                        }
                        f = s / h;
                        for (k = i; k < M; k++)
                        {
                            A[k][j] += f * A[k][i];
                        }
                    }

                    for (k = i; k < M; k++)
                    {
                        A[k][i] *= scale;
                    }
                }
            }

            W[i] = scale * g;
            g = s = scale = (T)0;
            if (i < M && i != (N - 1))
            {
                for (k = q; k < N; k++)
                {
                    scale += Math<T>::abs(A[i][k]);
                }
                if (scale != (T)0)
                {
                    for (k = q; k < N; k++)
                    {
                        A[i][k] /= scale;
                        s += A[i][k] * A[i][k];
                    }
                    f = A[i][q];
                    g = -makeSameSign(Math<T>::sqrt(s), f);
                    h = (f * g) - s;
                    A[i][q] = f - g;
                    for (k = q; k < N; k++)
                    {
                        rv1[k] = A[i][k] / h;
                    }
                    for (j = q; j < M; j++)
                    {
                        s = (T)0;
                        for (k = q; k < N; k++)
                        {
                            s += A[j][k] * A[i][k];
                        }
                        for (k = q; k < N; k++)
                        {
                            A[j][k] += s * rv1[k];
                        }
                    }
                    for (k = q; k < N; k++)
                    {
                        A[i][k] *= scale;
                    }
                }
            }
            anorm = std::max(anorm, Math<T>::abs(W[i]) + Math<T>::abs(rv1[i]));
        }

        //*************************************************************************
        // Accumulation of right-hand transformations
        // Set this "q" explicitly here, to avoid confusion.
        q = N;
        for (i = N - 1; i >= 0; i--)
        {
            // Protect against the first iteration.
            if (i < (N - 1))
            {
                if (g != (T)0)
                {
                    for (j = q; j < N; j++)
                    {
                        V[j][i] = (A[i][j] / A[i][q]) / g;
                    }
                    for (j = q; j < N; j++)
                    {
                        s = (T)0;
                        for (k = q; k < N; k++)
                        {
                            s += A[i][k] * V[k][j];
                        }
                        for (k = q; k < N; k++)
                        {
                            V[k][j] += s * V[k][i];
                        }
                    }
                }

                for (j = q; j < N; j++)
                {
                    V[i][j] = V[j][i] = (T)0;
                }
            }
            V[i][i] = (T)1;
            g = rv1[i];

            // NOTE!!! q updated here.
            q = i;
        }

        //**************************************************************************
        // Accumulation of left-hand transformations
        for (i = std::min(M, N) - 1; i >= 0; i--)
        {
            q = i + 1;
            g = W[i];
            for (j = q; j < N; j++)
            {
                A[i][j] = (T)0;
            }
            if (g != (T)0)
            {
                g = ((T)1) / g;
                for (j = q; j < N; j++)
                {
                    s = (T)0;
                    for (k = q; k < M; k++)
                    {
                        s += A[k][i] * A[k][j];
                    }
                    f = (s / A[i][i]) * g;
                    for (k = i; k < M; k++)
                    {
                        A[k][j] += f * A[k][i];
                    }
                }
                for (j = i; j < M; j++)
                {
                    A[j][i] *= g;
                }
            }
            else
            {
                for (j = i; j < M; j++)
                {
                    A[j][i] = (T)0;
                }
            }
            ++A[i][i];
        }

        //*************************************************************************
        // Diagonalization of the bidiagonal form. Loop over singular values,
        // and over allowed iterations.
        for (k = N - 1; k >= 0; k--)
        {
            for (its = 0; its < MAX_SVD_ITERS; its++)
            {
                // Test for splitting.
                bool splitFlag = true;
                for (q = k; q >= 0; q--)
                {
                    // Note that rv1[0] is always zero,
                    // and thus the break statement will be
                    // called for q = 0. However, to avoid
                    // relying on a potential roundoff snafu,
                    // I'm checking against the blah == 0
                    // case explicitly.
                    nm = q - 1;
                    if (q == 0 || relativeUnderflowTest(rv1[q], anorm))
                    {
                        splitFlag = false;
                        break;
                    }

                    if (relativeUnderflowTest(W[nm], anorm))
                    {
                        break;
                    }
                }

                // Cancellation of rv1[q], if q > 0.
                if (splitFlag)
                {
                    c = (T)0;
                    s = (T)1;
                    for (i = q; i <= k; i++)
                    {
                        f = s * rv1[i];
                        rv1[i] = c * rv1[i];
                        if (relativeUnderflowTest(f, anorm))
                        {
                            break;
                        }
                        g = W[i];
                        h = safeHypot(f, g);
                        W[i] = h;
                        h = ((T)1) / h;
                        c = g * h;
                        s = -f * h;
                        for (j = 0; j < M; j++)
                        {
                            y = A[j][nm];
                            z = A[j][i];
                            A[j][nm] = (y * c) + (z * s);
                            A[j][i] = (z * c) - (y * s);
                        }
                    }
                }

                // Convergence
                z = W[k];
                if (q == k)
                {
                    // Singular value is made non-negative.
                    if (z < (T)0)
                    {
                        W[k] = -z;
                        for (j = 0; j < N; j++)
                        {
                            V[j][k] = -V[j][k];
                        }
                    }
                    break;
                }

                // Check to see if we can't converge.
                if (its == MAX_SVD_ITERS - 1)
                {
                    // Do a cleanup, just in case this is a handle-able
                    // exception in the calling function.
                    delete[] rv1;
                    TWK_EXC_THROW_WHAT(EigenExc,
                                       "SVD: No convergence in max iters");
                }

                // Do some totally uncommented stuff.
                x = W[q];
                nm = k - 1;
                y = W[nm];
                g = rv1[nm];
                h = rv1[k];
                f = ((y - z) * (y + z) + (g - h) * (g + h)) / (((T)2) * h * y);
                g = safeHypot(f, (T)1);
                f = ((x - z) * (x + z)
                     + h * ((y / (f + makeSameSign(g, f))) - h))
                    / x;
                c = s = (T)1;
                for (j = q; j <= nm; j++)
                {
                    i = j + 1;
                    g = rv1[i];
                    y = W[i];
                    h = s * g;
                    g = c * g;
                    z = safeHypot(f, h);
                    rv1[j] = z;
                    c = f / z;
                    s = h / z;
                    f = (x * c) + (g * s);
                    g = (g * c) - (x * s);
                    h = (y * s);
                    y *= c;
                    for (p = 0; p < N; p++)
                    {
                        x = V[p][j];
                        z = V[p][i];
                        V[p][j] = (x * c) + (z * s);
                        V[p][i] = (z * c) - (x * s);
                    }

                    // Rotation can be arbitrary if z == 0
                    z = safeHypot(f, h);
                    W[j] = z;
                    if (z != (T)0)
                    {
                        z = ((T)1) / z;
                        c = f * z;
                        s = h * z;
                    }
                    f = (c * g) + (s * y);
                    x = (c * y) - (s * g);
                    for (p = 0; p < M; p++)
                    {
                        y = A[p][j];
                        z = A[p][i];
                        A[p][j] = (y * c) + (z * s);
                        A[p][i] = (z * c) - (y * s);
                    }
                }
                rv1[q] = (T)0;
                rv1[k] = f;
                W[k] = x;
            }
        }

        delete[] rv1;
    }

    //*****************************************************************************
    // More local functions for use in the "jacobi" routine below.
    // Use unnamed namespace for local functions
    namespace
    {

        template <class T1, class T2>
        inline void jacobiRotate(T1& a0, T1& a1, const T2& s, const T2& tau)
        {
            const T1 g = a0;
            const T1 h = a1;
            a0 = g - (s * (h + (g * tau)));
            a1 = h + (s * (g - (h * tau)));
        }

        template <class T>
        inline bool relativeUnderflowTest2(const T& f, const T& g)
        {
            const double adf = fabs(((double)f));
            const double dg = (double)g;
            const double added = adf + dg;

            const float a = (float)(added);
            const float b = (float)(adf);

            return a == b;
        }

    } // End unnamed namespace

    //*****************************************************************************
    // Much faster way of finding the eigenvalues of small, symmetric positive
    // definite matrices. Eigenvectors too.
    // Computes all eigenvalues and eigenvectors of a real symmetric matrix
    // a[1..n][1..n]. On output, elements of a above the diagonal are destroyed.
    // d[1..n] returns the eigenvalues of a. v[1..n][1..n] is a matrix whose
    // columns contain, on output, the normalized eigenvectors of a. Nrot
    // returns the number of Jacobi rotations that were required.
    //
    // The dummy variable is required for template resolution.

    template <class ARRAY2D_1, class ARRAY2D_2, class ARRAY1D, class T>
    void jacobiEigenSolve(ARRAY2D_1& a, int N, ARRAY1D& d, ARRAY2D_2& v,
                          int& numRotations, T dummy)
    {
        // Declare constants
        static const int MAX_JACOBI_ITERS = 50;

        // Declare variables that will be used.
        int j, iq, ip;
        float threshold, theta, tau, t, sum, s, h, g, c;

        // Allocate two working arrays
        T* b = new T[N];
        T* z = new T[N];

        // Set eigenvector matrix to identity.
        // Not using a matrix form to do this.
        for (ip = 0; ip < N; ip++)
        {
            for (iq = 0; iq < N; iq++)
            {
                v[ip][iq] = (T)0;
            }
            v[ip][ip] = (T)1;
        }

        // Set b & d temporary vectors to the
        // diagonal elements of a. If this matrix
        // is a variance-covariance matrix, these
        // elements will be the variance of each
        // variable.
        // The z vector will accumulate terms.
        for (ip = 0; ip < N; ip++)
        {
            b[ip] = d[ip] = a[ip][ip];
            z[ip] = (T)0;
        }

        // Iterate!
        for (numRotations = 0; numRotations < MAX_JACOBI_ITERS; numRotations++)
        {
            // Sum the off-diagonal elements.
            sum = (T)0;
            for (ip = 0; ip < N - 1; ip++)
            {
                for (iq = ip + 1; iq < N; iq++)
                {
                    sum += Math<T>::abs(a[ip][iq]);
                }
            }

            // Might want to do a "divisor too small"
            // check on sum.
            // This normal return relies on quadratic
            // convergence to machine underflow.
            // It also seems to assume that underflow
            // is always set to zero, which is not
            // necessarily true.
            // CJH try this:
            if (sum == (T)0)
            {
                delete[] b;
                delete[] z;
                return;
            }

            // For the first three rotations,
            // carry out the rotation only in certain
            // circumstances
            if (numRotations < 3)
            {
                // CJH HELLO MAGIC NUMBER
                threshold = ((T)0.2) * sum / (N * N);
            }
            else
            {
                threshold = (T)0;
            }

            // Again working on off-diagonal elements.
            for (ip = 0; ip < N - 1; ip++)
            {
                for (iq = ip + 1; iq < N; iq++)
                {
                    // CJH MAGIC NUMBER #2.
                    g = ((T)100) * Math<T>::abs(a[ip][iq]);

                    // More of this weird round-off error stuff.
                    // After four sweeps, skip the rotation if the
                    // off-diagonal element is small.
                    if (numRotations > 3 && relativeUnderflowTest2(d[ip], (T)g)
                        && relativeUnderflowTest2(d[iq], (T)g))
                    {
                        a[ip][iq] = (T)0;
                    }
                    else if (Math<T>::abs(a[ip][iq]) > threshold)
                    {
                        h = d[iq] - d[ip];
                        if (relativeUnderflowTest2(h, g))
                        {
                            t = (a[ip][iq]) / h;
                        }
                        else
                        {
                            theta = ((T)0.5) * h / (a[ip][iq]);
                            t = ((T)1)
                                / (Math<T>::abs(theta)
                                   + Math<T>::sqrt(((T)1) + (theta * theta)));
                            if (theta < (T)0)
                            {
                                t = -t;
                            }
                        }
                        c = ((T)1) / Math<T>::sqrt(((T)1) + (t * t));
                        s = t * c;
                        tau = s / (((T)1) + c);
                        h = t * a[ip][iq];
                        z[ip] -= h;
                        z[iq] += h;
                        d[ip] -= h;
                        d[iq] += h;
                        a[ip][iq] = (T)0;

                        // Case of rotations 0 <= j < p
                        for (j = 0; j <= ip - 1; j++)
                        {
                            jacobiRotate(a[j][ip], a[j][iq], (T)s, (T)tau);
                        }

                        // Case of rotations p < j < q
                        for (j = ip + 1; j <= iq - 1; j++)
                        {
                            jacobiRotate(a[ip][j], a[j][iq], (T)s, (T)tau);
                        }

                        // Case of rotations q < j < N
                        for (j = iq + 1; j < N; j++)
                        {
                            jacobiRotate(a[ip][j], a[iq][j], (T)s, (T)tau);
                        }

                        for (j = 0; j < N; j++)
                        {
                            jacobiRotate(v[j][ip], v[j][iq], (T)s, (T)tau);
                        }
                    }
                }
            }

            // Update d with the sum of t * apq, and reinitialize z.
            for (ip = 0; ip < N; ip++)
            {
                b[ip] += z[ip];
                d[ip] = b[ip];
                z[ip] = (T)0;
            }
        }

        //*************************************************************************
        // If we get here, we iterated too much.
        // Clean up, just in case routine can be properly exited.
        delete[] b;
        delete[] z;

        TWK_EXC_THROW_WHAT(EigenExc, "Jacobi: too many iterations");
    }

    //******************************************************************************
    //*****************************************************************************
    // FUNCTION TO SORT EIGENVECTORS AND VALUES
    // Not necessarily optimal.
    //******************************************************************************
    //******************************************************************************
    namespace
    {

        //******************************************************************************
        template <class T> struct IndexValuePair
        {
            IndexValuePair()
                : index(-1)
                , value(Math<T>::min())
            {
                // Nothing
            }

            IndexValuePair(int idx, const T& val)
                : index(idx)
                , value(val)
            {
                // Nothing
            }

            IndexValuePair(const IndexValuePair<T>& ivp)
                : index(ivp.index)
                , value(ivp.value)
            {
                // Nothing
            }

            IndexValuePair<T>& operator=(const IndexValuePair<T>& rhs)
            {
                index = rhs.index;
                value = rhs.value;
                return *this;
            }

            int index;
            T value;
        };

        //******************************************************************************
        template <class T> struct IvpComp
        {
            bool operator()(const IndexValuePair<T>& lhs,
                            const IndexValuePair<T>& rhs) const
            {
                return lhs.value > rhs.value;
            }
        };

    } // End unnamed namespace

    //******************************************************************************
    template <class ARRAY2D, class ARRAY1D, class T>
    void eigenSort(ARRAY1D& eigenValues, ARRAY2D& eigenVectorsTransposed,
                   int dimension, T dummy)
    {
        // Build a list of pairs.
        IndexValuePair<T>* pairs = new IndexValuePair<T>[dimension];
        for (int i = 0; i < dimension; i++)
        {
            pairs[i].index = i;
            pairs[i].value = eigenValues[i];
        }

        // Sort the pairs
        std::sort(pairs, pairs + dimension, IvpComp<T>());

        // Build temporary matrix.
        T* tempMat = new T[dimension * dimension];

        // Loop over vectors, copying sorted vectors into temp mat.
        T* tempMatIter = tempMat;
        int row;
        int vec;
        for (vec = 0; vec < dimension; vec++)
        {
            // Copy the value.
            eigenValues[vec] = pairs[vec].value;

            // Copy vector into the temp matrix.
            for (row = 0; row < dimension; row++, tempMatIter++)
            {
                (*tempMatIter) = eigenVectorsTransposed[row][pairs[vec].index];
            }
        }

        // Copy from temp mat back into eigen vector matrix.
        tempMatIter = tempMat;
        for (vec = 0; vec < dimension; vec++)
        {
            for (row = 0; row < dimension; row++, tempMatIter++)
            {
                eigenVectorsTransposed[row][vec] = (*tempMatIter);
            }
        }

        // Delete temporary arrays
        delete[] pairs;
        delete[] tempMat;
    }

} // End namespace TwkMath

#endif
