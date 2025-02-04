//******************************************************************************
// Copyright (c) 2001-2019 Autodesk Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkUtilClock_h_
#define _TwkUtilClock_h_

#include <TwkUtil/dll_defs.h>
#include <TwkUtil/Timer.h>

namespace TwkUtil
{

    class TWKUTIL_EXPORT SystemClock
    {
    public:
        typedef double Time;

        SystemClock();
        ~SystemClock();

        // Returns the number of seconds since EPOC.
        Time now() const;

    private:
        static Timer timer;
    };

} // namespace TwkUtil

#endif
