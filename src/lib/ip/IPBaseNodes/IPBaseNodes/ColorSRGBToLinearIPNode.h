//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPCore__ColorSRGBToLinearIPNode__h__
#define __IPCore__ColorSRGBToLinearIPNode__h__
#include <IPCore/IPNode.h>

namespace IPCore
{

    class ColorSRGBToLinearIPNode : public IPNode
    {
    public:
        ColorSRGBToLinearIPNode(const std::string& name,
                                const NodeDefinition* def, IPGraph* graph,
                                GroupIPNode* group = 0);

        virtual ~ColorSRGBToLinearIPNode();

        virtual IPImage* evaluate(const Context&);

    private:
    };

} // namespace IPCore
#endif // __IPCore__ColorSRGBToLinearIPNode__h__
