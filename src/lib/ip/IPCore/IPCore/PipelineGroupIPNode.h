//
//  Copyright (c) 2013 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCore__PipelineGroupIPNode__h__
#define __IPCore__PipelineGroupIPNode__h__
#include <iostream>
#include <IPCore/GroupIPNode.h>

namespace IPCore
{
    class AdaptorIPNode;

    //
    //  PipelineGroupIPNode
    //
    //  Holds an arbitrarily long pipeline of single input nodes.
    //

    class PipelineGroupIPNode : public GroupIPNode
    {
    public:
        PipelineGroupIPNode(const std::string& name, const NodeDefinition* def,
                            IPGraph* graph, GroupIPNode* group = 0);

        virtual ~PipelineGroupIPNode();
        virtual void copyNode(const IPNode*);
        virtual void setInputs(const IPNodes&);
        virtual void propertyChanged(const Property*);
        virtual void readCompleted(const std::string&, unsigned int);

        void setPipeline1(const std::string&);
        void setPipeline2(const std::string&, const std::string&);
        void setPipeline3(const std::string&, const std::string&,
                          const std::string&);
        void setPipeline4(const std::string&, const std::string&,
                          const std::string&, const std::string&);
        void setPipeline(const StringVector&);
        StringVector pipeline() const;

        const IPNodes& pipelineNodes() const { return m_nodes; }

        IPNode* findNodeInPipelineByTypeName(const std::string& name) const;

        StringProperty* pipelineProperty() const { return m_nodeTypeNames; }

        std::string profile() const;

    private:
        void clearPipeline();
        void buildPipeline();

    private:
        StringProperty* m_nodeTypeNames;
        IPNodes m_nodes;
    };

} // namespace IPCore

#endif // __IPCore__PipelineGroupIPNode__h__
