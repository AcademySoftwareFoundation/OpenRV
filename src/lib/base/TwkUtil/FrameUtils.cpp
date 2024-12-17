//******************************************************************************
// Copyright (c) 2001-2004 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkUtil/FrameUtils.h>
#include <TwkUtil/TwkRegEx.h>
#include <TwkUtil/File.h>
#include <TwkUtil/PathConform.h>
#include <stl_ext/string_algo.h>
#include <limits.h>
#include <stdio.h>
#include <list>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <iterator>
#include <assert.h>
#include <string.h>

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#define MAX_SEQUENCE_LEN 9
#define DIGIT_REGEX "(-?[0-9]+)"
#define DOT_DIGIT_REGEX "\\.(-?[0-9]+)\\."

#if 0
#define DB_DOIT true
#define DB_GENERAL 0x01
#define DB_ALL 0xff

#define DB_LEVEL DB_ALL

#define DB(x)                  \
    if (DB_GENERAL & DB_LEVEL) \
    cerr << "FrameUtils: " << x << endl
#define DBL(level, x)     \
    if (level & DB_LEVEL) \
    cerr << "FrameUtils: " << x << endl
#else
#define DB_DOIT false
#define DB(x)
#define DBL(level, x)
#endif

namespace TwkUtil
{
    using namespace std;

    // mR - double escape backslashes
#ifdef WIN32
    static const string notEscaped = "[^\\\\]";
#else
    static const string notEscaped = "[^\\]";
#endif
    static const string optionalMult = "(x([0-9]+))?";
    static const string optionalModulo = "(/([0-9]+))?";
    static const string optionalOffset1 = "([+-]?[0-9]+)?";
    static const string optionalOffset2 = "([+-]?[0-9]+)?";
    static const string optionalFPS = "([0-9]+)?";

    static const string preCookie = notEscaped + "(";
    static const string postCookie =
        optionalOffset1 + optionalMult + optionalModulo + optionalOffset2 + ")";

    static const string timeRange =
        "((([^0-9]-)?([0-9]+)?-?-?[0-9]+(x-?[0-9]+)?)(,(-?([0-9]+)?-?-?[0-9]+("
        "x-?[0-9]+)?))*)?"
        "([@#]+|%[-+ 0-9]*[di])";

    static const string timeRange2 = "(-?[0-9]+)-?(-*[0-9]+)?x?([0-9]+)?";

    static const string matchFront = "^";
    static const string matchBack = "$";

    static const RegEx timeRE(timeRange);
    static const RegEx prefixTimeRE("^([^0-9]-)");

    static const RegEx onlyTimeRE(matchFront + timeRange2 + matchBack);

    static const RegEx timeRE2(timeRange2);

    static const RegEx unPaddedFrameSymRE(preCookie + "(@+)" + postCookie);
    static const RegEx paddedFrameSymRE(preCookie + "(#+)" + postCookie);
    static const RegEx unPaddedTickSymRE(preCookie + "`" + optionalFPS
                                         + postCookie);
    static const RegEx pctSyntaxRE(preCookie + "(%[0-9+]*[di])" + postCookie);

    static const RegEx unPaddedFrameSymRENoEsc("^((@+)" + postCookie);
    static const RegEx paddedFrameSymRENoEsc("^((#+)" + postCookie);
    static const RegEx unPaddedTickSymRENoEsc("^(`" + optionalFPS + postCookie);
    static const RegEx pctSyntaxRENoEsc("^((%[0-9+]*[di])" + postCookie);

    static const RegEx stereoSeqRE("(.*)%[vV](.*)");

    static SequencePredicateExtensionSet sequenceExtenions;

    SequencePredicateExtensionSet& predicateFileExtensions()
    {
        return sequenceExtenions;
    }

    static bool isNumeric(string& s)
    {
        for (int i = 0; i < s.size(); ++i)
            if (!isdigit(s[i]))
                return false;
        return true;
    }

    bool GlobalExtensionPredicate(const string& pattern)
    {
        static const RegEx re(DIGIT_REGEX);
        string ext = extension(pattern);
        if (pattern[pattern.size() - 1] == '/')
            return false; // skip dirs
        //
        //  Allow files without extension, and with extensions that are numbers.
        //
        if (ext == "" || isNumeric(ext))
            return true;

        return sequenceExtenions.count(ext) != 0;
    }

    bool AnySequencePredicate(const string& pattern) { return true; }

    bool FalseSequencePredicate(const string& pattern) { return false; }

    //
    //  Compare two files lexically AND numerically
    //

#if 0
struct LexinumericCompare
{
    bool operator < (const string& a, const string& b) 
    {
    }
};
#endif

    static string replaceAll(string retStr, const RegEx& regex,
                             const string& replacement, int maxLen, int& count)
    {
        std::string result;
        bool endsWithReplacement = false;

        while (!retStr.empty())
        {

            Match m = Match(regex, retStr);
            if (m)
            {
                if (m.subLen(0) > maxLen)
                {
                    result += retStr.substr(0, m.subStartPos(0) + m.subLen(0));
                    endsWithReplacement = false;
                }
                else
                {
                    // Do not consider the dash character '-' as a minus sign if
                    // it is preceded by a place holder. Example:
                    //   Considering the file bar.10-001.foo
                    //   The resulting pattern must be bar.DIGIT-DIGIT.foo and
                    //   not bar.DIGITDIGIT.foo
                    int startPos = m.subStartPos(0);
                    if (startPos == 0 && endsWithReplacement)
                    {
                        ++startPos;
                    }

                    result += retStr.substr(0, startPos);
                    result += replacement;
                    endsWithReplacement = true;

                    ++count;
                    if (count > 100)
                        break; // Prevent infinite loops
                }
                retStr = retStr.substr(m.subStartPos(0) + m.subLen(0));
            }
            else
            {
                result += retStr;
                retStr.clear();
            }
        }

        return result;
    }

    //
    // Create a regular expression suitable for searching for parts of a file
    // sequence.
    //

    static string patternize(string retStr, int& count)
    {

        //
        //  Prefer .DIGIT. before plain digits
        //
        //  Actually don't prefer because upstream code needs to check how many
        //  files in the sequence match this pattern.  EG in the seq:
        //
        //  0001_blah.001.jpg
        //  0002_blah.001.jpg
        //
        //  the first pattern is the frame number, even though it's further from
        //  the extension _and_ does not have dots around it.  Upstream code can
        //  detect this, and also can prefer the patter with dots.

        static const RegEx re(DIGIT_REGEX);
        static const RegEx dotre(DOT_DIGIT_REGEX);
        static const RegEx phre("(PLACE\\|HOLDER)");
        count = 0;

        retStr = replaceAll(retStr, dotre, "PLACE|HOLDER", MAX_SEQUENCE_LEN + 2,
                            count);
        retStr =
            replaceAll(retStr, re, "PLACE|HOLDER", MAX_SEQUENCE_LEN, count);
        return replaceAll(retStr, phre, DIGIT_REGEX, INT_MAX, count);
    }

    //
    //  Returns all the SequenceNames in a directory.
    //
    SequenceNameList sequencesInDirectory(const string& path,
                                          SequencePredicate P,
                                          bool includeNonMatching,
                                          bool frameRanges, bool showDirs)
    {
        FileNameList allFiles;

        if (filesInDirectory(path.c_str(), allFiles, showDirs))
        {
            return sequencesInFileList(allFiles, P, includeNonMatching,
                                       frameRanges);
        }

        return SequenceNameList();
    }

    static string escapeString(string prose, string escapeString)
    {
        size_t loc = prose.find(escapeString);
        while (loc != string::npos)
        {
            prose.insert(loc, "\\");
            loc = prose.find(escapeString, loc + 2);
        }
        return prose;
    }

    static string escapeRegexConflicts(string prose)
    {
        prose = escapeString(prose, "(");
        prose = escapeString(prose, ")");
        prose = escapeString(prose, "[");
        prose = escapeString(prose, "]");
        prose = escapeString(prose, "?");
        prose = escapeString(prose, "*");
        prose = escapeString(prose, "+");
        prose = escapeString(prose, "^");
        prose = escapeString(prose, "|");
        prose = escapeString(prose, "$");

        return prose;
    }

    static int defaultMinSequenceSize = -2;

    SequenceNameList sequencesInFileList(const FileNameList& infiles,
                                         SequencePredicate P,
                                         bool includeNonMatching,
                                         bool frameRanges, int minSequenceSize)
    {
        if (defaultMinSequenceSize == -2)
        {
            const char* minSequenceSizeVar = getenv("TWK_MIN_SEQUENCE_SIZE");
            if (minSequenceSizeVar)
            {
                defaultMinSequenceSize = atoi(minSequenceSizeVar);
                if (defaultMinSequenceSize <= 1)
                //
                //  Never make sequences.
                //
                {
                    defaultMinSequenceSize = numeric_limits<int>::max();
                }
            }
            else
                defaultMinSequenceSize = 5;
        }
        if (minSequenceSize == -1)
            minSequenceSize = defaultMinSequenceSize;

        //
        //  NOTE: this function needs to maintain as much order as
        //  possible in the in input file list.
        //

        // Make a copy of the file list so this function will be
        // non-destructive. Using a list rather than a vector will make item
        // removal faster
        typedef list<string> TempFileList;
        TempFileList allfiles;
        copy(infiles.begin(), infiles.end(), back_inserter(allfiles));
        transform(allfiles.begin(), allfiles.end(), allfiles.begin(),
                  pathConform);

        SequenceNameList frameSequences;

        DB("sequencesInFileList " << infiles.size() << " files, min size "
                                  << minSequenceSize);

        while (allfiles.size() > 0)
        {
            const string& f = allfiles.front();

            if (P && !P(f))
            {
                frameSequences.push_back(f);
                allfiles.pop_front();
                continue;
            }

            //
            //  only patternize last element of path (don't look for frame
            //  numbers in directory names)
            //

            string path = "";
            string file = f;
            size_t lastSlash = f.rfind("/");
            if (lastSlash != string::npos)
            {
                file = f.substr(lastSlash + 1);
                path = f.substr(0, f.size() - file.size());
                path = escapeRegexConflicts(path);
            }
            DB("    path '" << path << "' file '" << file << "'");

            int nsubs = 0;
            string escapedFile = escapeRegexConflicts(file);
            string pattern = path + patternize(escapedFile, nsubs);
            DB("    pattern '" << pattern << "' file '" << escapedFile << "'");

            if (nsubs == 0)
            {
                // No frame numbers at all in this filename
                if (includeNonMatching)
                    frameSequences.push_back(f);
                allfiles.pop_front();
                continue;
            }

            try
            {
                //
                //  Because there can be multiple potential frame numbers,
                //  pick the one that matches the most number of frames
                //  (But do it from the end to the beg)
                //

                string bestPattern = "";
                size_t bestPatternCount = 0;
                bool bestPatternHasDot = false;
                int bestPatternLongestRun = 0;
                list<int> matchFrames;

                RegEx regEx("^" + pattern + "$");
                Match m(regEx, f);
                if (!m.foundMatch())
                {
                    DB("        pattern '" << pattern << "' not found in file '"
                                           << f << "'");
                    // Name is probably garbled.  Give up trying to find the
                    // sequence.
                    if (includeNonMatching)
                        frameSequences.push_back(f);
                    allfiles.pop_front();
                    continue;
                }

                for (int i = m.subCount() - 2; i >= 0; i--)
                {
                    int subStartPos = m.subStartPos(i);
                    bool hasDot =
                        (subStartPos > 0 && f[subStartPos - 1] == '.');
                    string subprefix = f.substr(0, subStartPos);
                    subprefix = escapeRegexConflicts(subprefix);
                    string subsuffix =
                        f.substr(m.subEndPos(i), f.size() - m.subEndPos(i));
                    subsuffix = escapeRegexConflicts(subsuffix);
                    string subpattern =
                        subprefix + string(DIGIT_REGEX) + subsuffix;
                    RegEx subpatternRe("^" + subpattern + "$");
                    size_t matchCount = 0;

                    matchFrames.clear();
                    for (TempFileList::iterator iter = allfiles.begin();
                         iter != allfiles.end(); ++iter)
                    {
                        if (subpatternRe.matches(iter->c_str()))
                        {
                            Match frameMatch(subpatternRe, iter->c_str());
                            matchFrames.push_back(frameMatch.subInt(0));
                            ++matchCount;
                        }
                    }
                    matchFrames.sort();
                    int contiguous = 0;
                    int longestRun = 0;
                    int lastFrame = matchFrames.front();
                    matchFrames.pop_front();
                    for (list<int>::iterator iter = matchFrames.begin();
                         iter != matchFrames.end(); ++iter)
                    {
                        if (*iter == (lastFrame + 1))
                            contiguous++;
                        else
                            contiguous = 0;

                        if (contiguous > longestRun)
                            longestRun = contiguous;
                        lastFrame = *iter;
                    }
                    DB("        subpattern '" << subpattern << "' hasDot "
                                              << hasDot << ", matches "
                                              << matchCount << " files");
                    //
                    //  Prefer pattern with the most contiguous frames or
                    //  preceded by dot.
                    //
                    if ((matchCount > bestPatternCount
                         && longestRun >= bestPatternLongestRun)
                        || (bestPatternLongestRun == 0
                            && longestRun >= minSequenceSize)
                        || (matchCount == bestPatternCount && hasDot
                            && !bestPatternHasDot))
                    {
                        bestPatternHasDot = hasDot;
                        bestPatternCount = matchCount;
                        bestPattern = subpattern;
                        bestPatternLongestRun = longestRun;
                        DB("        setting best pattern to " << bestPattern);
                    }
                }

                // If no pattern matched more than minSequenceSize, don't make
                // a sequence.  Just return the individual files as-is (assuming
                // includeNonMatching is true).
                if (bestPatternCount < minSequenceSize)
                {
                    RegEx bestPatternRe("^" + bestPattern + "$");

                    // mR - changed so it wouldn't crash in Windows
                    for (TempFileList::iterator iter = allfiles.begin();
                         iter != allfiles.end();
                         /* ++iter */)
                    {
                        const string& s = *iter;

                        if (m = Match(bestPatternRe, s))
                        {
                            if (includeNonMatching)
                                frameSequences.push_back(s);
                            allfiles.erase(iter++);
                        }
                        else
                        {
                            ++iter;
                        }
                    }
                    continue;
                }

                // Get a list of matching files and frame numbers, and determine
                // the padding
                RegEx bestPatternRe("^" + bestPattern + "$");
                FrameList frames;
                frames.clear();
                int pad = 1000;

                // mR - changed so it wouldn't crash in Windows
                for (TempFileList::iterator iter = allfiles.begin();
                     iter != allfiles.end();
                     /* ++iter */)
                {
                    const string& s = *iter;

                    if (m = Match(bestPatternRe, s))
                    {
                        frames.push_back(m.subInt(0));
                        pad = std::min(pad, m.subLen(0));
                        allfiles.erase(iter++);
                    }
                    else
                    {
                        ++iter;
                    }
                }

                // Determine the shake-like time range & padding strings
                string timeStr = frameRanges ? frameStr(frames) : "";
                //
                //  Giant strings of range,range,range, etc are not useful, and
                //  on windows they crash the qt file browser code because the
                //  columns need to be so wide, and then reconsitiuting the
                //  frame list with pcre/RegEx causes a crash there, so fall
                //  back to a simple range here.
                //
                if (timeStr.size() > 40)
                {
                    ostringstream rangeStr;
                    int smallest = numeric_limits<int>::max();
                    int largest = -numeric_limits<int>::max();
                    for (int i = 0; i < frames.size(); ++i)
                    {
                        if (frames[i] < smallest)
                            smallest = frames[i];
                        if (frames[i] > largest)
                            largest = frames[i];
                    }
                    rangeStr << smallest << "-" << largest;
                    timeStr = rangeStr.str();
                }

                if (pad == 4)
                {
                    timeStr += "#";
                }
                else
                {
                    for (int i = 0; i < pad; ++i)
                        timeStr += "@";
                }

                string frameSeq =
                    bestPattern.replace(bestPattern.find(DIGIT_REGEX),
                                        strlen(DIGIT_REGEX), timeStr);
                frameSequences.push_back(frameSeq);
            }
            catch (...)
            {
                //
                //  Windows regex code can throw on strange patterns, so
                //  if we end up here, just assume it's not a sequence
                //  and move on.
                if (includeNonMatching)
                    frameSequences.push_back(f);
                allfiles.pop_front();
            }
        }

        // remove any remaining escapes
        for (int i = 0; i < frameSequences.size(); ++i)
        {
            string checkStr = frameSequences[i];
            while (checkStr.find("\\") != checkStr.npos)
            {
                frameSequences[i] = checkStr.erase(checkStr.find("\\"), 1);
            }
        }

        if (DB_DOIT)
            for (int i = 0; i < frameSequences.size(); ++i)
                DB("    frameseq #" << i << ": " << frameSequences[i]);
        return frameSequences;
    }

    bool isStereoSequence(SequenceName seqName)
    {
        return Match(stereoSeqRE, seqName);
    }

    //
    //  Returns an ExistingFile for each frame in a SequenceName in order.  Note
    //  that an ExistingFile may or may not exist ;-) The "exists" data member
    //  of ExistingFile will be true or false as appropriate.
    //
    //  If this is really a sequence (an image file per frame), the "frame" data
    //  memeber of every existing file will hold the corresponding frame.
    //

    ExistingFileList existingFilesInSequence(const SequenceName& inSeqName,
                                             bool justOne)
    {
        std::string seqName = pathConform(inSeqName);

        DB("existingFilesInSequence seqName '" << seqName << "'");
        ExistingFileList existingFiles;

        string timeStr = "";
        string pattern = "";

        if (splitSequenceName(seqName, timeStr, pattern))
        {
            if (timeStr != "")
            {
                DB("    split timeStr '" << timeStr << "' pattern '" << pattern
                                         << "'");
                FrameList frms = frameRange(timeStr.c_str());

                string seqDir = dirname(seqName.c_str());
                FileNameList dirFiles;
                filesInDirectory(seqDir.c_str(), dirFiles, false);

                if (seqDir == "." && seqName[0] != '.')
                    seqDir = "";
                else
                    seqDir = seqDir + "/";

                std::set<string> dirFilesSet;
                for (int i = 0; i < dirFiles.size(); ++i)
                    dirFilesSet.insert(seqDir + dirFiles[i]);

                for (unsigned int i = 0; i < frms.size(); ++i)
                {
                    string filename =
                        replaceFrameSymbols(pattern.c_str(), frms[i]);
                    ExistingFile ef;
                    ef.name = filename;
                    //
                    //  fileExists calls stat, which is very slow on windows.
                    //  Since we know all these files are in the same dir, use
                    //  the dir table entries to see if they exist.
                    //
                    // ef.exists = fileExists( filename.c_str() );
                    //
                    ef.exists = (dirFilesSet.count(filename) != 0);
                    ef.frame = frms[i];

                    if (justOne)
                    {
                        if (ef.exists)
                        {
                            existingFiles.push_back(ef);
                            break;
                        }
                    }
                    else
                    {
                        existingFiles.push_back(ef);
                    }
                }
            }
            else
            {
                string dir = dirname(seqName);
                string seq = escapeRegexConflicts(basename(seqName));
                string fmt = "";
                string frameMatcher = ".*";

                DB("    split failed, trying direct match on seq '" << seq
                                                                    << "'");

                //
                //  Use NoEsc versions of regex first, since at this point there
                //  is definitely no directory, and the frame number may be at
                //  the beginning of the string.
                //

                Match m;
                bool doReplace = false;
                if ((m = Match(paddedFrameSymRENoEsc, seq))
                    || (m = Match(paddedFrameSymRE, seq)))
                {
                    DB("    padded match");
                    fmt = m.subStr(1);
                    doReplace = true;
                }
                else if ((m = Match(unPaddedFrameSymRENoEsc, seq))
                         || (m = Match(unPaddedFrameSymRE, seq)))
                {
                    DB("    unpadded match");
                    fmt = m.subStr(1);
                    doReplace = true;
                }
                else if ((m = Match(pctSyntaxRENoEsc, seq))
                         || (m = Match(pctSyntaxRE, seq)))
                {
                    DB("    pct match");
                    fmt = m.subStr(1);
                    doReplace = true;
                }
                if (doReplace)
                {
                    frameMatcher = "^" + seq + "$";
                    frameMatcher = frameMatcher.replace(
                        frameMatcher.find(fmt), fmt.size(), "([-0-9]+)");
                    seq = seq.replace(seq.find(fmt), fmt.size(), "*");
                }
                DB("    after matching seq '" << seq << "' fmt '" << fmt
                                              << "' frameMatcher '"
                                              << frameMatcher << "'");

                // Get all the files matching the sequenceName

                FileNameList allFiles;

                if (!filesInDirectory(dir.c_str(), seq.c_str(), allFiles))
                {
                    return existingFiles; // (still empty at this point)
                }

                if (doReplace)
                //
                //  We are replacing a format string, and there are some
                //  matching files in the directory, so find the min and max
                //  matching frames, assemble a timesStr, add to fmt and
                //  recurse. That way we we get a complete map of the frame
                //  range, not just the files that happen to currently exist.
                //
                {
                    int minFrame = numeric_limits<int>::max();
                    int maxFrame = numeric_limits<int>::min();

                    for (int i = 0; i < allFiles.size(); ++i)
                    {
                        if (m = Match(frameMatcher, allFiles[i]))
                        {
                            int f = atoi(m.subStr(0).c_str());

                            if (f < minFrame)
                                minFrame = f;
                            if (f > maxFrame)
                                maxFrame = f;
                        }
                    }
                    if (maxFrame != numeric_limits<int>::min())
                    {
                        string newSeqName = seqName;
                        ostringstream timeStrStr;
                        timeStrStr << minFrame << "-" << maxFrame << fmt;

                        newSeqName = newSeqName.replace(
                            newSeqName.find(fmt), fmt.size(), timeStrStr.str());

                        return existingFilesInSequence(newSeqName, justOne);
                    }
                }

                lexiNumericFileSort(allFiles);

                for (int i = 0; i < allFiles.size(); ++i)
                {
                    if (m = Match(frameMatcher, allFiles[i]))
                    {
                        DB("        file matches: " << allFiles[i] << " frame "
                                                    << m.subStr(0));
                        ExistingFile ef;
                        ef.name = dir + "/" + allFiles[i];
                        ef.exists = true;
                        ef.frame = atoi(m.subStr(0).c_str());
                        existingFiles.push_back(std::move(ef));
                        if (justOne)
                            break;
                    }
                }
            }

            if (existingFiles.empty()
                && (seqName.find_first_of("#@") != std::string::npos)
                && fileExists(seqName.c_str()))
            {
                // In this case the file path contains really # and @ characters
                ExistingFile ef;
                ef.name = seqName;
                ef.exists = true;
                ef.frame = -1;
                existingFiles.push_back(std::move(ef));
            }
        }
        else
        {
            DB("    splitSequenceName returned false");
            ExistingFile ef;
            ef.name = seqName;
            ef.exists = fileExists(seqName.c_str());
            ef.frame = -1;
            existingFiles.push_back(ef);
        }

        return existingFiles;
    }

    bool splitSequenceName(const SequenceName& sequenceName, string& timeStr,
                           string& sequencePattern)
    {
        if (Match m = Match(timeRE, sequenceName))
        {
            timeStr = m.subStr(0);

            // We need to keep (if any) what is prefixing the first minus symbol
            if (Match subMatch = Match(prefixTimeRE, timeStr))
            {
                timeStr.erase(0, 1);
            }

            if (atoi(timeStr.c_str()) == numeric_limits<int>::max())
            {
                return false;
            }

            sequencePattern = sequenceName;

            if (timeStr != "")
            {
                sequencePattern = sequencePattern.replace(
                    sequencePattern.find(timeStr), timeStr.size(), "");
            }

            return true;
        }
        else
        {
            return false;
        }
    }

    SequencePattern sequencePattern(const ExistingFileList& efl,
                                    bool onlyExisting, int minSequenceSize)
    {
        FileNameList files;

        for (unsigned int i = 0; i < efl.size(); ++i)
        {
            if (onlyExisting && !efl[i].exists)
                continue;
            files.push_back(efl[i].name);
        }

        SequenceNameList patterns =
            sequencesInFileList(files, NULL, true, true, minSequenceSize);
        return patterns.front();
    }

    //
    // See FrameUtils.h for more info
    //
    string replaceFrameSymbols(const string& cs, int frame, int fps)
    {
        Match m;
        string s = cs;

        DB("replaceFrameSymbols cs " << cs);

        //
        //  Always try to match frame numbers at beginning of string first.
        //

        m = Match(unPaddedFrameSymRENoEsc, s);
        if (!m)
            m = Match(unPaddedFrameSymRE, s);

        DB("    unpadded match '" << m << "'");

        while (m)
        {
            int f = frame;
            int pad = m.subLen(1);
            if (m.hasSub(2))
                f += m.subInt(2);
            if (m.hasSub(4))
                f *= m.subInt(4);
            if (m.hasSub(5))
                f %= m.subInt(5);
            if (m.hasSub(6))
                f += m.subInt(6);
            char fmtstr[16];
            snprintf(fmtstr, 16, "%%0%dd", pad);
            char fstr[16];
            snprintf(fstr, 16, fmtstr, f);
            s = s.replace(m.subStartPos(0), m.subLen(0), fstr);
            m = Match(unPaddedFrameSymRE, s);
        }

        m = Match(paddedFrameSymRENoEsc, s);
        if (!m)
            m = Match(paddedFrameSymRE, s);

        DB("    padded match '" << m << "'");

        while (m)
        {
            int f = frame;
            if (m.hasSub(2))
                f += m.subInt(2);
            if (m.hasSub(4))
                f *= m.subInt(4);
            if (m.hasSub(5))
                f %= m.subInt(5);
            if (m.hasSub(6))
                f += m.subInt(6);
            char fstr[16];
            snprintf(fstr, 16, "%04d", f);
            s = s.replace(m.subStartPos(0), m.subLen(0), fstr);
            m = Match(paddedFrameSymRE, s);
        }

        m = Match(unPaddedTickSymRENoEsc, s);
        if (!m)
            m = Match(unPaddedTickSymRE, s);

        DB("    unpaddedTick match '" << m << "'");

        while (m)
        {
            int f = frame;
            if (m.hasSub(1))
                fps = m.subInt(1);
            if (m.hasSub(2))
                f += m.subInt(2);
            if (m.hasSub(4))
                f *= m.subInt(4);
            if (m.hasSub(5))
                f %= m.subInt(5);
            if (m.hasSub(6))
                f += m.subInt(6);
            char fstr[16];
            snprintf(fstr, 16, "%d", (f * 6000 / fps));
            s = s.replace(m.subStartPos(0), m.subLen(0), fstr);
            m = Match(unPaddedTickSymRE, s);
        }

        m = Match(pctSyntaxRENoEsc, s);
        if (!m)
            m = Match(pctSyntaxRE, s);

        DB("    pcnt '" << m << "'");

        while (m)
        {
            int f = frame;
            if (m.hasSub(2))
                f += m.subInt(2);
            if (m.hasSub(4))
                f *= m.subInt(4);
            if (m.hasSub(5))
                f %= m.subInt(5);
            if (m.hasSub(6))
                f += m.subInt(6);
            char fstr[16];
            snprintf(fstr, 16, m.subStr(1).c_str(), f);
            s = s.replace(m.subStartPos(0), m.subLen(0), fstr);
            m = Match(pctSyntaxRE, s);
        }

        while (s.find("\\@") != s.npos)
            s = s.replace(s.find("\\@"), 2, "@");
        while (s.find("\\#") != s.npos)
            s = s.replace(s.find("\\#"), 2, "#");
        while (s.find("\\`") != s.npos)
            s = s.replace(s.find("\\`"), 2, "`");
        while (s.find("\\%") != s.npos)
            s = s.replace(s.find("\\%"), 2, "%");

        DB("    returning '" << s << "'");

        return s;
    }

    bool isFrameRange(const string& s) { return Match(onlyTimeRE, s); }

    FrameList frameRange(const string& s)
    {
        DB("frameRange s " << s);
        FrameList frames;
        vector<string> segments;
        stl_ext::tokenize(segments, s, string(","));

        for (unsigned int i = 0; i < segments.size(); ++i)
        {
            DB("    segment " << i << " " << segments[i]);
            Match m(timeRE2, segments[i]);

            if (!m)
            {
                cerr << "Invalid frame range: " << segments[i] << endl;
                continue;
            }

            int fs = m.subInt(0);
            int fe = fs;

            if (m.hasSub(1))
            {
                fe = m.subInt(1);
            }

            int fi = 1;

            if (m.hasSub(2))
            {
                fi = m.subInt(2);
            }

            for (int f = fs; f <= fe; f += fi)
            {
                DB("        adding frame " << f);
                frames.push_back(f);
            }
        }

        sort(frames.begin(), frames.end());
        frames.erase(unique(frames.begin(), frames.end()), frames.end());
        DB("returning list of " << frames.size() << " frames");
        return frames;
    }

    int guessIncrement(FrameList frames, int startPos, int lookahead)
    {
        // mR - consider when frames.size() <= 1
        // mR - consider when startPos >= frames.size()

        if (frames.size() <= 1 || (startPos + 1) >= frames.size())
            return 1;

        int iter = 1;
        iter = frames[startPos + 1] - frames[startPos];

        for (unsigned int i = startPos + 1; i < lookahead; ++i)
        {
            if ((i + 1) >= frames.size())
            {
                return 1;
            }

            if (frames[i + 1] - frames[i] != iter)
            {
                iter = 1;
                break;
            }
        }

        return iter;
    }

    string frameStr(FrameList frames)
    {
        // Create an empty time string
        ostringstream timeStr;

        // Special cases for empty list, or list with one number
        if (frames.size() == 0)
        {
            return "";
        }

        if (frames.size() == 1)
        {
            timeStr << frames[0];
            return timeStr.str();
        }

        // Remove duplicates and sort frames list
        sort(frames.begin(), frames.end());
        frames.erase(unique(frames.begin(), frames.end()), frames.end());

        // Begin the time range accumulator list
        FrameList frmRange;
        frmRange.push_back(frames[0]);

        // Figure out an initial frame increment
        int incr = guessIncrement(frames);

        // Iterate over frames
        for (unsigned int i = 0; i < frames.size(); ++i)
        {
            int frame = frames[i];
            int nextFrame = INT_MAX;
            if (i < frames.size() - 1)
            {
                nextFrame = frames[i + 1];
            }

            // Keep going as long as the numbers match the current increment
            if (nextFrame == frame + incr)
            {
                frmRange.push_back(frame);
                continue;
            }

            // If we didn't continue above, we've reached the end of a sequence
            // of consecutive numbers matching the current incr value.
            //
            // So, put the accumulated time range into the time string:
            if (frmRange.size() == 1)
            {
                timeStr << frame << ",";
            }
            else
            {
                if (incr == 1)
                {
                    timeStr << frmRange[0] << "-" << frame << ",";
                }
                else
                {
                    timeStr << frmRange[0] << "-" << frame << "x" << incr
                            << ",";
                }
            }

            // Clear the time range accumulator list, and begin the next one
            frmRange.clear();
            frmRange.push_back(nextFrame);

            // Figure out a new increment step
            incr = guessIncrement(frames, i + 1);
        }

        // Return the time string, chopping off the trailing comma
        return timeStr.str().substr(0, timeStr.str().size() - 1);
    }

    string firstFileInPattern(const SequenceName& seq)
    {
        ExistingFileList files = existingFilesInSequence(seq);
        return (files.size() > 0) ? files.front().name : "";
    }

    //
    //
    //

    string integrateFrameRange(const string& inpattern, const string& inrange)
    {
        static const RegEx split("(.*)([@#]+|%[-+ 0-9]*[di])(.*)");
        Match m = Match(split, inpattern);

        if (m)
        {
            ostringstream str;
            str << m.subStr(0) << inrange << m.subStr(1) << m.subStr(2);
            return str.str();
        }
        else
        {
            cout << "WARNING: ignoring sequence \"" << inpattern << " "
                 << inrange << "\" : not convertable" << endl;

            return inpattern;
        }
    }

    void convertNukeToShakeForm(const SequenceNameList& inlist,
                                SequenceNameList& outlist)
    {
        assert(inlist.size() % 2 == 0);

        for (unsigned int i = 0; i < inlist.size(); i += 2)
        {
            const string& pat = pathConform(inlist[i]);
            const string& frames = inlist[i + 1];

            outlist.push_back(integrateFrameRange(pat, frames));
        }
    }

    PatternFramePair sequenceOfFile(const std::string& infilename,
                                    SequencePredicate P, bool frameRanges)
    {
        string filename = pathConform(infilename);

        if (!fileExists(filename.c_str()))
            return PatternFramePair("", 0);
        string dir = dirname(filename);
        string file = basename(filename);
        SequenceNameList seqs = sequencesInDirectory(dir, P, true, frameRanges);

        for (int i = 0; i < seqs.size(); i++)
        {
            const string& seq = seqs[i];
            if (seq == file)
                return PatternFramePair(filename, 0);
            string t, p;

            if (!splitSequenceName(seq, t, p))
                continue;

            int start = 0;

            for (int q = 0; q < p.size() && q < file.size(); q++)
            {
                if (p[q] != file[q])
                    break;
                start++;
            }

            if (start == 0)
                continue;

            int end = 0;

            for (int q = 0; q < p.size() && q < file.size(); q++)
            {
                if (p[p.size() - q - 1] != file[file.size() - q - 1])
                    break;
                end++;
            }

            string n = file.substr(start, file.size() - end - start);
            int num = atoi(n.c_str());

            string test = replaceFrameSymbols(p, num);

            if (test == file)
            {
                string r = dir;
                r += "/";
                r += seq;
                return PatternFramePair(r, num);
            }
        }

        return PatternFramePair(filename, 0);
    }

} //  End namespace TwkUtil

#ifdef _MSC_VER
#undef snprintf
#endif
