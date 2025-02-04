//******************************************************************************
// Copyright (c) 2001 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkImgTiffIff_h_
#define _TwkImgTiffIff_h_

#include <TwkImg/TwkImgImage.h>
#include <TwkMath/Mat44.h>

namespace TwkImg
{

    //******************************************************************************
    class TiffIff
    {
    public:
        static Img4f* read(const char* fileName, bool showWarnings = false);
        static Img4uc* read4uc(const char* fileName, bool showWarnings = false);

        static bool write(const Img4f* img, const char* fileName,
                          const int depth = 16); // 8, 16, or 32 (float)
        static bool write(const Img4f* img, const char* fileName,
                          const TwkMath::Mat44f& Mcamera,
                          const TwkMath::Mat44f& Mscreen, const int depth = 16);

        static bool write(const Img4uc* img, const char* fileName);
        static bool write(const Img4uc* img, const char* fileName,
                          const TwkMath::Mat44f& Mcamera,
                          const TwkMath::Mat44f& Mscreen);

        static bool write(const Img4us* img, const char* fileName);
        static bool write(const Img4us* img, const char* fileName,
                          const TwkMath::Mat44f& Mcamera,
                          const TwkMath::Mat44f& Mscreen);
    };

} // End namespace TwkImg

#endif
