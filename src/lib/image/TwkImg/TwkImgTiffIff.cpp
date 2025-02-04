//******************************************************************************
// Copyright (c) 2001 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkImg/TwkImgTiffIff.h>
#include <TwkImg/TwkImgIffExc.h>
#include <TwkIos/TwkIosIostream.h>
#include <TwkIos/TwkIosFstream.h>
#include <TwkUtil/ByteSwap.h>
#include <TwkMath/Color.h>
#include <TwkMath/Function.h>

#include <math.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <string>
#include <tiffio.h>

#ifdef _MSC_VER
#include <winsock.h>
#else
#include <unistd.h>
#endif

using TwkMath::Col3f;
using TwkMath::Col3uc;
using TwkMath::Col3us;
using TwkMath::Col4f;
using TwkMath::Col4uc;
using TwkMath::Col4us;
using TwkMath::Mat44f;

//******************************************************************************
// Binary input flags

#ifndef TWK_NO_STD_IOS_BINARY_FLAG

#define BINARY_INPUT_FLAGS (STD_IOS::in | STD_IOS::binary)
#define BINARY_OUTPUT_FLAGS (STD_IOS::out | STD_IOS::binary)

#else

#define BINARY_INPUT_FLAGS (STD_IOS::in)
#define BINARY_OUTPUT_FLAGS (STD_IOS::out)

#endif

//******************************************************************************
namespace TwkImg
{
    using namespace std;

    //*****************************************************************************
    Img4f* TiffIff::read(const char* imgFileName, bool showWarnings)
    {
        if (!showWarnings)
        {
            // Suppress annoying messages about unknown tags, etc...
            TIFFSetErrorHandler(0);
            TIFFSetWarningHandler(0);
        }

        TIFF* tif = TIFFOpen(imgFileName, "r");
        if (tif == NULL)
        {
            IffExc exc("Could not open specified image file for reading");
            throw(exc);
        }

        int width;
        int height;
        uint16 bitsPerSample;
        uint16* sampleinfo;
        uint16 extrasamples;

        TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
        TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
        TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitsPerSample);

        TIFFGetFieldDefaulted(tif, TIFFTAG_EXTRASAMPLES, &extrasamples,
                              &sampleinfo);

        bool hasAlpha = false;
        if (extrasamples == 1)
        {
            hasAlpha = true;
        }

        Img4f* ret = new Img4f(width, height);
        tdata_t buf = _TIFFmalloc(TIFFScanlineSize(tif));

        switch (bitsPerSample)
        {
        case 32:
            for (int y = 0; y < height; ++y)
            {
                if (hasAlpha)
                {
                    TIFFReadScanline(tif, &(ret->pixel(0, height - y - 1)), y);
                }
                else
                {
                    TIFFReadScanline(tif, buf, y);
                    Col4f* dstPixel = (*ret)[height - y - 1];
                    Col3f* srcPixel = (Col3f*)buf;
                    for (int x = 0; x < width; ++x, ++dstPixel, ++srcPixel)
                    {
                        (*dstPixel).x = srcPixel->x;
                        (*dstPixel).y = srcPixel->y;
                        (*dstPixel).z = srcPixel->z;
                        (*dstPixel).w = 1.0f;
                    }
                }
            }
            break;
        case 16:
            for (int y = 0; y < height; ++y)
            {
                TIFFReadScanline(tif, buf, y);

                Col4f* dstPixel = (*ret)[height - y - 1];
                if (hasAlpha)
                {
                    Col4us* srcPixel = (Col4us*)buf;
                    for (int x = 0; x < width; ++x, ++dstPixel, ++srcPixel)
                    {
                        (*dstPixel).x = (float)srcPixel->x / 65535.0f;
                        (*dstPixel).y = (float)srcPixel->y / 65535.0f;
                        (*dstPixel).z = (float)srcPixel->z / 65535.0f;
                        (*dstPixel).w = (float)srcPixel->w / 65535.0f;
                    }
                }
                else
                {
                    Col3us* srcPixel = (Col3us*)buf;
                    for (int x = 0; x < width; ++x, ++dstPixel, ++srcPixel)
                    {
                        (*dstPixel).x = (float)srcPixel->x / 65535.0f;
                        (*dstPixel).y = (float)srcPixel->y / 65535.0f;
                        (*dstPixel).z = (float)srcPixel->z / 65535.0f;
                        (*dstPixel).w = 1.0f;
                    }
                }
            }
            break;
        case 8:
            for (int y = 0; y < height; ++y)
            {
                TIFFReadScanline(tif, buf, y);

                Col4f* dstPixel = (*ret)[height - y - 1];

                if (hasAlpha)
                {
                    Col4uc* srcPixel = (Col4uc*)buf;
                    for (int x = 0; x < width; ++x, ++dstPixel, ++srcPixel)
                    {
                        (*dstPixel).x = (float)srcPixel->x / 255.0f;
                        (*dstPixel).y = (float)srcPixel->y / 255.0f;
                        (*dstPixel).z = (float)srcPixel->z / 255.0f;
                        (*dstPixel).w = (float)srcPixel->w / 255.0f;
                    }
                }
                else
                {
                    Col3uc* srcPixel = (Col3uc*)buf;
                    for (int x = 0; x < width; ++x, ++dstPixel, ++srcPixel)
                    {
                        (*dstPixel).x = (float)srcPixel->x / 255.0f;
                        (*dstPixel).y = (float)srcPixel->y / 255.0f;
                        (*dstPixel).z = (float)srcPixel->z / 255.0f;
                        (*dstPixel).w = 1.0f;
                    }
                }
            }
            break;
        default:
            IffExc exc("Sorry, unsupported TIFF data type.");
            throw(exc);
        } // End switch( bitsPerSample )

        _TIFFfree(buf);

        TIFFClose(tif);

        return ret;
    }

    //*****************************************************************************
    Img4uc* TiffIff::read4uc(const char* imgFileName, bool showWarnings)
    {
        if (!showWarnings)
        {
            // Suppress annoying messages about unknown tags, etc...
            TIFFSetErrorHandler(0);
            TIFFSetWarningHandler(0);
        }

        TIFF* tif = TIFFOpen(imgFileName, "r");
        if (tif == NULL)
        {
            IffExc exc("Could not open specified image file for reading");
            throw(exc);
        }

        int width;
        int height;
        uint16 bitsPerSample;
        uint16* sampleinfo;
        uint16 extrasamples;

        TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
        TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
        TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitsPerSample);

        TIFFGetFieldDefaulted(tif, TIFFTAG_EXTRASAMPLES, &extrasamples,
                              &sampleinfo);

        bool hasAlpha = false;
        if (extrasamples == 1)
        {
            hasAlpha = true;
        }

        Img4uc* ret = new Img4uc(width, height);
        tdata_t buf = _TIFFmalloc(TIFFScanlineSize(tif));

        switch (bitsPerSample)
        {
        case 32:
            for (int y = 0; y < height; ++y)
            {
                TIFFReadScanline(tif, buf, y);

                Col4uc* dstPixel = (*ret)[height - y - 1];
                if (hasAlpha)
                {
                    Col4f* srcPixel = (Col4f*)buf;
                    (*dstPixel).x = (unsigned char)(TwkMath::clamp(
                        (*srcPixel).x * 255.0f, 0.0f, 255.0f));
                    (*dstPixel).y = (unsigned char)(TwkMath::clamp(
                        (*srcPixel).y * 255.0f, 0.0f, 255.0f));
                    (*dstPixel).z = (unsigned char)(TwkMath::clamp(
                        (*srcPixel).z * 255.0f, 0.0f, 255.0f));
                    (*dstPixel).w = (unsigned char)(TwkMath::clamp(
                        (*srcPixel).w * 255.0f, 0.0f, 255.0f));
                }
                else
                {
                    Col3f* srcPixel = (Col3f*)buf;
                    (*dstPixel).x = (unsigned char)(TwkMath::clamp(
                        (*srcPixel).x * 255.0f, 0.0f, 255.0f));
                    (*dstPixel).y = (unsigned char)(TwkMath::clamp(
                        (*srcPixel).y * 255.0f, 0.0f, 255.0f));
                    (*dstPixel).z = (unsigned char)(TwkMath::clamp(
                        (*srcPixel).z * 255.0f, 0.0f, 255.0f));
                    (*dstPixel).w = 255;
                }
            }
            break;
        case 16:
            for (int y = 0; y < height; ++y)
            {
                TIFFReadScanline(tif, buf, y);

                Col4uc* dstPixel = (*ret)[height - y - 1];
                if (hasAlpha)
                {
                    Col4us* srcPixel = (Col4us*)buf;
                    for (int x = 0; x < width; ++x, ++dstPixel, ++srcPixel)
                    {
                        (*dstPixel).x = (unsigned char)(TwkMath::clamp(
                            (*srcPixel).x / 255.0f, 0.0f, 255.0f));
                        (*dstPixel).y = (unsigned char)(TwkMath::clamp(
                            (*srcPixel).y / 255.0f, 0.0f, 255.0f));
                        (*dstPixel).z = (unsigned char)(TwkMath::clamp(
                            (*srcPixel).z / 255.0f, 0.0f, 255.0f));
                        (*dstPixel).w = (unsigned char)(TwkMath::clamp(
                            (*srcPixel).w / 255.0f, 0.0f, 255.0f));
                    }
                }
                else
                {
                    Col3us* srcPixel = (Col3us*)buf;
                    for (int x = 0; x < width; ++x, ++dstPixel, ++srcPixel)
                    {
                        (*dstPixel).x = (unsigned char)(TwkMath::clamp(
                            (*srcPixel).x / 255.0f, 0.0f, 255.0f));
                        (*dstPixel).y = (unsigned char)(TwkMath::clamp(
                            (*srcPixel).y / 255.0f, 0.0f, 255.0f));
                        (*dstPixel).z = (unsigned char)(TwkMath::clamp(
                            (*srcPixel).z / 255.0f, 0.0f, 255.0f));
                        (*dstPixel).w = 255;
                    }
                }
            }
            break;
        case 8:
            for (int y = 0; y < height; ++y)
            {
                TIFFReadScanline(tif, buf, y);

                Col4uc* dstPixel = (*ret)[height - y - 1];

                if (hasAlpha)
                {
                    TIFFReadScanline(tif, &(ret->pixel(0, height - y - 1)), y);
                }
                else
                {
                    Col3uc* srcPixel = (Col3uc*)buf;
                    for (int x = 0; x < width; ++x, ++dstPixel, ++srcPixel)
                    {
                        (*dstPixel).x = srcPixel->x;
                        (*dstPixel).y = srcPixel->y;
                        (*dstPixel).z = srcPixel->z;
                        (*dstPixel).w = 255;
                    }
                }
            }
            break;
        default:
            IffExc exc("Sorry, unsupported TIFF data type.");
            throw(exc);
        } // End switch( bitsPerSample )

        _TIFFfree(buf);

        TIFFClose(tif);

        return ret;
    }

//******************************************************************************
// The tags.
#define TIFFTAG_CAMERA_MATRIX 33306
#define TIFFTAG_SCREEN_MATRIX 33305

    //*****************************************************************************
    static bool TiffIff_write_f(const Img4f* img, const char* fileName,
                                const TwkMath::Mat44f* Mcamera,
                                const TwkMath::Mat44f* Mscreen, const int depth)
    {
        if (depth != 8 && depth != 16 && depth != 32)
        {
            string errmsg = "Unable to write bit depth: " + depth;
            IffExc exc(errmsg.c_str());
            throw(exc);
        }

        TIFF* tif = TIFFOpen(fileName, "w");
        if (tif == NULL)
        {
            IffExc exc("Could not open specified image file for writing");
            throw(exc);
        }

        TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, img->width());
        TIFFSetField(tif, TIFFTAG_IMAGELENGTH, img->height());
        TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 4);
        TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, depth);
        TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
        TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
        TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
        TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_PACKBITS);

        if (Mcamera != NULL)
        {
            TIFFSetField(tif, TIFFTAG_CAMERA_MATRIX, (uint16)16,
                         (const float*)Mcamera);
        }
        if (Mscreen != NULL)
        {
            TIFFSetField(tif, TIFFTAG_SCREEN_MATRIX, (uint16)16,
                         (const float*)Mscreen);
        }

        char hostname[64];
        gethostname(hostname, 64);
        TIFFSetField(tif, TIFFTAG_HOSTCOMPUTER, hostname);

        switch (depth)
        {
        case 32:
            TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
            break;
        case 16:
            TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
            break;
        case 8:
            TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
            break;
        default:
            break;
        }
        uint16 extrasamples = 1;
        uint16 sampleinfo[1] = {EXTRASAMPLE_ASSOCALPHA};
        TIFFSetField(tif, TIFFTAG_EXTRASAMPLES, extrasamples, sampleinfo);

        tdata_t buf = _TIFFmalloc(TIFFScanlineSize(tif));

        switch (depth)
        {
        case 8:
            for (int y = 0; y < img->height(); ++y)
            {
                Col4uc* dstPixel = (Col4uc*)buf;
                const Col4f* srcPixel = (*img)[img->height() - y - 1];

                for (int x = 0; x < img->width(); ++x, dstPixel++, ++srcPixel)
                {
                    (*dstPixel).x = (unsigned char)(TwkMath::clamp(
                        (*srcPixel).x * 255.0f, 0.0f, 255.0f));
                    (*dstPixel).y = (unsigned char)(TwkMath::clamp(
                        (*srcPixel).y * 255.0f, 0.0f, 255.0f));
                    (*dstPixel).z = (unsigned char)(TwkMath::clamp(
                        (*srcPixel).z * 255.0f, 0.0f, 255.0f));
                    (*dstPixel).w = (unsigned char)(TwkMath::clamp(
                        (*srcPixel).w * 255.0f, 0.0f, 255.0f));
                }
                if (TIFFWriteScanline(tif, buf, y, 0) == -1)
                {
                    IffExc exc("Error while writing TIFF scanline");
                    throw(exc);
                }
            }
            break;
        case 16:
            for (int y = 0; y < img->height(); ++y)
            {
                Col4us* dstPixel = (Col4us*)buf;
                const Col4f* srcPixel = (*img)[img->height() - y - 1];

                for (int x = 0; x < img->width(); ++x, dstPixel++, ++srcPixel)
                {
                    (*dstPixel).x = (unsigned short)(TwkMath::clamp(
                        (*srcPixel).x * 65535.0f, 0.0f, 65535.0f));
                    (*dstPixel).y = (unsigned short)(TwkMath::clamp(
                        (*srcPixel).y * 65535.0f, 0.0f, 65535.0f));
                    (*dstPixel).z = (unsigned short)(TwkMath::clamp(
                        (*srcPixel).z * 65535.0f, 0.0f, 65535.0f));
                    (*dstPixel).w = (unsigned short)(TwkMath::clamp(
                        (*srcPixel).w * 65535.0f, 0.0f, 65535.0f));
                }
                if (TIFFWriteScanline(tif, buf, y, 0) == -1)
                {
                    IffExc exc("Error while writing TIFF scanline");
                    throw(exc);
                }
            }
            break;
        case 32:
            for (int y = 0; y < img->height(); ++y)
            {
                Col4f* srcPixel = (Col4f*)(*img)[img->height() - y - 1];
                if (TIFFWriteScanline(tif, srcPixel, y, 0) == -1)
                {
                    IffExc exc("Error while writing TIFF scanline");
                    throw(exc);
                }
            }
            break;
        }

        _TIFFfree(buf);

        TIFFClose(tif);

        return true;
    }

    // *****************************************************************************
    //*****************************************************************************
    static bool TiffIff_write_uc(const Img4uc* img, const char* fileName,
                                 const Mat44f* Mcamera, const Mat44f* Mscreen)
    {
        TIFF* tif = TIFFOpen(fileName, "w");
        if (tif == NULL)
        {
            IffExc exc("Could not open specified image file for writing");
            throw(exc);
        }

        TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, img->width());
        TIFFSetField(tif, TIFFTAG_IMAGELENGTH, img->height());
        TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 4);
        TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
        TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
        TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
        TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
        TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_PACKBITS);
        TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);

        if (Mcamera != NULL)
        {
            TIFFSetField(tif, TIFFTAG_CAMERA_MATRIX, (uint16)16,
                         (const float*)Mcamera);
        }
        if (Mscreen != NULL)
        {
            TIFFSetField(tif, TIFFTAG_SCREEN_MATRIX, (uint16)16,
                         (const float*)Mscreen);
        }

        char hostname[64];
        gethostname(hostname, 64);
        TIFFSetField(tif, TIFFTAG_HOSTCOMPUTER, hostname);

        uint16 extrasamples = 1;
        uint16 sampleinfo[1] = {EXTRASAMPLE_ASSOCALPHA};
        TIFFSetField(tif, TIFFTAG_EXTRASAMPLES, extrasamples, sampleinfo);

        tdata_t buf = _TIFFmalloc(TIFFScanlineSize(tif));

        for (int y = 0; y < img->height(); ++y)
        {
            Col4uc* srcPixel = (Col4uc*)(*img)[img->height() - y - 1];
            if (TIFFWriteScanline(tif, srcPixel, y, 0) == -1)
            {
                IffExc exc("Error while writing TIFF scanline");
                throw(exc);
            }
        }

        _TIFFfree(buf);

        TIFFClose(tif);

        return true;
    }

    // *****************************************************************************
    //*****************************************************************************
    static bool TiffIff_write_us(const Img4us* img, const char* fileName,
                                 const Mat44f* Mcamera, const Mat44f* Mscreen)
    {
        TIFF* tif = TIFFOpen(fileName, "w");
        if (tif == NULL)
        {
            IffExc exc("Could not open specified image file for writing");
            throw(exc);
        }

        TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, img->width());
        TIFFSetField(tif, TIFFTAG_IMAGELENGTH, img->height());
        TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 4);
        TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 16);
        TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
        TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
        TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
        TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_PACKBITS);
        TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);

        if (Mcamera != NULL)
        {
            TIFFSetField(tif, TIFFTAG_CAMERA_MATRIX, (uint16)16,
                         (const float*)Mcamera);
        }
        if (Mscreen != NULL)
        {
            TIFFSetField(tif, TIFFTAG_SCREEN_MATRIX, (uint16)16,
                         (const float*)Mscreen);
        }

        char hostname[64];
        gethostname(hostname, 64);
        TIFFSetField(tif, TIFFTAG_HOSTCOMPUTER, hostname);

        uint16 extrasamples = 1;
        uint16 sampleinfo[1] = {EXTRASAMPLE_ASSOCALPHA};
        TIFFSetField(tif, TIFFTAG_EXTRASAMPLES, extrasamples, sampleinfo);

        tdata_t buf = _TIFFmalloc(TIFFScanlineSize(tif));

        for (int y = 0; y < img->height(); ++y)
        {
            Col4us* srcPixel = (Col4us*)(*img)[img->height() - y - 1];
            if (TIFFWriteScanline(tif, srcPixel, y, 0) == -1)
            {
                IffExc exc("Error while writing TIFF scanline");
                throw(exc);
            }
        }

        _TIFFfree(buf);

        TIFFClose(tif);

        return true;
    }

    //******************************************************************************
    //******************************************************************************
    bool TiffIff::write(const Img4f* img, const char* fileName, const int depth)
    {
        return TiffIff_write_f(img, fileName, NULL, NULL, depth);
    }

    bool TiffIff::write(const Img4f* img, const char* fileName,
                        const TwkMath::Mat44f& Mcamera,
                        const TwkMath::Mat44f& Mscreen, const int depth)
    {
        return TiffIff_write_f(img, fileName, &Mcamera, &Mscreen, depth);
    }

    bool TiffIff::write(const Img4uc* img, const char* fileName)
    {
        return TiffIff_write_uc(img, fileName, NULL, NULL);
    }

    bool TiffIff::write(const Img4uc* img, const char* fileName,
                        const TwkMath::Mat44f& Mcamera,
                        const TwkMath::Mat44f& Mscreen)
    {
        return TiffIff_write_uc(img, fileName, &Mcamera, &Mscreen);
    }

    bool TiffIff::write(const Img4us* img, const char* fileName)
    {
        return TiffIff_write_us(img, fileName, NULL, NULL);
    }

    bool TiffIff::write(const Img4us* img, const char* fileName,
                        const TwkMath::Mat44f& Mcamera,
                        const TwkMath::Mat44f& Mscreen)
    {
        return TiffIff_write_us(img, fileName, &Mcamera, &Mscreen);
    }

} // End namespace TwkImg
