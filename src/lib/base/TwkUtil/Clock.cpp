//******************************************************************************
// Copyright (c) 2001-2019 Autodesk Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkUtil/Clock.h>

namespace TwkUtil
{

    Timer SystemClock::timer(true);

    SystemClock::SystemClock() {}

    SystemClock::~SystemClock() {}

    SystemClock::Time SystemClock::now() const
    {
        struct timeval tv;
        if (timer.gettimeofdayWrapper(&tv) != 0)
            return 0;

        return tv.tv_sec + tv.tv_usec / 1e6;
    }

} // namespace TwkUtil
