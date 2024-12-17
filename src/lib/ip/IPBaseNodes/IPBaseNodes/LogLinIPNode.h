//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPGraph__LogLinIPNode__h__
#define __IPGraph__LogLinIPNode__h__
#include <IPCore/IPNode.h>

namespace IPCore
{

    class LogLinIPNode : public IPNode
    {
    public:
        LogLinIPNode(const std::string& name);
        void init();
        virtual ~LogLinIPNode();
        virtual void evaluate(IPState&, int frame);
    };

} // namespace IPCore

#endif // __IPGraph__LogLinIPNode__h__
