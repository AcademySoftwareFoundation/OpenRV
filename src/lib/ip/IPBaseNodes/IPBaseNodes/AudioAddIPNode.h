//
//  Copyright (c) 2015 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPBaseNodes__AudioAddIPNode__h__
#define __IPBaseNodes__AudioAddIPNode__h__
#include <IPCore/IPNode.h>
#include <iostream>

namespace IPCore
{

    /// Adds an audio source to an image source

    ///
    /// AudioAddIPNode mixes the audio from an image+audio source with any
    /// number of audio only sources. Any imagery provided by the audio
    /// only sources (not the first input) are ignored.
    ///

    class AudioAddIPNode : public IPNode
    {
    public:
        AudioAddIPNode(const std::string& name, const NodeDefinition* def,
                       IPGraph* graph, GroupIPNode* group = 0);

        virtual ~AudioAddIPNode();

        virtual IPImage* evaluate(const Context&);
        virtual IPImageID* evaluateIdentifier(const Context&);
        virtual size_t audioFillBuffer(const AudioContext&);

    private:
        FloatProperty* m_audioOffset;
    };

} // namespace IPCore

#endif // __IPBaseNodes__AudioAddIPNode__h__
