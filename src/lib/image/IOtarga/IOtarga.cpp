//
//  Copyright (c) 2009 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IOtarga/IOtarga.h>
#include <TwkUtil/FileStream.h>
#include <TwkUtil/FileMMap.h>
#include <TwkUtil/File.h>
#include <TwkFB/Exception.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <TwkFB/Operations.h>
#include <TwkMath/Iostream.h>
#include <TwkUtil/Interrupt.h>
#include <TwkUtil/StdioBuf.h>
#include <TwkUtil/ByteSwap.h>
#include <TwkMath/Color.h>
#include <stl_ext/string_algo.h>
#include <vector>
#include <string>
#ifdef _MSC_VER
#define snprintf _snprintf
#endif

namespace TwkFB
{
    using namespace std;
    using namespace TwkUtil;
    using namespace TwkMath;

    IOtarga::IOtarga(IOType type, size_t chunkSize, int maxAsync)
        : StreamingFrameBufferIO("IOtarga", "m7", type, chunkSize, maxAsync)
    {
        //
        //  Indicate which extensions this plugin will handle The
        //  comparison against. The extensions are case-insensitive so
        //  there's no reason to provide upper case versions.
        //

        StringPairVector codecs;
        codecs.push_back(StringPair("RLE", "Run Length Encoding"));
        codecs.push_back(StringPair("RAW", "Raw (no compression)"));

        unsigned int cap = ImageRead | ImageWrite | Int8Capable | BruteForceIO;

        addType("targa", "TARGA Image", cap, codecs);
        addType("tga", "TARGA Image", cap, codecs);
        addType("tpic", "TARGA Image", cap, codecs);
    }

    IOtarga::~IOtarga() {}

    string IOtarga::about() const { return "TARGA (Tweak)"; }

    bool IOtarga::sanityCheck(const TGAFileHeader& header) const
    {
        U8 type = header.imageType;
        const ColorMapSpec& cs = header.colorMapSpec;

        if ((type != NoImageDataType && type != RawColorMapType
             && type != RawTrueColorType && type != RawBlackAndWhiteType
             && type != RLEColorMapType && type != RLETrueColorType
             && type != RLEBlackAndWhiteType)
            || (header.colorMapType != 0 && header.colorMapType != 1)
            || (header.colorMapType == 0
                && (cs.startIndex != 0 || cs.length != 0 || cs.entrySize != 0)))
        {
            //
            //  NOT A SPEC TARGA FILE
            //

            return false;
        }
        else
        {
            return true;
        }
    }

    const char* IOtarga::typeToString(ImageType t) const
    {
        switch (t)
        {
        case NoImageDataType:
            return "No Image Data";
        case RawColorMapType:
            return "Uncompressed Color-map";
        case RawTrueColorType:
            return "Uncompressed True-color";
        case RawBlackAndWhiteType:
            return "Uncompressed Black-and-White";
        case RLEColorMapType:
            return "RLE Color-map";
        case RLETrueColorType:
            return "RLE True-color";
        case RLEBlackAndWhiteType:
            return "RLE Black-and-White";
        }

        return "Unknown";
    }

    void IOtarga::readAttributes(FrameBuffer& fb,
                                 const TGAFileHeader& header) const
    {
        fb.attribute<string>("TARGA/ImageType") =
            typeToString((ImageType)header.imageType);
        fb.attribute<int>("TARGA/PixelDepth") = header.imageSpec.pixelDepth;
    }

    void IOtarga::readExtensionArea(FrameBuffer& fb,
                                    const unsigned char* indata) const
    {
        const char* data = (const char*)indata;
        U16 extSize = *(U16*)data;
        data += 2;
        if (extSize != 495)
            return;

        fb.attribute<string>("TARGA/AuthorName") = data;
        data += 41;
        fb.attribute<string>("TARGA/AuthorComments") = data;
        data += 324;

        U16 month = *(U16*)data;
        data += 2;
        U16 day = *(U16*)data;
        data += 2;
        U16 year = *(U16*)data;
        data += 2;
        U16 hour = *(U16*)data;
        data += 2;
        U16 minute = *(U16*)data;
        data += 2;
        U16 sec = *(U16*)data;
        data += 2;

        if (year != 0)
        {
            ostringstream str;
            str << setfill('0');
            str << setw(4) << year << "." << setw(2) << month << "." << setw(2)
                << day << " " << setw(2) << hour << ":" << setw(2) << minute
                << ":" << setw(2) << sec;
            fb.attribute<string>("TARGA/DateTimeStamp") = str.str();
        }

        fb.attribute<string>("TARGA/JobID") = data;
        data += 41;

        hour = *(U16*)data;
        data += 2;
        minute = *(U16*)data;
        data += 2;
        sec = *(U16*)data;
        data += 2;

        if (hour != 0 || minute != 0 || sec != 0)
        {
            ostringstream str;
            str << setfill('0') << setw(2) << hour << ":" << setw(2) << minute
                << ":" << setw(2) << sec;
            fb.attribute<string>("TARGA/JobTime") = str.str();
        }

        fb.attribute<string>("TARGA/SoftwareID") = data;
        data += 41;

        U16 ver = *(U16*)data;
        data += 2;
        ASCII verletter = *data;
        data++;

        if (ver && verletter)
        {
            ostringstream str;
            str << (double(ver) / 100.0) << verletter;
            fb.attribute<string>("TARGA/SoftwareVersion") = str.str();
        }

        U8 A = *data;
        data++;
        U8 R = *data;
        data++;
        U8 G = *data;
        data++;
        U8 B = *data;
        data++;

        {
            ostringstream str;
            str << int(A) << ", " << int(R) << ", " << int(G) << ", " << int(B);
            fb.attribute<string>("TARGA/KeyColor") = str.str();
        }

        U16 gammaNum = *(U16*)data;
        data += 2;
        U16 gammaDen = *(U16*)data;
        data += 2;

        if (gammaDen != 0)
        {
            float gamma = double(gammaNum) / double(gammaDen);
            fb.attribute<float>("TARGA/Gamma") = gamma;
        }

        U32 ccOffset = *(U32*)data;
        data += 4;
        U32 psOffset = *(U32*)data;
        data += 4;
        U32 slOffset = *(U32*)data;
        data += 4;
        U8 attrType = *data;
        data++;

        {
            ostringstream str;

            switch (attrType)
            {
            case 0:
                str << "No Alpha";
                break;
            case 1:
                str << "Undefined Ignored";
                break;
            case 2:
                str << "Undefinied Not Ignored";
                break;
            case 3:
                str << "Unpremulted";
                break;
            case 4:
                str << "Premulted";
                break;
            default:
                str << "Unknown";
                break;
            }

            fb.attribute<string>("TARGA/AttributeType") = str.str();
        }
    }

    void IOtarga::readHeader(const unsigned char* data,
                             TGAFileHeader& header) const
    {
        header.IDLength = *data;
        data++;
        header.colorMapType = *data;
        data++;
        header.imageType = *data;
        data++;
        header.colorMapSpec.startIndex = *(U16*)data;
        data += 2;
        header.colorMapSpec.length = *(U16*)data;
        data += 2;
        header.colorMapSpec.entrySize = *data;
        data++;
        memcpy(&header.imageSpec, data, sizeof(ImageSpec));
        data += sizeof(ImageSpec);
    }

    void IOtarga::readFooter(const unsigned char* data,
                             TGAFileFooter& footer) const
    {
        footer.extOffset = *(U32*)data;
        data += 4;
        footer.devOffset = *(U32*)data;
        data += 4;
        memcpy(footer.signature, data, 18);
    }

    void IOtarga::getImageInfo(const std::string& filename, FBInfo& fbi) const
    {
        ifstream infile(UNICODE_C_STR(filename.c_str()), ios::binary | ios::in);

        if (!infile)
        {
            TWK_THROW_STREAM(IOException, "Unable to open TARGA file \""
                                              << filename << "\" for reading");
        }

        vector<unsigned char> data(3 + 5 + 10);
        TGAFileHeader header;
        infile.read((char*)&data.front(), data.size());
        readHeader(&data.front(), header);

#if defined(TWK_BIG_ENDIAN)
        header.swap();
#endif

        //
        //  Try and figure out if this really is a TGA file
        //

        if (!sanityCheck(header))
        {
            TWK_THROW_STREAM(IOException, "TARGA: cannot open " << filename);
        }

        if (header.imageType != RawTrueColorType
            && header.imageType != RLETrueColorType)
        {
            TWK_THROW_STREAM(IOException,
                             "TARGA: unsupport TARGA image type " << filename);
        }

        fbi.numChannels = (header.imageSpec.descriptor & 0xf) == 0xf ? 4 : 3;
        fbi.width = header.imageSpec.width;
        fbi.height = header.imageSpec.height;
        fbi.pixelAspect = double(1.0);
        fbi.dataType = FrameBuffer::UCHAR;

        switch ((header.imageSpec.descriptor & 0x30) >> 4)
        {
        case 0:
            fbi.orientation = FrameBuffer::NATURAL;
            break;
        case 1:
            fbi.orientation = FrameBuffer::BOTTOMRIGHT;
            break;
        case 2:
            fbi.orientation = FrameBuffer::TOPLEFT;
            break;
        case 3:
            fbi.orientation = FrameBuffer::TOPRIGHT;
            break;
        }

        TGAFileFooter footer;
        infile.seekg(-26, ios::end);
        infile.read((char*)&footer, 26);

        readAttributes(fbi.proxy, header);

        if (!strncmp(footer.signature, "TRUEVISION-XFILE.", 18))
        {
            fbi.proxy.attribute<string>("TARGA/Signature") =
                "TRUEVISION-XFILE.";

            if (footer.extOffset)
            {
                vector<char> extArea(2 + 41 + 324 + 12 + 41 + 6 + 41 + 3 + 4 + 4
                                     + 4 + 4 + 4 + 4 + 1);
                infile.seekg(footer.extOffset, ios::beg);
                infile.read(&extArea.front(), extArea.size());
                readExtensionArea(fbi.proxy,
                                  (const unsigned char*)&extArea.front());
            }
        }
        else
        {
            fbi.proxy.attribute<string>("TARGA/Signature") = "N/A";
        }
    }

    void IOtarga::readImage(FrameBuffer& fb, const std::string& filename,
                            const ReadRequest& request) const
    {
        FileStream::Type ftype = m_iotype == StandardIO
                                     ? FileStream::Buffering
                                     : (FileStream::Type)(m_iotype - 1);

        FileStream fmap(filename, ftype, m_iosize, m_iomaxAsync);

        TGAFileHeader header;
        const unsigned char* data = (const unsigned char*)fmap.data();
        size_t len = fmap.size();
        readHeader(data, header);

        TGAFileFooter footer;
        readFooter(data + (len - 26), footer);

        data += 3 + 5 + 10;

#if defined(TWK_BIG_ENDIAN)
        header.swap();
#endif

        //
        //  Try and figure out if this really is a TGA file
        //

        if (!sanityCheck(header))
        {
            TWK_THROW_STREAM(IOException, "TARGA: cannot open " << filename);
        }

        if (header.imageType != RawTrueColorType
            && header.imageType != RLETrueColorType)
        {
            TWK_THROW_STREAM(IOException,
                             "TARGA: unsupport TARGA image type " << filename);
        }

        const int ch = (header.imageSpec.descriptor & 0xf) == 0xf ? 4 : 3;
        const int w = header.imageSpec.width;
        const int h = header.imageSpec.height;

        fb.restructure(w, h, 0, 4, FrameBuffer::UCHAR);

        switch ((header.imageSpec.descriptor & 0x30) >> 4)
        {
        case 0:
            fb.setOrientation(FrameBuffer::NATURAL);
            break;
        case 1:
            fb.setOrientation(FrameBuffer::BOTTOMRIGHT);
            break;
        case 2:
            fb.setOrientation(FrameBuffer::TOPLEFT);
            break;
        case 3:
            fb.setOrientation(FrameBuffer::TOPRIGHT);
            break;
        }

        //
        //  Read ImageID section
        //

        if (header.IDLength != 0)
        {
            vector<char> identifier(header.IDLength + 1);
            identifier.back() = 0;
            memcpy(&identifier.front(), data, header.IDLength);
            fb.attribute<string>("TARGA/ImageIdentifier") = &identifier.front();
            data += header.IDLength;
        }

        if (header.colorMapType != 0)
        {
            //
        }

        data += header.colorMapSpec.length;

        //
        //  Read image data
        //

        size_t alphabits = header.imageSpec.descriptor & 0xf;

        switch (header.imageSpec.pixelDepth)
        {
        case 8:
            break;
        case 16:
            if (header.imageType == RawTrueColorType)
            {
                for (unsigned char *p = fb.pixels<unsigned char>(),
                                   *e = p + fb.planeSize();
                     p != e; p += 4, data += 2)
                {
                    U16 pixel = *(U16*)data;
                    p[0] = (pixel & 0x7c00) >> 7;
                    p[1] = (pixel & 0x3e0) >> 2;
                    p[2] = (pixel & 0x1f) << 3;
                    p[3] = 255;
                }
            }
            else if (header.imageType == RLETrueColorType)
            {
                for (unsigned char *p = fb.pixels<unsigned char>(),
                                   *e = p + fb.planeSize();
                     p < e;)
                {
                    const size_t repCount = *data;
                    data++;
                    const bool rawPacket = (repCount & 0x80) == 0;
                    const size_t count = (repCount & 0x7f) + 1;

                    for (size_t i = 0; i < count && p < e; i++, p += 4)
                    {
                        U16 pixel = *(U16*)data;
                        p[0] = (pixel & 0x7c00) >> 7;
                        p[1] = (pixel & 0x3e0) >> 2;
                        p[2] = (pixel & 0x1f) << 3;
                        p[3] = 255;

                        if (rawPacket)
                            data += 2;
                    }

                    if (!rawPacket)
                        data += 2;
                }
            }
            break;
        case 24:
            if (header.imageType == RawTrueColorType)
            {
                for (unsigned char *p = fb.pixels<unsigned char>(),
                                   *e = p + fb.planeSize();
                     p != e; p += 4, data += 3)
                {
                    p[0] = data[2];
                    p[1] = data[1];
                    p[2] = data[0];
                    p[3] = 255;
                }
            }
            else if (header.imageType == RLETrueColorType)
            {
                for (unsigned char *p = fb.pixels<unsigned char>(),
                                   *e = p + fb.planeSize();
                     p < e;)
                {
                    const size_t repCount = *data;
                    data++;
                    const bool rawPacket = (repCount & 0x80) == 0;
                    const size_t count = (repCount & 0x7f) + 1;

                    for (size_t i = 0; i < count && p < e; i++, p += 4)
                    {
                        p[0] = data[2];
                        p[1] = data[1];
                        p[2] = data[0];
                        p[3] = 255;

                        if (rawPacket)
                            data += 3;
                    }

                    if (!rawPacket)
                        data += 3;
                }
            }
            break;
        case 32:
            if (header.imageType == RawTrueColorType)
            {
                for (unsigned char *p = fb.pixels<unsigned char>(),
                                   *e = p + fb.planeSize();
                     p != e; p += 4, data += 4)
                {
                    p[0] = data[2];
                    p[1] = data[1];
                    p[2] = data[0];
                    p[3] = data[3];
                }
            }
            else if (header.imageType == RLETrueColorType)
            {
                for (unsigned char *p = fb.pixels<unsigned char>(),
                                   *e = p + fb.planeSize();
                     p < e;)
                {
                    const U8 repCount = *data;
                    data++;
                    const bool rawPacket = (repCount & 0x80) == 0;
                    const size_t count = (repCount & 0x7f) + 1;

                    for (size_t i = 0; i < count && p < e; i++, p += 4)
                    {
                        p[0] = data[2];
                        p[1] = data[1];
                        p[2] = data[0];
                        p[3] = data[3];

                        if (rawPacket)
                            data += 4;
                    }

                    if (!rawPacket)
                        data += 4;
                }
            }
            break;
        }

        if (!strncmp(footer.signature, "TRUEVISION-XFILE.", 18))
        {
            fb.attribute<string>("TARGA/Signature") = footer.signature;

            if (footer.extOffset)
            {
                readExtensionArea(fb, (unsigned char*)fmap.data()
                                          + footer.extOffset);
            }
        }
        else
        {
            fb.attribute<string>("TARGA/Signature") = "N/A";
        }

        readAttributes(fb, header);
    }

    void IOtarga::writeImage(const FrameBuffer& img,
                             const std::string& filename,
                             const WriteRequest& request) const
    {
        const FrameBuffer* outfb = &img;

        //
        //  Convert to UCHAR packed if not already.
        //

        if (img.isPlanar())
        {
            const FrameBuffer* fb = outfb;
            outfb = mergePlanes(outfb);
            if (fb != &img)
                delete fb;
        }

        //
        //  Convert everything to REC709
        //

        if (outfb->hasPrimaries() || outfb->isYUV() || outfb->isYRYBY()
            || outfb->dataType() >= FrameBuffer::PACKED_R10_G10_B10_X2)
        {
            const FrameBuffer* fb = outfb;
            outfb = convertToLinearRGB709(outfb);
            if (fb != &img)
                delete fb;
        }

        //
        //  Convert to the format we need
        //

        switch (img.dataType())
        {
        case FrameBuffer::HALF:
        case FrameBuffer::FLOAT:
        case FrameBuffer::USHORT:
        {
            const FrameBuffer* fb = outfb;
            outfb = copyConvert(outfb, FrameBuffer::UCHAR);
            if (fb != &img)
                delete fb;
            break;
        }
        default:
            break;
        }

        const bool rle = request.compression == "RLE";

        ofstream outfile(UNICODE_C_STR(filename.c_str()),
                         ios::binary | ios::out);

        if (!outfile)
        {
            TWK_THROW_STREAM(IOException,
                             "TARGA: cannot write TARGA file " << filename);
        }

        char b = 0;
        typedef unsigned char byte;

        bool alpha = outfb->hasChannel("A");

        outfile.write(&b, 1); // ID Length
        outfile.write(&b, 1); // colormap type
        b = rle ? RLETrueColorType : RawTrueColorType;
        outfile.write(&b, 1); // image type

        char five[5];
        memset(five, 0, 5 * sizeof(char));

        outfile.write(five, 5);

        short sz = 0;
        short w = outfb->width();
        short h = outfb->height();

        b = alpha ? 32 : 24;

        outfile.write((const char*)&sz, 2);
        outfile.write((const char*)&sz, 2);
        outfile.write((const char*)&w, 2);
        outfile.write((const char*)&h, 2);
        outfile.write((const char*)&b, 1);

        unsigned char imgd = alpha ? 8 : 0;

        switch (outfb->orientation())
        {
        case FrameBuffer::BOTTOMRIGHT:
            imgd &= 1 << 4;
            break;
        case FrameBuffer::TOPRIGHT:
            imgd &= 3 << 4;
            break;
        case FrameBuffer::TOPLEFT:
            imgd &= 2 << 4;
            break;
        default:
            break;
        }

        outfile.write((const char*)&imgd, 1);

        // no image id

        //
        //  Image data
        //

        if (rle)
        {
            int total = 0;
            bool newline = false;
            bool record = false;
            int count = 0;
            int loop = 0;
            int numChannels = (alpha) ? 4 : 3;
            vector<unsigned char> l(numChannels);
            for (const unsigned char *p = outfb->pixels<unsigned char>(),
                                     *e = p + outfb->planeSize();
                 p < e; p += numChannels)
            {
                if (record && (!pixelsMatch(p, &l[0], alpha) || newline))
                {
                    writeRLE(&outfile, &l[0], numChannels, count);
                    total += count;
                    count = 0;
                }
                l[2] = p[0];
                l[1] = p[1];
                l[0] = p[2];
                if (alpha)
                    l[3] = p[3];
                count++;
                record = true;
                loop++;
                newline = ((loop % outfb->width()) == 0);
            }
            writeRLE(&outfile, &l[0], numChannels, count);
        }
        else
        {
            const size_t ss = outfb->scanlineSize();
            vector<unsigned char> scanline(ss);

            for (size_t i = 0, h = outfb->height(); i < h; i++)
            {
                const unsigned char* p = outfb->scanline<unsigned char>(i);
                unsigned char* o = &scanline.front();

                if (alpha)
                {
                    for (const unsigned char* e = p + ss; p < e; p += 4, o += 4)
                    {
                        o[2] = p[0];
                        o[1] = p[1];
                        o[0] = p[2];
                        o[3] = p[3];
                    }
                }
                else
                {
                    for (const unsigned char* e = p + ss; p < e; p += 3, o += 3)
                    {
                        o[2] = p[0];
                        o[1] = p[1];
                        o[0] = p[2];
                    }
                }

                outfile.write((const char*)&scanline.front(), ss);
            }
        }
    }

    void IOtarga::writeRLE(ofstream* outfile, unsigned char* pixel,
                           int numChannels, int count) const
    {
        if ((count * numChannels) > (1 + numChannels))
        {
            while (count > 0)
            {
                int ncount = min(128, count);
                unsigned char head = (ncount - 1) | 0x80;
                outfile->write((const char*)&head, 1);
                outfile->write((const char*)pixel, numChannels);
                count -= ncount;
            }
        }
        else
        {
            unsigned char head = (count - 1) & ~0x80;
            outfile->write((const char*)&head, 1);
            for (int i = 0; i < count; i++)
            {
                outfile->write((const char*)pixel, numChannels);
            }
        }
    }

    bool IOtarga::pixelsMatch(const unsigned char* p1, const unsigned char* p2,
                              bool alpha) const
    {
        return (p1[0] == p2[2] && p1[1] == p2[1] && p1[2] == p2[0]
                && (!alpha || (alpha && p1[3] == p2[3])));
    }

} // namespace TwkFB
