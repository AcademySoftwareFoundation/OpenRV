//******************************************************************************
// Copyright (c) 2001 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkImgCineonIff_h_
#define _TwkImgCineonIff_h_

#include <TwkImg/TwkImgImage.h>
#include <iosfwd>

namespace TwkImg
{

    //******************************************************************************
    class CineonIff
    {
    public:
        static Img4f* read(const char* fileName, int codeShiftR = 0,
                           int codeShiftG = 0, int codeShiftB = 0,
                           bool linearize = true);

        static Img4f* read(std::istream& in, int codeShiftR = 0,
                           int codeShiftG = 0, int codeShiftB = 0,
                           bool linearize = true);

        static void write(const Img4f* img, const char* fileName,
                          int codeShiftR = 0, int codeShiftG = 0,
                          int codeShiftB = 0, bool deLinearize = true);

        // Image file name needed because it is stored
        // in the header
        static void write(const Img4f* img, std::ostream& out,
                          const char* fileName, int codeShiftR = 0,
                          int codeShiftG = 0, int codeShiftB = 0,
                          bool deLinearize = true);
    };

} // End namespace TwkImg

#endif
