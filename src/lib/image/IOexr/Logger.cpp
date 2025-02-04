//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
/*
 *  Logger.cpp
 *
 */

#include <IOexr/Logger.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#ifndef NDEBUG
#ifndef DEBUG_IOEXR
// #define DEBUG_IOEXR
#endif
#endif

Logger::Logger() { mLogLevel = LOG_DEBUG_VERBOSE; }

Logger::Logger(Logger const& cpy) { mLogLevel = cpy.mLogLevel; }

Logger& Logger::operator=(Logger const& /*eq*/) { return Instance(); }

Logger::~Logger() {}

void Logger::setLogLevel(int defaultLevel) { mLogLevel = defaultLevel; }

void Logger::log(const char* format, ...)
{
#ifdef DEBUG_IOEXR
    if (mLogLevel < LOG_DEBUG_VERBOSE)
#endif
        return;

    // create a local buffer to handle the string formatting
    char buffer[MAX_LOG_LINE];
    memset(buffer, 0, MAX_LOG_LINE * sizeof(char));

    va_list list;
    va_start(list, format);

#ifdef _MSC_VER // if compiling via Visual studio
    int len = 0;
    len = _vscprintf(format, list) // _vscprintf doesn't count
          + 1;                     // terminating '\0'

    vsprintf_s(buffer, MAX_LOG_LINE, format, list);
#else
    vsnprintf(buffer, MAX_LOG_LINE, format, list);
#endif
    va_end(list);

    size_t outLen = strlen(buffer);
    if (MAX_LOG_LINE - outLen > 3)
    {
        buffer[outLen] = '\r';
        buffer[outLen + 1] = '\n';
        buffer[outLen + 2] = 0;
    }
    else
    {
        buffer[outLen - 2] = '\r';
        buffer[outLen - 1] = '\n';
        buffer[outLen] = 0;
    }

    fprintf(stderr, "%s", buffer);
    fflush(stderr);
}

void Logger::log(int level, const char* format, ...)
{
#ifdef DEBUG_IOEXR
    if (mLogLevel < LOG_DEBUG_VERBOSE)
#endif
        return;

    // create a local buffer to handle the string formatting
    char buffer[MAX_LOG_LINE];
    memset(buffer, 0, MAX_LOG_LINE * sizeof(char));

    va_list list;
    va_start(list, format);

#ifdef _MSC_VER // if compiling via Visual studio
    int len = 0;
    len = _vscprintf(format, list) // _vscprintf doesn't count
          + 1;                     // terminating '\0'

    vsprintf_s(buffer, MAX_LOG_LINE, format, list);
#else
    vsnprintf(buffer, MAX_LOG_LINE, format, list);
#endif
    va_end(list);

    size_t outLen = strlen(buffer);
    if (MAX_LOG_LINE - outLen > 3)
    {
        buffer[outLen] = '\r';
        buffer[outLen + 1] = '\n';
        buffer[outLen + 2] = 0;
    }
    else
    {
        buffer[outLen - 2] = '\r';
        buffer[outLen - 1] = '\n';
        buffer[outLen] = 0;
    }

    fprintf(stderr, "%s", buffer);
    fflush(stderr);
}
