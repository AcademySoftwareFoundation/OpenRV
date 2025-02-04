//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCore__IPInstanceGroupNode__h__
#define __IPCore__IPInstanceGroupNode__h__
#include <iostream>
#include <IPCore/GroupIPNode.h>

namespace IPCore
{
    class NodeDefinition;

    /// IPInstanceGroupNode is an GroupIPNode which implements a
    /// GroupNodeDefinition

    ///
    /// IPInstanceGroupNode is an instance of a GroupNodeDefinition. The
    /// GroupNodeDefinition completely determines the behavior of the
    /// node: how its constructed, how subgraphs are built for each new
    /// input, etc.
    ///

    class IPInstanceGroupNode : public GroupIPNode
    {
    public:
        IPInstanceGroupNode(const std::string& name, const NodeDefinition* def,
                            IPGraph* graph, GroupIPNode* group = 0);

        virtual ~IPInstanceGroupNode();
        virtual IPImage* evaluate(const Context&);
    };

} // namespace IPCore

#endif // __IPCore__IPInstanceGroupNode__h__
