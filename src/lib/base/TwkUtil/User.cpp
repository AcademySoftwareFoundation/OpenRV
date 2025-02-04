//******************************************************************************
// Copyright (c) 2001-2003 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkUtil/User.h>

#include <iostream>
#include <sstream>

#ifndef PLATFORM_WINDOWS
#include <sys/types.h>
#include <unistd.h>
#endif

#ifdef PLATFORM_WINDOWS
#undef _NTDDK_
#define _WIN32_WINNT 0x0501
#define WINVER 0x0501 // both combined stand for Windows XP
#include <windows.h>
// #include <lm.h>
#include <sddl.h>
#endif

namespace TwkUtil
{
    using namespace std;

#ifdef PLATFORM_WINDOWS
    string uidString()
    {
        LPTSTR str = 0;
        HANDLE h = GetCurrentProcess();
        HANDLE newH;

        if (!OpenProcessToken(h, TOKEN_QUERY, &newH))
        {
            std::cerr << "ERROR: OpenProcessToken failed: " << GetLastError()
                      << std::endl;
            return string();
        }

        PTOKEN_USER ptu;
        DWORD outSize;

        if (!GetTokenInformation(newH, TokenUser, 0, 0, &outSize))
        {
            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            {
                std::cerr << "ERROR: GetTokenInformation failed: "
                          << GetLastError() << std::endl;
                return string();
            }
        }

        ptu = (PTOKEN_USER)malloc(outSize);
        if (!GetTokenInformation(newH, TokenUser, ptu, outSize, &outSize))
        {
            std::cerr << "ERROR: GetTokenInformation failed: " << GetLastError()
                      << std::endl;
            free(ptu);
            return string();
        }

        ConvertSidToStringSid(ptu->User.Sid, &str);
        free(ptu);
        CloseHandle(newH);

        return string((char*)str);
    }
#else
    string uidString()
    {
        ostringstream str;
        str << getuid();
        return str.str();
    }
#endif

} // namespace TwkUtil
