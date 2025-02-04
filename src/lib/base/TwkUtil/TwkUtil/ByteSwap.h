//******************************************************************************
// Copyright (c) 2001 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkUtilByteSwap_h_
#define _TwkUtilByteSwap_h_

#include <stdio.h>
#ifndef PLATFORM_WINDOWS
#include <stdint.h>
#endif
#include <algorithm>
#include <iostream>

// mR - 10/28/07
#ifdef _MSC_VER
// visual studio doesn't have inttypes.h
// (http://forums.microsoft.com/MSDN/ShowPost.aspx?PostID=135168&SiteID=1)
#ifndef uint16_t
#define uint16_t __int16
#endif

#ifndef uint32_t
#define uint32_t __int32
#endif
#endif

namespace TwkUtil
{

    //******************************************************************************
    //******************************************************************************
    // INLINE ROUTINES
    //******************************************************************************
    //******************************************************************************
    inline void swapWords(void* data, size_t size)
    {
        for (uint32_t *ip = (uint32_t*)data, *ep = ip + size; ip < ep; ip++)
        {
            const uint32_t x = *ip;

            *ip = ((uint32_t)((((uint32_t)(x) & 0xff000000) >> 24)
                              | (((uint32_t)(x) & 0x00ff0000) >> 8)
                              | (((uint32_t)(x) & 0x0000ff00) << 8)
                              | (((uint32_t)(x) & 0x000000ff) << 24)));
        }
    }

    inline void swapShorts(void* data, size_t size)
    {
        for (uint16_t *ip = (uint16_t*)data, *ep = ip + size; ip < ep; ip++)
        {
            const uint16_t x = *ip;

            *ip = ((uint16_t)((((uint16_t)(x) & 0xff00) >> 8)
                              | (((uint16_t)(x) & 0x00ff) << 8)));
        }
    }

    inline void swapShortsCopy(void* dst, const void* src, size_t size)
    {
        const uint16_t* ip = (const uint16_t*)src;
        for (uint16_t *op = (uint16_t*)dst, *ep = op + size; op < ep;
             ip++, op++)
        {
            const uint16_t x = *ip;

            *op = ((uint16_t)((((uint16_t)(x) & 0xff00) >> 8)
                              | (((uint16_t)(x) & 0x00ff) << 8)));
        }
    }

    //******************************************************************************
    //******************************************************************************
    // READ ROUTINES
    //******************************************************************************
    //******************************************************************************
    inline unsigned short readSwappedUshort(std::istream& stream)
    {
        char buf[2];
        stream.read(buf, 2);
        std::swap(buf[0], buf[1]);
        return *((unsigned short*)(buf));
    }

    //******************************************************************************
    inline short readSwappedShort(std::istream& stream)
    {
        char buf[2];
        stream.read(buf, 2);
        std::swap(buf[0], buf[1]);
        return *((short*)(buf));
    }

    //******************************************************************************
    inline int readSwappedInt(std::istream& stream)
    {
        char buf[4];
        stream.read(buf, 4);
        std::swap(buf[0], buf[3]);
        std::swap(buf[1], buf[2]);
        return *((long*)(buf));
    }

    //******************************************************************************
    inline unsigned int readSwappedUint(std::istream& stream)
    {
        char buf[4];
        stream.read(buf, 4);
        std::swap(buf[0], buf[3]);
        std::swap(buf[1], buf[2]);
        return *((unsigned int*)(buf));
    }

    //******************************************************************************
    inline long readSwappedLong(std::istream& stream)
    {
        char buf[4];
        stream.read(buf, 4);
        std::swap(buf[0], buf[3]);
        std::swap(buf[1], buf[2]);
        return *((long*)(buf));
    }

    //******************************************************************************
    inline unsigned long readSwappedUlong(std::istream& stream)
    {
        char buf[4];
        stream.read(buf, 4);
        std::swap(buf[0], buf[3]);
        std::swap(buf[1], buf[2]);
        return *((unsigned long*)(buf));
    }

    //******************************************************************************
    inline float readSwappedFloat(std::istream& stream)
    {
        char buf[4];
        stream.read(buf, 4);
        std::swap(buf[0], buf[3]);
        std::swap(buf[1], buf[2]);
        return *((float*)(buf));
    }

    //******************************************************************************
    //******************************************************************************
    // READ ROUTINES FOR FILES
    //******************************************************************************
    //******************************************************************************
    inline unsigned short readSwappedUshort(FILE* stream)
    {
        char buf[2];
        int r = fread((void*)buf, 2, 1, stream);
        std::swap(buf[0], buf[1]);
        return *((unsigned short*)(buf));
    }

    //******************************************************************************
    inline short readSwappedShort(FILE* stream)
    {
        char buf[2];
        int r = fread((void*)buf, 2, 1, stream);
        std::swap(buf[0], buf[1]);
        return *((short*)(buf));
    }

    //******************************************************************************
    inline int readSwappedInt(FILE* stream)
    {
        char buf[4];
        int r = fread((void*)buf, 4, 1, stream);
        std::swap(buf[0], buf[3]);
        std::swap(buf[1], buf[2]);
        return *((long*)(buf));
    }

    //******************************************************************************
    inline unsigned int readSwappedUint(FILE* stream)
    {
        char buf[4];
        int r = fread((void*)buf, 4, 1, stream);
        std::swap(buf[0], buf[3]);
        std::swap(buf[1], buf[2]);
        return *((unsigned int*)(buf));
    }

    //******************************************************************************
    inline long readSwappedLong(FILE* stream)
    {
        char buf[4];
        int r = fread((void*)buf, 4, 1, stream);
        std::swap(buf[0], buf[3]);
        std::swap(buf[1], buf[2]);
        return *((long*)(buf));
    }

    //******************************************************************************
    inline unsigned long readSwappedUlong(FILE* stream)
    {
        char buf[4];
        int r = fread((void*)buf, 4, 1, stream);
        std::swap(buf[0], buf[3]);
        std::swap(buf[1], buf[2]);
        return *((unsigned long*)(buf));
    }

    //******************************************************************************
    inline float readSwappedFloat(FILE* stream)
    {
        char buf[4];
        int r = fread((void*)buf, 4, 1, stream);
        std::swap(buf[0], buf[3]);
        std::swap(buf[1], buf[2]);
        return *((float*)(buf));
    }

    //******************************************************************************
    //******************************************************************************
    // WRITE ROUTINES
    //******************************************************************************
    //******************************************************************************
    inline void writeSwappedUshort(std::ostream& stream, unsigned short val)
    {
        char* buf = (char*)&val;
        std::swap(buf[0], buf[1]);
        stream.write(buf, 2);
    }

    //******************************************************************************
    inline void writeSwappedShort(std::ostream& stream, short val)
    {
        char* buf = (char*)&val;
        std::swap(buf[0], buf[1]);
        stream.write(buf, 2);
    }

    //******************************************************************************
    inline void writeSwappedInt(std::ostream& stream, int val)
    {
        char* buf = (char*)&val;
        std::swap(buf[0], buf[3]);
        std::swap(buf[1], buf[2]);
        stream.write(buf, 4);
    }

    //******************************************************************************
    inline void writeSwappedUint(std::ostream& stream, unsigned int val)
    {
        char* buf = (char*)&val;
        std::swap(buf[0], buf[3]);
        std::swap(buf[1], buf[2]);
        stream.write(buf, 4);
    }

    //******************************************************************************
    inline void writeSwappedLong(std::ostream& stream, long val)
    {
        char* buf = (char*)&val;
        std::swap(buf[0], buf[3]);
        std::swap(buf[1], buf[2]);
        stream.write(buf, 4);
    }

    //******************************************************************************
    inline void writeSwappedUlong(std::ostream& stream, unsigned long val)
    {
        char* buf = (char*)&val;
        std::swap(buf[0], buf[3]);
        std::swap(buf[1], buf[2]);
        stream.write(buf, 4);
    }

    //******************************************************************************
    inline void writeSwappedFloat(std::ostream& stream, float val)
    {
        char* buf = (char*)&val;
        std::swap(buf[0], buf[3]);
        std::swap(buf[1], buf[2]);
        stream.write(buf, 4);
    }

    //******************************************************************************
    //******************************************************************************
    // WRITE ROUTINES FOR FILES
    //******************************************************************************
    //******************************************************************************
    inline void writeSwappedUshort(FILE* stream, unsigned short val)
    {
        char* buf = (char*)&val;
        std::swap(buf[0], buf[1]);
        fwrite((const void*)buf, 2, 1, stream);
    }

    //******************************************************************************
    inline void writeSwappedShort(FILE* stream, short val)
    {
        char* buf = (char*)&val;
        std::swap(buf[0], buf[1]);
        fwrite((const void*)buf, 2, 1, stream);
    }

    //******************************************************************************
    inline void writeSwappedInt(FILE* stream, int val)
    {
        char* buf = (char*)&val;
        std::swap(buf[0], buf[3]);
        std::swap(buf[1], buf[2]);
        fwrite((const void*)buf, 4, 1, stream);
    }

    //******************************************************************************
    inline void writeSwappedUint(FILE* stream, unsigned int val)
    {
        char* buf = (char*)&val;
        std::swap(buf[0], buf[3]);
        std::swap(buf[1], buf[2]);
        fwrite((const void*)buf, 4, 1, stream);
    }

    //******************************************************************************
    inline void writeSwappedLong(FILE* stream, long val)
    {
        char* buf = (char*)&val;
        std::swap(buf[0], buf[3]);
        std::swap(buf[1], buf[2]);
        fwrite((const void*)buf, 4, 1, stream);
    }

    //******************************************************************************
    inline void writeSwappedUlong(FILE* stream, unsigned long val)
    {
        char* buf = (char*)&val;
        std::swap(buf[0], buf[3]);
        std::swap(buf[1], buf[2]);
        fwrite((const void*)buf, 4, 1, stream);
    }

    //******************************************************************************
    inline void writeSwappedFloat(FILE* stream, float val)
    {
        char* buf = (char*)&val;
        std::swap(buf[0], buf[3]);
        std::swap(buf[1], buf[2]);
        fwrite((const void*)buf, 4, 1, stream);
    }

} // End namespace TwkUtil

#ifdef _MSC_VER

#ifdef uint16_t
#undef uint16_t
#endif

#ifdef uint32_t
#undef uint32_t
#endif

#endif

#endif
