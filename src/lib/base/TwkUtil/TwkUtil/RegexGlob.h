//******************************************************************************
// Copyright (c) 2001-2002 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef __TWKUTILREGEXGLOB_H__
#define __TWKUTILREGEXGLOB_H__

#include <sys/types.h>
#ifdef _MSC_VER
#include <pcre2.h>
#include <pcre2posix.h>
#else
#include <regex.h>
#endif
#include <string>
#include <vector>
#include <map>
#include <TwkUtil/TwkRegEx.h>
#include <TwkExc/TwkExcException.h>

//
// This class basically works like glob(), except it uses the TwkUtilRegEx
// class for matching instead of shell wildcards.  In addition to the better
// pattern matching that regular expressions give, it can also return any
// subexpressions in the given regular expression (see 'man 7 regex' on
// Linux for for more info), and can return them as strings or ints.
//
//
// A great use of this class is to use the pattern GlobEx( "(*).(*).(*)" )
// to match image sequences.  This way, .fileSubStr( i, 0 ) will be the
// file prefix, .fileSubInt( i, 1 ) will be the frame number as an integer,
// and .fileSubStr( i, 2 ) will be the extension.  Pretty keen huh?
//

namespace TwkUtil
{

    class TWKUTIL_EXPORT RegexGlob
    {
    public:
        TWK_EXC_DECLARE(Exception, TwkExc::Exception, "RegexGlob::Exception: ");

        RegexGlob(const RegEx& regex, const std::string directory = ".");
        ~RegexGlob();

        int matchCount() const;
        std::string fileName(const int filenum) const;
        std::string fileSubStr(const int filenum, const int subnum = 0) const;
        int fileSubInt(const int filenum, const int subnum = 0) const;

        const std::vector<std::string> fileList() const { return m_fileList; }

        void refresh();

    protected:
        const RegEx m_regex;
        const std::string m_directory;

        // list of files matched
        std::vector<std::string> m_fileList;
    };

} // End namespace TwkUtil

#endif // End #ifdef __TWKUTILREGEXGLOB_H__
