//
//  Copyright (c) 2010 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCore/AdaptorIPNode.h>
#include <IPCore/ViewGroupIPNode.h>
#include <IPCore/NodeDefinition.h>
#include <IPCore/PipelineGroupIPNode.h>
#include <IPCore/SoundTrackIPNode.h>
#include <IPCore/DispTransform2DIPNode.h>
#include <IPCore/AudioTextureIPNode.h>

#include <sstream>

namespace IPCore
{
    using namespace std;

    ViewGroupIPNode::ViewGroupIPNode(const std::string& name,
                                     const NodeDefinition* def, IPGraph* graph,
                                     GroupIPNode* group)
        : GroupIPNode(name, def, graph, group)
    {
        string soundtrackType =
            def->stringValue("defaults.soundtrackType", "SoundTrack");
        string transformType =
            def->stringValue("defaults.dispTransformType", "DispTransform2D");
        string waveformType =
            def->stringValue("defaults.waveformType", "AudioTexture");
        string pipelineType =
            def->stringValue("defaults.pipelineType", "PipelineGroup");

        m_adaptor = newMemberNodeOfType<AdaptorIPNode>("Adaptor", "adaptor");
        m_viewPipelineNode = newMemberNodeOfType<PipelineGroupIPNode>(
            pipelineType, "viewPipeline");
        m_soundtrackNode =
            newMemberNodeOfType<SoundTrackIPNode>(soundtrackType, "soundtrack");
        m_transformNode =
            newMemberNodeOfType<DispTransform2DIPNode>(transformType, "dxform");
        m_waveformNode = newMemberNodeOfType<AudioTextureIPNode>(
            waveformType, "audioWaveform");

        m_viewPipelineNode->setInputs1(m_adaptor);
        m_soundtrackNode->setInputs1(m_viewPipelineNode);
        m_transformNode->setInputs1(m_soundtrackNode);
        m_waveformNode->setInputs1(m_soundtrackNode);

        setRoot(m_transformNode);
    }

    ViewGroupIPNode::~ViewGroupIPNode() {}

    void ViewGroupIPNode::setInputs(const IPNodes& newInputs)
    {
        //
        //  We shouldn't have more than one input
        //

        if (newInputs.size() > 1)
        {
            cout << "WARNING: ViewGroupIPNode: more than one input" << endl;
        }

        if (newInputs.size() == 1)
        {
            m_adaptor->setGroupInputNode(newInputs.front());
        }
        else
        {
            m_adaptor->setGroupInputNode(0);
        }

        IPNode::setInputs(newInputs);
    }

} // namespace IPCore
