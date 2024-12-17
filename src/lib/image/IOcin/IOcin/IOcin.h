//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IOcin__IOcin__h__
#define __IOcin__IOcin__h__
#include <TwkExc/TwkExcException.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkFB/StreamingIO.h>
#include <TwkUtil/ByteSwap.h>
#include <fstream>

namespace TwkUtil
{
    class FileStream;
}

namespace TwkFB
{

    //
    //  class IOcin
    //
    //  The Cineon reader can read into to a number of different storage
    //  formats. The fastest to read is RGB10_A2. However, some hardware (the
    //  mac) has trouble with glTexSubImage2D() and this internal format so its
    //  not fast to draw multiple images.
    //
    //  Another solution for drawing is RGB8_PLANAR which will always draw fast
    //  and consumes the least amount of memory. However, this requires some
    //  software conversion.
    //
    //  For image processing RGBA16 is the best format all around.
    //
    //  Chromaticities in Cineon files appear to be bogus (or I'm missing
    //  something). So this reader labels them so that RV will not use them
    //  (unlike EXR).
    //

#define CHAN_MASK 1023U

    class IOcin : public StreamingFrameBufferIO
    {
    public:
        //
        //  Types
        //

        enum CINOrientation
        {
            CIN_TOP_LEFT = 0,
            CIN_BOTTOM_LEFT = 1,
            CIN_TOP_RIGHT = 2,
            CIN_BOTTOM_RIGHT = 3,
            CIN_ROTATED_TOP_LEFT = 4,
            CIN_ROTATED_BOTTOM_LEFT = 5,
            CIN_ROTATED_TOP_RIGHT = 6,
            CIN_ROTATED_BOTTOM_RIGHT = 7
        };

        typedef unsigned int U32;
        typedef unsigned short U16;
        typedef unsigned char U8;
        typedef int S32;
        typedef float R32;
        typedef char ASCII;

        enum StorageFormat
        {
            RGB8,        // Good drawing speed
            RGB16,       // Good fidelity, slow to draw -- default
            RGBA8,       // Good drawing speed
            RGBA16,      // Good fidelity, slowest to draw
            RGB10_A2,    // Fastest to read, fast to draw
            A2_BGR10,    // Slow to read, fastest to draw (nvidia)
            RGB8_PLANAR, // Fastest to draw, second fastest to read
            RGB16_PLANAR // Faster to draw, good fidelity
        };

        struct FileInformation
        {
            U32 magic_num;        // magic number 0x802A5FD7
            U32 offset;           // offset to image data in bytes
            U32 gen_hdr_size;     // generic header length in bytes
            U32 ind_hdr_size;     // industry header length in bytes
            U32 user_data_size;   // user-defined data length in bytes
            U32 file_size;        // file size in bytes
            ASCII vers[8];        // which header format version used (v1.0)
            ASCII file_name[100]; // iamge file name
            ASCII
            create_date[12]; // file creation date "yyyy:mm:dd:hh:mm:ss:LTZ"
            ASCII
            create_time[12];    // file creation date "yyyy:mm:dd:hh:mm:ss:LTZ"
            ASCII Reserved[36]; // reserved field TBD (need to pad)

            void swap() { TwkUtil::swapWords(&magic_num, 6); }
        };

        struct ImageElement
        {
            U8 designator1;
            U8 designator2;
            U8 bits_per_pixel;
            U8 UNUSED;
            U32 pixels_per_line;
            U32 lines_per_image;
            R32 data_min;
            R32 quant_min;
            R32 data_max;
            R32 quant_max;

            void swap() { TwkUtil::swapWords(&pixels_per_line, 6); }
        };

        struct ImageInformation
        {
            U8 orientation;  // Image orientation
            U8 num_channels; // Number of image elements
            U8 UNUSED1[2];   // 2 byte space for word alignment
            ImageElement
                image_element[8]; // Note there are eight image elements
            R32 white_X;
            R32 white_Y;
            R32 red_X;
            R32 red_Y;
            R32 green_X;
            R32 green_Y;
            R32 blue_X;
            R32 blue_Y;
            ASCII label[200];
            ASCII reserved[28]; // Reserved for future use (padding)

            void swap()
            {
                for (int i = 0; i < 8; i++)
                    image_element[i].swap();
                TwkUtil::swapWords(&white_X, 8);
            }
        };

        struct DataFormatInformation
        {
            U8 interleave;
            U8 tightness : 1;
            U8 packing : 7;
            U8 data_sign;
            U8 image_sense;
            U32 eol_padding;
            U32 eoc_padding;
            ASCII reserved[20];

            void swap() { TwkUtil::swapWords(&eol_padding, 2); }
        };

        struct ImageOriginInformation
        {
            S32 x_offset;
            S32 y_offset;
            ASCII filename[100];
            ASCII creation_date[12];
            ASCII creation_time[12];
            ASCII input_device[64];
            ASCII input_device_model[32];
            ASCII input_device_serial[32];
            R32 input_device_X_pitch;
            R32 input_device_Y_pitch;
            R32 input_gamma;
            ASCII reserved[40];

            void swap()
            {
                TwkUtil::swapWords(&x_offset, 2);
                TwkUtil::swapWords(&input_device_X_pitch, 3);
            }
        };

        struct FilmInformation
        {
            U8 film_mfg_id; // Film manufacturer ID code (2 digits from film
                            // edge code)
            U8 film_type;   // File type (2 digits from film edge code )
            U8 offset;
            U8 prefix;
            U32 unknown1;
            U32 unknown2;
            ASCII unknown3[32];
            U32 unknown4;
            R32 unknown5;
            ASCII unknown6[32];
            ASCII unknown7[200];
            ASCII reserved[740];

            void swap()
            {
                TwkUtil::swapWords(&unknown1, 2);
                TwkUtil::swapWords(&unknown4, 2);
            }
        };

        struct Pixel
        {
            union
            {
                struct
                {
#if defined(TWK_LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN__)
                    U32 unused : 2;
                    U32 blue : 10;
                    U32 green : 10;
                    U32 red : 10;
#else
                    U32 red : 10;
                    U32 green : 10;
                    U32 blue : 10;
                    U32 unused : 2;
#endif
                };

                struct
                {
#if defined(TWK_LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN__)
                    U32 unused2 : 2;
                    U32 blue_most : 8;
                    U32 blue_least : 2;
                    U32 green_most : 8;
                    U32 green_least : 2;
                    U32 red_most : 8;
                    U32 red_least : 2;
#else
                    U32 red_most : 8;
                    U32 red_least : 2;
                    U32 green_most : 8;
                    U32 green_leaast : 2;
                    U32 blue_most : 8;
                    U32 blue_least : 2;
                    U32 unused2 : 2;
#endif
                };

                U32 pixelWord;
            };

            void setR(unsigned int R) { red = (R & CHAN_MASK); }

            void setG(unsigned int G) { green = (G & CHAN_MASK); }

            void setB(unsigned int B) { blue = (B & CHAN_MASK); }
        };

        //
        //  Ctors
        //

        IOcin(StorageFormat format = RGB16, bool useChromaticies = false,
              IOType type = StandardIO, size_t iosize = 61440,
              int maxAsync = 16);

        IOcin(const std::string& format, bool useChromaticies = false,
              IOType type = StandardIO, size_t iosize = 61440,
              int maxAsync = 16);

        virtual ~IOcin();

        void init();

        //
        //  FrameBufferIO API
        //

        virtual void readImage(FrameBuffer&, const std::string& filename,
                               const ReadRequest& request) const;

        virtual void writeImage(const FrameBuffer&, const std::string& filename,
                                const WriteRequest& request) const;

        virtual std::string about() const;

        virtual void getImageInfo(const std::string& filename, FBInfo&) const;

        void useChromaticities(bool b) { m_useChromaticities = b; }

        void format(StorageFormat f) { m_format = f; }

        void readAttrs(FrameBuffer& fb, FileInformation& genericHeader,
                       ImageInformation& infoHeader,
                       DataFormatInformation& dataFormatHeader,
                       ImageOriginInformation& originHeader,
                       FilmInformation& filmHeader) const;

        void readImage(TwkUtil::FileStream&, FrameBuffer&,
                       const std::string& filename,
                       const ReadRequest& request) const;

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

#endif // __IOcin__IOcin__h__
