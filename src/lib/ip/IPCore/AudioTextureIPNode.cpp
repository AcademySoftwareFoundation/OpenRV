//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCore/AudioTextureIPNode.h>
#include <IPCore/SoundTrackIPNode.h>

namespace IPCore
{
    using namespace std;

    AudioTextureIPNode::AudioTextureIPNode(const string& name,
                                           const NodeDefinition* def,
                                           IPGraph* graph, GroupIPNode* group)
        : IPNode(name, def, graph, group)
    {
        setWritable(false);
        setMaxInputs(1);
    }

    AudioTextureIPNode::~AudioTextureIPNode() {}

    IPImage* AudioTextureIPNode::evaluate(const Context& context)
    {
        if (inputs().empty())
        {
            return IPImage::newNoImage(this, "audio waveform unavailable");
        }
        else
        {
            SoundTrackIPNode* snode =
                static_cast<SoundTrackIPNode*>(inputs().front());
            return snode->evaluateAudioTexture(context);
        }
    }

    void AudioTextureIPNode::metaEvaluate(const Context&, MetaEvalVisitor&)
    {
        //
        //  There's no real evaluation path along audio texture so don't
        //  do anything here.
        //
    }

} // namespace IPCore
