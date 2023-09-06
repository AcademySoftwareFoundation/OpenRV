//******************************************************************************
// Copyright (c) 2023 Autodesk Inc. All rights reserved.
// 
// SPDX-License-Identifier: Apache-2.0
// 
//******************************************************************************

#include <TwkUtil/FileLogger.h>
#include <TwkUtil/EnvVar.h>
#include <QtCore/QtCore>
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <boost/algorithm/string.hpp>

namespace TwkUtil {

    static ENVVAR_STRING(evFileLogLevel, "RV_FILE_LOG_LEVEL", "debug");
    static ENVVAR_INT(evFileLogSize, "RV_FILE_LOG_SIZE", 1024*1024*5); // Default: 5MB
    static ENVVAR_INT(evFileLogNumFiles, "RV_FILE_LOG_NUM_FILES", 2);

    FileLogger::FileLogger()
    {
    #if defined(PLATFORM_DARWIN)
        std::string logFilePath = QDir::homePath().toStdString() + "/Library/Logs/" + QCoreApplication::organizationName().toStdString() + "/";
    #else
        std::string logFilePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation).toStdString() +  "/";
    #endif

        std::string name = QCoreApplication::applicationName().toStdString();
        m_logger = spdlog::rotating_logger_mt<spdlog::async_factory>(
            name,
            logFilePath.append(name + ".log"),
            evFileLogSize.getValue(),
            evFileLogNumFiles.getValue()
        ).get();

        setLogLevel(evFileLogLevel.getValue());

        m_logger->info("============ SESSION START ============");
    }

    FileLogger::~FileLogger() {
        m_logger->info("============  SESSION END  ============");        
    }

    void FileLogger::logToFile(spdlog::level::level_enum lineLevel, std::string& line)
    {
        // Remove the unecessary newline for the log file (it would 'double space' each line in the log)
        boost::trim_right(line);
        switch (lineLevel)
        {
            case spdlog::level::debug: m_logger->debug(line); break;
            case spdlog::level::info: m_logger->info(line); break;
            case spdlog::level::warn: m_logger->warn(line); break;
            case spdlog::level::err: m_logger->error(line); break;
        }
    }

    void FileLogger::setLogLevel(const std::string& level)
    {
        if (level == "trace")
        {
            m_logger->set_level(spdlog::level::trace);
        }
        else if (level == "debug" || level == "deb")
        {
            m_logger->set_level(spdlog::level::debug);
        }
        else if (level == "info")
        {
            m_logger->set_level(spdlog::level::info);
        }
        else if (level ==  "warning" || level == "warn")
        {
            m_logger->set_level(spdlog::level::warn);
        }
        else if (level == "error")
        {
            m_logger->set_level(spdlog::level::err);
        }
        else if (level == "critical")
        {
            m_logger->set_level(spdlog::level::critical);
        }
    }
}
