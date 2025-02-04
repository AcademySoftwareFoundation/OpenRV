//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __Read12Bit__Read12Bit__h__
#define __Read12Bit__Read12Bit__h__
#include <TwkExc/TwkExcException.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkUtil/ByteSwap.h>
#include <TwkFB/IO.h>
#include <fstream>

namespace TwkFB
{

    //
    //  class Read12Bit
    //
    //  Methods of reading memory mapped 12 bit cineon/dpx pixels
    //

    class Read12Bit
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

        static void planarConfig(TwkFB::FrameBuffer&, int, int,
                                 TwkFB::FrameBuffer::DataType);

        static void readRGB8_PLANAR(const std::string&, const unsigned char*,
                                    TwkFB::FrameBuffer&, int, int,
                                    size_t maxbytes, bool swap);

        static void readRGB16_PLANAR(const std::string&, const unsigned char*,
                                     TwkFB::FrameBuffer&, int, int,
                                     size_t maxbytes, bool swap);

        static void readNoPaddingRGB16(const std::string&, const unsigned char*,
                                       TwkFB::FrameBuffer&, int, int,
                                       size_t maxbytes, bool swap);
    };

} // namespace TwkFB

#endif // __Read12Bit__Read12Bit__h__
