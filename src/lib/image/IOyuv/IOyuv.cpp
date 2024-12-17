//******************************************************************************
// Copyright (c) 2008 Tippett Studio. All rights reserved.
//******************************************************************************

#include <IOyuv/IOyuv.h>
#include <TwkFB/Exception.h>
#include <TwkFB/Operations.h>
#include <TwkUtil/ByteSwap.h>
#include <TwkUtil/File.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#ifndef PLATFORM_WINDOWS
#include <unistd.h>
#endif

namespace TwkFB
{
    using namespace std;

    IOyuv::IOyuv()
        : FrameBufferIO("IOyuv", "mx")
    {
        //
        //  Indicate which extensions this plugin will handle The
        //  comparison against. The extensions are case-insensitive so
        //  there's no reason to provide upper case versions.
        //

        unsigned int cap = ImageRead | BruteForceIO;
        addType("yuv", "YCbCr raw image file", cap);

        //
        //  NOTE: if we had different compressor types the addType calls
        //  might look like this:
        //
        //  addType("imgext", "IMG image file", cap, "RLE", "ZIP", "LZW", NULL);
        //
        //  Where "RLE", ... "LZW" are all compressor types for this image
        //  file format. RVIO and other programs will use this information
        //  supplied by the user.
        //
    }

    IOyuv::~IOyuv() {}

    string IOyuv::about() const { return "Raw 4:2:2 HD/SD YUV"; }

    static void imageDimensions(size_t filesize, int& w, int& h)
    {
        switch (filesize)
        {
        case 640 * 486 * 2:
            w = 640;
            h = 486;
            break;
        case 720 * 480 * 2:
            w = 720;
            h = 480;
            break;
        case 720 * 486 * 2:
            w = 720;
            h = 486;
            break;
        case 720 * 512 * 2:
            w = 720;
            h = 512;
            break;
        case 1280 * 720 * 2:
            w = 1280;
            h = 720;
            break;
        case 1920 * 1080 * 2:
            w = 1920;
            h = 1080;
            break;

        default:
            TWK_THROW_STREAM(IOException,
                             "YUV: Unsupported file size: " << filesize);
        }
    }

    void IOyuv::getImageInfo(const std::string& filename, FBInfo& fbi) const
    {
        struct stat statbuf;

        if (stat(filename.c_str(), &statbuf))
        {
            TWK_THROW_STREAM(IOException, "YUV: can't open file: " << filename);
        }

        const size_t filesize = statbuf.st_size;

        imageDimensions(filesize, fbi.width, fbi.height);

        fbi.dataType = FrameBuffer::UCHAR;
        fbi.numChannels = 3;
        fbi.orientation = FrameBuffer::TOPLEFT;
    }

    void IOyuv::readImage(FrameBuffer& fb, const std::string& filename,
                          const ReadRequest& request) const
    {
        struct stat statbuf;

        if (stat(filename.c_str(), &statbuf))
        {
            TWK_THROW_STREAM(IOException, "YUV: can't open file: " << filename);
        }

        const size_t filesize = statbuf.st_size;
        int width = 0;
        int height = 0;

        imageDimensions(filesize, width, height);

        FrameBuffer::StringVector channels(3);
        channels[0] = "Y";
        channels[1] = "U";
        channels[2] = "V";

        FrameBuffer::Samplings xSamplings(3);
        xSamplings[0] = 1;
        xSamplings[1] = 2;
        xSamplings[2] = 2;

        FrameBuffer::Samplings ySamplings(3);
        ySamplings[0] = 1;
        ySamplings[1] = 1;
        ySamplings[2] = 1;

        fb.restructurePlanar(width, height, xSamplings, ySamplings, channels,
                             FrameBuffer::UCHAR, FrameBuffer::TOPLEFT);

        fb.setPrimaryColorSpace(ColorSpace::Rec601());
        fb.setConversion(ColorSpace::Rec601());

        FrameBuffer* Yplane = fb.firstPlane();
        FrameBuffer* Uplane = Yplane->nextPlane();
        FrameBuffer* Vplane = Uplane->nextPlane();

        FILE* yuvfile = TwkUtil::fopen(filename.c_str(), "rb");

        vector<unsigned char> line(width * 2);

        for (int y = 0; y < height; ++y)
        {
            int r = fread(&line.front(), width, 2, yuvfile);

            for (int x = 0; x < width; ++x)
            {
                const int offset = x * 2;

                if (x % 2)
                {
                    Vplane->pixel<unsigned char>(x / 2, y) = line[offset];
                }
                else
                {
                    Uplane->pixel<unsigned char>(x / 2, y) = line[offset];
                }

                Yplane->pixel<unsigned char>(x, y) = line[offset + 1];
            }
        }
    }

} //  End namespace TwkFB
