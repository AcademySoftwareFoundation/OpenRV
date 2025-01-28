//
//  Copyright (c) 2010 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCore__DisplayGroupIPNode__h__
#define __IPCore__DisplayGroupIPNode__h__
#include <iostream>
#include <IPCore/GroupIPNode.h>

namespace TwkApp
{
    class VideoDevice;
}

namespace IPCore
{
    class AdaptorIPNode;
    class DisplayStereoIPNode;
    class PipelineGroupIPNode;
    class ResizeIPNode;

    /// DisplayGroupIPNode manages a sub-graph for display

    ///
    /// The sub-graph contains at least a DisplayIPNode,
    /// DisplayStereoIPNode, and DispTransform2DIPNode. The NodeDefinition
    /// determines which actual node types are used.
    ///
    /// NOTE: this class is special in that it handles information about
    /// the current VideoDevice.
    ///

    class DisplayGroupIPNode : public GroupIPNode
    {
    public:
        typedef TwkApp::VideoDevice VideoDevice;

        DisplayGroupIPNode(const std::string& name, const NodeDefinition* def,
                           IPGraph* graph, GroupIPNode* group = 0);

        virtual ~DisplayGroupIPNode();

        virtual void setInputs(const IPNodes&);
        virtual IPImage* evaluate(const Context&);
        virtual IPImageID* evaluateIdentifier(const Context& context);
        virtual void metaEvaluate(const Context&, MetaEvalVisitor&);
        virtual ImageStructureInfo imageStructureInfo(const Context&) const;
        virtual void mediaInfo(const Context&, MediaInfoVector&) const;

        bool dualOutputMode() const;

        const VideoDevice* imageDevice() const;

        void setPhysicalVideoDevice(const VideoDevice*);

        const VideoDevice* physicalDevice() const { return m_physicalDevice; }

        void setOutputVideoDevice(const VideoDevice*);

        const VideoDevice* outputDevice() const { return m_outputDevice; }

        ResizeIPNode* resizeNode() const { return m_resizeNode; }

        DisplayStereoIPNode* stereoNode() const { return m_stereoNode; }

        PipelineGroupIPNode* displayPipelineNode() const
        {
            return m_displayPipelineNode;
        }

        void incrementRenderHashCount();

    private:
        void initNewContext(const Context&, Context&) const;

    private:
        const VideoDevice* m_physicalDevice;
        const VideoDevice* m_outputDevice;
        AdaptorIPNode* m_adaptor;
        ResizeIPNode* m_resizeNode;
        DisplayStereoIPNode* m_stereoNode;
        PipelineGroupIPNode* m_displayPipelineNode;
        StringProperty* m_deviceName;
        StringProperty* m_moduleName;
        StringProperty* m_systemProfileURL;
        StringProperty* m_systemProfileType;
        IntProperty* m_renderHashCount;
    };

} // namespace IPCore

#endif // __IPCore__DisplayGroupIPNode__h__
