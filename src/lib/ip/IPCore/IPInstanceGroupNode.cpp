//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCore/IPInstanceGroupNode.h>

namespace IPCore
{
    using namespace std;

    IPInstanceGroupNode::IPInstanceGroupNode(const std::string& name,
                                             const NodeDefinition* def,
                                             IPGraph* graph, GroupIPNode* group)
        : GroupIPNode(name, def, graph, group)
    {
    }

    IPInstanceGroupNode::~IPInstanceGroupNode() {}

    IPImage* IPInstanceGroupNode::evaluate(const Context&) { return NULL; }

} // namespace IPCore
