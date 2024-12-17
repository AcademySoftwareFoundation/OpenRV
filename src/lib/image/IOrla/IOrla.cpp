//******************************************************************************
// Copyright (c) 2001-2004 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <iostream>
#include <vector>
#include <string>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <IOrla/IOrla.h>
#include <IOrla/rlaFormat.h>
#include <TwkMath/Vec2.h>
#include <TwkMath/Iostream.h>
#include <TwkFB/Operations.h>
#include <TwkFB/Exception.h>
#include <TwkUtil/File.h>

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

// #define TWK_THROW_STREAM(EXC, STREAM_TOKENS)    \
// {                                               \
//     std::cerr << STREAM_TOKENS << std::endl;    \
//     exit(1);                                    \
// }

namespace TwkFB
{

    using namespace std;

    IOrla::IOrla()
        : FrameBufferIO("IOrla", "m99")
        , m_error(false)
    {
        //
        //  Indicate which extensions this plugin will handle The
        //  comparison against. The extensions are case-insensitive so
        //  there's no reason to provide upper case versions.
        //

        unsigned int cap = ImageRead | BruteForceIO;

        addType("rla", "Wavefront/3DStudio Image", cap);
        addType("rpf", "3DStudio Image", cap);

        //
        //  NOTE: if we had different compressor types the addType calls
        //  might look like this:
        //
        //  addType("imgext", "IMG image file", cap, "RLE", "ZIP", "LZW", NULL);
        //
        //  Where "RLE", ... "LZW" are all compressor types for this image
        //  file format. RVIO and other programs will use this information
        //  supplied by the user.
        //

        assert(sizeof(RLAHeader) == 740);
    }

    IOrla::~IOrla() {}

    std::string IOrla::about() const { return "RLA (Tweak)"; }

    void IOrla::throwError(const std::string& filename, FILE* file = 0) const
    {
        if (m_error)
        {
            m_error = false;
            if (file)
                fclose(file);
            TWK_THROW_STREAM(TwkFB::IOException,
                             "RLA: failed to open file \"" << filename);
        }
    }

    void IOrla::getImageInfo(const std::string& filename, FBInfo& fbi) const
    {
        RLAHeader header;
        FILE* infile = NULL;

        if (!(infile = TwkUtil::fopen(filename.c_str(), "rb")))
        {
            TWK_THROW_STREAM(IOException, "RLA: cannot open " << filename);
        }

        if (fread(&header, sizeof(RLAHeader), 1, infile) != 1)
        {
            TWK_THROW_STREAM(TwkFB::IOException,
                             "RLA: failed to read header of \"" << filename);
        }

        fclose(infile);

        header.conformByteOrder();

        if (header.revision != RLA_MAGIC && header.revision != RPF_MAGIC)
        {
            TWK_THROW_STREAM(
                IOException,
                "RLA: not a supported format revision: " << filename);
        }

        if (header.chan_bits <= 8)
            fbi.dataType = FrameBuffer::UCHAR;
        else if (header.chan_bits <= 16)
            fbi.dataType = FrameBuffer::USHORT;
        else if (header.chan_bits == 32)
            fbi.dataType = FrameBuffer::FLOAT;
        else
            TWK_THROW_STREAM(IOException,
                             "RLA: unsupported bit depth: " << filename)

        fbi.width = header.window.right - header.window.left + 1;
        fbi.height = header.window.top - header.window.bottom + 1;
        fbi.numChannels = header.num_chan + header.num_matte + header.num_aux;
        fbi.orientation = FrameBuffer::NATURAL;
    }

    void IOrla::readImage(FrameBuffer& fb, const std::string& filename,
                          const ReadRequest& request) const
    {
        RLAHeader header;
        FILE* infile = NULL;

        //
        // Open the file and read the standard RLA header, byte-swapping if
        // necessary
        //
        if (!(infile = TwkUtil::fopen(filename.c_str(), "rb")))
        {
            TWK_THROW_STREAM(IOException, "RLA: cannot open " << filename);
        }
        if (fread(&header, sizeof(RLAHeader), 1, infile) != 1)
        {
            TWK_THROW_STREAM(TwkFB::IOException,
                             "RLA: failed to read header of " << filename);
        }
        header.conformByteOrder();

        const int dataWidth =
            header.active_window.right - header.active_window.left + 1;
        const int dataHeight =
            header.active_window.top - header.active_window.bottom + 1;

        // TODO: Handle the case where different channels have different bit
        // depths!

        //     if ( header.num_matte > 0 && header.matte_bits !=
        //     header.chan_bits )
        //     {
        //         TWK_THROW_STREAM(TwkFB::IOException, "RLA: All channels must
        //         have the same bit depth "
        //                          << filename);
        //     }
        //     if ( header.num_aux > 0 && header.aux_bits != header.chan_bits )
        //     {
        //         TWK_THROW_STREAM(TwkFB::IOException, "RLA: All channels must
        //         have the same bit depth "
        //                          << filename);
        //     }

        StringVector allChannelNames;
        if (header.num_chan == 1)
        {
            allChannelNames.push_back("Y");
        }
        else if (header.num_chan == 3)
        {
            allChannelNames.push_back("R");
            allChannelNames.push_back("G");
            allChannelNames.push_back("B");
        }
        else
        {
            for (int i = 0; i < header.num_chan; ++i)
            {
                char name[17];
                snprintf(name, 16, "Channel_%d", i);
                allChannelNames.push_back(name);
            }
        }
        if (header.num_matte == 1)
        {
            allChannelNames.push_back("A");
        }
        else if (header.num_matte > 1)
        {
            for (int i = 0; i < header.num_matte; ++i)
            {
                char name[17];
                snprintf(name, 16, "Alpha_%d", i);
                allChannelNames.push_back(name);
            }
        }
        if (header.num_aux == 1)
        {
            allChannelNames.push_back("Z");
        }
        if (header.num_aux > 1)
        {
            for (int i = 0; i < header.num_aux; ++i)
            {
                char name[17];
                snprintf(name, 16, "Aux_%d", i);
                allChannelNames.push_back(name);
            }
        }

        //
        // Allocate and read the scanline offset table, byte-swapping if
        // necessary
        //
        std::vector<int> offsets(dataHeight);
        if (fread(&offsets.front(), sizeof(int), dataHeight, infile)
            != dataHeight)
        {
            TWK_THROW_STREAM(TwkFB::IOException,
                             "RLA: failed to read offset table of "
                                 << filename);
        }
#if defined(TWK_LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN__)
        TwkUtil::swapWords(&offsets.front(), offsets.size());
#endif

        //
        // Look at the program string and get a bitmask of
        // 3DS Max 'extras' that are in this file
        //

        MaxExtras maxExtras = getMaxExtras(header);
        // MaxExtras maxExtras = MAX_NO_EXTRAS;

        //
        // Read the 3DS Max RenderInfo header if needed
        //
        MaxRenderInfo maxRenderInfo;
        if (maxExtras & MAX_RENDER_INFO)
        {
            short renderInfoVersion;
            if (fread(&renderInfoVersion, sizeof(short), 1, infile) <= 0)
            {
                TWK_THROW_STREAM(IOException, "RLA: " << strerror(errno));
            }
            int renderInfoSize = sizeof(MaxRenderInfo);
            if (renderInfoVersion != 1000)
            {
                renderInfoSize -= sizeof(
                    MaxRECT); // the old record didn't have the region Rect.

                // The old version didn't start with a version word, but
                // with the projType, so we need to move the file pointer
                // back by the size of one short to undo the version read.
                fseek(infile, -(long)(sizeof(short)), SEEK_CUR);
            }
            if (fread(&maxRenderInfo, renderInfoSize, 1, infile) <= 0)
            {
                TWK_THROW_STREAM(IOException, "RLA: " << strerror(errno));
            }
        }

        //
        // At this point, we would read the 3ds max 'node name table', if we
        // cared.  For now, just indicate its presence
        //

        if (maxExtras & MAX_GB_Z)
            allChannelNames.push_back("Z");
        if (maxExtras & MAX_GB_MTL_ID)
            allChannelNames.push_back("MTL_ID");
        if (maxExtras & MAX_GB_NODE_ID)
            allChannelNames.push_back("NODE_ID");
        if (maxExtras & MAX_GB_UV)
            allChannelNames.push_back("UV");
        if (maxExtras & MAX_GB_NORMAL)
            allChannelNames.push_back("NORMAL");
        if (maxExtras & MAX_GB_REALPIX)
            allChannelNames.push_back("REALPIX");
        if (maxExtras & MAX_GB_COVERAGE)
            allChannelNames.push_back("COVERAGE");
        if (maxExtras & MAX_GB_BG)
            allChannelNames.push_back("BG");
        if (maxExtras & MAX_GB_NODE_RENDER_ID)
            allChannelNames.push_back("NODE_RENDER_ID");
        if (maxExtras & MAX_GB_COLOR)
            allChannelNames.push_back("COLOR");
        if (maxExtras & MAX_GB_TRANSP)
            allChannelNames.push_back("TRANSP");
        if (maxExtras & MAX_GB_VELOC)
            allChannelNames.push_back("VELOC");
        if (maxExtras & MAX_GB_WEIGHT)
            allChannelNames.push_back("WEIGHT");
        if (maxExtras & MAX_GB_MASK)
            allChannelNames.push_back("MASK");

        //
        // Compute some basic image properties and do attribute stuff
        //
        const int width = header.window.right - header.window.left + 1;
        const int height = header.window.top - header.window.bottom + 1;
        const int depth = 0;
        const int numChannels =
            request.allChannels ? allChannelNames.size()
                                : std::min(allChannelNames.size(), (size_t)4);
        FrameBuffer::DataType dataType;
        if (header.chan_bits <= 8)
            dataType = FrameBuffer::UCHAR;
        else if (header.chan_bits <= 16)
            dataType = FrameBuffer::USHORT;
        else if (header.chan_bits == 32)
            dataType = FrameBuffer::FLOAT;
        else
            TWK_THROW_STREAM(IOException,
                             "RLA: unsupported bit depth: " << filename)

        StringVector channelNames;
        for (int i = 0; i < numChannels; ++i)
        {
            channelNames.push_back(allChannelNames[i]);
        }

        fb.restructure(width, height, depth, numChannels, dataType, NULL,
                       &channelNames, FrameBuffer::NATURAL, true);

        if (maxExtras & MAX_RENDER_INFO)
        {
            fb.newAttribute<string>("RLA/3DS/projectionType",
                                    maxRenderInfo.projType == ProjPerspective
                                        ? "Perspective"
                                        : "Orthographic");
            fb.newAttribute("RLA/3DS/projScale", maxRenderInfo.projScale);
            fb.newAttribute("RLA/3DS/origin", maxRenderInfo.origin);
            fb.newAttribute<string>(
                "RLA/3DS/fieldRender",
                maxRenderInfo.fieldRender
                    ? (maxRenderInfo.fieldOdd ? "Odd" : "Even")
                    : "No");
            char renderTimeStr[32];
            snprintf(renderTimeStr, 32, "%d %d", maxRenderInfo.renderTime[0],
                     maxRenderInfo.renderTime[1]);
            fb.newAttribute<string>("RLA/3DS/renderTime",
                                    std::string(renderTimeStr));
            fb.newAttribute("RLA/3DS/worldToCam(t0)",
                            maxRenderInfo.worldToCam[0].asMat44());
            fb.newAttribute("RLA/3DS/camToWorld(t0)",
                            maxRenderInfo.camToWorld[0].asMat44());
            char regionStr[64];
            snprintf(renderTimeStr, 32, "%d, %d - %d, %d",
                     maxRenderInfo.region.left, maxRenderInfo.region.top,
                     maxRenderInfo.region.right, maxRenderInfo.region.bottom);
            fb.newAttribute("RLA/3DS/region", std::string(renderTimeStr));
        }
        if (maxExtras & MAX_NODE_TABLE)
            fb.newAttribute<string>("RLA/3DS Max Node name table", "Yes");
        if (maxExtras & MAX_LAYER_DATA)
            fb.newAttribute<string>("RLA/3DS Max Layer Data", "Yes");
        if (maxExtras & MAX_UNPREMULT)
            fb.newAttribute<string>("RLA/Non-premultiplied alpha", "Yes");

        fb.newAttribute("RLA/frame", int(header.frame));
        fb.newAttribute("RLA/aspect", std::string(header.aspect));
        fb.newAttribute("RLA/aspect ratio", std::string(header.aspect_ratio));
        fb.newAttribute("RLA/field", int(header.field));
        fb.newAttribute("RLA/job num", int(header.job_num));
        fb.newAttribute("RLA/filter", std::string(header.filter));
        fb.newAttribute("RLA/name", std::string(header.name));
        fb.newAttribute("RLA/desc", std::string(header.desc));
        fb.newAttribute("RLA/program", std::string(header.program));
        fb.newAttribute("RLA/machine", std::string(header.machine));
        fb.newAttribute("RLA/user", std::string(header.user));
        fb.newAttribute("RLA/date", std::string(header.date));
        fb.newAttribute("RLA/time", std::string(header.time));
        fb.newAttribute("RLA/Chromaticities/white",
                        std::string(header.white_pt));
        fb.newAttribute("RLA/Chromaticities/blue",
                        std::string(header.blue_pri));
        fb.newAttribute("RLA/Chromaticities/green",
                        std::string(header.green_pri));
        fb.newAttribute("RLA/Chromaticities/red", std::string(header.red_pri));
        fb.newAttribute("RLA/chan", std::string(header.chan));
        fb.newAttribute("RLA/gamma", std::string(header.gamma));
        fb.newAttribute("RLA/num_aux", int(header.num_aux));
        fb.newAttribute("RLA/num_matte", int(header.num_matte));
        fb.newAttribute("RLA/num_chan", int(header.num_chan));

        char revisionStr[32];
        snprintf(revisionStr, 32, "%s (0x%hX)",
                 header.revision == RLA_MAGIC ? "Wavefront" : "3DS Max",
                 header.revision);
        fb.newAttribute("RLA/revision", std::string(revisionStr));

        if (header.window.left != header.active_window.left
            || header.window.right != header.active_window.right
            || header.window.bottom != header.active_window.bottom
            || header.window.top != header.active_window.top)
        {
            fb.newAttribute("DataWindowSize",
                            TwkMath::Vec2i(dataWidth, dataHeight));
            fb.newAttribute(
                "DataWindowOrigin",
                TwkMath::Vec2i(header.window.left, header.window.bottom));
            fb.newAttribute("DisplayWindowSize", TwkMath::Vec2i(width, height));
            fb.newAttribute("DisplayWindowOrigin",
                            TwkMath::Vec2i(header.active_window.left,
                                           header.active_window.bottom));
        }

        std::string channelNamesStr;
        for (size_t i = 0; i < allChannelNames.size(); ++i)
        {
            channelNamesStr += allChannelNames[i] + ", ";
        }
        channelNamesStr[channelNamesStr.size() - 2] = 0; // Trim the last comma
        fb.newAttribute<string>("ChannelNamesInFile", channelNamesStr);
        fb.newAttribute("ChannelsInFile", allChannelNames.size());

        fb.setPrimaryColorSpace(ColorSpace::Rec709());
        fb.setTransferFunction(ColorSpace::Linear());

        //
        // Now we do the actual decoding
        //
        std::vector<unsigned char> lineBuffer(fb.scanlineSize());
        const int stride = fb.pixelSize();

        for (int y = 0; y < dataHeight; ++y)
        {
            if (fseek(infile, offsets[y], SEEK_SET) != 0)
            {
                if (!fb.hasAttribute("PartialImage"))
                {
                    std::cerr << "WARNING: RLA: incomplete image \"" << filename
                              << "\"" << std::endl;
                    fb.newAttribute("PartialImage", 1.0);
                }
                break;
            }

            unsigned char* scanline =
                fb.scanline<unsigned char>(y + header.active_window.bottom);
            scanline += header.active_window.left * fb.pixelSize();
            for (int c = 0; c < numChannels; ++c)
            {
                const int offset = c * fb.bytesPerChannel();

                signed short runlength;
                if (fread(&runlength, 2, 1, infile) != 1)
                {
                    if (!fb.hasAttribute("PartialImage"))
                    {
                        std::cerr << "WARNING: RLA: incomplete image \""
                                  << filename << "\"" << std::endl;
                        fb.newAttribute("PartialImage", 1.0);
                    }
                    break;
                }
#if defined(TWK_LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN__)
                TwkUtil::swapShorts(&runlength, 1);
#endif

                if (fread(&lineBuffer.front(), runlength, 1, infile) != 1)
                {
                    if (!fb.hasAttribute("PartialImage"))
                    {
                        std::cerr << "WARNING: RLA: incomplete image \""
                                  << filename << "\"" << std::endl;
                        fb.newAttribute("PartialImage", 1.0);
                    }
                    break;
                }

                if (header.chan_bits <= 8)
                {
                    rlaDecodeScanline(&lineBuffer.front(), scanline + offset,
                                      width, width, stride);
                }
                else if (header.chan_bits <= 16)
                {
                    // MSB is always decoded first in an RLA file!  However,
                    // WHERE it gets decoded differs depending on the endianness
                    // of the machine:
#if defined(TWK_LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN__)
                    unsigned char* linepos = rlaDecodeScanline(
                        &lineBuffer.front(), scanline + offset + 1, width,
                        width, stride);
                    rlaDecodeScanline(linepos, scanline + offset, width, width,
                                      stride);
#else
                    unsigned char* linepos = rlaDecodeScanline(
                        &lineBuffer.front(), scanline + offset, width, width,
                        stride);
                    rlaDecodeScanline(linepos, scanline + offset + 1, width,
                                      width, stride);
#endif
                }
                else if (header.chan_bits == 32)
                {
#if defined(TWK_LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN__)
                    TwkUtil::swapWords(&lineBuffer.front(), dataWidth);
#endif
                    float* floatScanline = reinterpret_cast<float*>(scanline);
                    float* floatBuffer =
                        reinterpret_cast<float*>(&lineBuffer.front());
                    for (int x = 0; x < dataWidth; ++x)
                    {
                        floatScanline[x * numChannels + c] = floatBuffer[x];
                    }
                }
            }

            // Promote funky bit depths to match framebuffer
            if (header.chan_bits < 8)
            {
                for (size_t i = 0; i < dataWidth * numChannels; ++i)
                {
                    scanline[i] = scanline[i] << (8 - header.chan_bits);
                }
            }
            else if (header.chan_bits != 8 && header.chan_bits < 16)
            {
                unsigned short* shortScanline = (unsigned short*)scanline;
                for (size_t i = 0; i < dataWidth * numChannels; ++i)
                {
                    shortScanline[i] = shortScanline[i]
                                       << (16 - header.chan_bits);
                }
            }
        }

        fclose(infile);
    }

#if 0
void
IOrla::writeImage(const FrameBuffer& img,
                   const std::string& filename,
                   const WriteRequest& request) const
{
}
#endif

} //  End namespace TwkFB

#ifdef _MSC_VER
#undef snprintf
#endif
