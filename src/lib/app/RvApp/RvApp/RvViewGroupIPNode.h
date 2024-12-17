//
//  Copyright (c) 2010 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __RvApp__RvViewGroupIPNode__h__
#define __RvApp__RvViewGroupIPNode__h__
#include <iostream>
#include <IPCore/ViewGroupIPNode.h>

namespace Rv
{

    using namespace IPCore;

    class RvViewGroupIPNode : public ViewGroupIPNode
    {
    public:
        RvViewGroupIPNode(const std::string& name, const NodeDefinition* def,
                          IPGraph* graph, GroupIPNode* group = 0)
            : ViewGroupIPNode(name, def, graph, group) {};

        virtual ~RvViewGroupIPNode();

        virtual IPImage* evaluate(const Context&);
    };

} // namespace Rv

#endif // __IPCore__ViewGroupIPNode__h__
