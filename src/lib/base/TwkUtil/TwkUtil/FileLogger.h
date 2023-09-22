//******************************************************************************
// Copyright (c) 2023 Autodesk Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkUtilLog_h_
#define _TwkUtilLog_h_

#include <string>
#include "spdlog/logger.h"
#include "TwkUtil/dll_defs.h"

namespace TwkUtil
{

  class TWKUTIL_EXPORT FileLogger
  {
   public:
    FileLogger();
    ~FileLogger();

    void logToFile( spdlog::level::level_enum lineLevel, std::string& line );

   private:
    spdlog::logger* m_logger;

    void setLogLevel( const std::string& level );
  };

}  // namespace TwkUtil

#endif
