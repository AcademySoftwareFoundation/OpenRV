//******************************************************************************
// Copyright (c) 2001 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkImg/TwkImgExrIff.h>
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

#ifdef _MSC_VER
#include <winsock.h>
#else
#include <unistd.h>
#endif

#include <ImfRgbaFile.h>
#include <ImfArray.h>
#include <ImfStringAttribute.h>
#include <ImfMatrixAttribute.h>

//******************************************************************************
namespace TwkImg
{
    using namespace std;
    using namespace Imf;
    using namespace Imath;
    using TwkMath::Mat44f;

    //*****************************************************************************
    Img4f* ExrIff::read(const char* imgFileName)
    {
        RgbaInputFile file(imgFileName);
        Box2i dw = file.dataWindow();

        int width = dw.max.x - dw.min.x + 1;
        int height = dw.max.y - dw.min.y + 1;

        Array2D<Rgba> exrBuffer;
        exrBuffer.resizeErase(height, width);

        file.setFrameBuffer(&exrBuffer[0][0] - dw.min.x - dw.min.y * width, 1,
                            width);
        file.readPixels(dw.min.y, dw.max.y);

        Img4f* ret = new Img4f(width, height);

        for (int y = 0; y < height; ++y)
        {
            size_t iy = height - y - 1;
            for (int x = 0; x < width; ++x)
            {
                ret->pixel(x, iy).x = (float)exrBuffer[y][x].r;
                ret->pixel(x, iy).y = (float)exrBuffer[y][x].g;
                ret->pixel(x, iy).z = (float)exrBuffer[y][x].b;
                ret->pixel(x, iy).w = (float)exrBuffer[y][x].a;
            }
        }

        return ret;
    }

    //*****************************************************************************
    Img4f* ExrIff::read(const char* imgFileName, TwkMath::Mat44f& Mcamera,
                        TwkMath::Mat44f& Mscreen)
    {
        RgbaInputFile file(imgFileName);
        Box2i dw = file.dataWindow();

        int width = dw.max.x - dw.min.x + 1;
        int height = dw.max.y - dw.min.y + 1;

        const Imf::M44fAttribute* w2cAttr =
            file.header().findTypedAttribute<Imf::M44fAttribute>(
                "worldToCamera");
        if (w2cAttr != NULL)
        {
            // Note that we're NOT reading it transposed!
            for (int i = 0; i < 4; ++i)
            {
                for (int j = 0; j < 4; ++j)
                {
                    Mcamera[i][j] = w2cAttr->value()[i][j];
                }
            }
        }

        const Imf::M44fAttribute* w2sAttr =
            file.header().findTypedAttribute<Imf::M44fAttribute>(
                "worldToScreen");
        if (w2sAttr != NULL)
        {
            // Note that we're NOT reading it TRANSPOSED!!!
            for (int i = 0; i < 4; ++i)
            {
                for (int j = 0; j < 4; ++j)
                {
                    Mscreen[i][j] = w2sAttr->value()[i][j];
                }
            }
        }

        Array2D<Rgba> exrBuffer;
        exrBuffer.resizeErase(height, width);

        file.setFrameBuffer(&exrBuffer[0][0] - dw.min.x - dw.min.y * width, 1,
                            width);
        file.readPixels(dw.min.y, dw.max.y);

        Img4f* ret = new Img4f(width, height);

        for (int y = 0; y < height; ++y)
        {
            size_t iy = height - y - 1;
            for (int x = 0; x < width; ++x)
            {
                ret->pixel(x, iy).x = (float)exrBuffer[y][x].r;
                ret->pixel(x, iy).y = (float)exrBuffer[y][x].g;
                ret->pixel(x, iy).z = (float)exrBuffer[y][x].b;
                ret->pixel(x, iy).w = (float)exrBuffer[y][x].a;
            }
        }

        return ret;
    }

    // *****************************************************************************
    bool ExrIff::write(const Img4f* img, const char* fileName,
                       Imf::Compression compression)
    {
        Header header(img->width(), img->height(),
                      1.0f,              // pixelAspectRatio = 1
                      Imath::V2f(0, 0),  // screenWindowCenter
                      1.0f,              // screenWindowWidth
                      Imf::INCREASING_Y, // lineOrder
                      compression);

        char hostname[64];
        gethostname(hostname, 64);
        header.insert("host", StringAttribute(hostname));

        RgbaOutputFile file(fileName, header, WRITE_RGBA);

        Box2i win = header.dataWindow();
        Array<Rgba> lineBuffer(img->width());
        file.setFrameBuffer(lineBuffer - win.min.x, 1, 0);

        for (int y = win.min.y; y <= win.max.y; ++y)
        {
            Imf::Rgba* pix = &lineBuffer[0];
            int iy = img->height() - y - 1;
            for (int x = win.min.x; x <= win.max.x; ++x)
            {
                pix->r = img->pixel(x, iy).x;
                pix->g = img->pixel(x, iy).y;
                pix->b = img->pixel(x, iy).z;
                pix->a = img->pixel(x, iy).w;
                ++pix;
            }
            file.writePixels(1);
        }

        return true;
    }

    // *****************************************************************************
    bool ExrIff::write(const Img4f* img, const char* fileName,
                       const Mat44f& Mcamera, const Mat44f& Mscreen,
                       Imf::Compression compression)
    {
        Header header(img->width(), img->height(),
                      1.0f,              // pixelAspectRatio = 1
                      Imath::V2f(0, 0),  // screenWindowCenter
                      1.0f,              // screenWindowWidth
                      Imf::INCREASING_Y, // lineOrder
                      compression);

        char hostname[64];
        gethostname(hostname, 64);
        header.insert("host", StringAttribute(hostname));

        Imath::M44f w2c;
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                w2c[i][j] = Mcamera[i][j];
            }
        }
        header.insert("worldToCamera", Imf::M44fAttribute(w2c));

        Imath::M44f w2s;
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                w2s[i][j] = Mscreen[i][j];
            }
        }
        header.insert("worldToScreen", Imf::M44fAttribute(w2s));

        RgbaOutputFile file(fileName, header, WRITE_RGBA);

        Box2i win = header.dataWindow();
        Array<Rgba> lineBuffer(img->width());
        file.setFrameBuffer(lineBuffer - win.min.x, 1, 0);

        for (int y = win.min.y; y <= win.max.y; ++y)
        {
            Imf::Rgba* pix = &lineBuffer[0];
            int iy = img->height() - y - 1;
            for (int x = win.min.x; x <= win.max.x; ++x)
            {
                pix->r = img->pixel(x, iy).x;
                pix->g = img->pixel(x, iy).y;
                pix->b = img->pixel(x, iy).z;
                pix->a = img->pixel(x, iy).w;
                ++pix;
            }
            file.writePixels(1);
        }

        return true;
    }

} // End namespace TwkImg
