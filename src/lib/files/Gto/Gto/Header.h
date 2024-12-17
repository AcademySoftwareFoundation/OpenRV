//******************************************************************************
// Copyright (c) 2002 Tweak Films.
//
// Copyright (c) 2009, Tweak Software
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#ifndef __Gto__Header__h__
#define __Gto__Header__h__

#ifdef __APPLE__
#ifndef PLATFORM_DARWIN
#define PLATFORM_DARWIN
#endif
#endif

namespace Gto
{

    //
    //  Types and MACROS
    //

#define GTO_MAGIC 0x29f
#define GTO_MAGICl 0x9f020000
#define GTO_MAGIC_TEXT 0x47544f61
#define GTO_MAGIC_TEXTl 0x614f5447
#define GTO_VERSION 4

    typedef unsigned int uint32;
    typedef int int32;
    typedef unsigned short uint16;
    typedef unsigned char uint8;
    typedef float float32;
    typedef double float64;

    struct Dimensions
    {
        Dimensions(uint32 _x = 1, uint32 _y = 0, uint32 _z = 0, uint32 _w = 0)
            : x(_x)
            , y(_y)
            , z(_z)
            , w(_w)
        {
        }

        uint32 x;
        uint32 y;
        uint32 z;
        uint32 w;
    };

    //
    //  File Header
    //

    struct Header
    {
        static const unsigned int MagicText = GTO_MAGIC_TEXT;
        static const unsigned int CigamText = GTO_MAGIC_TEXTl;
        static const unsigned int Magic = GTO_MAGIC;
        static const unsigned int Cigam = GTO_MAGICl;

        uint32 magic;
        uint32 numStrings;
        uint32 numObjects;
        uint32 version;
        uint32 flags; // undetermined;
    };

    //
    //  Object Header
    //

    struct ObjectHeader
    {
        uint32 name;         // string
        uint32 protocolName; // string
        uint32 protocolVersion;
        uint32 numComponents;
        uint32 pad;
    };

    struct ObjectHeader_v2
    {
        uint32 name;         // string
        uint32 protocolName; // string
        uint32 protocolVersion;
        uint32 numComponents;
    };

    //
    //  Componenent Header
    //

    enum ComponentFlags
    {
        Transposed = 1 << 0,
        Matrix = 1 << 1,
    };

    struct ComponentHeader
    {
        uint32 name; // string
        uint32 numProperties;
        uint32 flags;
        uint32 interpretation;
        uint32 childLevel;
    };

    struct ComponentHeader_v2
    {
        uint32 name; // string
        uint32 numProperties;
        uint32 flags;
    };

    //
    //  Property Header
    //

    enum DataType
    {
        Int,     // int32
        Float,   // float32
        Double,  // float64
        Half,    // float16
        String,  // string table indices
        Boolean, // bit
        Short,   // uint16
        Byte,    // uint8

        NumberOfDataTypes,
        ErrorType
    };

    struct PropertyHeader
    {
        PropertyHeader()
            : dims(0, 0, 0, 0)
        {
        }

        uint32 name; // string
        uint32 size;
        uint32 type;
        Dimensions dims;
        uint32 interpretation; // string
    };

    struct PropertyHeader_v3
    {
        uint32 name; // string
        uint32 size;
        uint32 type;
        uint32 width;
        uint32 interpretation; // string
        uint32 pad;
    };

    struct PropertyHeader_v2
    {
        uint32 name;
        uint32 size;
        uint32 type;
        uint32 width;
    };

} // namespace Gto

#endif // __Gto__Header__h__
