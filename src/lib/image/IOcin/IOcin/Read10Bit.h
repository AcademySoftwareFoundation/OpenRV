//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __Read10Bit__Read10Bit__h__
#define __Read10Bit__Read10Bit__h__
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

    class Read10Bit
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

        struct Pixel
        {
            union
            {
                struct
                {
#if defined(TWK_LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN__)
                    UInt unused : 2;
                    UInt blue : 10;
                    UInt green : 10;
                    UInt red : 10;
#else
                    UInt red : 10;
                    UInt green : 10;
                    UInt blue : 10;
                    UInt unused : 2;
#endif
                };

                struct
                {
#if defined(TWK_LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN__)
                    UInt unused2 : 2;
                    UInt blue_least : 2;
                    UInt blue_most : 8;
                    UInt green_least : 2;
                    UInt green_most : 8;
                    UInt red_least : 2;
                    UInt red_most : 8;
#else
                    UInt red_most : 8;
                    UInt red_least : 2;
                    UInt green_most : 8;
                    UInt green_leaast : 2;
                    UInt blue_most : 8;
                    UInt blue_least : 2;
                    UInt unused2 : 2;
#endif
                };

                U32 pixelWord;
            };

            void setR(unsigned int R) { red = (R & 1023U); }

            void setG(unsigned int G) { green = (G & 1023U); }

            void setB(unsigned int B) { blue = (B & 1023U); }
        };

        static void planarConfig(TwkFB::FrameBuffer&, int, int,
                                 TwkFB::FrameBuffer::DataType);

        static void readRGB8(const std::string&, const unsigned char*,
                             TwkFB::FrameBuffer&, int, int, size_t maxbytes,
                             bool swap);

        static void readRGBA8(const std::string&, const unsigned char*,
                              TwkFB::FrameBuffer&, int, int, size_t maxbytes,
                              bool alpha, bool swap);

        static void readRGB16(const std::string&, const unsigned char*,
                              TwkFB::FrameBuffer&, int, int, size_t maxbytes,
                              bool swap);

        static void readRGBA16(const std::string&, const unsigned char*,
                               TwkFB::FrameBuffer&, int, int, size_t maxbytes,
                               bool alpha, bool swap);

        static void readRGB10_A2(const std::string&, const unsigned char*,
                                 TwkFB::FrameBuffer&, int, int, size_t maxbytes,
                                 bool swap, bool useRaw = false,
                                 unsigned char* deletePointer = 0);

        static void readA2_BGR10(const std::string&, const unsigned char*,
                                 TwkFB::FrameBuffer&, int, int, size_t maxbytes,
                                 bool swap);

        static void readRGB8_PLANAR(const std::string&, const unsigned char*,
                                    TwkFB::FrameBuffer&, int, int,
                                    size_t maxbytes, bool swap);

        static void readRGB16_PLANAR(const std::string&, const unsigned char*,
                                     TwkFB::FrameBuffer&, int, int,
                                     size_t maxbytes, bool swap);

        //

        static void readYCrYCb8_422_PLANAR(const std::string&,
                                           const unsigned char*,
                                           TwkFB::FrameBuffer&, int, int,
                                           size_t maxbytes, bool swap);
    };

} // namespace TwkFB

#endif // __Read10Bit__Read10Bit__h__
