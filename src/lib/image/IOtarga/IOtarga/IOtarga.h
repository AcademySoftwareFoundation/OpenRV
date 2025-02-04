//
//  Copyright (c) 2009 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IOtarga__IOtarga__h__
#define __IOtarga__IOtarga__h__
#include <iostream>
#include <TwkExc/TwkExcException.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkUtil/ByteSwap.h>
#include <TwkFB/StreamingIO.h>
#include <fstream>
#include <pthread.h>

namespace TwkFB
{

    class IOtarga : public StreamingFrameBufferIO
    {
    public:
        //
        //  NOTE: file is always little endian
        //

        typedef unsigned int U32;
        typedef unsigned int UInt;
        typedef unsigned short U16;
        typedef unsigned char U8;
        typedef int S32;
        typedef float R32;
        typedef char ASCII;

        enum ImageType
        {
            NoImageDataType = 0,
            RawColorMapType = 1,
            RawTrueColorType = 2,
            RawBlackAndWhiteType = 3,
            RLEColorMapType = 9,
            RLETrueColorType = 10,
            RLEBlackAndWhiteType = 11
        };

        struct ColorMapSpec
        {
            U16 startIndex;
            U16 length;
            U8 entrySize;

            void swap() { TwkUtil::swapShorts(&startIndex, 2); }
        };

        struct ImageSpec
        {
            U16 xorigin;
            U16 yorigin;
            U16 width;
            U16 height;
            U8 pixelDepth;
            U8 descriptor; // 2 unused | 2 orientation | 4 alpha bits

            void swap() { TwkUtil::swapShorts(&xorigin, 4); }
        };

        struct TGAFileHeader
        {
            U8 IDLength;
            U8 colorMapType;
            U8 imageType;
            ColorMapSpec colorMapSpec; // will cause mis-alignment if imageSpec
            ImageSpec imageSpec;

            void swap()
            {
                colorMapSpec.swap();
                imageSpec.swap();
            }
        };

        struct TGAFileFooter
        {
            U32 extOffset;
            U32 devOffset;
            ASCII signature[18]; // wacky string footer + 8 ==
                                 // "TRUEVISION-XFILE.\0" means "new tga"
        };

        IOtarga(IOType ioMethod = StandardIO, size_t chunkSize = 61440,
                int maxAsync = 16);

        virtual ~IOtarga();

        virtual void readImage(FrameBuffer& fb, const std::string& filename,
                               const ReadRequest& request) const;
        virtual void writeImage(const FrameBuffer& img,
                                const std::string& filename,
                                const WriteRequest& request) const;
        virtual std::string about() const;
        virtual void getImageInfo(const std::string& filename, FBInfo&) const;

    private:
        bool sanityCheck(const TGAFileHeader&) const;
        const char* typeToString(ImageType) const;
        void readAttributes(FrameBuffer& fb, const TGAFileHeader& header) const;
        void readHeader(const unsigned char*, TGAFileHeader&) const;
        void readFooter(const unsigned char*, TGAFileFooter&) const;
        void readExtensionArea(FrameBuffer& fb,
                               const unsigned char* data) const;
        void writeRLE(std::ofstream* outfile, unsigned char* pixel,
                      int numChannels, int count) const;
        bool pixelsMatch(const unsigned char* p1, const unsigned char* p2,
                         bool alpha) const;
    };

} // namespace TwkFB

#endif // __IOtarga__IOtarga__h__
