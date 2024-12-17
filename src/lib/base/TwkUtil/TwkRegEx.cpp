//******************************************************************************
// Copyright (c) 2001-2002 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkUtil/TwkRegEx.h>
#include <sstream>
#include <algorithm>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

using namespace std;

/* AJG - snprintf'isms */
#ifdef _MSC_VER
#define snprintf _snprintf
#endif

namespace TwkUtil
{

    //******************************************************************************
    //******************************************************************************
    // REGEX CLASS
    //******************************************************************************
    //******************************************************************************

    //******************************************************************************
    // Default constructor. Makes a regular expression that no-one could ever
    // match.
    RegEx::RegEx()
        : m_pattern(MATCH_NOTHING)
        , m_flags(REG_EXTENDED)
        , m_regexCompStatus(-1)
    {
        init();
    }

    //******************************************************************************
    // Regular constructors...
    RegEx::RegEx(const char* pattern, int flags)
        : m_pattern(pattern)
        , m_flags(flags)
        , m_regexCompStatus(-1)
    {
        init();
    }

    //******************************************************************************
    RegEx::RegEx(const std::string& pattern, int flags)
        : m_pattern(pattern)
        , m_flags(flags)
        , m_regexCompStatus(-1)
    {
        init();
    }

    //******************************************************************************
    // Copy constructor
    RegEx::RegEx(const RegEx& regex)
        : m_pattern(regex.m_pattern)
        , m_flags(regex.m_flags)
        , m_regexCompStatus(-1)
    {
        init();
    }

    //******************************************************************************
    RegEx::~RegEx()
    {
        if (m_regexCompStatus == 0)
        {
            regfree(&m_preg);
        }
    }

    //******************************************************************************
    // Assignment operator
    RegEx& RegEx::operator=(const RegEx& copy)
    {
        if (m_regexCompStatus == 0)
        {
            regfree(&m_preg);
        }
        m_pattern = copy.m_pattern;
        m_flags = copy.m_flags;
        m_regexCompStatus = -1;
        init();
        return (*this);
    }

    //******************************************************************************
    void RegEx::init()
    {
        m_regexCompStatus = regcomp(&m_preg, m_pattern.c_str(), m_flags);
        if (m_regexCompStatus != 0)
        {
            char errbuf[128];
            regerror(m_regexCompStatus, &m_preg, errbuf, 127);
            char buf[256];
            snprintf(buf, 255, "%s: %s", errbuf, m_pattern.c_str());
            TWK_EXC_THROW_WHAT(Exception, (const char*)buf);
        }
    }

    // *****************************************************************************
    bool RegEx::matches(const char* fullStr) const
    {
        return regexec(&m_preg, fullStr, 0, NULL, 0) == 0;
    }

    //******************************************************************************
    const RegEx& RegEx::nothing()
    {
        static RegEx ret(MATCH_NOTHING);
        return ret;
    }

    //******************************************************************************
    const RegEx& RegEx::anything()
    {
        static RegEx ret(MATCH_ANYTHING);
        return ret;
    }

    //******************************************************************************
    //******************************************************************************
    // GLOB FUNCTION
    //******************************************************************************
    //******************************************************************************
    string GlobEx::deglobSyntax(const char* pattern)
    {
        string gpat(pattern);

        int lastFound = 0;
        while (gpat.find(".", lastFound) != gpat.npos)
        {
            gpat.replace(gpat.find(".", lastFound), 1, "\\.");
            lastFound = gpat.find(".", lastFound) + 1;
        }
        while (gpat.find("?") != gpat.npos)
        {
            gpat.replace(gpat.find("?"), 1, ".");
        }

        lastFound = 0;
        while (gpat.find("*", lastFound) != gpat.npos)
        {
            gpat.replace(gpat.find("*", lastFound), 1, ".*");
            lastFound = gpat.find("*", lastFound) + 1;
        }

        gpat = "^" + gpat + "$";

        return gpat;
    }

    //******************************************************************************
    //******************************************************************************
    // MATCH CLASS
    //******************************************************************************
    //******************************************************************************

    //******************************************************************************
    // Default constructor
    Match::Match()
        : m_regex(&RegEx::nothing())
        , m_fullStr("")
        , m_foundMatch(false)
        , m_pmatch(NULL)
    {
        init();
    }

    //******************************************************************************
    // Regular constructors
    Match::Match(const RegEx& regex, const char* fullStr)
        : m_regex(&regex)
        , m_fullStr(fullStr)
        , m_foundMatch(false)
        , m_pmatch(NULL)
    {
        init();
    }

    //******************************************************************************
    Match::Match(const RegEx& regex, const std::string& fullStr)
        : m_regex(&regex)
        , m_fullStr(fullStr)
        , m_foundMatch(false)
        , m_pmatch(NULL)
    {
        init();
    }

    //******************************************************************************
    // Copy constructor
    Match::Match(const Match& copy)
        : m_regex(copy.m_regex)
        , m_fullStr(copy.m_fullStr)
        , m_foundMatch(false)
        , m_pmatch(NULL)
    {
        init();
    }

    //******************************************************************************
    Match::~Match() { delete[] m_pmatch; }

    //******************************************************************************
    // Assignment operator
    Match& Match::operator=(const Match& copy)
    {
        m_regex = copy.m_regex;
        m_fullStr = copy.m_fullStr;
        m_foundMatch = false;
        delete[] m_pmatch;
        m_pmatch = NULL;
        init();
        return (*this);
    }

    //******************************************************************************
    void Match::init()
    {
        m_foundMatch = false;

        if (m_regex->subCount() > 0)
        {
            m_pmatch = new regmatch_t[m_regex->subCount() + 1];
        }

        int result = regexec(m_regex->preg(), m_fullStr.c_str(),
                             m_regex->subCount(), m_pmatch, 0);

        if (result == 0)
        {
            m_foundMatch = true;
        }
    }

    // *****************************************************************************
    bool Match::hasSub(const int subNum) const
    {
        assert(subNum >= 0 && subNum < m_regex->subCount());

        assert(m_foundMatch);

        if (m_pmatch[(subNum + 1)].rm_so < 0)
        {
            return false;
        }

        return true;
    }

    // *****************************************************************************
    int Match::subStartPos(const int subNum) const
    {
        assert(subNum >= 0 && subNum < m_regex->subCount());

        assert(m_foundMatch);

        if (m_pmatch[(subNum + 1)].rm_so < 0)
        {
            return -1;
        }

        return m_pmatch[(subNum + 1)].rm_so;
    }

    // *****************************************************************************
    int Match::subEndPos(const int subNum) const
    {
        assert(subNum >= 0 && subNum < m_regex->subCount());

        assert(m_foundMatch);

        if (m_pmatch[(subNum + 1)].rm_so < 0)
        {
            return -1;
        }

        return m_pmatch[(subNum + 1)].rm_eo;
    }

    //******************************************************************************
    int Match::subLen(const int subNum) const
    {
        assert(subNum >= 0 && subNum < m_regex->subCount());

        assert(m_foundMatch);

        if (m_pmatch[(subNum + 1)].rm_so < 0)
        {
            return -1;
        }

        regoff_t rm_so = m_pmatch[(subNum + 1)].rm_so;
        regoff_t rm_eo = m_pmatch[(subNum + 1)].rm_eo;

        return rm_eo - rm_so;
    }

    //******************************************************************************
    string Match::subStr(const int subNum) const
    {
        assert(subNum >= 0 && subNum < m_regex->subCount());

        assert(m_foundMatch);

        if (m_pmatch[(subNum + 1)].rm_so < 0)
        {
            return "";
        }

        regoff_t rm_so = m_pmatch[(subNum + 1)].rm_so;
        regoff_t rm_eo = m_pmatch[(subNum + 1)].rm_eo;

        return m_fullStr.substr(rm_so, rm_eo - rm_so);
    }

} // namespace TwkUtil

#ifdef _MSC_VER
#undef snprintf
#endif
