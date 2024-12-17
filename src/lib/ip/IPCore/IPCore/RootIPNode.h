//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCore__RootIPNode__h__
#define __IPCore__RootIPNode__h__
#include <iostream>
#include <IPCore/IPNode.h>

namespace IPCore
{

    //
    //  class RootIPNode
    //
    //  A root node holds all of the final render targets as
    //  children. There is only one root node in an IPGraph
    //
    //  NOTE: only the IPGraph can create and destory the RootIPNode
    //
    //  NOTE: do not confuse this with the SessionIPNode which holds
    //  global session information. The RootIPNode is only for aggregating
    //  evaluation.
    //

    class RootIPNode : public IPNode
    {
    public:
        RootIPNode(const std::string&, const NodeDefinition*, IPGraph*,
                   GroupIPNode*);

        virtual ~RootIPNode();

        virtual IPImage* evaluate(const Context&);
        virtual IPImageID* evaluateIdentifier(const Context&);
    };

} // namespace IPCore

#endif // __IPCore__RootIPNode__h__
