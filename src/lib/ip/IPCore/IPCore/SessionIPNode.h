//
//  Copyright (c) 2011 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCore__SessionIPNode__h__
#define __IPCore__SessionIPNode__h__
#include <iostream>
#include <IPCore/IPNode.h>

namespace IPCore
{

    /// SessionIPNode is used by IPGraph to keep a node representation of extra
    /// variables but it is not connected in the graph in any way

    class SessionIPNode : public IPNode
    {
    public:
        SessionIPNode(const std::string& name, const NodeDefinition* def,
                      IPGraph* graph, GroupIPNode* group = 0);

        virtual ~SessionIPNode();
    };

} // namespace IPCore

#endif // __IPCore__SessionIPNode__h__
