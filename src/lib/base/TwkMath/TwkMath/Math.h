//******************************************************************************
// Copyright (c) 2001 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkMathMath_h_
#define _TwkMathMath_h_

// UGh
#include <math.h>
#include <limits.h>
#include <float.h>
#include <algorithm>

//******************************************************************************
//******************************************************************************
// DEFINITIONS
//******************************************************************************
//******************************************************************************

#ifndef M_PI
#define M_PI 3.1415926535897931
#endif

#ifndef M_E
#define M_E 2.7182818284590452354
#endif

#ifndef M_PI_OVER_180
#define M_PI_OVER_180 0.0174532925199433
#endif

#ifndef M_180_OVER_PI
#define M_180_OVER_PI 57.2957795130823230
#endif

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#ifdef TWK_NO_FLOAT_INTRINSICS
#define fabsf(A) fabs(A)
#define acosf(A) acos(A)
#define asinf(A) asin(A)
#define atanf(A) atan(A)
#define atan2f(A, B) atan2(A, B)
#define cbrtf(A) cbrt(A)
#define ceilf(A) ceil(A)
#define cosf(A) cos(A)
#define coshf(A) cosh(A)
#define expf(A) exp(A)
#define floorf(A) floor(A)
#define logf(A) log(A)
#define log10f(A) log10(A)
#define powf(A, B) pow(A, B)
#define sinf(A) sin(A)
#define sinhf(A) sinh(A)
#define sqrtf(A) sqrt(A)
#define tanf(A) tan(A)
#define tanhf(A) tanh(A)
#endif

//******************************************************************************
// std::min & std::max overrides
#ifndef TWK_NO_STD_MIN_MAX
#define STD_MIN std::min
#define STD_MAX std::max
#else

//******************************************************************************
template <typename T> inline const T& STD_MIN(const T& x, const T& y)
{
    return (x < y ? x : y);
}

//******************************************************************************
template <typename T, class P>
inline const T& STD_MIN(const T& x, const T& y, P p)
{
    return (p(x, y) ? x : y);
}

//******************************************************************************
template <typename T> inline const T& STD_MAX(const T& x, const T& y)
{
    return (x < y ? y : x);
}

//******************************************************************************
template <typename T, class P>
inline const T& STD_MAX(const T& x, const T& y, P p)
{
    return (p(x, y) ? y : x);
}

#endif

namespace TwkMath
{

    //******************************************************************************
    // Why on earth do we need to have this silly
    // math class? Why not simply have overridden
    // functions? Well - suppose you're writing
    // a templated class, such as Vec3, which needs
    // to use math functions like sqrt. By providing
    // this templated math class, you make it possible
    // for the implementation of Vec3 to avoid
    // having local discrepancies for different data
    // types. This way, you only have local implementation
    // specifications in THIS file, instead of every
    // class that uses math functions.
    template <typename T> class Math
    {
    public:
        //**************************************************************************
        // TYPEDEFS
        //**************************************************************************
        typedef T value_type;

        //**************************************************************************
        // FUNCTIONS
        //**************************************************************************
        static T abs(const T& a);
        static T acos(const T& a);
        static T asin(const T& a);
        static T atan(const T& a);
        static T atan2(const T& y, const T& x);
        static T cbrt(const T& a);
        static T ceil(const T& a);
        static T cos(const T& a);
        static T cosh(const T& a);
        static T degToRad(const T& a);
        static T exp(const T& a);
        static T floor(const T& a);
        static T hypot(const T& x, const T& y);
        static T log(const T& a);
        static T log10(const T& a);
        static T mod(const T& numer, const T& denom);
        static T pi();
        static T e();
        static T pow(const T& body, const T& expon);
        static T radToDeg(const T& a);
        static T round(const T& a);
        static T sign(const T& a);
        static T sin(const T& a);
        static T sinh(const T& a);
        static T sqr(const T& a);
        static T sqrt(const T& a);
        static T tan(const T& a);
        static T tanh(const T& a);

        //**************************************************************************
        // LIMIT FUNCTIONS
        //**************************************************************************
        static T min();
        static T max();
        static T epsilon();
        static bool isFloat();
    };

    //******************************************************************************
    // TEMPLATE AND INLINE FUNCTIONS
    //******************************************************************************
    // First the normal template stuff.
    // These will be used unless a specific
    // implementation is found
    template <typename T> inline T Math<T>::abs(const T& a)
    {
        return (T)(::fabs((double)a));
    }

    //******************************************************************************
    template <typename T> inline T Math<T>::acos(const T& a)
    {
        return (T)(::acos((double)a));
    }

    //******************************************************************************
    template <typename T> inline T Math<T>::asin(const T& a)
    {
        return (T)(::asin((double)a));
    }

    //******************************************************************************
    template <typename T> inline T Math<T>::atan(const T& a)
    {
        return (T)(::atan((double)a));
    }

    //******************************************************************************
    template <typename T> inline T Math<T>::atan2(const T& y, const T& x)
    {
        return (T)(::atan2((double)y, (double)x));
    }

    //******************************************************************************
    template <typename T> inline T Math<T>::cbrt(const T& a)
    {
        return (T)(::cbrt((double)a));
    }

    //******************************************************************************
    template <typename T> inline T Math<T>::ceil(const T& a)
    {
        return (T)(::ceil((double)a));
    }

    //******************************************************************************
    template <typename T> inline T Math<T>::cos(const T& a)
    {
        return (T)(::cos((double)a));
    }

    //******************************************************************************
    template <typename T> inline T Math<T>::cosh(const T& a)
    {
        return (T)(::cosh((double)a));
    }

    //******************************************************************************
    template <typename T> inline T Math<T>::degToRad(const T& deg)
    {
        return (T)(((double)deg) * ((double)M_PI_OVER_180));
    }

    //******************************************************************************
    template <typename T> inline T Math<T>::exp(const T& a)
    {
        return (T)(::exp((double)a));
    }

    //******************************************************************************
    template <typename T> inline T Math<T>::floor(const T& a)
    {
        return (T)(::floor((double)a));
    }

    //******************************************************************************
    template <typename T> inline T Math<T>::hypot(const T& x, const T& y)
    {
        return (T)(::hypot((double)x, (double)y));
    }

    //******************************************************************************
    template <typename T> inline T Math<T>::log(const T& a)
    {
        return (T)(::log((double)a));
    }

    //******************************************************************************
    template <typename T> inline T Math<T>::log10(const T& a)
    {
        return (T)(::log10((double)a));
    }

    //******************************************************************************
    template <typename T> inline T Math<T>::mod(const T& numer, const T& denom)
    {
        return (T)(::fmod((double)numer, (double)denom));
    }

    //******************************************************************************
    template <typename T> inline T Math<T>::pi() { return (T)M_PI; }

    //******************************************************************************
    template <typename T> inline T Math<T>::e() { return (T)M_E; }

    //******************************************************************************
    template <typename T> inline T Math<T>::pow(const T& body, const T& expon)
    {
        return (T)(::pow((double)body, (double)expon));
    }

    //******************************************************************************
    template <typename T> inline T Math<T>::radToDeg(const T& rad)
    {
        return (T)(((double)rad) * ((double)M_180_OVER_PI));
    }

    //******************************************************************************
    template <typename T> inline T Math<T>::round(const T& dec)
    {
        return Math<T>::floor(dec + (T)0.5);
    }

    //******************************************************************************
    template <typename T> inline T Math<T>::sign(const T& a)
    {
        if (a < (T)0)
        {
            return (T)-1;
        }
        else if (a == (T)0)
        {
            return (T)0;
        }
        else
        {
            return (T)1;
        }
    }

    //******************************************************************************
    template <typename T> inline T Math<T>::sin(const T& a)
    {
        return (T)(::sin((double)a));
    }

    //******************************************************************************
    template <typename T> inline T Math<T>::sinh(const T& a)
    {
        return (T)(::sinh((double)a));
    }

    //******************************************************************************
    template <typename T> inline T Math<T>::sqr(const T& a) { return a * a; }

    //******************************************************************************
    template <typename T> inline T Math<T>::sqrt(const T& a)
    {
        return (T)(::sqrt((double)a));
    }

    //******************************************************************************
    template <typename T> inline T Math<T>::tan(const T& a)
    {
        return (T)(::tan((double)a));
    }

    //******************************************************************************
    template <typename T> inline T Math<T>::tanh(const T& a)
    {
        return (T)(::tanh((double)a));
    }

//******************************************************************************
//******************************************************************************
// FLOAT SPECIALIZATION
//******************************************************************************
//******************************************************************************
#ifdef TWK_NO_FLOAT_INTRINSICS
    template <> inline float Math<float>::abs(const float& a)
    {
        return ::fabs(a);
    }

    //******************************************************************************
    template <> inline float Math<float>::acos(const float& a)
    {
        return ::acos(a);
    }

    //******************************************************************************
    template <> inline float Math<float>::asin(const float& a)
    {
        return ::asin(a);
    }

    //******************************************************************************
    template <> inline float Math<float>::atan(const float& a)
    {
        return ::atan(a);
    }

    //******************************************************************************
    template <> inline float Math<float>::atan2(const float& y, const float& x)
    {
        return ::atan2(y, x);
    }

    //******************************************************************************
    template <> inline float Math<float>::cbrt(const float& a)
    {
        return ::cbrt(a);
    }

    //******************************************************************************
    template <> inline float Math<float>::ceil(const float& a)
    {
        return ::ceil(a);
    }

    //******************************************************************************
    template <> inline float Math<float>::cos(const float& a)
    {
        return ::cos(a);
    }

    //******************************************************************************
    template <> inline float Math<float>::cosh(const float& a)
    {
        return ::cosh(a);
    }

    //******************************************************************************
    template <> inline float Math<float>::exp(const float& a)
    {
        return ::exp(a);
    }

    //******************************************************************************
    template <> inline float Math<float>::floor(const float& a)
    {
        return ::floor(a);
    }

    //******************************************************************************
    template <> inline float Math<float>::hypot(const float& x, const float& y)
    {
        // Some implementations don't seem to have hypot, and in others,
        // such as the SGI implementation, the sqrt technique is faster
        // anyway.
        return ::sqrt((x * x) + (y * y));
    }

    //******************************************************************************
    template <> inline float Math<float>::log(const float& a)
    {
        return ::log(a);
    }

    //******************************************************************************
    template <> inline float Math<float>::log10(const float& a)
    {
        return ::log10(a);
    }

    //******************************************************************************
    template <>
    inline float Math<float>::mod(const float& numer, const float& denom)
    {
        return ::fmod(numer, denom);
    }

    //******************************************************************************
    template <>
    inline float Math<float>::pow(const float& body, const float& expon)
    {
        return ::pow(body, expon);
    }

    //******************************************************************************
    template <> inline float Math<float>::sin(const float& a)
    {
        return ::sin(a);
    }

    //******************************************************************************
    template <> inline float Math<float>::sinh(const float& a)
    {
        return ::sinh(a);
    }

    //******************************************************************************
    template <> inline float Math<float>::sqrt(const float& a)
    {
        return ::sqrt(a);
    }

    //******************************************************************************
    template <> inline float Math<float>::tan(const float& a)
    {
        return ::tan(a);
    }

    //******************************************************************************
    template <> inline float Math<float>::tanh(const float& a)
    {
        return ::tanh(a);
    }

#else
    template <> inline float Math<float>::abs(const float& a)
    {
        return ::fabsf(a);
    }

    //******************************************************************************
    template <> inline float Math<float>::acos(const float& a)
    {
        return ::acosf(a);
    }

    //******************************************************************************
    template <> inline float Math<float>::asin(const float& a)
    {
        return ::asinf(a);
    }

    //******************************************************************************
    template <> inline float Math<float>::atan(const float& a)
    {
        return ::atanf(a);
    }

    //******************************************************************************
    template <> inline float Math<float>::atan2(const float& y, const float& x)
    {
        return ::atan2f(y, x);
    }

    //******************************************************************************
    template <> inline float Math<float>::cbrt(const float& a)
    {
        /* ajg - no cubed root */
#if !defined _MSC_VER
        return ::cbrtf(a);
#else
        return 0;
#endif
    }

    //******************************************************************************
    template <> inline float Math<float>::ceil(const float& a)
    {
        return ::ceilf(a);
    }

    //******************************************************************************
    template <> inline float Math<float>::cos(const float& a)
    {
        return ::cosf(a);
    }

    //******************************************************************************
    template <> inline float Math<float>::cosh(const float& a)
    {
        return ::coshf(a);
    }

    //******************************************************************************
    template <> inline float Math<float>::exp(const float& a)
    {
        return ::expf(a);
    }

    //******************************************************************************
    template <> inline float Math<float>::floor(const float& a)
    {
        return ::floorf(a);
    }

    //******************************************************************************
    template <> inline float Math<float>::hypot(const float& x, const float& y)
    {
        // Some implementations don't seem to have hypot, and in others,
        // such as the SGI implementation, the sqrt technique is faster
        // anyway.
        return ::sqrtf((x * x) + (y * y));
    }

    //******************************************************************************
    template <> inline float Math<float>::log(const float& a)
    {
        return ::logf(a);
    }

    //******************************************************************************
    template <> inline float Math<float>::log10(const float& a)
    {
        return ::log10f(a);
    }

    //******************************************************************************
    template <>
    inline float Math<float>::mod(const float& numer, const float& denom)
    {
        return ::fmodf(numer, denom);
    }

    //******************************************************************************
    template <>
    inline float Math<float>::pow(const float& body, const float& expon)
    {
        return ::powf(body, expon);
    }

    //******************************************************************************
    template <> inline float Math<float>::sin(const float& a)
    {
        return ::sinf(a);
    }

    //******************************************************************************
    template <> inline float Math<float>::sinh(const float& a)
    {
        return ::sinhf(a);
    }

    //******************************************************************************
    template <> inline float Math<float>::sqrt(const float& a)
    {
        return ::sqrtf(a);
    }

    //******************************************************************************
    template <> inline float Math<float>::tan(const float& a)
    {
        return ::tanf(a);
    }

    //******************************************************************************
    template <> inline float Math<float>::tanh(const float& a)
    {
        return ::tanhf(a);
    }

#endif // TWK_NO_FLOAT_INTRINSICS

    //******************************************************************************
    template <> inline float Math<float>::degToRad(const float& deg)
    {
        return deg * (float)M_PI_OVER_180;
    }

    //******************************************************************************
    template <> inline float Math<float>::radToDeg(const float& rad)
    {
        return rad * (float)M_180_OVER_PI;
    }

    //******************************************************************************
    //******************************************************************************
    // INTEGER TYPE SPECIALIZATIONS
    //******************************************************************************
    //******************************************************************************
    template <> inline char Math<char>::abs(const char& v)
    {
        return (v < 0) ? -v : v;
    }

    //******************************************************************************
    template <> inline char Math<char>::ceil(const char& v) { return v; }

    //******************************************************************************
    template <> inline char Math<char>::floor(const char& v) { return v; }

    //******************************************************************************
    template <>
    inline char Math<char>::mod(const char& numer, const char& denom)
    {
        return numer % denom;
    }

    //******************************************************************************
    //******************************************************************************
    template <>
    inline unsigned char Math<unsigned char>::abs(const unsigned char& v)
    {
        return v;
    }

    //******************************************************************************
    template <>
    inline unsigned char Math<unsigned char>::ceil(const unsigned char& v)
    {
        return v;
    }

    //******************************************************************************
    template <>
    inline unsigned char Math<unsigned char>::floor(const unsigned char& v)
    {
        return v;
    }

    //******************************************************************************
    template <>
    inline unsigned char Math<unsigned char>::mod(const unsigned char& numer,
                                                  const unsigned char& denom)
    {
        return numer % denom;
    }

    //******************************************************************************
    //******************************************************************************
    template <> inline short Math<short>::abs(const short& v)
    {
        return (v < 0) ? -v : v;
    }

    //******************************************************************************
    template <> inline short Math<short>::ceil(const short& v) { return v; }

    //******************************************************************************
    template <> inline short Math<short>::floor(const short& v) { return v; }

    //******************************************************************************
    template <>
    inline short Math<short>::mod(const short& numer, const short& denom)
    {
        return numer % denom;
    }

    //******************************************************************************
    //******************************************************************************
    template <>
    inline unsigned short Math<unsigned short>::abs(const unsigned short& v)
    {
        return v;
    }

    //******************************************************************************
    template <>
    inline unsigned short Math<unsigned short>::ceil(const unsigned short& v)
    {
        return v;
    }

    //******************************************************************************
    template <>
    inline unsigned short Math<unsigned short>::floor(const unsigned short& v)
    {
        return v;
    }

    //******************************************************************************
    template <>
    inline unsigned short Math<unsigned short>::mod(const unsigned short& numer,
                                                    const unsigned short& denom)
    {
        return numer % denom;
    }

    //******************************************************************************
    //******************************************************************************
    template <> inline int Math<int>::abs(const int& v)
    {
        return (v < 0) ? -v : v;
    }

    //******************************************************************************
    template <> inline int Math<int>::ceil(const int& v) { return v; }

    //******************************************************************************
    template <> inline int Math<int>::floor(const int& v) { return v; }

    //******************************************************************************
    template <> inline int Math<int>::mod(const int& numer, const int& denom)
    {
        return numer % denom;
    }

    //******************************************************************************
    //******************************************************************************
    template <>
    inline unsigned int Math<unsigned int>::abs(const unsigned int& v)
    {
        return v;
    }

    //******************************************************************************
    template <>
    inline unsigned int Math<unsigned int>::ceil(const unsigned int& v)
    {
        return v;
    }

    //******************************************************************************
    template <>
    inline unsigned int Math<unsigned int>::floor(const unsigned int& v)
    {
        return v;
    }

    //******************************************************************************
    template <>
    inline unsigned int Math<unsigned int>::mod(const unsigned int& numer,
                                                const unsigned int& denom)
    {
        return numer % denom;
    }

    //******************************************************************************
    //******************************************************************************
    template <> inline long Math<long>::abs(const long& v)
    {
        return (v < 0) ? -v : v;
    }

    //******************************************************************************
    template <> inline long Math<long>::ceil(const long& v) { return v; }

    //******************************************************************************
    template <> inline long Math<long>::floor(const long& v) { return v; }

    //******************************************************************************
    template <>
    inline long Math<long>::mod(const long& numer, const long& denom)
    {
        return numer % denom;
    }

    //******************************************************************************
    //******************************************************************************
    template <>
    inline unsigned long Math<unsigned long>::abs(const unsigned long& v)
    {
        return v;
    }

    //******************************************************************************
    template <>
    inline unsigned long Math<unsigned long>::ceil(const unsigned long& v)
    {
        return v;
    }

    //******************************************************************************
    template <>
    inline unsigned long Math<unsigned long>::floor(const unsigned long& v)
    {
        return v;
    }

    //******************************************************************************
    template <>
    inline unsigned long Math<unsigned long>::mod(const unsigned long& numer,
                                                  const unsigned long& denom)
    {
        return numer % denom;
    }

    //******************************************************************************
    //******************************************************************************
    // LIMITS STUFF - may someday be removed...
    //******************************************************************************
    //******************************************************************************
    // char
    template <> inline char Math<char>::min() { return CHAR_MIN; }

    //******************************************************************************
    template <> inline char Math<char>::max() { return CHAR_MAX; }

    //******************************************************************************
    template <> inline char Math<char>::epsilon() { return 1; }

    //******************************************************************************
    template <> inline bool Math<char>::isFloat() { return false; }

    //******************************************************************************
    //******************************************************************************
    // unsigned char
    template <> inline unsigned char Math<unsigned char>::min() { return 0; }

    //******************************************************************************
    template <> inline unsigned char Math<unsigned char>::max()
    {
        return UCHAR_MAX;
    }

    //******************************************************************************
    template <> inline unsigned char Math<unsigned char>::epsilon()
    {
        return 1;
    }

    //******************************************************************************
    template <> inline bool Math<unsigned char>::isFloat() { return false; }

    //******************************************************************************
    //******************************************************************************
    // short
    template <> inline short Math<short>::min() { return SHRT_MIN; }

    //******************************************************************************
    template <> inline short Math<short>::max() { return SHRT_MAX; }

    //******************************************************************************
    template <> inline short Math<short>::epsilon() { return 1; }

    //******************************************************************************
    template <> inline bool Math<short>::isFloat() { return false; }

    //******************************************************************************
    //******************************************************************************
    // unsigned short
    template <> inline unsigned short Math<unsigned short>::min() { return 0; }

    //******************************************************************************
    template <> inline unsigned short Math<unsigned short>::max()
    {
        return USHRT_MAX;
    }

    //******************************************************************************
    template <> inline unsigned short Math<unsigned short>::epsilon()
    {
        return 1;
    }

    //******************************************************************************
    template <> inline bool Math<unsigned short>::isFloat() { return false; }

    //******************************************************************************
    //******************************************************************************
    // int
    template <> inline int Math<int>::min() { return INT_MIN; }

    //******************************************************************************
    template <> inline int Math<int>::max() { return INT_MAX; }

    //******************************************************************************
    template <> inline int Math<int>::epsilon() { return 1; }

    //******************************************************************************
    template <> inline bool Math<int>::isFloat() { return false; }

    //******************************************************************************
    //******************************************************************************
    // unsigned int
    template <> inline unsigned int Math<unsigned int>::min() { return 0; }

    //******************************************************************************
    template <> inline unsigned int Math<unsigned int>::max()
    {
        return UINT_MAX;
    }

    //******************************************************************************
    template <> inline unsigned int Math<unsigned int>::epsilon() { return 1; }

    //******************************************************************************
    template <> inline bool Math<unsigned int>::isFloat() { return false; }

    //******************************************************************************
    //******************************************************************************
    // long
    template <> inline long Math<long>::min() { return LONG_MIN; }

    //******************************************************************************
    template <> inline long Math<long>::max() { return LONG_MAX; }

    //******************************************************************************
    template <> inline long Math<long>::epsilon() { return 1; }

    //******************************************************************************
    template <> inline bool Math<long>::isFloat() { return false; }

    //******************************************************************************
    //******************************************************************************
    // unsigned long
    template <> inline unsigned long Math<unsigned long>::min() { return 0; }

    //******************************************************************************
    template <> inline unsigned long Math<unsigned long>::max()
    {
        return ULONG_MAX;
    }

    //******************************************************************************
    template <> inline unsigned long Math<unsigned long>::epsilon()
    {
        return 1;
    }

    //******************************************************************************
    template <> inline bool Math<unsigned long>::isFloat() { return false; }

    //******************************************************************************
    //******************************************************************************
    // float
    template <> inline float Math<float>::min() { return -FLT_MAX; }

    //******************************************************************************
    template <> inline float Math<float>::max() { return FLT_MAX; }

    //******************************************************************************
    template <> inline float Math<float>::epsilon() { return FLT_EPSILON; }

    //******************************************************************************
    template <> inline bool Math<float>::isFloat() { return true; }

    //******************************************************************************
    //******************************************************************************
    // double
    template <> inline double Math<double>::min() { return -DBL_MAX; }

    //******************************************************************************
    template <> inline double Math<double>::max() { return DBL_MAX; }

    //******************************************************************************
    template <> inline double Math<double>::epsilon() { return DBL_EPSILON; }

    //******************************************************************************
    template <> inline bool Math<double>::isFloat() { return true; }

} // End namespace TwkMath

#endif
