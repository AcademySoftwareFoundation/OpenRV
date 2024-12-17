//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IOsgi__IOsgi__h__
#define __IOsgi__IOsgi__h__
#include <TwkExc/TwkExcException.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkFB/IO.h>
#include <fstream>

namespace TwkFB
{

    /// Old skool SGI file reader

    ///
    /// Reads an SGI file as either 8 or 16 bit integral data. Ignores
    /// really old variants with colormaps, etc.
    ///
    /// Since the SGI channels are stored planar, this reader will read them
    /// like that. Planes are named according to their number starting with
    /// Red. So the first four planes are named: R G B A, the fifth Z.  Black
    /// and White images (bw) are Y and A if two channels. Single channel
    /// images are Y.
    ///
    /// The reader will accept .bw, .rgb, .rgba, .sgi extensions.
    ///
    /// The SGI format itself is not completely clear in the 1.0 spec for
    /// little endian machines. The file *should* always be big endian. This
    /// reader will assume that the magic number determines the endianess. If
    /// its swaped the whole file is assumed swaped. Which means this reader
    /// could read a little-endian SGI file on a big-endian machine (eventhough
    /// that should not exist).
    ///

    class IOsgi : public FrameBufferIO
    {
    public:
        //
        //  SGI file header
        //

        typedef int int32;
        typedef unsigned short uint16;
        typedef short int16;
        typedef unsigned char uint8;

        struct Header
        {
            int16 magic;
            uint8 storage;
            uint8 bytesPerChannel;
            uint16 dimension;
            uint16 width;
            uint16 height;
            uint16 numChannels;
            int32 minChannelVal;
            int32 maxChannelVal;
            char dummy1[4];
            char imageName[80];
            int32 colormapID;
            char dummy2[404];

            void swap();
            void valid();
        };

        static const short Magic = 0x01DA;
        static const short Cigam = 0xDA01;

        IOsgi();
        virtual ~IOsgi();

        virtual void readImage(FrameBuffer& fb, const std::string& filename,
                               const ReadRequest& request) const;

        virtual void writeImage(const FrameBuffer& img,
                                const std::string& filename,
                                const WriteRequest& request) const;

        virtual std::string about() const;
        virtual void getImageInfo(const std::string& filename, FBInfo&) const;

    private:
        void readVerbatim(std::ifstream&, const Header&, FrameBuffer&,
                          bool) const;
        void readRLE(std::ifstream&, const Header&, FrameBuffer&, bool) const;
    };

} // namespace TwkFB

#endif // __IOsgi__IOsgi__h__
