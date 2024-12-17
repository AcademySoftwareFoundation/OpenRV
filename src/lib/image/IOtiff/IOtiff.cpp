//******************************************************************************
// Copyright (c) 2001-2004 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <IOtiff/IOtiff.h>
#include <iostream>
#include <string>
#include <stl_ext/string_algo.h>
#include <tiffiop.h>
#include <tiffvers.h>
#include <TwkFB/Exception.h>
#include <TwkMath/Mat44.h>
#include <TwkMath/Iostream.h>
#include <TwkFB/Operations.h>
#include <TwkUtil/FileStream.h>
#include <TwkUtil/File.h>

#include <limits>

#if TIFFLIB_VERSION >= 20031226
#define HAS_TIFFFIELDWITHTAG 1
#endif

#define SAMPLEFORMAT_INT8_NORM_VALUE float(std::numeric_limits<char>::max())
#define SAMPLEFORMAT_INT16_NORM_VALUE \
    float(std::numeric_limits<short int>::max())
#define SAMPLEFORMAT_INT32_NORM_VALUE float(std::numeric_limits<int>::max())

namespace TwkFB
{
    using namespace std;
    using namespace TwkUtil;
    using namespace TwkMath;

    const unsigned short DEFAULT_TIFFTAG_BITSPERSAMPLE_VALUE = 8;
    const unsigned short DEFAULT_TIFFTAG_SAMPLEFORMAT_VALUE = SAMPLEFORMAT_UINT;
    const unsigned short DEFAULT_TIFFTAG_SAMPLESPERPIXEL_VALUE = 1;

    enum TagType
    {
        FLOAT_TAG,
        UINT_TAG,
        SHORT_TAG,
        USHORT_TAG,
        ASCII_TAG,
        MATRIX_TAG,
        FLOAT6_TAG,
        FLOAT2_TAG
    };

    struct TagEntry
    {
        unsigned int tag;
        TagType type;
    };

    struct TagNameValue
    {
        const char* name;
        unsigned short tag;
    };

    static const TagNameValue compTags[] = {
        {"NONE", COMPRESSION_NONE},
        {"CCITTRLE", COMPRESSION_CCITTRLE},
        {"CCITTFAX3", COMPRESSION_CCITTFAX3},
        {"CCITT_T4", COMPRESSION_CCITT_T4},
        {"CCITTFAX4", COMPRESSION_CCITTFAX4},
        {"CCITT_T6", COMPRESSION_CCITT_T6},
        {"LZW", COMPRESSION_LZW},
        {"JPEG", COMPRESSION_JPEG},
        {"NEXT", COMPRESSION_NEXT},
        {"CCITTRLEW", COMPRESSION_CCITTRLEW},
        {"PACKBITS", COMPRESSION_PACKBITS},
        {"THUNDERSCAN", COMPRESSION_THUNDERSCAN},
        {"IT8CTPAD", COMPRESSION_IT8CTPAD},
        {"IT8LW", COMPRESSION_IT8LW},
        {"IT8BL", COMPRESSION_IT8BL},
        {"PIXARFILM", COMPRESSION_PIXARFILM},
        {"PIXARLOG", COMPRESSION_PIXARLOG},
        {"DEFLATE", COMPRESSION_DEFLATE},
        {"ADOBE_DEFLATE", COMPRESSION_ADOBE_DEFLATE},
        {"DCS", COMPRESSION_DCS},
        {"JBIG", COMPRESSION_JBIG},
        {"SGILOG", COMPRESSION_SGILOG},
        {"SGILOG24", COMPRESSION_SGILOG24},
        {"JP2000", COMPRESSION_JP2000},
        {0, 0}};

    static const TagNameValue exifTags[] = {
        {"ExposureTime", EXIFTAG_EXPOSURETIME},
        {"FNumber", EXIFTAG_FNUMBER},
        {"ExposureProgram", EXIFTAG_EXPOSUREPROGRAM},
        {"SpectralSensitivity", EXIFTAG_SPECTRALSENSITIVITY},
        {"ISOSpeedRatings", EXIFTAG_ISOSPEEDRATINGS},
        {"OptoelectricConversionFactor", EXIFTAG_OECF},
        {"ExifVersion", EXIFTAG_EXIFVERSION},
        {"DateTimeOriginal", EXIFTAG_DATETIMEORIGINAL},
        {"DateTimeDigitized", EXIFTAG_DATETIMEDIGITIZED},
        {"ComponentsConfiguration", EXIFTAG_COMPONENTSCONFIGURATION},
        {"CompressedBitsPerPixel", EXIFTAG_COMPRESSEDBITSPERPIXEL},
        {"ShutterSpeedValue", EXIFTAG_SHUTTERSPEEDVALUE},
        {"ApertureValue", EXIFTAG_APERTUREVALUE},
        {"BrightnessValue", EXIFTAG_BRIGHTNESSVALUE},
        {"ExposureBiasValue", EXIFTAG_EXPOSUREBIASVALUE},
        {"MaxApertureValue", EXIFTAG_MAXAPERTUREVALUE},
        {"SubjectDistance", EXIFTAG_SUBJECTDISTANCE},
        {"MeteringMode", EXIFTAG_METERINGMODE},
        {"LightSource", EXIFTAG_LIGHTSOURCE},
        {"Flash", EXIFTAG_FLASH},
        {"FocalLength", EXIFTAG_FOCALLENGTH},
        {"SubjectArea", EXIFTAG_SUBJECTAREA},
        {"MakerNote", EXIFTAG_MAKERNOTE},
        {"UserComment", EXIFTAG_USERCOMMENT},
        {"SubSecTime", EXIFTAG_SUBSECTIME},
        {"SubSecTimeOriginal", EXIFTAG_SUBSECTIMEORIGINAL},
        {"SubSecTimeDigitized", EXIFTAG_SUBSECTIMEDIGITIZED},
        {"FlashpixVersion", EXIFTAG_FLASHPIXVERSION},
        {"PixelXDimension", EXIFTAG_PIXELXDIMENSION},
        {"PixelYDimension", EXIFTAG_PIXELYDIMENSION},
        {"RelatedSoundFile", EXIFTAG_RELATEDSOUNDFILE},
        {"FlashEnergy", EXIFTAG_FLASHENERGY},
        {"SpatialFrequencyResponse", EXIFTAG_SPATIALFREQUENCYRESPONSE},
        {"FocalPlaneXResolution", EXIFTAG_FOCALPLANEXRESOLUTION},
        {"FocalPlaneYResolution", EXIFTAG_FOCALPLANEYRESOLUTION},
        {"FocalPlaneResolutionUnit", EXIFTAG_FOCALPLANERESOLUTIONUNIT},
        {"SubjectLocation", EXIFTAG_SUBJECTLOCATION},
        {"ExposureIndex", EXIFTAG_EXPOSUREINDEX},
        {"SensingMethod", EXIFTAG_SENSINGMETHOD},
        {"FileSource", EXIFTAG_FILESOURCE},
        {"SceneType", EXIFTAG_SCENETYPE},
        {"CFAPattern", EXIFTAG_CFAPATTERN},
        {"CustomRendered", EXIFTAG_CUSTOMRENDERED},
        {"ExposureMode", EXIFTAG_EXPOSUREMODE},
        {"WhiteBalance", EXIFTAG_WHITEBALANCE},
        {"DigitalZoomRatio", EXIFTAG_DIGITALZOOMRATIO},
        {"FocalLengthIn35mmFilm", EXIFTAG_FOCALLENGTHIN35MMFILM},
        {"SceneCaptureType", EXIFTAG_SCENECAPTURETYPE},
        {"GainControl", EXIFTAG_GAINCONTROL},
        {"Contrast", EXIFTAG_CONTRAST},
        {"Saturation", EXIFTAG_SATURATION},
        {"Sharpness", EXIFTAG_SHARPNESS},
        {"DeviceSettingDescription", EXIFTAG_DEVICESETTINGDESCRIPTION},
        {"SubjectDistanceRange", EXIFTAG_SUBJECTDISTANCERANGE},
        {"ImageUniqueID", EXIFTAG_IMAGEUNIQUEID},
        {0, 0}};

    static unsigned short nameToTag(const char* name, const TagNameValue* tags)
    {
        for (const TagNameValue* t = tags; t->name; t++)
        {
            if (!strcmp(t->name, name))
                return t->tag;
        }

        return 0;
    }

    static std::string tagToName(unsigned short tag, const TagNameValue* tags)
    {
        for (const TagNameValue* t = tags; t->name; t++)
        {
            if (t->tag == tag)
                return t->name;
        }

        ostringstream str;
        str << "Unknown Tag (" << tag << ")";
        return str.str();
    }

    IOtiff::IOtiff(bool addAlphaTo3Channel, bool rgbPlanar, IOType ioMethod,
                   size_t chunkSize, int maxAsync)
        : StreamingFrameBufferIO("IOtiff", "m1", ioMethod, chunkSize, maxAsync)
        , m_rgbPlanar(rgbPlanar)
        , m_addAlphaTo3Channel(addAlphaTo3Channel)
    {
        TIFFSetErrorHandler(0);
        TIFFSetWarningHandler(0);

        unsigned int types = Int8Capable | Int16Capable | Float32Capable
                             | PlanarRead /*| PlanarWrite*/;
        unsigned int capio = ImageRead | ImageWrite | BruteForceIO | types;
        unsigned int capi = ImageRead | types | BruteForceIO;

        StringPairVector codecs;
        codecs.push_back(StringPair("NONE", "No compression"));
        codecs.push_back(StringPair("DEFLATE", "Deflate compression"));
        codecs.push_back(StringPair("LZW", "Lempel-Ziv  & Welch"));
        codecs.push_back(StringPair("PACKBITS", "Macintosh RLE"));
        codecs.push_back(StringPair(
            "ADOBE_DEFLATE", "Deflate compression as recognized by Adobe"));
        codecs.push_back(StringPair("CCITTRLE", "CCITT modified Huffman RLE"));
        codecs.push_back(StringPair("CCITTFAX3", "CCITT Group 3 fax encoding"));
        codecs.push_back(StringPair("CCITT_T4", "CCITT T.4 (TIFF 6 name)"));
        codecs.push_back(StringPair("CCITTFAX4", "CCITT Group 4 fax encoding"));
        codecs.push_back(StringPair("CCITT_T6", "CCITT T.6 (TIFF 6 name)"));
        codecs.push_back(StringPair("JPEG", "%JPEG DCT compression"));
        codecs.push_back(StringPair("NEXT", "NeXT 2-bit RLE"));
        codecs.push_back(StringPair("CCITTRLEW", "#1 w/ word alignment"));
        codecs.push_back(StringPair("THUNDERSCAN", "ThunderScan RLE"));
        codecs.push_back(StringPair("IT8CTPAD", "IT8 CT w/padding"));
        codecs.push_back(StringPair("IT8LW", "IT8 Linework RLE"));
        codecs.push_back(StringPair("IT8BL", "IT8 Binary line art"));
        codecs.push_back(StringPair("PIXARFILM", "Pixar companded 10bit LZW"));
        codecs.push_back(StringPair("PIXARLOG", "Pixar companded 11bit ZIP"));
        codecs.push_back(StringPair("DCS", "Kodak DCS encoding"));
        codecs.push_back(StringPair("JBIG", "ISO JBIG"));
        codecs.push_back(StringPair("SGILOG", "SGI Log Luminance RLE"));
        codecs.push_back(StringPair("SGILOG24", "SGI Log 24-bit packed"));
        codecs.push_back(StringPair("JP2000", "Leadtools JPEG2000"));

        addType("tif", "TIFF Image", capio, codecs);
        addType("tiff", "TIFF Image", capio, codecs);

        addType("sm", "Entropy (TIFF) Shadow Map", capi, codecs);
        addType("tex", "PRMan (TIFF) Texture Map", capi, codecs);
        addType("tx", "PRMan (TIFF) Texture Map", capi, codecs);
        addType("txt", "PRMan (TIFF) Texture Map", capi, codecs);
        addType("tdl", "3delight (TIFF) Mip-Mapped Texture", capi, codecs);
        addType("shd", "3delight (TIFF) Shadow Map", capi, codecs);
    }

    IOtiff::~IOtiff() {}

    string IOtiff::about() const
    {
        char temp[80];
        sprintf(temp, "TIFF (libtiff %d)", TIFFLIB_VERSION);
        return temp;
    }

    bool IOtiff::getBoolAttribute(const std::string& name) const
    {
        if (name == "addAlphaTo3Channel")
            return m_addAlphaTo3Channel;
        else if (name == "rgbPlanar")
            return m_rgbPlanar;
        return StreamingFrameBufferIO::getBoolAttribute(name);
    }

    void IOtiff::setBoolAttribute(const std::string& name, bool value)
    {
        if (name == "addAlphaTo3Channel")
            m_addAlphaTo3Channel = value;
        else if (name == "rgbPlanar")
            m_rgbPlanar = value;
        else
            StreamingFrameBufferIO::setBoolAttribute(name, value);
    }

    static void readAllTags(TIFF* tif, FrameBuffer& img)
    {
        //
        //  ICC Profile
        //
        //  I have to look for this before EXIFTAG_COLORSPACE or else I get
        //  crashing
        //

        uint32 Len;
        void* Buffer;
        bool set_profile = false;

        if (TIFFGetField(tif, TIFFTAG_ICCPROFILE, &Len, &Buffer))
        {
            img.setPrimaryColorSpace(ColorSpace::ICCProfile());
            img.setTransferFunction(ColorSpace::ICCProfile());
            img.setICCprofile(Buffer, Len);

            set_profile = true;

            //_TIFFfree(Buffer); // this is a bad idea apparently (causes a
            // crash)
        }

        //
        //  Read EXIF tags if present
        //

        uint32 exif_offset = 0;

        if (TIFFGetField(tif, TIFFTAG_EXIFIFD, &exif_offset))
        {
            tdir_t directory = TIFFCurrentDirectory(tif);

            if (TIFFReadEXIFDirectory(tif, exif_offset))
            {
                unsigned short v_short = 0;

                //
                // Special handling of color space tag
                //

                if (TIFFGetField(tif, EXIFTAG_COLORSPACE, &v_short))
                {
                    if (v_short == 1)
                    {
                        img.attribute<string>("EXIF/ColorSpace") = "1 (sRGB)";

                        if (!set_profile)
                        {
                            img.setPrimaryColorSpace(ColorSpace::Rec709());
                            img.setTransferFunction(ColorSpace::sRGB());
                        }
                    }
                    else if (v_short == 2)
                    {
                        //
                        //  NOTE: this might be wrong.
                        //  See: http://www.cpanforum.com/threads/2784
                        //

                        img.attribute<string>("EXIF/ColorSpace") =
                            "2 (Adobe RGB)";

                        if (!set_profile)
                        {
                            img.setPrimaryColorSpace(ColorSpace::ICCProfile());
                            img.attribute<string>(
                                ColorSpace::ICCProfileDescription()) =
                                string("Adobe RGB");
                        }
                    }
                    else
                    {
                        ostringstream str;
                        str << v_short << " (Uncalibrated)";
                        img.attribute<string>("EXIF/ColorSpace") = str.str();
                        img.setPrimaryColorSpace(ColorSpace::Generic());
                    }
                }
            }
        }

        //
        //  Now the rest of the tags
        //

        for (int fi = 0, nfi = tif->tif_nfields; fi < nfi; fi++)
        {
            const TIFFField* const fip = tif->tif_fields[fi];

            if (fip->field_tag != TIFFTAG_ICCPROFILE
                && // exclude tags we handle seperately
                fip->field_tag != EXIFTAG_COLORSPACE &&

                fip->field_tag != TIFFTAG_XRESOLUTION
                && fip->field_tag != TIFFTAG_YRESOLUTION
                && fip->field_tag != TIFFTAG_SOFTWARE
                && fip->field_tag != EXIFTAG_PIXELXDIMENSION
                && fip->field_tag != EXIFTAG_PIXELYDIMENSION
                && fip->field_tag != TIFFTAG_RESOLUTIONUNIT
                && fip->field_tag != TIFFTAG_PLANARCONFIG &&

                fip->field_tag != TIFFTAG_SUBIFD &&

                // There are some variable length tags that require more than
                // one argument. This is an attempt to filter those out.
                // However, this may be filtering EXIF tags ... its hard to tell
                // For example TIFFTAG_COLORMAP,  TIFFTAG_HALFTONEHINTS,
                // TIFFTAG_PAGENUMBER, and TIFFTAG_SUBIFD, ...
                // see tif_dir.c _TIFFVGetField for multiple uses of va_arg()
                // by a tag type.

                (fip->field_readcount != TIFF_VARIABLE
                 || fip->field_type == TIFF_ASCII)
                &&

                // fip->field_tag != TIFFTAG_COLORMAP &&
                // fip->field_tag != TIFFTAG_HALFTONEHINTS &&
                fip->field_tag != TIFFTAG_PAGENUMBER &&
                // fip->field_tag != TIFFTAG_SUBIFD &&
                fip->field_tag != TIFFTAG_YCBCRSUBSAMPLING &&
                // fip->field_tag != TIFFTAG_TRANSFERFUNCTION &&

                fip->field_tag != TIFFTAG_PHOTOMETRIC
                && fip->field_tag != TIFFTAG_IMAGEWIDTH
                && fip->field_tag != TIFFTAG_IMAGELENGTH
                && fip->field_tag != TIFFTAG_BITSPERSAMPLE
                && fip->field_tag != TIFFTAG_SAMPLESPERPIXEL)
            {
                unsigned char ch;
                char* text;
                float f;
                float* fp;
                unsigned int ui;
                unsigned short us;

                bool exif_tag = false;

                if (exif_offset)
                {
                    //
                    //  If we read EXIF, we have to check tags against known
                    //  EXIF
                    //

                    for (unsigned int tag = exifTags[0].tag, i = 0;
                         (tag = exifTags[i].tag) && !exif_tag; i++)
                    {
                        exif_tag = (tag == fip->field_tag);
                    }
                }

                string aname = (exif_tag ? string("EXIF/") : string("TIFF/"))
                               + fip->field_name;

                switch (fip->field_type)
                {
                case TIFF_ASCII:
                    if (fip->field_passcount)
                    {
                        if (TIFFGetField(tif, fip->field_tag, &Len, &text))
                        {
                            img.attribute<string>(aname) = text;
                        }
                    }
                    else if (TIFFGetField(tif, fip->field_tag, &text))
                    {
                        img.attribute<string>(aname) = text;
                    }
                    break;

                case TIFF_SHORT:
                    if (TIFFGetField(tif, fip->field_tag, &us))
                    {
                        img.attribute<int>(aname) = us;
                    }
                    break;

                case TIFF_LONG:
                    // sometimes this is LONG (which is supposed to be wrong)
                    if (aname == "TIFF/RichTIFFIPTC")
                    {
                        void* array = 0;

                        if (TIFFGetField(tif, fip->field_tag, &ui, &array))
                        {
                            img.attribute<string>(aname) = "";
                        }
                    }
                    else if (TIFFGetField(tif, fip->field_tag, &ui))
                    {
                        img.attribute<int>(aname) = ui;
                    }
                    break;

                case TIFF_RATIONAL:
                case TIFF_SRATIONAL:
                case TIFF_FLOAT:
                    if (TIFFGetField(tif, fip->field_tag, &f))
                    {
                        img.attribute<float>(aname) = f;
                    }
                    break;

                case TIFF_BYTE:
                    // could be BYTE
                    if (aname == "TIFF/RichTIFFIPTC")
                    {
                        char* array = 0;

                        if (TIFFGetField(tif, fip->field_tag, &ui, &array))
                        {
                            img.attribute<string>(aname) = array;
                        }
                    }
                    break;

                case TIFF_UNDEFINED: // means char, apparently
                    // could be undefined
                    if (aname == "TIFF/RichTIFFIPTC")
                    {
                        char* array = 0;

                        if (TIFFGetField(tif, fip->field_tag, &ui, &array))
                        {
                            img.attribute<string>(aname) = array;
                        }
                    }
                    // if(TIFFGetField(tif, fip->field_tag, &ch))
                    //{
                    // img.attribute<int>(aname) = ch;
                    //}
                    break;

                default:
                    break;
                }
            }
        }
    }

    //
    // Store signed int samples within a tiff as floats
    // into the frameBuffer.
    // If its a signed int format we have to read each
    // signed int sample, and cast it to a float and
    // normalize before store as a float into the frameBuffer.
    // The normalize value we use is the max value of its
    // signed type equivalent.
    //
    static void storeIntAsNormalizedFloatSamples(
        const int bitsPerSample, int rowSizeInBytes,
        const unsigned char* srcBufferRow, float* fbBufferRow)
    {
        switch (bitsPerSample)
        {
        case 8:
        {
            const char* srcPix = (const char*)srcBufferRow;
            for (int x = 0; x < rowSizeInBytes; ++x)
            {
                (*fbBufferRow++) =
                    float(*srcPix++) / SAMPLEFORMAT_INT8_NORM_VALUE;
            }
        }
        break;

        case 16:
        {
            const short int* srcPix = (const short int*)srcBufferRow;
            for (int x = 0; x < rowSizeInBytes; x += 2)
            {
                (*fbBufferRow++) =
                    float(*srcPix++) / SAMPLEFORMAT_INT16_NORM_VALUE;
            }
        }
        break;

        case 32:
        {
            const int* srcPix = (const int*)srcBufferRow;
            for (int x = 0; x < rowSizeInBytes; x += 4)
            {
                (*fbBufferRow++) =
                    float(*srcPix++) / SAMPLEFORMAT_INT32_NORM_VALUE;
            }
            break;
        }

        default:
            break;
        }
    }

    static void readContiguousScanlineImage3Planar(TIFF* tif, int w, int h,
                                                   FrameBuffer& img)
    {
        unsigned short orient = ORIENTATION_TOPLEFT;
        bool flip = false;
        bool flop = false;
        TIFFGetField(tif, TIFFTAG_ORIENTATION, &orient);
        flip = orient == ORIENTATION_TOPLEFT || orient == ORIENTATION_TOPRIGHT;
        flop = orient == ORIENTATION_TOPRIGHT || orient == ORIENTATION_BOTRIGHT;

        unsigned short sampleFormat = DEFAULT_TIFFTAG_SAMPLEFORMAT_VALUE;
        unsigned short bitsPerSample = DEFAULT_TIFFTAG_BITSPERSAMPLE_VALUE;
        TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitsPerSample);
        TIFFGetField(tif, TIFFTAG_SAMPLEFORMAT, &sampleFormat);

        tsize_t rowsize = 0;
        unsigned char* buf = 0;
        if (sampleFormat == SAMPLEFORMAT_INT)
        {
            rowsize = TIFFScanlineSize(tif);
            buf = (unsigned char*)_TIFFmalloc(rowsize);
        }

        for (int y = 0; y < h; ++y)
        {
            int flipY = flip ? h - y - 1 : y;

            if (sampleFormat == SAMPLEFORMAT_INT)
            {
                if (TIFFReadScanline(tif, buf, y) == -1)
                {
                    // Incomplete image read, just stop and return what we did
                    // get
                    break;
                }

                float* fbBuf = img.scanline<float>(flipY);

                storeIntAsNormalizedFloatSamples(bitsPerSample, rowsize, buf,
                                                 fbBuf);

                memcpy(fbBuf, buf, rowsize);
            }
            else
            {
                if (TIFFReadScanline(tif, img.scanline<unsigned char>(flipY), y)
                    == -1)
                {
                    // Incomplete image read, just stop and return what we did
                    // get
                    break;
                }
            }
        }

        if (buf)
            _TIFFfree(buf);

        if (flop)
        {
            img.setOrientation(FrameBuffer::BOTTOMRIGHT);
        }
    }

    static void readContiguousScanlineImage(TIFF* tif, int w, int h,
                                            FrameBuffer& img)
    {
        unsigned short orient = ORIENTATION_TOPLEFT;
        bool flip = false;
        bool flop = false;
        TIFFGetField(tif, TIFFTAG_ORIENTATION, &orient);
        flip = orient == ORIENTATION_TOPLEFT || orient == ORIENTATION_TOPRIGHT;
        flop = orient == ORIENTATION_TOPRIGHT || orient == ORIENTATION_BOTRIGHT;

        unsigned short sampleFormat = DEFAULT_TIFFTAG_SAMPLEFORMAT_VALUE;
        unsigned short bitsPerSample = DEFAULT_TIFFTAG_BITSPERSAMPLE_VALUE;
        TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitsPerSample);
        TIFFGetField(tif, TIFFTAG_SAMPLEFORMAT, &sampleFormat);

        tsize_t rowsize = 0;
        unsigned char* buf = 0;
        if (sampleFormat == SAMPLEFORMAT_INT)
        {
            rowsize = TIFFScanlineSize(tif);
            buf = (unsigned char*)_TIFFmalloc(rowsize);
        }

        //
        //  Because we use TIFF a lot, and tiff files are stored by default
        //  in TOPLEFT orientation, we'll "correct" it to make it bottom
        //  right. For scanline images this is not a huge hit -- we just
        //  read the image backwards. For orther orientations we'll set the
        //  FB image orientation
        //

        for (int y = 0; y < h; ++y)
        {
            int flipY = flip ? h - y - 1 : y;

            if (sampleFormat == SAMPLEFORMAT_INT)
            {
                if (TIFFReadScanline(tif, buf, y) == -1)
                {
                    // Incomplete image read, just stop and return what we did
                    // get
                    break;
                }

                float* fbBuf = img.scanline<float>(flipY);

                storeIntAsNormalizedFloatSamples(bitsPerSample, rowsize, buf,
                                                 fbBuf);

                memcpy(fbBuf, buf, rowsize);
            }
            else
            {
                if (TIFFReadScanline(tif, img.scanline<unsigned char>(flipY), y)
                    == -1)
                {
                    // Incomplete image read, just stop and return what we did
                    // get
                    break;
                }
            }
        }

        if (buf)
            _TIFFfree(buf);

        if (flop)
        {
            img.setOrientation(FrameBuffer::BOTTOMRIGHT);
        }
    }

    template <typename T>
    static void copyScanlineSamples(const T* src, T* dest, int width, int copy,
                                    int skip)
    {
        for (int i = 0; i < width; i++)
        {
            for (int c = 0; c < copy; c++)
            {
                *dest++ = *src++;
            }

            src += skip;
        }
    }

    template <typename T>
    static void copyAndNormalizeScanlineSamples(const T* src, float* dest,
                                                int width, int copy, int skip,
                                                float normValue)
    {
        for (int i = 0; i < width; i++)
        {
            for (int c = 0; c < copy; c++)
            {
                *dest++ = float(*src++) / normValue;
            }

            src += skip;
        }
    }

    static void readContiguousScanlineImageOverflow(TIFF* tif, int w, int h,
                                                    FrameBuffer& img)
    {
        //
        //  This function is used when we have an interleaved TIFF with more
        //  than 4 channels. The TwkFrameBuffer doesn't like more than 4
        //  interleaved channels, so we'll decode the scanline and copy only the
        //  first 4 samples from each pixel.
        //

        unsigned short samplesPerPixel = DEFAULT_TIFFTAG_SAMPLESPERPIXEL_VALUE;
        TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel);

        unsigned short orient = ORIENTATION_TOPLEFT;
        bool flip = false;
        bool flop = false;
        TIFFGetField(tif, TIFFTAG_ORIENTATION, &orient);
        flip = orient == ORIENTATION_TOPLEFT || orient == ORIENTATION_TOPRIGHT;
        flop = orient == ORIENTATION_TOPRIGHT || orient == ORIENTATION_BOTRIGHT;

        unsigned short sampleFormat = DEFAULT_TIFFTAG_SAMPLEFORMAT_VALUE;
        unsigned short bitsPerSample = DEFAULT_TIFFTAG_BITSPERSAMPLE_VALUE;
        TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitsPerSample);
        TIFFGetField(tif, TIFFTAG_SAMPLEFORMAT, &sampleFormat);

        if (unsigned char* buf =
                (unsigned char*)_TIFFmalloc(TIFFScanlineSize(tif)))
        {
            //
            //  Because we use TIFF a lot, and tiff files are stored by default
            //  in TOPLEFT orientation, we'll "correct" it to make it bottom
            //  right. For scanline images this is not a huge hit -- we just
            //  read the image backwards. For orther orientations we'll set the
            //  FB image orientation
            //

            for (int y = 0; y < h; ++y)
            {
                int flipY = flip ? h - y - 1 : y;

                if (TIFFReadScanline(tif, buf, y) == -1)
                {
                    // Incomplete image read, just stop and return what we did
                    // get
                    break;
                }

                if (sampleFormat == SAMPLEFORMAT_INT)
                {
                    switch (bitsPerSample)
                    {
                    case 8:
                        copyAndNormalizeScanlineSamples(
                            (unsigned char*)buf, img.scanline<float>(flipY), w,
                            4, samplesPerPixel - 4,
                            SAMPLEFORMAT_INT8_NORM_VALUE);
                        break;
                    case 16:
                        copyAndNormalizeScanlineSamples(
                            (unsigned short*)buf, img.scanline<float>(flipY), w,
                            4, samplesPerPixel - 4,
                            SAMPLEFORMAT_INT16_NORM_VALUE);
                        break;
                    case 32:
                        copyAndNormalizeScanlineSamples(
                            (float*)buf, img.scanline<float>(flipY), w, 4,
                            samplesPerPixel - 4, SAMPLEFORMAT_INT32_NORM_VALUE);
                        break;
                    }
                }
                else
                {
                    switch (bitsPerSample)
                    {
                    case 1:
                    case 8:
                        copyScanlineSamples((unsigned char*)buf,
                                            img.scanline<unsigned char>(flipY),
                                            w, 4, samplesPerPixel - 4);
                        break;
                    case 16:
                        copyScanlineSamples((unsigned short*)buf,
                                            img.scanline<unsigned short>(flipY),
                                            w, 4, samplesPerPixel - 4);
                        break;
                    case 32:
                        copyScanlineSamples((float*)buf,
                                            img.scanline<float>(flipY), w, 4,
                                            samplesPerPixel - 4);
                        break;
                    }
                }
            }

            _TIFFfree(buf);
        }

        if (flop)
        {
            img.setOrientation(FrameBuffer::BOTTOMRIGHT);
        }
    }

    static void readContiguousScanlineImageAsPlanar(TIFF* tif, int w, int h,
                                                    FrameBuffer& img)
    {
        //
        //  This function is used when we have an interleaved TIFF with more
        //  than 4 channels. In this case the FrameBuffer should be planar and
        //  we'll have to copy the interleaved scanline to the planes.
        //

        assert(img.isPlanar());

        unsigned short samplesPerPixel = DEFAULT_TIFFTAG_SAMPLESPERPIXEL_VALUE;
        TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel);

        unsigned short orient = ORIENTATION_TOPLEFT;
        bool flip = false;
        bool flop = false;
        TIFFGetField(tif, TIFFTAG_ORIENTATION, &orient);
        flip = orient == ORIENTATION_TOPLEFT || orient == ORIENTATION_TOPRIGHT;
        flop = orient == ORIENTATION_TOPRIGHT || orient == ORIENTATION_BOTRIGHT;

        unsigned short sampleFormat = DEFAULT_TIFFTAG_SAMPLEFORMAT_VALUE;
        unsigned short bitsPerSample = DEFAULT_TIFFTAG_BITSPERSAMPLE_VALUE;
        TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitsPerSample);
        TIFFGetField(tif, TIFFTAG_SAMPLEFORMAT, &sampleFormat);

        if (unsigned char* buf =
                (unsigned char*)_TIFFmalloc(TIFFScanlineSize(tif)))
        {
            //
            //  Because we use TIFF a lot, and tiff files are stored by default
            //  in TOPLEFT orientation, we'll "correct" it to make it bottom
            //  right. For scanline images this is not a huge hit -- we just
            //  read the image backwards. For orther orientations we'll set the
            //  FB image orientation
            //

            for (int y = 0; y < h; ++y)
            {
                int flipY = flip ? h - y - 1 : y;

                if (TIFFReadScanline(tif, buf, y) == -1)
                {
                    // Incomplete image read, just stop and return what we did
                    // get
                    break;
                }

                FrameBuffer* fbuf = &img;

                for (int c = 0;
                     c < img.planeSize() && c < samplesPerPixel && fbuf; c++)
                {

                    if (sampleFormat == SAMPLEFORMAT_INT)
                    {
                        switch (bitsPerSample)
                        {
                        case 8:
                            copyAndNormalizeScanlineSamples(
                                (unsigned char*)buf + c,
                                &fbuf->pixel<float>(0, flipY), w, 1,
                                samplesPerPixel - 1,
                                SAMPLEFORMAT_INT8_NORM_VALUE);
                            break;
                        case 16:
                            copyAndNormalizeScanlineSamples(
                                (unsigned short*)buf + c,
                                &fbuf->pixel<float>(0, flipY), w, 1,
                                samplesPerPixel - 1,
                                SAMPLEFORMAT_INT16_NORM_VALUE);
                            break;
                        case 32:
                            copyAndNormalizeScanlineSamples(
                                (float*)buf + c, &fbuf->pixel<float>(0, flipY),
                                w, 1, samplesPerPixel - 1,
                                SAMPLEFORMAT_INT32_NORM_VALUE);
                            break;
                        }
                    }
                    else
                    {
                        switch (bitsPerSample)
                        {
                        case 1:
                        case 8:
                            copyScanlineSamples(
                                (unsigned char*)buf + c,
                                &fbuf->pixel<unsigned char>(0, flipY), w, 1,
                                samplesPerPixel - 1);
                            break;
                        case 16:
                            copyScanlineSamples(
                                (unsigned short*)buf + c,
                                &fbuf->pixel<unsigned short>(0, flipY), w, 1,
                                samplesPerPixel - 1);
                            break;
                        case 32:
                            copyScanlineSamples((float*)buf + c,
                                                &fbuf->pixel<float>(0, flipY),
                                                w, 1, samplesPerPixel - 1);
                            break;
                        }
                    }

                    fbuf = fbuf->nextPlane();
                }
            }

            _TIFFfree(buf);
        }

        if (flop)
        {
            img.setOrientation(FrameBuffer::BOTTOMRIGHT);
        }
    }

    static void readPlanarScanlineImage(TIFF* tif, int w, int h, int planes,
                                        FrameBuffer& img)
    {
        unsigned short orient = ORIENTATION_TOPLEFT;
        bool flip = false;
        bool flop = false;
        TIFFGetField(tif, TIFFTAG_ORIENTATION, &orient);
        flip = orient == ORIENTATION_TOPLEFT || orient == ORIENTATION_TOPRIGHT;
        flop = orient == ORIENTATION_TOPRIGHT || orient == ORIENTATION_BOTRIGHT;

        unsigned short sampleFormat = DEFAULT_TIFFTAG_SAMPLEFORMAT_VALUE;
        unsigned short bitsPerSample = DEFAULT_TIFFTAG_BITSPERSAMPLE_VALUE;
        TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitsPerSample);
        TIFFGetField(tif, TIFFTAG_SAMPLEFORMAT, &sampleFormat);

        //
        //  Because we use TIFF a lot, and tiff files are stored by default
        //  in TOPLEFT orientation, we'll "correct" it to make it bottom
        //  right. For scanline images this is not a huge hit -- we just
        //  read the image backwards. For orther orientations we'll set the
        //  FB image orientation
        //

        FrameBuffer* fb = &img;

        tsize_t rowsize = 0;
        unsigned char* buf = 0;
        if (sampleFormat == SAMPLEFORMAT_INT)
        {
            rowsize = TIFFScanlineSize(tif);
            buf = (unsigned char*)_TIFFmalloc(rowsize);
        }

        for (int p = 0; p < planes && fb; ++p)
        {
            for (int y = 0; y < h; ++y)
            {
                int flipY = flip ? h - y - 1 : y;

                if (sampleFormat == SAMPLEFORMAT_INT)
                {
                    if (TIFFReadScanline(tif, buf, y, p) == -1)
                    {
                        // Incomplete image read, just stop and return what we
                        // did get
                        break;
                    }

                    float* fbBuf = &fb->pixel<float>(0, flipY);

                    storeIntAsNormalizedFloatSamples(bitsPerSample, rowsize,
                                                     buf, fbBuf);

                    memcpy(fbBuf, buf, rowsize);
                }
                else
                {
                    if (TIFFReadScanline(
                            tif, &fb->pixel<unsigned char>(0, flipY), y, p)
                        == -1)
                    {
                        // Incomplete image read, just stop and return what we
                        // did get
                        break;
                    }
                }
            }

            fb = fb->nextPlane();
        }

        if (buf)
            _TIFFfree(buf);

        if (flop)
        {
            img.setOrientation(FrameBuffer::BOTTOMRIGHT);
        }
    }

    static void readContiguousTiledImage(TIFF* tif, int w, int h,
                                         FrameBuffer& img)
    {
        tsize_t rowsize = TIFFTileRowSize(tif);

        unsigned short orient = ORIENTATION_TOPLEFT;
        bool flip = false;
        bool flop = false;
        TIFFGetField(tif, TIFFTAG_ORIENTATION, &orient);
        flip = orient == ORIENTATION_TOPLEFT || orient == ORIENTATION_TOPRIGHT;
        flop = orient == ORIENTATION_TOPRIGHT || orient == ORIENTATION_BOTRIGHT;

        if (flip)
        {
            img.setOrientation(flop ? FrameBuffer::TOPRIGHT
                                    : FrameBuffer::TOPLEFT);
        }
        else
        {
            img.setOrientation(flop ? FrameBuffer::BOTTOMRIGHT
                                    : FrameBuffer::NATURAL);
        }

        if (unsigned char* buf = (unsigned char*)_TIFFmalloc(TIFFTileSize(tif)))
        {
            uint32 tw, th, w, h;

            TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
            TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
            TIFFGetField(tif, TIFFTAG_TILEWIDTH, &tw);
            TIFFGetField(tif, TIFFTAG_TILELENGTH, &th);

            unsigned short sampleFormat = DEFAULT_TIFFTAG_SAMPLEFORMAT_VALUE;
            unsigned short bitsPerSample = DEFAULT_TIFFTAG_BITSPERSAMPLE_VALUE;
            TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitsPerSample);
            TIFFGetField(tif, TIFFTAG_SAMPLEFORMAT, &sampleFormat);

            // These checks are to handle the case where the
            // tile is larger than the image size.
            //
            uint32 copy_rowsize = ((w < tw) ? (w * rowsize / tw) : rowsize);

            for (uint32 row = 0; row < h; row += th)
            {
                for (uint32 col = 0; col < w; col += tw)
                {
                    if (TIFFReadTile(tif, buf, col, row, 0, 0) < 0)
                    {
                        _TIFFfree(buf);
                        return; // just return on partially read image
                    }
                    else
                    {
                        //
                        //  Copy the tile row over to the fb
                        //

                        for (int y = 0; y < th && row + y < h; y++)
                        {
                            if (sampleFormat == SAMPLEFORMAT_INT)
                            {
                                storeIntAsNormalizedFloatSamples(
                                    bitsPerSample, copy_rowsize,
                                    buf + y * rowsize,
                                    &img.pixel<float>(col, row + y));
                            }
                            else
                            {
                                unsigned char* s =
                                    &img.pixel<unsigned char>(col, row + y);
                                memcpy(s, buf + y * rowsize, copy_rowsize);
                            }
                        }
                    }
                }
            }

            _TIFFfree(buf);
        }
    }

    static void readPlanarTiledImage(TIFF* tif, int w, int h, FrameBuffer& img)
    {
        tsize_t rowsize = TIFFTileRowSize(tif);
        unsigned short orient = ORIENTATION_TOPLEFT;
        bool flip = false;
        bool flop = false;
        FrameBuffer::Orientation o;

        TIFFGetField(tif, TIFFTAG_ORIENTATION, &orient);
        flip = orient == ORIENTATION_TOPLEFT || orient == ORIENTATION_TOPRIGHT;
        flop = orient == ORIENTATION_TOPRIGHT || orient == ORIENTATION_BOTRIGHT;

        if (flip)
        {
            o = flop ? FrameBuffer::TOPRIGHT : FrameBuffer::TOPLEFT;
        }
        else
        {
            o = flop ? FrameBuffer::BOTTOMRIGHT : FrameBuffer::NATURAL;
        }

        if (unsigned char* buf = (unsigned char*)_TIFFmalloc(TIFFTileSize(tif)))
        {
            uint32 tw, th, w, h;
            unsigned short d = DEFAULT_TIFFTAG_SAMPLESPERPIXEL_VALUE;
            unsigned short sampleFormat = DEFAULT_TIFFTAG_SAMPLEFORMAT_VALUE;
            unsigned short bitsPerSample = DEFAULT_TIFFTAG_BITSPERSAMPLE_VALUE;

            TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
            TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
            TIFFGetField(tif, TIFFTAG_TILEWIDTH, &tw);
            TIFFGetField(tif, TIFFTAG_TILELENGTH, &th);
            TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &d);
            TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitsPerSample);
            TIFFGetField(tif, TIFFTAG_SAMPLEFORMAT, &sampleFormat);

            FrameBuffer* fb = &img;

            // These checks are to handle the case where the
            // tile is larger than the image size.
            //
            uint32 copy_rowsize = ((w < tw) ? (w * rowsize / tw) : rowsize);

            for (uint32 plane = 0; plane < d && fb; plane++)
            {
                fb->setOrientation(o);

                for (uint32 row = 0; row < h; row += th)
                {
                    for (uint32 col = 0; col < w; col += tw)
                    {
                        if (TIFFReadTile(tif, buf, col, row, 0, plane) < 0)
                        {
                            _TIFFfree(buf);
                            return; // just return on partially read image
                        }
                        else
                        {
                            //
                            //  Copy the tile row over to the fb
                            //

                            for (int y = 0; y < th && row + y < h; y++)
                            {
                                if (sampleFormat == SAMPLEFORMAT_INT)
                                {
                                    storeIntAsNormalizedFloatSamples(
                                        bitsPerSample, copy_rowsize,
                                        buf + y * rowsize,
                                        &fb->pixel<float>(col, row + y));
                                }
                                else
                                {
                                    unsigned char* s =
                                        &fb->pixel<unsigned char>(col, row + y);
                                    memcpy(s, buf + y * rowsize, copy_rowsize);
                                }
                            }
                        }
                    }
                }

                fb = fb->nextPlane();
            }

            _TIFFfree(buf);
        }
    }

    void IOtiff::getImageInfo(const std::string& filename, FBInfo& fbi) const
    {
#ifdef _MSC_VER
        TIFF* tif = TIFFOpenW(UNICODE_C_STR(filename.c_str()), "r");
#else
        TIFF* tif = TIFFOpen(UNICODE_C_STR(filename.c_str()), "r");
#endif

        if (!tif)
        {
            TWK_THROW_STREAM(Exception,
                             "TIFF cannot open \"" << filename << "\"");
        }

        unsigned short colorspace;
        unsigned short orient = ORIENTATION_TOPLEFT;

        TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &fbi.width);
        TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &fbi.height);
        TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &colorspace);
        TIFFGetField(tif, TIFFTAG_ORIENTATION, &orient);

        unsigned short samplesPerPixel = DEFAULT_TIFFTAG_SAMPLESPERPIXEL_VALUE;
        TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel);
        fbi.numChannels = samplesPerPixel;

        unsigned short bitsPerSample = DEFAULT_TIFFTAG_BITSPERSAMPLE_VALUE;
        TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitsPerSample);

        unsigned short sampleFormat = DEFAULT_TIFFTAG_SAMPLEFORMAT_VALUE;
        TIFFGetField(tif, TIFFTAG_SAMPLEFORMAT, &sampleFormat);

        uint16 config;
        TIFFGetField(tif, TIFFTAG_PLANARCONFIG, &config);

        float x_rez, y_rez;
        if (TIFFGetField(tif, TIFFTAG_XRESOLUTION, &x_rez)
            && TIFFGetField(tif, TIFFTAG_YRESOLUTION, &y_rez))
        {
            fbi.pixelAspect = x_rez != 0.0 ? y_rez / x_rez : 1.0;
            unsigned short unit = 0;

            if (TIFFGetField(tif, TIFFTAG_RESOLUTIONUNIT, &unit))
            {
                ostringstream str;

                switch (unit)
                {
                case 1:
                    str << "None";
                    break;
                case 2:
                    str << "Inch";
                    break;
                case 3:
                    str << "Centimeter";
                    break;
                default:
                    str << "Unknown (" << unit << ")";
                    break;
                }

                fbi.proxy.newAttribute("TIFF/ResolutionUnit", str.str());
            }

            fbi.proxy.setPixelAspectRatio(x_rez != 0.0 ? y_rez / x_rez : 1.0);
            fbi.proxy.newAttribute("TIFF/XResolution", x_rez);
            fbi.proxy.newAttribute("TIFF/YResolution", y_rez);
        }

        readAllTags(tif, fbi.proxy);

        switch (bitsPerSample)
        {
        case 1:
            fbi.dataType = FrameBuffer::BIT;
            break;
        case 8:
            if (sampleFormat == SAMPLEFORMAT_INT)
            {
                // We promote tif signed int types to
                // a float type framebuffer, normalized
                // by the unsigned 8 bit value.
                fbi.dataType = FrameBuffer::FLOAT;
            }
            else
            {
                fbi.dataType = FrameBuffer::UCHAR;
            }
            break;
        case 16:
            if (sampleFormat == SAMPLEFORMAT_INT)
            {
                // We promote tif signed int types to
                // a float type framebuffer, normalized
                // by the unsigned 16 bit value.
                fbi.dataType = FrameBuffer::FLOAT;
            }
            else
            {
                fbi.dataType = FrameBuffer::USHORT;
            }
            break;
        case 32:
            if (sampleFormat == SAMPLEFORMAT_INT)
            {
                // We promote tif signed int types to
                // a float type framebuffer, normalized
                // by the unsigned 16 bit value for this case.
                fbi.dataType = FrameBuffer::FLOAT;
            }
            else if (sampleFormat == SAMPLEFORMAT_UINT)
            {
                fbi.dataType = FrameBuffer::UINT;
            }
            else
            {
                fbi.dataType = FrameBuffer::FLOAT;
            }
            break;
        default:
            TIFFClose(tif);
            TWK_THROW_STREAM(Exception, "Sorry, unsupported bit depth.");
        }

        bool readAsRGBA = false;

        if ((colorspace != PHOTOMETRIC_RGB
             && colorspace != PHOTOMETRIC_MINISBLACK
             && colorspace != PHOTOMETRIC_MINISWHITE)
            || (m_addAlphaTo3Channel && samplesPerPixel == 3)
            || (config == PLANARCONFIG_CONTIG && samplesPerPixel > 4))
        {
            readAsRGBA = true;
        }

        if (readAsRGBA)
        {
            // TIFFReadRGBAImage(tif, width, height, p, 0);
            fbi.orientation = FrameBuffer::NATURAL;
        }
        else if (TIFFIsTiled(tif))
        {
            // readContiguousTiledImage(tif, width, height, fb);
            //  or readPlanarTiledImage(tif, width, height, fb);
            bool flip = false;
            bool flop = false;
            flip =
                orient == ORIENTATION_TOPLEFT || orient == ORIENTATION_TOPRIGHT;
            flop = orient == ORIENTATION_TOPRIGHT
                   || orient == ORIENTATION_BOTRIGHT;

            if (flip)
                fbi.orientation =
                    (flop ? FrameBuffer::TOPRIGHT : FrameBuffer::TOPLEFT);
            else
                fbi.orientation =
                    (flop ? FrameBuffer::BOTTOMRIGHT : FrameBuffer::NATURAL);
        }
        else
        {
            bool flip = false;
            bool flop = false;
            flop = orient == ORIENTATION_TOPRIGHT
                   || orient == ORIENTATION_BOTRIGHT;
            fbi.orientation =
                flop ? FrameBuffer::BOTTOMRIGHT : FrameBuffer::NATURAL;
        }

        TIFFClose(tif);
    }

    struct StreamData
    {
        StreamData(const std::string& filename,
                   FileStream::Type type = FileStream::Buffering,
                   size_t chunkSize = 61440, int maxInFlight = 16)
            : stream(filename, type, chunkSize, maxInFlight)
            , pos(0)
        {
            pos = (char*)stream.data();
        }

        FileStream stream;
        char* pos;
    };

    static tsize_t readproc(thandle_t userdata, tdata_t data, tsize_t size)
    {
        StreamData* stream = (StreamData*)userdata;
        memcpy(data, stream->pos, size);
        stream->pos += size;
        return size;
    }

    static tsize_t writeproc(thandle_t userdata, tdata_t data, tsize_t size)
    {
        StreamData* stream = (StreamData*)userdata;
        // dummy -- we don't write using FileStream
        return size;
    }

    static toff_t seekproc(thandle_t userdata, toff_t offset, int whence)
    {
        StreamData* stream = (StreamData*)userdata;
        switch (whence)
        {
        case SEEK_SET:
            stream->pos = (char*)stream->stream.data();
            stream->pos += offset;
            break;
        case SEEK_CUR:
            stream->pos += offset;
            break;
        case SEEK_END:
            stream->pos = (char*)stream->stream.data();
            stream->pos += (offset + toff_t(stream->stream.size()));
            break;
        }

        return toff_t(stream->pos - (char*)(stream->stream.data()));
    }

    static int closeproc(thandle_t userdata) { return 0; }

    static toff_t sizeproc(thandle_t userdata)
    {
        StreamData* stream = (StreamData*)userdata;
        return toff_t(stream->stream.size());
    }

    void IOtiff::readImage(FrameBuffer& fb, const std::string& filename,
                           const ReadRequest& request) const
    {
        TIFF* tif = NULL;
        StreamData* stream = NULL;

        try
        {
            if (m_iotype == StandardIO)
            {
#ifdef _MSC_VER
                tif = TIFFOpenW(UNICODE_C_STR(filename.c_str()), "r");
#else
                tif = TIFFOpen(UNICODE_C_STR(filename.c_str()), "r");
#endif
            }
            else
            {
                stream = new StreamData(
                    filename, (FileStream::Type)((unsigned int)m_iotype - 1),
                    m_iosize, m_iomaxAsync);

                tif = TIFFClientOpen(filename.c_str(), "r", (thandle_t)stream,
                                     readproc, writeproc, seekproc, closeproc,
                                     sizeproc, NULL, NULL);
            }

            if (!tif)
            {
                TWK_THROW_STREAM(Exception,
                                 "TIFF: cannot open \"" << filename << "\"");
            }

            int width;
            int height;
            unsigned short bitsPerSample = DEFAULT_TIFFTAG_BITSPERSAMPLE_VALUE;
            FrameBuffer::DataType dataType = FrameBuffer::UCHAR;
            unsigned short samplesPerPixel =
                DEFAULT_TIFFTAG_SAMPLESPERPIXEL_VALUE;
            uint16 config;
            bool istexture = false;
            bool isshadow = false;
            char* texformat = 0;
            char* datetime = 0;
            unsigned short colorspace = 2;
            float x_rez, y_rez;
            uint16 numExtra = 0, *extraSamples = 0;
            unsigned short sampleFormat = DEFAULT_TIFFTAG_SAMPLEFORMAT_VALUE;

            TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &colorspace);
            TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
            TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);
            TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bitsPerSample);
            TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel);
            TIFFGetField(tif, TIFFTAG_SAMPLEFORMAT, &sampleFormat);
            TIFFGetField(tif, TIFFTAG_PLANARCONFIG, &config);
            TIFFGetField(tif, TIFFTAG_EXTRASAMPLES, &numExtra, &extraSamples);

#if 0
        if (TIFFGetField(tif, TIFFTAG_PIXAR_TEXTUREFORMAT, &texformat) == 1)
        {
            istexture = true;
            isshadow = !strcmp(texformat, "Shadow");
        }
#endif

            FrameBuffer* img = NULL;
            bool readAsRGBA = false;

            if ((colorspace != PHOTOMETRIC_RGB
                 && colorspace != PHOTOMETRIC_MINISBLACK
                 && colorspace != PHOTOMETRIC_MINISWHITE)
                || (m_addAlphaTo3Channel && samplesPerPixel == 3)
                || (config == PLANARCONFIG_CONTIG && samplesPerPixel > 4))
            {
                readAsRGBA = true;
                samplesPerPixel = 4;
            }

            switch (bitsPerSample)
            {
            case 1:
                dataType = FrameBuffer::UCHAR;
                break;

            case 8:
                if (sampleFormat == SAMPLEFORMAT_INT)
                {
                    // We promote tif signed int types to
                    // a float type framebuffer, normalized
                    // by the unsigned 8 bit value.
                    dataType = FrameBuffer::FLOAT;
                }
                else
                {
                    dataType = FrameBuffer::UCHAR;
                }
                break;

            case 16:
                if (sampleFormat == SAMPLEFORMAT_INT)
                {
                    // We promote tif signed int types to
                    // a float type framebuffer, normalized
                    // by the unsigned 16 bit value.
                    dataType = FrameBuffer::FLOAT;
                }
                else
                {
                    dataType = FrameBuffer::USHORT;
                }
                break;

            case 32:
                if (sampleFormat == SAMPLEFORMAT_INT)
                {
                    // We promote tif signed int types to
                    // a float type framebuffer, normalized
                    // by the unsigned 16 bit value for this case.
                    dataType = FrameBuffer::FLOAT;
                }
                else if (sampleFormat == SAMPLEFORMAT_UINT)
                {
                    dataType = FrameBuffer::UINT;
                }
                else
                {
                    dataType = FrameBuffer::FLOAT;
                }
                break;

            default:
                TWK_THROW_STREAM(UnsupportedException,
                                 "TIFF: Unsupported bit depth ("
                                     << bitsPerSample << ") trying to read "
                                     << filename);
            }

            if (config == PLANARCONFIG_SEPARATE)
            {
                const char* chanNames[] = {"R", "G", "B", "A", "Z",
                                           "X", "Y", "P", "D", "Q"};

                StringVector planeNames;

                for (int i = 0; i < samplesPerPixel && i < 10; i++)
                {
                    planeNames.push_back(string(chanNames[i]));
                }

                fb.restructurePlanar(width, height, planeNames, dataType,
                                     FrameBuffer::NATURAL);
            }
            else
            {
                fb.restructure(
                    width, height, 0, min((int)samplesPerPixel, 4),
                    dataType); // interleaved, we can only do 4 channels
            }

            string message = "Reading TIFF " + stl_ext::basename(filename);

            if (readAsRGBA)
            {
                uint32* p = fb.begin<uint32>();
                const uint32* e = fb.end<uint32>();

                TIFFReadRGBAImage(tif, width, height, p, 0);

#ifdef __BIG_ENDIAN__
                for (; p < e; p++)
                {
                    const uint32 i = *p;
                    *p = (uint32(TIFFGetR(i)) << 24)
                         | (uint32(TIFFGetG(i)) << 16)
                         | (uint32(TIFFGetB(i)) << 8) | uint32(TIFFGetA(i));
                }
#endif
            }
            else if (TIFFIsTiled(tif))
            {
                if (config == PLANARCONFIG_CONTIG)
                {
                    readContiguousTiledImage(tif, width, height, fb);
                    fb.newAttribute("TIFF/PlanarConfig",
                                    string("Tiled Contiguous"));
                }
                else
                {
                    readPlanarTiledImage(tif, width, height, fb);
                    fb.newAttribute("TIFF/PlanarConfig",
                                    string("Tiled Separate"));
                }
            }
            else
            {
                if (config == PLANARCONFIG_CONTIG)
                {
                    if (samplesPerPixel == 1
                        || (dataType != FrameBuffer::USHORT
                            && samplesPerPixel <= 4))
                    {
                        readContiguousScanlineImage(tif, width, height, fb);
                    }
                    else
                    {
                        const char* chanNames[] = {"R", "G", "B", "A", "Z",
                                                   "X", "Y", "P", "D", "Q"};

                        if (!fb.isPlanar())
                        {
                            StringVector planeNames;

                            for (int i = 0; i < samplesPerPixel && i < 10; i++)
                            {
                                planeNames.push_back(string(chanNames[i]));
                            }

                            fb.restructurePlanar(width, height, planeNames,
                                                 dataType,
                                                 FrameBuffer::NATURAL);
                        }
                        readContiguousScanlineImageAsPlanar(tif, width, height,
                                                            fb);
                    }

                    fb.newAttribute("TIFF/PlanarConfig", string("Contiguous"));
                }
                else
                {
                    readPlanarScanlineImage(tif, width, height, samplesPerPixel,
                                            fb);
                    fb.newAttribute("TIFF/PlanarConfig", string("Separate"));
                }
            }

            //
            //  Here we have a workaround for Maya writing the resolution tags
            //  wrong. Someday this should be replaced with a rule in our fancy
            //  rules system.
            //

            char* software = NULL;
            bool isMaya = false;
            unsigned int compression;

            if (TIFFGetField(tif, TIFFTAG_COMPRESSION, &compression))
            {
                string compressionName = tagToName(compression, compTags);
                fb.newAttribute("TIFF/Compression", compressionName);
            }

            if (TIFFGetField(tif, TIFFTAG_SOFTWARE, &software))
            {
                fb.newAttribute("TIFF/Software", string(software));

                if (!strncmp(software, "Maya", 4))
                {
                    isMaya = true;
                }
            }

            if (TIFFGetField(tif, TIFFTAG_XRESOLUTION, &x_rez)
                && TIFFGetField(tif, TIFFTAG_YRESOLUTION, &y_rez))
            {
                unsigned short unit = 0;

                if (TIFFGetField(tif, TIFFTAG_RESOLUTIONUNIT, &unit))
                {
                    ostringstream str;

                    switch (unit)
                    {
                    case 1:
                        str << "None";
                        break;
                    case 2:
                        str << "Inch";
                        break;
                    case 3:
                        str << "Centimeter";
                        break;
                    default:
                        str << "Unknown (" << unit << ")";
                        break;
                    }

                    fb.newAttribute("TIFF/ResolutionUnit", str.str());
                }

                if (!isMaya)
                {
                    fb.setPixelAspectRatio(x_rez != 0.0 ? y_rez / x_rez : 1.0);
                }

                fb.newAttribute("TIFF/XResolution", x_rez);
                fb.newAttribute("TIFF/YResolution", y_rez);
            }

            readAllTags(tif, fb);

            TIFFClose(tif);

#if 0
        if (isshadow)
        {
            TwkFB::normalize(&fb, true, true);
        }
#endif

            if (numExtra)
            {
                switch (extraSamples[0])
                {
                case EXTRASAMPLE_UNASSALPHA:
                    fb.attribute<string>("AlphaType") = "Unpremultipled";
                    break;
                case EXTRASAMPLE_ASSOCALPHA:
                    fb.attribute<string>("AlphaType") = "Premultiplied";
                    break;
                default:
                    break;
                }
            }

            //
            //  Most cards can't handle RGB or RGBA 16bit int textures.
            //
            if (fb.dataType() == FrameBuffer::USHORT && fb.numChannels() != 1
                && !fb.isPlanar())
            {
                //  cerr << "converting tiff to planar" << endl;
                FrameBufferVector fbv = split(&fb);
                fbv[0]->setIdentifier(fb.identifier());
                fb.copyAttributesTo(fbv[0]);
                for (int j = 1; j < fbv.size(); ++j)
                    fbv[0]->appendPlane(fbv[j]);
                fb.copyFrom(fbv[0]);
                delete fbv[0];
            }
        }
        catch (...)
        {
            delete stream;
            stream = 0;
            throw;
        }

        delete stream;
    }

    void IOtiff::writeImage(const FrameBuffer& img, const std::string& filename,
                            const WriteRequest& request) const
    {
#ifdef _MSC_VER
        TIFF* tif = TIFFOpenW(UNICODE_C_STR(filename.c_str()), "w");
#else
        TIFF* tif = TIFFOpen(filename.c_str(), "w");
#endif
        const FrameBuffer* outfb = &img;
        bool yryby = img.isYRYBY() || img.isYRYBYPlanar();

        if (!tif)
        {
            TWK_THROW_STREAM(IOException, "TIFF: cannot open "
                                              << filename << " for writing");
        }

        switch (outfb->dataType())
        {
        case FrameBuffer::HALF:
            outfb = copyConvert(outfb, FrameBuffer::FLOAT);
            // fall through
        case FrameBuffer::FLOAT:
            TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
            break;
        case FrameBuffer::USHORT:
        case FrameBuffer::UCHAR:
            TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
            break;
        case FrameBuffer::PACKED_R10_G10_B10_X2:
        case FrameBuffer::PACKED_X2_B10_G10_R10:
        case FrameBuffer::PACKED_Y8_Cb8_Y8_Cr8:
        case FrameBuffer::PACKED_Cb8_Y8_Cr8_Y8:
        {
            const FrameBuffer* nfb = outfb;
            outfb = convertToLinearRGB709(outfb);
            delete nfb;
            TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
            break;
        }
        default:
            TWK_THROW_STREAM(Exception, "TIFF: Unsupported data format: "
                                            << outfb->dataType());
        }

        //
        //  If its planar and the request is to convert to packed or for
        //  the "common" format, then make the image packed RGB.
        //

        if (img.isPlanar()
            && (!request.keepPlanar || request.preferCommonFormat || yryby))
        {
            const FrameBuffer* fb = outfb;
            outfb = mergePlanes(outfb);
            if (fb != &img)
                delete fb;
        }

        //
        //  Convert YUV, YRYBY, or non-RGB 709 images to RGB 709 In the
        //  case of YRYBY, TIFF can't really store it so we need to
        //  convert no matter what.
        //

        if ((request.preferCommonFormat
             && (outfb->hasPrimaries() || outfb->isYUV()))
            || yryby)
        {
            const FrameBuffer* fb = outfb;
            outfb = convertToLinearRGB709(outfb);
            if (fb != &img)
                delete fb;
        }

        assert(!outfb->isPlanar()); // can't write the planar images yet

        unsigned short tifforientation;

        //
        //  Don't ask
        //

        switch (outfb->orientation())
        {
        case FrameBuffer::NATURAL:
            tifforientation = ORIENTATION_TOPLEFT;
            break;
        case FrameBuffer::TOPLEFT:
            tifforientation = ORIENTATION_BOTLEFT;
            break;
        case FrameBuffer::TOPRIGHT:
            tifforientation = ORIENTATION_BOTRIGHT;
            break;
        case FrameBuffer::BOTTOMRIGHT:
            tifforientation = ORIENTATION_TOPRIGHT;
            break;
        default:
            break;
        };

        TIFFSetField(tif, TIFFTAG_ORIENTATION, tifforientation);

        //
        //  Basic geometry
        //

        TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, outfb->width());
        TIFFSetField(tif, TIFFTAG_IMAGELENGTH, outfb->height());
        TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, outfb->numChannels());
        TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8 * outfb->bytesPerChannel());
        TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
        TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, outfb->height());

        //
        //  Compression
        //

        unsigned int compression = COMPRESSION_DEFLATE; // default zip compress

        if (unsigned int ctag =
                nameToTag(request.compression.c_str(), compTags))
        {
            compression = ctag;
        }
        else if (request.compression != "")
        {
            cerr << "WARNING: IOtiff: unknown compression type "
                 << request.compression << ", using DEFLATE instead" << endl;

            compression = COMPRESSION_DEFLATE;
        }

        TIFFSetField(tif, TIFFTAG_COMPRESSION, compression);

        //
        //  Photometric interpretation
        //

        if (outfb->isYUV())
        {
            TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_YCBCR);
        }
        else
        {
            TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
        }

        if (outfb->hasChannel("A") && outfb->numChannels() > 1)
        {
            uint16 extrasamples = 1;
            uint16 sampleinfo[1] = {EXTRASAMPLE_ASSOCALPHA};
            TIFFSetField(tif, TIFFTAG_EXTRASAMPLES, extrasamples, sampleinfo);
        }

        for (int y = 0; y < outfb->height(); ++y)
        {
            int flipY = outfb->height() - y - 1;

            if (TIFFWriteScanline(
                    tif, (void*)outfb->scanline<unsigned char>(flipY), y, 0)
                == -1)
            {
                TWK_THROW_STREAM(IOException,
                                 "TIFF: Error write scanline " << filename);
            }
        }

        TIFFClose(tif);
        if (outfb != &img)
            delete outfb;
    }

} //  End namespace TwkFB
