//******************************************************************************
// Copyright (c) 2001-2004 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef __FRAMEUTILS_H__
#define __FRAMEUTILS_H__

#include <string>
#include <vector>
#include <limits>
#include <set>
#include <TwkUtil/File.h>

namespace TwkUtil
{

    //
    //  Types
    //
    //  NOTE: there is a difference between a SequenceName and a
    //  SequencePattern. A SequenceName corresponds to existing files on
    //  disk. So for example, you cannot have offset or modulo in a
    //  SequenceName. A SequencePattern on the other hand, can have offset
    //  and modulo.
    //

    struct TWKUTIL_EXPORT ExistingFile
    {
        std::string name;
        int frame;
        bool exists : 1;
        bool readable : 1;
    };

    typedef std::string SequenceName;
    typedef std::string SequencePattern;
    typedef std::pair<SequencePattern, int> PatternFramePair;
    typedef std::vector<SequenceName> SequenceNameList;
    typedef std::vector<ExistingFile> ExistingFileList;
    typedef std::vector<int> FrameList;
    typedef std::set<std::string> SequencePredicateExtensionSet;
    typedef std::pair<std::string, std::string> StringPair;
    typedef std::vector<StringPair> StringPairVector;

    //
    //  Type of a function given a file or sequence pattern should return
    //  true if it *might* be a sequence or part of a sequence. If it
    //  returns the false, then the passed in string is a single file name
    //  and should not be contracted or expanded according to sequence
    //  rules.
    //

    typedef bool (*SequencePredicate)(const std::string&);

    //
    //  Predicates
    //

    TWKUTIL_EXPORT bool AnySequencePredicate(const std::string&);
    TWKUTIL_EXPORT bool FalseSequencePredicate(const std::string&);
    TWKUTIL_EXPORT bool GlobalExtensionPredicate(const std::string&);

    TWKUTIL_EXPORT SequencePredicateExtensionSet& predicateFileExtensions();

    //
    //  Returns all the SequenceNames in a list of files
    //
    //  If includeNonMatching is false, single files not part of a multi-frame
    //  sequence will be omitted.  If frameRanges is false, frame sequences will
    //  be returned without embedded frame information (i.e. just "foo.#.tif").
    //  Files will not be counted as part of a sequence if the size of the
    //  sequence they could make is fewer than minSequenceSize.
    //
    TWKUTIL_EXPORT SequenceNameList
    sequencesInFileList(const FileNameList& infiles, SequencePredicate P = NULL,
                        bool includeNonMatching = true, bool frameRanges = true,
                        int minSequenceSize = -1);

    //
    //  Returns all the SequenceNames in a directory.
    //
    //  If includeNonMatching is false, single files not part of a
    //  multi-frame sequence will be omitted.  If frameRanges is false,
    //  frame sequences will be returned without embedded frame
    //  information (i.e. just "foo.#.tif") The SequencePredicate function
    //  can be used to determine what files are acceptable as
    //  sequences. For example numbered .mov files may not be a good
    //  candidate for creating a sequence.
    //
    TWKUTIL_EXPORT SequenceNameList
    sequencesInDirectory(const std::string& path, SequencePredicate P = NULL,
                         bool includeNonMatching = true,
                         bool frameRanges = true, bool showDirs = false);

    //
    // Splits SequenceName with embedded frame range (i.e. "foo.1-10#.tif")
    // into a time string parseable with frameRange() and a sequencePattern
    // suitable for use with replaceFrameSymbols().  In the case where there
    // is no time range, timeStr will be an empty string, and sequencePattern
    // will be the same as sequenceName.  Currently only supports # and @ frame
    // cookies.  It will return false if there is no frame cookie at all.
    //
    TWKUTIL_EXPORT bool splitSequenceName(const SequenceName& sequenceName,
                                          std::string& timeStr,
                                          std::string& sequencePattern);

    //
    //  Returns a SequencePattern (i.e. foo.1-10#.tif) from an existing file
    //  list. If onlyExisting is true, only files which exist on disk will be
    //  represented in the returned pattern.
    //
    TWKUTIL_EXPORT SequencePattern sequencePattern(const ExistingFileList& efl,
                                                   bool onlyExisting = true,
                                                   int minSequenceSize = 1);

    //
    //  Existing sequences for stereo
    //

    TWKUTIL_EXPORT bool isStereoSequence(SequenceName);

    //
    // Returns an ordered list of ExistingFiles for each frame in a
    // SequenceName. If the SequenceName contains a time range, there will be an
    // entry for each frame, which may or may not exist on disk.  If there is a
    // frame cookie without a time range, only existing files matching
    // that pattern will be returned, and may or may not be consecutive (though
    // they will be in order).
    //
    TWKUTIL_EXPORT ExistingFileList
    existingFilesInSequence(const SequenceName&, bool justOne = false);

    //
    // Returns the first existing file in a sequence
    //
    TWKUTIL_EXPORT std::string firstFileInPattern(const SequenceName& seq);

    //
    //     Replaces ALL occurrences of frame symbols in string s with
    //     the given frame number.  fps is used only for Maya tick
    //     replacement.  The following symbols are understood:
    //
    //     @ -> unpadded frame number (repeatable, see examples)
    //     # -> frame number padded with 0 to four digits
    //     ` -> Maya "ticks", primarily used with .pdc files
    //     %d -> Standard Python/printf-style syntax (integers only)
    //
    //     You can specify a multipler with a 'x' after any frame symbol.
    //     You can specify a modulo with a '/' after any frame symbol.
    //
    //     You can specify a +/- frame offset after any frame symbol.
    //     The offset can be before and/or after the modulo, and will be
    //     calculated that order.
    //
    //     You can override the default fps with Maya tick replacements
    //     by specifying the frame rate immediately after the ` (see the
    //     last example below).
    //
    //     Symbols for which substitution is not desired can be escaped
    //     with a backslash ('\').  The backslash will be removed.
    //
    //     A few examples, assume frame = 12:
    //
    //     foo.%d.xxx      ->  foo.12.xxx
    //     foo.%04d.xxx    ->  foo.0012.xxx
    //     foo.@.xxx       ->  foo.12.xxx
    //     foo.@@@.xxx     ->  foo.012.xxx
    //     foo.#.xxx       ->  foo.0012.xxx
    //     foo.@+10.xxx    ->  foo.22.xxx
    //     foo.#-10.xxx    ->  foo.0002.xxx
    //     foo.`.xxx       ->  foo.3000.xxx
    //     foo.`30-10.xxx  ->  foo.0400.xxx
    //     foo.#/7.xxx     ->  foo.0005.xxx
    //     foo.#/7+1.xxx   ->  foo.0006.xxx
    //     foo\@.#.xxx     ->  foo@.0012.xxx
    //
    TWKUTIL_EXPORT std::string replaceFrameSymbols(const std::string& s,
                                                   int frame, int fps = 24);

    //
    //     Parse a Shake-like time string and return a list of corresponding
    //     numbers.
    //
    //     1-10          Frames 1 through 10
    //     1-10x2        Frames 1, 3, 5, 7, and 9 (but not 10!)
    //     1,10,20-30x2  Frames 1, 10, 20, 22, 24, 26, 28, and 30
    //     -15--10       Frames -15, -14, -13, -12, -11, and -10
    //
    //     List returned will NOT contain any duplicate numbers, and will be
    //     sorted lowest -> highest
    //
    //     Negative frame numbers are handled correctly.
    //

    TWKUTIL_EXPORT FrameList frameRange(const std::string& s);

    //
    //  Returns true if s looks like a frame range (e.g. "1-10" or "-1-10" or
    //  "1-10x2")
    //

    TWKUTIL_EXPORT bool isFrameRange(const std::string& s);

    //
    //     Return the number by which the first 'lookahead' numbers
    //     in frameList are increasing (or decreasing I suppose).  If
    //     the first 'lookahead' numbers aren't increasing by a constant
    //     amount, or there are fewer than 'lookahead' numbers in the
    //     list, 1 is returned.
    //
    TWKUTIL_EXPORT int guessIncrement(FrameList frames, int startPos = 0,
                                      int lookahead = 3);

    //
    //     Given a list of numbers, come up with a Shake-like time string
    //     that can be parsed with frameRange() above.   Duplicate numbers
    //     will be removed, and the frame spec will be sorted lowest -> highest.
    //

    TWKUTIL_EXPORT std::string frameStr(FrameList frames);

    //
    //  Integrate frame range (e.g. 1-10) into pattern (e.g. file.%04d.jpg and
    //  1-10 become file.1-10%04d.jpg)
    //

    TWKUTIL_EXPORT std::string integrateFrameRange(const std::string& inpattern,
                                                   const std::string& inrange);

    //
    //     Convert sequences in Nuke sequence form to Shake form
    //     The argument will be modified
    //

    TWKUTIL_EXPORT void convertNukeToShakeForm(const SequenceNameList& in,
                                               SequenceNameList& out);

    //
    //      Sequence of file
    //
    //      Find the sequence on disk that the passed in file is a part of
    //      If the file doesn't exist it returns "" If the file is not
    //      part of a sequence it returns the filename The
    //      SequencePredicate and frameRanges args are passed to
    //      sequencesInDirectory().
    //

    TWKUTIL_EXPORT PatternFramePair sequenceOfFile(const std::string& filename,
                                                   SequencePredicate P = NULL,
                                                   bool frameRanges = true);

} //  End namespace  TwkUtil

#endif // End #ifdef __FRAMEUTILS_H__
