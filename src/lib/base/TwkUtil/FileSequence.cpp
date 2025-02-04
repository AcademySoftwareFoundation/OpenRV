//******************************************************************************
// Copyright (c) 2001-2002 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <limits>
#include <stdlib.h>
#include <iostream>
#include <assert.h>
#include <string>
#include <vector>

#include <TwkUtil/FileSequence.h>
#include <TwkUtil/TwkRegEx.h>
#include <TwkUtil/RegexGlob.h>
#include <TwkExc/TwkExcException.h>

namespace TwkUtil
{
    using namespace std;

    FileSequence::FileSequence(string filePattern)
        : m_first(-1)
        , m_last(-1)
        , m_current(-1)
        , m_increment(1)
        , m_singleFile(false)
        , m_filePattern(filePattern)
    {
        refresh();
    }

    void FileSequence::refresh()
    {
        m_first = -1;
        m_last = -1;

        m_matches.clear();

        string filePattern = m_filePattern;

        //
        // Deal with single files differently:
        //
        if (filePattern.find('#') == filePattern.npos)
        {
            m_singleFile = true;
            m_first = 1;
            m_last = 1;
            m_current = 1;
            m_matches[m_current] = filePattern;
            return;
        }

        //
        // Split the directory (if there is one) from the file pattern
        //
        RegEx regEx1("(.+)/(.*)");
        Match dirMatch(regEx1, m_filePattern);
        if (!dirMatch)
        {
            m_directory = ".";
        }
        else
        {
            m_directory = dirMatch.subStr(0);
            filePattern = dirMatch.subStr(1);
        }

        //
        // Convert the "#" syntax into regex syntax
        //
        RegEx regEx2("(.+)\\.(.*)#\\.(.+)");
        Match fileMatch(regEx2, filePattern);

        if (!fileMatch)
        {
            TWK_THROW_EXC_STREAM("Couldn't parse '"
                                 << m_filePattern
                                 << "' into anything meaningful.\n");
        }
        m_filePrefix = fileMatch.subStr(0);
        m_fileRange = fileMatch.subStr(1);
        m_fileSuffix = fileMatch.subStr(2);

        string globPat = m_filePrefix + "\\.(-?[0-9]+)\\." + m_fileSuffix + "$";

        //
        // Find files matching our pattern
        //
        RegexGlob* rg = new RegexGlob(globPat, m_directory);
        if (rg->matchCount() <= 0)
        {
            TWK_THROW_EXC_STREAM("No files matched '" << m_filePattern << "'");
        }

        //
        // Add the found files to our filename map
        //
        int minFound = numeric_limits<int>::max();
        int maxFound = numeric_limits<int>::min();
        for (int i = 0; i < rg->matchCount(); ++i)
        {
            int framenum = rg->fileSubInt(i, 0);
            m_matches[framenum] = m_directory + "/" + rg->fileName(i);

            minFound = framenum < minFound ? framenum : minFound;
            maxFound = framenum > maxFound ? framenum : maxFound;
        }

        //
        // Determine the frame range
        //
        m_first = minFound;
        m_last = maxFound;

        if (!m_fileRange.empty())
        {
            RegEx regEx3("(-?[0-9]+)?-(-?[0-9]+)?x?(-?[0-9]+)?");
            Match range(regEx3, m_fileRange);
            if (!range)
            {
                TWK_THROW_EXC_STREAM("Couldn't parse '"
                                     << m_fileRange
                                     << "' into a meaningful frame range");
            }
            if (!range.subStr(0).empty())
            {
                if ((range.subInt(0) >= m_first) && (range.subInt(0) <= m_last))
                {
                    m_first = range.subInt(0);
                }
                else
                {
                    cerr << "FileSequence::Warning: Specified min '"
                         << range.subInt(0) << "' is outside "
                         << "range of matching files.  Using exising min '"
                         << m_first << "'." << endl;
                }
            }
            if (!range.subStr(1).empty())
            {
                if ((range.subInt(1) >= m_first) && (range.subInt(1) <= m_last))
                {
                    m_last = range.subInt(1);
                }
                else
                {
                    cerr << "FileSequence::Warning: Specified max '"
                         << range.subInt(1) << "' is outside "
                         << "range of matching files.  Using exising max '"
                         << m_last << "'." << endl;
                }
            }
            if (!range.subStr(2).empty())
            {
                if ((range.subInt(2) >= 1))
                {
                    m_increment = range.subInt(2);
                }
                else
                {
                    cerr << "FileSequence::Warning: Specified frame increment '"
                         << range.subInt(2) << "' is <= 0"
                         << ".  Using exising increment of 1" << endl;
                }
            }
        }

        delete rg;
    }

    const string FileSequence::file(const int filenum)
    {
        assert(filenum >= m_first && filenum <= m_last);
        return m_matches[filenum];
    }

    const string FileSequence::nextFile()
    {
        if (m_singleFile)
        {
            return m_matches[m_current];
        }
        m_current += m_increment;
        if (m_current > m_last)
        {
            m_current = m_first;
            return string();
        }

        return m_matches[m_current];
    }

} // End namespace TwkUtil
