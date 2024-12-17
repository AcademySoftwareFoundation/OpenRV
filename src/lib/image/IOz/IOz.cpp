//******************************************************************************
// Copyright (c) 2001-2004 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <IOz/IOz.h>
#include <TwkFB/Exception.h>
#include <TwkMath/Mat44.h>
#include <TwkUtil/File.h>
#include <ZFile/ZFileReader.h>
#include <iostream>
#include <sstream>
#include <string>

namespace TwkFB
{
    using namespace std;
    using namespace TwkMath;

    static bool normalizeOnInput = true;

    IOz::IOz(bool n)
        : FrameBufferIO()
        , m_normalizeOnInput(n)
    {
        unsigned int cap = ImageRead | BruteForceIO;
        addType("z", "Pixar Z-Depth", cap);
        addType("zfile", "Pixar Z-Depth", cap);
    }

    IOz::~IOz() {}

    string IOz::about() const
    {
        char temp[80];
        sprintf(temp, "Pixar Z (depth) file");
        return temp;
    }

    void IOz::getImageInfo(const std::string& filename, FBInfo& fbi) const
    {
        FILE* infile;
        ZFile::Header header;

        if (!(infile = TwkUtil::fopen(filename.c_str(), "rb")))
        {
            TWK_THROW_STREAM(IOException, "ZFile: cannot open " << filename);
        }

        fread(&header, sizeof(ZFile::Header), 1, infile);
        fclose(infile);

        if (header.magicNumber != ZFile::Header::Magic)
        {
            TWK_THROW_STREAM(IOException, "ZFile: not a z file: " << filename);
        }

        fbi.dataType = FrameBuffer::FLOAT;
        fbi.width = header.imageWidth;
        fbi.height = header.imageHeight;
        fbi.numChannels = 1;
        fbi.orientation = FrameBuffer::NATURAL;

        ostringstream Sstr;
        ostringstream Cstr;

        for (int i = 0; i < 16; i++)
        {
            if (i)
            {
                Sstr << " ";
                Cstr << " ";
            }
            Sstr << header.worldToScreen[i];
            Cstr << header.worldToCamera[i];
        }

        fbi.proxy.newAttribute("ZFile/worldToScreen", Sstr.str());
        fbi.proxy.newAttribute("ZFile/worldToCamera", Cstr.str());
        fbi.proxy.setPrimaryColorSpace(ColorSpace::NonColorData());
        fbi.proxy.setTransferFunction(ColorSpace::Linear());
    }

    void IOz::readImage(FrameBuffer& fb, const std::string& filename,
                        const ReadRequest& request) const
    {
        ZFile::Reader reader;
        reader.setInputFile(filename.c_str());

        if (reader.error() || reader.isEmpty())
        {
            TWK_THROW_STREAM(IOException, "ZFile: error reading: " << filename);
        }

        size_t w = reader.header().imageWidth;
        size_t h = reader.header().imageHeight;
        // fb.restructure(w, h, 0, 1, FrameBuffer::FLOAT, 0, 0,
        // FrameBuffer::TOPLEFT);
        fb.restructure(w, h, 0, 1, FrameBuffer::FLOAT);

        if (m_normalizeOnInput)
        {
            for (float *p = const_cast<float*>(reader.data()), *e = p + (w * h);
                 p != e; ++p)
            {
                if (*p >= 1e30)
                    *p = 0;
            }
        }

        memcpy(fb.pixels<float>(), reader.data(), sizeof(float) * w * h);

        ostringstream Sstr;
        ostringstream Cstr;

        for (int i = 0; i < 16; i++)
        {
            if (i)
            {
                Sstr << " ";
                Cstr << " ";
            }
            Sstr << reader.header().worldToScreen[i];
            Cstr << reader.header().worldToCamera[i];
        }

        fb.newAttribute("ZFile/worldToScreen", Sstr.str());
        fb.newAttribute("ZFile/worldToCamera", Cstr.str());
        fb.setPrimaryColorSpace(ColorSpace::NonColorData());
        fb.setTransferFunction(ColorSpace::Linear());
    }

    void IOz::writeImage(const FrameBuffer& img, const std::string& filename,
                         const WriteRequest& request) const
    {
    }

} //  End namespace TwkFB
