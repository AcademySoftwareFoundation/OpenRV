//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IOjpeg__IOjpeg__h__
#define __IOjpeg__IOjpeg__h__
#include <TwkExc/TwkExcException.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkFB/StreamingIO.h>
#include <TwkUtil/FileStream.h>
#include <stdio.h>

struct jpeg_decompress_struct;

namespace TwkFB
{

    //
    //  IOjpeg
    //
    //  The JPEG reader can be configured to return YUV planar if the
    //  StorageFormat is passed into the constructor. The reader will only
    //  do this if the image is YCbCr. Otherwise it will use the built in
    //  color space converter to RGB.
    //

    class IOjpeg : public StreamingFrameBufferIO
    {
    public:
        enum StorageFormat
        {
            RGB,
            YUV,
            RGBA
        };

        struct FileState
        {
            FileState(const std::string& name = "", FILE* f = 0,
                      TwkUtil::FileStream* s = 0)
                : filename(name)
                , file(f)
                , stream(s)
            {
            }

            ~FileState()
            {
                if (file)
                    fclose(file);
                delete stream;
            }

            std::string filename;
            FILE* file;
            TwkUtil::FileStream* stream;
        };

        typedef struct jpeg_decompress_struct Decompressor;

        IOjpeg(StorageFormat format = RGB, IOType ioMethod = StandardIO,
               size_t chunkSize = 61440, int maxAsync = 16);

        virtual ~IOjpeg();

        virtual void readImage(FrameBuffer& fb, const std::string& filename,
                               const ReadRequest& request) const;
        virtual void writeImage(const FrameBuffer& img,
                                const std::string& filename,
                                const WriteRequest& request) const;
        virtual std::string about() const;
        virtual void getImageInfo(const std::string& filename, FBInfo&) const;

        void format(StorageFormat f) { m_format = f; }

        void throwError(const FileState&) const;

        mutable bool m_error;

    private:
        void readImageRGB(FrameBuffer& fb, const FileState&,
                          Decompressor&) const;
        void readImageRGBA(FrameBuffer& fb, const FileState&,
                           Decompressor&) const;
        void readImageYUV(FrameBuffer& fb, const FileState&,
                          Decompressor&) const;

        bool canReadAsYUV(const Decompressor&) const;

        void planarConfig(FrameBuffer&, Decompressor&) const;
        void readAttributes(FrameBuffer&, Decompressor&) const;

    private:
        StorageFormat m_format;
    };

} // namespace TwkFB

#endif // __IOjpeg__IOjpeg__h__
