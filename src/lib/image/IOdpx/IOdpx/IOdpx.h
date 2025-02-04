//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IOdpx__IOdpx__h__
#define __IOdpx__IOdpx__h__
#include <TwkExc/TwkExcException.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkUtil/ByteSwap.h>
#include <TwkFB/StreamingIO.h>
#include <fstream>

namespace TwkFB
{

    //
    //  class IOdpx
    //

    class IOdpx : public StreamingFrameBufferIO
    {
    public:
        //
        //  Types
        //

        typedef unsigned int U32;
        typedef unsigned int UInt;
        typedef unsigned short U16;
        typedef unsigned char U8;
        typedef int S32;
        typedef float R32;
        typedef char ASCII;

        enum StorageFormat
        {
            RGB8,        // Good drawing speed
            RGB16,       // Good fidelity, slowest to draw -- default
            RGBA8,       // Good drawing speed
            RGBA16,      // Good fidelity, slowest to draw -- default
            RGB10_A2,    // Fastest to read, fast to draw
            A2_BGR10,    // Slow to read, fastest 10bit format to draw (nvidia)
            RGB8_PLANAR, // Fastest to draw, second fastest to read
            RGB16_PLANAR // Faster to draw, good fidelity
        };

        enum DPXOrientation
        {
            DPX_TOP_LEFT = 0,
            DPX_TOP_RIGHT = 1,
            DPX_BOTTOM_LEFT = 2,
            DPX_BOTTOM_RIGHT = 3,
            DPX_ROTATED_TOP_LEFT = 4,
            DPX_ROTATED_TOP_RIGHT = 6,
            DPX_ROTATED_BOTTOM_LEFT = 5,
            DPX_ROTATED_BOTTOM_RIGHT = 7
        };

        enum DPXColorSpace
        {
            DPX_USER_DEFINED_COLOR = 0,
            DPX_PRINTING_DENSITY = 1,
            DPX_LINEAR = 2,      // transfer only
            DPX_LOGARITHMIC = 3, // transfer only
            DPX_UNSPECIFIED_VIDEO = 4,
            DPX_SMPTE274M = 5,     // 1080 HD
            DPX_ITU_R709 = 6,      // Rec 709
            DPX_ITU_R601_625L = 7, // 625 lines Rec 601
            DPX_ITU_R601_525L = 8, // 525 lines Rec 601
            DPX_NTSC = 9,
            DPX_PAL = 10,
            DPX_Z_LINEAR = 11,     // transfer only
            DPX_Z_HOMOGENEOUS = 12 // transfer only
        };

        enum DPXPacking
        {
            DPX_PAD_NONE = 0,
            DPX_PAD_LSB_WORD = 1,
            DPX_PAD_MSB_WORD = 2
        };

        enum DPXPixelFormat
        {
            DPX_PIXEL_UNKNOWN = 0,
            DPX_PIXEL_R = 1,
            DPX_PIXEL_G = 2,
            DPX_PIXEL_B = 3,
            DPX_PIXEL_A = 4,
            DPX_PIXEL_Y = 6,
            DPX_PIXEL_CbCr = 7, // subsampled by 2
            DPX_PIXEL_Z = 8,
            DPX_PIXEL_COMPOSITE_VIDEO = 9,
            DPX_PIXEL_RGB = 50,
            DPX_PIXEL_RGBA = 51,
            DPX_PIXEL_ABGR = 52,
            DPX_PIXEL_CbYCrY_422 = 100, // (2vuy?) (SMPTE 125M)
            DPX_PIXEL_CbYACrYA_4224 = 101,
            DPX_PIXEL_CbYCr_444 = 102,
            DPX_PIXEL_CbYCrA_4444 = 103,
            DPX_PIXEL_USER_2_COMPONENT = 150,
            DPX_PIXEL_USER_3_COMPONENT = 151,
            DPX_PIXEL_USER_4_COMPONENT = 152,
            DPX_PIXEL_USER_5_COMPONENT = 153,
            DPX_PIXEL_USER_6_COMPONENT = 154,
            DPX_PIXEL_USER_7_COMPONENT = 155,
            DPX_PIXEL_USER_8_COMPONENT = 156
        };

        //
        //  Fields with a "C:" in the comments are the "core set" that are
        //  required to be filled in with valid information
        //

        struct DPXFileInformation
        {
            U32 magic_num; // C: magic number 0x53445058 (SDPX) or 0x58504453
                           // (XPDS)
            U32 offset;    // C: offset to image data in bytes
            ASCII
            vers[8]; // C: which header format version is being used (v1.0)
            U32 file_size;        // C: file size in bytes
            U32 ditto_key;        // read time short cut - 0 = same, 1 = new
            U32 gen_hdr_size;     // generic header length in bytes
            U32 ind_hdr_size;     // industry header length in bytes
            U32 user_data_size;   // user-defined data length in bytes
            ASCII file_name[100]; // image file name
            ASCII
            create_time[24];    // file creation date "yyyy:mm:dd:hh:mm:ss:LTZ"
            ASCII creator[100]; // file creator's name
            ASCII project[200]; // project name
            ASCII copyright[200]; // right to use or copyright info
            U32 key;              // encryption ( FFFFFFFF = unencrypted )
            ASCII Reserved[104];  // reserved field TBD (need to pad)

            void swap()
            {
                TwkUtil::swapWords(&magic_num, 2);
                TwkUtil::swapWords(&file_size, 5);
                TwkUtil::swapWords(&key, 1);
            }
        };

        struct DPXImageInformation
        {
            U16 orientation;         // C: image orientation
            U16 element_number;      // C: number of image elements
            U32 pixels_per_line;     // C: or x value
            U32 lines_per_image_ele; // C: or y value, per element

            struct _image_element
            {
                U32 data_sign;    // C: data sign (0 = unsigned, 1 = signed )
                                  // "Core set images are unsigned"
                U32 ref_low_data; // reference low data code value
                R32 ref_low_quantity;  // reference low quantity represented
                U32 ref_high_data;     // reference high data code value
                R32 ref_high_quantity; // reference high quantity represented
                U8 descriptor;         // C: descriptor for image element
                U8 transfer;     // C: transfer characteristics for element
                U8 colorimetric; // C: colormetric specification for element
                U8 bit_size;     // C: bit size for element
                U16 packing;     // C: packing for element
                U16 encoding;    // C: encoding for element
                U32 data_offset; // C: offset to data of element
                U32 eol_padding; // end of line padding used in element
                U32 eo_image_padding;  // end of image padding used in element
                ASCII description[32]; // description of element
            } image_element[8];        // NOTE THERE ARE EIGHT OF THESE

            U8 reserved[52]; // reserved for future use (padding)

            void swap()
            {
                TwkUtil::swapShorts(&this->orientation, 2);
                TwkUtil::swapWords(&this->pixels_per_line, 2);

                for (int i = 0; i < 8; i++)
                {
                    TwkUtil::swapWords(&this->image_element[i].data_sign, 5);
                    TwkUtil::swapShorts(&this->image_element[i].packing, 2);
                    TwkUtil::swapWords(&this->image_element[i].data_offset, 3);
                }
            }
        };

        struct DPXImageOrientation
        {
            U32 x_offset;            // X offset
            U32 y_offset;            // Y offset
            R32 x_center;            // X center
            R32 y_center;            // Y center
            U32 x_orig_size;         // X original size
            U32 y_orig_size;         // Y original size
            ASCII file_name[100];    // source image file name
            ASCII creation_time[24]; // source image creation date and time
            ASCII input_dev[32];     // input device name
            ASCII input_serial[32];  // input device serial number
            U16 border_XL;
            U16 border_XR;
            U16 border_YT;
            U16 border_YB;
            U32 pixel_aspect_H; // pixel aspect ratio (H:V)
            U32 pixel_aspect_V; // pixel aspect ratio (H:V)

            // SHOULD THIS BE 20?
            U8 reserved[28]; // reserved for future use (padding)

            void swap()
            {
                TwkUtil::swapWords(&x_offset, 6);
                TwkUtil::swapShorts(&border_XL, 4);
                TwkUtil::swapWords(&pixel_aspect_H, 2);
            }
        };

        struct DPXFilmHeader
        {
            ASCII film_mfg_id[2]; // film manufacturer ID code (2 digits from
                                  // film edge code)
            ASCII film_type[2];   // file type (2 digits from film edge code)
            ASCII offset[2];  // offset in perfs (2 digits from film edge code)
            ASCII prefix[6];  // prefix (6 digits from film edge code)
            ASCII count[4];   // count (4 digits from film edge code)
            ASCII format[32]; // format (i.e. academy)
            U32 frame_position;    // frame position in sequence
            U32 sequence_len;      // sequence length in frames
            U32 held_count;        // held count (1 = default)
            R32 frame_rate;        // frame rate of original in frames/sec
            R32 shutter_angle;     // shutter angle of camera in degrees
            ASCII frame_id[32];    // frame identification (i.e. keyframe)
            ASCII slate_info[100]; // slate information
            U8 reserved[56];       // reserved for future use (padding)

            void swap() { TwkUtil::swapWords(&frame_position, 5); }
        };

        struct DPXTelevisionHeader
        {
            U32 tim_code;    // SMPTE time code
            U32 userBits;    // SMPTE user bits
            U8 interlace;    // interlace ( 0 = noninterlaced, 1 = 2:1 interlace
            U8 field_num;    // field number
            U8 video_signal; // video signal standard (table 4)
            U8 unused;       // used for byte alignment only
            R32 hor_sample_rate;   // horizontal sampling rate in Hz
            R32 ver_sample_rate;   // vertical sampling rate in Hz
            R32 frame_rate;        // temporal sampling rate or frame rate in Hz
            R32 time_offset;       // time offset from sync to first pixel
            R32 gamma;             // gamma value
            R32 black_level;       // black level code value
            R32 black_gain;        // black gain
            R32 break_point;       // breakpoint
            R32 white_level;       // reference white level code value
            R32 integration_times; // integration time(s)
            U8 reserved[76];       // reserved for future use (padding)

            void swap()
            {
                TwkUtil::swapWords(&tim_code, 2);
                TwkUtil::swapWords(&hor_sample_rate, 10);
            }
        };

        struct DPXHeader
        {
            DPXFileInformation file;
            DPXImageInformation image;
            DPXImageOrientation orientation;
            DPXFilmHeader film;
            DPXTelevisionHeader tv;

            void swap()
            {
                file.swap();
                image.swap();
                orientation.swap();
                film.swap();
                tv.swap();
            }
        };

        struct UserDefinedData
        {
            ASCII UserId[32]; // User-defined identification string
            U8* Data;         // User-defined data
        };

        struct Pixel
        {
            union
            {
                struct
                {
#if defined(TWK_LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN__)
                    UInt unused : 2;
                    UInt blue : 10;
                    UInt green : 10;
                    UInt red : 10;
#else
                    UInt red : 10;
                    UInt green : 10;
                    UInt blue : 10;
                    UInt unused : 2;
#endif
                };

                struct
                {
#if defined(TWK_LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN__)
                    UInt unused2 : 2;
                    UInt blue_least : 2;
                    UInt blue_most : 8;
                    UInt green_least : 2;
                    UInt green_most : 8;
                    UInt red_least : 2;
                    UInt red_most : 8;
#else
                    UInt red_most : 8;
                    UInt red_least : 2;
                    UInt green_most : 8;
                    UInt green_leaast : 2;
                    UInt blue_most : 8;
                    UInt blue_least : 2;
                    UInt unused2 : 2;
#endif
                };

                U32 pixelWord;
            };

            void setR(unsigned int R) { red = (R & 1023U); }

            void setG(unsigned int G) { green = (G & 1023U); }

            void setB(unsigned int B) { blue = (B & 1023U); }
        };

        typedef Pixel OutputPixel;

        //
        //  Ctors
        //

        IOdpx(StorageFormat format = RGB16, bool useChromaticies = false,
              IOType type = StandardIO, size_t chunkSize = 61440,
              int maxAsync = 16);

        IOdpx(const std::string& format, bool useChromaticies = false,
              IOType type = StandardIO, size_t chunkSize = 61440,
              int maxAsync = 16);

        virtual ~IOdpx();

        void init();

        void useChromaticities(bool b) { m_useChromaticities = b; }

        void format(StorageFormat f) { m_format = f; }

        //
        //  FrameBufferIO API
        //

        virtual void readImages(FrameBufferVector&, const std::string& filename,
                                const ReadRequest& request) const;
        virtual void writeImage(const FrameBuffer&, const std::string& filename,
                                const WriteRequest& request) const;
        virtual std::string about() const;
        virtual void getImageInfo(const std::string& filename, FBInfo&) const;

        void readAttrs(FrameBuffer& fb, DPXHeader& header, int element) const;

        virtual bool getBoolAttribute(const std::string& name) const;
        virtual void setBoolAttribute(const std::string& name, bool value);
        virtual std::string getStringAttribute(const std::string& name) const;
        virtual void setStringAttribute(const std::string& name,
                                        const std::string& value);

    private:
        bool m_useChromaticities;
        StorageFormat m_format;
    };

} // namespace TwkFB

#endif // __IOdpx__IOdpx__h__
