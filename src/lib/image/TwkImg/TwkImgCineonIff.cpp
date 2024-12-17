//******************************************************************************
// Copyright (c) 2001 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkImg/TwkImgCineonIff.h>
#include <TwkImg/TwkImgIffExc.h>
#include <TwkUtil/ByteSwap.h>
#include <TwkUtil/File.h>
#include <TwkMath/Color.h>

#include <iostream>
#include <math.h>
#include <fstream>
#include <sys/types.h>
#include <time.h>
#include <string.h>

using TwkMath::Col3f;
using TwkMath::Col4f;

//******************************************************************************
// HANDLE BYTE STUFF

//******************************************************************************
// Binary input flags

#ifndef TWK_NO_STD_IOS_BINARY_FLAG

#define BINARY_INPUT_FLAGS (std::ios_base::in | std::ios_base::binary)
#define BINARY_OUTPUT_FLAGS (std::ios_base::out | std::ios_base::binary)

#else

#define BINARY_INPUT_FLAGS (std::ios_base::in)
#define BINARY_OUTPUT_FLAGS (std::ios_base::out)

#endif

//******************************************************************************
namespace TwkImg
{

    //******************************************************************************
    namespace
    {

        typedef unsigned int U32;
        typedef unsigned short U16;
        typedef unsigned char U8;
        typedef int S32;
        typedef float R32;
        typedef char ASCII;

#define CHAN_MASK 1023U

        struct PIXEL
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

                U32 pixelWord;
            };

            void setR(unsigned int R) { red = (R & CHAN_MASK); }

            void setG(unsigned int G) { green = (G & CHAN_MASK); }

            void setB(unsigned int B) { blue = (B & CHAN_MASK); }
        };

        //*****************************************************************************
        // Generic Image Data
        //
        // The generic image data header is just that - it contains all the
        // general information about the image (how big the headers are, what's
        // in the image, how big the image is, etc.). The information contained
        // is generally common to all image manipulation/graphics programs.
        struct FileInformation
        {
            FileInformation();

            // Stream IO
            void read(std::istream& stream);
            void write(std::ostream& stream) const;

            // Diagnostic reporting
            void writeAscii(std::ostream& stream) const;

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
        };

        //*****************************************************************************
        // ImageElement struct
        struct ImageElement
        {
            ImageElement();

            // Stream IO
            void read(std::istream& stream);
            void write(std::ostream& stream) const;

            // Diagnostic reporting
            void writeAscii(std::ostream& stream) const;

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
        };

        //*****************************************************************************
        // Image Information Header
        //
        // The image header contains specific image information about the image
        // (number of channels, resolution, etc.
        struct ImageInformation
        {
            ImageInformation();

            // Stream IO
            void read(std::istream& stream);
            void write(std::ostream& stream) const;

            // Diagnostic reporting
            void writeAscii(std::ostream& stream) const;

            // Image orientation
            U8 orientation;

            // Number of image elements
            U8 num_channels;

            // 2 byte space for word alignment
            U8 UNUSED1[2];

            // Note there are eight image elements
            ImageElement image_element[8];
            R32 white_X;
            R32 white_Y;
            R32 red_X;
            R32 red_Y;
            R32 green_X;
            R32 green_Y;
            R32 blue_X;
            R32 blue_Y;
            ASCII label[200];

            // Reserved for future use (padding)
            ASCII reserved[28];
        };

        //*****************************************************************************
        struct DataFormatInformation
        {
            DataFormatInformation();

            // Stream IO
            void read(std::istream& stream);
            void write(std::ostream& stream) const;

            // Diagnostic reporting
            void writeAscii(std::ostream& stream) const;

            U8 interleave;
            U8 tightness : 1;
            U8 packing : 7;
            U8 data_sign;
            U8 image_sense;
            U32 eol_padding;
            U32 eoc_padding;
            ASCII reserved[20];
        };

        //*****************************************************************************
        // ImageOriginInformation
        struct ImageOriginInformation
        {
            ImageOriginInformation();

            // Read/Write streams
            void read(std::istream& stream);
            void write(std::ostream& stream) const;

            // Diagnostic output
            void writeAscii(std::ostream& stream) const;

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
        };

        //*****************************************************************************
        // FilmInformation
        struct FilmInformation
        {
            FilmInformation();

            // Read/Write streams
            void read(std::istream& stream);
            void write(std::ostream& stream) const;

            // Diagnostic output
            void writeAscii(std::ostream& stream) const;

            // Film manufacturer ID code (2 digits from film edge code)
            U8 film_mfg_id;
            // File type (2 digits from film edge code )
            U8 film_type;
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
        };

        //*****************************************************************************
        FileInformation::FileInformation()
        {
            memset((void*)this, 0, sizeof(FileInformation));
        }

        //*****************************************************************************
        void FileInformation::read(std::istream& stream)
        {
#if defined(TWK_LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN__)
            magic_num = TwkUtil::readSwappedUint(stream);
            offset = TwkUtil::readSwappedUint(stream);
            gen_hdr_size = TwkUtil::readSwappedUint(stream);
            ind_hdr_size = TwkUtil::readSwappedUint(stream);
            user_data_size = TwkUtil::readSwappedUint(stream);
            file_size = TwkUtil::readSwappedUint(stream);
            stream.read((char*)vers, 8);
            stream.read((char*)file_name, 100);
            stream.read((char*)create_date, 12);
            stream.read((char*)create_time, 12);
            stream.read((char*)Reserved, 36);
#else
            stream.read((char*)this, sizeof(FileInformation));
#endif

            if (!stream)
            {
                IffExc exc("Could not read FileInformation data from stream");
                throw(exc);
            }
            else if (magic_num != 0x802A5FD7)
            {
                std::cerr << "Bad magic number. was supposed to be: "
                          << 0x802A5FD7 << std::endl
                          << "Instead got: " << magic_num << std::endl;
                IffExc exc("Wrong magic number for Film Industry Cineon file.");
                throw(exc);
            }
        }

        //*****************************************************************************
        void FileInformation::write(std::ostream& stream) const
        {
#if defined(TWK_LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN__)
            TwkUtil::writeSwappedUint(stream, magic_num);
            TwkUtil::writeSwappedUint(stream, offset);
            TwkUtil::writeSwappedUint(stream, gen_hdr_size);
            TwkUtil::writeSwappedUint(stream, ind_hdr_size);
            TwkUtil::writeSwappedUint(stream, user_data_size);
            TwkUtil::writeSwappedUint(stream, file_size);
            stream.write((char*)vers, 8);
            stream.write((char*)file_name, 100);
            stream.write((char*)create_date, 12);
            stream.write((char*)create_time, 12);
            stream.write((char*)Reserved, 36);
#else
            stream.write((char*)this, sizeof(FileInformation));
#endif

            if (!stream)
            {
                IffExc exc("Could not write FileInformation data to file");
                throw(exc);
            }
        }

        //*****************************************************************************
        void FileInformation::writeAscii(std::ostream& stream) const
        {
            stream << "------- File Header Information ("
                   << sizeof(FileInformation) << " bytes) -------" << std::endl
                   << "Magic Number: " << magic_num << std::endl
                   << "Offset to image data: " << offset << std::endl
                   << "File size: " << file_size << std::endl
                   << "Generic Header Size: " << gen_hdr_size << std::endl
                   << "Industry Header Size: " << ind_hdr_size << std::endl
                   << "User Data Size: " << user_data_size << std::endl
                   << "Version: " << vers << std::endl
                   << "File Name: " << file_name << std::endl
                   << "Create Date: " << create_date << std::endl
                   << "Create Time: " << create_time << std::endl
                   << "Reserved: " << Reserved << std::endl;
        }

        //*****************************************************************************
        ImageElement::ImageElement()
        {
            memset((void*)this, 0, sizeof(ImageElement));
        }

        //*****************************************************************************
        void ImageElement::read(std::istream& stream)
        {
#if defined(TWK_LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN__)
            stream.read((char*)(&designator1), 1);
            stream.read((char*)(&designator2), 1);
            stream.read((char*)(&bits_per_pixel), 1);
            stream.read((char*)(&UNUSED), 1);
            pixels_per_line = TwkUtil::readSwappedUint(stream);
            lines_per_image = TwkUtil::readSwappedUint(stream);
            data_min = TwkUtil::readSwappedFloat(stream);
            quant_min = TwkUtil::readSwappedFloat(stream);
            data_max = TwkUtil::readSwappedFloat(stream);
            quant_max = TwkUtil::readSwappedFloat(stream);
#else
            stream.read((char*)this, sizeof(ImageElement));
#endif
        }

        //*****************************************************************************
        void ImageElement::write(std::ostream& stream) const
        {
#if defined(TWK_LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN__)
            stream.write((const char*)(&designator1), 1);
            stream.write((const char*)(&designator2), 1);
            stream.write((const char*)(&bits_per_pixel), 1);
            stream.write((const char*)(&UNUSED), 1);
            TwkUtil::writeSwappedUint(stream, pixels_per_line);
            TwkUtil::writeSwappedUint(stream, lines_per_image);
            TwkUtil::writeSwappedFloat(stream, data_min);
            TwkUtil::writeSwappedFloat(stream, quant_min);
            TwkUtil::writeSwappedFloat(stream, data_max);
            TwkUtil::writeSwappedFloat(stream, quant_max);
#else
            stream.write((char*)this, sizeof(ImageElement));
#endif
        }

        //*****************************************************************************
        void ImageElement::writeAscii(std::ostream& stream) const
        {
            stream << "\tDesignator 1: "
                   << ((designator1 == 0) ? "Universal Metric"
                                          : "Vendor Specific")
                   << std::endl
                   << "\tDesignator 2: ";

            switch (designator2)
            {
            case 0:
                stream << "B&W" << std::endl;
                break;
            case 1:
                stream << "red (printing density)" << std::endl;
                break;
            case 2:
                stream << "green (printing density)" << std::endl;
                break;
            case 3:
                stream << "blue (printing density)" << std::endl;
                break;
            case 4:
                stream << "red (CCIR XA/11)" << std::endl;
                break;
            case 5:
                stream << "green (CCIR XA/11)" << std::endl;
                break;
            case 6:
                stream << "blue (CCIR XA/11)" << std::endl;
                break;
            default:
                stream << "Reserved" << std::endl;
                break;
            };

            stream << "\tBits Per Pixel: " << (int)bits_per_pixel << std::endl
                   << "\tPixels Per Line: " << pixels_per_line << std::endl
                   << "\tLines Per Image: " << lines_per_image << std::endl
                   << "\tData Min: " << data_min << std::endl
                   << "\tQuant Min: " << quant_min << std::endl
                   << "\tData Max: " << data_max << std::endl
                   << "\tQuant Max: " << quant_max << std::endl;
        }

        //*****************************************************************************
        ImageInformation::ImageInformation()
        {
            memset((void*)this, 0, sizeof(ImageInformation));
        }

        //*****************************************************************************
        void ImageInformation::read(std::istream& stream)
        {
#if defined(TWK_LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN__)
            stream.read((char*)(&orientation), 1);
            stream.read((char*)(&num_channels), 1);
            stream.read((char*)UNUSED1, 2);
            for (int i = 0; i < 8; ++i)
            {
                image_element[i].read(stream);
            }
            white_X = TwkUtil::readSwappedFloat(stream);
            white_Y = TwkUtil::readSwappedFloat(stream);
            red_X = TwkUtil::readSwappedFloat(stream);
            red_Y = TwkUtil::readSwappedFloat(stream);
            green_X = TwkUtil::readSwappedFloat(stream);
            green_Y = TwkUtil::readSwappedFloat(stream);
            blue_X = TwkUtil::readSwappedFloat(stream);
            blue_Y = TwkUtil::readSwappedFloat(stream);
            stream.read((char*)label, 200);
            stream.read((char*)reserved, 28);
#else
            stream.read((char*)this, sizeof(ImageInformation));
#endif
        }

        //*****************************************************************************
        void ImageInformation::write(std::ostream& stream) const
        {
#if defined(TWK_LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN__)
            stream.write((const char*)(&orientation), 1);
            stream.write((const char*)(&num_channels), 1);
            stream.write((const char*)UNUSED1, 2);
            for (int i = 0; i < 8; ++i)
            {
                image_element[i].write(stream);
            }
            TwkUtil::writeSwappedFloat(stream, white_X);
            TwkUtil::writeSwappedFloat(stream, white_Y);
            TwkUtil::writeSwappedFloat(stream, red_X);
            TwkUtil::writeSwappedFloat(stream, red_Y);
            TwkUtil::writeSwappedFloat(stream, green_X);
            TwkUtil::writeSwappedFloat(stream, green_Y);
            TwkUtil::writeSwappedFloat(stream, blue_X);
            TwkUtil::writeSwappedFloat(stream, blue_Y);
            stream.write((const char*)label, 200);
            stream.write((const char*)reserved, 28);
#else
            stream.write((const char*)this, sizeof(ImageInformation));
#endif
        }

        //*****************************************************************************
        void ImageInformation::writeAscii(std::ostream& stream) const
        {
            stream << "------- Image Header Information ("
                   << sizeof(ImageInformation) << " bytes) -------" << std::endl
                   << "Image Orientation: ";

            switch (orientation)
            {
            case 0:
                stream << "left to right, top to bottom" << std::endl;
                break;
            case 1:
                stream << "left to right, bottom to top" << std::endl;
                break;
            case 2:
                stream << "right to left, top to bottom" << std::endl;
                break;
            case 3:
                stream << "right to left, bottom to top" << std::endl;
                break;
            case 4:
                stream << "top to bottom, left to right" << std::endl;
                break;
            case 5:
                stream << "top to bottom, right to left" << std::endl;
                break;
            case 6:
                stream << "bottom to top, left to right" << std::endl;
                break;
            case 7:
                stream << "bottom to top, right to left" << std::endl;
                break;
            }

            stream << "Number of Channels: " << (int)num_channels << "-----"
                   << std::endl;

            for (int i = 0; i < 7; ++i)
            {
                stream << "Channel " << i << ": " << std::endl;
                image_element[i].writeAscii(stream);
            }

            stream << "-----" << std::endl
                   << "Whitepoint X: " << white_X << std::endl
                   << "Whitepoint Y: " << white_Y << std::endl
                   << "Red primary X: " << red_X << std::endl
                   << "Red primary Y: " << red_Y << std::endl
                   << "Green primary X: " << green_X << std::endl
                   << "Green primary Y: " << green_Y << std::endl
                   << "Blue primary X: " << blue_X << std::endl
                   << "Blue primary Y: " << blue_Y << std::endl
                   << "Label: " << label << std::endl
                   << "Reserved: " << reserved << std::endl;
        }

        //*****************************************************************************
        DataFormatInformation::DataFormatInformation()
        {
            memset((void*)this, 0, sizeof(DataFormatInformation));
        }

        //*****************************************************************************
        void DataFormatInformation::read(std::istream& stream)
        {
#if defined(TWK_LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN__)
            // Notice that I'm reading in interleave, tightness, packing &
            // data_sign with the "interleave" read by reading in 4 bytes.
            stream.read((char*)(&interleave), 4);
            eol_padding = TwkUtil::readSwappedUint(stream);
            eoc_padding = TwkUtil::readSwappedUint(stream);
            stream.read((char*)reserved, 20);
#else
            stream.read((char*)this, sizeof(DataFormatInformation));
#endif
        }

        //*****************************************************************************
        void DataFormatInformation::write(std::ostream& stream) const
        {
#if defined(TWK_LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN__)
            // Writing interleave, tightness, packing, data_sign & image_sense
            // all at once by writing 4 bytes from the interleave address.
            stream.write((const char*)(&interleave), 4);
            TwkUtil::writeSwappedUint(stream, eol_padding);
            TwkUtil::writeSwappedUint(stream, eoc_padding);
            stream.write((const char*)reserved, 20);
#else
            stream.write((const char*)this, sizeof(DataFormatInformation));
#endif
        }

        //*****************************************************************************
        void DataFormatInformation::writeAscii(std::ostream& stream) const
        {
            stream << "------- Data Format Header Information ("
                   << sizeof(DataFormatInformation) << " bytes) -------"
                   << std::endl;

            stream << "Data Interleave: ";
            switch (interleave)
            {
            case 0:
                stream << "pixel" << std::endl;
                break;
            case 1:
                stream << "line" << std::endl;
                break;
            case 2:
                stream << "channel" << std::endl;
                break;
            case 3:
                stream << "user-defined" << std::endl;
                break;
            }

            stream << "Packing: ";
            switch (packing)
            {
            case 0:
                stream << "Use all bits" << std::endl;
                break;
            case 1:
                stream << "8-bit boundaries, left justified" << std::endl;
                break;
            case 2:
                stream << "8-bit boundaries, right justified" << std::endl;
                break;
            case 3:
                stream << "16-bit boundaries, left justified" << std::endl;
                break;
            case 4:
                stream << "16-bit boundaries, right justified" << std::endl;
                break;
            case 5:
                stream << "32-bit boundaries, left justified" << std::endl;
                break;
            case 6:
                stream << "32-bit boundaries, right justified" << std::endl;
                break;
            default:
                stream << "Unknown packing" << std::endl;
                break;
            }

            stream << "Packing Tightness: "
                   << ((tightness == 0) ? "At most one pixel per cell"
                                        : "As many fields as possible per cell")
                   << std::endl;

            stream << "Data Sign: "
                   << ((data_sign == 0) ? "unsigned" : "signed") << std::endl;

            stream << "Image Sense: "
                   << ((image_sense == 0) ? "positive" : "negative")
                   << std::endl;

            stream << "End of Line Padding: " << eol_padding << std::endl
                   << "End of Channel Padding: " << eoc_padding << std::endl
                   << "Reserved: " << reserved << std::endl;
        }

        //*****************************************************************************
        ImageOriginInformation::ImageOriginInformation()
        {
            memset((void*)this, 0, sizeof(ImageOriginInformation));
        }

        //*****************************************************************************
        void ImageOriginInformation::read(std::istream& stream)
        {
#if defined(TWK_LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN__)
            x_offset = TwkUtil::readSwappedInt(stream);
            y_offset = TwkUtil::readSwappedInt(stream);
            stream.read((char*)filename, 100);
            stream.read((char*)creation_date, 12);
            stream.read((char*)creation_time, 12);
            stream.read((char*)input_device, 64);
            stream.read((char*)input_device_model, 32);
            stream.read((char*)input_device_serial, 32);
            input_device_X_pitch = TwkUtil::readSwappedFloat(stream);
            input_device_Y_pitch = TwkUtil::readSwappedFloat(stream);
            input_gamma = TwkUtil::readSwappedFloat(stream);
            stream.read((char*)reserved, 40);
#else
            stream.read((char*)this, sizeof(ImageOriginInformation));
#endif
        }

        //*****************************************************************************
        void ImageOriginInformation::write(std::ostream& stream) const
        {
#if defined(TWK_LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN__)
            TwkUtil::writeSwappedInt(stream, x_offset);
            TwkUtil::writeSwappedInt(stream, y_offset);
            stream.write((const char*)filename, 100);
            stream.write((const char*)creation_date, 12);
            stream.write((const char*)creation_time, 12);
            stream.write((const char*)input_device, 64);
            stream.write((const char*)input_device_model, 32);
            stream.write((const char*)input_device_serial, 32);
            TwkUtil::writeSwappedFloat(stream, input_device_X_pitch);
            TwkUtil::writeSwappedFloat(stream, input_device_Y_pitch);
            TwkUtil::writeSwappedFloat(stream, input_gamma);
            stream.write((const char*)reserved, 40);
#else
            stream.write((const char*)this, sizeof(ImageOriginInformation));
#endif
        }

        //*****************************************************************************
        void ImageOriginInformation::writeAscii(std::ostream& stream) const
        {
            stream << "------- Image Origin Header Information ("
                   << sizeof(ImageOriginInformation) << " bytes) -------"
                   << std::endl
                   << "X offset: " << x_offset << std::endl
                   << "Y offset: " << y_offset << std::endl
                   << "Image Filename: " << filename << std::endl
                   << "Creation Date: " << creation_date << std::endl
                   << "Creation Time: " << creation_time << std::endl
                   << "Input Device: " << input_device << std::endl
                   << "Input Device Model: " << input_device_model << std::endl
                   << "Input Device Serial: " << input_device_serial
                   << std::endl
                   << "X Input Device pitch: " << input_device_X_pitch
                   << std::endl
                   << "Y Input Device pitch: " << input_device_Y_pitch
                   << std::endl
                   << "Image gamma of capture device: " << input_gamma
                   << std::endl
                   << "Reserved: " << reserved << std::endl;
        }

        //*****************************************************************************
        FilmInformation::FilmInformation()
        {
            memset((void*)this, 0, sizeof(FilmInformation));
        }

        //*****************************************************************************
        void FilmInformation::read(std::istream& stream)
        {
#if defined(TWK_LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN__)
            stream.read((char*)(&film_mfg_id), 1);
            stream.read((char*)(&film_type), 1);
            stream.read((char*)(&offset), 1);
            stream.read((char*)(&prefix), 1);
            unknown1 = TwkUtil::readSwappedInt(stream);
            unknown2 = TwkUtil::readSwappedInt(stream);
            stream.read((char*)unknown3, 32);
            unknown4 = TwkUtil::readSwappedInt(stream);
            unknown5 = TwkUtil::readSwappedFloat(stream);
            stream.read((char*)unknown6, 32);
            stream.read((char*)unknown7, 200);
            stream.read((char*)reserved, 740);
#else
            stream.read((char*)this, sizeof(FilmInformation));
#endif
        }

        //*****************************************************************************
        void FilmInformation::write(std::ostream& stream) const
        {
#if defined(TWK_LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN__)
            stream.write((const char*)(&film_mfg_id), 1);
            stream.write((const char*)(&film_type), 1);
            stream.write((const char*)(&offset), 1);
            stream.write((const char*)(&prefix), 1);
            TwkUtil::writeSwappedInt(stream, unknown1);
            TwkUtil::writeSwappedInt(stream, unknown2);
            stream.write((const char*)unknown3, 32);
            TwkUtil::writeSwappedInt(stream, unknown4);
            TwkUtil::writeSwappedFloat(stream, unknown5);
            stream.write((const char*)unknown6, 32);
            stream.write((const char*)unknown7, 200);
            stream.write((const char*)reserved, 740);
#else
            stream.write((const char*)this, sizeof(FilmInformation));
#endif
        }

        //*****************************************************************************
        void FilmInformation::writeAscii(std::ostream& stream) const
        {
            stream << "------- Film-Specific Header Information ("
                   << sizeof(FilmInformation) << " bytes ) -------" << std::endl
                   << "Film Manufacturer ID: " << (int)film_mfg_id << std::endl
                   << "Film Type ID: " << (int)film_type << std::endl
                   << "Offset: " << (int)offset << std::endl
                   << "Prefix: " << (int)prefix << std::endl
                   << "Unknown1: " << unknown1 << std::endl
                   << "Unknown2: " << unknown2 << std::endl
                   << "Format: " << unknown3 << std::endl
                   << "Frame Pos in Sequence: " << unknown4 << std::endl
                   << "Frame Rate: " << unknown5 << std::endl
                   << "Frame Attribute: " << unknown6 << std::endl
                   << "Slate Info: " << unknown7 << std::endl
                   << "Reserved: " << reserved << std::endl;
        }

        //*****************************************************************************
        // Printing Density to Relative Exposure
        void printingDensityToRelativeExposure(const PIXEL& pd, Col3f& re,
                                               int codeShiftR, int codeShiftG,
                                               int codeShiftB)
        {
            // Step 0. Code shift.
            int red = (codeShiftR + (int)pd.red) & CHAN_MASK;
            int green = (codeShiftG + (int)pd.green) & CHAN_MASK;
            int blue = (codeShiftB + (int)pd.blue) & CHAN_MASK;

            // Step 1. Convert the printing density code values
            // to a floating point representation. This is accomplished
            // by multiplying by 0.002
            re.x = 0.002f * (float)(red);
            re.y = 0.002f * (float)(green);
            re.z = 0.002f * (float)(blue);

            // Step 2. Divide the printing density data by the gamma of
            // the negative film (0.6) to convert to relative log
            // exposure.
            re /= 0.6f;

            // Step 3. Adjust the reference white log exposure to
            // 0.0 by subtracting 2.28333. This number is computed by
            // multiplying the reference white code value by 0.002 and
            // dividing by the gamma of the film (0.6)
            // Ref white offset = 685 * 0.002 / 0.6 = 2.28333
            re -= 2.28333f;

            // Take the antilog (base 10) of the relative log exposure
            // data to convert to linear relative exposure.
            re.x = powf(10.0f, re.x);
            re.y = powf(10.0f, re.y);
            re.z = powf(10.0f, re.z);
        }

        //*****************************************************************************
        void relativeExposureToPrintingDensity(const Col3f& re, PIXEL& pd,
                                               int codeShiftR, int codeShiftG,
                                               int codeShiftB)
        {
            Col3f tmp(re);

            // Clamp out invalid numbers
            static const float minFloatVal = powf(10.0f, -2.28333f) + 0.00001f;

            // which will mess up the log thing below.
            if (tmp.x < minFloatVal)
            {
                tmp.x = minFloatVal;
            }
            if (tmp.y < minFloatVal)
            {
                tmp.y = minFloatVal;
            }
            if (tmp.z < minFloatVal)
            {
                tmp.z = minFloatVal;
            }

            // Step 1. Take the log (base 10) of the linear relative exposure
            // to convert to relative log exposure
            static const float LN_10 = logf(10.0f);
            tmp.x = logf(tmp.x) / LN_10;
            tmp.y = logf(tmp.y) / LN_10;
            tmp.z = logf(tmp.z) / LN_10;

            // Step 2. Adjust the reference white log exposure from 0.0
            // by adding 2.28333. See above function for the explanation
            // of the computation of this number.
            tmp += 2.28333f;

            // Step 3. Multiply the relative log exposure by the gamma
            // of the negative film to convert to printing density
            tmp *= 0.6f;

            // Step 4. Convert from floating point representation to
            // an integer representation.
            tmp /= 0.002f;

            // Step 5. Put into the PIXEL struct.
            const unsigned int tr = static_cast<unsigned int>(
                std::clamp(tmp.x + (float)codeShiftR, 0.0f, 1023.0f));
            const unsigned int tg = static_cast<unsigned int>(
                std::clamp(tmp.y + (float)codeShiftG, 0.0f, 1023.0f));
            const unsigned int tb = static_cast<unsigned int>(
                std::clamp(tmp.z + (float)codeShiftB, 0.0f, 1023.0f));
            pd.setR(tr);
            pd.setG(tg);
            pd.setB(tb);
            pd.unused = 0;
        }

    } // End anonymous namespace

    //*****************************************************************************
    Img4f* CineonIff::read(std::istream& cinStream, int codeShiftR,
                           int codeShiftG, int codeShiftB, bool linearize)
    {
        FileInformation genericHeader;
        ImageInformation infoHeader;
        DataFormatInformation dataFormatHeader;
        ImageOriginInformation originHeader;
        FilmInformation filmHeader;

        // Read the headers
        genericHeader.read(cinStream);
        infoHeader.read(cinStream);
        dataFormatHeader.read(cinStream);
        originHeader.read(cinStream);
        filmHeader.read(cinStream);

#if 0
    genericHeader.writeAscii( STD_COUT );
    infoHeader.writeAscii( STD_COUT );
    dataFormatHeader.writeAscii( STD_COUT );
    originHeader.writeAscii( STD_COUT );
    filmHeader.writeAscii( STD_COUT );
#endif

        // Seek to the beginning of the pixel information.
        cinStream.seekg(genericHeader.offset, std::ios_base::beg);

        // Get the width and height of the image
        const int width = infoHeader.image_element[0].pixels_per_line;
        const int height = infoHeader.image_element[0].lines_per_image;

        // Create the output space
        Img4f* ret = new Img4f(width, height);

        // Loop over rows, backwards because cineon files
        // are generally stored top to bottom.
        union
        {
            char srcPixelBytes[4];
            PIXEL srcPixel;
        };

        for (int y = height - 1; y >= 0; --y)
        {
            Col4f* dstPixel = (*ret)[y];
            for (int x = 0; x < width; ++x, ++dstPixel)
            {
                cinStream.read((char*)srcPixelBytes, sizeof(PIXEL));

#if defined(TWK_LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN__)
                // Must swap bytes.
                std::swap(srcPixelBytes[0], srcPixelBytes[3]);
                std::swap(srcPixelBytes[1], srcPixelBytes[2]);
#endif

                if (linearize)
                {
                    printingDensityToRelativeExposure(
                        srcPixel, (*((Col3f*)dstPixel)), codeShiftR, codeShiftG,
                        codeShiftB);
                }
                else
                {
                    (*dstPixel).x =
                        STD_MAX(((int)srcPixel.red) + codeShiftR, 0);
                    (*dstPixel).y =
                        STD_MAX(((int)srcPixel.green) + codeShiftG, 0);
                    (*dstPixel).z =
                        STD_MAX(((int)srcPixel.blue) + codeShiftB, 0);
                    (*dstPixel) /= 1023.0f;
                }
                (*dstPixel).w = 1.0f;
            }
        }

        return ret;
    }

    //*****************************************************************************
    Img4f* CineonIff::read(const char* imgFileName, int codeShiftR,
                           int codeShiftG, int codeShiftB, bool linearize)
    {
        std::ifstream imgStream(UNICODE_C_STR(imgFileName), BINARY_INPUT_FLAGS);

        if (!imgStream)
        {
            IffExc exc("Could not open specified image file");
            throw(exc);
        }

        Img4f* ret = CineonIff::read(imgStream, codeShiftR, codeShiftG,
                                     codeShiftB, linearize);

        imgStream.close();

        return ret;
    }

// The files from the Cineon system had a "user data" section
// which Shake files do not.
#define NEED_USER_DATA 0

    //*****************************************************************************
    void CineonIff::write(const Img4f* img, std::ostream& cinStream,
                          const char* fileName, int codeShiftR, int codeShiftG,
                          int codeShiftB, bool deLinearize)
    {
        assert(img != NULL);
        if (fileName == NULL)
        {
            fileName = "unnamed";
        }

        // Get width & height.
        const int width = img->width();
        const int height = img->height();

        FileInformation genericHeader;
        ImageInformation infoHeader;
        DataFormatInformation dataFormatHeader;
        ImageOriginInformation originHeader;
        FilmInformation filmHeader;

        time_t tme = time(NULL);
        struct tm* tmp = localtime(&tme);
        char dateStr[256];
        strftime(dateStr, 256, "%Y:%m:%d:%H:%M:%S%Z", tmp);

        // Write values into headers
        // GENERIC HEADER
        genericHeader.magic_num = 0x802A5FD7;
#if NEED_USER_DATA
        genericHeader.offset = 32256;
        genericHeader.file_size = (width * height * sizeof(PIXEL)) + 32256;
#else
        genericHeader.offset = 2048;
        genericHeader.file_size = (width * height * sizeof(PIXEL)) + 2048;
#endif
        genericHeader.gen_hdr_size = 1024;
        genericHeader.ind_hdr_size = 1024;

#if NEED_USER_DATA
        genericHeader.user_data_size = 30208;
#else
        genericHeader.user_data_size = 0;
#endif
        strcpy(genericHeader.vers, "V4.5");
        strncpy(genericHeader.file_name, fileName, 99);
        strncpy(genericHeader.create_date, dateStr, 10);
        strncpy(genericHeader.create_time, dateStr + 11, 8);

        // Image head
        infoHeader.orientation = 0; // left to right, top to bottom
        infoHeader.num_channels = 3;

        // Red
        infoHeader.image_element[0].designator1 = 0; // Universal Metric
        infoHeader.image_element[0].designator2 = 1; // Red printing density
        infoHeader.image_element[0].bits_per_pixel = 10;
        infoHeader.image_element[0].pixels_per_line = width;
        infoHeader.image_element[0].lines_per_image = height;
        infoHeader.image_element[0].data_min = 0.0f;
        infoHeader.image_element[0].quant_min = 0.0f;
        infoHeader.image_element[0].data_max = 1023.0f;
        infoHeader.image_element[0].quant_max = 2.046f;

        // Green
        infoHeader.image_element[1].designator1 = 0; // Universal Metric
        infoHeader.image_element[1].designator2 = 2; // Green printing density
        infoHeader.image_element[1].bits_per_pixel = 10;
        infoHeader.image_element[1].pixels_per_line = width;
        infoHeader.image_element[1].lines_per_image = height;
        infoHeader.image_element[1].data_min = 0.0f;
        infoHeader.image_element[1].quant_min = 0.0f;
        infoHeader.image_element[1].data_max = 1023.0f;
        infoHeader.image_element[1].quant_max = 2.046f;

        // Blue
        infoHeader.image_element[2].designator1 = 0; // Universal Metric
        infoHeader.image_element[2].designator2 = 3; // Blue printing density
        infoHeader.image_element[2].bits_per_pixel = 10;
        infoHeader.image_element[2].pixels_per_line = width;
        infoHeader.image_element[2].lines_per_image = height;
        infoHeader.image_element[2].data_min = 0.0f;
        infoHeader.image_element[2].quant_min = 0.0f;
        infoHeader.image_element[2].data_max = 1023.0f;
        infoHeader.image_element[2].quant_max = 2.046f;

        // Data header
        dataFormatHeader.interleave = 0;  // pixel
        dataFormatHeader.packing = 2;     // 8 bit boundaries, right justified
        dataFormatHeader.tightness = 1;   // As many fields as possible per cell
        dataFormatHeader.data_sign = 0;   // unsigned
        dataFormatHeader.image_sense = 0; // positive
        dataFormatHeader.eol_padding = 0;
        dataFormatHeader.eoc_padding = 0;

        // Image Origin header
        originHeader.x_offset = 0;
        originHeader.y_offset = 0;
        strncpy(originHeader.filename, fileName, 99);
        strncpy(originHeader.creation_date, dateStr, 10);
        strncpy(originHeader.creation_time, dateStr + 11, 8);
        strcpy(originHeader.input_device, "Tweak Software");
        strcpy(originHeader.input_device_model, "FB");
        originHeader.input_gamma = 1.0;
        // originHeader.input_device_X_pitch = -1.0;
        // originHeader.input_device_Y_pitch = -1.0;

        // One at at time, write the headers to the stream.
        genericHeader.write(cinStream);
        infoHeader.write(cinStream);
        dataFormatHeader.write(cinStream);
        originHeader.write(cinStream);
        filmHeader.write(cinStream);

        // Write a big block of zeros to the file to get to
        // the beginning of the pixel information.
#if NEED_USER_DATA
        char userBuf[30208];
        memset((void*)userBuf, 0, 30208);
        cinStream.write((const char*)userBuf, 30208);
#endif

        // Loop over rows, backwards because cineon files
        // are generally stored top to bottom.
        union
        {
            char dstPixelBytes[4];
            PIXEL dstPixel;
        };

        for (int y = height - 1; y >= 0; --y)
        {
            const Col4f* srcPixel = (*img)[y];
            for (int x = 0; x < width; ++x, ++srcPixel)
            {
                if (deLinearize)
                {
                    relativeExposureToPrintingDensity(
                        (*((Col3f*)srcPixel)), (PIXEL&)dstPixel, codeShiftR,
                        codeShiftG, codeShiftB);
                }
                else
                {
                    const float tr = std::clamp(
                        (srcPixel->x * 1023.0f) + codeShiftR, 0.0f, 1023.0f);
                    const float tg = std::clamp(
                        (srcPixel->y * 1023.0f) + codeShiftG, 0.0f, 1023.0f);
                    const float tb = std::clamp(
                        (srcPixel->z * 1023.0f) + codeShiftB, 0.0f, 1023.0f);

                    dstPixel.setR((unsigned int)tr);
                    dstPixel.setG((unsigned int)tg);
                    dstPixel.setB((unsigned int)tb);
                    dstPixel.unused = 0;
                }

#if defined(TWK_LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN__)
                // Must swap bytes
                std::swap(dstPixelBytes[0], dstPixelBytes[3]);
                std::swap(dstPixelBytes[1], dstPixelBytes[2]);
#endif

                cinStream.write((const char*)dstPixelBytes, sizeof(PIXEL));
            }
        }
    }

    //*****************************************************************************
    void CineonIff::write(const Img4f* img, const char* imgFileName,
                          int codeShiftR, int codeShiftG, int codeShiftB,
                          bool deLinearize)
    {
        std::ofstream imgStream(UNICODE_C_STR(imgFileName),
                                BINARY_OUTPUT_FLAGS);

        if (!imgStream)
        {
            IffExc exc("Could not open specified image file");
            throw(exc);
        }

        CineonIff::write(img, imgStream, imgFileName, codeShiftR, codeShiftG,
                         codeShiftB, deLinearize);

        imgStream.close();
    }

} // End namespace TwkImg
