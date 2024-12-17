//******************************************************************************
// Copyright (c) 2001-2003 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef __TWKIMGRGBEIFF_H__
#define __TWKIMGRGBEIFF_H__

#include <TwkExc/TwkExcException.h>
#include <TwkImg/TwkImgImage.h>

namespace TwkImg
{

    TWK_EXC_DECLARE(Exception, TwkExc::Exception, "RgbeIff::Exception: ");

    class RgbeIff
    {

    public:
        static const unsigned short Magic = 0x01DA;
        static const unsigned short Cigam = 0xDA01;

        typedef struct
        {
            unsigned short magic;
            char storage;
            char bpc;
            unsigned short dimension;
            unsigned short xsize;
            unsigned short ysize;
            unsigned short zsize;
            long pixmin;
            long pixmax;
            char dummy[4];
            char imagename[80];
            long colormap;
            char dummy2[404];
        } SGI_HDR;

        static TwkImg::Img4f* read(const char* filename);
    };

} //  End namespace TwkImg

#endif // End #ifdef __TWKIMGRGBEIFF_H__
