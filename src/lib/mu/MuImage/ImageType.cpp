//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <MuImage/ImageType.h>
#include <Mu/BaseFunctions.h>
#include <Mu/Function.h>
#include <Mu/MemberFunction.h>
#include <Mu/MemberVariable.h>
#include <Mu/Module.h>
#include <Mu/ReferenceType.h>
#include <Mu/Thread.h>
#include <Mu/Vector.h>
#include <Mu/VariantType.h>
#include <Mu/VariantTagType.h>
#include <MuLang/DynamicArrayType.h>
#include <MuLang/DynamicArray.h>
#include <MuLang/ExceptionType.h>
#include <MuLang/MuLangContext.h>
#include <MuLang/StringType.h>
#include <ctype.h>
#include <iostream>
#include <sstream>
#include <tiffio.h>
#include <algorithm>

namespace Mu
{
    using namespace std;
    using namespace Mu;

    Vector4f ImageType::ImageStruct::sample(float x, float y)
    {
        double dx = 1.0 / double(width);
        double dy = 1.0 / double(height);

        int px = int(x * (width - 1));
        int py = int(y * (height - 1));
        int px1 = px >= width ? px : px + 1;
        int py1 = py >= height ? py : py + 1;

        float rx = (x - (dx * px)) / dx;
        float ry = (y - (dy * py)) / dy;
        float rx0 = 1.0 - rx;
        float ry0 = 1.0 - ry;

        Vector4f p0 = pixel(px, py);
        Vector4f p1 = pixel(px1, py);
        Vector4f p2 = pixel(px1, py1);
        Vector4f p3 = pixel(px, py1);
        Vector4f pa = rx0 * p0 + rx * p1;
        Vector4f pb = rx0 * p3 + rx * p2;

        return ry0 * pa + ry * pb;
    }

    static void readTIFF(ImageType::ImageStruct* im,
                         const Mu::String& imgFileName, Thread& thread)
    {
        Process* process = thread.process();
        MuLangContext* context = (MuLangContext*)process->context();

        //
        // Suppress annoying messages about unknown tags, etc...
        //

        TIFFSetErrorHandler(0);
        TIFFSetWarningHandler(0);

        TIFF* tif = TIFFOpen(imgFileName.c_str(), "r");

        if (!tif)
        {
            const Mu::MuLangContext* context =
                static_cast<const Mu::MuLangContext*>(thread.context());
            ExceptionType::Exception* e =
                new ExceptionType::Exception(context->exceptionType());
            e->string() = "failed to open image \"";
            e->string() += imgFileName;
            e->string() += "\"";
            thread.setException(e);
            throw ProgramException(e);
        }

        uint16* sampleinfo;
        uint16 extrasamples;
        uint16 bps;

        int width, height;

        TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
        TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
        TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bps);

        im->width = width;
        im->height = height;

        TIFFGetFieldDefaulted(tif, TIFFTAG_EXTRASAMPLES, &extrasamples,
                              &sampleinfo);

        unsigned short orient = ORIENTATION_TOPLEFT;
        TIFFGetField(tif, TIFFTAG_ORIENTATION, &orient);
        bool flip =
            orient == ORIENTATION_BOTLEFT || orient == ORIENTATION_BOTRIGHT;
        bool flop =
            orient == ORIENTATION_BOTRIGHT || orient == ORIENTATION_TOPRIGHT;
        flop = false;

        bool hasAlpha = extrasamples == 1 ? true : false;

        DynamicArrayType* dataType = static_cast<DynamicArrayType*>(
            context->arrayType(context->vec4fType(), 1, 0));
        im->data = new DynamicArray(dataType, 1);
        im->data->resize(width * height);

        vector<char> buf(TIFFScanlineSize(tif));

        int y0 = 0;
        int y1 = height;
        int inc = 1;

        switch (bps)
        {
        case 32:
            for (int y = 0; y < height; y++)
            {
                int oy = flip ? (height - 1 - y) : y;

                if (hasAlpha)
                {
                    TIFFReadScanline(tif, (float*)im->row(oy), y);
                }
                else
                {
                    TIFFReadScanline(tif, &buf[0], y);
                    Vector4f* dstPixel = im->row(oy);
                    Vector3f* srcPixel = reinterpret_cast<Vector3f*>(&buf[0]);

                    for (int x = 0; x < width; ++x, ++dstPixel, ++srcPixel)
                    {
                        (*dstPixel)[0] = (*srcPixel)[0];
                        (*dstPixel)[1] = (*srcPixel)[1];
                        (*dstPixel)[2] = (*srcPixel)[2];
                        (*dstPixel)[3] = 1.0f;
                    }
                }

                if (flop)
                    reverse(im->row(oy), im->row(oy) + width);
            }
            break;

        case 16:
            for (int y = 0; y < height; y++)
            {
                int oy = flip ? (height - 1 - y) : y;
                TIFFReadScanline(tif, &buf[0], y);
                Vector4f* dstPixel = im->row(oy);

                if (hasAlpha)
                {
                    Vector4s* srcPixel = reinterpret_cast<Vector4s*>(&buf[0]);

                    for (int x = 0; x < width; ++x, ++dstPixel, ++srcPixel)
                    {
                        (*dstPixel)[0] = (*srcPixel)[0] / 65535.0f;
                        (*dstPixel)[1] = (*srcPixel)[1] / 65535.0f;
                        (*dstPixel)[2] = (*srcPixel)[2] / 65535.0f;
                        (*dstPixel)[3] = (*srcPixel)[3] / 65535.0f;
                    }
                }
                else
                {
                    Vector3s* srcPixel = reinterpret_cast<Vector3s*>(&buf[0]);

                    for (int x = 0; x < width; ++x, ++dstPixel, ++srcPixel)
                    {
                        (*dstPixel)[0] = (*srcPixel)[0] / 65535.0f;
                        (*dstPixel)[1] = (*srcPixel)[1] / 65535.0f;
                        (*dstPixel)[2] = (*srcPixel)[2] / 65535.0f;
                        (*dstPixel)[3] = 1.0f;
                    }
                }

                if (flop)
                    reverse(im->row(oy), im->row(oy) + width);
            }
            break;

        case 8:

            for (int y = 0; y < height; y++)
            {
                int oy = flip ? (height - 1 - y) : y;
                TIFFReadScanline(tif, &buf[0], y);
                Vector4f* dstPixel = im->row(oy);

                if (hasAlpha)
                {
                    Vector4c* srcPixel = reinterpret_cast<Vector4c*>(&buf[0]);

                    for (int x = 0; x < width; ++x, ++dstPixel, ++srcPixel)
                    {
                        (*dstPixel)[0] = (*srcPixel)[0] / 255.0f;
                        (*dstPixel)[1] = (*srcPixel)[1] / 255.0f;
                        (*dstPixel)[2] = (*srcPixel)[2] / 255.0f;
                        (*dstPixel)[3] = (*srcPixel)[3] / 255.0f;
                    }
                }
                else
                {
                    Vector3c* srcPixel = reinterpret_cast<Vector3c*>(&buf[0]);

                    for (int x = 0; x < width; ++x, ++dstPixel, ++srcPixel)
                    {
                        (*dstPixel)[0] = (*srcPixel)[0] / 255.0f;
                        (*dstPixel)[1] = (*srcPixel)[1] / 255.0f;
                        (*dstPixel)[2] = (*srcPixel)[2] / 255.0f;
                        (*dstPixel)[3] = 1.0f;
                    }
                }

                if (flop)
                    reverse(im->row(oy), im->row(oy) + width);
            }
            break;

        default:

            TIFFClose(tif);
            {
                width = 0;
                const Mu::MuLangContext* context =
                    static_cast<const Mu::MuLangContext*>(thread.context());
                ExceptionType::Exception* e =
                    new ExceptionType::Exception(context->exceptionType());
                e->string() = "Unsupported bit depth in image file \"";
                e->string() += imgFileName;
                e->string() += "\"";
                thread.setException(e);
                throw ProgramException(e);
            }
        }

        TIFFClose(tif);
    }

    //----------------------------------------------------------------------

    ImageType::ImageType(Context* c, Class* super)
        : Class(c, "image", super)
    {
    }

    ImageType::~ImageType() {}

    void ImageType::load()
    {
        USING_MU_FUNCTION_SYMBOLS;

        Symbol* s = scope();
        MuLangContext* context = (MuLangContext*)globalModule()->context();
        Context* c = context;

        String tname = s->name();
        tname += ".";
        tname += "image";
        // string tname = "image";

        String rname = tname + "&";

        const char* tn = tname.c_str();
        const char* rn = rname.c_str();

        s->addSymbols(new ReferenceType(c, "image&", this),

                      new Function(c, "image", ImageType::construct, None,
                                   Return, tn, Args, "string", End),

                      new Function(c, "image", BaseFunctions::dereference, Cast,
                                   Return, tn, Args, rn, End),

                      EndArguments);

        globalScope()->addSymbols(

            new Function(c, "=", BaseFunctions::assign, AsOp, Return, rn, Args,
                         rn, tn, End),

            EndArguments);

        //
        //  Can't make the array type until the reference type above exists
        //

        context->arrayType(context->vec4fType(), 1, 0);
        context->arrayType(context->byteType(), 1, 0);
        context->arrayType(context->floatType(), 1, 0);

        addSymbols(new MemberVariable(c, "name", "string"),
                   new MemberVariable(c, "data", "(vector float[4])[]"),
                   new MemberVariable(c, "width", "int"),
                   new MemberVariable(c, "height", "int"),

                   new Function(c, "()", ImageType::sample, Mapped, Return,
                                "vector float[4]", Args, tn, "float", "float",
                                End),

                   EndArguments);
    }

    NODE_IMPLEMENTATION(ImageType::construct, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const Class* c = static_cast<const ImageType*>(NODE_THIS.type());

        const StringType::String* file = NODE_ARG_OBJECT(0, StringType::String);

        ClassInstance* o = ClassInstance::allocate(c);
        ImageStruct* im = reinterpret_cast<ImageStruct*>(o->structure());
        im->name = file;

        readTIFF(im, file->c_str(), NODE_THREAD);
        NODE_RETURN(Pointer(o));
    }

    NODE_IMPLEMENTATION(ImageType::sample, Vector4f)
    {
        ClassInstance* o = NODE_ARG_OBJECT(0, ClassInstance);
        ImageStruct* im = reinterpret_cast<ImageStruct*>(o->structure());

        if (!o)
            throw NilArgumentException();
        float x = NODE_ARG(1, float);
        float y = NODE_ARG(2, float);
        NODE_RETURN(im->sample(x, y));
    }

} // namespace Mu
