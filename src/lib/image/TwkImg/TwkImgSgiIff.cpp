//******************************************************************************
// Copyright (c) 2001-2002 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkImg/TwkImgSgiIff.h>
#include <TwkUtil/ByteSwap.h>
#include <TwkUtil/File.h>
#include <algorithm>
#include <assert.h>
#include <string.h>
#include <fstream>
#include <iostream>

using namespace TwkUtil;
using TwkMath::Col4f;
using TwkMath::Col4uc;

#ifdef TWK_NO_SGI_BYTE_ORDER
#define BINARY_INPUT_FLAGS (std::ios_base::in | std::ios_base::binary)
#define BINARY_OUTPUT_FLAGS (std::ios_base::out | std::ios_base::binary)
#else
#define BINARY_INPUT_FLAGS (std::ios_base::in)
#define BINARY_OUTPUT_FLAGS (std::ios_base::out)
#endif

namespace TwkImg
{

    //******************************************************************************
    SgiIffBase::Header::Header() { memset((void*)this, 0, sizeof(Header)); }

    //******************************************************************************
    SgiIffBase::Header::Header(int W, int H, int NC)
    {
        assert(W > 0);
        assert(H > 0);
        assert(NC == 1 || NC == 3 || NC == 4);

        memset((void*)this, 0, sizeof(Header));

        magic = 474;
        storage = 0;
        bytesPerChannel = 1;
        dimension = 3;
        width = W;
        height = H;
        numChannels = NC;
        minChannelVal = 0;
        maxChannelVal = 255;
        sprintf(imageName, "Image name unsupported");
        colormapID = 0;
    }

    //******************************************************************************
    void SgiIffBase::Header::read(std::istream& stream)
    {
#ifdef TWK_NO_SGI_BYTE_ORDER
        magic = readSwappedShort(stream);
#else
        stream.read((char*)(&magic), sizeof(short));
#endif
        if (magic != 474)
        {
            throw(IffExc("Invalid magic number. Not an RGB file"));
        }

        stream.get(storage);
        if (storage != 0 && storage != 1)
        {
            throw(IffExc("Invalid storage type."));
        }

        stream.get(bytesPerChannel);
        // Uncomment the "2" to use USHORT images
        if (bytesPerChannel != 1 /*&& bytesPerChannel != 2*/)
        {
            throw(IffExc("Invalid number of bytes per channel"));
        }

#ifdef TWK_NO_SGI_BYTE_ORDER
        dimension = readSwappedUshort(stream);
#else
        stream.read((char*)(&dimension), sizeof(unsigned short));
#endif
        if (dimension != 3)
        {
            throw(IffExc("Unsupported image dimension"));
        }

#ifdef TWK_NO_SGI_BYTE_ORDER
        width = readSwappedUshort(stream);
        height = readSwappedUshort(stream);
#else
        stream.read((char*)(&width), sizeof(unsigned short));
        stream.read((char*)(&height), sizeof(unsigned short));
#endif

#ifdef TWK_NO_SGI_BYTE_ORDER
        numChannels = readSwappedUshort(stream);
#else
        stream.read((char*)(&numChannels), sizeof(unsigned short));
#endif
        if (numChannels != 1 && numChannels != 3 && numChannels != 4)
        {
            throw(IffExc("Unsupported number of channels"));
        }

#ifdef TWK_NO_SGI_BYTE_ORDER
        minChannelVal = readSwappedLong(stream);
#else
        stream.read((char*)(&minChannelVal), sizeof(long));
#endif

#if 0
    if ( minChannelVal != 0 )
    {
        throw( IffExc( "Unsupported minimum pixel value" ) );
    }
#endif

#ifdef TWK_NO_SGI_BYTE_ORDER
        maxChannelVal = readSwappedLong(stream);
#else
        stream.read((char*)(&maxChannelVal), sizeof(long));
#endif

#if 0
    if ( maxChannelVal != 255 )
    {
        throw( IffExc( "Unsupported maximum pixel value" ) );
    }
#endif

        stream.read((char*)(dummy1), 4);
        stream.read((char*)(imageName), 80);
#ifdef TWK_NO_SGI_BYTE_ORDER
        colormapID = readSwappedLong(stream);
#else
        stream.read((char*)(&colormapID), sizeof(long));
#endif
        if (colormapID != 0)
        {
            throw(IffExc("Unsupported colormap id"));
        }

        stream.read((char*)(dummy2), 404);

        if (!stream)
        {
            throw(IffExc("Corrupted input file stream"));
        }
    }

    //******************************************************************************
    void SgiIffBase::Header::write(std::ostream& stream)
    {
#ifdef TWK_NO_SGI_BYTE_ORDER
        writeSwappedShort(stream, magic);
        stream.put(storage);
        stream.put(bytesPerChannel);
        writeSwappedUshort(stream, dimension);
        writeSwappedUshort(stream, width);
        writeSwappedUshort(stream, height);
        writeSwappedUshort(stream, numChannels);
        writeSwappedLong(stream, minChannelVal);
        writeSwappedLong(stream, maxChannelVal);
        stream.write((char*)(dummy1), 4);
        stream.write((char*)(imageName), 80);
        writeSwappedLong(stream, colormapID);
        stream.write((char*)(dummy2), 404);
#else
        stream.write((char*)(this), sizeof(Header));
#endif

        if (!stream)
        {
            throw(IffExc("Corrupted output file stream"));
        }
    }

    //******************************************************************************
    void SgiIffBase::readPixelsRLE(Img4uc& img, SgiIffBase::Header& header,
                                   std::istream& stream)
    {
        // Scanlines in RGB images are separated by channels,
        // so that each channel is completely stored before the
        // next channel is begun.
        int tabLen = header.height * header.numChannels;
        assert(tabLen > 0);
        long* offsetTable = new long[tabLen];
        long* lengthTable = new long[tabLen];

        // Read offset table
        int i;
        for (i = 0; i < tabLen; i++)
        {
#ifdef TWK_NO_SGI_BYTE_ORDER
            offsetTable[i] = readSwappedLong(stream);
#else
            stream.read((char*)(&(offsetTable[i])), sizeof(long));
#endif
        }
        // Read length table
        for (i = 0; i < tabLen; i++)
        {
#ifdef TWK_NO_SGI_BYTE_ORDER
            lengthTable[i] = readSwappedLong(stream);
#else
            stream.read((char*)(&(lengthTable[i])), sizeof(long));
#endif
        }

        if (!stream)
        {
            delete[] offsetTable;
            delete[] lengthTable;
            throw(IffExc("Corrupted input file stream"));
        }

        // Create a row that can be used to read in scanlines.
        // The row needs to be big enough to hold the biggest possible
        // scanline
        int scanlineMax = (header.width * (header.bytesPerChannel + 1)) + 1;
        unsigned char* scanline = new unsigned char[scanlineMax];

        int tableIndex = 0;

        // Loop over channels
        for (int c = 0; c < header.numChannels; c++)
        {
            // Loop over rows
            for (int row = 0; row < header.height; row++, tableIndex++)
            {
                // Seek stream to beginning of row
                stream.seekg(offsetTable[tableIndex]);

                // Get scanline length
                long scanlineLength = lengthTable[tableIndex];
                assert(scanlineLength <= scanlineMax);

                // Read scanline into buffer
                stream.read((char*)scanline, scanlineLength);

                // Decompress scanline
                unsigned char* srcPtr = scanline;
                Col4uc* dstPtrPixel = img.begin() + (row * header.width);
                unsigned char* dstPtr = c + (unsigned char*)dstPtrPixel;
                int pixelsRead = 0;
                while (1)
                {
                    unsigned char nextByte = *srcPtr;
                    srcPtr++;
                    unsigned char count = nextByte & 0x7f;
                    if (count == 0)
                    {
                        break;
                    }

                    // If the high order bit is 1, that means we copy
                    // "count" bytes from the src to the dst channel
                    if (nextByte & 0x80)
                    {
                        // Copy "count" bytes from scanline
                        // into our buffer
                        for (unsigned char xc = 0; xc < count;
                             xc++, dstPtr += sizeof(Col4uc), srcPtr++)
                        {
                            *dstPtr = *srcPtr;
                        }
                    }
                    else
                    {
                        unsigned char chanVal = *srcPtr;
                        srcPtr++;
                        for (unsigned char xc = 0; xc < count;
                             xc++, dstPtr += sizeof(Col4uc))
                        {
                            *dstPtr = chanVal;
                        }
                    }

                    pixelsRead += count;
                }
                if (pixelsRead != header.width || !stream)
                {
                    delete[] offsetTable;
                    delete[] lengthTable;
                    delete[] scanline;
                    throw(IffExc("Corrupt RLE image file"));
                }
            }
        }

        // Set alpha to one if it did not exist in the file
        if (header.numChannels == 3)
        {
            Col4uc* pixel = img.begin();
            const int np = img.numPixels();
            for (int p = 0; p < np; ++p, ++pixel)
            {
                (*pixel).w = 255;
            }
        }

        // We're done. delete temporary stuff.
        delete[] offsetTable;
        delete[] lengthTable;
        delete[] scanline;
    }

    //******************************************************************************
    void SgiIffBase::readPixels(Img4uc& img, SgiIffBase::Header& header,
                                std::istream& stream)
    {
        // Allocate a scanline
        unsigned char* scanline = new unsigned char[header.width];

        // Loop over channels and rows.
        for (int c = 0; c < header.numChannels; c++)
        {
            // Loop over rows
            for (int row = 0; row < header.height; row++)
            {
                // Read scanline
                stream.read((char*)scanline, header.width);

                // Copy from scanline into dst.
                unsigned char* srcPtr = scanline;
                Col4uc* dstPtrPixel = img.begin() + (row * header.width);
                unsigned char* dstPtr = c + (unsigned char*)dstPtrPixel;
                for (int col = 0; col < header.width;
                     col++, srcPtr++, dstPtr += sizeof(Col4uc))
                {
                    *dstPtr = *srcPtr;
                }

                // Error checking
                if (!stream)
                {
                    delete[] scanline;
                    throw(IffExc("Corrupt uncompressed image file"));
                }
            }
        }

        // Set alpha to one if it did not exist in the file
        if (header.numChannels == 3)
        {
            Col4uc* pixel = img.begin();
            const int np = img.numPixels();
            for (int p = 0; p < np; ++p, ++pixel)
            {
                (*pixel).w = 255;
            }
        }

        // Clean up.
        delete[] scanline;
    }

    //******************************************************************************
    void SgiIffBase::writePixels(const Img4uc& img, SgiIffBase::Header& header,
                                 std::ostream& stream)
    {
        // Allocate a scanline
        unsigned char* scanline = new unsigned char[header.width];

        // Loop over channels and rows.
        for (int c = 0; c < header.numChannels; c++)
        {
            // Loop over rows
            for (int row = 0; row < header.height; row++)
            {
                // Copy scanline
                unsigned char* dstPtr = scanline;
                const Col4uc* srcPtrPixel = img.begin() + (row * header.width);
                unsigned char* srcPtr = c + (unsigned char*)srcPtrPixel;
                for (int col = 0; col < header.width;
                     col++, dstPtr++, srcPtr += sizeof(Col4uc))
                {
                    *dstPtr = *srcPtr;
                }

                // Write scanline
                stream.write((char*)scanline, header.width);

                // Error checking
                if (!stream)
                {
                    delete[] scanline;
                    throw(IffExc("Can't write uncompressed image file"));
                }
            }
        }

        // Clean up.
        delete[] scanline;
    }

    //******************************************************************************
    Img4uc* SgiIff4uc::read(const char* imgFileName)
    {
        std::ifstream imgInputStream(UNICODE_C_STR(imgFileName),
                                     BINARY_INPUT_FLAGS);

        if (!imgInputStream)
        {
            throw(IffExc("Could not open specified image file"));
        }

        Img4uc* ret = SgiIff4uc::read(imgInputStream);

        imgInputStream.close();

        return ret;
    }

    //******************************************************************************
    Img4uc* SgiIff4uc::read(std::istream& imgInputStream)
    {
        // Make a header
        Header header;
        header.read(imgInputStream);

        // Allocate pixels
        if (header.width <= 0 || header.height <= 0)
        {
            throw(IffExc("Invalid image width or height"));
        }

        // Create uc pixels
        Img4uc* img4uc = new Img4uc(header.width, header.height);

        // Copy white into alpha channel. readPixels
        // will overwrite this if there is alpha information
        // in the file.
        Col4uc* srcPixel = img4uc->begin();
        Col4uc* const srcEnd = srcPixel + img4uc->numPixels();
        for (/*No initializer*/; srcPixel < srcEnd; ++srcPixel)
        {
            (*srcPixel).w = 255;
        }

        // Now we need to determine whether or not read
        // as run length encoded or not.
        if (header.storage == 1)
        {
            readPixelsRLE(*img4uc, header, imgInputStream);
        }
        else
        {
            readPixels(*img4uc, header, imgInputStream);
        }

        // Return img4uc
        return img4uc;
    }

    //******************************************************************************
    Img4f* SgiIff4f::read(const char* imgFileName)
    {
        std::ifstream imgInputStream(UNICODE_C_STR(imgFileName),
                                     BINARY_INPUT_FLAGS);

        if (!imgInputStream)
        {
            throw(IffExc("Could not open specified image file"));
        }

        Img4f* ret = SgiIff4f::read(imgInputStream);

        imgInputStream.close();

        return ret;
    }

    //******************************************************************************
    Img4f* SgiIff4f::read(std::istream& imgInputStream)
    {
        // Make a header
        Header header;
        header.read(imgInputStream);

        // Allocate pixels
        if (header.width <= 0 || header.height <= 0)
        {
            throw(IffExc("Invalid image width or height"));
        }

        // Create uc pixels
        Img4uc img4uc(header.width, header.height);

        // Copy white into alpha channel. readPixels
        // will overwrite this if there is alpha information
        // in the file.
        Col4uc* srcPixel = img4uc.begin();
        Col4uc* const srcEnd = srcPixel + img4uc.numPixels();
        for (/*No initializer*/; srcPixel < srcEnd; ++srcPixel)
        {
            (*srcPixel).w = 255;
        }

        // Now we need to determine whether or not read
        // as run length encoded or not.
        if (header.storage == 1)
        {
            readPixelsRLE(img4uc, header, imgInputStream);
        }
        else
        {
            readPixels(img4uc, header, imgInputStream);
        }

        // Now create float pixels and perform conversion
        Img4f* retImg = new Img4f(header.width, header.height);
        srcPixel = img4uc.begin();
        Col4f* dstPixel = retImg->begin();
        Col4f* const dstEnd = dstPixel + retImg->numPixels();
        for (; dstPixel < dstEnd; ++srcPixel, ++dstPixel)
        {
            (*dstPixel).x = ((float)((*srcPixel).x)) / 255.0f;
            (*dstPixel).y = ((float)((*srcPixel).y)) / 255.0f;
            (*dstPixel).z = ((float)((*srcPixel).z)) / 255.0f;
            (*dstPixel).w = ((float)((*srcPixel).w)) / 255.0f;
        }

        // Return retImg. img4uc will be deleted automatically as we exit.
        return retImg;
    }

    //******************************************************************************
    void SgiIff4uc::write(const Img4uc* img, std::ostream& imgOutputStream)
    {
        // Verify
        assert(img != NULL);

        // Make header
        Header header(img->width(), img->height(), 4);
        header.write(imgOutputStream);

        // Now write pixels
        writePixels(*img, header, imgOutputStream);
    }

    //******************************************************************************
    void SgiIff4uc::write(const Img4uc* img, const char* imgFileName)
    {
        assert(img != NULL);

        std::ofstream imgOutputStream(UNICODE_C_STR(imgFileName),
                                      BINARY_OUTPUT_FLAGS);

        if (!imgOutputStream)
        {
            throw(IffExc("Could not open specified image file"));
        }

        SgiIff4uc::write(img, imgOutputStream);

        // Close the file.
        imgOutputStream.close();
    }

    //******************************************************************************
    void SgiIff4f::write(const Img4f* img, std::ostream& imgOutputStream)
    {
        // Verify
        assert(img != NULL);

        // Make header
        Header header(img->width(), img->height(), 4);
        header.write(imgOutputStream);

        // Create uc pixels and perform conversion;
        Img4uc img4uc(img->width(), img->height());

        // Copy pixels from image.
        const Col4f* srcPixel = img->begin();
        Col4uc* dstPixel = img4uc.begin();
        Col4uc* const dstEnd = dstPixel + img4uc.numPixels();
        for (; dstPixel < dstEnd; ++srcPixel, ++dstPixel)
        {
            (*dstPixel).x =
                (unsigned char)(std::clamp((*srcPixel).x, 0.0f, 1.0f) * 255.0f);
            (*dstPixel).y =
                (unsigned char)(std::clamp((*srcPixel).y, 0.0f, 1.0f) * 255.0f);
            (*dstPixel).z =
                (unsigned char)(std::clamp((*srcPixel).z, 0.0f, 1.0f) * 255.0f);
            (*dstPixel).w =
                (unsigned char)(std::clamp((*srcPixel).w, 0.0f, 1.0f) * 255.0f);
        }

        // Now write pixels
        writePixels(img4uc, header, imgOutputStream);
    }

    //******************************************************************************
    void SgiIff4f::write(const Img4f* img, const char* imgFileName)
    {
        assert(img != NULL);

        std::ofstream imgOutputStream(UNICODE_C_STR(imgFileName),
                                      BINARY_OUTPUT_FLAGS);

        if (!imgOutputStream)
        {
            throw(IffExc("Could not open specified image file"));
        }

        SgiIff4f::write(img, imgOutputStream);

        // Close the file.
        imgOutputStream.close();
    }

} // End namespace TwkImg
