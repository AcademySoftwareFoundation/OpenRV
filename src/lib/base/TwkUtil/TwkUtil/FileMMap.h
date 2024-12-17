//******************************************************************************
// Copyright (c) 2008 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkUtil__FileMMap__h__
#define __TwkUtil__FileMMap__h__

#include <string>
#ifdef WIN32
#define ssize_t long
#else
#include <sys/mman.h>
#include <sys/types.h>
#endif

#include <TwkUtil/dll_defs.h>

namespace TwkUtil
{

    /// Setup a memory maped file for reading

    ///
    /// This struct is just a simple way to make a memory mapped file for
    /// reading without worrying about exception safety. After
    /// constructed, the rawdata field will have a pointer to the
    /// beginning of the memory mapped region. The file is mapped
    /// read-only.
    ///

#ifndef WIN32

    struct TWKUTIL_EXPORT FileMMap
    {
        FileMMap(const std::string& filename, bool reallyMMap = false);
        ~FileMMap();

        void* rawdata;
        ssize_t fileSize;

        int file;
        bool deleteData;
    };

#else

    struct FileMMap
    {
        FileMMap(const std::string& filename, bool reallyMMap = true);
        ~FileMMap();

        void* rawdata;
        ssize_t fileSize;

        long fileHandle;
        long mapHandle;

        bool deleteData;
    };

#endif

} // namespace TwkUtil

#endif // __TwkUtil__FileMMap__h__
