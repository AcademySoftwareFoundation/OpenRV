//----------------------------------------------------------------------
//
// Copyright (c) 2001-2002 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//

#ifndef _TwkMathMat44_h_
#define _TwkMathMat44_h_

#include <TwkMath/MatrixCommon.h>
#include <TwkMath/MatrixAlgo.h>
#include <TwkMath/Vec3.h>
#include <TwkMath/Vec4.h>
#include <sys/types.h>
#include <assert.h>

#define T0 ((T)0)
#define T1 ((T)1)

namespace TwkMath
{

    // SQUARE MATRIX, SIZE 4x4
    template <typename T> class Mat44
    {
    public:
        //
        // TYPEDEFS
        //
        typedef T value_type;
        typedef size_t size_type;

        //
        // DATA
        //

#ifdef COMPILER_GCC2
        T m00;
        T m01;
        T m02;
        T m03;
        T m10;
        T m11;
        T m12;
        T m13;
        T m20;
        T m21;
        T m22;
        T m23;
        T m30;
        T m31;
        T m32;
        T m33;
#else
        union
        {
            T m[4][4];

            struct
            {
                T m00;
                T m01;
                T m02;
                T m03;
                T m10;
                T m11;
                T m12;
                T m13;
                T m20;
                T m21;
                T m22;
                T m23;
                T m30;
                T m31;
                T m32;
                T m33;
            };
        };
#endif

        //**************************************************************************
        // CONSTRUCTORS
        //**************************************************************************
        // Default Constructor.
        // Makes matrix into identity.
        Mat44()
            : m00(T1)
            , m01(T0)
            , m02(T0)
            , m03(T0)
            , m10(T0)
            , m11(T1)
            , m12(T0)
            , m13(T0)
            , m20(T0)
            , m21(T0)
            , m22(T1)
            , m23(T0)
            , m30(T0)
            , m31(T0)
            , m32(T0)
            , m33(T1)
        {
        }

        // Constructor for use when no operations are desired
        explicit Mat44(void* dummy) {}

        // Single valued constructor. Initializes all elements to value.
        explicit Mat44(const T& val)
            : m00(val)
            , m01(val)
            , m02(val)
            , m03(val)
            , m10(val)
            , m11(val)
            , m12(val)
            , m13(val)
            , m20(val)
            , m21(val)
            , m22(val)
            , m23(val)
            , m30(val)
            , m31(val)
            , m32(val)
            , m33(val)
        {
        }

        // Direct constructor. Initializes elements directly
        Mat44(const T& v00, const T& v01, const T& v02, const T& v03,
              const T& v10, const T& v11, const T& v12, const T& v13,
              const T& v20, const T& v21, const T& v22, const T& v23,
              const T& v30, const T& v31, const T& v32, const T& v33)
            : m00(v00)
            , m01(v01)
            , m02(v02)
            , m03(v03)
            , m10(v10)
            , m11(v11)
            , m12(v12)
            , m13(v13)
            , m20(v20)
            , m21(v21)
            , m22(v22)
            , m23(v23)
            , m30(v30)
            , m31(v31)
            , m32(v32)
            , m33(v33)
        {
        }

        // Another direct constructor, using an array
        // Values are copied in row-major form.
        explicit Mat44(const T array[])
            : m00(array[0])
            , m01(array[1])
            , m02(array[2])
            , m03(array[3])
            , m10(array[4])
            , m11(array[5])
            , m12(array[6])
            , m13(array[7])
            , m20(array[8])
            , m21(array[9])
            , m22(array[10])
            , m23(array[11])
            , m30(array[12])
            , m31(array[13])
            , m32(array[14])
            , m33(array[15])
        {
        }

        // Another direct constructor, using a two dimensional array
        // Values are copied in row-major form.
        explicit Mat44(const T array[4][4])
            : m00(array[0][0])
            , m01(array[0][1])
            , m02(array[0][2])
            , m03(array[0][3])
            , m10(array[1][0])
            , m11(array[1][1])
            , m12(array[1][2])
            , m13(array[1][3])
            , m20(array[2][0])
            , m21(array[2][1])
            , m22(array[2][2])
            , m23(array[2][3])
            , m30(array[3][0])
            , m31(array[3][1])
            , m32(array[3][2])
            , m33(array[3][3])
        {
        }

        // Copy constructor from a Mat44 of a different type.
        // Non explicit, so automatic promotion may occur.
        template <typename T2>
        Mat44(const Mat44<T2>& copy)
            : m00(copy.m00)
            , m01(copy.m01)
            , m02(copy.m02)
            , m03(copy.m03)
            , m10(copy.m10)
            , m11(copy.m11)
            , m12(copy.m12)
            , m13(copy.m13)
            , m20(copy.m20)
            , m21(copy.m21)
            , m22(copy.m22)
            , m23(copy.m23)
            , m30(copy.m30)
            , m31(copy.m31)
            , m32(copy.m32)
            , m33(copy.m33)
        {
        }

        // Copy constructor from a Mat44 of same type.
        // Specialization of above function
        Mat44(const Mat44<T>& copy)
            : m00(copy.m00)
            , m01(copy.m01)
            , m02(copy.m02)
            , m03(copy.m03)
            , m10(copy.m10)
            , m11(copy.m11)
            , m12(copy.m12)
            , m13(copy.m13)
            , m20(copy.m20)
            , m21(copy.m21)
            , m22(copy.m22)
            , m23(copy.m23)
            , m30(copy.m30)
            , m31(copy.m31)
            , m32(copy.m32)
            , m33(copy.m33)
        {
        }

        //**************************************************************************
        // ASSIGNMENT OPERATORS
        //**************************************************************************
        // Single value
        Mat44<T>& operator=(const T& val)
        {
            m00 = val;
            m01 = val;
            m02 = val;
            m03 = val;
            m10 = val;
            m11 = val;
            m12 = val;
            m13 = val;
            m20 = val;
            m21 = val;
            m22 = val;
            m23 = val;
            m30 = val;
            m31 = val;
            m32 = val;
            m33 = val;
            return *this;
        }

        // Assignment from a Mat44 of the same type.
        // Specialization of above function
        template <typename S> Mat44<T>& operator=(const Mat44<S>& copy)
        {
            m00 = copy.m00;
            m01 = copy.m01;
            m02 = copy.m02;
            m03 = copy.m03;
            m10 = copy.m10;
            m11 = copy.m11;
            m12 = copy.m12;
            m13 = copy.m13;
            m20 = copy.m20;
            m21 = copy.m21;
            m22 = copy.m22;
            m23 = copy.m23;
            m30 = copy.m30;
            m31 = copy.m31;
            m32 = copy.m32;
            m33 = copy.m33;
            return *this;
        }

        //**************************************************************************
        // DIMENSION FUNCTION
        //**************************************************************************
        static size_type rowDimension() { return 4; }

        static size_type colDimension() { return 4; }

        //**************************************************************************
        // DATA ACCESS OPERATORS
        //**************************************************************************
        const T* operator[](size_type rowIndex) const;
        T* operator[](size_type rowIndex);
        const T& operator()(size_type row, size_type col) const;
        T& operator()(size_type row, size_type col);

        template <typename S>
        void set(const S& v00, const S& v01, const S& v02, const S& v03,
                 const S& v10, const S& v11, const S& v12, const S& v13,
                 const S& v20, const S& v21, const S& v22, const S& v23,
                 const S& v30, const S& v31, const S& v32, const S& v33)
        {
            m00 = v00;
            m01 = v01;
            m02 = v02;
            m03 = v03;
            m10 = v10;
            m11 = v11;
            m12 = v12;
            m13 = v13;
            m20 = v20;
            m21 = v21;
            m22 = v22;
            m23 = v23;
            m30 = v30;
            m31 = v31;
            m32 = v32;
            m33 = v33;
        }

        //**************************************************************************
        // MATRIX OPERATIONS
        //**************************************************************************
        // Create identity matrix
        void makeIdentity()
        {
            m00 = T1;
            m01 = T0;
            m02 = T0;
            m03 = T0;
            m10 = T0;
            m11 = T1;
            m12 = T0;
            m13 = T0;
            m20 = T0;
            m21 = T0;
            m22 = T1;
            m23 = T0;
            m30 = T0;
            m31 = T0;
            m32 = T0;
            m33 = T1;
        }

        // transpose the matrix
        void transpose();

        // Return a copy of the matrix, transposed.
        Mat44<T> transposed() const;

        // Retrun the determinant of the matrix.
        T determinant() const;

        // Invert the matrix. Will throw TwkMath::SingularMatrixExc
        // if the inversion is not possible.
        void invert();

        // Return a copy of the matrix, inverted.
        // Will throw TwkMath::SingularMatrixExc if
        // the inversion is not possible.
        Mat44<T> inverted() const;

        //**************************************************************************
        // HOMOGENEOUS GEOMETRIC OPERATIONS
        //**************************************************************************
        // These are in row major form (column is the quick iterator)

        // Is the matrix affine or not? (Do parallel lines remain parallel
        // after being transformed by the matrix)
        bool isAffine() const
        {
            return ((m33 == T1) && (m32 == T0) && (m31 == T0) && (m20 == T0));
        }

        // Create a translation matrix.
        template <typename S> void makeTranslation(const Vec3<S>& t)
        {
            m00 = T1;
            m01 = T0;
            m02 = T0;
            m03 = t.x;
            m10 = T0;
            m11 = T1;
            m12 = T0;
            m13 = t.y;
            m20 = T0;
            m21 = T0;
            m22 = T1;
            m23 = t.z;
            m30 = T0;
            m31 = T0;
            m32 = T0;
            m33 = T1;
        }

        // Modify translations of matrix
        template <typename S> void setTranslation(const Vec3<S>& t)
        {
            m03 = t.x;
            m13 = t.y;
            m23 = t.z;
        }

        template <typename S> void addTranslation(const Vec3<S>& t)
        {
            m03 += t.x;
            m13 += t.y;
            m23 += t.z;
        }

        // Make a scale matrix
        template <typename S> void makeScale(const Vec3<S>& t)
        {
            m00 = t.x;
            m01 = T0;
            m02 = T0;
            m03 = T0;
            m10 = T0;
            m11 = t.y;
            m12 = T0;
            m13 = T0;
            m20 = T0;
            m21 = T0;
            m22 = t.z;
            m23 = T0;
            m30 = T0;
            m31 = T0;
            m32 = T0;
            m33 = T1;
        }

        // Create rotation matrix
        template <typename S>
        void makeRotation(const Vec3<S>& axis, const T& angleInRadians);

        // Transform a homogeneous vector by the matrix
        template <typename S> Vec3<S> transform(const Vec3<S>& v) const;

        // Transform without translation the homogeneous vector by the matrix.
        // This only works with affine matrices because the homogeneous
        // coordinate is ignored.
        template <typename S> Vec3<S> transformDir(const Vec3<S>& v) const;

        //**************************************************************************
        // ARITHMETIC OPERATORS
        //**************************************************************************
        template <typename S> Mat44<T>& operator+=(const Mat44<S>& v);
        template <typename S> Mat44<T>& operator-=(const Mat44<S>& v);
        template <typename S> Mat44<T>& operator*=(const S& val);
        template <typename S> Mat44<T>& operator*=(const Mat44<S>& v);
        template <typename S> Mat44<T>& operator/=(const S& val);
    };

    // TYPEDEFS

    typedef Mat44<float> Mat44f;
    typedef Mat44<double> Mat44d;

    // TEMPLATE AND INLINE FUNCTIONS

    template <typename T>
    inline const T*
    Mat44<T>::operator[](typename Mat44<T>::size_type rowIndex) const
    {
        assert(rowIndex < 4);
#ifdef COMPILER_GCC2
        return ((const T*)this) + (rowIndex * 4);
#else
        return (const T*)(m[rowIndex]);
#endif
    }

    template <typename T>
    inline T* Mat44<T>::operator[](typename Mat44<T>::size_type rowIndex)
    {
        assert(rowIndex < 4);
#ifdef COMPILER_GCC2
        return ((T*)this) + (rowIndex * 4);
#else
        return (T*)(m[rowIndex]);
#endif
    }

    template <typename T>
    inline const T&
    Mat44<T>::operator()(typename Mat44<T>::size_type rowIndex,
                         typename Mat44<T>::size_type colIndex) const
    {
        assert(rowIndex < 4);
        assert(colIndex < 4);
#ifdef COMPILER_GCC2
        return ((T*)this)[(rowIndex * 4) + colIndex];
#else
        return m[rowIndex][colIndex];
#endif
    }

    template <typename T>
    inline T& Mat44<T>::operator()(typename Mat44<T>::size_type rowIndex,
                                   typename Mat44<T>::size_type colIndex)
    {
        assert(rowIndex < 4);
        assert(colIndex < 4);
#ifdef COMPILER_GCC2
        return ((T*)this)[(rowIndex * 4) + colIndex];
#else
        return m[rowIndex][colIndex];
#endif
    }

    template <typename T> inline void Mat44<T>::transpose()
    {
        T tmp = m01;
        m01 = m10;
        m10 = tmp;

        tmp = m02;
        m02 = m20;
        m20 = tmp;

        tmp = m03;
        m03 = m30;
        m30 = tmp;

        tmp = m12;
        m12 = m21;
        m21 = tmp;

        tmp = m13;
        m13 = m31;
        m31 = tmp;

        tmp = m23;
        m23 = m32;
        m32 = tmp;
    }

    template <typename T> inline Mat44<T> Mat44<T>::transposed() const
    {
        return Mat44<T>(m00, m10, m20, m30, m01, m11, m21, m31, m02, m12, m22,
                        m32, m03, m13, m23, m33);
    }

    template <typename T> T Mat44<T>::determinant() const
    {
        return determinant4x4(m00, m01, m02, m03, m10, m11, m12, m13, m20, m21,
                              m22, m23, m30, m31, m32, m33);
    }

    template <typename T> void Mat44<T>::invert()
    {
        // Check for affine case, which is extremely common.
        if (isAffine())
        {
            // Matrix is affine. Remove the translation, invert.
            const T tx = m03;
            const T ty = m13;
            const T tz = m23;

            // Invert upper left corner
            invert3x3(m00, m01, m02, m10, m11, m12, m20, m21, m22);

            // Now do translation side.
            m03 = -((m00 * tx) + (m01 * ty) + (m02 * tz));
            m13 = -((m10 * tx) + (m11 * ty) + (m12 * tz));
            m23 = -((m20 * tx) + (m21 * ty) + (m22 * tz));
        }
        else
        {
            gaussjInvert<T, Mat44<T>, 4>::doit(*this);
        }
    }

    template <typename T> inline Mat44<T> Mat44<T>::inverted() const
    {
        Mat44<T> ret(*this);
        ret.invert();
        return ret;
    }

    template <typename T>
    template <typename S>
    void Mat44<T>::makeRotation(const Vec3<S>& axis, const T& ang)
    {
        const Vec3<T> naxis(axis.normalized());
        const T cosAngle = Math<T>::cos(ang);
        const T sinAngle = Math<T>::sin(ang);

        m00 = naxis[0] * naxis[0];
        m01 = m10 = naxis[0] * naxis[1];
        m02 = m20 = naxis[0] * naxis[2];
        m03 = m30 = T0;
        m11 = naxis[1] * naxis[1];
        m12 = m21 = naxis[1] * naxis[2];
        m13 = m31 = T0;
        m22 = naxis[2] * naxis[2];
        m23 = m32 = T0;
        m33 = T1;

        m00 += cosAngle * (T1 - m00);
        m01 += cosAngle * (T0 - m01) + (-naxis.z * sinAngle);
        m02 += cosAngle * (T0 - m02) + (naxis.y * sinAngle);

        m10 += cosAngle * (T0 - m10) + (naxis.z * sinAngle);
        m11 += cosAngle * (T1 - m11);
        m12 += cosAngle * (T0 - m12) + (-naxis.x * sinAngle);

        m20 += cosAngle * (T0 - m20) + (-naxis.y * sinAngle);
        m21 += cosAngle * (T0 - m21) + (naxis.x * sinAngle);
        m22 += cosAngle * (T1 - m22);
    }

    template <typename T>
    template <typename S>
    Vec3<S> Mat44<T>::transform(const Vec3<S>& v) const
    {
        if (isAffine())
        {
            return Vec3<S>((m00 * v.x) + (m01 * v.y) + (m02 * v.z) + m03,
                           (m10 * v.x) + (m11 * v.y) + (m12 * v.z) + m13,
                           (m20 * v.x) + (m21 * v.y) + (m22 * v.z) + m23);
        }
        else
        {
            const T w = (m30 * v.x) + (m31 * v.y) + (m32 * v.z) + m33;
            return Vec3<S>(((m00 * v.x) + (m01 * v.y) + (m02 * v.z) + m03) / w,
                           ((m10 * v.x) + (m11 * v.y) + (m12 * v.z) + m13) / w,
                           ((m20 * v.x) + (m21 * v.y) + (m22 * v.z) + m23) / w);
        }
    }

    template <typename T>
    template <typename S>
    inline Vec3<S> Mat44<T>::transformDir(const Vec3<S>& d) const
    {
        // assert( isAffine() );
        return Vec3<S>((m00 * d.x) + (m01 * d.y) + (m02 * d.z),
                       (m10 * d.x) + (m11 * d.y) + (m12 * d.z),
                       (m20 * d.x) + (m21 * d.y) + (m22 * d.z));
    }

    template <typename T>
    template <typename S>
    inline Mat44<T>& Mat44<T>::operator+=(const Mat44<S>& v)
    {
        m00 += v.m00;
        m01 += v.m01;
        m02 += v.m02;
        m03 += v.m03;

        m10 += v.m10;
        m11 += v.m11;
        m12 += v.m12;
        m13 += v.m13;

        m20 += v.m20;
        m21 += v.m21;
        m22 += v.m22;
        m23 += v.m23;

        m30 += v.m30;
        m31 += v.m31;
        m32 += v.m32;
        m33 += v.m33;
        return *this;
    }

    template <typename T>
    template <typename S>
    inline Mat44<T>& Mat44<T>::operator-=(const Mat44<S>& v)
    {
        m00 -= v.m00;
        m01 -= v.m01;
        m02 -= v.m02;
        m03 -= v.m03;

        m10 -= v.m10;
        m11 -= v.m11;
        m12 -= v.m12;
        m13 -= v.m13;

        m20 -= v.m20;
        m21 -= v.m21;
        m22 -= v.m22;
        m23 -= v.m23;

        m30 -= v.m30;
        m31 -= v.m31;
        m32 -= v.m32;
        m33 -= v.m33;
        return *this;
    }

    template <typename T>
    template <typename S>
    inline Mat44<T>& Mat44<T>::operator*=(const S& v)
    {
        m00 *= v;
        m01 *= v;
        m02 *= v;
        m03 *= v;

        m10 *= v;
        m11 *= v;
        m12 *= v;
        m13 *= v;

        m20 *= v;
        m21 *= v;
        m22 *= v;
        m23 *= v;

        m30 *= v;
        m31 *= v;
        m32 *= v;
        m33 *= v;
        return *this;
    }

    template <typename T>
    template <typename S>
    Mat44<T>& Mat44<T>::operator*=(const Mat44<S>& v)
    {
        const T v00 =
            (m00 * v.m00) + (m01 * v.m10) + (m02 * v.m20) + (m03 * v.m30);
        const T v01 =
            (m00 * v.m01) + (m01 * v.m11) + (m02 * v.m21) + (m03 * v.m31);
        const T v02 =
            (m00 * v.m02) + (m01 * v.m12) + (m02 * v.m22) + (m03 * v.m32);
        const T v03 =
            (m00 * v.m03) + (m01 * v.m13) + (m02 * v.m23) + (m03 * v.m33);

        const T v10 =
            (m10 * v.m00) + (m11 * v.m10) + (m12 * v.m20) + (m13 * v.m30);
        const T v11 =
            (m10 * v.m01) + (m11 * v.m11) + (m12 * v.m21) + (m13 * v.m31);
        const T v12 =
            (m10 * v.m02) + (m11 * v.m12) + (m12 * v.m22) + (m13 * v.m32);
        const T v13 =
            (m10 * v.m03) + (m11 * v.m13) + (m12 * v.m23) + (m13 * v.m33);

        const T v20 =
            (m20 * v.m00) + (m21 * v.m10) + (m22 * v.m20) + (m23 * v.m30);
        const T v21 =
            (m20 * v.m01) + (m21 * v.m11) + (m22 * v.m21) + (m23 * v.m31);
        const T v22 =
            (m20 * v.m02) + (m21 * v.m12) + (m22 * v.m22) + (m23 * v.m32);
        const T v23 =
            (m20 * v.m03) + (m21 * v.m13) + (m22 * v.m23) + (m23 * v.m33);

        const T v30 =
            (m30 * v.m00) + (m31 * v.m10) + (m32 * v.m20) + (m33 * v.m30);
        const T v31 =
            (m30 * v.m01) + (m31 * v.m11) + (m32 * v.m21) + (m33 * v.m31);
        const T v32 =
            (m30 * v.m02) + (m31 * v.m12) + (m32 * v.m22) + (m33 * v.m32);
        const T v33 =
            (m30 * v.m03) + (m31 * v.m13) + (m32 * v.m23) + (m33 * v.m33);

        m00 = v00;
        m01 = v01;
        m02 = v02;
        m03 = v03;

        m10 = v10;
        m11 = v11;
        m12 = v12;
        m13 = v13;

        m20 = v20;
        m21 = v21;
        m22 = v22;
        m23 = v23;

        m30 = v30;
        m31 = v31;
        m32 = v32;
        m33 = v33;
        return *this;
    }

    template <typename T>
    template <typename S>
    inline Mat44<T>& Mat44<T>::operator/=(const S& v)
    {
        assert(v != T0);
        m00 /= v;
        m01 /= v;
        m02 /= v;
        m03 /= v;

        m10 /= v;
        m11 /= v;
        m12 /= v;
        m13 /= v;

        m20 /= v;
        m21 /= v;
        m22 /= v;
        m23 /= v;

        m30 /= v;
        m31 /= v;
        m32 /= v;
        m33 /= v;
        return *this;
    }

    // FUNCTIONS WHICH OPERATE ON MAT44

    template <typename T>
    inline typename Mat44<T>::size_type rowDimension(const Mat44<T>&)
    {
        return 4;
    }

    template <typename T>
    inline typename Mat44<T>::size_type colDimension(const Mat44<T>&)
    {
        return 4;
    }

    template <typename T> inline void makeIdentity(Mat44<T>& m)
    {
        m.makeIdentity();
    }

    template <typename T> inline void transpose(Mat44<T>& m) { m.transpose(); }

    template <typename T> inline Mat44<T> transposed(const Mat44<T>& m)
    {
        return m.transposed();
    }

    template <typename T> inline T determinant(const Mat44<T>& m)
    {
        return m.determinant();
    }

    template <typename T> inline Mat44<T> invert(const Mat44<T>& m)
    {
        return m.inverted();
    }

    template <typename T> inline bool isAffine(const Mat44<T>& m)
    {
        return m.isAffine();
    }

    template <typename T, typename S>
    inline void makeTranslation(Mat44<T>& m, const Vec3<S>& t)
    {
        m.makeTranslation(t);
    }

    template <typename T, typename S>
    inline Mat44<T> translationMatrix(const Vec3<S>& t)
    {
        Mat44<T> ret((void*)1);
        ret.makeTranslation(t);
        return ret;
    }

    template <typename T, typename S>
    inline void setTranslation(Mat44<T>& m, const Vec3<S>& t)
    {
        m.setTranslation(t);
    }

    template <typename T, typename S>
    inline void addTranslation(Mat44<T>& m, const Vec3<S>& t)
    {
        m.addTranslation(t);
    }

    template <typename T, typename S>
    inline void makeScale(Mat44<T>& m, const Vec3<S>& s)
    {
        m.makeScale(s);
    }

    template <typename T, typename S>
    inline Mat44<T> scaleMatrix(const Vec3<S>& s)
    {
        Mat44<T> ret((void*)1);
        ret.makeScale(s);
        return ret;
    }

    template <typename T, typename S>
    inline void makeRotation(Mat44<T>& m, const Vec3<S>& axis,
                             const T& angleInRadians)
    {
        m.makeRotation(axis, angleInRadians);
    }

    template <typename T, typename S>
    inline Mat44<T> rotationMatrix(const Vec3<S>& axis, const T& angleInRadians)
    {
        Mat44<T> ret((void*)1);
        ret.makeRotation(axis, angleInRadians);
        return ret;
    }

    template <typename T, typename S>
    inline Vec3<T> transform(const Mat44<T>& m, const Vec3<S>& v)
    {
        return m.transform(v);
    }

    template <typename T, typename S>
    inline Vec3<T> transformDir(const Mat44<T>& m, const Vec3<S>& v)
    {
        return m.transformDir(v);
    }

    // ARITHMETIC OPERATORS

    // ADDITION
    // Matrices can only be added to other matrices of the same size.
    // No scalars, no vectors.
    template <typename T>
    inline Mat44<T> operator+(const Mat44<T>& a, const Mat44<T>& b)
    {
        return Mat44<T>(
            a.m00 + b.m00, a.m01 + b.m01, a.m02 + b.m02, a.m03 + b.m03,

            a.m10 + b.m10, a.m11 + b.m11, a.m12 + b.m12, a.m13 + b.m13,

            a.m20 + b.m20, a.m21 + b.m21, a.m22 + b.m22, a.m23 + b.m23,

            a.m30 + b.m30, a.m31 + b.m31, a.m32 + b.m32, a.m33 + b.m33);
    }

    // NEGATION
    template <typename T> inline Mat44<T> operator-(const Mat44<T>& a)
    {
        return Mat44<T>(-a.m00, -a.m01, -a.m02, -a.m03, -a.m10, -a.m11, -a.m12,
                        -a.m13, -a.m20, -a.m21, -a.m22, -a.m23, -a.m30, -a.m31,
                        -a.m32, -a.m33);
    }

    // SUBTRACTION
    // Matrices can only be subtracted from other matrices of the same size.
    // No scalars, no vectors.
    template <typename T>
    inline Mat44<T> operator-(const Mat44<T>& a, const Mat44<T>& b)
    {
        return Mat44<T>(
            a.m00 - b.m00, a.m01 - b.m01, a.m02 - b.m02, a.m03 - b.m03,

            a.m10 - b.m10, a.m11 - b.m11, a.m12 - b.m12, a.m13 - b.m13,

            a.m20 - b.m20, a.m21 - b.m21, a.m22 - b.m22, a.m23 - b.m23,

            a.m30 - b.m30, a.m31 - b.m31, a.m32 - b.m32, a.m33 - b.m33);
    }

    // SCALAR MULTIPLICATION
    // Matrices can be left and right multiplied by scalars, both of which
    // produce the same effect of multiplying all elements of the matrix.
    template <typename T, typename S>
    inline Mat44<T> operator*(const Mat44<T>& a, const S& b)
    {
        return Mat44<T>(a.m00 * b, a.m01 * b, a.m02 * b, a.m03 * b, a.m10 * b,
                        a.m11 * b, a.m12 * b, a.m13 * b, a.m20 * b, a.m21 * b,
                        a.m22 * b, a.m23 * b, a.m30 * b, a.m31 * b, a.m32 * b,
                        a.m33 * b);
    }

    template <typename T, typename S>
    inline Mat44<T> operator*(const S& a, const Mat44<T>& b)
    {
        return Mat44<T>(a * b.m00, a * b.m01, a * b.m02, a * b.m03, a * b.m10,
                        a * b.m11, a * b.m12, a * b.m13, a * b.m20, a * b.m21,
                        a * b.m22, a * b.m23, a * b.m30, a * b.m31, a * b.m32,
                        a * b.m33);
    }

    // VECTOR MULTIPLICATION
    // Matrices can be left multiplied by a vector, which results in a vector.
    // Matrices can be right multiplied by a vector, which results in a vector.
    template <typename T, typename S>
    inline Vec4<S> operator*(const Vec4<S>& v, const Mat44<T>& m)
    {
        return Vec4<S>(
            (v.x * m.m00) + (v.y * m.m10) + (v.z * m.m20) + (v.w * m.m30),
            (v.x * m.m01) + (v.y * m.m11) + (v.z * m.m21) + (v.w * m.m31),
            (v.x * m.m02) + (v.y * m.m12) + (v.z * m.m22) + (v.w * m.m32),
            (v.x * m.m03) + (v.y * m.m13) + (v.z * m.m23) + (v.w * m.m33));
    }

    template <typename T, typename S>
    inline Vec4<S> operator*(const Mat44<T>& m, const Vec4<S>& v)
    {
        return Vec4<S>(
            (m.m00 * v.x) + (m.m01 * v.y) + (m.m02 * v.z) + (m.m03 * v.w),
            (m.m10 * v.x) + (m.m11 * v.y) + (m.m12 * v.z) + (m.m13 * v.w),
            (m.m20 * v.x) + (m.m21 * v.y) + (m.m22 * v.z) + (m.m23 * v.w),
            (m.m30 * v.x) + (m.m31 * v.y) + (m.m32 * v.z) + (m.m33 * v.w));
    }

    // HOMOGENEOUS VECTOR MULTIPLICATION
    // Either direction.
    template <typename T, typename S>
    inline Vec3<S> operator*(const Vec3<S>& v, const Mat44<T>& m)
    {
        Vec4<S> v4(v.x, v.y, v.z, T1);
        v4 = v4 * m;
        return Vec3<S>(v4.x / v4.w, v4.y / v4.w, v4.z / v4.w);
    }

    template <typename T, typename S>
    inline Vec3<S> operator*(const Mat44<T>& m, const Vec3<S>& v)
    {
        Vec4<S> v4(v.x, v.y, v.z, T1);
        v4 = m * v4;
        return Vec3<S>(v4.x / v4.w, v4.y / v4.w, v4.z / v4.w);
    }

    // MATRIX MULTIPLICATION
    template <typename T>
    inline Mat44<T> operator*(const Mat44<T>& a, const Mat44<T>& b)
    {
        return Mat44<T>((a.m00 * b.m00) + (a.m01 * b.m10) + (a.m02 * b.m20)
                            + (a.m03 * b.m30),
                        (a.m00 * b.m01) + (a.m01 * b.m11) + (a.m02 * b.m21)
                            + (a.m03 * b.m31),
                        (a.m00 * b.m02) + (a.m01 * b.m12) + (a.m02 * b.m22)
                            + (a.m03 * b.m32),
                        (a.m00 * b.m03) + (a.m01 * b.m13) + (a.m02 * b.m23)
                            + (a.m03 * b.m33),

                        (a.m10 * b.m00) + (a.m11 * b.m10) + (a.m12 * b.m20)
                            + (a.m13 * b.m30),
                        (a.m10 * b.m01) + (a.m11 * b.m11) + (a.m12 * b.m21)
                            + (a.m13 * b.m31),
                        (a.m10 * b.m02) + (a.m11 * b.m12) + (a.m12 * b.m22)
                            + (a.m13 * b.m32),
                        (a.m10 * b.m03) + (a.m11 * b.m13) + (a.m12 * b.m23)
                            + (a.m13 * b.m33),

                        (a.m20 * b.m00) + (a.m21 * b.m10) + (a.m22 * b.m20)
                            + (a.m23 * b.m30),
                        (a.m20 * b.m01) + (a.m21 * b.m11) + (a.m22 * b.m21)
                            + (a.m23 * b.m31),
                        (a.m20 * b.m02) + (a.m21 * b.m12) + (a.m22 * b.m22)
                            + (a.m23 * b.m32),
                        (a.m20 * b.m03) + (a.m21 * b.m13) + (a.m22 * b.m23)
                            + (a.m23 * b.m33),

                        (a.m30 * b.m00) + (a.m31 * b.m10) + (a.m32 * b.m20)
                            + (a.m33 * b.m30),
                        (a.m30 * b.m01) + (a.m31 * b.m11) + (a.m32 * b.m21)
                            + (a.m33 * b.m31),
                        (a.m30 * b.m02) + (a.m31 * b.m12) + (a.m32 * b.m22)
                            + (a.m33 * b.m32),
                        (a.m30 * b.m03) + (a.m31 * b.m13) + (a.m32 * b.m23)
                            + (a.m33 * b.m33));
    }

    // DIVISION
    // Matrices can only be divided by scalar.
    // Scalars cannot be divided by matrices.
    template <typename T, typename S>
    inline Mat44<T> operator/(const Mat44<T>& a, const S& b)
    {
        assert(b != T0);
        return Mat44<T>(a.m00 / b, a.m01 / b, a.m02 / b, a.m03 / b,

                        a.m10 / b, a.m11 / b, a.m12 / b, a.m13 / b,

                        a.m20 / b, a.m21 / b, a.m22 / b, a.m23 / b,

                        a.m30 / b, a.m31 / b, a.m32 / b, a.m33 / b);
    }

    // COMPARISON OPERATORS

    template <typename T>
    inline bool operator==(const Mat44<T>& a, const Mat44<T>& b)
    {
        return ((a.m00 == b.m00) && (a.m01 == b.m01) && (a.m02 == b.m02)
                && (a.m03 == b.m03) &&

                (a.m10 == b.m10) && (a.m11 == b.m11) && (a.m12 == b.m12)
                && (a.m13 == b.m13) &&

                (a.m20 == b.m20) && (a.m21 == b.m21) && (a.m22 == b.m22)
                && (a.m23 == b.m23) &&

                (a.m30 == b.m30) && (a.m31 == b.m31) && (a.m32 == b.m32)
                && (a.m33 == b.m33));
    }

    template <typename T>
    inline bool operator!=(const Mat44<T>& a, const Mat44<T>& b)
    {
        return ((a.m00 != b.m00) || (a.m01 != b.m01) || (a.m02 != b.m02)
                || (a.m03 != b.m03) ||

                (a.m10 != b.m10) || (a.m11 != b.m11) || (a.m12 != b.m12)
                || (a.m13 != b.m13) ||

                (a.m20 != b.m20) || (a.m21 != b.m21) || (a.m22 != b.m22)
                || (a.m23 != b.m23) ||

                (a.m30 != b.m30) || (a.m31 != b.m31) || (a.m32 != b.m32)
                || (a.m33 != b.m33));
    }

} // End namespace TwkMath

#undef T0
#undef T1

#endif
