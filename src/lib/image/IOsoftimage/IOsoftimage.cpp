//******************************************************************************
// Copyright (c) 2001-2004 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <IOsoftimage/IOsoftimage.h>
#include <TwkFB/Exception.h>
#include <TwkUtil/ByteSwap.h>
#include <TwkUtil/File.h>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>

namespace TwkFB
{
    using namespace std;
    using namespace TwkUtil;

    void IOsoftimage::Header::swap()
    {
        swapWords(&magic, 2);
        swapWords(&id, 1);
        swapShorts(&width, 2);
        swapWords(&pixelAspect, 1);
        swapShorts(&fields, 2);
    }

    void IOsoftimage::Header::valid()
    {
        if (magic != Magic)
        {
            TWK_THROW_STREAM(IOException, "SGI header has non-standard values");
        }
    }

    IOsoftimage::IOsoftimage()
        : FrameBufferIO()
    {
        unsigned int cap = ImageRead;
        addType("pic", "Softimage PIC file", cap);
    }

    IOsoftimage::~IOsoftimage() {}

    string IOsoftimage::about() const { return "Softimage .pic (Tweak)"; }

    void IOsoftimage::readAttrs(TwkFB::FrameBuffer& fb, const Header& header,
                                const ChannelPacketVector& packets) const
    {
        string fieldString;

        switch (header.fields)
        {
        default:
        case NoField:
            fieldString = "No Field";
            break;
        case OddField:
            fieldString = "Odd";
            break;
        case EvenField:
            fieldString = "Even";
            break;
        case FullFrame:
            fieldString = "Full Frame";
            break;
        }

        fb.newAttribute<string>("SI/Fields", fieldString);
        fb.newAttribute<string>("SI/Comment", header.comment);
        fb.newAttribute<float>("SI/Version", header.version);

        //
        //
        //

        ostringstream str;

        for (size_t i = 0; i < packets.size(); i++)
        {
            const ChannelPacket& packet = packets[i];

            switch (packet.channel)
            {
            case ColorChannels:
                str << "RGB:";
                break;
            case AlphaChannel:
                str << "A:";
                break;
            }

            str << int(packet.size) << ":";

            switch (packet.type)
            {
            case Uncompressed:
                str << "Uncompressed";
                break;
            case MixedRunLength:
                str << "RLE";
                break;
            default:
                str << "Unknown(" << packet.type << ")";
                break;
            }

            str << " ";
        }

        fb.newAttribute<string>("SI/Packets", str.str());

        //
        //  Like SGI, we'll assume Rec709 primaries + linear
        //

        fb.setPrimaryColorSpace(ColorSpace::Rec709());
        fb.setTransferFunction(ColorSpace::Linear());
    }

    void IOsoftimage::getImageInfo(const std::string& filename,
                                   FBInfo& fbi) const
    {
        ifstream infile(UNICODE_C_STR(filename.c_str()), ios::binary | ios::in);

        if (!infile)
        {
            TWK_THROW_STREAM(IOException,
                             "Unable to open softimage .pic file \""
                                 << filename << "\" for reading");
        }

        Header header;
        infile.read((char*)&header, sizeof(Header));

        ChannelPacketVector packets;
        ChannelPacket packet;
        packet.chained = 1;
        uint8 allchannels = 0;

        while (packet.chained)
        {
            infile.read((char*)&packet, sizeof(ChannelPacket));
            packets.push_back(packet);
            allchannels |= packet.channel;
        }

        if (header.magic == Magic || header.magic == Cigam)
        {
            if (header.magic == Cigam)
                header.swap();
            header.valid();

            fbi.width = header.width;
            fbi.height = header.height;
            fbi.numChannels = 0;

            if (allchannels & RedChannel)
                fbi.numChannels++;
            if (allchannels & GreenChannel)
                fbi.numChannels++;
            if (allchannels & BlueChannel)
                fbi.numChannels++;
            if (allchannels & AlphaChannel)
                fbi.numChannels++;

            fbi.dataType = FrameBuffer::UCHAR;

            readAttrs(fbi.proxy, header, packets);
        }
        else
        {
            TWK_THROW_STREAM(IOException,
                             "Bad magic number in softimage file \"" << filename
                                                                     << "\"");
        }
    }

    void IOsoftimage::readPixels(ifstream& infile, const Header& header,
                                 FrameBuffer& fb, bool swap,
                                 const ChannelPacketVector& packets) const
    {
        vector<FrameBuffer*> fbs;
        for (FrameBuffer* f = &fb; f; f = f->nextPlane())
            fbs.push_back(f);
        const size_t npackets = packets.size();

        for (size_t row = 0; row < header.height; row++)
        {
            vector<char> buffer(header.width * 3);
            infile.read((char*)&buffer.front(), 10);

            if (row == 0)
            {
                cout << "row 0 = " << hex;

                for (size_t x = 0; x < 10; x++)
                {
                    cout << size_t((unsigned char)buffer[x]) << " ";
                }

                cout << dec << endl;
            }

            for (size_t i = 0; i < npackets; i++)
            {
                const ChannelPacket& ch = packets[i];

                if ((ch.channel & ColorChannels) == ColorChannels)
                {
                    if (ch.type == MixedRunLength)
                    {
                    }
                    else
                    {
                    }
                }
                else if (ch.channel & AlphaChannel)
                {
                    if (ch.type == MixedRunLength)
                    {
                    }
                    else
                    {
                    }
                }
            }
        }
    }

    void IOsoftimage::readImage(FrameBuffer& fb, const std::string& filename,
                                const ReadRequest& request) const
    {
        ifstream infile(UNICODE_C_STR(filename.c_str()), ios::binary);

        if (!infile)
        {
            TWK_THROW_STREAM(IOException, "Unable to open softimage file \""
                                              << filename << "\" for reading");
        }

        Header header;
        infile.read((char*)&header, sizeof(Header));

        ChannelPacketVector packets;
        ChannelPacket packet;
        packet.chained = 1;
        uint8 allchannels = 0;

        while (packet.chained)
        {
            infile.read((char*)&packet, sizeof(ChannelPacket));
            packets.push_back(packet);
            allchannels |= packet.channel;
        }

        int nchannels = 0;
        if (allchannels & RedChannel)
            nchannels++;
        if (allchannels & GreenChannel)
            nchannels++;
        if (allchannels & BlueChannel)
            nchannels++;
        if (allchannels & AlphaChannel)
            nchannels++;

        if (header.magic == Magic || header.magic == Cigam)
        {
            const bool swap = header.magic == Cigam;
            if (swap)
                header.swap();
            header.valid();

            const int w = header.width;
            const int h = header.height;

            FrameBuffer::DataType dataType = FrameBuffer::UCHAR;

            FrameBuffer::StringVector names;

            if (allchannels & ColorChannels)
            {
                names.push_back("R");
                names.push_back("G");
                names.push_back("B");
            }

            if (allchannels & AlphaChannel)
            {
                names.push_back("A");
            }

            //
            //  The softimage format is very weird -- just make the output
            //  planar so we don't have to have multiple ways of doing
            //  things. The downside is that it may not be the fastest reader
            //  possible.
            //

            fb.restructurePlanar(w, h, names, dataType, FrameBuffer::NATURAL);

            readPixels(infile, header, fb, swap, packets);

            readAttrs(fb, header, packets);
        }
        else
        {
            TWK_THROW_STREAM(IOException,
                             "Bad magic number in softimage file \"" << filename
                                                                     << "\"");
        }
    }

    void IOsoftimage::writeImage(const FrameBuffer& img,
                                 const std::string& filename,
                                 const WriteRequest& request) const
    {
        FrameBufferIO::writeImage(img, filename, request);
    }

} //  End namespace TwkFB
