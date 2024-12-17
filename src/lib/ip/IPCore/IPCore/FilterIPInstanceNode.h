//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCore__FilterIPInstanceNode__h__
#define __IPCore__FilterIPInstanceNode__h__
#include <iostream>
#include <IPCore/IPInstanceNode.h>

namespace IPCore
{

    class FilterIPInstanceNode : public IPInstanceNode
    {
    public:
        FilterIPInstanceNode(const std::string& name, const NodeDefinition* def,
                             IPGraph* graph, GroupIPNode* group = 0);

        virtual ~FilterIPInstanceNode();

        virtual IPImage* evaluate(const Context&);
    };

} // namespace IPCore

#endif // __IPCore__FilterIPInstanceNode__h__
