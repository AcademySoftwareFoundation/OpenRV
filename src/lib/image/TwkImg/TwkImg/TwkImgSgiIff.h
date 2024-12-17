//******************************************************************************
// Copyright (c) 2001-2002 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#ifndef _TwkImgSgiIff_h_
#define _TwkImgSgiIff_h_

#include <TwkImg/TwkImgImage.h>
#include <TwkImg/TwkImgIffExc.h>

namespace TwkImg
{

    //******************************************************************************
    // Base image reader, reads into pixels of 4uc.
    class SgiIffBase
    {
    protected:
        struct Header
        {
        public:
            Header();
            Header(int width, int height, int numChannels);

            void read(std::istream& in);
            void write(std::ostream& out);

            short magic;
            char storage;
            char bytesPerChannel;
            unsigned short dimension;
            unsigned short width;
            unsigned short height;
            unsigned short numChannels;
            long minChannelVal;
            long maxChannelVal;
            char dummy1[4];
            char imageName[80];
            long colormapID;
            char dummy2[404];
        };

        // Pixel reading routines
        static void readPixelsRLE(Img4uc& into, Header& aHeader,
                                  std::istream& in);
        static void readPixels(Img4uc& into, Header& aHeader, std::istream& in);
        static void writePixels(const Img4uc& outof, Header& aHeader,
                                std::ostream& out);
    };

    //******************************************************************************
    // Reads into an image 4uc. Temporary solution
    class SgiIff4uc : public SgiIffBase
    {
    public:
        static Img4uc* read(const char* fileName);
        static Img4uc* read(std::istream& in);
        static void write(const Img4uc* img, const char* fileName);
        static void write(const Img4uc* img, std::ostream& out);
    };

    //******************************************************************************
    // Reads into an image 4f. Temporary solution
    class SgiIff4f : public SgiIffBase
    {
    public:
        static Img4f* read(const char* fileName);
        static Img4f* read(std::istream& in);
        static void write(const Img4f* img, const char* fileName);
        static void write(const Img4f* img, std::ostream& out);
    };

} // End namespace TwkImg

#endif
