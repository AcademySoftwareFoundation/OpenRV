//******************************************************************************
// Copyright (c) 2001-2004 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <IOrgbe/IOrgbe.h>
#include <IOrgbe/rgbe.h>
#include <TwkFB/Exception.h>
#include <TwkFB/Operations.h>
#include <TwkImg/TwkImgCineonIff.h>
#include <TwkUtil/Interrupt.h>
#include <TwkUtil/File.h>
#include <TwkMath/Color.h>
#include <fstream>
#include <iostream>
#include <stl_ext/string_algo.h>
#include <string>

namespace TwkFB
{
    using namespace std;
    using namespace TwkUtil;
    using namespace TwkMath;
    using namespace std;

    //----------------------------------------------------------------------

    IOrgbe::IOrgbe()
        : FrameBufferIO()
    {
        unsigned int cap = ImageWrite | ImageRead | BruteForceIO;

        addType("rgbe", "Radiance HDR RGB + Exponent Image", cap);
        addType("hdr", "Radiance HDR RGB + Exponent Image", cap);
    }

    IOrgbe::~IOrgbe() {}

    string IOrgbe::about() const { return "RGBE (Radiance)"; }

    void IOrgbe::getImageInfo(const std::string& filename, FBInfo& fbi) const
    {
        FILE* infile = TwkUtil::fopen(filename.c_str(), "r");

        if (!infile)
        {
            TWK_THROW_STREAM(TwkFB::IOException,
                             "RGBE I/O: can't get info about " << filename);
        }

        int width;
        int height;

        if (RGBE_ReadHeader(infile, &width, &height, 0, true)
            == RGBE_RETURN_SUCCESS)
        {
            fbi.numChannels = 3;
            fbi.dataType = FrameBuffer::FLOAT;
            fbi.width = width;
            fbi.height = height;
            fbi.orientation = FrameBuffer::TOPLEFT;
        }
        else
        {
            TWK_THROW_STREAM(IOException,
                             "RGBE I/O: can't get info about " << filename);
        }
    }

    void IOrgbe::readImage(FrameBuffer& fb, const std::string& filename,
                           const ReadRequest& request) const
    {
        FILE* infile = TwkUtil::fopen(filename.c_str(), "rb");

        if (!infile)
        {
            TWK_THROW_STREAM(TwkFB::IOException,
                             "RGBE I/O: can't get info about " << filename);
        }

        int w;
        int h;

        if (RGBE_ReadHeader(infile, &w, &h, 0) == RGBE_RETURN_SUCCESS)
        {
            fb.restructure(w, h, 0, 3, FrameBuffer::FLOAT);

            if (RGBE_ReadPixels_RLE(infile, fb.pixels<float>(), w, h)
                != RGBE_RETURN_SUCCESS)
            {
                fclose(infile);
                TWK_THROW_STREAM(IOException,
                                 "RGBE I/O: can't get pixel data for "
                                     << filename);
            }

            fb.setPrimaryColorSpace(
                ColorSpace::Rec709()); // assume rec709 primaries
            fb.setTransferFunction(ColorSpace::Linear());
            fb.setOrientation(FrameBuffer::TOPLEFT);
        }
        else
        {
            fclose(infile);
            TWK_THROW_STREAM(IOException,
                             "RGBE I/O: can't get info about " << filename);
        }

        fclose(infile);
    }

    void IOrgbe::writeImage(const FrameBuffer& img, const std::string& filename,
                            const WriteRequest& request) const
    {
        FrameBuffer* outfb = const_cast<FrameBuffer*>(&img);

        //
        //  Convert to packed if not already.
        //

        if (img.isPlanar())
        {
            const FrameBuffer* fb = outfb;
            outfb = mergePlanes(outfb);
            if (fb != &img)
                delete fb;
        }

        if (img.numChannels() != 3)
        {
            const FrameBuffer* fb = outfb;
            vector<string> mapping;
            mapping.push_back("R");
            mapping.push_back("G");
            mapping.push_back("B");
            outfb = channelMap(const_cast<FrameBuffer*>(outfb), mapping);
            if (fb != &img)
                delete fb;
        }

        //
        //  Flip and Flop to get in the right orientation
        //

        bool needflip = false;
        bool needflop = false;

        switch (img.orientation())
        {
        case FrameBuffer::NATURAL:
            needflip = true;
            break;
        case FrameBuffer::TOPRIGHT:
        case FrameBuffer::BOTTOMRIGHT:
            needflop = true;
            break;
        default:
            break;
        };

        if (needflop)
        {
            if (outfb == &img)
                outfb = outfb->copy();
            flop(const_cast<FrameBuffer*>(outfb));
        }

        if (needflip)
        {
            if (outfb == &img)
                outfb = outfb->copy();
            flip(const_cast<FrameBuffer*>(outfb));
        }

        if (outfb->dataType() != FrameBuffer::FLOAT)
        {
            const FrameBuffer* fb = outfb;
            outfb = copyConvert(outfb, FrameBuffer::FLOAT);
            if (fb != &img)
                delete fb;
        }

        //
        //  Don't do the lin->log
        //

        FILE* outfile = TwkUtil::fopen(filename.c_str(), "wb");

        if (!outfile)
        {
            TWK_THROW_STREAM(TwkFB::IOException,
                             "RGBE I/O: can't write " << filename);
        }

        RGBE_WriteHeader(outfile, outfb->width(), outfb->height(), 0);

        RGBE_WritePixels_RLE(outfile, outfb->pixels<float>(), outfb->width(),
                             outfb->height());
        fclose(outfile);

        if (outfb != &img)
            delete outfb;
    }

} //  End namespace TwkFB
