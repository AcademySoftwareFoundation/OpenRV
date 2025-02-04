//
//  Copyright (c) 2009 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IOcin__Read8Bit__h__
#define __IOcin__Read8Bit__h__
#include <TwkExc/TwkExcException.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkUtil/ByteSwap.h>
#include <TwkFB/IO.h>
#include <fstream>

namespace TwkFB
{

    //
    //  class Read10Bit
    //
    //  Methods of reading memory mapped 10 bit cineon/dpx pixels
    //

    class Read8Bit
    {
    public:
        //
        //  Types
        //

        typedef unsigned int U32;
        typedef unsigned int UInt;
        typedef unsigned short U16;
        typedef unsigned char U8;
        typedef int S32;
        typedef float R32;
        typedef char ASCII;

        typedef U8 Pixel;

        static void planarConfig(TwkFB::FrameBuffer&, int, int,
                                 TwkFB::FrameBuffer::DataType);

        static void readRGB8(const std::string&, const unsigned char*,
                             TwkFB::FrameBuffer&, int, int, size_t maxbytes,
                             bool swap, bool useRaw = false,
                             unsigned char* deletePointer = 0);

        static void readRGBA8(const std::string&, const unsigned char*,
                              TwkFB::FrameBuffer&, int, int, size_t maxbytes,
                              bool swap, bool useRaw = false,
                              unsigned char* deletePointer = 0);

        static void readRGB8_PLANAR(const std::string&, const unsigned char*,
                                    TwkFB::FrameBuffer&, int, int,
                                    size_t maxbytes, bool swap);
    };

} // namespace TwkFB

#endif // __IOcin__Read8Bit__h__
