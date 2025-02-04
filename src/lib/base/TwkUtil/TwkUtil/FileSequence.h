//******************************************************************************
// Copyright (c) 2001-2002 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef __TWKUTILFILESEQUENCE_H__
#define __TWKUTILFILESEQUENCE_H__

// ajg // #include <glob.h>
#include <string>
#include <map>
#include <TwkExc/TwkExcException.h>

/*

 This class handles a sequence of files of the format
 "<prefix>.<num>.<suffix>"  The filePattern passed into the constructor
 can be a

*/

namespace TwkUtil
{

    class FileSequence
    {
    public:
        TWK_EXC_DECLARE(Exception, TwkExc::Exception,
                        "FileSequence::Exception: ");

        FileSequence(std::string filePattern);

        ~FileSequence() {}

        void refresh();

        const int first() const { return m_first; }

        const int last() const { return m_last; }

        const int increment() const { return m_increment; }

        const int current() const { return m_current; }

        const int numfiles() const { return m_last - m_first + 1; }

        bool singleFile() const { return m_singleFile; }

        const std::string file(int filenum);
        const std::string nextFile();

        const std::string firstFile() { return m_matches[m_first]; }

        const std::string lastFile() { return m_matches[m_last]; }

        const std::string currentFile() { return m_matches[m_current]; }

    protected:
        int m_first;
        int m_last;
        int m_increment;
        int m_current;
        bool m_singleFile;
        std::string m_filePattern;
        std::string m_fileRange;
        std::string m_directory;
        std::string m_filePrefix;
        std::string m_fileSuffix;

        std::map<int, std::string> m_matches;
    };

} // End namespace TwkUtil

#endif // End #ifdef __TWKUTILFILESEQUENCE_H__
