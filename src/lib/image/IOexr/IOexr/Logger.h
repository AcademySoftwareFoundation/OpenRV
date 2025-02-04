//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
/*
 *  Logger.h
 *
 */
#ifndef _LOGGER_H
#define _LOGGER_H

#define LOG_NONE (0)
#define LOG_USER_ERROR (1)
#define LOG_USER_WARNING (2)
#define LOG_CRITICAL_ERROR (3)
#define LOG_MINOR_ERROR (4)
#define LOG_ALWAYS (6)
#define LOG_DEBUG (8)
#define LOG_DEBUG_VERBOSE (9)

#define LOG ::Logger::Instance()
#define MAX_LOG_LINE (2048)

class Logger
{
public:
    /**
     * Return the singleton instance of the logger.
     *
     * Logger object is created if necessary
     *
     * @return Logger instance
     */
    static Logger& Instance()
    {
        static Logger theSingleton;
        return theSingleton;
    }

    /**
     * Sets the loggling level
     *
     * @param defaultLevel
     */
    void setLogLevel(int defaultLevel);

    /**
     * Log a message.
     *
     * This uses a printf()-style formatted string.
     *
     * @param level The log level to log at
     * @param format The format of the message
     */
    void log(int level, const char* format, ...);
    void log(const char* format, ...);

    /**
     * Returns the current log level
     *
     * @return int log level
     */
    int getLogLevel() { return mLogLevel; }

private:
    Logger();                         // ctor hidden
    Logger(Logger const&);            // copy ctor hidden
    Logger& operator=(Logger const&); // assign op. hidden
    ~Logger();                        // dtor hidden

    int mLogLevel; /**< current log level */
};

#endif // _LOGGER_H
