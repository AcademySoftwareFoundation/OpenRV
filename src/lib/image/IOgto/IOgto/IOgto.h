//
//  Copyright (c) 2009 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IOgto__IOgto__h__
#define __IOgto__IOgto__h__
#include <iostream>
#include <TwkExc/TwkExcException.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkUtil/ByteSwap.h>
#include <TwkFB/StreamingIO.h>
#include <fstream>
#include <pthread.h>

namespace Gto
{
    class Reader;
    class Writer;
} // namespace Gto

namespace TwkFB
{

    //
    //  IOgto
    //
    //  GTO can encode multiple layer/view single images or even movie
    //  files. This code handles single image encoding/decoding and
    //  functions useful for doing the same as a movie format.
    //
    //  The main purpose of this is to serialize a TwkFB::FrameBuffer or
    //  possibly a vector of them. This is used by the 32/64 sidecar. This
    //  format would also work well as a disk cache or ingestion format
    //  (with proxies) since it can store multiple images efficiently (not
    //  interleaved) and allows seeking.
    //

    class IOgto : public StreamingFrameBufferIO
    {
    public:
        //
        //  Types
        //

        struct WriteState
        {
            size_t index;
            std::string viewName;
            const FrameBuffer* fb;
            std::vector<std::string> planeNames;
            std::vector<const TwkFB::FrameBuffer*> planes;
        };

        //
        //  Constructors
        //

        IOgto(IOType ioMethod = StandardIO, size_t chunkSize = 61440,
              int maxAsync = 16);

        virtual ~IOgto();

        virtual void readImages(FrameBufferVector& fbs,
                                const std::string& filename,
                                const ReadRequest& request) const;

        virtual void writeImages(const ConstFrameBufferVector& img,
                                 const std::string& filename,
                                 const WriteRequest& request) const;

        virtual void writeImageStream(const ConstFrameBufferVector&,
                                      std::ostream&, const WriteRequest&) const;

        virtual void readImageStream(FrameBufferVector&, std::istream&,
                                     const std::string& filename,
                                     const ReadRequest&) const;

        virtual void readImageInMemory(FrameBufferVector&, void* data,
                                       size_t size, const std::string& filename,
                                       const ReadRequest&) const;

        virtual std::string about() const;
        virtual void getImageInfo(const std::string& filename, FBInfo&) const;

    private:
        void writeOneImageStream(Gto::Writer&, WriteState&, std::ostream&,
                                 const WriteRequest&) const;

        void declareOneImageStream(Gto::Writer&, WriteState&,
                                   const WriteRequest&) const;
    };

} // namespace TwkFB

#endif // __IOgto__IOgto__h__
