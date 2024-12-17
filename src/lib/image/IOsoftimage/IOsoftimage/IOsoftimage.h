//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IOsoftimage__IOsoftimage__h__
#define __IOsoftimage__IOsoftimage__h__
#include <TwkExc/TwkExcException.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkFB/IO.h>
#include <fstream>

namespace TwkFB
{

    /// Softimage PIC file reader

    ///
    /// Reads an softimage pic file as either 8 bit integral data.
    ///

    class IOsoftimage : public FrameBufferIO
    {
    public:
        //
        //  SOFTIMAGE file header
        //

        typedef int int32;
        typedef unsigned int uint32;
        typedef unsigned short uint16;
        typedef short int16;
        typedef unsigned char uint8;

        enum ChannelCode
        {
            RedChannel = 0x80,
            GreenChannel = 0x40,
            BlueChannel = 0x20,
            AlphaChannel = 0x10,

            ColorChannels = RedChannel | GreenChannel | BlueChannel,
            AllChannels = ColorChannels | AlphaChannel
        };

        enum Fields
        {
            NoField = 0,
            OddField = 1,
            EvenField = 2,
            FullFrame = 3
        };

        struct ChannelPacket
        {
            uint8 chained;
            uint8 size;
            uint8 type;
            uint8 channel;
        };

        enum ChannelType
        {
            Uncompressed = 0,
            MixedRunLength = 2
        };

        typedef std::vector<ChannelPacket> ChannelPacketVector;

        struct Header
        {
            uint32 magic;
            float version;
            char comment[80];
            uint32 id;
            uint16 width;
            uint16 height;
            float pixelAspect;
            uint16 fields;
            uint16 padding;

            void swap();
            void valid();
        };

        static const uint32 Magic = 0x5380f634;
        static const uint32 Cigam = 0x34F68053;

        IOsoftimage();
        virtual ~IOsoftimage();

        virtual void readImage(FrameBuffer& fb, const std::string& filename,
                               const ReadRequest& request) const;

        virtual void writeImage(const FrameBuffer& img,
                                const std::string& filename,
                                const WriteRequest& request) const;

        virtual std::string about() const;
        virtual void getImageInfo(const std::string& filename, FBInfo&) const;

    private:
        void readPixels(std::ifstream&, const Header&, FrameBuffer&, bool,
                        const ChannelPacketVector&) const;

        void readAttrs(TwkFB::FrameBuffer&, const Header&,
                       const ChannelPacketVector&) const;
    };

} // namespace TwkFB

#endif // __IOsoftimage__IOsoftimage__h__
