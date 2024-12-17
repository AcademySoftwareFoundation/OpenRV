///*****************************************************************************/
// Copyright (c) 2019 Autodesk, Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//*****************************************************************************/

#ifndef __TwkUtil__Macros__h__
#define __TwkUtil__Macros__h__

/*==============================================================================
 * EXTERNAL DECLARATIONS
 *============================================================================*/

#include <cmath>
#include <cstdio>

//=============================================================================
// UTILITY MACROS

#define SMALL_NUMBER 0.0001f

template <typename T> inline T Fabs(T v) { return fabsf(float(v)); }

template <> inline double Fabs<double>(double v) { return fabs(v); }

#define EQUAL_TOL(a, b, tol) (Fabs((a) - (b)) < tol)
#define EQUAL(a, b) EQUAL_TOL(a, b, SMALL_NUMBER)

#ifndef ROUND
#define ROUND(x) ((int)((x) + 0.5))
#endif

/* Handle signed arguments properly */
#ifndef ROUNDS
#define ROUNDS(v) ((v) >= 0.0 ? (int)((v) + 0.5) : (int)((v) - 0.5))
#endif

/* Round up to the next multiple of b (works only for int types) */
#ifndef ROUNDUP
#define ROUNDUP(a, b) (((a + b - 1) / b) * b)
#endif

// Assert only if in internal build.
//
#define RV_NOT_USED(expr)   \
    do                      \
    {                       \
        (void)sizeof(expr); \
    } while (0)
#ifdef INTERNAL
#define RV_ASSERT_INTERNAL(expr) assert((expr))
#else
#define RV_ASSERT_INTERNAL(expr) RV_NOT_USED(expr)
#endif

#ifndef RV_STRING
#define RV_STRING(s) #s
#endif

//------------------------------------------------------------------------------
//
#define RV_LOG(format, ...)                                              \
    {                                                                    \
        if (evDebug.getValue())                                          \
        {                                                                \
            char dbgString[1024];                                        \
            snprintf(dbgString, sizeof(dbgString), format, __VA_ARGS__); \
            dbgString[sizeof(dbgString) - 1] = 0x0;                      \
            std::cout << dbgString << std::endl;                         \
        }                                                                \
    }

#endif // __TwkUtil__Macros__h__
