//
//  Copyright (c) 2015 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPBaseNodes/AudioAddIPNode.h>
#include <TwkAudio/Audio.h>

namespace IPCore
{
    using namespace std;
    using namespace TwkAudio;
    using namespace TwkContainer;
    using namespace TwkMath;

    AudioAddIPNode::AudioAddIPNode(const std::string& name,
                                   const NodeDefinition* def, IPGraph* graph,
                                   GroupIPNode* group)
        : IPNode(name, def, graph, group)
    {
        m_audioOffset = declareProperty<FloatProperty>("audio.offset", 0.0f);
    }

    AudioAddIPNode::~AudioAddIPNode() {}

    IPImage* AudioAddIPNode::evaluate(const Context& context)
    {
        //
        //  Just evaluate the first input for imagery
        //

        const IPNodes& nodes = inputs();
        if (nodes.empty())
            return IPImage::newNoImage(this, "No Input");

        IPImage* root = nodes.front()->evaluate(context);
        if (!root)
            return IPImage::newNoImage(this, "No Input");

        return root;
    }

    IPImageID* AudioAddIPNode::evaluateIdentifier(const Context& context)
    {
        const IPNodes& nodes = inputs();
        if (nodes.empty())
            return 0;
        return nodes.front()->evaluateIdentifier(context);
    }

    size_t AudioAddIPNode::audioFillBuffer(const AudioContext& context)
    {
        const IPNodes& nodes = inputs();
        if (nodes.empty())
            return 0;

        size_t rval = nodes.front()->audioFillBuffer(context);

        if (nodes.size() > 1)
        {
            for (size_t i = 1; i < nodes.size(); i++)
            {
                AudioBuffer audioBuffer(
                    context.buffer.size(), context.buffer.channels(),
                    context.buffer.rate(), context.buffer.startTime());
                AudioContext subContext(audioBuffer, context.fps);

                const Time contextStart = samplesToTime(
                    context.buffer.startSample(), context.buffer.rate());
                const Time offset =
                    propertyValue<FloatProperty>(m_audioOffset, 0.0f);

                subContext.buffer.setStartTime(offset + contextStart);
                subContext.buffer.zero();

                rval = max(inputs()[i]->audioFillBuffer(subContext), rval);

                transform(context.buffer.pointer(),
                          context.buffer.pointer()
                              + context.buffer.sizeInFloats(),
                          subContext.buffer.pointer(), context.buffer.pointer(),
                          plus<float>());
            }
        }

        return rval;
    }

} // namespace IPCore
