//******************************************************************************
// Copyright (c) 2001-2003 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <stdio.h>
#include <TwkImgRGBEIff.h>
#include <TwkUtil/File.h>

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

namespace TwkImg
{

    // *****************************************************************************
    static long swapLong(long foo)
    {
        unsigned char* buf = (unsigned char*)&foo;
        std::swap(buf[0], buf[3]);
        std::swap(buf[1], buf[2]);
        return *((long*)(buf));
    }

    // *****************************************************************************
    static unsigned short swapUShort(unsigned short foo)
    {
        unsigned char* buf = (unsigned char*)&foo;
        std::swap(buf[0], buf[1]);
        return *((unsigned short*)(buf));
    }

    // *****************************************************************************
    static void swapHeader(RgbeIff::SGI_HDR& header)
    {
        header.magic = swapUShort(header.magic);
        header.dimension = swapUShort(header.dimension);
        header.xsize = swapUShort(header.xsize);
        header.ysize = swapUShort(header.ysize);
        header.zsize = swapUShort(header.zsize);
        header.pixmin = swapLong(header.pixmin);
        header.pixmax = swapLong(header.pixmax);
        header.colormap = swapLong(header.colormap);
    }

    // *****************************************************************************
    static inline int expandrow(unsigned char* optr, unsigned char* iptr,
                                int offset, int step = 1)
    {
        unsigned char pixel;
        unsigned char count;
        int pixelsDecoded = 0;

        optr += offset;
        while (1)
        {
            pixel = *iptr++;
            if (!(count = (pixel & 0x7f)))
            {
                return pixelsDecoded;
            }
            if (pixel & 0x80)
            {
                while (count--)
                {
                    *optr = *iptr++;
                    optr += step;
                    ++pixelsDecoded;
                }
            }
            else
            {
                pixel = *iptr++;
                while (count--)
                {
                    *optr = pixel;
                    optr += step;
                    ++pixelsDecoded;
                }
            }
        }
    }

    // *****************************************************************************
    static Img4f* readRLE(FILE* f, RgbeIff::SGI_HDR& header, bool swapped)
    {
        vector<long> offsets(header.ysize * header.zsize);
        fread(offsets, header.ysize * header.zsize * sizeof(long), 1, f);

        long lengths[header.ysize * header.zsize];
        fread(lengths, header.ysize * header.zsize * sizeof(long), 1, f);

        unsigned char** imgBuf[header.zsize];
        for (int z = 0; z < header.zsize; ++z)
        {
            imgBuf[z] = new unsigned char*[header.ysize];

            for (int y = 0; y < header.ysize; ++y)
            {
                imgBuf[z][y] = new unsigned char[header.xsize];
            }
        }

        Img4f* outImg = new Img4f(header.xsize, header.ysize);

        int tabPos = 0;
        for (int z = 0; z < header.zsize; ++z)
        {
            for (int y = 0; y < header.ysize; ++y)
            {
                long buffLen = 0;
                long seekTo = 0;
                if (swapped)
                {
                    seekTo = swapLong(offsets[tabPos]);
                    buffLen = swapLong(lengths[tabPos]);
                }
                else
                {
                    seekTo = offsets[tabPos];
                    buffLen = lengths[tabPos];
                }

                if (fseek(f, seekTo, SEEK_SET) != 0)
                {
                    char msg[256];
                    snprintf(msg, 256,
                             "Error seeking to image data at "
                             "offset %d, channel %d, line %d",
                             seekTo, z, y);
                    TWK_EXC_THROW_WHAT(Exception, msg);
                }

                unsigned char* RLEbuffer = new unsigned char[buffLen];
                if (fread(RLEbuffer, 1, buffLen, f) != buffLen)
                {
                    char msg[256];
                    snprintf(msg, 256,
                             "Error reading image data at "
                             "channel %d, line %d",
                             z, y);
                    TWK_EXC_THROW_WHAT(Exception, msg);
                }

                int decoded = expandrow(imgBuf[z][y], RLEbuffer, 0, 1);
                if (decoded != header.xsize)
                {
                    char msg[256];
                    snprintf(msg, 256,
                             "Bad RLE data in image at "
                             "channel %d, line %d",
                             z, y);
                    TWK_EXC_THROW_WHAT(Exception, msg);
                }

                delete[] RLEbuffer;
                ++tabPos;
            }
        }

        for (int y = 0; y < header.ysize; ++y)
        {
            for (int x = 0; x < header.xsize; ++x)
            {
                float f = 0;
                if (imgBuf[3][y][x])
                {
                    f = ldexp(1.0, imgBuf[3][y][x] - (128 + 8));
                }

                outImg->pixel(x, y).x = imgBuf[0][y][x] * f;
                outImg->pixel(x, y).y = imgBuf[1][y][x] * f;
                outImg->pixel(x, y).z = imgBuf[2][y][x] * f;

                if (header.zsize == 5)
                {
                    outImg->pixel(x, y).w = imgBuf[4][y][x] / 255.0f;
                }
                else
                {
                    outImg->pixel(x, y).w = 1.0f;
                }
            }
        }

        for (int z = 0; z < header.zsize; ++z)
        {
            for (int y = 0; y < header.ysize; ++y)
            {
                delete[] imgBuf[z][y];
            }
            delete[] imgBuf[z];
        }

        return outImg;
    }

    // *****************************************************************************
    static Img4f* readUncompressed(FILE* f, RgbeIff::SGI_HDR& header)
    {
        unsigned char** imgBuf[header.zsize];
        for (int z = 0; z < header.zsize; ++z)
        {
            imgBuf[z] = new unsigned char*[header.ysize];

            for (int y = 0; y < header.ysize; ++y)
            {
                imgBuf[z][y] = new unsigned char[header.xsize];
            }
        }

        for (int z = 0; z < header.zsize; ++z)
        {
            for (int y = 0; y < header.ysize; ++y)
            {
                long buffLen = header.xsize;
                fread(imgBuf[z][y], buffLen, 1, f);
            }
        }

        Img4f* outImg = new Img4f(header.xsize, header.ysize);

        for (int y = 0; y < header.ysize; ++y)
        {
            for (int x = 0; x < header.xsize; ++x)
            {
                float f = 0;
                if (imgBuf[3][y][x])
                {
                    f = ldexp(1.0, imgBuf[3][y][x] - (128 + 8));
                }

                outImg->pixel(x, y).x = imgBuf[0][y][x] * f;
                outImg->pixel(x, y).y = imgBuf[1][y][x] * f;
                outImg->pixel(x, y).z = imgBuf[2][y][x] * f;

                if (header.zsize == 5)
                {
                    outImg->pixel(x, y).w = imgBuf[4][y][x] / 255.0f;
                }
                else
                {
                    outImg->pixel(x, y).w = 1.0f;
                }
            }
        }

        for (int z = 0; z < header.zsize; ++z)
        {
            for (int y = 0; y < header.ysize; ++y)
            {
                delete[] imgBuf[z][y];
            }
            delete[] imgBuf[z];
        }

        return outImg;
    }

    // *****************************************************************************
    TwkImg::Img4f* RgbeIff::read(const char* filename)
    {
        FILE* f = TwkUtil::fopen(filename, "rb");
        SGI_HDR header;
        memset(&header, 0, sizeof(header));

        fread(&header, sizeof(header), 1, f);

        bool swapped;
        if (header.magic == Magic)
        {
            // Native byte order
            swapped = false;
        }
        else if (header.magic == Cigam)
        {
            // Swapped byte order
            swapped = true;
            swapHeader(header);
        }
        else
        {
            char msg[256];
            snprintf(msg, 256, "Bad magic number: 0x%04X", header.magic);
            TWK_EXC_THROW_WHAT(Exception, msg);
        }

        Img4f* outImg = NULL;
        if (header.storage == 1)
        {
            outImg = readRLE(f, header, swapped);
        }
        else
        {
            outImg = readUncompressed(f, header);
        }

        fclose(f);

        return outImg;
    }

} //  End namespace TwkImg

#ifdef _MSC_VER
#undef snprintf
#endif
