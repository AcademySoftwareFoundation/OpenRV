//******************************************************************************
// Copyright (c) 2001-2019 Autodesk Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkUtilLog_h_
#define _TwkUtilLog_h_

#include <string>
#include <iostream>
#include <TwkUtil/Clock.h>
#include <TwkUtil/dll_defs.h>

namespace TwkUtil
{

    enum LogLevel
    {
        DebugLogLevel,
        InfoLogLevel,
        WarnLogLevel,
        ErrorLogLevel
    };

    class TWKUTIL_EXPORT Log
    {
    public:
        Log();
        Log(const std::string& moduleName, LogLevel level = InfoLogLevel);
        ~Log();

        template <class T> Log& operator<<(const T& msg)
        {
            std::cout << msg;

            opened = true;
            return *this;
        }

    private:
        bool opened = false;
        LogLevel level = DebugLogLevel;
        SystemClock clock;

        static std::string asString(LogLevel level);
        std::string timestampAsString() const;
    };

} // namespace TwkUtil

#endif
