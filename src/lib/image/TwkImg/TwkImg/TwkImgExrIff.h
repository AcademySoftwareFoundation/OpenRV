//******************************************************************************
// Copyright (c) 2001 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkImgExrIff_h_
#define _TwkImgExrIff_h_

#include <TwkImg/TwkImgImage.h>
#include <TwkMath/Mat44.h>

#include <ImfCompression.h>

namespace TwkImg
{

    //******************************************************************************
    class ExrIff
    {
    public:
        static Img4f* read(const char* fileName);

        static Img4f* read(const char* fileName, TwkMath::Mat44f& Mcamera,
                           TwkMath::Mat44f& Mscreen);

        static bool write(const Img4f* img, const char* fileName,
                          Imf::Compression compression = Imf::PIZ_COMPRESSION);

        static bool write(const Img4f* img, const char* fileName,
                          const TwkMath::Mat44f& Mcamera,
                          const TwkMath::Mat44f& Mscreen,
                          Imf::Compression compression = Imf::PIZ_COMPRESSION);
    };

} // End namespace TwkImg

#endif
