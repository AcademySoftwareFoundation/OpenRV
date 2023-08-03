//******************************************************************************
// Copyright (c) 2023 Autodesk Inc. All rights reserved.
// 
// SPDX-License-Identifier: Apache-2.0
// 
//******************************************************************************

#ifndef _TwkUtilLog_h_
#define _TwkUtilLog_h_

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG

#include <TwkUtil/dll_defs.h>
#include <spdlog/logger.h>
#include <string>
#include <iostream>

namespace TwkUtil {

class TWKUTIL_EXPORT FileLogger
{
public:
    FileLogger();
    ~FileLogger();

    void logToFile(spdlog::level::level_enum lineLevel, std::string& line);
private:
    spdlog::logger*             m_logger;

    void setLogLevel(const std::string& level);
};

}

#endif