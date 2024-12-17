//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkUtil__SystemInfo__h__
#define __TwkUtil__SystemInfo__h__
#include <TwkExc/TwkExcException.h>
#include <TwkUtil/dll_defs.h>

namespace TwkUtil
{

    class TWKUTIL_EXPORT SystemInfo
    {
    public:
        TWK_EXC_DECLARE(SystemCallFailedException, TwkExc::Exception,
                        "System call failed");

        //
        //  CPU information
        //

        static size_t numCPUs();

        //
        //  This function returns the number of "usable" bytes. This is
        //  approximately the amount of physical memory not being used by
        //  other applications. Or stated another way, the amount of
        //  memory could in theory allocate without causing swapping to
        //  occur.
        //

        static size_t usableMemory();

        // Returns information about the system memory status (in bytes).
        //    physTotal: total amount of physical memory.
        //    physFree: total amount of free physical memory. This value
        //    includes
        //              chached and inactive memory amounts since they can be
        //              reclaimed.
        //    physInactive: total amount inactive memory. Inactive memory is
        //    memory
        //                  that has not been recently used and can be reclaimed
        //                  for other purposes just like free memory. It is kept
        //                  around as an optimization in case it is required
        //                  again.
        //    physCached: total amount of cached memory. Cached memory is memory
        //    that
        //                contains information cached by the OS and can be
        //                reclaimed for other purposes just like free memory.
        //                The biggest client of cached memory is the file cache.
        //    swapTotal: total amount of swap space.
        //    swapFree: total amount of free swap space.
        //    swapUsed: total amount of used swap space.
        //
        // returns true when successful. Unwanted information can be ignored by
        // providing NULL pointers.
        static bool getSystemMemoryInfo(size_t* physTotal, size_t* physFree,
                                        size_t* physUsed, size_t* physInactive,
                                        size_t* physCached, size_t* swapTotal,
                                        size_t* swapFree, size_t* swapUsed);

        //
        //  Maximum usable VRAM (may be set by user)
        //

        static size_t maxVRAM() { return m_maxVRAM; }

        static void setMaxVRAM(size_t m) { m_maxVRAM = m; }

        //
        //  Set usable memory (instead of getting it from OS)
        //

        static void setUseableMemory(size_t m) { m_useableMemory = m; }

        //
        //  Return or set the graphics hardware bit depth. Since this
        //  function requires GL calls, it necessary to set the value from
        //  elsewhere in your application. For example, you could set it
        //  in main or in a plugin that uses GL.
        //
        //  If this function returns 0 -- then the value has not yet been
        //  defined.
        //

        static size_t maxColorBitDepth() { return m_colorBitDepth; }

        static void setMaxColorBitDepth(size_t d) { m_colorBitDepth = d; }

    private:
        static size_t m_colorBitDepth;
        static size_t m_maxVRAM;
        static size_t m_useableMemory;
    };

}; // namespace TwkUtil

#endif // __TwkUtil__SystemInfo__h__
