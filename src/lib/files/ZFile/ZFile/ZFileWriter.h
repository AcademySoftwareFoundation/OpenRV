//-*****************************************************************************
// Copyright (c) 2001-2004 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//-*****************************************************************************

#include <ZFile/ZFileHeader.h>
#include <iostream>
#include <TwkMath/Mat44.h>

namespace ZFile
{

    //-*****************************************************************************
    struct ZFileWriter
    {
        static bool write(std::ostream& out, const Header& header,
                          const float* data);

        static bool write(std::ostream& out, int width, int height,
                          const TwkMath::Mat44f& w2s,
                          const TwkMath::Mat44f& w2c, const float* data);

        static bool write(const char* outFileName, const Header& header,
                          const float* data);

        static bool write(const char* outFileName, int width, int height,
                          const TwkMath::Mat44f& w2s,
                          const TwkMath::Mat44f& w2c, const float* data);
    };

} // End namespace ZFile
