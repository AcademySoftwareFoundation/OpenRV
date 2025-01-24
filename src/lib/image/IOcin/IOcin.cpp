//******************************************************************************
// Copyright (c) 2001-2004 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <IOcin/IOcin.h>
#include <TwkUtil/FileStream.h>
#include <IOcin/Read10Bit.h>
#include <TwkFB/Exception.h>
#include <TwkFB/Operations.h>
#include <TwkImg/TwkImgCineonIff.h>
#include <TwkMath/Iostream.h>
#include <TwkUtil/Interrupt.h>
#include <TwkMath/Color.h>
#include <fstream>
#include <iostream>
#include <stl_ext/string_algo.h>
#include <string>

#ifdef _MSC_VER
// visual studio doesn't have inttypes.h
// (http://forums.microsoft.com/MSDN/ShowPost.aspx?PostID=135168&SiteID=1)
#define uint16_t __int16
#define uint32_t __int32
// "remove" restrict on WIN32
#define restrict
#endif

namespace TwkFB
{
    using namespace std;
    using namespace TwkUtil;
    using namespace TwkMath;
    using namespace std;

    static void setFBOrientation(FrameBuffer& infb,
                                 FrameBuffer::Orientation orientation)
    {
        for (FrameBuffer* fb = &infb; fb; fb = fb->nextPlane())
        {
            fb->setOrientation(orientation);
        }
    }

    struct NameValue
    {
        const char* name;
        unsigned int value;
    };

    static NameValue orientationNames[] = {
        {"Top Left", IOcin::CIN_TOP_LEFT},
        {"Bottom Left", IOcin::CIN_BOTTOM_LEFT},
        {"Top Right", IOcin::CIN_TOP_RIGHT},
        {"Bottom Right", IOcin::CIN_BOTTOM_RIGHT},
        {"Rotated Top Left", IOcin::CIN_ROTATED_TOP_LEFT},
        {"Rotated Bottom Left", IOcin::CIN_ROTATED_BOTTOM_LEFT},
        {"Rotated Top Right", IOcin::CIN_ROTATED_TOP_RIGHT},
        {"Rotated Bottom Right", IOcin::CIN_ROTATED_BOTTOM_RIGHT},
        {"", 0}};

    //----------------------------------------------------------------------
    //
    //  Cineon structs
    //

    IOcin::IOcin(StorageFormat format, bool useChromaticies, IOType type,
                 size_t iosize, int maxAsync)
        : StreamingFrameBufferIO("IOcin", "m6", type, iosize, maxAsync)
        , m_format(format)
        , m_useChromaticities(useChromaticies)
    {
        init();
    }

    IOcin::IOcin(const std::string& format, bool useChromaticies, IOType type,
                 size_t iosize, int maxAsync)
        : StreamingFrameBufferIO("IOcin", "m6", type, iosize, maxAsync)
        , m_useChromaticities(useChromaticies)
    {
        if (format == "RGB8")
            m_format = RGB8;
        else if (format == "RGBA8")
            m_format = RGBA8;
        else if (format == "RGB16")
            m_format = RGB16;
        else if (format == "RGBA16")
            m_format = RGBA16;
        else if (format == "RGB8_PLANAR")
            m_format = RGB8_PLANAR;
        else if (format == "RGB16_PLANAR")
            m_format = RGB16_PLANAR;
        else if (format == "RGB10")
            m_format = RGB10_A2;
        else if (format == "RGB10_A2")
            m_format = RGB10_A2;
        else if (format == "A2_BGR10")
            m_format = A2_BGR10;
        else
            m_format = RGB8;
        init();
    }

    bool IOcin::getBoolAttribute(const std::string& name) const
    {
        if (name == "useChromaticies")
            return m_useChromaticities;
        return StreamingFrameBufferIO::getBoolAttribute(name);
    }

    void IOcin::setBoolAttribute(const std::string& name, bool value)
    {
        if (name == "useChromaticies")
            m_useChromaticities = value;
        StreamingFrameBufferIO::setBoolAttribute(name, value);
    }

    std::string IOcin::getStringAttribute(const std::string& name) const
    {
        if (name == "format")
        {
            switch (m_format)
            {
            case RGB8:
                return "RGB8";
            case RGB16:
                return "RGB16";
            case RGBA8:
                return "RGBA8";
            case RGBA16:
                return "RGBA16";
            case RGB10_A2:
                return "RGB10_A2";
            case A2_BGR10:
                return "A2_BGR10";
            case RGB8_PLANAR:
                return "RGB8_PLANAR";
            case RGB16_PLANAR:
                return "RGB16_PLANAR";
            }
        }

        return StreamingFrameBufferIO::getStringAttribute(name);
    }

    void IOcin::setStringAttribute(const std::string& name,
                                   const std::string& value)
    {
        if (name == "format")
        {
            if (value == "RGB8")
                m_format = RGB8;
            else if (value == "RGB16")
                m_format = RGB16;
            else if (value == "RGBA8")
                m_format = RGBA8;
            else if (value == "RGBA16")
                m_format = RGBA16;
            else if (value == "RGB10_A2")
                m_format = RGB10_A2;
            else if (value == "A2_BGR10")
                m_format = A2_BGR10;
            else if (value == "RGB8_PLANAR")
                m_format = RGB8_PLANAR;
            else if (value == "RGB16_PLANAR")
                m_format = RGB16_PLANAR;

            return;
        }

        return StreamingFrameBufferIO::setStringAttribute(name, value);
    }

    void IOcin::init()
    {
        unsigned int cap = ImageWrite | ImageRead | BruteForceIO;
        addType("cin", "Kodak Cineon Digital Film Image", cap);
        addType("cineon", "Kodak Cineon Digital Film Image", cap);
    }

    IOcin::~IOcin() {}

    string IOcin::about() const { return "Cineon (Tweak)"; }

    void IOcin::readAttrs(FrameBuffer& fb, FileInformation& genericHeader,
                          ImageInformation& infoHeader,
                          DataFormatInformation& dataFormatHeader,
                          ImageOriginInformation& originHeader,
                          FilmInformation& filmHeader) const
    {
        union InfStruct
        {
            float f;
            unsigned int i;
        };

        InfStruct infs;
        infs.i = 0x7f800000;
        float fundef = infs.f;
        unsigned char bundef = 255;
        unsigned int iundef = 0xffffffff;

        Vec2f vundef(fundef, fundef);
        Vec2f red(infoHeader.red_X, infoHeader.red_Y);
        Vec2f green(infoHeader.green_X, infoHeader.green_Y);
        Vec2f blue(infoHeader.blue_X, infoHeader.blue_Y);
        Vec2f white(infoHeader.white_X, infoHeader.white_Y);

        //
        //  Color Space
        //
        //  For Cineon we will assume Log.  Not so for DPX.
        //

        fb.setPrimaryColorSpace(ColorSpace::Rec709());
        fb.setTransferFunction(ColorSpace::CineonLog());

        if (red != vundef && green != vundef && blue != vundef
            && white != vundef)
        {
            if (m_useChromaticities)
            {
                fb.setPrimaries(white.x, white.y, red.x, red.y, green.x,
                                green.y, blue.x, blue.y);
            }
            else
            {
                fb.attribute<Vec2f>("CIN/Chromaticities/white") = white;
                fb.attribute<Vec2f>("CIN/Chromaticities/blue") = blue;
                fb.attribute<Vec2f>("CIN/Chromaticities/green") = green;
                fb.attribute<Vec2f>("CIN/Chromaticities/red") = red;
            }
        }

        string inputDevice;
        inputDevice += originHeader.input_device;
        inputDevice += " ";
        inputDevice += originHeader.input_device_model;
        inputDevice += " ";
        inputDevice += originHeader.input_device_serial;

        Vec2f pitch(originHeader.input_device_X_pitch,
                    originHeader.input_device_Y_pitch);

        if (*filmHeader.unknown7 && strlen(filmHeader.unknown7) < 200)
        {
            {
                fb.newAttribute("CIN-Film/Field11",
                                string(filmHeader.unknown7));
            }
        }

        if (*filmHeader.unknown6 && strlen(filmHeader.unknown6) < 32)
        {
            fb.newAttribute("CIN-Film/Field10", string(filmHeader.unknown6));
        }

        if (*filmHeader.unknown3 && strlen(filmHeader.unknown3) < 32)
        {
            fb.newAttribute("CIN-Film/Field7", string(filmHeader.unknown3));
        }

        if (filmHeader.prefix != bundef)
            fb.newAttribute("CIN-Film/Prefix", int(filmHeader.prefix));
        if (filmHeader.offset != bundef)
            fb.newAttribute("CIN-Film/Offset", int(filmHeader.offset));
        if (filmHeader.film_type != bundef)
            fb.newAttribute("CIN-Film/Type", int(filmHeader.film_type));
        if (filmHeader.film_mfg_id != bundef)
            fb.newAttribute("CIN-Film/ManufacturerID",
                            int(filmHeader.film_mfg_id));

        if (dataFormatHeader.data_sign != bundef)
            fb.newAttribute("CIN-Format/DataSign",
                            int(dataFormatHeader.data_sign));
        if (dataFormatHeader.image_sense != bundef)
            fb.newAttribute("CIN-Format/Sense",
                            int(dataFormatHeader.image_sense));

        if (originHeader.input_gamma != fundef)
            fb.newAttribute("CIN-Origin/Gamma",
                            float(originHeader.input_gamma));
        if (pitch != vundef)
            fb.newAttribute("CIN-Origin/Pitch", pitch);

        if (inputDevice != "")
            fb.newAttribute("CIN-Origin/Device", inputDevice);
        fb.newAttribute("CIN-Origin/CreationTime",
                        string(originHeader.creation_time));
        fb.newAttribute("CIN-Origin/CreationDate",
                        string(originHeader.creation_date));
        fb.newAttribute("CIN-Origin/filename", string(originHeader.filename));
        if (*infoHeader.label)
            fb.newAttribute("CIN/Label", string(infoHeader.label));

        const char* ostring =
            infoHeader.orientation < 8
                ? orientationNames[infoHeader.orientation].name
                : (const char*)0;

        if (ostring == 0)
        {
            ostringstream str;
            str << "Undefined (" << infoHeader.orientation << ")";
        }
        else
        {
            fb.newAttribute("CIN/Orientation", string(ostring));
        }

        fb.newAttribute("CIN/CreationTime", string(genericHeader.create_time));
        fb.newAttribute("CIN/CreationDate", string(genericHeader.create_date));
        fb.newAttribute("CIN/Version", string(genericHeader.vers));

        fb.newAttribute("ChannelsInFile", int(infoHeader.num_channels));
    }

    void IOcin::getImageInfo(const std::string& filename, FBInfo& fbi) const
    {
        const unsigned char* data = 0;
        FileInformation genericHeader;
        ImageInformation infoHeader;
        DataFormatInformation dataFormatHeader;
        ImageOriginInformation originHeader;
        FilmInformation filmHeader;

        //
        //  Memory map is faster/more efficient (since we only read the bytes we
        //  want for the header), but some filesystems don't support it so fall
        //  back to Buffered reads in that case.
        //

        try
        {
            FileStream fmap(filename, FileStream::MemoryMap, m_iosize,
                            m_iomaxAsync);
            data = (const unsigned char*)fmap.data();

            memcpy((char*)&genericHeader, data, sizeof(FileInformation));
            data += sizeof(FileInformation);
            memcpy((char*)&infoHeader, data, sizeof(ImageInformation));
            data += sizeof(ImageInformation);
            memcpy((char*)&dataFormatHeader, data,
                   sizeof(DataFormatInformation));
            data += sizeof(DataFormatInformation);
            memcpy((char*)&originHeader, data, sizeof(ImageOriginInformation));
            data += sizeof(ImageOriginInformation);
            memcpy((char*)&filmHeader, data, sizeof(FilmInformation));
            data += sizeof(FilmInformation);
        }
        catch (...)
        {
            FileStream fmap(filename, FileStream::Buffering, m_iosize,
                            m_iomaxAsync);
            data = (const unsigned char*)fmap.data();

            memcpy((char*)&genericHeader, data, sizeof(FileInformation));
            data += sizeof(FileInformation);
            memcpy((char*)&infoHeader, data, sizeof(ImageInformation));
            data += sizeof(ImageInformation);
            memcpy((char*)&dataFormatHeader, data,
                   sizeof(DataFormatInformation));
            data += sizeof(DataFormatInformation);
            memcpy((char*)&originHeader, data, sizeof(ImageOriginInformation));
            data += sizeof(ImageOriginInformation);
            memcpy((char*)&filmHeader, data, sizeof(FilmInformation));
            data += sizeof(FilmInformation);
        }

#if defined(__LITTLE_ENDIAN__) || defined(TWK_LITTLE_ENDIAN)
        genericHeader.swap();
        infoHeader.swap();
        dataFormatHeader.swap();
        originHeader.swap();
        filmHeader.swap();
#endif

        if (genericHeader.magic_num != 0x802A5FD7)
        {
            TWK_THROW_STREAM(IOException, "CIN: cannot open " << filename);
        }

        fbi.numChannels = infoHeader.num_channels;
        fbi.width = infoHeader.image_element[0].pixels_per_line;
        fbi.height = infoHeader.image_element[0].lines_per_image;

        FrameBuffer::Orientation orient = FrameBuffer::TOPLEFT;

        switch (infoHeader.orientation)
        {
        case CIN_ROTATED_TOP_RIGHT:
        case CIN_TOP_RIGHT:
            orient = FrameBuffer::TOPRIGHT;
            break;
        case CIN_ROTATED_BOTTOM_RIGHT:
        case CIN_BOTTOM_RIGHT:
            orient = FrameBuffer::BOTTOMRIGHT;
            break;
        case CIN_ROTATED_TOP_LEFT:
        case CIN_TOP_LEFT:
            orient = FrameBuffer::TOPLEFT;
            break;
        case CIN_ROTATED_BOTTOM_LEFT:
        case CIN_BOTTOM_LEFT:
            orient = FrameBuffer::BOTTOMLEFT;
            break;
        }

        fbi.orientation = orient;

        switch (m_format)
        {
        default:
        case RGB8:
        case RGB8_PLANAR:
        case RGBA8:
            fbi.dataType = FrameBuffer::UCHAR;
            break;
        case RGB16:
        case RGB16_PLANAR:
        case RGBA16:
            fbi.dataType = FrameBuffer::USHORT;
            break;
        case RGB10_A2:
            fbi.dataType = FrameBuffer::PACKED_R10_G10_B10_X2;
            fbi.numChannels = 1;
            break;
        case A2_BGR10:
            fbi.dataType = FrameBuffer::PACKED_X2_B10_G10_R10;
            fbi.numChannels = 1;
            break;
        }

        readAttrs(fbi.proxy, genericHeader, infoHeader, dataFormatHeader,
                  originHeader, filmHeader);
    }

    void IOcin::readImage(FrameBuffer& fb, const std::string& filename,
                          const ReadRequest& request) const
    {
        FileStream::Type ftype = m_iotype == StandardIO
                                     ? FileStream::Buffering
                                     : (FileStream::Type)(m_iotype - 1);

        FileStream fmap(filename, ftype, m_iosize, m_iomaxAsync);

        readImage(fmap, fb, filename, request);
    }

    void IOcin::readImage(FileStream& fmap, FrameBuffer& fb,
                          const std::string& filename,
                          const ReadRequest& request) const
    {
        const unsigned char* data = (const unsigned char*)fmap.data();

        FileInformation genericHeader;
        ImageInformation infoHeader;
        DataFormatInformation dataFormatHeader;
        ImageOriginInformation originHeader;
        FilmInformation filmHeader;

        memcpy((char*)&genericHeader, data, sizeof(FileInformation));
        data += sizeof(FileInformation);

        memcpy((char*)&infoHeader, data, sizeof(ImageInformation));
        data += sizeof(ImageInformation);

        memcpy((char*)&dataFormatHeader, data, sizeof(DataFormatInformation));
        data += sizeof(DataFormatInformation);

        memcpy((char*)&originHeader, data, sizeof(ImageOriginInformation));
        data += sizeof(ImageOriginInformation);

        memcpy((char*)&filmHeader, data, sizeof(FilmInformation));
        data += sizeof(FilmInformation);

#if defined(__LITTLE_ENDIAN__) || defined(TWK_LITTLE_ENDIAN)
        bool swap = true;
#else
        bool swap = false;
#endif

        if (swap)
        {
            genericHeader.swap();
            infoHeader.swap();
            dataFormatHeader.swap();
            originHeader.swap();
            filmHeader.swap();
        }

        data = genericHeader.offset + (unsigned char*)fmap.data();
        const int w = infoHeader.image_element[0].pixels_per_line;
        const int h = infoHeader.image_element[0].lines_per_image;

        size_t headerSizes = (char*)data - (char*)fmap.data();
        size_t maxData = fmap.size() - headerSizes;
        if (maxData == 0)
        {
            TWK_THROW_STREAM(IOException, "CIN: file truncated: " << filename);
        }

        switch (m_format)
        {
        default:
        case RGB8:
            Read10Bit::readRGB8(filename, data, fb, w, h, maxData, swap);
            break;
        case RGB16:
            Read10Bit::readRGB16(filename, data, fb, w, h, maxData, swap);
            break;
        case RGBA8:
            Read10Bit::readRGBA8(filename, data, fb, w, h, maxData, false,
                                 swap);
            break;
        case RGBA16:
            Read10Bit::readRGBA16(filename, data, fb, w, h, maxData, false,
                                  swap);
            break;
        case RGB10_A2:
            Read10Bit::readRGB10_A2(filename, data, fb, w, h, maxData, swap);
            break;
        case A2_BGR10:
            Read10Bit::readA2_BGR10(filename, data, fb, w, h, maxData, swap);
            break;
        case RGB8_PLANAR:
            Read10Bit::readRGB8_PLANAR(filename, data, fb, w, h, maxData, swap);
            break;
        case RGB16_PLANAR:
            Read10Bit::readRGB16_PLANAR(filename, data, fb, w, h, maxData,
                                        swap);
            break;
        }

        if ((w * h * 4) > maxData)
        {
            fb.attribute<float>("PartialImage") = 1.0;
        }

        switch (infoHeader.orientation)
        {
        case CIN_TOP_LEFT:
        case CIN_ROTATED_TOP_LEFT:
            break; // this is the default for CIN, the reading code all does
                   // this itself
        case CIN_BOTTOM_LEFT:
        case CIN_ROTATED_BOTTOM_LEFT:
            setFBOrientation(fb, FrameBuffer::NATURAL);
            break;
        case CIN_TOP_RIGHT:
        case CIN_ROTATED_TOP_RIGHT:
            setFBOrientation(fb, FrameBuffer::TOPRIGHT);
            break;
        case CIN_BOTTOM_RIGHT:
        case CIN_ROTATED_BOTTOM_RIGHT:
            setFBOrientation(fb, FrameBuffer::BOTTOMRIGHT);
            break;
        }

        readAttrs(fb, genericHeader, infoHeader, dataFormatHeader, originHeader,
                  filmHeader);
    }

    void IOcin::writeImage(const FrameBuffer& img, const std::string& filename,
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

        if (img.numChannels() != 4)
        {
            const FrameBuffer* fb = outfb;
            vector<string> mapping;
            mapping.push_back("R");
            mapping.push_back("G");
            mapping.push_back("B");
            mapping.push_back("A");
            outfb = channelMap(const_cast<FrameBuffer*>(outfb), mapping);
            if (fb != &img)
                delete fb;
        }

        if (outfb->dataType() != FrameBuffer::FLOAT)
        {
            const FrameBuffer* fb = outfb;
            outfb = copyConvert(outfb, FrameBuffer::FLOAT);
            if (fb != &img)
                delete fb;
        }

        //
        //  Flip and Flop to get in the right orientation. NOTE: cineon
        //  images have orientation, but we want to default to writing out
        //  a "common" cineon.
        //

        if (request.preferCommonFormat)
        {
            bool needflip = false;
            bool needflop = false;

            switch (outfb->orientation())
            {
            case FrameBuffer::TOPLEFT:
                needflip = true;
                break;
            case FrameBuffer::TOPRIGHT:
            case FrameBuffer::BOTTOMRIGHT:
                needflop = true;
                break;
            default:
                break;
            };

            if (needflop || needflip)
            {
                if (outfb == &img)
                    outfb = img.copy();
                if (needflop)
                    flop(outfb);
                if (needflip)
                    flip(outfb);
            }
        }

        //
        //  Don't do the lin->log
        //

        TwkImg::Img4f image(outfb->width(), outfb->height(),
                            outfb->pixels<Col4f>());

        TwkImg::CineonIff::write(&image, filename.c_str(), 0, 0, 0, false);

        if (outfb != &img)
            delete outfb;
    }

} //  End namespace TwkFB

#ifdef _MSC_VER
#undef uint16_t
#undef uint32_t
#undef restrict
#endif
