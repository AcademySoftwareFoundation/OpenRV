//
//  Copyright (c) 2009 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IOexr__FileStreamIStream__h__
#define __IOexr__FileStreamIStream__h__
#include <iostream>
#include <ImfStdIO.h>
#include <OpenEXRConfig.h>
#include <TwkUtil/FileStream.h>

//
//  OpenEXR 3.3 rewired the C++ API onto the OpenEXRCore reader. Streams that
//  do not report their size() or support stateless (offset-based, thread-safe)
//  reads fall onto a much slower read path and cannot have their scanlines
//  decompressed concurrently. Since the whole file is already resident in one
//  buffer (see TwkUtil::FileStream), we can implement both cheaply. Guard the
//  overrides so the class still builds against pre-3.3 OpenEXR (which lacks
//  these virtuals).
//
#if OPENEXR_VERSION_MAJOR > 3 || (OPENEXR_VERSION_MAJOR == 3 && OPENEXR_VERSION_MINOR >= 3)
#define IOEXR_HAS_STATELESS_ISTREAM 1
#endif

namespace TwkFB
{

    /// An Imf::IStream which uses TwkUtil::FileStream to pre-load image data

    ///
    /// This class uses TwkUtil::FileStream to scarf data into memory
    /// completely (or as a memory map). The main reason to use this is to
    /// pass in FileStream's Async type to get better performance on
    /// windows across the network. On Linux/Mac its questionable whether
    /// this class is helpful.
    ///

    class FileStreamIStream : public Imf::IStream
    {
    public:
        typedef TwkUtil::FileStream FileStream;
        typedef FileStream::Type Type;

        FileStreamIStream(const std::string& filename, Type type = FileStream::Buffering, size_t chunkSize = 61440, int maxAsync = 16);

        virtual ~FileStreamIStream();

        //
        //  Imf::IStream API
        //

        virtual bool isMemoryMapped() const;
        virtual bool read(char c[/*n*/], int n);
        virtual char* readMemoryMapped(int n);
        virtual uint64_t tellg();
        virtual void seekg(uint64_t pos);
        virtual void clear();

#ifdef IOEXR_HAS_STATELESS_ISTREAM
        //
        //  The complete file is already in a single contiguous buffer, so we
        //  can report its size and service concurrent, offset-based reads
        //  directly from that buffer without touching any shared state. This
        //  restores the fast/parallel reader path in OpenEXR >= 3.3.
        //
        virtual int64_t size();
        virtual bool isStatelessRead() const;
        virtual int64_t read(void* buf, uint64_t sz, uint64_t offset);
#endif

    private:
        FileStream m_stream;
        size_t m_index;
    };

} // namespace TwkFB

#endif // __IOexr__FileStreamIStream__h__
