//******************************************************************************
// Copyright (c) 2001-2004 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <IOdpx/IOdpx.h>
#include <IOcin/IOcin.h>
#include <IOcin/Read10Bit.h>
#include <IOcin/Read16Bit.h>
#include <IOcin/Read8Bit.h>
#include <IOcin/Read12Bit.h>
#include <TwkFB/Exception.h>
#include <TwkFB/Operations.h>
#include <TwkMath/Iostream.h>
#include <TwkUtil/Interrupt.h>
#include <TwkUtil/StdioBuf.h>
#include <TwkUtil/File.h>
#include <TwkMath/Color.h>
#include <fstream>
#include <iostream>
#include <stl_ext/string_algo.h>
#include <string>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <TwkUtil/FileStream.h>
#ifdef _MSC_VER
#define snprintf _snprintf
#endif

namespace TwkFB
{
    using namespace std;
    using namespace TwkUtil;
    using namespace TwkMath;
    using namespace std;

    typedef IOdpx::U32 U32;

    static U32 timeCodeStringToU32TC(const string& s)
    {
        vector<string> tokens;
        stl_ext::tokenize(tokens, s, ":");

        U32 v = 0;

        for (size_t i = 0; i < tokens.size(); i++)
        {
            long l = strtol(tokens[i].c_str(), NULL, 16);
            v |= l << ((3 - i) * 8);
        }

        return v;
    }

    static string U32TCToTimeCodeString(U32 u)
    {
        //
        //  Total insanity.  An unsubstantiated rumor states that dpx header
        //  time codes are actually base 10.  IE each byte of the timecode
        //  (frames, seconds, minutes, hours) is represented as a 2 hexadecimal
        //  digits, where the digit that would usually be in the "16's"
        //  place should be considered to be in the "10's" place.  Hence
        //  the below.
        //
        //  The sole coroboration of this is that a dpx file we happened
        //  to have lying around called "blah.00019.dpx" had a time code
        //  of 0x01000019.
        //
        //

        char temp[80];

        sprintf(temp, "%02d:%02d:%02d:%02d",
                ((u >> 28) & 0xF) * 10 + ((u >> 24) & 0xF),
                ((u >> 20) & 0xF) * 10 + ((u >> 16) & 0xF),
                ((u >> 12) & 0xF) * 10 + ((u >> 8) & 0xF),
                ((u >> 4) & 0xF) * 10 + ((u >> 0) & 0xF));

        return temp;
    }

    static int U32TCByteToInt(U32 u)
    {
        return int((u & 0xf) + 10 * ((u & 0xf0) >> 4));
    }

    static int U32TCToFrames(U32 u, float fps)
    {
        int f = 0;

        for (size_t i = 0; i < 4; i++)
        {
            int sh = i * 8;
            int v = U32TCByteToInt((u & (0xff << sh)) >> sh);

            switch (i)
            {
            case 0:
                f += v;
                break;
            case 1:
                f += v * fps + .49999;
                break;
            case 2:
                f += 60 * v * fps + .49999;
                break;
            case 3:
                f += 60 * 60 * v * fps + .49999;
                break;
            }
        }

        return f;
    }

    static U32 framesToU32TC(int f, float fps)
    {
        U32 u = 0;
        int hours = 0;
        int mins = 0;
        int secs = 0;

        if (f >= 60 * 60 * fps)
        {
            hours = f / (60 * 60 * fps);
            f -= hours * 60 * 60 * fps;
        }

        if (f >= 60 * fps)
        {
            mins = f / (60 * fps);
            f -= mins * 60 * fps;
        }

        if (f >= fps)
        {
            secs = f / fps;
            f -= secs * fps;
        }

        //
        //  This is lame but WTF
        //

        char temp[80];

        sprintf(temp, "%02d:%02d:%02d:%02d", hours, mins, secs, f);

        return timeCodeStringToU32TC(temp);
    }

#define DPX_NO_DATA_U32 IOdpx::U32(0xffffffff)
#define DPX_NO_DATA_ASCII IOdpx::ASCII(0xff)
#define DPX_NO_DATA_U8 IOdpx::U8(0xff)

    //
    //  Float "undefined" val is a 32-bit NaN with 1's in all bits.
    //
    static IOdpx::U32 u32NAN = DPX_NO_DATA_U32;
#define DPX_NO_DATA_R32 (*((IOdpx::R32*)(&u32NAN)))

//
//  For shake-written dpx files.
//
#define DPX_NO_DATA_R32_CUTOFF IOdpx::R32(2.139e+09)

    static void setFBOrientation(FrameBuffer& infb,
                                 FrameBuffer::Orientation orientation)
    {
        for (FrameBuffer* fb = &infb; fb; fb = fb->nextPlane())
        {
            fb->setOrientation(orientation);
        }
    }

    static inline bool hasData(IOdpx::ASCII* p)
    {
        return *p && *p != DPX_NO_DATA_ASCII;
    }

    static inline bool hasData(IOdpx::R32 p)
    {
        return p == p && // spec requires undefined R32 = 0xffffffff (NaN)
               p < DPX_NO_DATA_R32_CUTOFF; // shake puts high values in R32
                                           // fields
    }

    static inline bool hasData(IOdpx::U32 p) { return p != DPX_NO_DATA_U32; }

    static inline bool hasData(IOdpx::U8 p) { return p != DPX_NO_DATA_U8; }

    static inline void fillNoData(IOdpx::ASCII* p, size_t n)
    {
        fill(p, p + n, DPX_NO_DATA_ASCII);
    }

    static inline void fillNoData(IOdpx::R32& p) { p = DPX_NO_DATA_R32; }

    static inline void fillNoData(IOdpx::U32& p) { p = DPX_NO_DATA_U32; }

    static void setField(IOdpx::ASCII* p, size_t n, const std::string& s)
    {
        fillNoData(p, n);
        snprintf(p, n, "%s", s.c_str());
    }

    struct NameValue
    {
        const char* name;
        unsigned int value;
    };

    static NameValue orientation[] = {
        {"Top Left", IOdpx::DPX_TOP_LEFT},
        {"Bottom Left", IOdpx::DPX_BOTTOM_LEFT},
        {"Top Right", IOdpx::DPX_TOP_RIGHT},
        {"Bottom Right", IOdpx::DPX_BOTTOM_RIGHT},
        {"Rotated Top Left", IOdpx::DPX_ROTATED_TOP_LEFT},
        {"Rotated Bottom Left", IOdpx::DPX_ROTATED_BOTTOM_LEFT},
        {"Rotated Top Right", IOdpx::DPX_ROTATED_TOP_RIGHT},
        {"Rotated Bottom Right", IOdpx::DPX_ROTATED_BOTTOM_RIGHT},
        {"", 0}};

    static NameValue colorspace[] = {
        {"Other (0)", IOdpx::DPX_USER_DEFINED_COLOR},
        {"Printing Density", IOdpx::DPX_PRINTING_DENSITY},
        {"Linear", IOdpx::DPX_LINEAR},
        {"Logarithmic", IOdpx::DPX_LOGARITHMIC},
        {"Unspecified Video", IOdpx::DPX_UNSPECIFIED_VIDEO},
        {"SMPTE 274M", IOdpx::DPX_SMPTE274M},
        {"ITU Rec-709", IOdpx::DPX_ITU_R709},
        {"ITU Rec-601 625 lines", IOdpx::DPX_ITU_R601_625L},
        {"ITU Rec-601 525 lines", IOdpx::DPX_ITU_R601_525L},
        {"NTSC", IOdpx::DPX_NTSC},
        {"PAL", IOdpx::DPX_PAL},
        {"Linear Depth", IOdpx::DPX_Z_LINEAR},
        {"Homogeneous Depth", IOdpx::DPX_Z_HOMOGENEOUS},
        {"", 0}};

    static NameValue packing[] = {{"Not Padded", IOdpx::DPX_PAD_NONE},
                                  {"LSB Padded", IOdpx::DPX_PAD_LSB_WORD},
                                  {"MSB Padded", IOdpx::DPX_PAD_MSB_WORD},
                                  {"", 0}};

    static NameValue pixelFormat[] = {
        {"Unknown Format", IOdpx::DPX_PIXEL_UNKNOWN},
        {"R", IOdpx::DPX_PIXEL_R},
        {"G", IOdpx::DPX_PIXEL_G},
        {"B", IOdpx::DPX_PIXEL_B},
        {"A", IOdpx::DPX_PIXEL_A},
        {"Y", IOdpx::DPX_PIXEL_Y},
        {"CbCr", IOdpx::DPX_PIXEL_CbCr},
        {"Z", IOdpx::DPX_PIXEL_Z},
        {"Composite Video", IOdpx::DPX_PIXEL_COMPOSITE_VIDEO},
        {"RGB", IOdpx::DPX_PIXEL_RGB},
        {"RGBA", IOdpx::DPX_PIXEL_RGBA},
        {"ABGR", IOdpx::DPX_PIXEL_ABGR},
        {"CbYCrY 4:2:2", IOdpx::DPX_PIXEL_CbYCrY_422},
        {"CbYACrYA 4:2:2:4", IOdpx::DPX_PIXEL_CbYACrYA_4224},
        {"CbYCr 4:4:4", IOdpx::DPX_PIXEL_CbYCr_444},
        {"CbYCrA 4:4:4:4", IOdpx::DPX_PIXEL_CbYCrA_4444},
        {"User Defined 2 Component", IOdpx::DPX_PIXEL_USER_2_COMPONENT},
        {"User Defined 3 Component", IOdpx::DPX_PIXEL_USER_3_COMPONENT},
        {"User Defined 4 Component", IOdpx::DPX_PIXEL_USER_4_COMPONENT},
        {"User Defined 5 Component", IOdpx::DPX_PIXEL_USER_5_COMPONENT},
        {"User Defined 6 Component", IOdpx::DPX_PIXEL_USER_6_COMPONENT},
        {"User Defined 7 Component", IOdpx::DPX_PIXEL_USER_7_COMPONENT},
        {"User Defined 8 Component", IOdpx::DPX_PIXEL_USER_8_COMPONENT},
        {"", 0}};

    static const char* nameForValue(const NameValue* ptr, unsigned int value)
    {
        for (const NameValue* nv = ptr; nv->name; nv++)
        {
            if (nv->value == value)
            {
                return nv->name;
            }
        }

        return "-Undefined-";
    }

#if 0
//----------------------------------------------------------------------
//      COMMENTED OUT

#define GET(T, p) \
    *(T*)p;       \
    p += sizeof(T)
#define GETSTRING(o, p, s)          \
    memcpy(o, p, sizeof(char) * s); \
    p += sizeof(char)

static const char* read(IOdpx::DPXFileInformation& h, const char* p)
{
    h.magic_num = GET(IOdpx::U32,p);
    h.offset    = GET(IOdpx::U32,p);

    GETSTRING(h.vers, p, 8);
    
    h.file_size      = GET(IOdpx::U32, p);
    h.ditto_key      = GET(IOdpx::U32, p);
    h.gen_hdr_size   = GET(IOdpx::U32, p);
    h.ind_hdr_size   = GET(IOdpx::U32, p);
    h.user_data_size = GET(IOdpx::U32, p);

    GETSTRING(h.file_name, p, 100);
    GETSTRING(h.create_time, p, 24);
    GETSTRING(h.creator, p, 100);
    GETSTRING(h.project, p, 200);
    GETSTRING(h.copyright, p, 200);

    h.key = GET(IOdpx::U32, p);

    GETSTRING(h.Reserved, p, 104);
    return p;
}

static const char* read(IOdpx::DPXImageInformation& h, const char* p)
{
    h.orientation         = GET(IOdpx::U16, p);
    h.element_number      = GET(IOdpx::U16, p);
    h.pixels_per_line     = GET(IOdpx::U32, p);
    h.lines_per_image_ele = GET(IOdpx::U32, p);

    for (size_t i = 0; i < 8; i++)
    {
        h.image_element[i].data_sign         = GET(IOdpx::U32, p);
        h.image_element[i].ref_low_data      = GET(IOdpx::U32, p);
        h.image_element[i].ref_low_quantity  = GET(IOdpx::R32, p);
        h.image_element[i].ref_high_data     = GET(IOdpx::U32, p);
        h.image_element[i].ref_high_quantity = GET(IOdpx::R32, p);
        h.image_element[i].descriptor        = GET(IOdpx::U8, p);
        h.image_element[i].transfer          = GET(IOdpx::U8, p);
        h.image_element[i].colorimetric      = GET(IOdpx::U8, p);
        h.image_element[i].bit_size          = GET(IOdpx::U8, p);
        h.image_element[i].packing           = GET(IOdpx::U16, p);
        h.image_element[i].encoding          = GET(IOdpx::U16, p);
        h.image_element[i].data_offset       = GET(IOdpx::U32, p);
        h.image_element[i].eol_padding       = GET(IOdpx::U32, p);
        h.image_element[i].eo_image_padding  = GET(IOdpx::U32, p);

        GETSTRING(h.image_element[i].description, p, 32);
    }

    GETSTRING(h.reserved, p, 52);
    return p;
}

static const char* read(IOdpx::DPXImageOrientation& h, const char* p)
{
    h.x_offset    = GET(IOdpx::U32, p);
    h.y_offset    = GET(IOdpx::U32, p);
    h.x_center    = GET(IOdpx::R32, p);
    h.y_center    = GET(IOdpx::R32, p);
    h.x_orig_size = GET(IOdpx::U32, p);
    h.y_orig_size = GET(IOdpx::U32, p);
    
    GETSTRING(h.file_name, p, 100);
    GETSTRING(h.creation_time, p, 24);
    GETSTRING(h.input_dev, p, 32);
    GETSTRING(h.input_serial, p, 32);

    h.border_XL      = GET(IOdpx::U16, p);
    h.border_XR      = GET(IOdpx::U16, p);
    h.border_YT      = GET(IOdpx::U16, p);
    h.border_YB      = GET(IOdpx::U16, p);
    h.pixel_aspect_H = GET(IOdpx::U32, p);
    h.pixel_aspect_V = GET(IOdpx::U32, p);

    GETSTRING(h.reserved, p, 28);
    return p;
}

static const char* read(IOdpx::DPXFilmHeader& h, const char* p)
{
    GETSTRING(h.film_mfg_id, p, 2);
    GETSTRING(h.film_type, p, 2);
    GETSTRING(h.offset, p, 2);
    GETSTRING(h.prefix, p, 6);
    GETSTRING(h.count, p, 2);
    GETSTRING(h.format, p, 32);
    
    h.frame_position = GET(IOdpx::U32, p);
    h.sequence_len   = GET(IOdpx::U32, p);
    h.held_count     = GET(IOdpx::U32, p);
    h.frame_rate     = GET(IOdpx::R32, p);
    h.shutter_angle  = GET(IOdpx::R32, p);

    GETSTRING(h.frame_id, p, 32);
    GETSTRING(h.slate_info, p, 100);
    GETSTRING(h.reserved, p, 56);
    return p;
}

static const char* read(IOdpx::DPXTelevisionHeader& h, const char* p)
{
    h.tim_code          = GET(IOdpx::U32, p);
    h.userBits          = GET(IOdpx::U32, p);
    h.interlace         = GET(IOdpx::U8, p);
    h.field_num         = GET(IOdpx::U8, p);
    h.video_signal      = GET(IOdpx::U8, p);
    h.unused            = GET(IOdpx::U8, p);
    h.hor_sample_rate   = GET(IOdpx::R32, p);
    h.ver_sample_rate   = GET(IOdpx::R32, p);
    h.frame_rate        = GET(IOdpx::R32, p);
    h.time_offset       = GET(IOdpx::R32, p);
    h.gamma             = GET(IOdpx::R32, p);
    h.black_level       = GET(IOdpx::R32, p);
    h.black_gain        = GET(IOdpx::R32, p);
    h.break_point       = GET(IOdpx::R32, p);
    h.white_level       = GET(IOdpx::R32, p);
    h.integration_times = GET(IOdpx::R32, p);

    GETSTRING(h.reserved, p, 76);
    return p;
}

static const char* read(IOdpx::DPXHeader& h, const char* p)
{
    p = read(h.file, p);
    p = read(h.image, p);
    p = read(h.orientation, p);
    p = read(h.film, p);
    p = read(h.tv, p);
    return p;
}

#endif
    //----------------------------------------------------------------------

    //----------------------------------------------------------------------

    IOdpx::IOdpx(StorageFormat format, bool useChromaticies, IOType type,
                 size_t chunksize, int maxAsync)
        : StreamingFrameBufferIO("IOdpx", "m5", type, chunksize, maxAsync)
        , m_format(format)
        , m_useChromaticities(useChromaticies)
    {
        init();
    }

    IOdpx::IOdpx(const std::string& format, bool useChromaticies, IOType type,
                 size_t chunksize, int maxAsync)
        : StreamingFrameBufferIO("IOdpx", "m5", type, chunksize, maxAsync)
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

    void IOdpx::init()
    {
        unsigned int cap = ImageWrite | ImageRead | BruteForceIO;

        StringPairVector codecs;
        StringPairVector eparams;
        StringPairVector dparams;

        codecs.push_back(StringPair("NONE", "uncompressed"));

        eparams.push_back(StringPair(
            "transfer",
            "Transfer function (LOG, DENSITY, REC709, USER, VIDEO, SMPTE274M, "
            "REC601-625, REC601-525, NTSC, PAL, or number)"));
        eparams.push_back(StringPair(
            "colorimetric",
            "Colorimetric specification (REC709, USER, VIDEO, SMPTE274M, "
            "REC601-625, REC601-525, NTSC, PAL, or number)"));
        eparams.push_back(StringPair("creator", "ASCII string"));
        eparams.push_back(StringPair("copyright", "ASCII string"));
        eparams.push_back(StringPair("project", "ASCII string"));
        eparams.push_back(StringPair(
            "orientation",
            "Pixel Origin string or int (TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, "
            "BOTTOM_RIGHT, ROTATED_TOP_LEFT, ROTATED_TOP_RIGHT, "
            "ROTATED_BOTTOM_LEFT, ROTATED_BOTTOM_RIGHT)"));
        eparams.push_back(StringPair(
            "create_time", "ISO 8601 ASCII string: YYYY:MM:DD:hh:mm:ssTZ "));
        eparams.push_back(
            StringPair("film/mfg_id", "2 digit manufacturer ID edge code"));
        eparams.push_back(
            StringPair("film/type", "2 digit film type edge code"));
        eparams.push_back(StringPair("film/offset",
                                     "2 digit film offset in perfs edge code"));
        eparams.push_back(
            StringPair("film/prefix", "6 digit film prefix edge code"));
        eparams.push_back(
            StringPair("film/count", "4 digit film count edge code"));
        eparams.push_back(
            StringPair("film/format", "32 char film format (e.g. Academy)"));
        eparams.push_back(
            StringPair("film/frame_position", "Frame position in sequence"));
        eparams.push_back(StringPair("film/sequence_len", "Sequence length"));
        eparams.push_back(
            StringPair("film/frame_rate", "Frame rate (frames per second)"));
        eparams.push_back(
            StringPair("film/shutter_angle", "Shutter angle in degrees"));
        eparams.push_back(
            StringPair("film/frame_id", "32 character frame identification"));
        eparams.push_back(
            StringPair("film/slate_info", "100 character slate info"));
        eparams.push_back(
            StringPair("tv/time_code", "SMPTE time code as XX:XX:XX:XX"));
        eparams.push_back(
            StringPair("tv/user_bits", "SMPTE user bits as XX:XX:XX:XX"));
        eparams.push_back(
            StringPair("tv/interlace", "Interlace (0=no, 1=2:1)"));
        eparams.push_back(StringPair("tv/field_num", "Field number"));
        eparams.push_back(StringPair(
            "tv/video_signal", "Video signal standard 0-254 (see DPX spec)"));
        eparams.push_back(StringPair("tv/horizontal_sample_rate",
                                     "Horizontal sampling rate in Hz"));
        eparams.push_back(StringPair("tv/vertical_sample_rate",
                                     "Vertical sampling rate in Hz"));
        eparams.push_back(StringPair(
            "tv/frame_rate", "Temporal sampling rate or frame rate in Hz"));
        eparams.push_back(StringPair(
            "tv/time_offset", "Time offset from sync to first pixel in ms"));
        eparams.push_back(StringPair("tv/gamma", "Gamma"));
        eparams.push_back(StringPair("tv/black_level", "Black level"));
        eparams.push_back(StringPair("tv/black_gain", "Black gain"));
        eparams.push_back(StringPair("tv/break_point", "Breakpoint"));
        eparams.push_back(StringPair("tv/white_level", "White level"));
        eparams.push_back(
            StringPair("tv/integration_times", "Integration times"));
        eparams.push_back(StringPair("source/x_offset", "X offset"));
        eparams.push_back(StringPair("source/y_offset", "X offset"));
        eparams.push_back(StringPair("source/x_center", "X center"));
        eparams.push_back(StringPair("source/y_center", "Y center"));
        eparams.push_back(
            StringPair("source/x_original_size", "X original size"));
        eparams.push_back(
            StringPair("source/y_original_size", "Y original size"));
        eparams.push_back(StringPair("source/file_name", "Source file name"));
        eparams.push_back(
            StringPair("source/creation_time",
                       "Source creation time YYYY:MM:DD:hh:mm:ssTZ"));
        eparams.push_back(StringPair("source/input_dev", "Input device name"));
        eparams.push_back(
            StringPair("source/input_dev", "Input device serial number"));
        eparams.push_back(
            StringPair("source/border_XL", "Border validity left"));
        eparams.push_back(
            StringPair("source/border_XR", "Border validity right"));
        eparams.push_back(
            StringPair("source/border_YT", "Border validity top"));
        eparams.push_back(
            StringPair("source/border_YB", "Border validity bottom"));
        eparams.push_back(StringPair("source/pixel_aspect_H",
                                     "Pixel aspect ratio horizonal component"));
        eparams.push_back(StringPair("source/pixel_aspect_V",
                                     "Pixel aspect ratio vertical component"));

        addType("dpx", "Digital Picture Exchange Image", cap, codecs, eparams,
                dparams);
    }

    bool IOdpx::getBoolAttribute(const std::string& name) const
    {
        if (name == "useChromaticies")
            return m_useChromaticities;
        return StreamingFrameBufferIO::getBoolAttribute(name);
    }

    void IOdpx::setBoolAttribute(const std::string& name, bool value)
    {
        if (name == "useChromaticies")
            m_useChromaticities = value;
        StreamingFrameBufferIO::setBoolAttribute(name, value);
    }

    std::string IOdpx::getStringAttribute(const std::string& name) const
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

    void IOdpx::setStringAttribute(const std::string& name,
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

    IOdpx::~IOdpx() {}

    string IOdpx::about() const { return "DPX (Tweak)"; }

    static void setAttr(FrameBuffer& fb, const char* name, char* value,
                        size_t size)
    {
        if (hasData(value))
        {
            vector<char> buffer(size + 1);

            // I have implemented the functional equivalence of
            // snprintf(), to avoid out-of-bounds reads should
            // "value" not be null terminated. There is
            // an assumption by snprintf that it is.
            // There is still the likelihood of an out of bounds read
            // if "value" has legit data and is less than size bytes
            // without being null terminated.
            //
            // snprintf(&buffer.front(), size, "%s", value);
            //
            for (size_t i = 0; i < size; i++)
            {
                if ((int)value[i] < 0 || (int)value[i] >= 127)
                    buffer[i] = '?';
                else
                    buffer[i] = value[i];

                if (value[i] == '\0')
                    break;
            }
            buffer.back() = 0;
            fb.newAttribute(name, string(&buffer.front()));
        }
    }

    void IOdpx::readAttrs(FrameBuffer& fb, DPXHeader& header, int e) const
    {
        if (header.file.user_data_size)
        {
            fb.attribute<int>("DPX/userDataSize") =
                int(header.file.user_data_size);
        }

        if (hasData(header.orientation.pixel_aspect_H)
            && hasData(header.orientation.pixel_aspect_V)
            && header.orientation.pixel_aspect_H != 0
            && header.orientation.pixel_aspect_V != 0)
        {
            fb.setPixelAspectRatio(
                float(double(header.orientation.pixel_aspect_H)
                      / double(header.orientation.pixel_aspect_V)));
        }
        else
        {
            fb.setPixelAspectRatio(1.0);
        }

        //
        //
        //

        //
        //  Color Space
        //
        //  If a file is specifically tagged, we'll use it.
        //  Otherwise, leave it to the user.
        //
        //  We just check the first image element.
        //

        U32 fmt = header.image.image_element[e].descriptor;
        bool packedYUV = fmt == IOdpx::DPX_PIXEL_CbYCrY_422;
        const U8 transfer = header.image.image_element[e].transfer;
        const U8 colorimetric = header.image.image_element[e].colorimetric;

        if (packedYUV)
        {
        }

        if (colorimetric == DPX_SMPTE274M || colorimetric == DPX_ITU_R709
            || colorimetric == DPX_ITU_R601_625L
            || colorimetric == DPX_ITU_R601_525L || colorimetric == DPX_NTSC
            || colorimetric == DPX_PAL)
        {
            fb.setPrimaryColorSpace(ColorSpace::Rec709());
        }

        if (packedYUV)
        {
            if (colorimetric == DPX_ITU_R709)
            {
                fb.setConversion(ColorSpace::Rec709());
            }
            else if (colorimetric == DPX_ITU_R601_625L
                     || colorimetric == DPX_ITU_R601_525L
                     || colorimetric == DPX_NTSC || colorimetric == DPX_PAL)
            {
                fb.setConversion(ColorSpace::Rec601());
            }
            else
            {
                fb.setConversion(ColorSpace::Rec601());
            }
        }

        if (transfer == DPX_PRINTING_DENSITY || transfer == DPX_LOGARITHMIC)
        {
            fb.setTransferFunction(ColorSpace::CineonLog());

            if (header.tv.white_level != 0 && hasData(header.tv.white_level))
            {
                fb.attribute<float>(ColorSpace::WhitePoint()) =
                    header.tv.white_level;
                fb.attribute<float>(ColorSpace::BlackPoint()) =
                    header.tv.black_level;
            }

            //
            // Cineon specs the max softClip (whitept - breakpt) at 50.
            //
            if (header.tv.break_point != 0 && hasData(header.tv.break_point)
                && (header.tv.white_level - header.tv.break_point) <= 50.0f)
            {
                fb.attribute<float>(ColorSpace::BreakPoint()) =
                    header.tv.break_point;
            }

            if (hasData(header.tv.gamma) && header.tv.gamma != 0.0)
            {
                fb.attribute<float>(ColorSpace::Gamma()) = header.tv.gamma;
            }
        }
        else if (transfer == DPX_SMPTE274M || transfer == DPX_ITU_R709)
        {
            fb.setTransferFunction(ColorSpace::Rec709());
        }
        else if (transfer == DPX_ITU_R601_625L || transfer == DPX_ITU_R601_525L
                 || transfer == DPX_NTSC || transfer == DPX_PAL)
        {
            fb.setTransferFunction(ColorSpace::Rec601());
        }

        char temp[128];

        if (header.tv.interlace == 0)
        {
            fb.newAttribute("DPX-TV/Interlace", string("Noninterlaced"));
        }
        else if (header.tv.interlace == 1)
        {
            fb.newAttribute("DPX-TV/Interlace", string("2:1"));
        }
        else if (header.tv.interlace != 255)
        {
            fb.newAttribute("DPX-TV/Interlace", int(header.tv.interlace));
        }

        if (hasData(header.tv.integration_times))
            fb.newAttribute("DPX-TV/IntegrationTimes",
                            (header.tv.integration_times));
        if (hasData(header.tv.white_level))
            fb.newAttribute("DPX-TV/WhiteLevel", (header.tv.white_level));
        if (hasData(header.tv.break_point))
            fb.newAttribute("DPX-TV/BreakPoint", (header.tv.break_point));
        if (hasData(header.tv.black_gain))
            fb.newAttribute("DPX-TV/BlackGain", (header.tv.black_gain));
        if (hasData(header.tv.black_level))
            fb.newAttribute("DPX-TV/BlackLevel", (header.tv.black_level));
        if (hasData(header.tv.gamma))
            fb.newAttribute("DPX-TV/Gamma", (header.tv.gamma));
        if (hasData(header.tv.time_offset))
            fb.newAttribute("DPX-TV/TimeOffset", (header.tv.time_offset));
        if (hasData(header.tv.frame_rate))
            fb.newAttribute("DPX-TV/FrameRate", (header.tv.frame_rate));
        if (hasData(header.tv.ver_sample_rate))
            fb.newAttribute("DPX-TV/VeritcalSampleRate",
                            (header.tv.ver_sample_rate));
        if (hasData(header.tv.hor_sample_rate))
            fb.newAttribute("DPX-TV/HorizontalSampleRate",
                            (header.tv.hor_sample_rate));

        if (header.tv.video_signal != 255)
            fb.newAttribute("DPX-TV/VideoSignal", int(header.tv.video_signal));
        if (header.tv.field_num != 255)
            fb.newAttribute("DPX-TV/FieldNumber", int(header.tv.field_num));

        if (hasData(header.tv.tim_code))
        {
            fb.newAttribute("DPX-TV/TimeCode",
                            U32TCToTimeCodeString(header.tv.tim_code));
        }

        if (hasData(header.tv.userBits))
        {
            fb.newAttribute("DPX-TV/UserBits",
                            U32TCToTimeCodeString(header.tv.userBits));
        }

        if (hasData(header.film.sequence_len))
        {
            fb.newAttribute("DPX-MP/SequenceLength",
                            int(header.film.sequence_len));
        }

        if (hasData(header.film.frame_position))
        {
            fb.newAttribute("DPX-MP/FramePosition",
                            int(header.film.frame_position));
        }

        setAttr(fb, "DPX-MP/Format", header.film.format, 32);
        setAttr(fb, "DPX-MP/FrameId", header.film.frame_id, 32);
        setAttr(fb, "DPX-MP/Slate", header.film.slate_info, 100);

        if (hasData(header.film.shutter_angle))
            fb.newAttribute("DPX-MP/ShutterAngle", header.film.shutter_angle);
        if (hasData(header.film.frame_rate))
            fb.newAttribute("DPX-MP/FrameRate", header.film.frame_rate);
        if (hasData(header.film.held_count))
            fb.newAttribute("DPX-MP/HeldCount", header.film.held_count);

        if (hasData(header.film.count))
        {
            sprintf(temp, "%c%c%c%c", header.film.count[0],
                    header.film.count[1], header.film.count[2],
                    header.film.count[3]);

            fb.newAttribute("DPX-MP/Count", string(temp));
        }

        if (hasData(header.film.prefix))
        {
            sprintf(temp, "%c%c%c%c%c%c", header.film.prefix[0],
                    header.film.prefix[1], header.film.prefix[2],
                    header.film.prefix[3], header.film.prefix[4],
                    header.film.prefix[5]);

            fb.newAttribute("DPX-MP/Prefix", string(temp));
        }

        if (hasData(header.film.offset))
        {
            sprintf(temp, "%c%c", header.film.offset[0], header.film.offset[1]);

            fb.newAttribute("DPX-MP/Offset", string(temp));
        }

        if (hasData(header.film.film_type))
        {
            sprintf(temp, "%c%c", header.film.film_type[0],
                    header.film.film_type[1]);

            fb.newAttribute("DPX-MP/FilmType", string(temp));
        }

        if (hasData(header.film.film_mfg_id))
        {
            sprintf(temp, "%c%c", header.film.film_mfg_id[0],
                    header.film.film_mfg_id[1]);

            fb.newAttribute("DPX-MP/ManufacturerId", string(temp));
        }

        for (int i = 0; i < header.image.element_number; i++)
        {
            sprintf(temp, "DPX-%d/Packing", i);
            fb.newAttribute(
                temp, string(nameForValue(
                          packing, header.image.image_element[i].packing)));

            if (header.image.image_element[i].encoding)
                fb.newAttribute("DPX/Encoding",
                                int(header.image.image_element[i].encoding));

            fb.newAttribute("DPX/BitSize",
                            int(header.image.image_element[i].bit_size));

            sprintf(temp, "DPX-%d/Transfer", i);
            fb.newAttribute(
                temp, string(nameForValue(
                          colorspace, header.image.image_element[i].transfer)));
            sprintf(temp, "DPX-%d/Colorimetric", i);
            fb.newAttribute(
                temp,
                string(nameForValue(
                    colorspace, header.image.image_element[i].colorimetric)));

            if (header.image.image_element[i].eol_padding > 0)
            {
                sprintf(temp, "DPX-%d/EndOfLinePadding", i);
                fb.newAttribute(temp,
                                int(header.image.image_element[i].eol_padding));
            }

            if (header.image.image_element[i].eo_image_padding > 0)
            {
                sprintf(temp, "DPX-%d/EndOfImagePadding", i);
                fb.newAttribute(
                    temp, int(header.image.image_element[i].eo_image_padding));
            }

            sprintf(temp, "DPX-%d/Descriptor", i);
            fb.newAttribute(
                temp,
                string(nameForValue(pixelFormat,
                                    header.image.image_element[i].descriptor)));

            if (hasData(header.image.image_element[i].description))
            {
                sprintf(temp, "DPX-%d/Description", i);
                fb.newAttribute(
                    temp, string(header.image.image_element[i].description));
            }
        }

        if (hasData(header.orientation.pixel_aspect_V))
        {
            fb.newAttribute("DPX-Source/VerticalPixelAspect",
                            header.orientation.pixel_aspect_V);
        }

        if (hasData(header.orientation.pixel_aspect_H))
        {
            fb.newAttribute("DPX-Source/HorizontalPixelAspect",
                            header.orientation.pixel_aspect_H);
        }

        fb.newAttribute(
            "DPX/Orientation",
            string(nameForValue(orientation, header.image.orientation)));

        setAttr(fb, "DPX-Source/InputSerialNumber",
                header.orientation.input_serial, 32);
        setAttr(fb, "DPX-Source/InputDevice", header.orientation.input_dev, 32);
        setAttr(fb, "DPX-Source/CreationTime", header.orientation.creation_time,
                24);
        setAttr(fb, "DPX-Source/FileName", header.orientation.file_name, 100);
        fb.newAttribute("DPX/DataOffset", int(header.file.offset));
        setAttr(fb, "DPX/Copyright", header.file.copyright, 200);
        setAttr(fb, "DPX/Creator", header.file.creator, 100);
        setAttr(fb, "DPX/Project", header.file.project, 200);
        setAttr(fb, "DPX/CreateTime", header.file.create_time, 24);
        setAttr(fb, "DPX/FileName", header.file.file_name, 100);
        setAttr(fb, "DPX/Version", header.file.vers, 8);

        int numChannels = 0;

        switch (header.image.image_element[0].descriptor)
        {
        case DPX_PIXEL_UNKNOWN:
            numChannels = 0;
            break;
        case DPX_PIXEL_R:
            numChannels = 1;
            break;
        case DPX_PIXEL_G:
            numChannels = 1;
            break;
        case DPX_PIXEL_B:
            numChannels = 1;
            break;
        case DPX_PIXEL_A:
            numChannels = 1;
            break;
        case DPX_PIXEL_Y:
            numChannels = 1;
            break;
        case DPX_PIXEL_CbCr:
            numChannels = 2;
            break;
        case DPX_PIXEL_Z:
            numChannels = 1;
            break;
        case DPX_PIXEL_COMPOSITE_VIDEO:
            numChannels = 3;
            break;
        case DPX_PIXEL_RGB:
            numChannels = 3;
            break;
        case DPX_PIXEL_RGBA:
            numChannels = 4;
            break;
        case DPX_PIXEL_ABGR:
            numChannels = 4;
            break;
        case DPX_PIXEL_CbYCrY_422:
            numChannels = 3;
            break;
        case DPX_PIXEL_CbYACrYA_4224:
            numChannels = 3;
            break;
        case DPX_PIXEL_CbYCr_444:
            numChannels = 3;
            break;
        case DPX_PIXEL_CbYCrA_4444:
            numChannels = 3;
            break;
        case DPX_PIXEL_USER_2_COMPONENT:
            numChannels = 2;
            break;
        case DPX_PIXEL_USER_3_COMPONENT:
            numChannels = 3;
            break;
        case DPX_PIXEL_USER_4_COMPONENT:
            numChannels = 4;
            break;
        case DPX_PIXEL_USER_5_COMPONENT:
            numChannels = 5;
            break;
        case DPX_PIXEL_USER_6_COMPONENT:
            numChannels = 6;
            break;
        case DPX_PIXEL_USER_7_COMPONENT:
            numChannels = 7;
            break;
        case DPX_PIXEL_USER_8_COMPONENT:
            numChannels = 8;
            break;
        }

        fb.newAttribute("ChannelsInFile", numChannels);
    }

    void IOdpx::getImageInfo(const std::string& filename, FBInfo& fbi) const
    {
        const unsigned char* data = 0;
        DPXHeader header;

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
            memcpy((char*)&header, data, sizeof(DPXHeader));
        }
        catch (...)
        {
            FileStream fmap(filename, 0, sizeof(DPXHeader),
                            FileStream::Buffering, m_iosize, m_iomaxAsync);
            data = (const unsigned char*)fmap.data();
            memcpy((char*)&header, data, sizeof(DPXHeader));
        }

        bool swap = header.file.magic_num != 0x53445058;

        if (swap)
            header.swap();

        if (header.file.magic_num != 0x53445058)
        {
            if (header.file.magic_num == 0x802A5FD7)
            {
                //
                //  Photoshop can write out a "dpx" file that's actually a
                //  cineon file. So just pretend like it works
                //

                IOcin cin(IOcin::StorageFormat(m_format), m_useChromaticities,
                          IOcin::IOType(m_iotype), m_iosize, m_iomaxAsync);

                cin.getImageInfo(filename, fbi);
                return;
            }
            else
            {
                TWK_THROW_STREAM(IOException, "DPX: cannot open "
                                                  << filename
                                                  << " -- bad magic number");
            }
        }

        const int numImages = header.image.element_number;
        const int inputBits = int(header.image.image_element[0].bit_size);
        const U32 fmt = header.image.image_element[0].descriptor;
        const bool packedYUV = fmt == IOdpx::DPX_PIXEL_CbYCrY_422;
        const bool alpha = fmt == IOdpx::DPX_PIXEL_RGBA;
        const bool threeOrFourChannel = packedYUV
                                        || fmt == IOdpx::DPX_PIXEL_RGBA
                                        || fmt == IOdpx::DPX_PIXEL_RGB;

        if (numImages > 1)
        {
            for (size_t i = 0; i < numImages; i++)
            {
                ostringstream str;
                str << i << endl;
                fbi.views.push_back(str.str());
            }
        }

        if (inputBits == 10
            && header.image.image_element[0].packing == IOdpx::DPX_PAD_NONE)
        {
            //  Some writers just put 0s in header, but we can't read "unfilled"
            //  10bit pixels anyway, so try LSB Filling.

            cerr << "WARNING: DPX metadata specifies unaligned pixels: ignoring"
                 << endl;
            header.image.image_element[0].packing = IOdpx::DPX_PAD_LSB_WORD;
        }

        const U16 packing0 = header.image.image_element[0].packing;

        if (((inputBits == 10 || inputBits == 12)
             && packing0 == IOdpx::DPX_PAD_MSB_WORD)
            || inputBits == 32 || inputBits == 64 || !threeOrFourChannel)
        {
            TWK_THROW_STREAM(UnsupportedException,
                             "DPX: unsupported internal layout " << filename);
        }

        fbi.width = header.image.pixels_per_line;
        fbi.height = header.image.lines_per_image_ele;

        if (hasData(header.orientation.pixel_aspect_H)
            && hasData(header.orientation.pixel_aspect_V)
            && header.orientation.pixel_aspect_H != 0
            && header.orientation.pixel_aspect_V != 0)
        {
            fbi.pixelAspect = double(header.orientation.pixel_aspect_H)
                              / double(header.orientation.pixel_aspect_V);
        }
        else
        {
            fbi.pixelAspect = 1.0;
        }

        FrameBuffer::Orientation orient = FrameBuffer::TOPLEFT;

        switch (header.image.orientation)
        {
        case DPX_ROTATED_TOP_RIGHT:
        case DPX_TOP_RIGHT:
            orient = FrameBuffer::TOPRIGHT;
            break;
        case DPX_ROTATED_TOP_LEFT:
        case DPX_TOP_LEFT:
            orient = FrameBuffer::TOPLEFT;
            break;
        case DPX_ROTATED_BOTTOM_LEFT:
        case DPX_BOTTOM_LEFT:
            orient = FrameBuffer::BOTTOMLEFT;
            break;
        case DPX_ROTATED_BOTTOM_RIGHT:
        case DPX_BOTTOM_RIGHT:
            orient = FrameBuffer::BOTTOMRIGHT;
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
            break;
        case A2_BGR10:
            fbi.dataType = FrameBuffer::PACKED_X2_B10_G10_R10;
            break;
        }

        //
        //  Above switch handles 10- or 12-bit files, but if file is 8 or 16 bit
        //  we always read to that format.
        //
        if (inputBits == 8)
            fbi.dataType = FrameBuffer::UCHAR;
        if (inputBits == 16)
            fbi.dataType = FrameBuffer::USHORT;

        readAttrs(fbi.proxy, header, 0);
        fbi.numChannels = fbi.proxy.attribute<int>("ChannelsInFile");
    }

    void IOdpx::readImages(FrameBufferVector& fbs, const std::string& filename,
                           const ReadRequest& request) const
    {
        FileStream::Type ftype = m_iotype == StandardIO
                                     ? FileStream::Buffering
                                     : (FileStream::Type)(m_iotype - 1);

        FileStream fstream(filename, ftype, m_iosize, m_iomaxAsync);

        DPXHeader header;
        const unsigned char* data = (const unsigned char*)fstream.data();
        memcpy((char*)&header, data, sizeof(DPXHeader));

        bool swap = header.file.magic_num != 0x53445058;
        if (swap)
            header.swap();

        ostringstream readerComment;

        if (header.file.magic_num != 0x53445058)
        {
            if (header.file.magic_num == 0x802A5FD7)
            {
                //
                //  Its a Cineon file
                //

                IOcin cin(IOcin::StorageFormat(m_format), m_useChromaticities,
                          IOcin::IOType(m_iotype), m_iosize, m_iomaxAsync);

                fbs.push_back(new FrameBuffer());
                cin.readImage(fstream, *fbs.front(), filename, request);
                fbs.front()->attribute<string>("IOdpx/WARNING") =
                    "File is actually a Cineon file";
                return;
            }
            else
            {
                TWK_THROW_STREAM(IOException, "DPX: cannot open " << filename);
            }
        }

        int numImages = header.image.element_number;

        for (size_t i = 0; i < numImages; i++) // only first image to start with
        {
            if (request.views.empty() && i > 0)
                break;

            if (!request.views.empty())
            {
                bool found = false;

                for (size_t q = 0; q < request.views.size(); q++)
                {
                    size_t viewnum = atoi(request.views[q].c_str());
                    if (viewnum == i)
                        found = true;
                }

                if (!found)
                    break;
            }

            FrameBuffer* fbp = new FrameBuffer();
            FrameBuffer& fb = *fbp;
            fbs.push_back(fbp);

            size_t maxData = 0;

            if (fstream.size() > header.file.offset)
            {
                maxData = fstream.size() - header.file.offset;
            }

            if (maxData == 0)
            {
                TWK_THROW_STREAM(IOException,
                                 "DPX: file truncated: " << filename);
            }

            const int inputBits = int(header.image.image_element[i].bit_size);
            const U32 fmt = header.image.image_element[i].descriptor;
            const bool alpha = fmt == IOdpx::DPX_PIXEL_RGBA;
            const bool packedYUV = fmt == IOdpx::DPX_PIXEL_CbYCrY_422;
            const bool threeOrFourChannel = packedYUV
                                            || fmt == IOdpx::DPX_PIXEL_RGBA
                                            || fmt == IOdpx::DPX_PIXEL_RGB;
            const int w = header.image.pixels_per_line;
            const int h = header.image.lines_per_image_ele;
            U16 packing = header.image.image_element[i].packing;

            data = (unsigned char*)fstream.data()
                   + header.image.image_element[i].data_offset;

            bool badPadding = false;

            if (inputBits == 10 && packing == IOdpx::DPX_PAD_NONE
                && // DPX_PAD_NONE == 0
                maxData == (w * h * sizeof(U32)))
            {
                //
                //  Some writers just put 0s in header, and in this case this
                //  may be a mismarked header and in fact its LSB padded data
                //  (the most common) since the data size is consistent with
                //  that not the unpadded case.
                //

                cout << "WARNING: DPX: image claims no padding but data size "
                        "indicates otherwise, "
                     << "reading as if LSB padded" << endl;

                packing = (U16)IOdpx::DPX_PAD_LSB_WORD;
                readerComment << "Assumed LSB padding from data size" << endl;
            }

            if (((inputBits == 10 || inputBits == 12)
                 && packing == IOdpx::DPX_PAD_MSB_WORD)
                || inputBits == 32 || inputBits == 64 || !threeOrFourChannel)
            {
                TWK_THROW_STREAM(UnsupportedException,
                                 "DPX: unsupported internal layout "
                                     << filename);
            }

            //
            // RAW OPTIMIZATION:
            //
            // Should probably make this 4096 not 16 for alignment, but
            // the GL manuals imply 16 is ok. Its definitely not possibly
            // to use raw for MemoryMappedIO because the memory *will* be
            // unmapped no matter what. (It will core dump to remind you
            // of this).
            //

            const size_t alignment = size_t(data) & 0xFFF;
            const bool canUseRaw =
                size_t(data) % 16 == 0 && m_iotype != MemoryMappedIO;
            bool didUseRaw = false;

            bool readit = false;

            try
            {
                if (inputBits == 10)
                {
                    bool nonspec = false;

                    if (packedYUV)
                    {
                        Read10Bit::readYCrYCb8_422_PLANAR(filename, data, fb, w,
                                                          h, maxData, swap);
                        readit = true;
                    }
                    else
                    {
                        switch (m_format)
                        {
                        default:
                        case RGB8:
                        {
                            if (alpha)
                                Read10Bit::readRGBA8(filename, data, fb, w, h,
                                                     maxData, true, swap);
                            else
                                Read10Bit::readRGB8(filename, data, fb, w, h,
                                                    maxData, swap);
                            readit = true;
                            break;
                        }
                        case RGBA8:
                            Read10Bit::readRGBA8(filename, data, fb, w, h,
                                                 maxData, alpha, swap);
                            readit = true;
                            break;
                        case RGB16:
                        {
                            if (alpha)
                                Read10Bit::readRGBA16(filename, data, fb, w, h,
                                                      maxData, alpha, swap);
                            else
                                Read10Bit::readRGB16(filename, data, fb, w, h,
                                                     maxData, swap);
                            readit = true;
                            break;
                        }
                        case RGBA16:
                            Read10Bit::readRGBA16(filename, data, fb, w, h,
                                                  maxData, alpha, swap);
                            readit = true;
                            break;
                        case RGB10_A2:
                        {
                            if (alpha)
                            {
                                Read10Bit::readRGBA8(filename, data, fb, w, h,
                                                     maxData, true, swap);
                                nonspec = true;
                            }
                            else
                            {
                                //
                                // check for partial data in which case
                                // we cannot use raw directly.
                                //

                                const bool partial =
                                    size_t(4 * w * h) > maxData;
                                didUseRaw = canUseRaw && !partial;

                                Read10Bit::readRGB10_A2(
                                    filename, data, fb, w, h, maxData, swap,
                                    didUseRaw, (unsigned char*)fstream.data());
                            }

                            readit = true;
                            break;
                        }
                        case A2_BGR10:
                        {
                            if (alpha)
                            {
                                Read10Bit::readRGBA8(filename, data, fb, w, h,
                                                     maxData, true, swap);
                                nonspec = true;
                            }
                            else
                            {
                                didUseRaw = false;

                                Read10Bit::readA2_BGR10(filename, data, fb, w,
                                                        h, maxData, swap);
                            }

                            readit = true;
                            break;
                        }
                        case RGB8_PLANAR:
                        {
                            if (alpha)
                            {
                                Read10Bit::readRGBA8(filename, data, fb, w, h,
                                                     maxData, true, swap);
                                nonspec = true;
                            }
                            else
                            {
                                Read10Bit::readRGB8_PLANAR(filename, data, fb,
                                                           w, h, maxData, swap);
                            }

                            readit = true;
                            break;
                        }
                        case RGB16_PLANAR:
                        {
                            if (alpha)
                            {
                                Read10Bit::readRGBA8(filename, data, fb, w, h,
                                                     maxData, true, swap);
                                nonspec = true;
                            }
                            else
                            {
                                Read10Bit::readRGB16_PLANAR(
                                    filename, data, fb, w, h, maxData, swap);
                            }

                            readit = true;
                            break;
                        }
                        }
                    }

                    if (((w * h * ((alpha) ? 4 : 3)) > maxData) && !packedYUV)
                    {
                        fb.attribute<float>("PartialImage") = 1.0;
                    }

                    if (nonspec)
                    {
                        readerComment << "Used non-spec scanline boundaries."
                                      << endl;
                    }
                }
                else if (inputBits == 12)
                {
                    if (packing == IOdpx::DPX_PAD_NONE)
                    {
                        Read12Bit::readNoPaddingRGB16(filename, data, fb, w, h,
                                                      maxData, swap);
                    }
                    else
                    {
                        switch (m_format)
                        {
                        default:
                        case RGB8:
                        case RGBA8:
                        case RGB8_PLANAR:
                            Read12Bit::readRGB8_PLANAR(filename, data, fb, w, h,
                                                       maxData, swap);
                            break;
                        case RGB10_A2:
                        case A2_BGR10:
                        case RGB16:
                        case RGBA16:
                        case RGB16_PLANAR:
                            Read12Bit::readRGB16_PLANAR(filename, data, fb, w,
                                                        h, maxData, swap);
                            break;
                        }

                        if ((w * h * 6) > maxData)
                        {
                            fb.attribute<float>("PartialImage") = 1.0;
                        }
                    }
                }
                else if (inputBits == 16)
                {
                    if (alpha)
                    {
                        const bool partial = (w * h * 8) > maxData;

                        if (canUseRaw && !partial)
                        {
                            didUseRaw = true;

                            Read16Bit::readRGBA16(
                                filename, data, fb, w, h, maxData, true, swap,
                                didUseRaw, (unsigned char*)fstream.data());
                        }
                        else
                        {
                            Read16Bit::readRGBA16(filename, data, fb, w, h,
                                                  maxData, true, swap);
                        }

                        readit = true;
                        if (partial)
                            fb.attribute<float>("PartialImage") = 1.0;
                    }
                    else
                    {
                        const bool partial = (w * h * 6) > maxData;

                        if (m_format == RGB16 && canUseRaw && !partial)
                        {
                            didUseRaw = true;
                            Read16Bit::readRGB16(
                                filename, data, fb, w, h, maxData, swap,
                                didUseRaw, (unsigned char*)fstream.data());
                        }
                        else
                        {
                            Read16Bit::readRGB16_PLANAR(filename, data, fb, w,
                                                        h, maxData, swap);
                        }

                        if (partial)
                            fb.attribute<float>("PartialImage") = 1.0;
                    }
                }
                else if (inputBits == 8)
                {
                    if (alpha)
                    {
                        const bool partial = (w * h * 4) > maxData;

                        if (canUseRaw && !partial)
                        {
                            didUseRaw = true;
                            Read8Bit::readRGBA8(filename, data, fb, w, h,
                                                maxData, swap, didUseRaw,
                                                (unsigned char*)fstream.data());
                        }
                        else
                        {
                            Read8Bit::readRGBA8(filename, data, fb, w, h,
                                                maxData, swap);
                        }

                        if (partial)
                            fb.attribute<float>("PartialImage") = 1.0;
                    }
                    else
                    {
                        //
                        // no raw case for this right now: RGB8 has
                        // performance issues on playback for most cards.
                        //

                        const bool partial = (w * h * 3) > maxData;

                        Read8Bit::readRGB8_PLANAR(filename, data, fb, w, h,
                                                  maxData, swap);

                        if (partial)
                            fb.attribute<float>("PartialImage") = 1.0;
                    }

                    readit = true;
                }
            }
            catch (...)
            {
                fb.newAttribute("PartialImage", 1.0f);
            }

            string rc = readerComment.str();

            if (rc != "")
            {
                fb.attribute<string>("IOdpx/Comment") = rc;
            }

            {
                ostringstream str;
                if (swap)
                    str << "Swapped ";
                if (didUseRaw)
                    str << "Raw ";
                str << " LSbs=0x" << hex << alignment << " ";
                fb.newAttribute("IOdpx/Other", str.str());
            }

            if (didUseRaw)
            {
                fstream.setDeleteOnDestruction(false);
            }

            switch (header.image.orientation)
            {
            case DPX_TOP_LEFT:
            case DPX_ROTATED_TOP_LEFT:
                setFBOrientation(fb, FrameBuffer::TOPLEFT);
                break; // this is the default for DPX, the reading code all does
                       // this itself
            case DPX_BOTTOM_LEFT:
            case DPX_ROTATED_BOTTOM_LEFT:
                setFBOrientation(fb, FrameBuffer::NATURAL);
                break;
            case DPX_TOP_RIGHT:
            case DPX_ROTATED_TOP_RIGHT:
                setFBOrientation(fb, FrameBuffer::TOPRIGHT);
                break;
            case DPX_BOTTOM_RIGHT:
            case DPX_ROTATED_BOTTOM_RIGHT:
                setFBOrientation(fb, FrameBuffer::BOTTOMRIGHT);
                break;
            }

            readAttrs(fb, header, i);

            if (!readit && fb.hasAttribute("ChannelsInFile")
                && 4 == fb.attribute<int>("ChannelsInFile"))
            {
                cerr << "WARNING: This type of RGBA DPX file is not currently "
                        "supported."
                     << endl;
            }
        }
    }

    void IOdpx::writeImage(const FrameBuffer& img, const std::string& filename,
                           const WriteRequest& request) const
    {
        int bits = 10;
        float fps = 24.0;

        ofstream outfile(UNICODE_C_STR(filename.c_str()), ios_base::binary);
        if (!outfile)
            TWK_THROW_STREAM(IOException, "DPX: cannot open " << filename);

        // U32 utc = timeCodeStringToU32TC("78:65:43:21");
        // U32 utc = timeCodeStringToU32TC("00:00:01:23");
        // int uframes = U32TCToFrames(utc, 23.98);
        // cout << hex << "TC: " << utc << endl;
        // cout << "U32: " << U32TCToTimeCodeString(utc) << endl;
        // cout << "frames = " << dec << uframes << endl;
        // cout << "U32: " << hex << framesToU32TC(uframes, 23.98) << endl;

        const FrameBuffer* outfb = &img;

        DPXColorSpace transfer = DPX_PRINTING_DENSITY;
        DPXColorSpace colorimetric = DPX_ITU_R709;
        string creator = "Tweak Software";
        int orient = DPX_TOP_LEFT; // "core set" dpx by default
        string copyright;
        string project;
        string reserved;
        string createtime;

        string film_mfg_id;
        string film_type;
        string film_offset;
        string film_prefix;
        string film_count;
        string film_format;
        U32 film_frame_position = DPX_NO_DATA_U32;
        U32 film_sequence_len = DPX_NO_DATA_U32;
        R32 film_fps = DPX_NO_DATA_R32;
        R32 film_shutter_angle = DPX_NO_DATA_R32;
        string film_frame_id;
        string film_slate_info;

        U32 tv_tim_code = DPX_NO_DATA_U32;
        U32 tv_userBits = DPX_NO_DATA_U32;
        U8 tv_interlace = 0;
        U8 tv_field_num = 0;
        U8 tv_video_signal = 0;
        R32 tv_hor_sample_rate = DPX_NO_DATA_R32;
        R32 tv_ver_sample_rate = DPX_NO_DATA_R32;
        R32 tv_frame_rate = DPX_NO_DATA_R32;
        R32 tv_time_offset = DPX_NO_DATA_R32;
        R32 tv_black_level = DPX_NO_DATA_R32;
        R32 tv_black_gain = DPX_NO_DATA_R32;
        R32 tv_break_point = DPX_NO_DATA_R32;
        R32 tv_white_level = DPX_NO_DATA_R32;
        R32 tv_integration_times = DPX_NO_DATA_R32;
        R32 tv_gamma = DPX_NO_DATA_R32;

        U32 o_x_offset = DPX_NO_DATA_U32;
        U32 o_y_offset = DPX_NO_DATA_U32;
        R32 o_x_center = DPX_NO_DATA_R32;
        R32 o_y_center = DPX_NO_DATA_R32;
        U32 o_x_orig_size = DPX_NO_DATA_U32;
        U32 o_y_orig_size = DPX_NO_DATA_U32;
        string o_file_name;
        string o_creation_time;
        string o_input_dev;
        string o_input_serial;
        U16 o_border_XL = 0;
        U16 o_border_XR = 0;
        U16 o_border_YT = 0;
        U16 o_border_YB = 0;
        U32 o_pixel_aspect_H = DPX_NO_DATA_U32;
        U32 o_pixel_aspect_V = DPX_NO_DATA_U32;

        //
        //  Find existing image attributes that override defaults
        //  (e.g. color)
        //

        if (img.hasAttribute(ColorSpace::TransferFunction()))
        {
            string t = img.attribute<string>(ColorSpace::TransferFunction());

            if (t == ColorSpace::Rec709())
                transfer = DPX_ITU_R709;
            else if (t == ColorSpace::CineonLog())
                transfer = DPX_PRINTING_DENSITY;
            else if (t == ColorSpace::Linear())
                transfer = DPX_LINEAR;
            else
                transfer = DPX_USER_DEFINED_COLOR;
        }

        //
        //  Parse outparams. Get the "output" params which are provided by code
        //  (not the user) and use those to initialize everything.
        //

        int fs = 0;
        int fe = 0;
        int fc = 0;
        int fpos = 0;
        int seqlen = 0;

        for (size_t i = 0; i < request.parameters.size(); i++)
        {
            const StringPair pair = request.parameters[i];
            string name = pair.first;
            string value = pair.second;

            if (name == "output/start_frame")
                fs = atoi(value.c_str());
            else if (name == "output/end_frame")
                fe = atoi(value.c_str());
            else if (name == "output/frame")
                fc = atoi(value.c_str());
            else if (name == "output/frame_position")
                fpos = atoi(value.c_str());
            else if (name == "output/sequence_len")
                seqlen = atoi(value.c_str());
            else if (name == "output/fps")
            {
                fps = atof(value.c_str());
                tv_frame_rate = fps;
                film_fps = fps;
            }
            else if (name == "output/transfer")
            {
                if (value == ColorSpace::Rec709())
                    transfer = DPX_ITU_R709;
                else if (value == ColorSpace::sRGB())
                    transfer = DPX_USER_DEFINED_COLOR;
                else if (value == ColorSpace::CineonLog())
                    transfer = DPX_PRINTING_DENSITY;
                else if (value == ColorSpace::Linear())
                    transfer = DPX_LINEAR;
            }
            else if (name == "output/pa")
            {
                double pa = atof(value.c_str());
                o_pixel_aspect_H = U32(1000000 * pa);
                o_pixel_aspect_V = U32(1000000);
            }
        }

        //
        //  Default time values for the header.
        //

        film_frame_position = fpos;
        film_sequence_len = seqlen;
        tv_tim_code = framesToU32TC(fc, fps);
        tv_userBits = framesToU32TC(fpos, fps);

        size_t pixelAlignment = 0;

        //
        //  Get any overriding parameters
        //

        for (size_t i = 0; i < request.parameters.size(); i++)
        {
            const StringPair pair = request.parameters[i];
            string name = pair.first;
            string value = pair.second;

            if (name == "alignment")
            {
                pixelAlignment = atoi(value.c_str());
            }
            else if (name == "transfer" || name == "dpx/transfer")
            {
                if (value == "LOG")
                    transfer = DPX_LOGARITHMIC;
                else if (value == "DENSITY")
                    transfer = DPX_PRINTING_DENSITY;
                else if (value == "LINEAR")
                    transfer = DPX_LINEAR;
                else if (value == "REC709")
                    transfer = DPX_ITU_R709;
                else if (value == "USER")
                    transfer = DPX_USER_DEFINED_COLOR;
                else if (value == "VIDEO")
                    transfer = DPX_UNSPECIFIED_VIDEO;
                else if (value == "SMPTE274M")
                    transfer = DPX_SMPTE274M;
                else if (value == "REC601-625")
                    transfer = DPX_ITU_R601_625L;
                else if (value == "REC601-525")
                    transfer = DPX_ITU_R601_525L;
                else if (value == "NTSC")
                    transfer = DPX_NTSC;
                else if (value == "PAL")
                    transfer = DPX_PAL;
                else if (value.size() >= 1)
                {
                    if (value[0] >= '0' && value[0] <= '9')
                    {
                        transfer = (DPXColorSpace)atoi(value.c_str());
                    }
                }
            }
            else if (name == "colorimetric" || name == "dpx/colorimetric")
            {
                if (value == "REC709")
                    colorimetric = DPX_ITU_R709;
                else if (value == "USER")
                    colorimetric = DPX_USER_DEFINED_COLOR;
                else if (value == "VIDEO")
                    colorimetric = DPX_UNSPECIFIED_VIDEO;
                else if (value == "SMPTE274M")
                    colorimetric = DPX_SMPTE274M;
                else if (value == "REC601-625")
                    colorimetric = DPX_ITU_R601_625L;
                else if (value == "REC601-525")
                    colorimetric = DPX_ITU_R601_525L;
                else if (value == "NTCS")
                    colorimetric = DPX_NTSC;
                else if (value == "PAL")
                    colorimetric = DPX_PAL;
                else if (value.size() >= 1)
                {
                    if (value[0] >= '0' && value[0] <= '9')
                    {
                        colorimetric = (DPXColorSpace)atoi(value.c_str());
                    }
                }
            }
            else if (name == "orientation")
            {
                if (value == "TOP_LEFT")
                    orient = DPX_TOP_LEFT;
                else if (value == "TOP_RIGHT")
                    orient = DPX_TOP_RIGHT;
                else if (value == "BOTTOM_LEFT")
                    orient = DPX_BOTTOM_LEFT;
                else if (value == "BOTTOM_RIGHT")
                    orient = DPX_BOTTOM_RIGHT;
                else if (value == "ROTATED_TOP_LEFT")
                    orient = DPX_ROTATED_TOP_LEFT;
                else if (value == "ROTATED_TOP_RIGHT")
                    orient = DPX_ROTATED_TOP_RIGHT;
                else if (value == "ROTATED_BOTTOM_LEFT")
                    orient = DPX_ROTATED_BOTTOM_LEFT;
                else if (value == "ROTATED_BOTTOM_RIGHT")
                    orient = DPX_ROTATED_BOTTOM_RIGHT;
                else if (value.size() >= 1)
                {
                    if (value[0] >= '0' && value[0] <= '9')
                    {
                        orient = atoi(value.c_str());
                    }
                }
            }
            else if (name == "creator")
                creator = value;
            else if (name == "copyright" || name == "output/copyright")
                copyright = value;
            else if (name == "project")
                project = value;
            else if (name == "reserved")
                reserved = value;
            else if (name == "create_time")
                createtime = value;
            else if (name == "film/mfg_id")
                film_mfg_id = value;
            else if (name == "film/type")
                film_type = value;
            else if (name == "film/offset")
                film_offset = value;
            else if (name == "film/prefix")
                film_prefix = value;
            else if (name == "film/count")
                film_count = value;
            else if (name == "film/format")
                film_format = value;
            else if (name == "film/frame_position")
            {
                film_frame_position = atoi(value.c_str());
            }
            else if (name == "film/sequence_len")
            {
                film_sequence_len = atoi(value.c_str());
            }
            else if (name == "film/fps")
                film_fps = atof(value.c_str());
            else if (name == "film/shutter_angle")
                film_shutter_angle = atof(value.c_str());
            else if (name == "film/frame_id")
                film_frame_id = value;
            else if (name == "film/slate_info")
                film_slate_info = value;
            else if (name == "tv/time_code")
            {
                tv_tim_code = framesToU32TC(
                    U32TCToFrames(timeCodeStringToU32TC(value), fps) + fpos,
                    fps);
            }
            else if (name == "tv/user_bits")
            {
                tv_userBits = framesToU32TC(
                    U32TCToFrames(timeCodeStringToU32TC(value), fps) + fpos,
                    fps);
            }
            else if (name == "tv/interlace")
                tv_interlace = atoi(value.c_str());
            else if (name == "tv/field_num")
                tv_field_num = atoi(value.c_str());
            else if (name == "tv/video_signal")
                tv_video_signal = atoi(value.c_str());
            else if (name == "tv/horizontal_sample_rate")
                tv_hor_sample_rate = atof(value.c_str());
            else if (name == "tv/vertical_sample_rate")
                tv_ver_sample_rate = atof(value.c_str());
            else if (name == "tv/frame_rate")
                tv_frame_rate = atof(value.c_str());
            else if (name == "tv/time_offset")
                tv_time_offset = atof(value.c_str());
            else if (name == "output/gamma" || name == "tv/gamma")
                tv_gamma = atof(value.c_str());
            else if (name == "tv/black_level")
                tv_black_level = atof(value.c_str());
            else if (name == "tv/black_gain")
                tv_black_gain = atof(value.c_str());
            else if (name == "tv/break_point")
                tv_break_point = atof(value.c_str());
            else if (name == "tv/white_level")
                tv_white_level = atof(value.c_str());
            else if (name == "tv/integration_times")
                tv_integration_times = atof(value.c_str());
            else if (name == "source/x_offset")
                o_x_offset = atoi(value.c_str());
            else if (name == "source/y_offset")
                o_y_offset = atoi(value.c_str());
            else if (name == "source/x_original_size")
                o_x_orig_size = atoi(value.c_str());
            else if (name == "source/y_original_size")
                o_y_orig_size = atoi(value.c_str());
            else if (name == "source/x_center")
                o_x_center = atof(value.c_str());
            else if (name == "source/y_center")
                o_y_center = atof(value.c_str());
            else if (name == "source/file_name")
                o_file_name = value;
            else if (name == "source/creation_time")
                o_creation_time = value;
            else if (name == "source/input_dev")
                o_input_dev = value;
            else if (name == "source/input_serial")
                o_input_serial = value;
            else if (name == "source/border_XL")
                o_border_XL = atoi(value.c_str());
            else if (name == "source/border_XR")
                o_border_XR = atoi(value.c_str());
            else if (name == "source/border_YT")
                o_border_YT = atoi(value.c_str());
            else if (name == "source/border_YB")
                o_border_YB = atoi(value.c_str());
            else if (name == "output/pa/numerator"
                     || name == "source/pixel_aspect_H")
                o_pixel_aspect_H = atoi(value.c_str());
            else if (name == "output/pa/denominator"
                     || name == "source/pixel_aspect_V")
                o_pixel_aspect_V = atoi(value.c_str());
        }

        //
        //  Convert to interleaved if not already.
        //

        if (img.isPlanar())
        {
            const FrameBuffer* fb = outfb;
            outfb = mergePlanes(outfb);
            if (fb != &img)
                delete fb;
        }

        //
        //  Convert color to REC709 primaries
        //

        if (outfb->hasPrimaries() || outfb->isYUV() || outfb->isYRYBY()
            || outfb->dataType() >= FrameBuffer::PACKED_R10_G10_B10_X2)
        {
            const FrameBuffer* fb = outfb;
            outfb = convertToLinearRGB709(outfb);
            transfer = DPX_LINEAR;
            if (fb != &img)
                delete fb;
        }

        //
        //  Convert to 3 channel, RGB
        //

        if (img.channelNames().size() != 3 || img.channelName(0) != "R"
            || img.channelName(1) != "G" || img.channelName(2) != "B")
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

        switch (img.dataType())
        {
        case FrameBuffer::HALF:
        case FrameBuffer::FLOAT:
        {
            const FrameBuffer* fb = outfb;
            outfb = copyConvert(outfb, FrameBuffer::USHORT);
            if (fb != &img)
                delete fb;
            break;
        }
        default:
            break;
        }

        const size_t imageDataInBytes =
            outfb->width() * outfb->height() * sizeof(U32);

        //
        //  HEADERS
        //

        DPXHeader header;
        memset(&header, 0, sizeof(header));

        size_t dataOffset = sizeof(header);

        if (pixelAlignment > sizeof(header))
        {
            dataOffset = pixelAlignment;
        }

        //
        //  FILE header
        //
        header.file.magic_num = 0x53445058; // native endian
        header.file.offset = dataOffset;
        header.file.file_size = dataOffset + imageDataInBytes;
        header.file.ditto_key = 1; // new
        header.file.key = 0xFFFFFFFF;
        header.file.user_data_size = 0;
        header.file.gen_hdr_size = sizeof(DPXFileInformation)
                                   + sizeof(DPXImageInformation)
                                   + sizeof(DPXImageOrientation); // FIX
        header.file.ind_hdr_size =
            sizeof(DPXFilmHeader) + sizeof(DPXTelevisionHeader); // FIX

        setField(header.file.vers, 8, "V2.0");
        setField(header.file.file_name, 100, filename);
        setField(header.file.creator, 100, creator);
        setField(header.file.copyright, 200, copyright);
        setField(header.file.project, 200, project);
        setField(header.file.create_time, 24, createtime);
        setField(header.file.Reserved, 104, reserved);

        //
        //  IMAGE header
        //

        //
        //  Since some software gets freaked out by dpx orientation, we'll
        //  just be nice and output what most software does
        //

        bool doflip = false;
        bool doflop = false;

        //
        //  Set for top left first
        //

        switch (outfb->orientation())
        {
        case FrameBuffer::NATURAL:

            switch (orient)
            {
            case DPX_BOTTOM_LEFT:
            case DPX_ROTATED_BOTTOM_LEFT:
                break;
            case DPX_TOP_LEFT:
            case DPX_ROTATED_TOP_LEFT:
                doflip = true;
                break;
            case DPX_TOP_RIGHT:
            case DPX_ROTATED_TOP_RIGHT:
                doflop = true;
                doflip = true;
                break;
            case DPX_BOTTOM_RIGHT:
            case DPX_ROTATED_BOTTOM_RIGHT:
                doflop = true;
                break;
            }
            break;

        case FrameBuffer::TOPLEFT:
            switch (orient)
            {
            case DPX_BOTTOM_LEFT:
            case DPX_ROTATED_BOTTOM_LEFT:
                doflip = true;
                break;
            case DPX_TOP_LEFT:
            case DPX_ROTATED_TOP_LEFT:
                break;
            case DPX_TOP_RIGHT:
            case DPX_ROTATED_TOP_RIGHT:
                doflop = true;
                break;
            case DPX_BOTTOM_RIGHT:
            case DPX_ROTATED_BOTTOM_RIGHT:
                doflop = true;
                doflip = true;
                break;
            }
            break;

        case FrameBuffer::TOPRIGHT:
            switch (orient)
            {
            case DPX_BOTTOM_LEFT:
            case DPX_ROTATED_BOTTOM_LEFT:
                doflop = true;
                doflip = true;
                break;
            case DPX_TOP_LEFT:
            case DPX_ROTATED_TOP_LEFT:
                doflop = true;
                break;
            case DPX_TOP_RIGHT:
            case DPX_ROTATED_TOP_RIGHT:
                break;
            case DPX_BOTTOM_RIGHT:
            case DPX_ROTATED_BOTTOM_RIGHT:
                doflip = true;
                break;
            }
            break;

        case FrameBuffer::BOTTOMRIGHT:
            switch (orient)
            {
            case DPX_BOTTOM_LEFT:
            case DPX_ROTATED_BOTTOM_LEFT:
                doflop = true;
                break;
            case DPX_TOP_LEFT:
            case DPX_ROTATED_TOP_LEFT:
                doflop = true;
                doflip = true;
                break;
            case DPX_TOP_RIGHT:
            case DPX_ROTATED_TOP_RIGHT:
                doflip = true;
                break;
            case DPX_BOTTOM_RIGHT:
            case DPX_ROTATED_BOTTOM_RIGHT:
                break;
            }
            break;

        default:
            break;
        }

        //
        //  Now check the desired outgoing orientation
        //

        if (doflop)
        {
            //
            //  Unlikely case, we can be inefficient for easy of implementation
            //

            if (outfb == &img)
                outfb = img.copy();
            FrameBuffer* fb = const_cast<FrameBuffer*>(outfb);
            flop(fb);
            outfb = fb;
        }

        header.image.orientation = U16(orient);
        header.image.element_number = 1;
        header.image.pixels_per_line = outfb->width();
        header.image.lines_per_image_ele = outfb->height();

        // fillNoData(header.image.reserved, 52);

        DPXImageInformation::_image_element& e = header.image.image_element[0];

        e.data_sign = 0;
        e.ref_low_data = 0;
        e.ref_low_quantity = 0.0;
        e.ref_high_data = 1023;
        e.ref_high_quantity = 2.048;
        e.descriptor = DPX_PIXEL_RGB;
        e.transfer = transfer;
        e.colorimetric = colorimetric;
        e.bit_size = 10;
        e.packing = 1;
        e.encoding = 0;
        e.data_offset = dataOffset;
        e.eol_padding = 0;
        e.eo_image_padding = 0;

        //
        //  ORIENTATION header
        //

        header.orientation.x_offset = o_x_offset;
        header.orientation.y_offset = o_y_offset;
        header.orientation.border_XL = o_border_XL;
        header.orientation.border_XR = o_border_XR;
        header.orientation.border_YT = o_border_YT;
        header.orientation.border_YB = o_border_YB;
        header.orientation.pixel_aspect_H = o_pixel_aspect_H;
        header.orientation.pixel_aspect_V = o_pixel_aspect_V;
        header.orientation.x_center = (!hasData(o_x_center))
                                          ? header.image.pixels_per_line / 2
                                          : o_x_center;
        header.orientation.y_center = (!hasData(o_y_center))
                                          ? header.image.lines_per_image_ele / 2
                                          : o_y_center;
        header.orientation.x_orig_size = (!hasData(o_x_orig_size))
                                             ? header.image.pixels_per_line
                                             : o_x_orig_size;
        header.orientation.y_orig_size = (!hasData(o_y_orig_size))
                                             ? header.image.lines_per_image_ele
                                             : o_y_orig_size;

        setField(header.orientation.file_name, 100, o_file_name);
        setField(header.orientation.creation_time, 24, o_creation_time);
        setField(header.orientation.input_dev, 32, o_input_dev);
        setField(header.orientation.input_serial, 32, o_input_serial);

        // fillNoData(header.orientation.reserved, 28);

        //
        //  FILM header
        //

        setField(header.film.film_mfg_id, 2, film_mfg_id);
        setField(header.film.film_type, 2, film_type);
        setField(header.film.offset, 2, film_offset);
        setField(header.film.prefix, 6, film_prefix);
        setField(header.film.count, 4, film_count);
        setField(header.film.format, 32, film_format);

        header.film.frame_position = film_frame_position;
        header.film.sequence_len = film_sequence_len;
        header.film.frame_rate = film_fps;
        header.film.shutter_angle = film_shutter_angle;
        header.film.held_count = 1;
        setField(header.film.frame_id, 32, film_frame_id);
        setField(header.film.slate_info, 100, film_slate_info);
        // fillNoData(header.film.reserved, 56);

        //
        //  TV header
        //

        header.tv.tim_code = tv_tim_code;
        header.tv.userBits = tv_userBits;
        header.tv.interlace = tv_interlace;
        header.tv.field_num = tv_field_num;
        header.tv.video_signal = tv_video_signal;
        header.tv.hor_sample_rate = tv_hor_sample_rate;
        header.tv.ver_sample_rate = tv_ver_sample_rate;
        header.tv.frame_rate = tv_frame_rate;
        header.tv.time_offset = tv_time_offset;
        header.tv.gamma = tv_gamma;
        header.tv.black_level = tv_black_level;
        header.tv.black_gain = tv_black_gain;
        header.tv.break_point = tv_break_point;
        header.tv.white_level = tv_white_level;
        header.tv.integration_times = tv_integration_times;

        //
        //  Write header
        //

        outfile.write((const char*)&header, sizeof(header));

        if (dataOffset != sizeof(header))
        {
            size_t paddingSize = dataOffset - sizeof(header);
            unsigned char* padding = new unsigned char[paddingSize];
            memset(padding, 0xff, paddingSize);
            outfile.write((const char*)padding, paddingSize);
            delete[] padding;
        }

        //
        //  Write Image Data
        //

        vector<OutputPixel> outScanline(outfb->width());
        OutputPixel* o = &outScanline.front();

        unsigned int ch = outfb->numChannels();

        if (outfb->dataType() == FrameBuffer::USHORT)
        {
            for (size_t i = 0; i < outfb->height(); i++)
            {
                size_t index = doflip ? outfb->height() - i - 1 : i;

                const unsigned short* in =
                    outfb->scanline<unsigned short>(index);
                OutputPixel* out = o;
                const unsigned short* e = in + (outfb->width() * ch);

                for (const unsigned short* p = in; p < e; p += ch, out++)
                {
                    out->red = p[0] >> 6;
                    out->green = p[1] >> 6;
                    out->blue = p[2] >> 6;
                    out->unused = 0;
                }

                outfile.write((const char*)o, outScanline.size() * sizeof(U32));
            }
        }
        else
        {
            for (size_t i = 0; i < outfb->height(); i++)
            {
                size_t index = doflip ? outfb->height() - i - 1 : i;

                const unsigned char* in = outfb->scanline<unsigned char>(index);
                OutputPixel* out = o;
                const unsigned char* e = in + (outfb->width() * ch);

                for (const unsigned char* p = in; p < e; p += ch, out++)
                {
                    out->red = p[0] << 2;
                    out->green = p[1] << 2;
                    out->blue = p[2] << 2;
                    out->unused = 0;
                }

                outfile.write((const char*)o, outScanline.size() * sizeof(U32));
            }
        }

        if (outfb != &img)
            delete outfb;
    }

} //  End namespace TwkFB
