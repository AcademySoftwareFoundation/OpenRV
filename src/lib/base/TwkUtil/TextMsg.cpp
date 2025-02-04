//******************************************************************************
// Copyright (c) 2001-2002 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkUtil/TextMsg.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

namespace TwkUtil
{
    using namespace std;

    //
    // Class set-up (compile-time, I believe)
    //
    std::ostream* TextMsg::m_outStream = &std::cout;
    void (*TextMsg::m_outFunc)(const char*) = NULL;
    char TextMsg::m_allPrefix[32] = "";
    bool TextMsg::m_printTagPrefix = true;
    bool TextMsg::m_tags[NUM_TAGS] = {
        false, // DEBUG
        false, // DEBUG1
        false, // DEBUG2
        false, // DEBUG3
        false, // DEBUG4
        true,  // INFO
        true,  // WARNING
        true   // ERROR
    };

    char* TextMsg::m_tagStr[NUM_TAGS] = {
        "DEBUG   ", // DEBUG
        "DEBUG1  ", // DEBUG1
        "DEBUG2  ", // DEBUG2
        "DEBUG3  ", // DEBUG3
        "DEBUG4  ", // DEBUG4
        "INFO    ", // INFO
        "WARNING ", // WARNING
        "ERROR   "  // ERROR
    };
    bool m_printTagPrefix = true;

    //
    // Member functions...
    //
    void TextMsg::setTagPrefix(int tag, char* str)
    {
        assert(tag >= 0 && tag < NUM_TAGS);
        strncpy(m_tagStr[tag], str, 8);
    }

    void TextMsg::printf(int tag, const char* format, ...)
    {
        assert(tag >= 0 && tag < NUM_TAGS);
        if (!m_tags[tag])
        {
            return;
        }

        char* ubuffer;
        char* buffer;
        va_list args;

        va_start(args, format);
        vasprintf(&ubuffer, format, args); // TODO: make this POSIX-compliant
        va_end(args);

        if (m_allPrefix)
        {
            // TODO: make this POSIX-compliant
            asprintf(&buffer, "%s: %s: %s", m_allPrefix, m_tagStr[tag],
                     ubuffer);
        }
        else
        {
            // TODO: make this POSIX-compliant
            asprintf(&buffer, "%s: %s", m_tagStr[tag], ubuffer);
        }

        if (m_outStream)
        {
            *m_outStream << buffer;
        }
        else
        {
            assert(m_outFunc);
            m_outFunc(buffer);
        }
        free(buffer);
        free(ubuffer);
    }

    void TextMsg::printf(const char* format, ...)
    {
        char* buffer;
        va_list args;

        va_start(args, format);
        vasprintf(&buffer, format, args); // TODO: make this POSIX-compliant
        va_end(args);

        if (m_outStream)
        {
            *m_outStream << buffer;
        }
        else
        {
            assert(m_outFunc);
            m_outFunc(buffer);
        }
        free(buffer);
    }

} // End namespace TwkUtil
