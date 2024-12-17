//******************************************************************************
// Copyright (c) 2008 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <TwkExc/Exception.h>
#include <TwkUtil/FileMMap.h>
#include <TwkUtil/File.h>
#include <iostream>
#include <stdio.h>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stl_ext/replace_alloc.h>
#include <string.h>
#ifndef PLATFORM_WINDOWS
#include <unistd.h>
#endif
#ifdef WIN32
#include <posix_file.h>
#include <windows.h>
#endif

namespace TwkUtil
{
    using namespace std;

#ifndef WIN32

    FileMMap::FileMMap(const string& filename, bool reallyMMap)
        : file(-1)
        , rawdata(0)
        , fileSize(0)
        , deleteData(!reallyMMap)
    {
        if (!(file = open(filename.c_str(), O_RDONLY)))
        {
            TWK_THROW_EXC_STREAM("MMap: cannot open " << filename);
        }

        fileSize = lseek(file, 0, SEEK_END);

        if (reallyMMap)
        {
            rawdata = mmap(0, fileSize, PROT_READ, MAP_SHARED, file, 0);

            if (rawdata == MAP_FAILED)
            {
                close(file);
                TWK_THROW_EXC_STREAM("MMap: " << strerror(errno) << ": "
                                              << filename);
            }
        }
        else
        {
            lseek(file, 0, SEEK_SET);
            unsigned char* p = new unsigned char[fileSize];
            read(file, p, fileSize);
            rawdata = p;
        }
    }

    FileMMap::~FileMMap()
    {
        if (!deleteData)
        {
            if (rawdata && fileSize)
                munmap(rawdata, fileSize);
        }
        else
        {
            unsigned char* p = (unsigned char*)rawdata;
            delete[] p;
        }

        if (file != -1)
            close(file);
    }

#else

    FileMMap::FileMMap(const string& filename, bool reallyMMap)
        : fileHandle(0)
        , rawdata(0)
        , mapHandle(0)
        , fileSize(0)
        , deleteData(!reallyMMap)
    {
        //
        //  WINDOWS ONLY
        //

        struct __stat64 fileStat;
        int err = _wstat64(UNICODE_C_STR(filename.c_str()), &fileStat);

        if (0 != err)
        {
            TWK_THROW_EXC_STREAM("MMAp: no stat");
        }

        fileSize = fileStat.st_size;

        OFSTRUCT of;

        fileHandle = (long)CreateFile(
            filename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
            OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN | FILE_ATTRIBUTE_NORMAL,
            NULL);

        if (((HANDLE)fileHandle) == INVALID_HANDLE_VALUE)
        {
            TWK_THROW_EXC_STREAM("CreateFile: cannot open " << filename);
        }

        if (reallyMMap)
        {
            mapHandle = (long)CreateFileMapping(((HANDLE)fileHandle), NULL,
                                                PAGE_READONLY, 0, 0, NULL);

            if (!mapHandle || ((HANDLE)mapHandle) == INVALID_HANDLE_VALUE)
            {
                CloseHandle((HANDLE)fileHandle);
                TWK_THROW_EXC_STREAM("CreateFileMapping: cannot open "
                                     << filename);
            }

            rawdata =
                (void*)MapViewOfFile((HANDLE)mapHandle, FILE_MAP_READ, 0, 0, 0);

            if (!rawdata)
            {
                CloseHandle((HANDLE)fileHandle);
                TWK_THROW_EXC_STREAM("MapViewOfFile: cannot open " << filename);
            }
        }
        else
        {
            DWORD nread = 0;
            rawdata = TWK_ALLOCATE_ARRAY_PAGE_ALIGNED(void*, fileSize);

            if (!ReadFile((HANDLE)fileHandle, rawdata, fileSize, &nread, NULL))
            {
                CloseHandle((HANDLE)fileHandle);
                TWK_THROW_EXC_STREAM("ReadFile: failed reading " << filename);
            }

            CloseHandle((HANDLE)fileHandle);
        }
    }

    FileMMap::~FileMMap()
    {
        if (mapHandle)
        {
            if (rawdata)
                UnmapViewOfFile(rawdata);
            if (fileHandle && ((HANDLE)fileHandle) != INVALID_HANDLE_VALUE)
                CloseHandle((HANDLE)fileHandle);
        }
        else
        {
            TWK_DEALLOCATE_ARRAY(rawdata);
        }
    }

#endif

} // namespace TwkUtil
