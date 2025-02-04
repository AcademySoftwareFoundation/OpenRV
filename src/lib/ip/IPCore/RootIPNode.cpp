//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCore/RootIPNode.h>
#include <IPCore/GroupIPNode.h>
#include <IPCore/IPGraph.h>

namespace IPCore
{
    using namespace std;

    RootIPNode::RootIPNode(const string& name, const NodeDefinition* def,
                           IPGraph* graph, GroupIPNode* group)
        : IPNode(name, def, graph, 0)
    {
        setWritable(false);
        setUnconstrainedInputs(true);
    }

    RootIPNode::~RootIPNode() {}

    IPImage* RootIPNode::evaluate(const Context& context)
    {
        const IPNodes& nodes = inputs();
        if (nodes.empty())
            return IPImage::newNoImage(this, "No Input");
        IPImage* root =
            new IPImage(this, IPImage::GroupType, 0, 0, 1.0, IPImage::NoBuffer);

        try
        {
            // make sure audio waveform is the first
            int hasAudioWave = -1;
            for (size_t i = 1; i < nodes.size(); i++)
            {
                if (nodes[i]->name().find("audioWaveform") != std::string::npos)
                {
                    hasAudioWave = i;
                    break;
                }
            }
            size_t index = 0;
            if (hasAudioWave >= 1)
            {
                if (IPImage* img = nodes[hasAudioWave]->evaluate(context))
                {
                    root->appendChild(img);
                }
                for (size_t i = 0; i < hasAudioWave; i++)
                {
                    if (IPImage* img = nodes[i]->evaluate(context))
                    {
                        root->appendChild(img);
                    }
                }
                for (size_t i = hasAudioWave + 1; i < nodes.size(); i++)
                {
                    if (IPImage* img = nodes[i]->evaluate(context))
                    {
                        root->appendChild(img);
                    }
                }
            }
            else
            {
                for (size_t i = 0; i < nodes.size(); i++)
                {
                    if (IPImage* img = nodes[i]->evaluate(context))
                    {
                        root->appendChild(img);
                    }
                }
            }
        }
        catch (exception& exc)
        {
            //
            //  If we have evaluated any children, those FBs will have been
            //  checked out of the cache.  They must be checked back in or they
            //  will not be properly dereferenced and we'll never be able to
            //  delete them.
            //

            TWK_CACHE_LOCK(graph()->cache(), "root exc");
            graph()->cache().checkInAndDelete(root);
            TWK_CACHE_UNLOCK(graph()->cache(), "root exc");
            throw;
        }

        return root;
    }

    IPImageID* RootIPNode::evaluateIdentifier(const Context& context)
    {
        const IPNodes& nodes = inputs();
        if (nodes.empty())
            return 0;

        IPImageID* idnode = new IPImageID;
        IPImageID* next = 0;

        for (int i = 0; i < nodes.size(); i++)
        {
            IPImageID* child = inputs()[i]->evaluateIdentifier(context);
            if (child)
            {
                if (next)
                    next->next = child;
                next = child;
                if (!i)
                    idnode->children = child;
            }
        }

        return idnode;
    }

} // namespace IPCore
