//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <IPBaseNodes/ChannelMapIPNode.h>
#include <IPCore/GroupIPNode.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkFB/Operations.h>
#include <iostream>
#include <stl_ext/stl_ext_algo.h>
#include <stl_ext/string_algo.h>

namespace IPCore
{
    using namespace TwkContainer;
    using namespace TwkFB;
    using namespace std;
    using namespace stl_ext;

    ChannelMapIPNode::ChannelMapIPNode(const std::string& name,
                                       const NodeDefinition* def,
                                       IPGraph* graph, GroupIPNode* group)
        : IPNode(name, def, graph, group)
    {
        m_showErrorMessageInEvaluate = true;
        setMaxInputs(1);
        m_channels = createProperty<StringProperty>("format.channels");
    }

    ChannelMapIPNode::~ChannelMapIPNode() {}

    void ChannelMapIPNode::setChannelMap(const std::vector<std::string>& chmap)
    {
        if (StringProperty* p = m_channels)
        {
            p->resize(chmap.size());
            p->valueContainer() = chmap;
        }
    }

    void ChannelMapIPNode::outputDisconnect(IPNode* node)
    {
        IPNode::outputDisconnect(node);
    }

    IPImage* ChannelMapIPNode::evaluate(const Context& context)
    {
        IPImage* head = IPNode::evaluate(context);
        if (!head)
            return IPImage::newNoImage(this, "No Input");
        int frame = context.frame;

        StringProperty* chmap = m_channels;

        if (!chmap->empty())
        {
            ostringstream str;

            for (int i = 0; i < chmap->size(); i++)
            {
                str << ":" << (*chmap)[i];
            }

            IPImage* img = head;

            if (img->fb)
            {
                FrameBuffer* in = img->fb;
                FrameBuffer* fb = 0;

                fb = TwkFB::channelMapToPlanar(in, chmap->valueContainer());

                if (fb)
                {
                    fb->setIdentifier(in->identifier());
                    in->copyAttributesTo(fb);
                    if (in->hasAttribute("PixelAspectRatio"))
                    {
                        fb->setPixelAspectRatio(in->pixelAspectRatio());
                    }
                    img->fb = fb;
                    delete in;
                }
                else
                {
                    if (m_showErrorMessageInEvaluate)
                    {
                        ostringstream str;
                        for (int i = 0; i < chmap->size(); i++)
                        {
                            if (i)
                            {
                                str << ", ";
                            }
                            str << (*chmap)[i];
                        }
                        cerr << "ERROR: channel map '" << str.str()
                             << "' failed" << endl;
                        m_showErrorMessageInEvaluate = false;
                    }
                }
                //
                //  Still need to copy the mapping into the identifer,
                //  whether the mapping fails or not, otherwise we don't
                //  match evaluateIdentifier.
                //
                img->fb->idstream() << str.str();
            }
        }

        return head;
    }

    IPImageID* ChannelMapIPNode::evaluateIdentifier(const Context& context)
    {
        IPImageID* imgid = IPNode::evaluateIdentifier(context);

        StringProperty* chmap = m_channels;

        IPImageID* i = imgid;
        ostringstream str;

        if (!chmap->empty() && chmap->front() != "")
        {
            str << i->id;

            for (int q = 0; q < chmap->size(); q++)
            {
                str << ":" << (*chmap)[q];
            }

            i->id = str.str();
        }

        return imgid;
    }

    void ChannelMapIPNode::propertyChanged(const Property* p)
    {
        m_showErrorMessageInEvaluate = true;

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
