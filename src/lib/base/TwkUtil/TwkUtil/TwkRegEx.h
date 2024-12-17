//******************************************************************************
// Copyright (c) 2001-2002 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
//
// This class is a wrapper around the POSIX regex functions.  It will work
// with strings or char *s, and can return subexpressions as strings, ints,
// or floats.  Examples:
//
//     RegEx re( "(.+)\\.([0-9]+)\\.(.+)" );
//
//     Match m1( re, "image.0123.jpg" );
//     Match m2( re, "nonimage.txt" );
//
//     if( m1 ) .....       // evaluates to true (match found)
//
//     if( m2 ) .....       // evaluates to false (no match)
//
// You can also use it like this:
//
//     if( Match( RegEx( "\\.cpp" ), "myfile_a.cpp" ) ) ...
//
// which evaluates to true, or this:
//
//     cout << Match( RegEx( "(.+)\\." ), "myfile_a.cpp" ).subStr( 0 );
//
// which will output "myfile_a"
//
// ALSO:
//
// The GlobEx is a thin wrapper class around RegEx.
//
// It will translate the given pattern from shell "glob" syntax
// into posix regular expression syntax.  Use like this:
//
//    if( Match( GlobEx( "*.txt" ), filename ) ) ...
//
// Note that you can still use parens for sub-expression extraction if you want,
// thus:
//
//  cout << Match( GlobEx( "*.(*).jpg" ), "img.0123.jpg" ).subInt( 0 );
//
// Will output 123.
//

#ifndef _TwkUtilRegEx_h_
#define _TwkUtilRegEx_h_

#include <TwkExc/TwkExcException.h>
#include <TwkUtil/dll_defs.h>
#include <math.h>
#include <stdlib.h>

#include <sys/types.h>
#ifdef _MSC_VER
#include <pcre2.h>
#include <pcre2posix.h>
#else
#include <regex.h>
#endif
#include <string>

namespace TwkUtil
{

//
// MATCH_NOTHING will always fail because it tries to match any character
// at the end of the string _followed by_ any character at the beginning
// of the string.
//
#define MATCH_NOTHING "$.^."
#define MATCH_ANYTHING ".*"

    //******************************************************************************
    // POSIX Regular expression class
    class TWKUTIL_EXPORT RegEx
    {
    public:
        // Exception class
        TWK_EXC_DECLARE(Exception, TwkExc::Exception, "RegEx::Exception: ");

        // Default constructor
        RegEx();

        // Normal constructors
        RegEx(const char* pattern, int flags = REG_EXTENDED);
        RegEx(const std::string& pattern, int flags = REG_EXTENDED);

        // Copy constructor
        RegEx(const RegEx& copy);

        // Destructor
        ~RegEx();

        // Assignment operator
        RegEx& operator=(const RegEx& copy);

        // Handy functions for initializing default patterns
        static const RegEx& nothing();
        static const RegEx& anything();

    protected:
        // Constructor helper
        void init();

    public:
        // Public functions
        const std::string& pattern() const { return m_pattern; }

        // Useful for testing a pattern without the overhead of creating a full
        // Match object
        bool matches(const char* fullStr) const;

        bool matches(const std::string& fullStr) const
        {
            return matches(fullStr.c_str());
        }

    protected:
        // Match functions
        friend class Match;

        const int subCount() const { return m_preg.re_nsub + 1; }

        const regex_t* preg() const { return &m_preg; }

    protected:
        std::string m_pattern;
        int m_flags;
        regex_t m_preg;
        int m_regexCompStatus;
    };

    //******************************************************************************
    // GlobEx class. Transparent wrapper around RegEx that uses shell glob
    // syntax.
    class TWKUTIL_EXPORT GlobEx : public RegEx
    {
    public:
        // Default constructor
        GlobEx()
            : RegEx()
        {
        }

        // Regular constructors
        GlobEx(const char* pattern, int flags = REG_EXTENDED)
            : RegEx(deglobSyntax(pattern), flags)
        {
        }

        GlobEx(const std::string& pattern, int flags = REG_EXTENDED)
            : RegEx(deglobSyntax(pattern.c_str()), flags)
        {
        }

        // Copy constructor
        GlobEx(const GlobEx& copy)
            : RegEx(copy)
        {
        }

        // Assignment operator
        GlobEx& operator=(const GlobEx& copy)
        {
            return (GlobEx&)(RegEx::operator=(copy));
        }

        // Deglob syntax thingy.
        static std::string deglobSyntax(const char* globSyntaxPattern);
    };

    //******************************************************************************
    // Match class. Notice that it us automatically cast to a bool.
    class TWKUTIL_EXPORT Match
    {
    public:
        // Exception class
        TWK_EXC_DECLARE(Exception, TwkExc::Exception, "Match::Exception: ");

        // Default constructor
        Match();

        // Regular constructors
        Match(const RegEx& regex, const char* fullStr);
        Match(const RegEx& regex, const std::string& fullStr);

        // Copy constructor
        Match(const Match& copy);

        // Destructor
        ~Match();

        // Assignment operator
        Match& operator=(const Match& copy);

    protected:
        // Constructor helper
        void init();

    public:
        // Bool casting operator
        bool foundMatch() const { return m_foundMatch; }

        operator bool() const { return m_foundMatch; }

        // Sub info functions
        int subCount() const { return m_regex->subCount(); }

        bool hasSub(const int subNum) const;
        int subStartPos(const int subNum) const;
        int subEndPos(const int subNum) const;
        int subLen(const int subNum) const;
        std::string subStr(const int subNum) const;

        int subInt(const int subNum) const
        {
            return atoi(subStr(subNum).c_str());
        }

        float subFloat(const int subNum) const
        {
            return atof(subStr(subNum).c_str());
        }

    protected:
        const RegEx* m_regex;
        std::string m_fullStr;
        bool m_foundMatch;
        regmatch_t* m_pmatch;
    };

} // End namespace TwkUtil

#endif
