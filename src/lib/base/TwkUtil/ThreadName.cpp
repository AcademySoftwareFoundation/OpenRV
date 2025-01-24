//
//  Copyright (c) 2014 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <pthread.h>
#if defined(PLATFORM_LINUX)
#include <sys/prctl.h>
#endif
#include <TwkUtil/ThreadName.h>

namespace TwkUtil
{
    using namespace std;

    void setThreadName(const string& name)
    {
#if defined(PLATFORM_APPLE_MACH_BSD)
        pthread_setname_np(name.c_str());
#endif

#if defined(PLATFORM_LINUX)
        prctl(PR_SET_NAME, (unsigned long)(name.c_str()), 0, 0, 0);
#endif
    }

    string getThreadName()
    {
        string s;
#if defined(PLATFORM_APPLE_MACH_BSD)
        char buf[32];
        pthread_getname_np(pthread_self(), buf, 32);
        s = buf;
#endif
        return s;
    }

} // namespace TwkUtil
