//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkMath__TwkMathTime__h__
#define __TwkMath__TwkMathTime__h__
#include <TwkMath/Math.h>
#include <limits>

namespace TwkMath
{

// mR - max() macro conflicts with max() call below - dumb
#ifdef _MSC_VER
#ifdef max
#undef max
#endif
#endif

    typedef double Time;
    static const Time Always = std::numeric_limits<Time>::max();
    static const Time Never = -Always;

} // namespace TwkMath

#endif // __TwkMath__TwkMathTime__h__
