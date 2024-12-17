//******************************************************************************
// Copyright (c) 2001-2003 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#if !defined(_MSC_VER) && !defined(PLATFORM_WINDOWS)
#include <unistd.h>
#endif

#include <TwkUtil/Daemon.h>

namespace TwkUtil
{
    //
    // From Advanced Programming in the UNIX Environment p.418
    //
    int daemonInit()
    {
#if !defined(_MSC_VER) && !defined(PLATFORM_WINDOWS)
        pid_t pid;

        if ((pid = fork()) < 0)
        {
            if ((pid = fork()) < 0)
            {
                return -1;
            }
            else if (pid != 0)
            {
                exit(0); // 1st child goes bye-bye
            }
        }
        else if (pid != 0)
        {
            exit(0); // Parent goes bye-bye
        }

        // 2nd child continues...
        setsid();

        // close open descriptors (OK; only 3 of them...)
        fclose(stdin);
        fclose(stdout);
        fclose(stderr);

        return pid;
#else
        abort();
        return 0;
#endif
    }

} //  End namespace TwkUtil
