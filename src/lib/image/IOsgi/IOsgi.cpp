//******************************************************************************
// Copyright (c) 2001-2004 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <IOsgi/IOsgi.h>
#include <TwkFB/Exception.h>
#include <TwkUtil/ByteSwap.h>
#include <TwkUtil/File.h>
#include <fstream>
#include <iostream>
#include <string>

namespace TwkFB
{
    using namespace std;
    using namespace TwkUtil;

    void IOsgi::Header::swap()
    {
        swapShorts(&magic, 1);
        swapShorts(&dimension, 4);
        swapWords(&minChannelVal, 2);
        swapWords(&colormapID, 1);
    }

    void IOsgi::Header::valid()
    {
        if ((storage != 1 && storage != 0) || (magic != Cigam && magic != Magic)
            || (bytesPerChannel != 1 && bytesPerChannel != 2)
            || (dimension < 1 || dimension > 3) || (colormapID != 0))
        {
            TWK_THROW_STREAM(IOException, "SGI header has non-standard values");
        }
    }

    static void planeNames(int nc, FrameBuffer::StringVector& planes)
    {
        const char* ch = 0;

        if (nc > 31)
            TWK_THROW_STREAM(IOException, "Too many channels in SGI file");

        switch (nc)
        {
        case 1:
            ch = "Y";
            break;
        case 2:
            ch = "YA";
            break;
        case 3:
            ch = "RGB";
            break;
        case 4:
            ch = "RGBA";
            break;
        default:
            ch = "RGBAZabcdefghikjlmnopqrstuvwxyz";
            break;
        }

        for (const char* p = ch; p && *p; p++)
        {
            char temp[2];
            temp[1] = 0;
            temp[0] = *p;
            planes.push_back(temp);
        }
    }

    IOsgi::IOsgi()
        : FrameBufferIO("IOsgi", "m99")
    {
        unsigned int cap = ImageRead | BruteForceIO;

        addType("sgi", "Silicon Graphics RGB Image", cap);
        addType("bw", "Silicon Graphics Monochrome Image", cap);
        addType("rgb", "Silicon Graphics RGB Image", cap);
        addType("rgba", "Silicon Graphics RGBA Image", cap);
    }

    IOsgi::~IOsgi() {}

    string IOsgi::about() const { return "SGI (Tweak)"; }

    void IOsgi::getImageInfo(const std::string& filename, FBInfo& fbi) const
    {
        ifstream infile(UNICODE_C_STR(filename.c_str()), ios::binary | ios::in);

        if (!infile)
        {
            TWK_THROW_STREAM(IOException, "Unable to open SGI file \""
                                              << filename << "\" for reading");
        }

        Header header;
        infile.read((char*)&header, sizeof(Header));

        if (header.magic == Magic || header.magic == Cigam)
        {
            if (header.magic == Cigam)
                header.swap();
            header.valid();

            //
            //  SGI is always planar on disk, so we'll just make it planar
            //  in memory to avoid a bunch of copies.
            //

            fbi.width = header.width;
            fbi.height = header.height;
            fbi.numChannels = header.numChannels;
            fbi.orientation = FrameBuffer::NATURAL;
            fbi.dataType = header.bytesPerChannel == 1 ? FrameBuffer::UCHAR
                                                       : FrameBuffer::USHORT;
        }
        else
        {
            TWK_THROW_STREAM(IOException, "Bad magic number in SGI file \""
                                              << filename << "\"");
        }
    }

    void IOsgi::readVerbatim(ifstream& infile, const Header& header,
                             FrameBuffer& fb, bool swap) const
    {
        //
        //  This is the easy case: just scarf each plane into its FB
        //

        for (FrameBuffer* p = fb.firstPlane(); p; p = p->nextPlane())
        {
            infile.read(p->scanline<char>(0), p->planeSize());

            if (infile.fail() && infile.eof())
            {
                fb.newAttribute("PartialImage", 1.0);
                break;
            }

            if (swap && p->dataType() == FrameBuffer::USHORT)
            {
                swapShorts(p->scanline<short>(0), p->width() * p->height());
            }
        }
    }

    static void expandBytes(unsigned char* optr, unsigned char* iptr,
                            size_t expected)
    {
        unsigned char pixel = 0, count = 0;
        size_t total = 0;

        while (1)
        {
            pixel = *iptr++;

            if (!(count = (pixel & 0x7f)))
                return;

            if (pixel & 0x80)
            {
                while (count--)
                {
                    if (total >= expected)
                    {
                        TWK_THROW_STREAM(IOException, "bad scanline");
                    }

                    *optr++ = *iptr++;
                    total++;
                }
            }
            else
            {
                pixel = *iptr++;

                while (count--)
                {
                    if (total >= expected)
                    {
                        TWK_THROW_STREAM(IOException, "bad scanline");
                    }

                    *optr++ = pixel;
                    total++;
                }
            }
        }
    }

    static void expandShorts(unsigned short* optr, unsigned short* iptr,
                             size_t expected)
    {
        unsigned short pixel = 0, count = 0;
        size_t total = 0;

        while (1)
        {
            pixel = *iptr++;

            if (!(count = (pixel & 0x7f)))
                return;

            if (pixel & 0x80)
            {
                while (count--)
                {
                    if (total >= expected)
                    {
                        TWK_THROW_STREAM(IOException, "bad scanline");
                    }

                    *optr++ = *iptr++;
                    total++;
                }
            }
            else
            {
                pixel = *iptr++;

                while (count--)
                {
                    if (total >= expected)
                    {
                        TWK_THROW_STREAM(IOException, "bad scanline");
                    }

                    *optr++ = pixel;
                    total++;
                }
            }
        }
    }

    void IOsgi::readRLE(ifstream& infile, const Header& header, FrameBuffer& fb,
                        bool swap) const
    {
        const size_t numPlanes = fb.numPlanes();
        const size_t numEntries = numPlanes * header.height;

        //
        //  These are the RLE start and length tables
        //

        vector<int32> offsets(numEntries);
        vector<int32> lengths(numEntries);
        const size_t bytes = numEntries * sizeof(int32);

        infile.read((char*)&offsets.front(), bytes);
        infile.read((char*)&lengths.front(), bytes);

        if (infile.fail() || infile.eof())
        {
            TWK_THROW_STREAM(IOException, "IOsgi: premature eof or failure");
        }

        if (swap)
        {
            swapWords(&offsets.front(), offsets.size());
            swapWords(&lengths.front(), lengths.size());
        }

        int32 maxsize = *max_element(lengths.begin(), lengths.end());
        int32 minsize = *min_element(lengths.begin(), lengths.end());

        if (minsize <= 0 || maxsize <= 0)
        {
            TWK_THROW_STREAM(IOException, "IOsgi: bad RLE table");
        }

        vector<unsigned char> scanbuffer(maxsize);
        unsigned char* bufp = &scanbuffer.front();
        size_t index = 0;

        for (FrameBuffer* p = fb.firstPlane(); p;
             p = p->nextPlane(), index += numEntries / fb.numPlanes())
        {
            const int32* otable = &offsets.front() + index;
            const int32* ltable = &lengths.front() + index;

            for (size_t y = 0; y < header.height; y++)
            {
                const int32 offset = otable[y];
                const int32 len = ltable[y]; // is this in bytes? Unclear!

                infile.seekg(offset);
                infile.read((char*)bufp, len);

                if (infile.fail() || infile.eof())
                {
                    TWK_THROW_STREAM(IOException,
                                     "IOsgi: premature eof or failure");
                }

                if (header.bytesPerChannel == 1)
                {
                    expandBytes(p->scanline<unsigned char>(y), bufp,
                                header.width);
                }
                else
                {
                    if (swap)
                        swapShorts(bufp, len / 2);

                    expandShorts(p->scanline<unsigned short>(y),
                                 (unsigned short*)bufp, header.width);
                }
            }
        }
    }

    void IOsgi::readImage(FrameBuffer& fb, const std::string& filename,
                          const ReadRequest& request) const
    {
        ifstream infile(UNICODE_C_STR(filename.c_str()), ios::binary);

        if (!infile)
        {
            TWK_THROW_STREAM(IOException, "Unable to open SGI file \""
                                              << filename << "\" for reading");
        }

        Header header;
        infile.read((char*)&header, sizeof(Header));

        if (header.magic == Magic || header.magic == Cigam)
        {
            const bool swap = header.magic == Cigam;
            if (swap)
                header.swap();
            header.valid();

            const bool rle = header.storage == 1;
            const int w = header.width;
            const int h = header.height;
            const int nplanes = header.numChannels;
            const int nc = 1;

            FrameBuffer::DataType dataType = header.bytesPerChannel == 1
                                                 ? FrameBuffer::UCHAR
                                                 : FrameBuffer::USHORT;

            FrameBuffer::StringVector names;
            planeNames(nplanes, names);

            //
            //  SGI is always planar on disk, so we'll just make it planar
            //  in memory to avoid a bunch of copies.
            //

            fb.restructurePlanar(w, h, names, dataType, FrameBuffer::NATURAL);

            if (rle)
                readRLE(infile, header, fb, swap);
            else
                readVerbatim(infile, header, fb, swap);

            //
            //  Assume Rec709 primaries + linear
            //

            fb.setPrimaryColorSpace(ColorSpace::Rec709());
            fb.setTransferFunction(ColorSpace::Linear());
        }
        else
        {
            TWK_THROW_STREAM(IOException, "Bad magic number in SGI file \""
                                              << filename << "\"");
        }
    }

    void IOsgi::writeImage(const FrameBuffer& img, const std::string& filename,
                           const WriteRequest& request) const
    {
        FrameBufferIO::writeImage(img, filename, request);
    }

} //  End namespace TwkFB
