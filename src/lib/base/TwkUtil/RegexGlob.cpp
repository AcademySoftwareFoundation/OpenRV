//******************************************************************************
// Copyright (c) 2001-2002 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <assert.h>
#include <stdlib.h>
#include <sys/types.h>
#include <iostream>
#include <string>
#include <algorithm>
#include <map>
#include <TwkUtil/TwkRegEx.h>
#include <TwkUtil/RegexGlob.h>
#include <TwkExc/TwkExcException.h>
#include <dirent.h>

namespace TwkUtil
{

    using namespace std;

    RegexGlob::RegexGlob(const RegEx& regex, const string directory)
        : m_regex(regex)
        , m_directory(directory)
    {
        refresh();
    }

    RegexGlob::~RegexGlob() { m_fileList.clear(); }

    void RegexGlob::refresh()
    {
        m_fileList.clear();

        // Open the directory
        DIR* dir = opendir(m_directory.c_str());
        struct dirent* dp;

        if (dir == NULL)
        {
            string str("Unable to open directory with opendir(\"" + m_directory
                       + "\")");
            TWK_EXC_THROW_WHAT(Exception, str);
        }

        // Read through all the files in the given directory,
        // adding them if they match our regex
        while ((dp = readdir(dir)) != NULL)
        {
            if (Match(m_regex, dp->d_name))
            {
                // File matched, so store it's info
                m_fileList.push_back(dp->d_name);
            }
        }

        if (closedir(dir))
        {
            TWK_EXC_THROW_WHAT(Exception, "closedir() failed");
        }

        sort(m_fileList.begin(), m_fileList.end()); // Sort the files
    }

    string RegexGlob::fileSubStr(const int filenum, const int subnum) const
    {
        assert(filenum >= 0 && filenum <= matchCount());

        return Match(m_regex, m_fileList[filenum]).subStr(subnum);
    }

    int RegexGlob::fileSubInt(const int filenum, const int subnum) const
    {
        return atoi(fileSubStr(filenum, subnum).c_str());
    }

    int RegexGlob::matchCount() const
    {
        if (m_fileList.empty())
        {
            return 0;
        }

        return m_fileList.size();
    }

    std::string RegexGlob::fileName(const int filenum) const
    {
        assert(filenum >= 0 && filenum <= matchCount());
        return m_fileList[filenum];
    }

} // End namespace TwkUtil
