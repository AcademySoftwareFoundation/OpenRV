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
#include <TwkUtil/FileStream.h>

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

        FileStreamIStream(const std::string& filename,
                          Type type = FileStream::Buffering,
                          size_t chunkSize = 61440, int maxAsync = 16);

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

    private:
        FileStream m_stream;
        size_t m_index;
    };

} // namespace TwkFB

#endif // __IOexr__FileStreamIStream__h__
