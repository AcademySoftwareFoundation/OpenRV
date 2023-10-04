//******************************************************************************
// Copyright (c) 2001-2002 Tweak Inc. All rights reserved.
// 
// SPDX-License-Identifier: Apache-2.0
// 
//******************************************************************************

#ifndef _TwkUtilFile_h_
#define _TwkUtilFile_h_

#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string>
#include <vector>
#include <TwkUtil/dll_defs.h>


//
// Use macro UNICODE_C_STR to convert "const char*" arg to unicode
// versions. i.e. on Windows it returns "const wchar_t*" while
// on linux/osx it returns what was passed in.
// e.g. ifstream mystream(UNICODE_C_STR(filename));
// 
// Use macro UNICODE_STR to convert std::string arg to unicode
// string version. i.e. on Windows it returns "std::wstring" while
// on linux/osx it returns what was passed in.
//
#ifndef UNICODE_C_STR
#ifdef _MSC_VER
#define UNICODE_STR(_str) TwkUtil::to_wstring(_str.c_str())
#define UNICODE_C_STR(_cstr) TwkUtil::to_wstring(_cstr).c_str()
#else
#define UNICODE_STR(_str) _str
#define UNICODE_C_STR(_cstr) _cstr
#endif
#endif

namespace TwkUtil {

typedef std::vector<std::string> FileNameList;

// *****************************************************************************
#ifdef _MSC_VER
#define F_OK 0
#define W_OK 2
#define R_OK 4
TWKUTIL_EXPORT std::wstring to_wstring(const char *c);
TWKUTIL_EXPORT std::string to_utf8(const wchar_t *wc);
TWKUTIL_EXPORT int stat(const char *c, struct _stat64 *buffer);
#else
TWKUTIL_EXPORT int stat(const char *c, struct stat *buffer);
#endif
TWKUTIL_EXPORT FILE* fopen(const char *c, const char *mode);
TWKUTIL_EXPORT int open(const char *c, int oflag);
TWKUTIL_EXPORT int access(const char *path, int mode);

TWKUTIL_EXPORT bool modificationTime( const char *fname, time_t &modTime );
TWKUTIL_EXPORT bool creationTime( const char *fname, time_t &creTime );
TWKUTIL_EXPORT bool accessTime( const char *fname, time_t &accTime );
TWKUTIL_EXPORT bool fileExists( const char *fname );

// Returns the directory part of the path
TWKUTIL_EXPORT std::string dirname( std::string path );

// Returns the file part of the path
TWKUTIL_EXPORT std::string basename( std::string path );

// Returns "file" for "/tmp/file.tif"
TWKUTIL_EXPORT std::string prefix( std::string path );

// Returns the frame number if file is name.####.ext, else -1
TWKUTIL_EXPORT int frameNumber( std::string path );

// Returns the file extension (if there is one, "" otherwise )
TWKUTIL_EXPORT std::string extension( std::string path );

// Expand ~
TWKUTIL_EXPORT void tildeExp( std::string &str );  // Expands inline
TWKUTIL_EXPORT std::string tildeExp( const std::string &str ); // Returns new string

// Expand $ environment variables
TWKUTIL_EXPORT void varExp( std::string &str );  // Expands inline
TWKUTIL_EXPORT std::string varExp( const std::string &str ); // Returns new string

// Expand both ~ and $ environment variables 
TWKUTIL_EXPORT void varTildExp( std::string &str );  // Expands inline
TWKUTIL_EXPORT std::string varTildExp( const std::string &str ); // Returns new string

// Returns the username of the file's owner
TWKUTIL_EXPORT std::string fileOwner( std::string path );

// Returns a pretty date/time string for given file's modification time
TWKUTIL_EXPORT std::string dateTimeStr( std::string path );

// Searches a : separated path for filename(s) matching pattern, returns 
// list of matches with full path to each file.
TWKUTIL_EXPORT FileNameList findInPath( std::string filepattern, 
                                        std::string path );

// Returns a list of all filenames in the given directory.  The
// full path is NOT prepended to the filenames.
TWKUTIL_EXPORT bool filesInDirectory(const char *directory, 
                      FileNameList &fileList,
                      bool showDirs = false);

// Returns a list of all filenames in the given directory that match
// the given file pattern as determined by fnmatch().  The full path 
// is NOT prepended to the filenames.
TWKUTIL_EXPORT bool filesInDirectory( const char *directory, 
                                      const char *pattern,
                                      FileNameList &fileList,
                                      bool showDirs = false);

TWKUTIL_EXPORT bool isDirectory(const char* filename);

inline bool isDirectory(const std::string& filename) 
{ 
    return isDirectory(filename.c_str()); 
}

// Convenience wrappers for access
TWKUTIL_EXPORT bool isReadable(const char* path);
TWKUTIL_EXPORT bool isWritable(const char* path);

//
//  Sort a file name list so that negative numbers, etc, are handled
//  numerically instead of lexically
//

TWKUTIL_EXPORT void lexiNumericFileSort(FileNameList&);

#ifdef PLATFORM_WINDOWS
//
// A function for acquiring admin privileges on Windows and executing
// an arbitrary shell command.
//
TWKUTIL_EXPORT int runAsAdministrator( std::string command,
                                       std::string directory = "" );
//
// Accompanying convenience function for asking for admin permissions
// to copy files.
//
TWKUTIL_EXPORT int copyAsAdministrator( std::string source,
                                        std::string destination,
                                        bool overwrite = false );
#endif
} // End namespace TwkUtil

#endif
