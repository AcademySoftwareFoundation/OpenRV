//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCore__UITextureIPNode__h__
#define __IPCore__UITextureIPNode__h__
#include <IPCore/IPNode.h>
#include <iostream>

namespace IPCore
{

    //
    //  AudioTextureIPNode provides a render output for the audio waveform
    //  texture. This node can only be attached to a SoundTrackIPNode
    //

    class AudioTextureIPNode : public IPNode
    {
    public:
        AudioTextureIPNode(const std::string& name, const NodeDefinition* def,
                           IPGraph* graph, GroupIPNode* group = 0);

        virtual ~AudioTextureIPNode();

        virtual IPImage* evaluate(const Context&);
        virtual void metaEvaluate(const Context&, MetaEvalVisitor&);
    };

} // namespace IPCore

#endif // __IPCore__AudioTextureIPNode__h__
