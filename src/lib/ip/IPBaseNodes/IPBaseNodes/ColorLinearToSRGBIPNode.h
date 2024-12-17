//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPCore__ColorLinearToSRGBIPNode__h__
#define __IPCore__ColorLinearToSRGBIPNode__h__
#include <IPCore/IPNode.h>

namespace IPCore
{

    class ColorLinearToSRGBIPNode : public IPNode
    {
    public:
        ColorLinearToSRGBIPNode(const std::string& name,
                                const NodeDefinition* def, IPGraph* graph,
                                GroupIPNode* group = 0);

        virtual ~ColorLinearToSRGBIPNode();

        virtual IPImage* evaluate(const Context&);

    private:
    };

} // namespace IPCore
#endif // __IPCore__ColorLinearToSRGBIPNode__h__
