//******************************************************************************
// Copyright (c) 2001 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkMathVec1_h_
#define _TwkMathVec1_h_

#include <TwkMath/Math.h>
#include <sys/types.h>
#include <assert.h>

namespace TwkMath
{

    //******************************************************************************
    template <typename T> class Vec1
    {
    public:
        //**************************************************************************
        // TYPEDEFS
        //**************************************************************************
        typedef T value_type;
        typedef size_t size_type;
        typedef T* iterator;
        typedef const T* const_iterator;

        //**************************************************************************
        // DATA MEMBERS
        //**************************************************************************
        T x;

        //**************************************************************************
        // CONSTRUCTORS
        //**************************************************************************
        // Default Constructor.
        // Intentionally does nothing to avoid unnecessary instructions
        Vec1() {}

        // Single valued constructor. Initializes elements to value.
        // Explicit because we don't want scalars to be automatically
        // turned into vectors, unless the programmer specifically
        // requests it.
        explicit Vec1(const T& val)
            : x(val)
        {
        }

        // Copy constructor from a Vec2 of a different type
        // Non explicit, so automatic promotion may occur.
        // Still in-class, though, due to VisualC++'s inability
        // to handle out-of-class template-template member functions.
        template <class T2>
        Vec1(const Vec1<T2>& copy)
            : x(copy.x)
        {
        }

        // Copy constructor. Copies elements one by one. In class because
        // it is a specialization of the above template-template
        // member function
        Vec1(const Vec1<T>& copy)
            : x(copy.x)
        {
        }

        //**************************************************************************
        // ASSIGNMENT OPERATORS
        //**************************************************************************
        // Single valued assignment. All elements set to value
        Vec1<T>& operator=(const T& value);

        // Copy from a vec2 of any type. Must be in-class due to VisualC++
        // problems.
        template <class T2> Vec1<T>& operator=(const Vec1<T2>& copy)
        {
            x = copy.x;
            return *this;
        }

        // Copy assignment. Elements copied one by one. In-class because it is
        // a specialization of the above in-class function.
        Vec1<T>& operator=(const Vec1<T>& copy)
        {
            x = copy.x;
            return *this;
        }

        //**************************************************************************
        // DIMENSION FUNCTION
        //**************************************************************************
        static size_type dimension() { return 1; }

        //**************************************************************************
        // DATA ACCESS OPERATORS
        //**************************************************************************
        const T& operator[](size_type index) const;
        T& operator[](size_type index);

        void set(const T& vx) { x = vx; }

        //**************************************************************************
        // ITERATORS
        //**************************************************************************
        const_iterator begin() const;
        iterator begin();
        const_iterator end() const;

        //**************************************************************************
        // ARITHMETIC OPERATORS
        //**************************************************************************
        Vec1<T>& operator+=(const T& rhs);
        Vec1<T>& operator+=(const Vec1<T>& rhs);

        Vec1<T>& operator-=(const T& rhs);
        Vec1<T>& operator-=(const Vec1<T>& rhs);

        // Multiplication and Divide are COMPONENT-WISE!!!
        Vec1<T>& operator*=(const T& rhs);
        Vec1<T>& operator*=(const Vec1<T>& rhs);

        Vec1<T>& operator/=(const T& rhs);
        Vec1<T>& operator/=(const Vec1<T>& rhs);
    };

    //******************************************************************************
    // TYPEDEFS
    //******************************************************************************
    typedef Vec1<unsigned char> Vec1uc;
    typedef Vec1<char> Vec1c;
    typedef Vec1<unsigned short> Vec1us;
    typedef Vec1<short> Vec1s;
    typedef Vec1<unsigned int> Vec1ui;
    typedef Vec1<int> Vec1i;
    typedef Vec1<unsigned long> Vec1ul;
    typedef Vec1<long> Vec1l;
    typedef Vec1<float> Vec1f;
    typedef Vec1<double> Vec1d;

    //******************************************************************************
    // TEMPLATE AND INLINE FUNCTIONS
    //******************************************************************************
    template <typename T> inline Vec1<T>& Vec1<T>::operator=(const T& value)
    {
        x = value;
        return *this;
    }

    //******************************************************************************
    template <typename T>
    inline const T& Vec1<T>::operator[](typename Vec1<T>::size_type index) const
    {
        assert(index == 0);
        return x;
    }

    //******************************************************************************
    template <typename T>
    inline T& Vec1<T>::operator[](typename Vec1<T>::size_type index)
    {
        assert(index == 0);
        return x;
    }

    //******************************************************************************
    template <typename T>
    inline typename Vec1<T>::const_iterator Vec1<T>::begin() const
    {
        return static_cast<const_iterator>(&x);
    }

    //******************************************************************************
    template <typename T> inline typename Vec1<T>::iterator Vec1<T>::begin()
    {
        return static_cast<iterator>(&x);
    }

    //******************************************************************************
    template <typename T>
    inline typename Vec1<T>::const_iterator Vec1<T>::end() const
    {
        return static_cast<const_iterator>(&x + 1);
    }

    //******************************************************************************
    template <typename T> inline Vec1<T>& Vec1<T>::operator+=(const T& rhs)
    {
        x += rhs;
        return *this;
    }

    //******************************************************************************
    template <typename T>
    inline Vec1<T>& Vec1<T>::operator+=(const Vec1<T>& rhs)
    {
        x += rhs.x;
        return *this;
    }

    //******************************************************************************
    template <typename T> inline Vec1<T>& Vec1<T>::operator-=(const T& rhs)
    {
        x -= rhs;
        return *this;
    }

    //******************************************************************************
    template <typename T>
    inline Vec1<T>& Vec1<T>::operator-=(const Vec1<T>& rhs)
    {
        x -= rhs.x;
        return *this;
    }

    //******************************************************************************
    template <typename T> inline Vec1<T>& Vec1<T>::operator*=(const T& rhs)
    {
        x *= rhs;
        return *this;
    }

    //******************************************************************************
    template <typename T>
    inline Vec1<T>& Vec1<T>::operator*=(const Vec1<T>& rhs)
    {
        x *= rhs.x;
        return *this;
    }

    //******************************************************************************
    template <typename T> inline Vec1<T>& Vec1<T>::operator/=(const T& rhs)
    {
        assert(rhs != (T)0);
        x /= rhs;
        return *this;
    }

    //******************************************************************************
    template <typename T>
    inline Vec1<T>& Vec1<T>::operator/=(const Vec1<T>& rhs)
    {
        assert(rhs.x != (T)0);
        x /= rhs.x;
        return *this;
    };

    //******************************************************************************
    //******************************************************************************
    // FUNCTIONS WHICH OPERATE ON VEC2
    //******************************************************************************
    //******************************************************************************
    template <typename T> inline size_t dimension(Vec1<T>) { return 1; }

    //******************************************************************************
    //******************************************************************************
    // GLOBAL ARITHMETIC OPERATORS
    //******************************************************************************
    //******************************************************************************
    // ADDITION
    template <typename T> inline Vec1<T> operator+(const Vec1<T>& a, const T& b)
    {
        return Vec1<T>(a.x + b);
    }

    //******************************************************************************
    template <typename T> inline Vec1<T> operator+(const T& a, const Vec1<T>& b)
    {
        return Vec1<T>(a + b.x);
    }

    //******************************************************************************
    template <typename T>
    inline Vec1<T> operator+(const Vec1<T>& a, const Vec1<T>& b)
    {
        return Vec1<T>(a.x + b.x);
    }

    //******************************************************************************
    // NEGATION
    template <typename T> inline Vec1<T> operator-(const Vec1<T>& a)
    {
        return Vec1<T>(-a.x);
    }

    //******************************************************************************
    // SUBTRACTION
    template <typename T> inline Vec1<T> operator-(const Vec1<T>& a, const T& b)
    {
        return Vec1<T>(a.x - b);
    }

    //******************************************************************************
    template <typename T> inline Vec1<T> operator-(const T& a, const Vec1<T>& b)
    {
        return Vec1<T>(a - b.x);
    }

    //******************************************************************************
    template <typename T>
    inline Vec1<T> operator-(const Vec1<T>& a, const Vec1<T>& b)
    {
        return Vec1<T>(a.x - b.x);
    }

    //******************************************************************************
    // MULTIPLICATION
    template <typename T> inline Vec1<T> operator*(const Vec1<T>& a, const T& b)
    {
        return Vec1<T>(a.x * b);
    }

    //******************************************************************************
    template <typename T> inline Vec1<T> operator*(const T& a, const Vec1<T>& b)
    {
        return Vec1<T>(a * b.x);
    }

    //******************************************************************************
    template <typename T>
    inline Vec1<T> operator*(const Vec1<T>& a, const Vec1<T>& b)
    {
        return Vec1<T>(a.x * b.x);
    }

    //******************************************************************************
    // DIVISION
    template <typename T> inline Vec1<T> operator/(const Vec1<T>& a, const T& b)
    {
        assert(b != (T)0);
        return Vec1<T>(a.x / b);
    }

    //******************************************************************************
    template <typename T> inline Vec1<T> operator/(const T& a, const Vec1<T>& b)
    {
        assert(b.x != (T)0);
        return Vec1<T>(a / b.x);
    }

    //******************************************************************************
    template <typename T>
    inline Vec1<T> operator/(const Vec1<T>& a, const Vec1<T>& b)
    {
        assert(b.x != (T)0);
        return Vec1<T>(a.x / b.x);
    }

    //******************************************************************************
    //******************************************************************************
    // COMPARISON OPERATORS
    //******************************************************************************
    //******************************************************************************
    template <typename T, typename T2>
    inline bool operator==(const Vec1<T>& a, const Vec1<T2>& b)
    {
        return (a.x == b.x);
    }

    //******************************************************************************
    template <typename T, typename T2>
    inline bool operator!=(const Vec1<T>& a, const Vec1<T2>& b)
    {
        return (a.x != b.x);
    }

} // End namespace TwkMath

#endif
