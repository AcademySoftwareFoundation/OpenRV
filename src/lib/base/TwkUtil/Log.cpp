//******************************************************************************
// Copyright (c) 2001-2019 Autodesk Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkUtil/Log.h>
#include <TwkUtil/Timer.h>

#include <cmath>

#ifdef WIN32
#define snprintf _snprintf
#endif

namespace TwkUtil
{

    Log::Log() {}

    Log::Log(const std::string& moduleName, LogLevel level)
    {
        this->level = level;

        operator<<(timestampAsString() + ':');
        if (!moduleName.empty())
            operator<<(moduleName + ':');

        operator<<(asString(level) + ':');
    }

    Log::~Log()
    {
        if (opened)
        {
            std::cout << std::endl;
        }
    }

    std::string Log::asString(LogLevel level)
    {
        std::string label;
        switch (level)
        {
        case DebugLogLevel:
            label = "DEBUG";
            break;
        case InfoLogLevel:
            label = "INFO ";
            break;
        case WarnLogLevel:
            label = "WARN ";
            break;
        case ErrorLogLevel:
            label = "ERROR";
            break;
        }
        return label;
    }

    std::string Log::timestampAsString() const
    {
        double now = clock.now();

        double intPart = 0;
        double fractPart = modf(now, &intPart);

        time_t t = (time_t)intPart;
        tm* ptm = localtime(&t);

        char buffer[100];
        snprintf(buffer, sizeof(buffer), "%04d/%02d/%02d %02d:%02d:%02d.%06d",
                 ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday,
                 ptm->tm_hour, ptm->tm_min, ptm->tm_sec,
                 (int)(fractPart * 1e6));

        return buffer;
    }

} // namespace TwkUtil
