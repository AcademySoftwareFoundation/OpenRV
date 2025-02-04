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

#include <IOexr/Logger.h>

namespace TwkFB
{
    using namespace std;
    using namespace TwkUtil;

    void IOexr::readMultiViewChannelList(
        const std::string& filename, const std::string& layer,
        const std::string& view, FrameBuffer& fb, Imf::MultiPartInputFile& file,
        int partNum, Imf::ChannelList& cl, bool useRGBAReader,
        bool convertYRYBY, bool planar3channel, bool allChannels,
        bool inheritChannels, bool noOneChannelPlanes, bool stripAlpha,
        bool readWindowIsDisplayWindow, IOexr::ReadWindow window)
    {
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
        int numChannels = 0;
        int totalChannels = 0;
        bool planarYRYBY = false;
        bool planar3 = false;
        vector<Imf::ChannelList::Iterator> planeChannels;

        FrameBuffer::DataType dataType;
        vector<string> channelNames;
        vector<string> sampleNames;
        Imf::PixelType exrPixelType;
        string originalChannels;
        ostringstream originalSampling;

        for (Imf::ChannelList::ConstIterator i = cl.begin(); i != cl.end();
             ++i, totalChannels++)
        {
            if (originalChannels.size())
            {
                originalChannels += ", ";
                originalSampling << ", ";
            }

            originalChannels += i.name();

            originalSampling << i.name() << ":";
            if (i.channel().xSampling == i.channel().ySampling)
            {
                originalSampling << i.channel().xSampling;
            }
            else
            {
                originalSampling << i.channel().xSampling << "x"
                                 << i.channel().ySampling;
            }

            exrPixelType = i.channel().type;

            if ((!strcmp(i.name(), "RY") || !strcmp(i.name(), "BY")))
            {
                if (convertYRYBY)
                {
                    useRGBAReader = true;
                }
                else
                {
                    planarYRYBY = true;
                }
            }

            channelNames.push_back(i.name());
        }

#ifdef DEBUG_IOEXR
        for (int c = 0; c < channelNames.size(); ++c)
        {
            LOG.log("Reading full channel name: %s", channelNames[c].c_str());
        }
#endif

        //
        //  Sort the channels so that R G B A are first in the list and
        //  the rest of the channels appear in their natural order
        //

        stable_sort(channelNames.begin(), channelNames.end(), ChannelComp);

        //
        //  If no layer or view is specified, nuke any layers
        //

        if (!allChannels && layer == "" && view == "")
        {
            vector<string> channelNames0 = channelNames;

            vector<string>::iterator i =
                remove_if(channelNames.begin(), channelNames.end(),
                          LayerOrNamedViewChannel);

            //
            //  Only alpha -- find some R G B for us
            //

            if (i != channelNames.end() && i != channelNames.begin())
            {
                //
                //  If i == channelNames.begin() that means ALL of the
                //  channels have layer or view names. So we need to
                //  include them.
                //

                channelNames.erase(i, channelNames.end());
            }

            if (inheritChannels)
            {
                int a = alphaIndex(channelNames);

                if (channelNames0.size() > 1 && channelNames.size() == 1
                    && a != -1)
                {
                    int r = redIndex(channelNames0);
                    int g = greenIndex(channelNames0);
                    int b = blueIndex(channelNames0);

                    if (r != -1 && g != -1 && b != -1)
                    {
                        vector<string> newChannels;
                        newChannels.push_back(channelNames0[r]);
                        newChannels.push_back(channelNames0[g]);
                        newChannels.push_back(channelNames0[b]);
                        newChannels.push_back(channelNames.front());
                        channelNames = newChannels;
                    }
                }
            }

#ifdef DEBUG_IOEXR
            for (int c = 0; c < channelNames.size(); ++c)
            {
                LOG.log("Reading updated full channel name: %s",
                        channelNames[c].c_str());
            }
#endif
        }

        if (!planarYRYBY && noOneChannelPlanes && channelNames.size() == 3)
        {
            useRGBAReader = true;
        }

        if (useRGBAReader)
        {
            //
            //  If the image contains subsampled values, let's just let
            //  the EXR library deal with it.
            //

            channelNames.clear();
            channelNames.push_back("R");
            channelNames.push_back("G");
            channelNames.push_back("B");
            if (!stripAlpha)
                channelNames.push_back("A");
        }

        //
        //  If we only have three channels and they are known r, g, b channels
        //  and there's a default alpha use it.
        //

        if (inheritChannels && channelNames.size() == 3)
        {
            if (redIndex(channelNames) != -1 && greenIndex(channelNames) != -1
                && blueIndex(channelNames) != -1 && !stripAlpha)
            {
                //
                //  See if there's a default alpha channel
                //

                string alpha = findAnAlpha(file, partNum, layer, view);
                if (alpha != "")
                    channelNames.push_back(alpha);
            }
        }

        //
        //  Unless we're reading all of the channels, nuke all but the first
        //  four channels.
        //

        if (!allChannels && channelNames.size() >= 4)
        {
            numChannels = 4;
            if (stripAlpha
                && (channelNames[3] == "A" || channelNames[3] == "a"))
                numChannels = 3;
        }
        else
        {
            numChannels = channelNames.size();
        }
        planar3 = numChannels == 3 && planar3channel;

        //
        //  Determine the type of the output FB
        //

        switch (exrPixelType)
        {
        case Imf::FLOAT:
            dataType = FrameBuffer::FLOAT;
            break;
        case Imf::HALF:
            dataType = FrameBuffer::HALF;
            break;
        case Imf::UINT:
            dataType = FrameBuffer::UINT;
            break;
        default:
            TWK_EXC_THROW_WHAT(Exception, "Unsupported exr data type");
        }

        //
        //  Setup framebuffer
        //

        if (!useRGBAReader && (planarYRYBY || planar3))
        {
            Imf::ChannelList::Iterator ci0 =
                findChannelWithBasename(planarYRYBY ? "Y" : "R", cl);
            Imf::ChannelList::Iterator ci1 =
                findChannelWithBasename(planarYRYBY ? "RY" : "G", cl);
            Imf::ChannelList::Iterator ci2 =
                findChannelWithBasename(planarYRYBY ? "BY" : "B", cl);
            Imf::ChannelList::Iterator ci3 = findChannelWithBasename("A", cl);

            size_t clSize = channelListSize(cl);
            bool checkForDuplicates = false;

            if (ci0 == cl.end())
            {
                checkForDuplicates = true;
                ci0 = cl.begin();
            }
            if (ci1 == cl.end())
            {
                checkForDuplicates = true;
                if (clSize > 1)
                {
                    ci1 = ++cl.begin();
                }
                else
                {
                    ci1 = cl.begin();
                }
            }
            if (ci2 == cl.end())
            {
                checkForDuplicates = true;
                if (clSize > 2)
                {
                    ci2 = ++(++cl.begin());
                }
                else
                {
                    if (clSize > 1)
                    {
                        ci2 = ++cl.begin();
                    }
                    else
                    {
                        ci2 = cl.begin();
                    }
                }
            }

            bool has4 = (!stripAlpha && ci3 != cl.end());

            planeChannels.resize((has4 || noOneChannelPlanes) ? 4 : 3);
            FrameBuffer::StringVector planeNames(has4 ? 4 : 3);
            FrameBuffer::Samplings xsamps(has4 || noOneChannelPlanes ? 4 : 3);
            FrameBuffer::Samplings ysamps(has4 || noOneChannelPlanes ? 4 : 3);

            planeChannels[0] = ci0;
            planeChannels[1] = ci1;
            planeChannels[2] = ci2;

            planeNames[0] = planarYRYBY ? "Y" : "R";
            xsamps[0] = ci0.channel().xSampling;
            ysamps[0] = ci0.channel().ySampling;

            planeNames[1] = planarYRYBY ? "RY" : "G";
            xsamps[1] = ci1.channel().xSampling;
            ysamps[1] = ci1.channel().ySampling;

            planeNames[2] = planarYRYBY ? "BY" : "B";
            xsamps[2] = ci2.channel().xSampling;
            ysamps[2] = ci2.channel().ySampling;

            if (has4 || noOneChannelPlanes)
            {
                planeChannels[3] = ci3;
                planeNames.resize(4);
                planeNames[3] = "A";
                xsamps[3] = ci3.channel().xSampling;
                ysamps[3] = ci3.channel().ySampling;
            }

            size_t nchannels = noOneChannelPlanes ? 2 : 1;

            if (noOneChannelPlanes)
            {
                planeNames.erase(planeNames.begin() + 3);
                planeChannels.erase(planeChannels.begin() + 3);
                planeNames.insert(planeNames.begin() + 1, "A");
                planeChannels.insert(planeChannels.begin() + 1, ci3);
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

            outfb->restructurePlanar(width, height, xsamps, ysamps, planeNames,
                                     dataType, FrameBuffer::BOTTOMLEFT,
                                     nchannels);
        }
        else
        {
            outfb->restructure(width, height, 0, numChannels, dataType);
        }

        //
        //  Copy over the channel names
        //

        string cch;

        if (planarYRYBY || planar3)
        {
            ostringstream str;

            for (int c = 0; c < numChannels; ++c)
            {
                if (c)
                    str << ", ";
                str << channelNames[c];
            }

            cch = str.str();
        }
        else
        {
            for (int c = 0; c < numChannels; ++c)
            {
                outfb->setChannelName(c, channelNames[c]);
                if (c)
                    cch += ", ";
                cch += channelNames[c];
            }
        }

        //
        //  Color space (EXR is always linear with Rec709 primaries)
        //

        outfb->setPrimaryColorSpace(ColorSpace::Rec709());
        outfb->setTransferFunction(ColorSpace::Linear());

        //
        //  Assign attributes
        //
        readAllAttributes(file, *outfb);

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

        outfb->newAttribute("ChannelsInFile", totalChannels);
        outfb->newAttribute("ChannelNamesInFile",
                            stl_ext::wrap(originalChannels));
        outfb->newAttribute("ChannelSamplingInFile",
                            stl_ext::wrap(originalSampling.str()));
        outfb->newAttribute("ChannelsRead", stl_ext::wrap(cch));
        outfb->newAttribute("IOexr/ReadWindow", string(rw));

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

        if (useRGBAReader) // valid for numparts=1 file.
        {
            //
            //  Let the EXR library handle the subsampling
            //
            Imf::RgbaInputFile rgbafile(filename.c_str());

            if (!view.empty())
            {
                if (layer.empty())
                {
                    rgbafile.setLayerName(view);
                }
                else
                {
                    rgbafile.setLayerName(view + "." + layer);
                }
            }
            else if (!layer.empty())
            {
                rgbafile.setLayerName(layer);
            }

            try
            {
                rgbafile.setFrameBuffer(outfb->pixels<Imf::Rgba>()
                                            - bufWin.min.x
                                            - bufWin.min.y * outfb->width(),
                                        1, outfb->width());
                rgbafile.readPixels(datWin.min.y, datWin.max.y);
                outfb->newAttribute("IOexr/Comment", string("Read as RGBA"));
            }
            catch (const std::exception& exc)
            {
                cerr << "WARNING: EXR: incomplete image \"" << filename << "\""
                     << endl;

                if (!outfb->hasAttribute("PartialImage"))
                {
                    outfb->newAttribute("PartialImage", 1.0);
                }
            }
        }
        else
        {
            //
            //  This is the general reader path.  Make exr frame buffer
            //  for reading. Note the funky origin of the frame buffer --
            //  its actually an out-of-bounds pointer. This is because we
            //  only want to read the data window (not the union of the
            //  display and data windows).
            //

            Imf::FrameBuffer exrFrameBuffer;

            if (planarYRYBY || planar3)
            {
                if (noOneChannelPlanes)
                {
                    unsigned int chindex = 0;

                    for (FrameBuffer* p = outfb; p; p = p->nextPlane())
                    {
                        Imf::ChannelList::Iterator ci =
                            planeChannels[chindex++];

                        int xs = ci.channel().xSampling;
                        int ys = ci.channel().ySampling;

#ifdef DEBUG_IOEXR
                        LOG.log("Reading channel name: %s", ci.name());
#endif
                        exrFrameBuffer.insert(
                            ci.name(),
                            Imf::Slice(exrPixelType,
                                       p->pixels<char>()
                                           - p->scanlineSize() * bufWin.min.y
                                           - p->pixelSize() * bufWin.min.x,
                                       p->pixelSize(), p->scanlineSize(), xs,
                                       ys));

                        ci = planeChannels[chindex++];

                        if (ci != cl.end())
                        {
                            xs = ci.channel().xSampling;
                            ys = ci.channel().ySampling;

#ifdef DEBUG_IOEXR
                            LOG.log("Reading channel name: %s", ci.name());
#endif
                            exrFrameBuffer.insert(
                                ci.name(),
                                Imf::Slice(
                                    exrPixelType,
                                    p->pixels<char>()
                                        - p->scanlineSize() * bufWin.min.y
                                        - p->pixelSize() * bufWin.min.x
                                        + p->bytesPerChannel(),
                                    p->pixelSize(), p->scanlineSize(), xs, ys));
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
                        const Imf::ChannelList::Iterator ci =
                            planeChannels[chindex++];

                        const int xs = ci.channel().xSampling;
                        const int ys = ci.channel().ySampling;

#ifdef DEBUG_IOEXR
                        LOG.log("Reading channel name: %s", ci.name());
#endif
                        exrFrameBuffer.insert(
                            ci.name(),
                            Imf::Slice(exrPixelType,
                                       p->pixels<char>()
                                           - p->scanlineSize() * bufWin.min.y
                                           - p->pixelSize() * bufWin.min.x,
                                       p->pixelSize(), p->scanlineSize(), xs,
                                       ys));
                    }
                }
            }
            else
            {
                for (int c = 0; c < numChannels; ++c)
                {
                    string name = channelNames[c];

#ifdef DEBUG_IOEXR
                    LOG.log("Reading channel name: %s", name.c_str());
#endif
                    exrFrameBuffer.insert(
                        name.c_str(),            // name
                        Imf::Slice(exrPixelType, // type
                                   outfb->pixels<char>()
                                       - outfb->scanlineSize() * bufWin.min.y
                                       - outfb->pixelSize() * bufWin.min.x
                                       + outfb->bytesPerChannel() * c,
                                   outfb->pixelSize(),      // xStride
                                   outfb->scanlineSize())); // yStride
                }
            }

            Imf::InputPart inpart(file, partNum);
            inpart.setFrameBuffer(exrFrameBuffer);

            try
            {
                inpart.readPixels(datWin.min.y, datWin.max.y);
            }
            catch (...)
            {
                cerr << "WARNING: EXR: incomplete image \"" << filename << "\""
                     << endl;

                if (!outfb->hasAttribute("PartialImage"))
                {
                    outfb->newAttribute("PartialImage", 1.0f);
                }
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

    void IOexr::readImagesFromMultiViewFile(
        Imf::MultiPartInputFile& file, FrameBufferVector& fbs,
        const string& filename, const string& requestedView,
        const string& requestedLayer, const string& requestedChannel,
        const bool requestedAllChannels, const int partNum,
        const ViewNames& views, bool requestedViewIsDefaultView) const
    {
#ifdef DEBUG_IOEXR
        LOG.log("Reading multiview exr with views:");
        for (int i = 0; i < views.size(); ++i)
        {
            LOG.log("View %d = %s", i + 1, views[i].c_str());
        }
#endif

        Imf::ChannelList fcl = file.header(partNum).channels();

        Imf::ChannelList cl;
        if (requestedView.empty())
        {
            cl = fcl;
        }
        else
        {
            cl = Imf::channelsInView(requestedView, fcl, views);

            // If the requestedView is the defaultView we need to include
            // channels that have no exr layer. This is a heuristic
            // as layerless channel names are said to belong to the defaultView.
            if (requestedViewIsDefaultView)
            {
                Imf::ChannelList dcl = Imf::channelsInView("", fcl, views);
                for (Imf::ChannelList::Iterator i = dcl.begin(); i != dcl.end();
                     ++i)
                {
                    cl.insert(i.name(), i.channel());
                }
            }

            //
            //  If no channels were found for this view, go back to
            //  using the "default" view/layer.  Maybe this file has a
            //  left view and no right or vice-versa.
            //
            if (cl.end() == cl.begin())
                cl = fcl;
        }

        //
        //  If there's a channel in the request so narrow to that
        //  specific channel.
        //

        bool stripAlpha = m_stripAlpha;

        if (!requestedChannel.empty())
        {
            string requestedFullChannel = requestedChannel;
            if (!requestedView.empty() && !requestedViewIsDefaultView)
            {
                requestedFullChannel =
                    requestedView + "." + requestedFullChannel;
            }

            if (!requestedLayer.empty())
            {
                requestedFullChannel =
                    requestedLayer + "." + requestedFullChannel;
            }

            Imf::ChannelList ncl;
            if (Imf::Channel* c = cl.findChannel(requestedFullChannel))
            {
                ncl.insert(requestedFullChannel, *c);
            }

            //
            //  Don't strip alpha if specifically requested
            //
            if (requestedChannel == "A" || requestedChannel == "a")
                stripAlpha = false;

            cl = ncl;
        }

        //
        //  Get a list of all the channels in the file and then decide
        //  what to do with them.
        //
        fbs.push_back(new FrameBuffer());
        if (requestedLayer.empty())
        {
            readMultiViewChannelList(
                filename, requestedLayer, requestedView, *fbs.back(), file,
                partNum, cl, m_rgbaOnly, m_convertYRYBY, m_planar3channel,
                requestedAllChannels, m_inheritChannels, m_noOneChannelPlanes,
                stripAlpha, m_readWindowIsDisplayWindow, m_readWindow);
        }
        else
        {
            //
            //  Specific layers requested
            //

            Imf::ChannelList::ConstIterator cstart;
            Imf::ChannelList::ConstIterator cend;
            cl.channelsInLayer(requestedLayer, cstart, cend);

            Imf::ChannelList ncl;
            for (Imf::ChannelList::ConstIterator ci = cstart; ci != cend; ++ci)
            {
                ncl.insert(ci.name(), ci.channel());
            }

            readMultiViewChannelList(
                filename, requestedLayer, requestedView, *fbs.back(), file,
                partNum, ncl, m_rgbaOnly, m_convertYRYBY, m_planar3channel,
                requestedAllChannels, m_inheritChannels, m_noOneChannelPlanes,
                stripAlpha, m_readWindowIsDisplayWindow, m_readWindow);
        }

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

    void IOexr::getMultiViewImageInfo(const Imf::MultiPartInputFile& file,
                                      const ViewNames& views, FBInfo& fbi) const
    {
        const int partNum = 0;
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

        Imf::PixelType exrPixelType;
        Imf::ChannelList cl = file.header(partNum).channels();
        fbi.numChannels = 0;

        fbi.proxy.setPrimaryColorSpace(ColorSpace::Rec709());
        fbi.proxy.setTransferFunction(ColorSpace::Linear());
        fbi.proxy.setPixelAspectRatio(fbi.pixelAspect);

        //
        //  The layers are found via the header's channel list
        //

        set<string> layers;
        cl.layers(layers);

        //
        //  Count the total number of channels in the file
        //

        fbi.numChannels = channelListSize(cl);
        fbi.channelInfos.resize(fbi.numChannels);
        size_t index = 0;

        set<string> channelSet;

        for (Imf::ChannelList::ConstIterator i = cl.begin(); i != cl.end();
             ++i, index++)
        {
            exrPixelType = i.channel().type;
            FBInfo::ChannelInfo& cinfo = fbi.channelInfos[index];
            // setChannelInfo(i, cinfo);
            setChannelInfo(baseChannelName(i.name()), i.channel().type, cinfo);
            channelSet.insert(i.name());
        }

        //
        //  The views (if they exist) are in the multiView attribute
        //

        if (!views.empty())
        {
            fbi.views = views;
            fbi.defaultView = Imf::defaultViewName(views);
            // Maintain sorted order of multiview

            Imf::ChannelList noviewChannels = channelsInNoView(cl, views);
            size_t numNoViewChannels = channelListSize(noviewChannels);
            bool hasNoViewChannels = numNoViewChannels > 0;

            fbi.viewInfos.resize(fbi.views.size()
                                 + (hasNoViewChannels ? 1 : 0));
            int defaultViewIndex = -1;

            for (size_t i = 0; i < fbi.viewInfos.size(); i++)
            {
                FBInfo::ViewInfo* vinfo = &(fbi.viewInfos[i]);

                bool noview = hasNoViewChannels && i == fbi.views.size();
                if (!noview)
                {
                    vinfo->name = fbi.views[i];
                    if (vinfo->name == fbi.defaultView)
                        defaultViewIndex = i;
                }
                else if (defaultViewIndex != -1)
                {
                    vinfo = &(fbi.viewInfos[defaultViewIndex]);
                }

                Imf::ChannelList vcl =
                    noview ? noviewChannels
                           : channelsInView(vinfo->name, cl, views);
                set<string> vlayers;
                vcl.layers(vlayers);

                vinfo->layers.resize(vinfo->layers.size() + vlayers.size());

                size_t lindex = 0;
                for (set<string>::const_iterator si = vlayers.begin();
                     si != vlayers.end(); ++si, lindex++)
                {
                    string layerName = *si;

                    //
                    //  If the view name is tacked on the end, take it off.
                    //
                    string vstr = string(".") + vinfo->name;
                    size_t pos = layerName.find(vstr);
                    if (pos != string::npos
                        && pos == layerName.size() - vstr.size())
                    {
                        layerName.erase(pos);
                    }

                    FBInfo::LayerInfo& linfo = vinfo->layers[lindex];
                    if (layerName != vinfo->name)
                    {
                        linfo.name = layerName;
                    }
                    Imf::ChannelList::ConstIterator cb;
                    Imf::ChannelList::ConstIterator ce;
                    vcl.channelsInLayer(*si, cb, ce);

                    size_t n = channelListIteratorDifference(cb, ce);
                    linfo.channels.resize(n);

                    for (size_t q = 0; q < n; q++, cb++)
                    {
                        // setChannelInfo(cb, linfo.channels[q]);
                        setChannelInfo(baseChannelName(cb.name()),
                                       cb.channel().type, linfo.channels[q]);
                        channelSet.erase(cb.name());
                    }
                }
            }
            if (hasNoViewChannels && defaultViewIndex != -1)
            //
            //  Then we added the noview channels to the defaultView, so get rid
            //  of the last view in our list, which was only to hold noview
            //  channels.
            //
            {
                fbi.viewInfos.pop_back();
            }

            //
            //  Add non-layer channels to first view
            //

            FBInfo::ViewInfo& vinfo = fbi.viewInfos.front();
            vinfo.otherChannels.resize(channelSet.size());
            size_t count = 0;

            for (set<string>::const_iterator i = channelSet.begin();
                 i != channelSet.end(); ++i, count++)
            {
                Imf::ChannelList::ConstIterator ci = cl.find(*i);
                // setChannelInfo(ci, vinfo.otherChannels[count]);
                setChannelInfo(baseChannelName(ci.name()), ci.channel().type,
                               vinfo.otherChannels[count]);
            }
        }
        else
        {
            //
            //  Report layers under unnamed view
            //

            fbi.viewInfos.resize(1);
            FBInfo::ViewInfo& vinfo = fbi.viewInfos.front();
            vinfo.layers.resize(layers.size());
            vinfo.name = "";
            fbi.defaultView = "";

            size_t lindex = 0;
            for (set<string>::const_iterator si = layers.begin();
                 si != layers.end(); ++si, lindex++)
            {
                const string& layerName = *si;
                FBInfo::LayerInfo& linfo = vinfo.layers[lindex];
                linfo.name = layerName;
                Imf::ChannelList::ConstIterator cb;
                Imf::ChannelList::ConstIterator ce;
                cl.channelsInLayer(layerName, cb, ce);

                size_t n = channelListIteratorDifference(cb, ce);
                linfo.channels.resize(n);

                for (size_t q = 0; q < n; q++, cb++)
                {
                    // setChannelInfo(cb, linfo.channels[q]);
                    setChannelInfo(baseChannelName(cb.name()),
                                   cb.channel().type, linfo.channels[q]);

                    //
                    //  Remove channels that live in a layer from the
                    //  channel name set. The left overs will have no
                    //  layer.
                    //

                    channelSet.erase(cb.name());
                }
            }

            //
            //  Add non-layer channels to otherChannels
            //

            vinfo.otherChannels.resize(channelSet.size());
            size_t count = 0;

            for (set<string>::const_iterator i = channelSet.begin();
                 i != channelSet.end(); ++i, count++)
            {
                Imf::ChannelList::ConstIterator ci = cl.find(*i);
                setChannelInfo(ci, vinfo.otherChannels[count]);
            }
        }

        //
        //  Remove any layer names which also appear as view names. This may
        //  nuke some channel combonations (like right.right.R) but for the
        //  love all that is sacred can we not make channel names like that in
        //  this multiview business?
        //

        set_difference(layers.begin(), layers.end(), fbi.views.begin(),
                       fbi.views.end(), back_inserter(fbi.layers));

        //
        //  Just float and half for EXR
        //

        switch (exrPixelType)
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
            TWK_THROW_STREAM(Exception,
                             "EXR: Unsupported data type: " << exrPixelType);
        }

        //
        //  Put the attributes onto the proxy image
        //
        readAllAttributes(file, fbi.proxy);
    }

    void IOexr::writeImagesToMultiViewFile(const ConstFrameBufferVector& fbs,
                                           const std::string& filename,
                                           const WriteRequest& request) const
    {
#if 0 //  ALAN_UNCROP
    cerr << "IOexr::writeImages file " << filename << endl;
    const FrameBuffer *myFB = fbs.front();
    cerr << "    " << myFB->identifier() << " uncrop w "
            " w " << myFB->width() << " h " << myFB->height() << 
            " uncrop w " << myFB->uncropWidth() << " h " << myFB->uncropHeight() << 
            " x " << myFB->uncropX() << " y " << myFB->uncropY() << 
            " on " << myFB->uncrop() << 
            endl;
#endif
        const FrameBuffer* lead_outfb = fbs[0];
        Imf::PixelType pixelType;
        Imf::LineOrder order;
        ConstFrameBufferVector outfbs(fbs.size());

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

        for (int i = 0; i < fbs.size(); i++)
        {
            const FrameBuffer* outfb = fbs[i];

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
                    pixelType = Imf::HALF;
                }
                else
                {
                    pixelType = Imf::FLOAT;
                }
                if (fb != outfb && fb != fbs[i])
                    delete fb;
                break;
            }
            case FrameBuffer::HALF:
                pixelType = Imf::HALF;
                break;
            case FrameBuffer::UCHAR:
            {
                const FrameBuffer* fb = outfb;
                outfb = copyConvert(outfb, FrameBuffer::HALF);
                if (fb != outfb && fb != fbs[i])
                    delete fb;
                pixelType = Imf::HALF;
                break;
            }
            case FrameBuffer::USHORT:
            {
                const FrameBuffer* fb = outfb;
                if (acesFile)
                {
                    outfb = copyConvert(outfb, FrameBuffer::HALF);
                    pixelType = Imf::HALF;
                }
                else
                {
                    outfb = copyConvert(outfb, FrameBuffer::FLOAT);
                    pixelType = Imf::FLOAT;
                }
                if (fb != outfb && fb != fbs[i])
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
                if (fb != outfb && fb != fbs[i])
                    delete fb;
                fb = outfb;
                outfb = copyConvert(outfb, FrameBuffer::HALF);
                if (fb != outfb && fb != fbs[i])
                    delete fb;
                pixelType = Imf::HALF;
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
                if (fb != outfb && fb != fbs[i])
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
                if (fb != outfb && fb != fbs[i])
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
                if (outfb == fbs[i])
                    outfb = outfb->copy();
                if (needsFlip)
                    flip(const_cast<FrameBuffer*>(outfb));
                if (needsFlop)
                    flop(const_cast<FrameBuffer*>(outfb));
                if (fb != outfb && fb != fbs[i])
                    delete fb;
            }

            outfbs[i] = outfb;
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

        Imf::FrameBuffer frameBuffer;
        Imath::Box2i window(Imath::V2i(0, 0),
                            Imath::V2i(outfbs.front()->width() - 1,
                                       outfbs.front()->height() - 1));

        Imath::Box2i displayWindow, dataWindow;
        displayWindow = window;
        dataWindow = window;

#if 0 //  ALAN_UNCROP
    //We should be able to make output exrs with proper data windows if the 
    //uncrop is set in the incoming fbs.
    if (outfbs.front()->uncrop())
    {
        int w = outfbs.front()->uncropWidth();
        int h = outfbs.front()->uncropWidth();
        int x = outfbs.front()->uncropX();
        int y = outfbs.front()->uncropY();

        displayWindow = Imath::Box2i(Imath::V2i(0,0), Imath::V2i(w-1, h-1));
        dataWindow    = Imath::Box2i(Imath::V2i(x,y), Imath::V2i(x+outfbs.front()->width()-1, y+outfbs.front()->height()-1));
    }
#endif

        Imf::Header header(displayWindow, dataWindow,
                           1.0f,             // pixelAspectRatio = 1
                           Imath::V2f(0, 0), // screenWindowCenter
                           1.0f,             // screenWindowWidth
                           order,            // lineOrder
                           compression);

        if (userAttrs.size())
        {
            for (size_t i = 0; i < userAttrs.size(); i++)
            {
                header.insert(userNames[i], *userAttrs[i]);
            }
        }

        for (int i = 0; useAttrs && i < outfbs.size(); i++)
        {
            const FrameBuffer* outfb = outfbs[i];

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

                string::size_type p = a->name().find("/./");
                if (p == string::npos)
                    continue;

                string name = a->name().substr(p + 3, string::npos);

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

                    if (name.find("EXR/") == 0)
                    {
                        name = name.substr(4, string::npos);

                        if (name == "compression" || name == "dataWindow"
                            || name == "displayWindow" || name == "lineOrder"
                            || name == "screenWindowCenter"
                            || name == "screenWindowWidth"
                            || name == "pixelAspectRatio"
                            || name == "ChannelsRead"
                            || name == "ChannelsInFile"
                            || name == "ChannelSamplingInFile"
                            || name == "AlphaType")
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
        }

        if ((compression == Imf::DWAA_COMPRESSION)
            || (compression == Imf::DWAB_COMPRESSION))
        {
            float dwaCompressionLevel =
                45.0f;                   // Default value in ImfDwaCompressor().
            if (request.quality != 0.9f) // the rvio default
            {
                dwaCompressionLevel = request.quality;
            }

            Imf::FloatAttribute attr(dwaCompressionLevel);
            header.insert("dwaCompressionLevel", attr);
        }

        //
        // stereo
        //

        if (fbs.size() == 2)
        {
            Imf::StringVector sv;

            sv.push_back("left");
            sv.push_back("right");

            addMultiView(header, sv);
        }

        // bool yca = false;

        for (int i = 0; i < outfbs.size(); i++)
        {
            const FrameBuffer* outfb = outfbs[i];

            string prefix("");
            if (fbs.size() == 2 && i == 1)
                prefix = "right.";

            if (outfb->isPlanar())
            {
                // yca = true;

                for (const FrameBuffer* fb = outfb; fb; fb = fb->nextPlane())
                {
                    int xsamp = outfb->width() / fb->width();
                    int ysamp = outfb->height() / fb->height();
                    string chname = prefix + fb->channelName(0);

                    header.channels().insert(
                        chname.c_str(),
                        Imf::Channel(pixelType, xsamp, ysamp,
                                     chname == "RY" || chname == "BY"));

                    frameBuffer.insert(
                        chname.c_str(), // name
                        Imf::Slice(
                            pixelType,                                // type
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

                    header.channels().insert(chname.c_str(), pixelType);

                    frameBuffer.insert(
                        chname.c_str(),       // name
                        Imf::Slice(pixelType, // type
                                   &outfb->pixel<char>(0, outfb->height() - 1,
                                                       c),   // base
                                   outfb->pixelSize(),       // xStride
                                   -outfb->scanlineSize())); // yStride
                }
            }
        }

        header.sanityCheck();

        // if (acesFile)
        // {
        //     Imf::AcesOutputFile file(filename.c_str(),
        //                              header,
        //                              yca ? Imf::WRITE_YCA : Imf::WRITE_RGBA);
        //     file.setFrameBuffer(frameBuffer);
        //     file.writePixels(lead_outfb->height());
        // }
        // else
        {
            Imf::OutputFile file(filename.c_str(), header);
            file.setFrameBuffer(frameBuffer);
            file.writePixels(lead_outfb->height());
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
