//******************************************************************************
// Copyright (c) 2001-2002 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkMathMat33_h_
#define _TwkMathMat33_h_

#include <TwkMath/MatrixCommon.h>
#include <TwkMath/Vec2.h>
#include <TwkMath/Vec3.h>
#include <TwkMath/Math.h>
#include <sys/types.h>
#include <assert.h>

#define T0 ((T)0)
#define T1 ((T)1)

namespace TwkMath
{

    //******************************************************************************
    // SQUARE MATRIX, SIZE 3x3
    template <typename T> class Mat33
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
        T m02;
        T m10;
        T m11;
        T m12;
        T m20;
        T m21;
        T m22;
#else
        union
        {
            T m[3][3];

            struct
            {
                T m00;
                T m01;
                T m02;
                T m10;
                T m11;
                T m12;
                T m20;
                T m21;
                T m22;
            };
        };
#endif

        //**************************************************************************
        // CONSTRUCTORS
        //**************************************************************************
        // Default Constructor.
        // Makes matrix into identity.
        Mat33()
            : m00(T1)
            , m01(T0)
            , m02(T0)
            , m10(T0)
            , m11(T1)
            , m12(T0)
            , m20(T0)
            , m21(T0)
            , m22(T1)
        {
        }

        // Constructor for use when no operations are desired
        explicit Mat33(void* dummy) {}

        // Single valued constructor. Initializes all elements to value.
        explicit Mat33(const T& val)
            : m00(val)
            , m01(val)
            , m02(val)
            , m10(val)
            , m11(val)
            , m12(val)
            , m20(val)
            , m21(val)
            , m22(val)
        {
        }

        explicit Mat33(const Vec3<T>& diag)
            : m00(diag.x)
            , m01(T0)
            , m02(T0)
            , m10(T0)
            , m11(diag.y)
            , m12(T0)
            , m20(T0)
            , m21(T0)
            , m22(diag.z)
        {
        }

        // Direct constructor. Initializes elements directly
        Mat33(const T& v00, const T& v01, const T& v02, const T& v10,
              const T& v11, const T& v12, const T& v20, const T& v21,
              const T& v22)
            : m00(v00)
            , m01(v01)
            , m02(v02)
            , m10(v10)
            , m11(v11)
            , m12(v12)
            , m20(v20)
            , m21(v21)
            , m22(v22)
        {
        }

        // Another direct constructor, using an array
        // Values are copied in row-major form.
        explicit Mat33(const T array[])
            : m00(array[0])
            , m01(array[1])
            , m02(array[2])
            , m10(array[3])
            , m11(array[4])
            , m12(array[5])
            , m20(array[6])
            , m21(array[7])
            , m22(array[8])
        {
        }

        // Copy constructor from a Mat33 of a different type.
        // Non explicit, so automatic promotion may occur.
        template <typename T2>
        Mat33(const Mat33<T2>& copy)
            : m00(copy.m00)
            , m01(copy.m01)
            , m02(copy.m02)
            , m10(copy.m10)
            , m11(copy.m11)
            , m12(copy.m12)
            , m20(copy.m20)
            , m21(copy.m21)
            , m22(copy.m22)
        {
        }

        // Copy constructor from a Mat33 of same type.
        // Specialization of above function
        Mat33(const Mat33<T>& copy)
            : m00(copy.m00)
            , m01(copy.m01)
            , m02(copy.m02)
            , m10(copy.m10)
            , m11(copy.m11)
            , m12(copy.m12)
            , m20(copy.m20)
            , m21(copy.m21)
            , m22(copy.m22)
        {
        }

        //**************************************************************************
        // ASSIGNMENT OPERATORS
        //**************************************************************************
        // Single value
        Mat33<T>& operator=(const T& val)
        {
            m00 = val;
            m01 = val;
            m02 = val;
            m10 = val;
            m11 = val;
            m12 = val;
            m20 = val;
            m21 = val;
            m22 = val;
            return *this;
        }

        // Assignment from a Mat33 of a different type
        template <typename T2> Mat33<T>& operator=(const Mat33<T2>& copy)
        {
            m00 = copy.m00;
            m01 = copy.m01;
            m02 = copy.m02;
            m10 = copy.m10;
            m11 = copy.m11;
            m12 = copy.m12;
            m20 = copy.m20;
            m21 = copy.m21;
            m22 = copy.m22;
            return *this;
        }

        // Assignment from a Mat33 of the same type.
        // Specialization of above function
        Mat33<T>& operator=(const Mat33<T>& copy)
        {
            m00 = copy.m00;
            m01 = copy.m01;
            m02 = copy.m02;
            m10 = copy.m10;
            m11 = copy.m11;
            m12 = copy.m12;
            m20 = copy.m20;
            m21 = copy.m21;
            m22 = copy.m22;
            return *this;
        }

        //**************************************************************************
        // DIMENSION FUNCTION
        //**************************************************************************
        static size_type rowDimension() { return 3; }

        static size_type colDimension() { return 3; }

        //**************************************************************************
        // DATA ACCESS OPERATORS
        //**************************************************************************
        const T* operator[](size_type rowIndex) const;
        T* operator[](size_type rowIndex);
        const T& operator()(size_type row, size_type col) const;
        T& operator()(size_type row, size_type col);

        void set(const T& v00, const T& v01, const T& v02, const T& v10,
                 const T& v11, const T& v12, const T& v20, const T& v21,
                 const T& v22)
        {
            m00 = v00;
            m01 = v01;
            m02 = v02;
            m10 = v10;
            m11 = v11;
            m12 = v12;
            m20 = v20;
            m21 = v21;
            m22 = v22;
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
            m10 = T0;
            m11 = T1;
            m12 = T0;
            m20 = T0;
            m21 = T0;
            m22 = T1;
        }

        // transpose the matrix
        void transpose();

        // Return a copy of the matrix, transposed.
        Mat33<T> transposed() const;

        // Return the determinant of the matrix.
        T determinant() const;

        // Invert the matrix. Will throw TwkMath::SingularMatrixExc
        // if the inversion is not possible.
        void invert();

        // Return a copy of the matrix, inverted.
        // Will throw TwkMath::SingularMatrixExc if
        // the inversion is not possible.
        Mat33<T> inverted() const;

        //**************************************************************************
        // HOMOGENEOUS GEOMETRIC OPERATIONS
        //**************************************************************************
        // These are in row major form (column is the quick iterator)

        // Is the matrix affine or not? (Do parallel lines remain parallel
        // after being transformed by the matrix)
        bool isAffine() const
        {
            return ((m22 == T1) && (m21 == T0) && (m20 == T0));
        }

        // Create a translation matrix.
        void makeTranslation(const Vec2<T>& t)
        {
            m00 = T1;
            m01 = T0;
            m02 = t.x;
            m10 = T0;
            m11 = T1;
            m12 = t.y;
            m20 = T0;
            m21 = T0;
            m22 = T1;
        }

        // Modify translations of matrix
        void setTranslation(const Vec2<T>& t)
        {
            m02 = t.x;
            m12 = t.y;
        }

        void addTranslation(const Vec2<T>& t)
        {
            m02 += t.x;
            m12 += t.y;
        }

        // Make a scale matrix
        void makeScale(const Vec2<T>& t)
        {
            m00 = t.x;
            m01 = T0;
            m02 = T0;
            m10 = T0;
            m11 = t.y;
            m12 = T0;
            m20 = T0;
            m21 = T0;
            m22 = T1;
        }

        // Create rotation matrix
        void makeRotation(const T& angleInRadians);

        // Transform a homogeneous vector by the matrix
        Vec2<T> transform(const Vec2<T>& v) const;

        // Transform without translation the homogeneous vector by the matrix.
        // This only works with affine matrices because the homogeneous
        // coordinate is ignored.
        Vec2<T> transformDir(const Vec2<T>& v) const;

        //**************************************************************************
        // ARITHMETIC OPERATORS
        //**************************************************************************
        Mat33<T>& operator+=(const Mat33<T>& v);
        Mat33<T>& operator-=(const Mat33<T>& v);
        Mat33<T>& operator*=(const T& val);
        Mat33<T>& operator*=(const Mat33<T>& v);
        Mat33<T>& operator/=(const T& val);
    };

    //******************************************************************************
    // TYPEDEFS
    //******************************************************************************
    typedef Mat33<float> Mat33f;
    typedef Mat33<double> Mat33d;

    //******************************************************************************
    // TEMPLATE AND INLINE FUNCTIONS
    //******************************************************************************
    template <typename T>
    inline const T*
    Mat33<T>::operator[](typename Mat33<T>::size_type rowIndex) const
    {
        assert(rowIndex < 3);
#ifdef COMPILER_GCC2
        return ((const T*)this) + (rowIndex * 3);
#else
        return (const T*)(m[rowIndex]);
#endif
    }

    //******************************************************************************
    template <typename T>
    inline T* Mat33<T>::operator[](typename Mat33<T>::size_type rowIndex)
    {
        assert(rowIndex < 3);
#ifdef COMPILER_GCC2
        return ((T*)this) + (rowIndex * 3);
#else
        return (T*)(m[rowIndex]);
#endif
    }

    //******************************************************************************
    template <typename T>
    inline const T&
    Mat33<T>::operator()(typename Mat33<T>::size_type rowIndex,
                         typename Mat33<T>::size_type colIndex) const
    {
        assert(rowIndex < 3);
        assert(colIndex < 3);
#ifdef COMPILER_GCC2
        return ((const T*)this)[(rowIndex * 3) + colIndex];
#else
        return m[rowIndex][colIndex];
#endif
    }

    //******************************************************************************
    template <typename T>
    inline T& Mat33<T>::operator()(typename Mat33<T>::size_type rowIndex,
                                   typename Mat33<T>::size_type colIndex)
    {
        assert(rowIndex < 3);
        assert(colIndex < 3);
#ifdef COMPILER_GCC2
        return ((const T*)this)[(rowIndex * 3) + colIndex];
#else
        return m[rowIndex][colIndex];
#endif
    }

    //******************************************************************************
    template <typename T> inline void Mat33<T>::transpose()
    {
        T tmp = m01;
        m01 = m10;
        m10 = tmp;

        tmp = m02;
        m02 = m20;
        m20 = tmp;

        tmp = m12;
        m12 = m21;
        m21 = tmp;
    }

    //******************************************************************************
    template <typename T> inline Mat33<T> Mat33<T>::transposed() const
    {
        return Mat33<T>(m00, m10, m20, m01, m11, m21, m02, m12, m22);
    }

    //******************************************************************************
    template <typename T> inline T Mat33<T>::determinant() const
    {
        return determinant3x3(m00, m01, m02, m10, m11, m12, m20, m21, m22);
    }

    //******************************************************************************
    template <typename T> void Mat33<T>::invert()
    {
        // Check for affine case, which is extremely common.
        if (isAffine())
        {
            // Matrix is affine. Remove the translation, invert.
            const T tx = m02;
            const T ty = m12;

            // Invert upper left corner
            invert2x2(m00, m01, m10, m11);

            // Now do translation side.
            m02 = -((m00 * tx) + (m01 * ty));
            m12 = -((m10 * tx) + (m11 * ty));
        }
        else
        {
            invert3x3(m00, m01, m02, m10, m11, m12, m20, m21, m22);
        }
    }

    //******************************************************************************
    template <typename T> inline Mat33<T> Mat33<T>::inverted() const
    {
        Mat33<T> ret(*this);
        ret.invert();
        return ret;
    }

    //******************************************************************************
    template <typename T> inline void Mat33<T>::makeRotation(const T& ang)
    {
        const T cosa = Math<T>::cos(ang);
        const T sina = Math<T>::sin(ang);

        m00 = cosa;
        m01 = -sina;
        m02 = T0;
        m10 = sina;
        m11 = cosa;
        m12 = T0;
        m20 = T0;
        m21 = T0;
        m22 = T1;
    }

    //******************************************************************************
    template <typename T> Vec2<T> Mat33<T>::transform(const Vec2<T>& v) const
    {
        if (isAffine())
        {
            return Vec2<T>((m00 * v.x) + (m01 * v.y) + m02,
                           (m10 * v.x) + (m11 * v.y) + m12);
        }
        else
        {
            const T w = (m20 * v.x) + (m21 * v.y) + m22;
            return Vec2<T>(((m00 * v.x) + (m01 * v.y) + m02) / w,
                           ((m10 * v.x) + (m11 * v.y) + m12) / w);
        }
    }

    //******************************************************************************
    template <typename T>
    inline Vec2<T> Mat33<T>::transformDir(const Vec2<T>& d) const
    {
        assert(isAffine());
        return Vec2<T>((m00 * this->v.x) + (m01 * this->v.y),
                       (m10 * this->v.x) + (m11 * this->v.y));
    }

    //******************************************************************************
    template <typename T>
    inline Mat33<T>& Mat33<T>::operator+=(const Mat33<T>& v)
    {
        m00 += v.m00;
        m01 += v.m01;
        m02 += v.m02;

        m10 += v.m10;
        m11 += v.m11;
        m12 += v.m12;

        m20 += v.m20;
        m21 += v.m21;
        m22 += v.m22;
        return *this;
    }

    //******************************************************************************
    template <typename T>
    inline Mat33<T>& Mat33<T>::operator-=(const Mat33<T>& v)
    {
        m00 -= v.m00;
        m01 -= v.m01;
        m02 -= v.m02;

        m10 -= v.m10;
        m11 -= v.m11;
        m12 -= v.m12;

        m20 -= v.m20;
        m21 -= v.m21;
        m22 -= v.m22;
        return *this;
    }

    //******************************************************************************
    template <typename T> inline Mat33<T>& Mat33<T>::operator*=(const T& v)
    {
        m00 *= v;
        m01 *= v;
        m02 *= v;

        m10 *= v;
        m11 *= v;
        m12 *= v;

        m20 *= v;
        m21 *= v;
        m22 *= v;
        return *this;
    }

    //******************************************************************************
    template <typename T> Mat33<T>& Mat33<T>::operator*=(const Mat33<T>& v)
    {
        const T v00 = (m00 * v.m00) + (m01 * v.m10) + (m02 * v.m20);
        const T v01 = (m00 * v.m01) + (m01 * v.m11) + (m02 * v.m21);
        const T v02 = (m00 * v.m02) + (m01 * v.m12) + (m02 * v.m22);

        const T v10 = (m10 * v.m00) + (m11 * v.m10) + (m12 * v.m20);
        const T v11 = (m10 * v.m01) + (m11 * v.m11) + (m12 * v.m21);
        const T v12 = (m10 * v.m02) + (m11 * v.m12) + (m12 * v.m22);

        const T v20 = (m20 * v.m00) + (m21 * v.m10) + (m22 * v.m20);
        const T v21 = (m20 * v.m01) + (m21 * v.m11) + (m22 * v.m21);
        const T v22 = (m20 * v.m02) + (m21 * v.m12) + (m22 * v.m22);

        m00 = v00;
        m01 = v01;
        m02 = v02;

        m10 = v10;
        m11 = v11;
        m12 = v12;

        m20 = v20;
        m21 = v21;
        m22 = v22;
        return *this;
    }

    //******************************************************************************
    template <typename T> inline Mat33<T>& Mat33<T>::operator/=(const T& v)
    {
        assert(v != T0);
        m00 /= v;
        m01 /= v;
        m02 /= v;

        m10 /= v;
        m11 /= v;
        m12 /= v;

        m20 /= v;
        m21 /= v;
        m22 /= v;
        return *this;
    }

    //******************************************************************************
    //******************************************************************************
    // FUNCTIONS WHICH OPERATE ON MAT33
    //******************************************************************************
    //******************************************************************************
    template <typename T>
    inline typename Mat33<T>::size_type rowDimension(const Mat33<T>&)
    {
        return 3;
    }

    //******************************************************************************
    template <typename T>
    inline typename Mat33<T>::size_type colDimension(const Mat33<T>&)
    {
        return 3;
    }

    //******************************************************************************
    template <typename T> inline void makeIdentity(Mat33<T>& m)
    {
        m.makeIdentity();
    }

    //******************************************************************************
    template <typename T> inline void transpose(Mat33<T>& m) { m.transpose(); }

    //******************************************************************************
    template <typename T> inline Mat33<T> transposed(const Mat33<T>& m)
    {
        return m.transposed();
    }

    //******************************************************************************
    template <typename T> inline T determinant(const Mat33<T>& m)
    {
        return m.determinant();
    }

    //******************************************************************************
    template <typename T> inline void invert(Mat33<T>& m) { m.invert(); }

    //******************************************************************************
    template <typename T> inline Mat33<T> inverted(const Mat33<T>& m)
    {
        return m.inverted();
    }

    //******************************************************************************
    template <typename T> inline bool isAffine(const Mat33<T>& m)
    {
        return m.isAffine();
    }

    //******************************************************************************
    template <typename T>
    inline void makeTranslation(Mat33<T>& m, const Vec2<T>& t)
    {
        m.makeTranslation(t);
    }

    //******************************************************************************
    template <typename T> inline Mat33<T> translationMatrix(const Vec2<T>& t)
    {
        Mat33<T> ret((void*)1);
        ret.makeTranslation(t);
        return ret;
    }

    //******************************************************************************
    template <typename T>
    inline void setTranslation(Mat33<T>& m, const Vec2<T>& t)
    {
        m.setTranslation(t);
    }

    //******************************************************************************
    template <typename T>
    inline void addTranslation(Mat33<T>& m, const Vec2<T>& t)
    {
        m.addTranslation(t);
    }

    //******************************************************************************
    template <typename T> inline void makeScale(Mat33<T>& m, const Vec2<T>& s)
    {
        m.makeScale(s);
    }

    //******************************************************************************
    template <typename T> inline Mat33<T> scaleMatrix(const Vec2<T>& s)
    {
        Mat33<T> ret((void*)1);
        ret.makeScale(s);
        return ret;
    }

    //******************************************************************************
    template <typename T>
    inline void makeRotation(Mat33<T>& m, const T& angleInRadians)
    {
        m.makeRotation(angleInRadians);
    }

    //******************************************************************************
    template <typename T>
    inline Mat33<T> rotationMatrix(const T& angleInRadians)
    {
        Mat33<T> ret((void*)1);
        ret.makeRotation(angleInRadians);
        return ret;
    }

    //******************************************************************************
    template <typename T>
    inline Vec2<T> transform(const Mat33<T>& m, const Vec2<T>& v)
    {
        return m.transform(v);
    }

    //******************************************************************************
    template <typename T>
    inline Vec2<T> transformDir(const Mat33<T>& m, const Vec2<T>& v)
    {
        return m.transformDir(v);
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
    inline Mat33<T> operator+(const Mat33<T>& a, const Mat33<T>& b)
    {
        return Mat33<T>(a.m00 + b.m00, a.m01 + b.m01, a.m02 + b.m02,

                        a.m10 + b.m10, a.m11 + b.m11, a.m12 + b.m12,

                        a.m20 + b.m20, a.m21 + b.m21, a.m22 + b.m22);
    }

    //******************************************************************************
    // NEGATION
    template <typename T> inline Mat33<T> operator-(const Mat33<T>& a)
    {
        return Mat33<T>(-a.m00, -a.m01, -a.m02, -a.m10, -a.m11, -a.m12, -a.m20,
                        -a.m21, -a.m22);
    }

    //******************************************************************************
    // SUBTRACTION
    // Matrices can only be subtracted from other matrices of the same size.
    // No scalars, no vectors.
    template <typename T>
    inline Mat33<T> operator-(const Mat33<T>& a, const Mat33<T>& b)
    {
        return Mat33<T>(a.m00 - b.m00, a.m01 - b.m01, a.m02 - b.m02,

                        a.m10 - b.m10, a.m11 - b.m11, a.m12 - b.m12,

                        a.m20 - b.m20, a.m21 - b.m21, a.m22 - b.m22);
    }

    //******************************************************************************
    // SCALAR MULTIPLICATION
    // Matrices can be left and right multiplied by scalars, both of which
    // produce the same effect of multiplying all elements of the matrix.
    template <typename T>
    inline Mat33<T> operator*(const Mat33<T>& a, const T& b)
    {
        return Mat33<T>(a.m00 * b, a.m01 * b, a.m02 * b, a.m10 * b, a.m11 * b,
                        a.m12 * b, a.m20 * b, a.m21 * b, a.m22 * b);
    }

    //******************************************************************************
    template <typename T>
    inline Mat33<T> operator*(const T& a, const Mat33<T>& b)
    {
        return Mat33<T>(a * b.m00, a * b.m01, a * b.m02, a * b.m10, a * b.m11,
                        a * b.m12, a * b.m20, a * b.m21, a * b.m22);
    }

    //******************************************************************************
    // VECTOR MULTIPLICATION
    // Matrices can be left multiplied by a vector, which results in a vector.
    // Matrices can be right multiplied by a vector, which results in a vector.
    template <typename T>
    inline Vec3<T> operator*(const Vec3<T>& v, const Mat33<T>& m)
    {
        return Vec3<T>((v.x * m.m00) + (v.y * m.m10) + (v.z * m.m20),
                       (v.x * m.m01) + (v.y * m.m11) + (v.z * m.m21),
                       (v.x * m.m02) + (v.y * m.m12) + (v.z * m.m22));
    }

    //******************************************************************************
    template <typename T>
    inline Vec3<T> operator*(const Mat33<T>& m, const Vec3<T>& v)
    {
        return Vec3<T>((m.m00 * v.x) + (m.m01 * v.y) + (m.m02 * v.z),
                       (m.m10 * v.x) + (m.m11 * v.y) + (m.m12 * v.z),
                       (m.m20 * v.x) + (m.m21 * v.y) + (m.m22 * v.z));
    }

    //******************************************************************************
    // HOMOGENEOUS VECTOR MULTIPLICATION
    // Either direction.
    template <typename T>
    inline Vec2<T> operator*(const Vec2<T>& v, const Mat33<T>& m)
    {
        Vec3<T> v3(v.x, v.y, T1);
        v3 = v3 * m;
        return Vec2<T>(v3.x / v3.z, v3.y / v3.z);
    }

    //******************************************************************************
    template <typename T>
    inline Vec2<T> operator*(const Mat33<T>& m, const Vec2<T>& v)
    {
        Vec3<T> v3(v.x, v.y, T1);
        v3 = m * v3;
        return Vec2<T>(v3.x / v3.z, v3.y / v3.z);
    }

    //******************************************************************************
    // MATRIX MULTIPLICATION
    template <typename T>
    inline Mat33<T> operator*(const Mat33<T>& a, const Mat33<T>& b)
    {
        return Mat33<T>((a.m00 * b.m00) + (a.m01 * b.m10) + (a.m02 * b.m20),
                        (a.m00 * b.m01) + (a.m01 * b.m11) + (a.m02 * b.m21),
                        (a.m00 * b.m02) + (a.m01 * b.m12) + (a.m02 * b.m22),

                        (a.m10 * b.m00) + (a.m11 * b.m10) + (a.m12 * b.m20),
                        (a.m10 * b.m01) + (a.m11 * b.m11) + (a.m12 * b.m21),
                        (a.m10 * b.m02) + (a.m11 * b.m12) + (a.m12 * b.m22),

                        (a.m20 * b.m00) + (a.m21 * b.m10) + (a.m22 * b.m20),
                        (a.m20 * b.m01) + (a.m21 * b.m11) + (a.m22 * b.m21),
                        (a.m20 * b.m02) + (a.m21 * b.m12) + (a.m22 * b.m22));
    }

    //******************************************************************************
    // DIVISION
    // Matrices can only be divided by scalar.
    // Scalars cannot be divided by matrices.
    template <typename T>
    inline Mat33<T> operator/(const Mat33<T>& a, const T& b)
    {
        assert(b != T0);
        return Mat33<T>(a.m00 / b, a.m01 / b, a.m02 / b,

                        a.m10 / b, a.m11 / b, a.m12 / b,

                        a.m20 / b, a.m21 / b, a.m22 / b);
    }

    //******************************************************************************
    //******************************************************************************
    // COMPARISON OPERATORS
    //******************************************************************************
    //******************************************************************************
    template <typename T>
    inline bool operator==(const Mat33<T>& a, const Mat33<T>& b)
    {
        return ((a.m00 == b.m00) && (a.m01 == b.m01) && (a.m02 == b.m02) &&

                (a.m10 == b.m10) && (a.m11 == b.m11) && (a.m12 == b.m12) &&

                (a.m20 == b.m20) && (a.m21 == b.m21) && (a.m22 == b.m22));
    }

    //******************************************************************************
    template <typename T>
    inline bool operator!=(const Mat33<T>& a, const Mat33<T>& b)
    {
        return ((a.m00 != b.m00) || (a.m01 != b.m01) || (a.m02 != b.m02) ||

                (a.m10 != b.m10) || (a.m11 != b.m11) || (a.m12 != b.m12) ||

                (a.m20 != b.m20) || (a.m21 != b.m21) || (a.m22 != b.m22));
    }

    //******************************************************************************

    template <typename T>
    bool zeroWithinTolerance(const Mat33<T>& M, T tolerance)
    {
        if (Math<T>::abs(M.m00) > tolerance || Math<T>::abs(M.m01) > tolerance
            || Math<T>::abs(M.m02) > tolerance
            || Math<T>::abs(M.m10) > tolerance
            || Math<T>::abs(M.m11) > tolerance
            || Math<T>::abs(M.m12) > tolerance
            || Math<T>::abs(M.m20) > tolerance
            || Math<T>::abs(M.m21) > tolerance
            || Math<T>::abs(M.m22) > tolerance)
        {
            return false;
        }

        return true;
    }

    //**********************************************************************

    template <typename T> Mat33<T> orthonormalize(const Mat33<T>& M)
    {
        //
        //  Orthoganalize and normalize basis vectors
        //

        Vec3<T> a(M(0, 0), M(1, 0), M(2, 0));
        Vec3<T> b(M(0, 1), M(1, 1), M(2, 1));
        Vec3<T> c(M(0, 2), M(1, 2), M(2, 2));

        c = cross(a, b);
        b = cross(c, a);

        a.normalize();
        b.normalize();
        c.normalize();

        return Mat33<T>(a.x, b.x, c.x, a.y, b.y, c.y, a.z, b.z, c.z);
    }

} // End namespace TwkMath

#undef T0
#undef T1

#endif
