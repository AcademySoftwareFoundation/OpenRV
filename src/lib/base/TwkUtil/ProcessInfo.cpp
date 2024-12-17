//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <TwkUtil/ProcessInfo.h>

#ifdef PLATFORM_WINDOWS
#include "windows.h"
#else
#include <unistd.h>
#endif

namespace TwkUtil
{

#ifdef PLATFORM_WINDOWS

    size_t processID() { return size_t(GetCurrentProcessId()); }

#else

    size_t processID() { return size_t(getpid()); }

#endif

} // namespace TwkUtil
