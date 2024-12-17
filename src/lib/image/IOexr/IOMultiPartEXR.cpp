//******************************************************************************
// Copyright (c) 2001-2013 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <IOexr/IOexr.h>
#include <IOexr/FileStreamIStream.h>
#include <ImfBoxAttribute.h>
#include <ImathBoxAlgo.h>
#include <ImfChannelList.h>
#include <ImfCompression.h>
#include <ImfVersion.h>
#include <ImfStandardAttributes.h>
#include <ImfCompressionAttribute.h>
#include <ImfTileDescriptionAttribute.h>
#include <ImfMultiPartInputFile.h>
#include <ImfInputPart.h>
#include <ImfInputFile.h>
#include <ImfMultiPartOutputFile.h>
#include <ImfOutputPart.h>
#include <ImfPartType.h>
#include <ImfRgbaFile.h>
#include <ImfIntAttribute.h>
#include <ImfLineOrderAttribute.h>
#include <ImfMatrixAttribute.h>
#include <ImfMultiView.h>
#include <ImfOutputFile.h>
#include <ImfAcesFile.h>
#include <ImfStringAttribute.h>
#include <ImfStringVectorAttribute.h>
#include <ImfTileDescription.h>
#include <ImfVecAttribute.h>
#include <TwkUtil/Interrupt.h>
#include <TwkUtil/TwkRegEx.h>
#include <TwkMath/Iostream.h>
#include <TwkMath/Vec2.h>
#include <TwkFB/Exception.h>
#include <TwkFB/Operations.h>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stl_ext/string_algo.h>
#include <string>
#include <vector>
#include <map>
#include <list>

#include <IOexr/Logger.h>

namespace TwkFB
{
    using namespace std;
    using namespace TwkUtil;

    //
    //  Gets the bigger of the types specified by fbDataType/exrPixelTypes and
    //  channel.
    //
    void IOexr::getBiggerFrameBufferAndEXRPixelType(
        const Imf::Channel& channel, FrameBuffer::DataType& fbDataType,
        Imf::PixelType& exrPixelType)
    {
        switch (channel.type)
        {
        case Imf::FLOAT:
            fbDataType = FrameBuffer::FLOAT;
            exrPixelType = Imf::FLOAT;
            break;
        case Imf::HALF:
            if (fbDataType != FrameBuffer::FLOAT)
            {
                fbDataType = FrameBuffer::HALF;
                exrPixelType = Imf::HALF;
            }
            break;
        case Imf::UINT:
            fbDataType = FrameBuffer::UINT;
            exrPixelType = Imf::UINT;
            break;
        default:
            TWK_EXC_THROW_WHAT(Exception, "Unsupported exr data type");
        }
    }

    void IOexr::readMultiPartChannelList(
        const std::string& filename, const std::string& view, FrameBuffer& fb,
        Imf::MultiPartInputFile& file, vector<MultiPartChannel>& channelsRead,
        bool convertYRYBY, bool planar3channel, bool allChannels,
        bool inheritChannels, bool noOneChannelPlanes, bool stripAlpha,
        bool readWindowIsDisplayWindow, IOexr::ReadWindow window)
    {
        // Move the outfb setup and attributes to
        // a new function.
        // The return dsp and dat windows

        const int partNum = channelsRead[0].partNumber;
        vector<MultiPartChannel> channelsReadPartZero;
        FrameBuffer* outfb = &fb;
        Imath::Box2i dspWin = file.header(partNum).displayWindow();
        Imath::Box2i datWin = file.header(partNum).dataWindow();
        Imath::Box2i unionWin;
        unionWin.extendBy(datWin);
        unionWin.extendBy(dspWin);

        Imath::Box2i bufWin;
        Imath::Box2i clipped;
        bool dataWinOutsideDspWin = false;

        switch (window)
        {
        case IOexr::DataWindow:
            bufWin.extendBy(datWin);
            if (readWindowIsDisplayWindow)
                dspWin = bufWin;
            break;
        default:
        case IOexr::DataInsideDisplayWindow:
            bufWin.extendBy(datWin);
            clipped.min = Imath::clip(dspWin.min, datWin);
            clipped.max = Imath::clip(dspWin.max, datWin);

            if (!dspWin.intersects(clipped.min)
                && !dspWin.intersects(clipped.max))
            {
                // datwin outside dspWin
                dataWinOutsideDspWin = true;
            }
            else
            {
                if (!dspWin.intersects(datWin.min)
                    || !dspWin.intersects(datWin.max))
                {
                    outfb = new FrameBuffer();
                }
            }
            break;
        case IOexr::DisplayWindow:
            bufWin.extendBy(unionWin);
            if (unionWin != dspWin)
                outfb = new FrameBuffer();
            break;
        case IOexr::UnionWindow:
            bufWin.extendBy(unionWin);
            if (readWindowIsDisplayWindow)
                dspWin = bufWin;
            break;
        }

        int width = bufWin.max.x - bufWin.min.x + 1;
        int height = bufWin.max.y - bufWin.min.y + 1;
        int spadding = 0;
        float pixelAspect = file.header(partNum).pixelAspectRatio();

        const bool isMultiPart = (file.parts() > 1 ? true : false);

        const int totalChannels = channelsRead.size();
        bool planarYRYBY = false;
        bool planar3 = false;

        vector<string> sampleNames;
        string originalChannels;
        ostringstream originalSampling;
        ostringstream partsInFileNameStr;
        ostringstream partsInFileNumStr;

        set<int> partNumbersInFile;

        for (int i = 0; i < channelsRead.size(); ++i)
        {
            const MultiPartChannel& mpChannel = channelsRead[i];

            if (isMultiPart)
            {
                partNumbersInFile.insert(mpChannel.partNumber);
            }

            if (mpChannel.partNumber == 0)
            {
                channelsReadPartZero.push_back(mpChannel);
            }

            string sep;
            if (i)
            {
                if (mpChannel.partNumber != channelsRead[i - 1].partNumber)
                {
                    // Use ';' as a separate as we are on new part's channels.
                    sep = " | ";
                }
                else
                {
                    sep = ", ";
                }

                originalChannels += sep;
                originalSampling << sep;
            }

            originalChannels += mpChannel.name;
            originalSampling << mpChannel.name << ":";

            if (mpChannel.channel.xSampling == mpChannel.channel.ySampling)
            {
                originalSampling << mpChannel.channel.xSampling;
            }
            else
            {
                originalSampling << mpChannel.channel.xSampling << "x"
                                 << mpChannel.channel.ySampling;
            }

            if ((!strcmp(mpChannel.name.c_str(), "RY")
                 || !strcmp(mpChannel.name.c_str(), "BY")))
            {
                if (!convertYRYBY)
                {
                    planarYRYBY = true;
                }
            }
        }

#ifdef DEBUG_IOEXR
        for (int i = 0; i < channelsRead.size(); ++i)
        {
            LOG.log("Before sort: Reading full channel name: %s",
                    channelsRead[i].fullname.c_str());
        }
#endif

        //
        //  If no layer or view is specified, nuke any layers
        //

        if (!allChannels)
        {
            //
            // Choose the default Part/Layer following that logic:
            // - Choose Part 0 if it contain at least the rgb channel
            // - Find the part containing the rgba layer (From Ordering)
            // - If we are guessing the inheritance, we use the first r/g/b/a we
            // find from any part
            // - If not, we use the first element in our list
            //

            bool reduced = false;
            int r = redIndex(channelsReadPartZero);
            int g = greenIndex(channelsReadPartZero);
            int b = blueIndex(channelsReadPartZero);
            if (r != -1 && g != -1 && b != -1)
            {
                channelsRead = channelsReadPartZero;
                reduced = true;
            }

            //
            //  Sort the channels so that RGBA layer > RGBA channel are first in
            //  the list and the rest of the channels appear in their natural
            //  order
            //
            stable_sort(channelsRead.begin(), channelsRead.end(),
                        ChannelCompMP);

            if (inheritChannels && !reduced)
            {
                // Use first alpha/rgb channel from any layer/part
                int a = alphaIndex(channelsRead);

                if (a != -1)
                {
                    int r = redIndex(channelsRead);
                    int g = greenIndex(channelsRead);
                    int b = blueIndex(channelsRead);

                    if (r != -1 && g != -1 && b != -1)
                    {
                        vector<MultiPartChannel> newChannels;
                        newChannels.push_back(channelsRead[r]);
                        newChannels.push_back(channelsRead[g]);
                        newChannels.push_back(channelsRead[b]);
                        newChannels.push_back(channelsRead[a]);
                        channelsRead = newChannels;
                    }
                }
            }
            else
            {
                //
                // Reduce channelsRead to the first layer/part in list
                // (Could be an incomplete rgb layer)
                //
                if (!reduced)
                {
                    int firstPartNum = channelsRead[0].partNumber;
                    vector<MultiPartChannel> channelsReorderedByPart;

                    // Add all matching part number channels in order.
                    for (int i = 0; i < channelsRead.size(); ++i)
                    {
                        if (channelsRead[i].partNumber == firstPartNum)
                        {
                            channelsReorderedByPart.push_back(channelsRead[i]);
                        }
                    }

                    // Finally, reassign the vector back.
                    channelsRead = channelsReorderedByPart;
                }
            }
        }
        else
        {
            stable_sort(channelsRead.begin(), channelsRead.end(),
                        ChannelCompMP);

            int r = redIndex(channelsReadPartZero);
            int g = greenIndex(channelsReadPartZero);
            int b = blueIndex(channelsReadPartZero);
            if (r != -1 && g != -1 && b != -1)
            {
                // If Part 0 has a valid RGB, put it back to the front
                vector<MultiPartChannel> channelsReorderedByPart;
                channelsReorderedByPart.push_back(channelsReadPartZero[r]);
                channelsReorderedByPart.push_back(channelsReadPartZero[g]);
                channelsReorderedByPart.push_back(channelsReadPartZero[b]);
                int a = alphaIndex(channelsReadPartZero);
                if (a != -1)
                    channelsReorderedByPart.push_back(channelsReadPartZero[a]);

                for (int i = 0; i < channelsRead.size(); ++i)
                {
                    if (channelsRead[i].partNumber != 0)
                    {
                        channelsReorderedByPart.push_back(channelsRead[i]);
                    }
                }
                channelsRead = channelsReorderedByPart;
            }
        }

#ifdef DEBUG_IOEXR
        for (int i = 0; i < channelsRead.size(); ++i)
        {
            LOG.log("After sort: Reading updated full channel name: %s",
                    channelsRead[i].fullname.c_str());
        }
#endif

        // Store the version with channels relevant with the layer/parts
        vector<MultiPartChannel> planarRead;
        if (planarYRYBY || planar3channel)
        {
            planarRead = channelsRead;
        }

        //
        //  If we only have three channels and they are known r, g, b channels
        //  and there's a default alpha use it.
        //
        if (inheritChannels && channelsRead.size() == 3 && !stripAlpha)
        {
            if (redIndex(channelsRead) != -1 && greenIndex(channelsRead) != -1
                && blueIndex(channelsRead) != -1)
            {
                //
                //  See if there's a default alpha channel
                //
                MultiPartChannel alpha;
                findAnAlphaInView(file, view, channelsRead.front().partName,
                                  alpha);
                if (alpha.name != "")
                {
                    channelsRead.push_back(alpha);
                }
            }
        }

        //
        //  Unless we're reading all of the channels, nuke all but the first
        //  four channels.
        //
        int numChannels = 0;
        if (!allChannels)
        {
            int a = alphaIndex(channelsRead);
            int r = redIndex(channelsRead);
            if (channelsRead.size() >= 4)
            {
                // Alpha channel need to come from the same part unless we are
                // guessing/inheriting
                if ((a == 3) && !stripAlpha
                    && (inheritChannels
                        || channelsRead[a].partNumber
                               == channelsRead[r].partNumber))
                {
                    numChannels = 4;
                }
                else
                {
                    numChannels = 3;
                }
            }
            else
            {
                numChannels = channelsRead.size();
            }
        }
        else
        {
            numChannels = channelsRead.size();
        }
        planar3 = numChannels == 3 && planar3channel;

        //
        //  Setup framebuffer
        //
        Imf::PixelType exrPixelType = Imf::HALF;
        vector<vector<MultiPartChannel>::const_iterator> planeChannels;
        if (planarYRYBY || planar3)
        {
            vector<MultiPartChannel>::const_iterator ci0 =
                findChannelWithBasenameMP(planarYRYBY ? "Y" : "R", planarRead);
            vector<MultiPartChannel>::const_iterator ci1 =
                findChannelWithBasenameMP(planarYRYBY ? "RY" : "G", planarRead);
            vector<MultiPartChannel>::const_iterator ci2 =
                findChannelWithBasenameMP(planarYRYBY ? "BY" : "B", planarRead);
            vector<MultiPartChannel>::const_iterator ci3 =
                findChannelWithBasenameMP("A", planarRead);

            bool checkForDuplicates = false;

            if (ci0 == planarRead.end())
            {
                checkForDuplicates = true;
                ci0 = planarRead.begin();
            }

            if (ci1 == planarRead.end())
            {
                checkForDuplicates = true;
                if (planarRead.size() > 1)
                {
                    ci1 = ++planarRead.begin();
                }
                else
                {
                    ci1 = planarRead.begin();
                }
            }

            if (ci2 == planarRead.end())
            {
                checkForDuplicates = true;
                if (planarRead.size() > 2)
                {
                    ci2 = ++(++planarRead.begin());
                }
                else
                {
                    if (planarRead.size() > 1)
                    {
                        ci2 = ++planarRead.begin();
                    }
                    else
                    {
                        ci2 = planarRead.begin();
                    }
                }
            }

            bool has4 =
                (!stripAlpha && ci3 != planarRead.end() && numChannels > 3);

            planeChannels.resize((has4 || noOneChannelPlanes) ? 4 : 3);
            FrameBuffer::StringVector planeNames(has4 ? 4 : 3);
            FrameBuffer::Samplings xsamps(has4 || noOneChannelPlanes ? 4 : 3);
            FrameBuffer::Samplings ysamps(has4 || noOneChannelPlanes ? 4 : 3);

            planeChannels[0] = ci0;
            planeChannels[1] = ci1;
            planeChannels[2] = ci2;

            planeNames[0] = planarYRYBY ? "Y" : "R";
            xsamps[0] = ci0->channel.xSampling;
            ysamps[0] = ci0->channel.ySampling;

            planeNames[1] = planarYRYBY ? "RY" : "G";
            xsamps[1] = ci1->channel.xSampling;
            ysamps[1] = ci1->channel.ySampling;

            planeNames[2] = planarYRYBY ? "BY" : "B";
            xsamps[2] = ci2->channel.xSampling;
            ysamps[2] = ci2->channel.ySampling;

            if (has4 || noOneChannelPlanes)
            {
                planeChannels[3] = ci3;
                planeNames.resize(4);
                planeNames[3] = "A";
                xsamps[3] = ci3->channel.xSampling;
                ysamps[3] = ci3->channel.ySampling;
            }

            size_t nchannels = noOneChannelPlanes ? 2 : 1;

            if (noOneChannelPlanes)
            {
                planeNames.erase(planeNames.begin() + 3);
                planeChannels.erase(planeChannels.begin() + 3);
                planeNames.insert(planeNames.begin() + 1, "A");
                planeChannels.insert(planeChannels.begin() + 1, ci3);
            }

            //
            // We scan through all the read channels and pick
            // the biggest of all the channel types i.e. float.
            //
            FrameBuffer::DataType dataType = FrameBuffer::HALF;
            for (int i = 0; i < nchannels && dataType != FrameBuffer::FLOAT;
                 ++i)
            {
                getBiggerFrameBufferAndEXRPixelType(
                    planeChannels[i]->channel.type, dataType, exrPixelType);
            }

            if (checkForDuplicates)
            {
                for (int i = 0; i < planeChannels.size(); ++i)
                {
                    // Remove duplicates
                    for (int j = i + 1; j < planeChannels.size();)
                    {
                        if (planeChannels[i] == planeChannels[j])
                        {
                            planeChannels.erase(planeChannels.begin() + j);
                            planeNames.erase(planeNames.begin() + j);
                            xsamps.erase(xsamps.begin() + j);
                            ysamps.erase(ysamps.begin() + j);
                        }
                        else
                        {
                            ++j;
                        }
                    }
                }
            }
#ifdef DEBUG_IOEXR
            LOG.log("outfb channel type is %d %d", (int)dataType,
                    (int)exrPixelType);
            LOG.log("planeNamesSize=%d has4=%d noOneChannelPlanes=%d",
                    (int)planeNames.size(), (int)has4, (int)noOneChannelPlanes);
#endif
            outfb->restructurePlanar(width, height, xsamps, ysamps, planeNames,
                                     dataType, FrameBuffer::BOTTOMLEFT,
                                     nchannels);
        }
        else
        {
            //
            // We scan through all the read channels and pick
            // the biggest of all the channel types i.e. float.
            //
            FrameBuffer::DataType dataType = FrameBuffer::HALF;
            for (int c = 0; c < numChannels && dataType != FrameBuffer::FLOAT;
                 ++c)
            {
                getBiggerFrameBufferAndEXRPixelType(
                    channelsRead[c].channel.type, dataType, exrPixelType);
            }

#ifdef DEBUG_IOEXR
            LOG.log("outfb channel type is %d %d", (int)dataType,
                    (int)exrPixelType);
#endif
            outfb->restructure(width, height, 0, numChannels, dataType);
        }

        if (!planarYRYBY && !planar3)
            for (int c = 0; c < numChannels; ++c)
            {
                outfb->setChannelName(c, channelsRead[c].name);
            }

        //
        //  Color space (EXR is always linear with Rec709 primaries)
        //
        outfb->setPrimaryColorSpace(ColorSpace::Rec709());
        outfb->setTransferFunction(ColorSpace::Linear());

        //
        //  Assign attributes
        //
        set<int> partNumbersRead;
        if (isMultiPart)
        {
            for (int i = 0; i < channelsRead.size(); ++i)
            {
                const MultiPartChannel& mpChannel = channelsRead[i];
                partNumbersRead.insert(mpChannel.partNumber);
            }
        }

        readAllAttributes(file, *outfb, partNumbersRead);

        //
        //  Copy over the channel names
        //
        {
            string cch;
            if (planarYRYBY || planar3)
            {
                ostringstream str;

                for (int c = 0; c < numChannels; ++c)
                {
                    if (c)
                        str << ", ";
                    str << planeChannels[c]->name;
                }

                cch = str.str();
            }
            else
            {
                for (int c = 0; c < numChannels; ++c)
                {
                    if (c)
                        cch += ", ";
                    cch += channelsRead[c].name;
                }
            }

            outfb->newAttribute("ChannelsInFile", totalChannels);
            outfb->newAttribute("ChannelNamesInFile",
                                stl_ext::wrap(originalChannels));
            outfb->newAttribute("ChannelSamplingInFile",
                                stl_ext::wrap(originalSampling.str()));
            outfb->newAttribute("ChannelsRead", stl_ext::wrap(cch));
        }

        // Attributes for parts.
        if (isMultiPart)
        {
            ostringstream nameStr;
            ostringstream numStr;

            for (set<int>::const_iterator it = partNumbersInFile.begin();
                 it != partNumbersInFile.end(); ++it)
            {
                if (it != partNumbersInFile.begin())
                {
                    nameStr << ", ";
                    numStr << ", ";
                }
                numStr << *it;
                nameStr << (file.header(*it).hasName() ? file.header(*it).name()
                                                       : "");
            }
            outfb->newAttribute("PartNumbersInFile",
                                stl_ext::wrap(numStr.str()));
            outfb->newAttribute("PartNamesInFile",
                                stl_ext::wrap(nameStr.str()));

            nameStr.clear();
            nameStr.str("");
            numStr.clear();
            numStr.str("");

            // For numbers and names of parts that are read for display
            for (set<int>::const_iterator it = partNumbersRead.begin();
                 it != partNumbersRead.end(); ++it)
            {
                if (it != partNumbersRead.begin())
                {
                    nameStr << ", ";
                    numStr << ", ";
                }
                numStr << *it;
                nameStr << (file.header(*it).hasName() ? file.header(*it).name()
                                                       : "");
            }
            outfb->newAttribute("PartNumbersRead", stl_ext::wrap(numStr.str()));
            outfb->newAttribute("PartNamesRead", stl_ext::wrap(nameStr.str()));
        }

        // Read ReadWindow Attribute
        {
            const char* rw = 0;
            switch (window)
            {
            case IOexr::DataWindow:
                rw = "Data";
                break;
            case IOexr::DisplayWindow:
                rw = "Display";
                break;
            case IOexr::DataInsideDisplayWindow:
                rw = "Data Cropped to Display";
                break;
            default:
            case IOexr::UnionWindow:
                rw = "Union";
                break;
            }
            outfb->newAttribute("IOexr/ReadWindow", string(rw));
        }

        //
        //  Initialze unused pixels -- this could be optimized by
        //  carefully setting only the unused portions instead of the
        //  entire image.
        //

        if (datWin != bufWin)
        {
            for (FrameBuffer* f = outfb; f; f = f->nextPlane())
            {
                memset(f->pixels<unsigned char>(), 0, f->allocSize());
            }
        }

        //
        //  Set up uncrop regions
        //

        if (bufWin != dspWin)
        {
            outfb->setUncrop(dspWin.size().x + 1,          // width
                             dspWin.size().y + 1,          // height
                             bufWin.min.x - dspWin.min.x,  // x offset
                             bufWin.min.y - dspWin.min.y); // y offset
        }

        //
        //  Set the orientation
        //

        for (FrameBuffer* f = outfb; f; f = f->nextPlane())
        {
            f->setOrientation(FrameBuffer::TOPLEFT);
        }

        //
        //  Set pixel aspect ratio (if non-square)
        //

        for (FrameBuffer* f = outfb; f; f = f->nextPlane())
        {
            f->setPixelAspectRatio(pixelAspect);
        }

        //
        //  This is the general reader path.  Make exr frame buffer
        //  for reading. Note the funky origin of the frame buffer --
        //  its actually an out-of-bounds pointer. This is because we
        //  only want to read the data window (not the union of the
        //  display and data windows).
        //

        // We have one Imf::FrameBuffer for each part in the
        // file. Note: not all them will be populated; only
        // those where the requested channels are located.
        //
        vector<Imf::FrameBuffer> exrFrameBuffer;
        exrFrameBuffer.resize(file.parts());

        //
        //  Initialize partsRead for file reading.
        //
        set<int> partsRead;
        if (planarYRYBY || planar3)
        {
            if (noOneChannelPlanes)
            {
                unsigned int chindex = 0;

                for (FrameBuffer* p = outfb; p; p = p->nextPlane())
                {
                    vector<MultiPartChannel>::const_iterator ci =
                        planeChannels[chindex++];

                    int xs = ci->channel.xSampling;
                    int ys = ci->channel.ySampling;
#ifdef DEBUG_IOEXR
                    LOG.log("X Reading part channel number: %d name: %s",
                            ci->partNumber, ci->name.c_str());
#endif
                    partsRead.insert(ci->partNumber);
                    exrFrameBuffer[ci->partNumber].insert(
                        ci->name,
                        Imf::Slice(exrPixelType,
                                   p->pixels<char>()
                                       - p->scanlineSize() * bufWin.min.y
                                       - p->pixelSize() * bufWin.min.x,
                                   p->pixelSize(), p->scanlineSize(), xs, ys));

                    ci = planeChannels[chindex++];

                    if (ci != planarRead.end())
                    {
                        xs = ci->channel.xSampling;
                        ys = ci->channel.ySampling;
#ifdef DEBUG_IOEXR
                        LOG.log("XX Reading part channel number: %d name: %s",
                                ci->partNumber, ci->name.c_str());
#endif
                        partsRead.insert(ci->partNumber);
                        exrFrameBuffer[ci->partNumber].insert(
                            ci->name,
                            Imf::Slice(exrPixelType,
                                       p->pixels<char>()
                                           - p->scanlineSize() * bufWin.min.y
                                           - p->pixelSize() * bufWin.min.x
                                           + p->bytesPerChannel(),
                                       p->pixelSize(), p->scanlineSize(), xs,
                                       ys));
                    }
                    else
                    {
                        outfb->attribute<string>("AlphaType") = "None";
                    }
                }
            }
            else
            {
                unsigned int chindex = 0;

                for (FrameBuffer* p = outfb; p; p = p->nextPlane())
                {
                    vector<MultiPartChannel>::const_iterator ci =
                        planeChannels[chindex++];

                    const int xs = ci->channel.xSampling;
                    const int ys = ci->channel.ySampling;

#ifdef DEBUG_IOEXR
                    LOG.log("XXX Reading part channel number: %d name: %s "
                            "xs=%d ys=%d",
                            ci->partNumber, ci->name.c_str(), xs, ys);
#endif
                    partsRead.insert(ci->partNumber);
                    exrFrameBuffer[ci->partNumber].insert(
                        ci->name,
                        Imf::Slice(exrPixelType,
                                   p->pixels<char>()
                                       - p->scanlineSize() * bufWin.min.y
                                       - p->pixelSize() * bufWin.min.x,
                                   p->pixelSize(), p->scanlineSize(), xs, ys));
                }
            }
        }
        else
        {
            for (int c = 0; c < numChannels; ++c)
            {
                const MultiPartChannel& mpChannel = channelsRead[c];
                partsRead.insert(mpChannel.partNumber);
#ifdef DEBUG_IOEXR
                LOG.log("XXXX Reading part channel number: %d name: %s",
                        mpChannel.partNumber, mpChannel.name.c_str());
#endif
                exrFrameBuffer[mpChannel.partNumber].insert(
                    mpChannel.name.c_str(),  // name
                    Imf::Slice(exrPixelType, // type
                               outfb->pixels<char>()
                                   - outfb->scanlineSize() * bufWin.min.y
                                   - outfb->pixelSize() * bufWin.min.x
                                   + outfb->bytesPerChannel() * c,
                               outfb->pixelSize(),      // xStride
                               outfb->scanlineSize())); // yStride
            }
        }

        //
        //  Read in the data of the channels from the
        //  various parts into outfb.
        //
        bool isPartialImage = false;
        int i = 0;
        for (set<int>::const_iterator it = partsRead.begin();
             it != partsRead.end(); ++it, ++i)
        {
#ifdef DEBUG_IOEXR
            LOG.log("Reading part %d into framebuffer %d... ", (*it), i);
#endif
            // From set of parts
            Imf::InputPart inpart(file, (*it));
            inpart.setFrameBuffer(exrFrameBuffer[*it]);

            try
            {
                inpart.readPixels(datWin.min.y, datWin.max.y);
            }
            catch (...)
            {
                isPartialImage = true;
            }
        }

        if (isPartialImage)
        {
            cerr << "WARNING: EXR: incomplete image \"" << filename << "\""
                 << endl;
            if (!outfb->hasAttribute("PartialImage"))
            {
                outfb->newAttribute("PartialImage", 1.0f);
            }
        }

        if (outfb != &fb)
        {
            // Note: We will never get into this if-block of
            // code for the case were the data window is outside
            // the display window because outfb == fb in this case.

            //
            //  Crop from the buffer window
            //

            if (window == IOexr::DisplayWindow)
            {
                const int x0 = dspWin.min.x - bufWin.min.x;
                const int y0 = dspWin.min.y - bufWin.min.y;
                const int x1 = dspWin.max.x - bufWin.min.x;
                const int y1 = dspWin.max.y - bufWin.min.y;

                cropInto(outfb, &fb, x0, y0, x1, y1);
                outfb->copyAttributesTo(&fb);
            }
            else if (window == IOexr::DataInsideDisplayWindow)
            {
                const int x0 = clipped.min.x - datWin.min.x;
                const int y0 = clipped.min.y - datWin.min.y;
                const int x1 = clipped.max.x - datWin.min.x;
                const int y1 = clipped.max.y - datWin.min.y;

                cropInto(outfb, &fb, x0, y0, x1, y1);
                outfb->copyAttributesTo(&fb);
                fb.setUncrop(dspWin.size().x + 1, dspWin.size().y + 1,
                             clipped.min.x - dspWin.min.x,
                             clipped.min.y - dspWin.min.y);
            }
            else
            {
                outfb->copyAttributesTo(&fb);
            }

            fb.setOrientation(FrameBuffer::TOPLEFT);

            for (FrameBuffer* f = &fb; f; f = f->nextPlane())
            {
                f->setPixelAspectRatio(outfb->pixelAspectRatio());
            }
            delete outfb;
        }
    }

    //
    //  Returns true if the requestedView and requestedLayer
    //  matches the layer of the channelNameInFile in the
    //  partNameInFile when viewHasChannelConflict is either true or false.
    //
    bool IOexr::doesRequestedLayerExists(bool viewHasChannelConflict,
                                         const string& requestedView,
                                         const string& requestedLayer,
                                         const string& partNameInFile,
                                         const string& channelNameInFile)
    {

        if (viewHasChannelConflict)
        {
            string fullChannelNameInFile =
                partNameInFile + "." + channelNameInFile;
            string baseName, layerNameInFile;
            channelSplit(fullChannelNameInFile, baseName, layerNameInFile);
            // Lets check if part names contain no occurrences of .<view> in
            // their names. i.e. orig part name = <rv layer>
            if (layerNameInFile == requestedLayer)
                return true;

            if (!requestedView.empty())
            {
                // Lets check if view was stripped at the end of orig part name
                // =  <rvlayer>.<view>
                string requestedLayerInFile =
                    requestedLayer + "." + requestedView;
                if (layerNameInFile == requestedLayerInFile)
                    return true;

                // Lets check if view was stripped at the beginning of orig part
                // name =  <view>.<rvlayer>
                requestedLayerInFile = requestedView + "." + requestedLayer;
                if (layerNameInFile == requestedLayerInFile)
                    return true;

                // Lets check if view was stripped at locations of "." of orig
                // part name.
                size_t pos = 0;
                string viewStr = "." + requestedView + ".";
                requestedLayerInFile = requestedLayer;
                while ((pos = requestedLayerInFile.find(".", pos))
                       != std::string::npos)
                {
                    requestedLayerInFile.replace(pos, 1, viewStr);
                    pos += viewStr.length();
                }

                if (layerNameInFile == requestedLayerInFile)
                    return true;
            }
        }
        else
        {
            string baseName, layerNameInFile;
            channelSplit(channelNameInFile, baseName, layerNameInFile);
            if (layerNameInFile == requestedLayer)
                return true;
        }
        return false;
    }

    //
    //  Returns true if the requestedView, requestedLayer and requestedChannel
    //  matches the channel of the channelNameInFile in the
    //  partNameInFile when viewHasChannelConflict is either true or false.
    //
    bool IOexr::doesRequestedChannelExists(bool viewHasChannelConflict,
                                           const string& requestedView,
                                           const string& requestedLayer,
                                           const string& requestedChannel,
                                           const string& partNameInFile,
                                           const string& channelNameInFile)
    {
        string requestLayerDot = requestedLayer + ".";
        string requestedFullChannelNameInFile =
            requestLayerDot + requestedChannel;

        if (viewHasChannelConflict)
        {
            string fullChannelNameInFile =
                partNameInFile + "." + channelNameInFile;
            // Lets check if part names contain no occurrences of .<view> in
            // their names. i.e. orig part name = <rv layer>
            if (fullChannelNameInFile == requestedFullChannelNameInFile)
                return true;

            if (!requestedView.empty())
            {
                // Lets check if view was stripped at the end of orig part name
                // =  <rvlayer>.<view>
                requestedFullChannelNameInFile =
                    requestLayerDot + requestedView + "." + requestedChannel;
                if (fullChannelNameInFile == requestedFullChannelNameInFile)
                    return true;

                // Lets check if view was stripped at the beginning of orig part
                // name =  <view>.<rvlayer>
                requestedFullChannelNameInFile =
                    requestedView + "." + requestLayerDot + requestedChannel;
                if (fullChannelNameInFile == requestedFullChannelNameInFile)
                    return true;

                // Lets check if view was stripped at locations of "." of orig
                // part name.
                size_t pos = 0;
                string viewStr = "." + requestedView + ".";
                requestedFullChannelNameInFile = requestedLayer;
                while ((pos = requestedFullChannelNameInFile.find(".", pos))
                       != std::string::npos)
                {
                    requestedFullChannelNameInFile.replace(pos, 1, viewStr);
                    pos += viewStr.length();
                }
                requestedFullChannelNameInFile =
                    requestedFullChannelNameInFile + "." + requestedChannel;
                if (fullChannelNameInFile == requestedFullChannelNameInFile)
                    return true;
            }
        }
        else
        {
            if (channelNameInFile == requestedFullChannelNameInFile)
                return true;
        }
        return false;
    }

    void IOexr::addToMultiPartChannelList(vector<MultiPartChannel>& rcl,
                                          const int partNumber,
                                          const string& partName,
                                          const string& channelName,
                                          const Imf::Channel& channel)
    {
        MultiPartChannel mpChannel;
        mpChannel.partNumber = partNumber;
        mpChannel.partName = partName;
        mpChannel.name = channelName;
        mpChannel.fullname = fullChannelName(mpChannel);
        mpChannel.channel = channel;
        rcl.push_back(mpChannel);
    }

    void IOexr::readImagesFromMultiPartFile(
        Imf::MultiPartInputFile& file, FrameBufferVector& fbs,
        const std::string& filename, const string& requestedView,
        const string& requestedLayer, const string& requestedChannel,
        const bool requestedAllChannels) const
    {
#ifdef DEBUG_IOEXR
        LOG.log("Reading multipart exr with %d parts.", file.parts());
#endif
        const int numOfParts = file.parts();

        // Determine if the requestedView has conflicting channel names.
        bool viewHasChannelConflict = false;
        {
            set<string> channelNamesInView;
            for (int p = 0; p < numOfParts; ++p)
            {
                const Imf::Header& header = file.header(p);
                const string& view = (header.hasView() ? header.view() : "");
                if (view != requestedView)
                    continue;

                // Check for conflict in part p's channel list.
                const Imf::ChannelList& cl = header.channels();
                for (Imf::ChannelList::ConstIterator ci = cl.begin();
                     ci != cl.end(); ++ci)
                {
                    if (channelNamesInView.find(ci.name())
                        != channelNamesInView.end())
                    {
                        // Found a channel conflict in 'view'.
                        viewHasChannelConflict = true;
#ifdef DEBUG_IOEXR
                        LOG.log("***View %s has conflicting channels",
                                view.c_str());
#endif
                        p = numOfParts;
                        break;
                    }
                    channelNamesInView.insert(ci.name());
                }
            }
        }

        // The variable requestedMPChannelList will be the list of channel
        // to be read from the exr multipart file based on the
        // requestedView/Layer/Channel specification.
        vector<MultiPartChannel> requestedMPChannelList;
        bool stripAlpha = m_stripAlpha;

        for (int p = 0; p < numOfParts; ++p)
        {
            const Imf::Header& header = file.header(p);
            const string& view = (header.hasView() ? header.view() : "");

            if (!requestedView.empty() && view != requestedView)
            {
                // We do not care about channel within this view and it
                // is not the requestedView.
                continue;
            }

            const Imf::ChannelList& cl = header.channels();
            const string& partName = (header.hasName() ? header.name() : "");
            if (!requestedLayer.empty())
            {
                // Layer specified
                if (!requestedChannel.empty())
                {
                    // Layer and Channel specified
                    for (Imf::ChannelList::ConstIterator ci = cl.begin();
                         ci != cl.end(); ++ci)
                    {
                        string ch = ci.name();

                        if (doesRequestedChannelExists(
                                viewHasChannelConflict, requestedView,
                                requestedLayer, requestedChannel, partName, ch))
                        {
                            addToMultiPartChannelList(requestedMPChannelList, p,
                                                      partName, ch,
                                                      ci.channel());

                            //
                            //  Don't strip alpha if specifically requested
                            //
                            if (ch == "A" || ch == "a")
                                stripAlpha = false;

                            p = numOfParts; // exit the search as we hv already
                                            // found the channel
                            break;
                        }
                    }
                }
                else
                {
                    // Layer only specified
                    for (Imf::ChannelList::ConstIterator ci = cl.begin();
                         ci != cl.end(); ++ci)
                    {
                        string ch = ci.name();
                        if (doesRequestedLayerExists(
                                viewHasChannelConflict, requestedView,
                                requestedLayer, partName, ch))
                        {
                            addToMultiPartChannelList(requestedMPChannelList, p,
                                                      partName, ch,
                                                      ci.channel());
                        }
                    }
                }
            }
            else
            {
                // No layer specified.
                if (requestedChannel.empty())
                {
                    // No channel specified
                    for (Imf::ChannelList::ConstIterator ci = cl.begin();
                         ci != cl.end(); ++ci)
                    {
                        addToMultiPartChannelList(requestedMPChannelList, p,
                                                  partName, ci.name(),
                                                  ci.channel());
                    }
                }
                else
                {
                    // Channel specified in view without layers
                    // Also implies viewHasChannelConflict=false; since
                    // if viewHasChannelConflict=true there must be layers.
                    for (Imf::ChannelList::ConstIterator ci = cl.begin();
                         ci != cl.end(); ++ci)
                    {
                        string ch = ci.name();
                        if (requestedChannel == ch)
                        {
                            addToMultiPartChannelList(requestedMPChannelList, p,
                                                      partName, ch,
                                                      ci.channel());

                            //
                            //  Don't strip alpha if specifically requested
                            //
                            if (ch == "A" || ch == "a")
                                stripAlpha = false;

                            p = numOfParts; // exit the search as we hv already
                                            // found the channel
                            break;
                        }
                    } // if (requestedChannel.empty())
                } // if (!requestedChannel.empty())
            } // if (!requestedLayer.empty())
        } // end of parts loop

        if (requestedMPChannelList.empty())
        {
            // NB: This really should not happen but could if an
            // invalid requestedView/Layer/Channel selection was passed in.
            cerr << "ERROR: EXR: invalid "
                 << "view=\"" << requestedView << "\","
                 << "layer=\"" << requestedLayer << "\","
                 << "channel=\"" << requestedChannel << "\" "
                 << "request made on image \"" << filename << "\":" << endl;
            return;
        }

        //
        //  We now have a list of all the channels in the file so we now  decide
        //  which ones populate the display framebuffer with their data.
        //
        fbs.push_back(new FrameBuffer());

        readMultiPartChannelList(
            filename, requestedView, *fbs.back(), file, requestedMPChannelList,
            m_convertYRYBY, m_planar3channel, requestedAllChannels,
            m_inheritChannels, m_noOneChannelPlanes, stripAlpha,
            m_readWindowIsDisplayWindow, m_readWindow);

        if (!requestedChannel.empty())
        {
            fbs.back()->attribute<string>("Channel") = requestedChannel;
        }

        if (!requestedLayer.empty())
        {
            fbs.back()->attribute<string>("Layer") = requestedLayer;
        }

        fbs.back()->attribute<string>("View") = requestedView;
    }

    bool IOexr::stripViewFromName(string& name, const string& view)
    {
        bool didStrip = false;
        if (!view.empty() && !name.empty())
        {
            size_t pos = 0;
            string viewStr = "." + view;

            while ((pos = name.find(viewStr, pos)) != std::string::npos)
            {
                name.erase(pos, viewStr.length());
                didStrip = true;
                pos++;
            }
            pos = 0;
            viewStr = view + ".";
            while ((pos = name.find(viewStr, pos)) != std::string::npos)
            {
                name.erase(pos, viewStr.length());
                didStrip = true;
                pos++;
            }
        }
        return didStrip;
    }

    string IOexr::viewStrippedPartQualifiedName(
        bool doStrip, const Imf::MultiPartInputFile& file, int partnum,
        const string& view, const string& name)
    {
        string result = (file.header(partnum).hasName()
                             ? file.header(partnum).name() + "." + name
                             : name);

        if (doStrip)
            stripViewFromName(result, view);

        return result;
    }

    void IOexr::getMultiPartImageInfo(const Imf::MultiPartInputFile& file,
                                      FBInfo& fbi) const
    {
#ifdef DEBUG_IOEXR
        LOG.log("***Entering: getMultiPartImageInfo");
#endif
        const int partNum = 0;
        const string viewLessPartName = "";
        Imf::ChannelList clEmptyList;

        Imath::Box2i dataWin = file.header(partNum).dataWindow();
        Imath::Box2i dispWin = file.header(partNum).displayWindow();

        fbi.width = dataWin.max.x - dataWin.min.x + 1;
        fbi.height = dataWin.max.y - dataWin.min.y + 1;
        fbi.uncropWidth = dispWin.max.x - dispWin.min.x + 1;
        fbi.uncropHeight = dispWin.max.y - dispWin.min.y + 1;
        fbi.uncropX = dataWin.min.x - dispWin.min.x;
        fbi.uncropY = dataWin.min.y - dispWin.min.y;
        fbi.pixelAspect = file.header(partNum).pixelAspectRatio();
        fbi.orientation = FrameBuffer::TOPLEFT; // we read them like this

        set<Imf::PixelType> exrPixelType;

        fbi.proxy.setPrimaryColorSpace(ColorSpace::Rec709());
        fbi.proxy.setTransferFunction(ColorSpace::Linear());
        fbi.proxy.setPixelAspectRatio(fbi.pixelAspect);

        const int numOfParts = file.parts();
        const bool isMultiPart = (numOfParts > 1 ? true : false);
#ifdef DEBUG_IOEXR
        LOG.log("***numOfParts = %d", numOfParts);
#endif

        // Determine the total number of channels.
        // Determine which view's have channel conflicts
        fbi.numChannels = 0;
        set<string> views;
        map<string, bool> viewChannelConflictTable;
        map<string, Imf::ChannelList> viewChannelListTable;
        set<FBInfo::ChannelInfo> baseChannelInfos;
        for (int p = 0; p < numOfParts; ++p)
        {
            const Imf::Header& header = file.header(p);
            // Get part channelList.
            const Imf::ChannelList& cl = header.channels();
            fbi.numChannels += channelListSize(cl);
#ifdef DEBUG_IOEXR
            LOG.log("***fbi.numChannels for part %d = %d", p,
                    channelListSize(cl));
#endif

            // Determine the view of that part.
            const string& view =
                (header.hasView() ? header.view() : viewLessPartName);
            views.insert(view);
            if (viewChannelConflictTable.count(view) == 0)
            {
                viewChannelConflictTable[view] = false;
            }

            for (Imf::ChannelList::ConstIterator ci = cl.begin();
                 ci != cl.end(); ++ci)
            {
                exrPixelType.insert(ci.channel().type);
                FBInfo::ChannelInfo cinfo;
                setChannelInfo(baseChannelName(ci.name()), ci.channel().type,
                               cinfo);
                fbi.channelInfos.push_back(
                    cinfo); // ***CBB: This should really be a set.
#ifdef DEBUG_IOEXR
                LOG.log("***fbi.ChannelInfo.name = %s",
                        fbi.channelInfos.back().name.c_str());
#endif
                if (!viewChannelConflictTable[view]
                    && viewChannelListTable[view].findChannel(ci.name()) != 0)
                {
                    // Found a channel conflict in 'view'.
                    viewChannelConflictTable[view] = true;
                }
                viewChannelListTable[view].insert(ci.name(), ci.channel());
            }
        }

        //
        // Initialize view's part named channel table if view has a conflict
        //
        map<string, Imf::ChannelList> viewPartNamedChannelListTable;
        for (set<string>::const_iterator v = views.begin(); v != views.end();
             ++v)
        {
            const string& view = *v;
            if (!viewChannelConflictTable[view])
                continue;
#ifdef DEBUG_IOEXR
            LOG.log("***View %s has conflicting channels", view.c_str());
#endif

            // If there is a part name conflict within
            // a view's parts if we strip the view name
            // from the part name, then doStrip is false;
            // NB: We only need to do this if view != "".
            bool doViewStrip = true;
            if (!view.empty())
            {
                for (int p = 0; p < numOfParts; ++p)
                {
                    const Imf::Header& header = file.header(p);
                    const string& partView =
                        (header.hasView() ? header.view() : viewLessPartName);
                    if (partView != view)
                        continue;

                    // Determine if there is a part name conflict if we do view
                    // stripping of part names.
                    string partName =
                        (file.header(p).hasName() ? file.header(p).name() : "");
                    if (!stripViewFromName(partName, view))
                    {
                        doViewStrip = false;
#ifdef DEBUG_IOEXR
                        LOG.log(
                            "***View %s has conflicting view stripped parts",
                            view.c_str());
#endif
                        break;
                    }
                }
            }

            for (int p = 0; p < numOfParts; ++p)
            {
                const Imf::Header& header = file.header(p);
                const string& partView =
                    (header.hasView() ? header.view() : viewLessPartName);
                if (partView != view)
                    continue;

                // Get part channelList.
                const Imf::ChannelList& cl = header.channels();
                for (Imf::ChannelList::ConstIterator ci = cl.begin();
                     ci != cl.end(); ++ci)
                {
                    string partNamedChannel = viewStrippedPartQualifiedName(
                        doViewStrip, file, p, view, ci.name());

                    viewPartNamedChannelListTable[view].insert(partNamedChannel,
                                                               ci.channel());
                }
            }
        }

        //
        //  Get or derived RV layers from multipart info that was
        //  read in earlier.
        //
        fbi.viewInfos.resize(views.size());
        int vindex = 0;
        for (set<string>::const_iterator v = views.begin(); v != views.end();
             ++v, ++vindex)
        {
            const string& view = *v;
            if (!view.empty())
            {
                fbi.views.push_back(view);
#ifdef DEBUG_IOEXR
                LOG.log("***fbi.views = %s", fbi.views.back().c_str());
#endif
            }
            FBInfo::ViewInfo& vinfo = fbi.viewInfos[vindex];
            vinfo.name = view;
#ifdef DEBUG_IOEXR
            LOG.log("***fbi.ViewInfo.name = %s", view.c_str());
#endif

            const Imf::ChannelList& vcl =
                (viewChannelConflictTable[view]
                     ? viewPartNamedChannelListTable[view]
                     : viewChannelListTable[view]);

#ifdef DEBUG_IOEXR
            for (Imf::ChannelList::ConstIterator ci = vcl.begin();
                 ci != vcl.end(); ++ci)
            {
                if (viewChannelConflictTable[view])
                {
                    LOG.log("***view=%s  partnamedchannel=%s", view.c_str(),
                            ci.name());
                }
                else
                {
                    LOG.log("***view=%s  channel=%s", view.c_str(), ci.name());
                }
            }
#endif
            // Determine what the layers are per view.
            set<string> exrLayers;
            vcl.layers(exrLayers);
            exrLayers.insert(""); // Add a default layer
            for (set<string>::const_iterator li = exrLayers.begin();
                 li != exrLayers.end(); ++li)
            {
                fbi.layers.push_back(*li);
#ifdef DEBUG_IOEXR
                LOG.log("***view=%s  layer=%s", view.c_str(), li->c_str());
                LOG.log("***fbi.layers = %s", fbi.layers.back().c_str());
#endif
                Imf::ChannelList::ConstIterator cb;
                Imf::ChannelList::ConstIterator ce;
                if (li->empty())
                {
                    cb = vcl.begin();
                    ce = vcl.end();
                }
                else
                {
                    vcl.channelsInLayer(*li, cb, ce);
                }

                Imf::ChannelList cl;
                for (Imf::ChannelList::ConstIterator ci = cb; ci != ce; ++ci)
                {
                    string baseName, layerName;
                    channelSplit(ci.name(), baseName, layerName);
                    if ((*li) == layerName)
                    {
                        string baseName = baseChannelName(ci.name());
                        cl.insert(baseName.c_str(), ci.channel());
#ifdef DEBUG_IOEXR
                        LOG.log("***view=%s  layer=%s  channel=%s",
                                view.c_str(), li->c_str(), baseName.c_str());
#endif
                    }
                }

                if (channelListSize(cl) > 0)
                {
                    FBInfo::LayerInfo linfo;
                    // If a view has layer names
                    linfo.name = (*li);
                    vinfo.layers.push_back(linfo);
#ifdef DEBUG_IOEXR
                    LOG.log("***fbi.ViewInfo.LayerInfo.name = %s",
                            vinfo.layers.back().name.c_str());
#endif
                    vinfo.layers.back().channels.resize(channelListSize(cl));
                    int cindex = 0;
                    for (Imf::ChannelList::ConstIterator ci = cl.begin();
                         ci != cl.end(); ++ci, ++cindex)
                    {
                        FBInfo::ChannelInfo& cinfo =
                            vinfo.layers.back().channels[cindex];
                        setChannelInfo(ci, cinfo);
#ifdef DEBUG_IOEXR
                        LOG.log(
                            "***fbi.ViewInfo.LayerInfo.ChannelInfo.name = %s",
                            cinfo.name.c_str());
#endif
                    }
                }
            }
        }

        //
        //  Set the default view.
        //  Check if there is an exrAttribute first, if there isnt then
        //  use the "view" of part zero.
        //
        {
            const Imf::Header& header = file.header(0);
            if (const Imf::StringAttribute* sAttr =
                    header.findTypedAttribute<Imf::StringAttribute>(
                        "defaultView"))
            {
                fbi.defaultView = sAttr->value();
            }
            else
            {
                fbi.defaultView =
                    (header.hasView() ? header.view() : viewLessPartName);
            }
#ifdef DEBUG_IOEXR
            LOG.log("***fbi.defaultView = %s", fbi.defaultView.c_str());
#endif
        }

        //
        //  Just float and half for EXR
        //
        for (set<Imf::PixelType>::const_iterator it = exrPixelType.begin();
             it != exrPixelType.end(); ++it)
        {
            switch (*it)
            {
            case Imf::FLOAT:
                fbi.dataType = FrameBuffer::FLOAT;
                break;
            case Imf::HALF:
                fbi.dataType = FrameBuffer::HALF;
                break;
            case Imf::UINT:
                fbi.dataType = FrameBuffer::UINT;
                break;
            default:
                TWK_EXC_THROW_WHAT(Exception, "Unsupported exr data type");
            }

            // We have the worst case buffer size so dont bother checking
            // anymore.
            if (fbi.dataType == FrameBuffer::FLOAT)
            {
                break;
            }
        }

        //
        //  Put the attributes onto the proxy image
        //
        readAllAttributes(file, fbi.proxy);

#ifdef DEBUG_IOEXR
        LOG.log("***Leaving: getMultiPartImageInfo");
#endif
    }

    void IOexr::writeImagesToMultiPartFile(const ConstFrameBufferVector& fbs,
                                           const std::string& filename,
                                           const WriteRequest& request) const
    {
        const int numOfParts = fbs.size();
        Imf::LineOrder order;
        ConstFrameBufferVector outfbs(numOfParts);
        vector<Imf::PixelType> pixelTypes(numOfParts);

        bool useAttrs = false;
        RegEx attrRE;
        RegEx userRE("([^:]+):(f|i|v2i|v2f|v3i|v3f|c|s|b2i|b2f|sv|m33f|m44f)");
        vector<string> userNames;
        vector<Imf::Attribute*> userAttrs;

        bool acesFile = filename.size() > 5
                        && filename.find(".aces") == filename.size() - 5;

        bool usePrimaries = false;
        Imf::Chromaticities ch;
        Imath::V2f adoptedNeutral;

        if (acesFile)
        {
            ch = acesChromaticities();
            usePrimaries = true;
            adoptedNeutral = ch.white;
        }

        for (size_t i = 0; i < request.parameters.size(); i++)
        {
            const StringPair pair = request.parameters[i];

            string name = pair.first;
            string value = pair.second;

            if (name == "passthrough")
            {
                useAttrs = true;
                attrRE = RegEx(value);
            }
            else if (name == "output/pa")
            {
                double pa = atof(value.c_str());
                userNames.push_back("pixelAspectRatio");
                userAttrs.push_back(
                    new Imf::FloatAttribute(atof(value.c_str())));
            }
            else if (name == "output/ACES")
            {
                //
                //  Special cookie for ACES output
                //

                acesFile = true;
                ch = acesChromaticities();
                usePrimaries = true;
            }
            else if (name == "output/transfer")
            {
                if (value != ColorSpace::Linear())
                {
                    userNames.push_back("transferFunction");
                    userAttrs.push_back(new Imf::StringAttribute(value));
                }
            }
            else if (name == "output/gamma")
            {
                userNames.push_back("transferFunction");
                userAttrs.push_back(
                    new Imf::StringAttribute(ColorSpace::Gamma()));
                userNames.push_back("gamma");
                userAttrs.push_back(
                    new Imf::FloatAttribute(atof(value.c_str())));
            }
            else if (name == "output/chromaticities")
            {
                usePrimaries = true;

                vector<string> parts;
                stl_ext::tokenize(parts, value, ",");
                while (parts.size() < 8)
                    parts.push_back(parts.back());
                ch.red[0] = atof(parts[0].c_str());
                ch.red[1] = atof(parts[1].c_str());
                ch.green[0] = atof(parts[2].c_str());
                ch.green[1] = atof(parts[3].c_str());
                ch.blue[0] = atof(parts[4].c_str());
                ch.blue[1] = atof(parts[5].c_str());
                ch.white[0] = atof(parts[6].c_str());
                ch.white[1] = atof(parts[7].c_str());
            }
            else if (name == "output/neutral")
            {
                vector<string> parts;
                stl_ext::tokenize(parts, value, ",");
                while (parts.size() < 2)
                    parts.push_back(parts.back());
                adoptedNeutral[0] = atof(parts[0].c_str());
                adoptedNeutral[1] = atof(parts[1].c_str());
            }

            Match m(userRE, name);

            if (m)
            {
                string t = m.subStr(1);

                if (t == "s")
                {
                    userNames.push_back(m.subStr(0));
                    userAttrs.push_back(new Imf::StringAttribute(value));
                }
                else if (t == "sv")
                {
                    Imf::StringVector v;
                    vector<string> parts;
                    stl_ext::tokenize(parts, value, ",");
                    for (size_t q = 0; q < parts.size(); q++)
                        v.push_back(parts[q]);

                    userNames.push_back(m.subStr(0));
                    userAttrs.push_back(new Imf::StringVectorAttribute(v));
                }
                else if (t == "f")
                {
                    userNames.push_back(m.subStr(0));
                    userAttrs.push_back(
                        new Imf::FloatAttribute(atof(value.c_str())));
                }
                else if (t == "i")
                {
                    userNames.push_back(m.subStr(0));
                    userAttrs.push_back(
                        new Imf::IntAttribute(atoi(value.c_str())));
                }
                else if (t == "v2i")
                {
                    Imath::V2i v;
                    vector<string> parts;
                    stl_ext::tokenize(parts, value, ",");
                    if (parts.size() == 1)
                        parts.push_back(parts[0]);
                    v[0] = atoi(parts[0].c_str());
                    v[1] = atoi(parts[1].c_str());

                    userNames.push_back(m.subStr(0));
                    userAttrs.push_back(new Imf::V2iAttribute(v));
                }
                else if (t == "v2f")
                {
                    Imath::V2f v;
                    vector<string> parts;
                    stl_ext::tokenize(parts, value, ",");
                    if (parts.size() == 1)
                        parts.push_back(parts[0]);
                    v[0] = atof(parts[0].c_str());
                    v[1] = atof(parts[1].c_str());

                    userNames.push_back(m.subStr(0));
                    userAttrs.push_back(new Imf::V2fAttribute(v));
                }
                else if (t == "v3i")
                {
                    Imath::V3i v;
                    vector<string> parts;
                    stl_ext::tokenize(parts, value, ",");
                    if (parts.size() == 1)
                        parts.push_back(parts[0]);
                    if (parts.size() == 2)
                        parts.push_back(parts[1]);
                    v[0] = atoi(parts[0].c_str());
                    v[1] = atoi(parts[1].c_str());
                    v[2] = atoi(parts[2].c_str());

                    userNames.push_back(m.subStr(0));
                    userAttrs.push_back(new Imf::V3iAttribute(v));
                }
                else if (t == "v3f")
                {
                    Imath::V3f v;
                    vector<string> parts;
                    stl_ext::tokenize(parts, value, ",");
                    if (parts.size() == 1)
                        parts.push_back(parts[0]);
                    if (parts.size() == 2)
                        parts.push_back(parts[1]);
                    v[0] = atof(parts[0].c_str());
                    v[1] = atof(parts[1].c_str());
                    v[2] = atof(parts[2].c_str());

                    userNames.push_back(m.subStr(0));
                    userAttrs.push_back(new Imf::V3fAttribute(v));
                }
                else if (t == "b2i")
                {
                    Imath::Box2i b;
                    vector<string> parts;
                    stl_ext::tokenize(parts, value, ",");
                    while (parts.size() < 4)
                        parts.push_back(parts.back());
                    b.min[0] = atoi(parts[0].c_str());
                    b.min[1] = atoi(parts[1].c_str());
                    b.max[0] = atoi(parts[2].c_str());
                    b.max[1] = atoi(parts[3].c_str());

                    userNames.push_back(m.subStr(0));
                    userAttrs.push_back(new Imf::Box2iAttribute(b));
                }
                else if (t == "b2f")
                {
                    Imath::Box2f b;
                    vector<string> parts;
                    stl_ext::tokenize(parts, value, ",");
                    while (parts.size() < 4)
                        parts.push_back(parts.back());
                    b.min[0] = atof(parts[0].c_str());
                    b.min[1] = atof(parts[1].c_str());
                    b.max[0] = atof(parts[2].c_str());
                    b.max[1] = atof(parts[3].c_str());

                    userNames.push_back(m.subStr(0));
                    userAttrs.push_back(new Imf::Box2fAttribute(b));
                }
                else if (t == "c")
                {
                    Imf::Chromaticities chr;
                    vector<string> parts;
                    stl_ext::tokenize(parts, value, ",");
                    while (parts.size() < 8)
                        parts.push_back(parts.back());
                    chr.red[0] = atof(parts[0].c_str());
                    chr.red[1] = atof(parts[1].c_str());
                    chr.green[0] = atof(parts[2].c_str());
                    chr.green[1] = atof(parts[3].c_str());
                    chr.blue[0] = atof(parts[4].c_str());
                    chr.blue[1] = atof(parts[5].c_str());
                    chr.white[0] = atof(parts[6].c_str());
                    chr.white[1] = atof(parts[7].c_str());

                    userNames.push_back(m.subStr(0));
                    userAttrs.push_back(new Imf::ChromaticitiesAttribute(chr));
                }
                else if (t == "m33f")
                {
                    Imath::M33f mx;
                    vector<string> parts;
                    stl_ext::tokenize(parts, value, ",");
                    while (parts.size() < 9)
                        parts.push_back(parts.back());
                    mx[0][0] = atof(parts[0].c_str());
                    mx[0][1] = atof(parts[1].c_str());
                    mx[0][2] = atof(parts[2].c_str());
                    mx[1][0] = atof(parts[3].c_str());
                    mx[1][1] = atof(parts[4].c_str());
                    mx[1][2] = atof(parts[5].c_str());
                    mx[2][0] = atof(parts[6].c_str());
                    mx[2][1] = atof(parts[7].c_str());
                    mx[2][2] = atof(parts[8].c_str());

                    userNames.push_back(m.subStr(0));
                    userAttrs.push_back(new Imf::M33fAttribute(mx));
                }
                else if (t == "m44f")
                {
                    Imath::M44f mx;
                    vector<string> parts;
                    stl_ext::tokenize(parts, value, ",");
                    while (parts.size() < 16)
                        parts.push_back(parts.back());
                    mx[0][0] = atof(parts[0].c_str());
                    mx[0][1] = atof(parts[1].c_str());
                    mx[0][2] = atof(parts[2].c_str());
                    mx[0][3] = atof(parts[3].c_str());
                    mx[1][0] = atof(parts[4].c_str());
                    mx[1][1] = atof(parts[5].c_str());
                    mx[1][2] = atof(parts[6].c_str());
                    mx[1][3] = atof(parts[7].c_str());
                    mx[2][0] = atof(parts[8].c_str());
                    mx[2][1] = atof(parts[9].c_str());
                    mx[2][2] = atof(parts[10].c_str());
                    mx[2][3] = atof(parts[11].c_str());
                    mx[3][0] = atof(parts[12].c_str());
                    mx[3][1] = atof(parts[13].c_str());
                    mx[3][2] = atof(parts[14].c_str());
                    mx[3][3] = atof(parts[15].c_str());

                    userNames.push_back(m.subStr(0));
                    userAttrs.push_back(new Imf::M44fAttribute(mx));
                }
            }
        }

        if (usePrimaries)
        {
            userNames.push_back("chromaticities");
            userAttrs.push_back(new Imf::ChromaticitiesAttribute(ch));
            userNames.push_back("adoptedNeutral");
            userAttrs.push_back(new Imf::V2fAttribute(adoptedNeutral));
        }

        //
        //   SMPTE spec (2065-4) requires this:
        //

        if (acesFile)
        {
            userNames.push_back("acesImageContainerFlag");
            userAttrs.push_back(new Imf::IntAttribute(1));
        }

        //
        //  Get the fbs into the correct datatype, orientation, etc
        //

        for (int p = 0; p < numOfParts; ++p)
        {
            const FrameBuffer* outfb = fbs[p];

            //
            //  Convert to FLOAT or HALF if USHORT or UCHAR repsectively
            //

            switch (outfb->dataType())
            {
            case FrameBuffer::FLOAT:
            {
                const FrameBuffer* fb = outfb;
                if (acesFile)
                {
                    outfb = copyConvert(outfb, FrameBuffer::HALF);
                    pixelTypes[p] = Imf::HALF;
                }
                else
                {
                    pixelTypes[p] = Imf::FLOAT;
                }
                if (fb != outfb && fb != fbs[p])
                    delete fb;
                break;
            }
            case FrameBuffer::HALF:
                pixelTypes[p] = Imf::HALF;
                break;
            case FrameBuffer::UCHAR:
            {
                const FrameBuffer* fb = outfb;
                outfb = copyConvert(outfb, FrameBuffer::HALF);
                if (fb != outfb && fb != fbs[p])
                    delete fb;
                pixelTypes[p] = Imf::HALF;
                break;
            }
            case FrameBuffer::USHORT:
            {
                const FrameBuffer* fb = outfb;
                if (acesFile)
                {
                    outfb = copyConvert(outfb, FrameBuffer::HALF);
                    pixelTypes[p] = Imf::HALF;
                }
                else
                {
                    outfb = copyConvert(outfb, FrameBuffer::FLOAT);
                    pixelTypes[p] = Imf::FLOAT;
                }
                if (fb != outfb && fb != fbs[p])
                    delete fb;
                break;
            }
            case FrameBuffer::PACKED_R10_G10_B10_X2:
            case FrameBuffer::PACKED_X2_B10_G10_R10:
            case FrameBuffer::PACKED_Y8_Cb8_Y8_Cr8:
            case FrameBuffer::PACKED_Cb8_Y8_Cr8_Y8:
            {
                const FrameBuffer* fb = outfb;
                outfb = convertToLinearRGB709(outfb);
                if (fb != outfb && fb != fbs[p])
                    delete fb;
                fb = outfb;
                outfb = copyConvert(outfb, FrameBuffer::HALF);
                if (fb != outfb && fb != fbs[p])
                    delete fb;
                pixelTypes[p] = Imf::HALF;
                break;
            }
            default:
                abort();
                break;
            }

            //
            //  If its planar and the request is to convert to packed or for
            //  the "common" format, then make the image packed.
            //

            if (outfb->isPlanar()
                && (!request.keepPlanar || request.preferCommonFormat))
            {
                const FrameBuffer* fb = outfb;
                outfb = mergePlanes(outfb);
                if (fb != outfb && fb != fbs[p])
                    delete fb;
            }

            //
            //  Convert YUV, YRYBY, or non-RGB 709 images to RGB 709 if the
            //  "common" format is requested.
            //

            if (request.preferCommonFormat && !acesFile
                && (outfb->hasPrimaries() || outfb->isYUV()
                    || outfb->isYRYBY()))
            {
                const FrameBuffer* fb = outfb;

                cout << "INFO: IOexr: converting to REC 709 RGB because "
                     << (outfb->hasPrimaries() ? " image has chromaticities "
                                               : "")
                     << (outfb->isYRYBY() ? " image is Y RY BY" : "") << endl;

                outfb = convertToLinearRGB709(outfb);
                if (fb != outfb && fb != fbs[p])
                    delete fb;
            }

            //
            //  Get the orientation. Exr doesn't do flopped images, so we'll
            //  flop it ourselves if necessary.
            //
            //  Actually, we always have to tell EXR about our slices from top
            //  to bottom, so this only effects how the writing is done.  We
            //  could just as easily always do INCREASING_Y.
            //

            order = Imf::INCREASING_Y;
            bool needsFlip = false;
            bool needsFlop = false;

            switch (outfb->orientation())
            {
            case FrameBuffer::TOPLEFT:
                needsFlip = true;
                break;
            case FrameBuffer::TOPRIGHT:
                needsFlip = true;
                needsFlop = true;
                break;
            case FrameBuffer::BOTTOMRIGHT:
                needsFlop = true;
                break;
            default:
                break;
            }

            if (needsFlip || needsFlop)
            {
                const FrameBuffer* fb = outfb;
                if (outfb == fbs[p])
                    outfb = outfb->copy();
                if (needsFlip)
                    flip(const_cast<FrameBuffer*>(outfb));
                if (needsFlop)
                    flop(const_cast<FrameBuffer*>(outfb));
                if (fb != outfb && fb != fbs[p])
                    delete fb;
            }

            outfbs[p] = outfb;
        }

        Imf::Compression compression = Imf::PIZ_COMPRESSION;
        if (request.compression == "ZIP")
            compression = Imf::ZIP_COMPRESSION;
        else if (request.compression == "ZIPS")
            compression = Imf::ZIPS_COMPRESSION;
        else if (request.compression == "PXR24")
            compression = Imf::PXR24_COMPRESSION;
        else if (request.compression == "RLE")
            compression = Imf::RLE_COMPRESSION;
        else if (request.compression == "B44")
            compression = Imf::B44_COMPRESSION;
        else if (request.compression == "B44A")
            compression = Imf::B44A_COMPRESSION;
        else if (request.compression == "DWAA")
            compression = Imf::DWAA_COMPRESSION;
        else if (request.compression == "DWAB")
            compression = Imf::DWAB_COMPRESSION;
        else if (request.compression == "NONE")
            compression = Imf::NO_COMPRESSION;
        else if (request.compression == "PIZ")
            compression = Imf::PIZ_COMPRESSION;
        else if (request.compression != "")
        {
            cerr << "WARNING: IOexr: unknown compression type "
                 << request.compression << ", using PIZ instead" << endl;
        }

        if (acesFile)
        {
            if (compression != Imf::PIZ_COMPRESSION
                && compression != Imf::B44A_COMPRESSION
                && compression != Imf::NO_COMPRESSION)
            {
                if (compression == Imf::B44_COMPRESSION)
                {
                    cerr << " WARNING:: IOexr: using B44A compression instead "
                            "of "
                         << request.compression << " for ACES output" << endl;
                    compression = Imf::B44A_COMPRESSION;
                }
                else
                {
                    cerr << "WARNING: IOexr: using PIZ compression instead of "
                         << request.compression << " for ACES output" << endl;
                    compression = Imf::PIZ_COMPRESSION;
                }
            }

            for (size_t i = 0; i < userNames.size(); i++)
            {
                Imf::ChromaticitiesAttribute* a = 0;

                if (userNames[i] == "chromaticities"
                    && (a = dynamic_cast<Imf::ChromaticitiesAttribute*>(
                            userAttrs[i])))
                {
                    if (!isAces(a->value()))
                    {
                        TWK_THROW_STREAM(
                            Exception,
                            "ERROR: EXR: chromaticities are not ACES");
                    }
                }
            }
        }

        //
        // See if rv's outfb have names that can be used for views and parts.
        //
        vector<string> partNames;
        vector<string> partViews(numOfParts);
        vector<string> partTypes(numOfParts);

        for (int p = 0; p < numOfParts; ++p)
        {
            //
            //  Image attrs
            //
            const FrameBuffer::AttributeVector& attrs = outfbs[p]->attributes();

            for (size_t q = 0; q < attrs.size(); q++)
            {
                FBAttribute* a = attrs[q];
                string::size_type cp = a->name().find("/./");
                if (cp == string::npos)
                    continue;

                string name = a->name().substr(cp + 3, string::npos);

                if (name.find("EXR/") == 0)
                {
                    name = name.substr(4, string::npos);

                    if (name == "type")
                    {
                        partTypes[p] = a->valueAsString();
                        continue;
                    }

                    if (name == "view")
                    {
                        partViews[p] = a->valueAsString();
                        continue;
                    }

                    if (name == "name")
                    {
                        partNames.push_back(a->valueAsString());
                        continue;
                    }
                }
            }
        }

        if (numOfParts > 1)
        {
            if (partNames.size() != numOfParts)
            {
                partNames.clear();
                if (numOfParts == 2)
                {
                    partNames.push_back("left");
                    partNames.push_back("right");
                }
                else
                {
                    // Multipart names must be unique so
                    // lets name them "part0"... "partN-1".
                    for (int p = 0; p < numOfParts; ++p)
                    {
                        ostringstream nameStr;
                        nameStr << "part" << p;
                        partNames.push_back(nameStr.str());
                    }
                }
            }

            if ((numOfParts == 2) && (partViews[0] == "" || partViews[1] == ""))
            {
                partViews[0] = "left";
                partViews[1] = "right";
            }
        }

        //
        //  Initialize headers for multipart
        //
        vector<Imf::Header> headers;
        vector<Imf::FrameBuffer> output_frameBuffers(numOfParts);

        for (int p = 0; p < numOfParts; ++p)
        {
            const FrameBuffer* outfb = outfbs[p];
            Imath::Box2i window(
                Imath::V2i(0, 0),
                Imath::V2i(outfb->width() - 1, outfb->height() - 1));

            Imath::Box2i displayWindow, dataWindow;
            displayWindow = window;
            dataWindow = window;

            Imf::Header header(displayWindow, dataWindow,
                               1.0f,             // pixelAspectRatio = 1
                               Imath::V2f(0, 0), // screenWindowCenter
                               1.0f,             // screenWindowWidth
                               order,            // lineOrder
                               compression);

            string prefix("");
            // Handle exr layered naming here
            // if (fbs.size() == 2 && p == 1) prefix = "right.";

            if (!partNames.empty())
            {
                header.setName(partNames[p]);
            }

            if (!partViews[p].empty())
            {
                header.setView(partViews[p]);
            }

            // Set the type
            if (partTypes[p].empty())
            {
                header.setType(Imf::SCANLINEIMAGE);
            }
            else
            {
                header.setType(partTypes[p]);
            }

            if (userAttrs.size())
            {
                for (size_t i = 0; i < userAttrs.size(); i++)
                {
                    header.insert(userNames[i], *userAttrs[i]);
                }
            }

            //
            //  Image attrs
            //
            const FrameBuffer::AttributeVector& attrs = outfb->attributes();

            for (size_t q = 0; q < attrs.size(); q++)
            {
                FBAttribute* a = attrs[q];

                //
                //  NOTE: this is a bit weird. When rvio generates a
                //  framebuffer from the graph, it accumulates *all* of
                //  the attributes from *all* of the images. In order to
                //  keep track of where they came from the full path to
                //  the source+media is prepended to the name. So here
                //  we're just removing that part and using the simple
                //  attribute name directly.
                //

                string::size_type cp = a->name().find("/./");
                if (cp == string::npos)
                    continue;

                string name = a->name().substr(cp + 3, string::npos);

                if (name.find("IOexr/") == 0 || name.find("ColorSpace/") == 0
                    || name == "")
                {
                    continue;
                }

                if (attrRE.matches(name))
                {
                    if (name == "View" || name == "Sequence" || name == "Eye"
                        || name == "File" || name == "SourceFrame"
                        || name == "AlphaType" || name == "RVSource")
                    {
                        continue;
                    }

                    ostringstream partStr;
                    partStr << "EXR/" << p << "/";
                    if (name.find(partStr.str()) != 0)
                    {
                        partStr.clear();
                        partStr.str("");
                        partStr << "EXR/";
                        if (name.find(partStr.str()) != 0)
                        {
                            partStr.clear();
                            partStr.str("");
                        }
                    }

                    if (partStr.str() != "")
                    {
                        name = name.substr(partStr.str().size(), string::npos);

                        if (name == "compression" || name == "dataWindow"
                            || name == "displayWindow" || name == "lineOrder"
                            || name == "name" || name == "view"
                            || name == "type" || name == "screenWindowCenter"
                            || name == "screenWindowWidth"
                            || name == "pixelAspectRatio"
                            || name == "ChannelsRead"
                            || name == "ChannelNamesInFile"
                            || name == "ChannelsInFile"
                            || name == "ChannelSamplingInFile"
                            || name == "PartNumbersInFile"
                            || name == "PartNamesInFile" || name == "AlphaType")
                        {
                            continue;
                        }
                    }

                    if (FloatAttribute* ta = dynamic_cast<FloatAttribute*>(a))
                    {
                        Imf::TypedAttribute<float> attr(ta->value());
                        header.insert(name, attr);
                    }
                    else if (IntAttribute* ta = dynamic_cast<IntAttribute*>(a))
                    {
                        Imf::TypedAttribute<int> attr(ta->value());
                        header.insert(name, attr);
                    }
                    else
                    {
                        Imf::TypedAttribute<string> attr(a->valueAsString());
                        header.insert(name, attr);
                    }
                }
            }

            if ((compression == Imf::DWAA_COMPRESSION)
                || (compression == Imf::DWAB_COMPRESSION))
            {
                float dwaCompressionLevel =
                    45.0f; // Default value in ImfDwaCompressor().
                if (request.quality != 0.9f) // the rvio default
                {
                    dwaCompressionLevel = request.quality;
                }

                Imf::FloatAttribute attr(dwaCompressionLevel);
                header.insert("dwaCompressionLevel", attr);
            }

            if (outfb->isPlanar())
            {
                for (const FrameBuffer* fb = outfb; fb; fb = fb->nextPlane())
                {
                    int xsamp = outfb->width() / fb->width();
                    int ysamp = outfb->height() / fb->height();
                    string chname = prefix + fb->channelName(0);

                    header.channels().insert(
                        chname.c_str(),
                        Imf::Channel(pixelTypes[p], xsamp, ysamp,
                                     chname == "RY" || chname == "BY"));

                    output_frameBuffers[p].insert(
                        chname.c_str(), // name
                        Imf::Slice(
                            pixelTypes[p],                            // type
                            &fb->pixel<char>(0, fb->height() - 1, 0), // base
                            fb->pixelSize(),                          // xStride
                            -fb->scanlineSize(),                      // yStride
                            xsamp,   // xSamples
                            ysamp)); // ySamples
                }
            }
            else
            {
                for (int c = 0; c < outfb->numChannels(); ++c)
                {
                    string chname = prefix + outfb->channelName(c);

                    header.channels().insert(chname.c_str(), pixelTypes[p]);

                    output_frameBuffers[p].insert(
                        chname.c_str(),           // name
                        Imf::Slice(pixelTypes[p], // type
                                   &outfb->pixel<char>(0, outfb->height() - 1,
                                                       c),   // base
                                   outfb->pixelSize(),       // xStride
                                   -outfb->scanlineSize())); // yStride
                }
            }

            headers.push_back(header);
        }

        // Write the parts
        Imf::MultiPartOutputFile outfile(filename.c_str(), &headers[0],
                                         headers.size());

        for (int p = 0; p < numOfParts; ++p)
        {
            const FrameBuffer* outfb = outfbs[p];
            Imf::OutputPart outpart(outfile, p);
            outpart.setFrameBuffer(output_frameBuffers[p]);
            outpart.writePixels(outfbs[p]->height());
        }

        for (int i = 0; i < fbs.size(); i++)
        {
            if (outfbs[i] != fbs[i])
                delete outfbs[i];
        }

        for (size_t i = 0; i < userAttrs.size(); i++)
        {
            delete userAttrs[i];
        }
    }

} //  End namespace TwkFB
