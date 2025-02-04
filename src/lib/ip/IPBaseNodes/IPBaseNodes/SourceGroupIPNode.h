//
//  Copyright (c) 2010 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPGraph__SourceGroupIPNode__h__
#define __IPGraph__SourceGroupIPNode__h__
#include <iostream>
#include <IPBaseNodes/SourceIPNode.h>
#include <IPCore/GroupIPNode.h>

namespace IPCore
{
    class PipelineGroupIPNode;

    /// SourceGroupIPNode manages a sub-graph

    ///
    /// The sub-graph contains one SourceIPNode as a single leaf. The rest
    /// of the nodes in the sub-graph include caching, color, luts, etc.
    ///

    class SourceGroupIPNode : public GroupIPNode
    {
    public:
        SourceGroupIPNode(const std::string& name, const NodeDefinition* def,
                          IPGraph* graph, GroupIPNode* group = 0);

        SourceGroupIPNode(const std::string& name, const NodeDefinition* def,
                          SourceIPNode*, IPGraph* graph,
                          GroupIPNode* group = 0);

        virtual ~SourceGroupIPNode();

        virtual void setInputs(const IPNodes&);
        virtual void readCompleted(const std::string& type,
                                   unsigned int version);

        void setUINameFromMedia(int index = 0);

        SourceIPNode* sourceNode() const { return m_sourceNode; }

        IPNode* beforeLinearizeNode() const { return m_beforeLinearizeNode; }

        IPNode* afterLinearizeNode() const { return m_afterLinearizeNode; }

        IPNode* beforeColorNode() const { return m_beforeColorNode; }

        IPNode* afterColorNode() const { return m_afterColorNode; }

        PipelineGroupIPNode* linearizePipeline() const
        {
            return m_linearizePipeline;
        }

        PipelineGroupIPNode* colorPipeline() const { return m_colorPipeline; }

        PipelineGroupIPNode* lookPipeline() const { return m_lookPipeline; }

    private:
        void init(const std::string&, const NodeDefinition*, SourceIPNode*,
                  IPGraph*, GroupIPNode*);

    private:
        SourceIPNode* m_sourceNode;
        IPNode* m_beforeLinearizeNode;
        IPNode* m_afterLinearizeNode;
        IPNode* m_beforeColorNode;
        IPNode* m_afterColorNode;
        PipelineGroupIPNode* m_linearizePipeline;
        PipelineGroupIPNode* m_colorPipeline;
        PipelineGroupIPNode* m_lookPipeline;

        IntProperty* m_markersIn;
        IntProperty* m_markersOut;
        Vec4fProperty* m_markersColor;
        StringProperty* m_markersName;
    };

} // namespace IPCore

#endif // __IPGraph__SourceGroupIPNode__h__
