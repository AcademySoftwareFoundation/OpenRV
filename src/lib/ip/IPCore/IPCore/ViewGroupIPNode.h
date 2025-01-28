//
//  Copyright (c) 2010 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCore__ViewGroupIPNode__h__
#define __IPCore__ViewGroupIPNode__h__
#include <iostream>
#include <IPCore/GroupIPNode.h>

namespace TwkApp
{
    class VideoDevice;
}

namespace IPCore
{
    class AdaptorIPNode;
    class SoundTrackIPNode;
    class DispTransform2DIPNode;
    class AudioTextureIPNode;
    class PipelineGroupIPNode;

    //
    //  ViewGroupIPNode manages a sub-graph for the portion of the display stack
    //  that is not per-device.  Each graph has a single ViewGroup, the is a
    //  top-level node that does not appear in the session manager and cannot be
    //  deleted or explicitly created by the user.
    //

    class ViewGroupIPNode : public GroupIPNode
    {
    public:
        ViewGroupIPNode(const std::string& name, const NodeDefinition* def,
                        IPGraph* graph, GroupIPNode* group = 0);

        virtual ~ViewGroupIPNode();

        virtual void setInputs(const IPNodes&);

        // virtual IPImage* evaluate(const Context&);

        DispTransform2DIPNode* transformNode() const { return m_transformNode; }

        SoundTrackIPNode* soundtrackNode() const { return m_soundtrackNode; }

        AudioTextureIPNode* waveformNode() const { return m_waveformNode; }

        PipelineGroupIPNode* viewPipelineNode() const
        {
            return m_viewPipelineNode;
        }

    private:
        AdaptorIPNode* m_adaptor;
        PipelineGroupIPNode* m_viewPipelineNode;
        DispTransform2DIPNode* m_transformNode;
        SoundTrackIPNode* m_soundtrackNode;
        AudioTextureIPNode* m_waveformNode;
        StringProperty* m_deviceName;
    };

} // namespace IPCore

#endif // __IPCore__ViewGroupIPNode__h__
