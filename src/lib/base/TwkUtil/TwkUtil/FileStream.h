//******************************************************************************
// Copyright (c) 2008 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkUtil__FileStream__h__
#define __TwkUtil__FileStream__h__
#include <string>
#ifdef WIN32
#define ssize_t long
#endif
#include <TwkUtil/dll_defs.h>

namespace TwkUtil
{

    /// Stream a file

    ///
    /// This class encapulates cross-platform "file streaming". File
    /// streaming in this use of the phrase means "openning a file once,
    /// reading it sequentially and completely into a memory buffer".
    ///
    /// This class may be using memory mapping, direct i/o, or advice on
    /// file descriptors. You can choose the method when you create
    /// it. Not all platforms have the same capabilities so in some cases
    /// the prefered method will not be available and another will be used
    /// instead.
    ///
    /// If deleteOnDestruction is false: you need to use the macro
    /// TWK_DEALLOCATE to free the memory returned by data();
    ///

    class TWKUTIL_EXPORT FileStream
    {
    public:
        enum Type
        {
            Buffering,
            NonBuffering,
            MemoryMap,
            ASyncBuffering,
            ASyncNonBuffering
        };

        FileStream(const std::string& filename, size_t startOffset,
                   size_t readSize, Type type = Buffering,
                   size_t chunkSize = 61440, int maxInFlight = 16,
                   bool deleteOnDestruction = true);

        FileStream(const std::string& filename, Type type = Buffering,
                   size_t chunkSize = 61440, int maxInFlight = 16,
                   bool deleteOnDestruction = true);

        ~FileStream();

        void* data() const { return m_rawdata; }

        ssize_t size() const { return m_fileSize; }

        void setDeleteOnDestruction(bool b) { m_deleteOnDestruction = b; }

        static void deleteDataPointer(void*);

        //
        //  Return MB/sec throughput.  Note that this doesn't work for
        //  MemoryMap IO operations, since the actual IO in that case is
        //  deferred.  This number is an average over all time.
        //  resetMbps resets the average.
        //
        static double mbps();
        static void resetMbps();

    private:
        void initialize();

    private:
        std::string m_filename;
        Type m_type;
        size_t m_chunkSize;
        void* m_rawdata;
        ssize_t m_fileSize;
        size_t m_startOffset;
        size_t m_readSize;
        size_t m_size;
        void* m_private;
        int m_file;
        int m_maxInFlight;
        bool m_deleteOnDestruction;
    };

} // namespace TwkUtil

#endif // __TwkUtil__FileStream__h__
