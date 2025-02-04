//******************************************************************************
// Copyright (c) 2001-2002 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef __TWKUTILTEXTMSG_H__
#define __TWKUTILTEXTMSG_H__

#include <assert.h>
#include <string.h>
#include <iostream>
#include <TwkUtil/dll_defs.h>

namespace TwkUtil
{

    enum MsgTag
    {
        DEBUG,
        DEBUG1,
        DEBUG2,
        DEBUG3,
        DEBUG4,
        INFO,
        WARNING,
        ERROR,
        NUM_TAGS
    };

    class TWKUTIL_EXPORT TextMsg : public ostream
    {
    public:
        static void enable(int tag);
        static void disable(int tag);
        static bool isEnabled(int tag);

        static void setTagPrefix(int tag, char* str);
        static void setAllPrefix(const char* str);

        static void printf(int tag, const char* format, ...);
        static void printf(const char* format, ...);
        static void setOutFunction(void (*outFunc)(const char*));
        static void setOutStream(std::ostream& outStream);

    protected:
        static std::ostream* m_outStream;
        static void (*m_outFunc)(const char*);

        static bool m_tags[NUM_TAGS];
        static bool m_printTagPrefix;
        static char m_allPrefix[32];
        static char* m_tagStr[NUM_TAGS];

    private:
        // Disallow user-created instances of this class
        TextMsg() {}
    };

    //******************************************************************************
    // INLINE FUNCTIONS
    //******************************************************************************
    inline void TextMsg::enable(int tag)
    {
        assert(tag >= 0 && tag < NUM_TAGS);
        m_tags[tag] = true;
    }

    inline void TextMsg::disable(int tag)
    {
        assert(tag >= 0 && tag < NUM_TAGS);
        m_tags[tag] = false;
    }

    inline bool TextMsg::isEnabled(int tag)
    {
        assert(tag >= 0 && tag < NUM_TAGS);
        return m_tags[tag];
    }

    inline void TextMsg::setAllPrefix(const char* str)
    {
        strncpy(m_allPrefix, str, 32);
    }

    inline void TextMsg::setOutFunction(void (*outFunc)(const char*))
    {
        m_outFunc = outFunc;
        m_outStream = NULL;
    }

    inline void TextMsg::setOutStream(std::ostream& outStream)
    {
        m_outStream = &outStream;
        m_outFunc = NULL;
    }

} // End namespace TwkUtil

#endif // End #ifdef __TWKUTILTEXTMSG_H__
