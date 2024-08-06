//******************************************************************************
// Copyright (c) 2001-2003 Tweak Inc. All rights reserved.
// 
// SPDX-License-Identifier: Apache-2.0
// 
//******************************************************************************
#include <TwkUtil/File.h>
#include <TwkUtil/TwkRegEx.h>
#include <TwkUtil/RegexGlob.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <dirent.h>
#include <stdio.h>
#include <algorithm>

#ifndef _MSC_VER
#include <pwd.h>
#include <unistd.h>
#endif

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <codecvt>
#include <wchar.h>
#include <QFileInfo>
#include <TwkQtCoreUtil/QtConvert.h>
#include <mutex>
#endif

#include <time.h>
#include <stl_ext/string_algo.h>

#ifdef PLATFORM_WINDOWS
#include <windows.h>
extern Q_CORE_EXPORT int qt_ntfs_permission_lookup;
std::mutex g_perms_mutex;
#endif

namespace TwkUtil {
using namespace std;

#ifdef _MSC_VER
wstring to_wstring(const char *c)
{
    wstring_convert<codecvt_utf8<wchar_t> > wstr;
    return wstr.from_bytes(c);
}

string to_utf8(const wchar_t *wc)
{
    wstring_convert<codecvt_utf8_utf16<wchar_t>, wchar_t> wstr;
    return wstr.to_bytes(wc);
}

#ifdef PLATFORM_WINDOWS
bool fileExistsWithLongPathSupport(const string& filePath)
{
    // Convert the filepath to a wide string to accommodate all UTF-8 characters
    wstring wFilePath = to_wstring(filePath.c_str());
    wstring extendedFilePath = L"\\\\?\\" + wFilePath;

    HANDLE fileHandle = CreateFileW(
        extendedFilePath.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (fileHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(fileHandle);
        return true;
    }
    return false;
}
#endif

int stat(const char *c, struct _stat64 *buffer)
{
    return _wstat64(to_wstring(c).c_str(), buffer);
}

FILE* fopen(const char *c, const char *mode)
{
    return _wfopen(to_wstring(c).c_str(), to_wstring(mode).c_str());
}

int open(const char *c, int oflag)
{
    return _wopen(to_wstring(c).c_str(), oflag);
}

int access(const char *path, int mode)
{
    if( !isDirectory(path) ) 
    {
        return _waccess(to_wstring(path).c_str(), mode);
    }

    // access does not work with directories on Windows, so let's use Qt, but
    // Qt permission checks are turned off by default on Windows
    QFileInfo qf(TwkQtCoreUtil::UTF8::qconvert(path));

    {
      std::lock_guard<std::mutex> guard(g_perms_mutex);
      qt_ntfs_permission_lookup++;
    }

    int result = 0;
    if( mode | F_OK ) result = qf.exists() ? result : -1;
    if( mode | R_OK ) result = qf.isReadable() ? result : -1;
    if( mode | W_OK ) result = qf.isWritable() ? result : -1;

    {
      std::lock_guard<std::mutex> guard(g_perms_mutex);
      qt_ntfs_permission_lookup--;
    }

    return result;
}
#else
int stat(const char *c, struct stat *buffer)
{
    return ::stat(c, buffer);
}

FILE* fopen(const char *c, const char *mode)
{
    return ::fopen(c, mode);
}

int open(const char *c, int oflag)
{
    return ::open(c, oflag);
}

int access(const char *path, int mode)
{
    return ::access(path, mode);
}
#endif



//******************************************************************************
// Useful for finding the time of a file.
// Returns the time by reference, returns
// true if file exists, false otherwise.
bool modificationTime( const char *fname, time_t &modTime )
{
#ifdef _MSC_VER
    struct _stat64 statBuf;
#else
    struct stat statBuf;
#endif
    int status = TwkUtil::stat( fname, &statBuf );

    if ( status != 0 )
    {
        return false;
    }
    else
    {
        modTime = statBuf.st_mtime;
        return true;
    }
}

//******************************************************************************
bool creationTime( const char *fname, time_t &creTime )
{
#ifdef _MSC_VER
    struct _stat64 statBuf;
#else
    struct stat statBuf;
#endif
    int status = TwkUtil::stat( fname, &statBuf );

    if ( status != 0 )
    {
        return false;
    }
    else
    {
        creTime = statBuf.st_ctime;
        return true;
    }
}

//******************************************************************************
bool accessTime( const char *fname, time_t &accTime )
{
#ifdef _MSC_VER
    struct _stat64 statBuf;
#else
    struct stat statBuf;
#endif
    int status = TwkUtil::stat( fname, &statBuf );

    if ( status != 0 )
    {
        return false;
    }
    else
    {
        accTime = statBuf.st_atime;
        return true;
    }
}

//******************************************************************************
bool fileExists( const char *fname )
{
#ifdef _MSC_VER
    struct _stat64 statBuf;
#else
    struct stat statBuf;
#endif

#ifdef PLATFORM_WINDOWS

    string filePath = fname; // Casting to string
    if (filePath.length() > 260) return fileExistsWithLongPathSupport(filePath);

#endif

    int status = TwkUtil::stat( fname, &statBuf );

    if ( status != 0 )
    {
        return false;
    }
    else
    {
        return true;
    }
}

// *****************************************************************************
string dirname( string path )
{
    size_t lastSlash = path.rfind( "/" );
    if( lastSlash == path.npos )
    {
        return ".";
    }

    return path.substr( 0, lastSlash );
}

// *****************************************************************************
string basename( string path )
{
    size_t lastSlash = path.rfind( "/" );
    if( lastSlash == path.npos )
    {
        return path.c_str();
    }

    return path.substr( lastSlash + 1, path.size() );
}

// *****************************************************************************
string prefix( string path )
{
    string filename( basename( path ) );
    if( filename.find( "." ) == filename.npos )
    {
        return filename.c_str();
    }
    return filename.substr( 0, filename.find( "." ) );
}

// *****************************************************************************
int frameNumber( string path )
{
    string filename( basename( path ) );
    RegEx regEx( ".+\\.([0-9]+)\\..+" );
    Match fileMatch( regEx, filename );
    if( ! fileMatch )
    {
        return -1;
    }

    return fileMatch.subInt( 0 );
}

// *****************************************************************************
string extension( string path )
{
    string filename( basename( path ) );

    if( filename.find( "." ) == filename.npos )
    {
        return "";
    }
    return filename.substr( filename.rfind(".") + 1, filename.size() );
}


// *****************************************************************************
string fileOwner( string path )
{
    // mR - 10/28/07
#ifdef _MSC_VER
    return "" ;
#else
    struct stat sbuf;
    int result = TwkUtil::stat( path.c_str(), &sbuf );
    if( result != 0 )
    {
        return "";
    }

    struct passwd *pbuf = getpwuid( sbuf.st_uid );
    if( pbuf == NULL )
    {
        return "";
    }

    return pbuf->pw_name;
#endif
}

// *********************;********************************************************
string dateTimeStr( string path )
{
    time_t modTime;
    if( ! modificationTime( path.c_str(), modTime ) )
    {
        return "";
    }

    char strbuf[128];
    strftime( strbuf, 128, "%c", localtime( &modTime ) );
    strbuf[127] = 0;

    return strbuf;
}


// *****************************************************************************
FileNameList findInPath( string filepattern, string path )
{
    vector<string> matchedFiles;
    vector<string> pathdirs;

#ifdef WIN32
    stl_ext::tokenize( pathdirs, path, ";" );
#else
    stl_ext::tokenize( pathdirs, path, ":" );
#endif

    for (int d = 0; d < pathdirs.size(); ++d)
    {
        try
        {
            RegexGlob re(filepattern, pathdirs[d]);

            for (int f = 0; f < re.matchCount(); ++f)
            {
                matchedFiles.push_back( pathdirs[d] + "/" + re.fileName(f) );
            }
        }
        catch (...)
        {
            // Ignore errors in path
        }
    }

    return matchedFiles;
}

// *****************************************************************************
string tildeExp( const string &str )
{
    string tmp( str );
    tildeExp( tmp );
    return tmp;
}

// *****************************************************************************
void tildeExp( string &str )
{
    if (str.length() == 0 || str[0] != '~')
    {
        // Nothing to do here...
        return;
    }

// mR - 10/28/07
#ifndef _MSC_VER
    const char *pfx = NULL;
    string::size_type pos = str.find_first_of('/');

    if (str.length() == 1 || pos == 1)
    {
        pfx = getenv("HOME");
        if (!pfx)
        {
            // Punt. We're trying to expand ~/, but HOME isn't set
            struct passwd *pw = getpwuid(getuid());
            if (pw)
            {
                pfx = pw->pw_dir;
            }
        }
    }
    else
    {
        string user(str,1,(pos==string::npos) ? string::npos : pos-1);
        struct passwd *pw = getpwnam(user.c_str());
        if (pw)
        {
            pfx = pw->pw_dir;
        }
    }

    // if we failed to find an expansion, return the path unchanged.

    if (pfx)
    {
        str.replace( 0, pos, pfx );
    }
    return;
#endif
}

// *****************************************************************************
string varExp( const string &str )
{
    string tmp( str );
    varExp( tmp );
    return tmp;
}

// *****************************************************************************
void varExp( string &str )
{
    if( str.empty() || ( str.size() < 4 ) ) return;

    string::size_type posA = 0, posB = 0;
    static const char vardelim = '$';
    posA = str.find( vardelim );
    if( string::npos == posA )
    {
        return;
    }
    static const char opener = '{';
    static const char closer = '}';
    string tmpvar;
    static const string allowable_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_?"; // why a '?' ???
    char atc;
    posA = str.size() - 1;
    bool slashmode = false;
    for( ; posA != string::npos; posA-- )
    {
        atc = str[posA];
        if( atc != vardelim )
        {
            continue;
        }
        if( posA>0 && !slashmode && str[posA-1] == '\\' ) slashmode = true;
        if( slashmode )
        {
            slashmode = false;
            continue;
        }
        posB = str.find_first_not_of( allowable_chars, posA+1 );
        if( posB != posA +1 ) posB -= 1;
        if( posB == string::npos )
        {
            posB = str.size() -1;
        }

        tmpvar = string();
        if( posB == posA + 1 )
        {
            atc = str[posB];
            if( atc != opener )
            {
                tmpvar += atc;
                if( tmpvar.find_first_of( allowable_chars ) != 0 )
                {
                    tmpvar = string();
                }
            }
            else
            {
                const size_t maxpos = str.size()-1;
                while( atc != closer && posB <= maxpos )
                {
                    atc = str[++posB];
                    if ( atc != closer ) tmpvar += atc;
                }
            }
        }
        else
        {
            tmpvar = str.substr( posA+1, posB-posA );
        }
        if( tmpvar.empty() ) continue;
        if( getenv( tmpvar.c_str() ) )
        {
            str.erase( posA, posB - posA +1 );
            str.insert( posA, getenv( tmpvar.c_str() ) );
        }
    }
    return;
}

// *****************************************************************************
string varTildExp( const string &str )
{
    string tmp( str );
    tildeExp( tmp );
    varExp( tmp );
    return tmp;
}

// *****************************************************************************
void varTildExp( string &str )
{
    tildeExp( str );
    varExp( str );
}

// *****************************************************************************
bool filesInDirectory( const char *directory, FileNameList &fileList, bool showDirs )
{
    fileList.clear();

#ifdef _MSC_VER
    WIN32_FIND_DATAW find_data;
    wstring wFolderMask = to_wstring( directory ) + L"/*";
    HANDLE find_handle = FindFirstFileW( wFolderMask.c_str(), &find_data );
    if( find_handle == INVALID_HANDLE_VALUE )
    {
      return false;
    }

    do
    {
      wstring entry_name = find_data.cFileName;
      if( entry_name != L"." && entry_name != L".." )
      {
        if( !showDirs && ( find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) ) continue;
        fileList.push_back( to_utf8( entry_name.c_str() ) );
      }
    } while( FindNextFileW( find_handle, &find_data ) );

    FindClose( find_handle );
#else
    DIR *dir = opendir( directory );
    if( ! dir )
    {
        return false;
    }
    while( dirent *d = readdir( dir ) )
    {
        if( !showDirs && d->d_type == DT_DIR ) continue; // Do not return directories
        fileList.push_back( d->d_name );
    }

    closedir( dir );
#endif

    return true;
}

// *****************************************************************************
bool filesInDirectory( const char *directory, const char *pattern,
                       FileNameList &fileList,
                       bool showDirs)
{
    fileList.clear();

#ifdef _MSC_VER
    WIN32_FIND_DATAW find_data;
    wstring wFolderMask = to_wstring( directory ) + L"/*";
    HANDLE find_handle = FindFirstFileW( wFolderMask.c_str(), &find_data );
    if( find_handle == INVALID_HANDLE_VALUE )
    {
        return false;
    }
    do
    {
        wstring entry_name = find_data.cFileName;
        if( entry_name != L"." && entry_name != L".." )
        {
          if( !showDirs && ( find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) ) continue;

          const auto filename = to_utf8( entry_name.c_str() );
          if( fnmatch( pattern, filename.c_str(), 0 ) != FNM_NOMATCH )
          {
            fileList.push_back( filename );
          }
      }
    } while( FindNextFileW( find_handle, &find_data ) );

    FindClose( find_handle );
#else
    DIR *dir = opendir( directory );
    if( !dir )
    {
        return false;
    }
    while( dirent *d = readdir( dir ) )
    {
        if( fnmatch( pattern, d->d_name, 0 ) != FNM_NOMATCH )
        {
            if( !showDirs && d->d_type == DT_DIR ) continue; // Do not return directories
            fileList.push_back( d->d_name );
        }
    }

    closedir( dir );
#endif

    return true;
}


bool isDirectory(const char* directory)
{
#ifdef _MSC_VER
    DWORD ftyp = GetFileAttributesW( to_wstring( directory ).c_str() );
    return ftyp & FILE_ATTRIBUTE_DIRECTORY;
#else
    DIR *dir = opendir( directory );

    if (dir)
    {
        closedir(dir);
        return true;
    }
#endif

    return false;
}

bool isReadable(const char* path)
{
    return TwkUtil::access(path, R_OK) == 0;
}

bool isWritable(const char* path)
{
    return TwkUtil::access(path, W_OK) == 0;
}

//----------------------------------------------------------------------
//  lexinumeric sort
//

static const RegEx maybePaddedNumberRE = "(-?[0-9]+)";

struct NumString
{
    NumString(const std::string& s) : str(s), isnum(false), number(0) {}
    NumString(int n) : str(""), isnum(true), number(n) {}

    int number;
    std::string str;
    bool isnum;
};

struct SplitNumberedString
{
    std::vector<NumString> parts;
    std::string file;
};

typedef std::vector<SplitNumberedString> SplitNumberedStrings;



bool
lexinumericCompare(const SplitNumberedString& a, const SplitNumberedString& b)
{
    const size_t n = min(a.parts.size(), b.parts.size());

    for (size_t i = 0; i < n; i++)
    {
        const bool aisnum = a.parts[i].isnum;
        if (aisnum != b.parts[i].isnum) return aisnum;

        if (aisnum)
        {
            const int na = a.parts[i].number;
            const int nb = b.parts[i].number;
            if (na < nb) return true;
            else if (nb > na) return false;
        }
        else
        {
            const int c = a.parts[i].str.compare(b.parts[i].str);

            if (c < 0) return true;
            else if (c > 0) return false;
        }
    }

    return false;
}

void
makeSplitNumberedStrings(const FileNameList& files, SplitNumberedStrings& parts)
{
    parts.resize(files.size());

    for (size_t i = 0; i < files.size(); i++)
    {
        SplitNumberedString& element = parts[i];
        string file = files[i];
        element.file = file;
        
        while (Match m = Match(maybePaddedNumberRE, file))
        {
            string num  = m.subStr(0);
            size_t pos  = file.find(num);
            string p0("");

            if (pos != 0) p0 = file.substr(0, pos);
            file = file.substr(pos + num.size());

            if (p0 != "") element.parts.push_back(NumString(p0));
            element.parts.push_back(NumString(atoi(num.c_str())));
        }

        element.parts.push_back(NumString(file));

#if 0
        for (size_t i = 0; i < element.parts.size(); i++)
        {
            cout << " ";
            if (element.parts[i].isnum) cout << "#(" << element.parts[i].number << ")";
            else cout << "\"" << element.parts[i].str << "\"";
        }

        cout << endl;
#endif
    }
}

void
lexiNumericFileSort(FileNameList& files)
{
    SplitNumberedStrings sns;
    makeSplitNumberedStrings(files, sns);
    sort(sns.begin(), sns.end(), lexinumericCompare);

    for (size_t i = 0; i < sns.size(); i++)
    {
        files[i] = sns[i].file;
    }
}

#ifdef PLATFORM_WINDOWS
int
runAsAdministrator( std::string command, std::string directory )
{
    HWND hwnd = NULL;
    LPCSTR lpOperation = "runas";
    LPCTSTR lpFile = getenv("COMSPEC");
    if (!lpFile) lpFile = "C:\\Windows\\System32\\cmd.exe";
    LPCTSTR lpParameters = command.c_str();
    LPCTSTR lpDirectory = (directory.empty()) ? NULL : directory.c_str();
    int nShowCmd = SW_SHOWNA;

    int result = (int)ShellExecute ( hwnd,
                                     lpOperation,
                                     lpFile,
                                     lpParameters,
                                     lpDirectory,
                                     nShowCmd);
    return result;
}

int
copyAsAdministrator( std::string source, std::string destination, bool overwrite )
{
    std::string copy("/c copy \"" + source +  "\" \"" + destination + "\"");
    
    if ( overwrite )
    {
        copy += " /y";
    }

    return runAsAdministrator(copy);
}

#endif

}  //  End namespace TwkUtil
