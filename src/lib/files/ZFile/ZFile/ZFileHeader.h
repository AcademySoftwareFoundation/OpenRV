//*****************************************************************************
// Copyright (c) 2001 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//*****************************************************************************

#ifndef __ZFile__ZFileHeader__h__
#define __ZFile__ZFileHeader__h__

namespace ZFile
{

    typedef float Matrix44[16];
    typedef int int32;
    typedef unsigned int uint32;
    typedef short int16;
    typedef unsigned short uint16;

    //
    // struct Header
    //
    // Header structure for the Pixar zfile format.  All values stored
    // in a zfile are in the native byte-order of the system which
    // wrote it.  All the data after the header to the end of the file
    // is IEEE 32 bit floating point numbers (imageWidth*imageHeight
    // in total).
    //

    struct Header
    {
        static const unsigned int Magic = 0x2f0867ab;
        static const unsigned int Cigam = 0xab67082f;

        uint32 magicNumber;     // magic number, either Magic or Cigam
        uint16 imageWidth;      // width of image in pixels
        uint16 imageHeight;     // height of image in pixels
        Matrix44 worldToScreen; // row major world to camera transform
        Matrix44 worldToCamera; // row major world to screen transform
    };

} // namespace ZFile

#endif // __ZFile__ZFileHeader__h__
