//******************************************************************************
// Copyright (c) 2001-2002 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkMathMat22_h_
#define _TwkMathMat22_h_

#include <TwkMath/MatrixCommon.h>
#include <TwkMath/Vec2.h>
#include <sys/types.h>
#include <assert.h>

#define T0 ((T)0)
#define T1 ((T)1)

namespace TwkMath
{

    //******************************************************************************
    // SQUARE MATRIX, SIZE 2x2
    template <typename T> class Mat22
    {
    public:
        //**************************************************************************
        // TYPEDEFS
        //**************************************************************************
        typedef T value_type;
        typedef size_t size_type;

        //**************************************************************************
        // DATA
        //**************************************************************************
#ifdef COMPILER_GCC2
        T m00;
        T m01;
        T m10;
        T m11;
#else
        union
        {
            T m[2][2];

            struct
            {
                T m00;
                T m01;
                T m10;
                T m11;
            };
        };
#endif

        //**************************************************************************
        // CONSTRUCTORS
        //**************************************************************************
        // Default Constructor.
        // Makes matrix into identity.
        Mat22()
            : m00(T1)
            , m01(T0)
            , m10(T0)
            , m11(T1)
        {
        }

        // Constructor for use when no operations are desired
        explicit Mat22(void* dummy) {}

        // Single valued constructor. Initializes all elements to value.
        explicit Mat22(const T& val)
            : m00(val)
            , m01(val)
            , m10(val)
            , m11(val)
        {
        }

        // Direct constructor. Initializes elements directly
        Mat22(const T& v00, const T& v01, const T& v10, const T& v11)
            : m00(v00)
            , m01(v01)
            , m10(v10)
            , m11(v11)
        {
        }

        // Another direct constructor, using an array
        // Values are copied in row-major form.
        explicit Mat22(const T array[])
            : m00(array[0])
            , m01(array[1])
            , m10(array[2])
            , m11(array[3])
        {
        }

        // Copy constructor from a Mat22 of a different type.
        // Non explicit, so automatic promotion may occur.
        template <typename T2>
        Mat22(const Mat22<T2>& copy)
            : m00(copy.m00)
            , m01(copy.m01)
            , m10(copy.m10)
            , m11(copy.m11)
        {
        }

        // Copy constructor from a Mat22 of same type.
        // Specialization of above function
        Mat22(const Mat22<T>& copy)
            : m00(copy.m00)
            , m01(copy.m01)
            , m10(copy.m10)
            , m11(copy.m11)
        {
        }

        //**************************************************************************
        // ASSIGNMENT OPERATORS
        //**************************************************************************
        // Single value
        Mat22<T>& operator=(const T& val)
        {
            m00 = val;
            m01 = val;
            m10 = val;
            m11 = val;
            return *this;
        }

        // Assignment from a Mat22 of a different type
        template <typename T2> Mat22<T>& operator=(const Mat22<T2>& copy)
        {
            m00 = copy.m00;
            m01 = copy.m01;
            m10 = copy.m10;
            m11 = copy.m11;
            return *this;
        }

        // Assignment from a Mat22 of the same type.
        // Specialization of above function
        Mat22<T>& operator=(const Mat22<T>& copy)
        {
            m00 = copy.m00;
            m01 = copy.m01;
            m10 = copy.m10;
            m11 = copy.m11;
            return *this;
        }

        //**************************************************************************
        // DIMENSION FUNCTION
        //**************************************************************************
        static size_type rowDimension() { return 2; }

        static size_type colDimension() { return 2; }

        //**************************************************************************
        // DATA ACCESS OPERATORS
        //**************************************************************************
        const T* operator[](size_type rowIndex) const;
        T* operator[](size_type rowIndex);
        const T& operator()(size_type row, size_type col) const;
        T& operator()(size_type row, size_type col);

        void set(const T& v00, const T& v01, const T& v10, const T& v11)
        {
            m00 = v00;
            m01 = v01;
            m10 = v10;
            m11 = v11;
        }

        //**************************************************************************
        // MATRIX OPERATIONS
        //**************************************************************************
        // Create identity matrix
        void makeIdentity()
        {
            m00 = T1;
            m01 = T0;
            m10 = T0;
            m11 = T1;
        }

        // transpose the matrix
        void transpose();

        // Return a copy of the matrix, transposed.
        Mat22<T> transposed() const;

        // Return the determinant of the matrix.
        T determinant() const;

        // Invert the matrix. Will throw TwkMath::SingularMatrixExc
        // if the inversion is not possible.
        void invert();

        // Return a copy of the matrix, inverted.
        // Will throw TwkMath::SingularMatrixExc if
        // the inversion is not possible.
        Mat22<T> inverted() const;

        //**************************************************************************
        // ARITHMETIC OPERATORS
        //**************************************************************************
        Mat22<T>& operator+=(const Mat22<T>& v);
        Mat22<T>& operator-=(const Mat22<T>& v);
        Mat22<T>& operator*=(const T& val);
        Mat22<T>& operator*=(const Mat22<T>& v);
        Mat22<T>& operator/=(const T& val);
    };

    //******************************************************************************
    // TYPEDEFS
    //******************************************************************************
    typedef Mat22<float> Mat22f;
    typedef Mat22<double> Mat22d;

    //******************************************************************************
    // TEMPLATE AND INLINE FUNCTIONS
    //******************************************************************************
    template <typename T>
    inline const T*
    Mat22<T>::operator[](typename Mat22<T>::size_type rowIndex) const
    {
        assert(rowIndex < 2);
#ifdef COMPILER_GCC2
        return ((const T*)this) + (rowIndex * 2);
#else
        return (const T*)(m[rowIndex]);
#endif
    }

    //******************************************************************************
    template <typename T>
    inline T* Mat22<T>::operator[](typename Mat22<T>::size_type rowIndex)
    {
        assert(rowIndex < 2);
#ifdef COMPILER_GCC2
        return ((T*)this) + (rowIndex * 2);
#else
        return (T*)(m[rowIndex]);
#endif
    }

    //******************************************************************************
    template <typename T>
    inline const T&
    Mat22<T>::operator()(typename Mat22<T>::size_type rowIndex,
                         typename Mat22<T>::size_type colIndex) const
    {
        assert(rowIndex < 2);
        assert(colIndex < 2);
#ifdef COMPILER_GCC2
        return ((const T*)this)[(rowIndex * 2) + colIndex];
#else
        return m[rowIndex][colIndex];
#endif
    }

    //******************************************************************************
    template <typename T>
    inline T& Mat22<T>::operator()(typename Mat22<T>::size_type rowIndex,
                                   typename Mat22<T>::size_type colIndex)
    {
        assert(rowIndex < 2);
        assert(colIndex < 2);
#ifdef COMPILER_GCC2
        return ((const T*)this)[(rowIndex * 2) + colIndex];
#else
        return m[rowIndex][colIndex];
#endif
    }

    //******************************************************************************
    template <typename T> inline void Mat22<T>::transpose()
    {
        const T tmp = m01;
        m01 = m10;
        m10 = tmp;
    }

    //******************************************************************************
    template <typename T> inline Mat22<T> Mat22<T>::transposed() const
    {
        return Mat22<T>(m00, m10, m01, m11);
    }

    //******************************************************************************
    template <typename T> inline T Mat22<T>::determinant() const
    {
        return determinant2x2(m00, m01, m10, m11);
    }

    //******************************************************************************
    template <typename T> inline void Mat22<T>::invert()
    {
        invert2x2(m00, m01, m10, m11);
    }

    //******************************************************************************
    template <typename T> inline Mat22<T> Mat22<T>::inverted() const
    {
        Mat22<T> ret(*this);
        ret.invert();
        return ret;
    }

    //******************************************************************************
    template <typename T>
    inline Mat22<T>& Mat22<T>::operator+=(const Mat22<T>& v)
    {
        m00 += v.m00;
        m01 += v.m01;
        m10 += v.m10;
        m11 += v.m11;
        return *this;
    }

    //******************************************************************************
    template <typename T>
    inline Mat22<T>& Mat22<T>::operator-=(const Mat22<T>& v)
    {
        m00 -= v.m00;
        m01 -= v.m01;
        m10 -= v.m10;
        m11 -= v.m11;
        return *this;
    }

    //******************************************************************************
    template <typename T> inline Mat22<T>& Mat22<T>::operator*=(const T& v)
    {
        m00 *= v;
        m01 *= v;
        m10 *= v;
        m11 *= v;
        return *this;
    }

    //******************************************************************************
    template <typename T> Mat22<T>& Mat22<T>::operator*=(const Mat22<T>& v)
    {
        const T v00 = (m00 * v.m00) + (m01 * v.m10);
        const T v01 = (m00 * v.m01) + (m01 * v.m11);
        const T v10 = (m10 * v.m00) + (m11 * v.m10);
        const T v11 = (m10 * v.m01) + (m11 * v.m11);
        m00 = v00;
        m01 = v01;
        m10 = v10;
        m11 = v11;
        return *this;
    }

    //******************************************************************************
    template <typename T> inline Mat22<T>& Mat22<T>::operator/=(const T& v)
    {
        assert(v != T0);
        m00 /= v;
        m01 /= v;
        m10 /= v;
        m11 /= v;
        return *this;
    }

    //******************************************************************************
    //******************************************************************************
    // FUNCTIONS WHICH OPERATE ON MAT22
    //******************************************************************************
    //******************************************************************************
    template <typename T>
    inline typename Mat22<T>::size_type rowDimension(const Mat22<T>&)
    {
        return 2;
    }

    //******************************************************************************
    template <typename T>
    inline typename Mat22<T>::size_type colDimension(const Mat22<T>&)
    {
        return 2;
    }

    //******************************************************************************
    template <typename T> inline void makeIdentity(Mat22<T>& m)
    {
        m.makeIdentity();
    }

    //******************************************************************************
    template <typename T> inline void transpose(Mat22<T>& m) { m.transpose(); }

    //******************************************************************************
    template <typename T> inline Mat22<T> transposed(const Mat22<T>& m)
    {
        return m.transposed();
    }

    //******************************************************************************
    template <typename T> inline T determinant(const Mat22<T>& m)
    {
        return m.determinant();
    }

    //******************************************************************************
    template <typename T> inline void invert(Mat22<T>& m) { m.invert(); }

    //******************************************************************************
    template <typename T> inline Mat22<T> inverted(const Mat22<T>& m)
    {
        return m.inverted();
    }

    //******************************************************************************
    //******************************************************************************
    // ARITHMETIC OPERATORS
    //******************************************************************************
    //******************************************************************************
    // ADDITION
    // Matrices can only be added to other matrices of the same size.
    // No scalars, no vectors.
    template <typename T>
    inline Mat22<T> operator+(const Mat22<T>& a, const Mat22<T>& b)
    {
        return Mat22<T>(a.m00 + b.m00, a.m01 + b.m01, a.m10 + b.m10,
                        a.m11 + b.m11);
    }

    //******************************************************************************
    // NEGATION
    template <typename T> inline Mat22<T> operator-(const Mat22<T>& a)
    {
        return Mat22<T>(-a.m00, -a.m01, -a.m10, -a.m11);
    }

    //******************************************************************************
    // SUBTRACTION
    // Matrices can only be subtracted from other matrices of the same size.
    // No scalars, no vectors.
    template <typename T>
    inline Mat22<T> operator-(const Mat22<T>& a, const Mat22<T>& b)
    {
        return Mat22<T>(a.m00 - b.m00, a.m01 - b.m01, a.m10 - b.m10,
                        a.m11 - b.m11);
    }

    //******************************************************************************
    // SCALAR MULTIPLICATION
    // Matrices can be left and right multiplied by scalars, both of which
    // produce the same effect of multiplying all elements of the matrix.
    template <typename T>
    inline Mat22<T> operator*(const Mat22<T>& a, const T& b)
    {
        return Mat22<T>(a.m00 * b, a.m01 * b, a.m10 * b, a.m11 * b);
    }

    //******************************************************************************
    template <typename T>
    inline Mat22<T> operator*(const T& a, const Mat22<T>& b)
    {
        return Mat22<T>(a * b.m00, a * b.m01, a * b.m10, a * b.m11);
    }

    //******************************************************************************
    // VECTOR MULTIPLICATION
    // Matrices can be left multiplied by a vector, which results in a vector.
    // Matrices can be right multiplied by a vector, which results in a vector.
    template <typename T>
    inline Vec2<T> operator*(const Vec2<T>& v, const Mat22<T>& m)
    {
        return Vec2<T>((v.x * m.m00) + (v.y * m.m10),
                       (v.x * m.m01) + (v.y * m.m11));
    }

    //******************************************************************************
    template <typename T>
    inline Vec2<T> operator*(const Mat22<T>& m, const Vec2<T>& v)
    {
        return Vec2<T>((m.m00 * v.x) + (m.m01 * v.y),
                       (m.m10 * v.x) + (m.m11 * v.y));
    }

    //******************************************************************************
    // MATRIX MULTIPLICATION
    template <typename T>
    inline Mat22<T> operator*(const Mat22<T>& a, const Mat22<T>& b)
    {
        return Mat22<T>((a.m00 * b.m00) + (a.m01 * b.m10),
                        (a.m00 * b.m01) + (a.m01 * b.m11),
                        (a.m10 * b.m00) + (a.m11 * b.m10),
                        (a.m10 * b.m01) + (a.m11 * b.m11));
    }

    //******************************************************************************
    // DIVISION
    // Matrices can only be divided by scalar.
    // Scalars cannot be divided by matrices.
    template <typename T>
    inline Mat22<T> operator/(const Mat22<T>& a, const T& b)
    {
        assert(b != T0);
        return Mat22<T>(a.m00 / b, a.m01 / b, a.m10 / b, a.m11 / b);
    }

    //******************************************************************************
    //******************************************************************************
    // COMPARISON OPERATORS
    //******************************************************************************
    //******************************************************************************
    template <typename T>
    inline bool operator==(const Mat22<T>& a, const Mat22<T>& b)
    {
        return ((a.m00 == b.m00) && (a.m01 == b.m01) && (a.m10 == b.m10)
                && (a.m11 == b.m11));
    }

    //******************************************************************************
    template <typename T>
    inline bool operator!=(const Mat22<T>& a, const Mat22<T>& b)
    {
        return ((a.m00 != b.m00) || (a.m01 != b.m01) || (a.m10 != b.m10)
                || (a.m11 != b.m11));
    }

} // End namespace TwkMath

#undef T0
#undef T1

#endif
