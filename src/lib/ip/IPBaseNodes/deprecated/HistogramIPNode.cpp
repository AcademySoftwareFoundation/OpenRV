//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <deprecated/HistogramIPNode.h>
#include <IPCore/GroupIPNode.h>
#include <TwkFB/Operations.h>
#include <TwkFB/Histogram.h>
#include <TwkMath/Vec2.h>
#include <TwkMath/Iostream.h>
#include <limits>
#include <cmath>

namespace IPCore
{
    using namespace TwkFB;
    using namespace TwkContainer;
    using namespace TwkMath;
    using namespace std;

    HistogramIPNode::HistogramIPNode(const std::string& name, IPGraph* g,
                                     GroupIPNode* group)
        : IPNode(name, g, group)
    {
        init();
    }

    void HistogramIPNode::init()
    {
        setProtocol("RVHistogram");
        setMaxInputs(1);

        IntProperty* ip;
        Property::Info* info = new Property::Info();
        info->setPersistent(false);

        ip = createProperty<IntProperty>("node", "active");
        ip->resize(1);
        ip->front() = 0;

        ip = createProperty<IntProperty>("histogram", "size");
        ip->resize(1);
        ip->front() = 100;
    }

    HistogramIPNode::~HistogramIPNode() {}

    void HistogramIPNode::computeHistorgram(IPImage* image)
    {
        if (FrameBuffer* fb = image->fb)
        {
            IntProperty* size = property<IntProperty>("histogram", "size");

            vector<FloatProperty*> fp;
            size_t nplanes = fb->numPlanes();
            size_t nchannels = fb->numChannels();

            size_t n = max(nplanes, nchannels);

            FBHistorgram histogram(n);

            MinMaxPair rmm = computeChannelHistogram(image->fb, histogram,
                                                     size->front(), false);

            fb->attribute<Vec2f>("Histogram/Bounds") =
                Vec2f(rmm.first, rmm.second);

            float min = numeric_limits<float>::max();
            float max = numeric_limits<float>::min();

            float minAlt = numeric_limits<float>::max();
            float maxAlt = numeric_limits<float>::min();

            //
            //  We're only going to use with the first three channels
            //  These are the display R G B channels
            //

            bool hasRGB = false;

            for (int i = histogram.size() - 1; i >= 0; i--)
            {
                const ChannelHistogram& ch = histogram[i];
                ostringstream str;
                str << "Histogram/" << ch.channelName;

                TypedFBVectorAttribute<float>* attr =
                    new TypedFBVectorAttribute<float>(str.str(), ch.histogram);

                fb->addAttribute(attr);

                if (ch.channelName == "R" || ch.channelName == "G"
                    || ch.channelName == "B")
                {
                    if (ch.min > numeric_limits<float>::min())
                        min = std::min(min, ch.min);
                    if (ch.max < numeric_limits<float>::max())
                        max = std::max(max, ch.max);
                    hasRGB = true;
                }
                else
                {
                    if (ch.min > numeric_limits<float>::min())
                        minAlt = std::min(minAlt, ch.min);
                    if (ch.max < numeric_limits<float>::max())
                        maxAlt = std::max(maxAlt, ch.max);
                }
            }

            if (hasRGB)
            {
                if (std::fabs(min) < numeric_limits<float>::epsilon())
                    min = 0;
                if (std::fabs(max) < numeric_limits<float>::epsilon())
                    max = 0;

                image->fb->attribute<Vec2f>("ColorBounds") = Vec2f(min, max);
            }
            else
            {
                if (std::fabs(minAlt) < numeric_limits<float>::epsilon())
                    minAlt = 0;
                if (std::fabs(maxAlt) < numeric_limits<float>::epsilon())
                    maxAlt = 0;

                image->fb->attribute<Vec2f>("ColorBounds") =
                    Vec2f(minAlt, maxAlt);
            }
        }
    }

    IPImage* HistogramIPNode::evaluate(const Context& context)
    {
        if (IPImage* image = IPNode::evaluate(context))
        {
            if (image->fb && image->fb->identifier() != m_currentID)
            {
                if (IntProperty* ip =
                        createProperty<IntProperty>("node", "active"))
                {
                    if (ip->front() == 1)
                    {
                        computeHistorgram(image);
                        IntProperty* size =
                            property<IntProperty>("histogram", "size");
                        image->fb->idstream() << ":h_" << size->front();
                    }
                }
            }

            return image;
        }
        else
        {
            return IPImage::newNoImage(this, "No Input");
        }
    }

    IPImageID* HistogramIPNode::evaluateIdentifier(const Context& context)
    {
        IPImageID* imgid = IPNode::evaluateIdentifier(context);

        bool active = false;

        if (IntProperty* ip = property<IntProperty>("node", "active"))
        {
            if (ip->size() && ip->front() == 1)
            {
                active = true;
            }
        }

        if (active)
        {
            if (IntProperty* size = property<IntProperty>("histogram", "size"))
            {
                if (size->size() > 0)
                {
                    ostringstream str;
                    str << imgid->id << ":h_" << size->front();
                    imgid->id = str.str();
                }
            }
        }

        return imgid;
    }

    void HistogramIPNode::propertyChanged(const Property* p)
    {
        //
        //  All properties of this node affect caching of all frames of the
        //  corresponding source.  so if the props change, flush all frames from
        //  this source.
        //
        if (group())
            group()->flushIDsOfGroup();

        IPNode::propertyChanged(p);
    }

} // namespace IPCore
