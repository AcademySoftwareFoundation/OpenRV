//-*****************************************************************************
// Copyright (c) 2001-2004 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//-*****************************************************************************

#include <ZFile/ZFileWriter.h>
#include <ZFile/ZFileHeader.h>
#include <TwkUtil/File.h>
#include <fstream>

using namespace TwkMath;

namespace ZFile
{

    //-*****************************************************************************
    bool ZFileWriter::write(std::ostream& out, const Header& header,
                            const float* data)
    {
        out.write((const char*)&header.magicNumber, sizeof(header.magicNumber));
        out.write((const char*)&header.imageWidth, sizeof(header.imageWidth));
        out.write((const char*)&header.imageHeight, sizeof(header.imageHeight));
        out.write((const char*)&header.worldToScreen,
                  sizeof(header.worldToScreen));
        out.write((const char*)&header.worldToCamera,
                  sizeof(header.worldToCamera));

        size_t rowSize = header.imageWidth * sizeof(float);
        for (int row = header.imageHeight - 1; row >= 0; --row)
        {
            out.write((const char*)(data + row * header.imageWidth), rowSize);
        }

        if (!out)
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    //-*****************************************************************************
    bool ZFileWriter::write(const char* fileName, const Header& header,
                            const float* data)
    {
        std::ofstream out(UNICODE_C_STR(fileName), std::ofstream::binary);

        if (!out)
        {
            return false;
        }
        else
        {
            return write(out, header, data);
        }
    }

    //-*****************************************************************************
    bool ZFileWriter::write(std::ostream& out, int width, int height,
                            const Mat44f& w2s, const Mat44f& w2c,
                            const float* data)
    {
        Header header;
        header.magicNumber = Header::Magic;
        header.imageWidth = width;
        header.imageHeight = height;
        const Mat44f w2st = w2s.transposed();
        const Mat44f w2ct = w2c.transposed();
        memcpy((void*)header.worldToScreen, (const void*)&w2st,
               16 * sizeof(float));
        memcpy((void*)header.worldToCamera, (const void*)&w2ct,
               16 * sizeof(float));

        return write(out, header, data);
    }

    //-*****************************************************************************
    bool ZFileWriter::write(const char* fileName, int width, int height,
                            const Mat44f& w2s, const Mat44f& w2c,
                            const float* data)
    {
        std::ofstream out(UNICODE_C_STR(fileName), std::ofstream::binary);

        if (!out)
        {
            return false;
        }
        else
        {
            return write(out, width, height, w2s, w2c, data);
        }
    }

} // End namespace ZFile
