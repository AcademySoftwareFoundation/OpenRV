//
//  Copyright (c) 2010 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPGraph__RetimeGroupIPNode__h__
#define __IPGraph__RetimeGroupIPNode__h__
#include <iostream>
#include <IPCore/GroupIPNode.h>

namespace IPCore
{
    class RetimeIPNode;
    class PaintIPNode;

    /// RetimeGroupIPNode manages a sub-graph that includes a retime

    ///
    /// The sub-graph contains one retime node and a paint node
    ///

    class RetimeGroupIPNode : public GroupIPNode
    {
    public:
        RetimeGroupIPNode(const std::string& name, const NodeDefinition* def,
                          IPGraph* graph, GroupIPNode* group = 0);

        virtual ~RetimeGroupIPNode();

        virtual void setInputs(const IPNodes&);
        virtual IPNode* newSubGraphForInput(size_t, const IPNodes&);

        RetimeIPNode* retimeNode() const { return m_retimeNode; }

    private:
        std::string retimeType();
        std::string paintType();

    private:
        RetimeIPNode* m_retimeNode;
    };

} // namespace IPCore

#endif // __IPGraph__RetimeGroupIPNode__h__
