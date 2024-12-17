//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPCore__ColorGrayScaleIPNode__h__
#define __IPCore__ColorGrayScaleIPNode__h__
#include <IPCore/IPNode.h>

namespace IPCore
{

    class ColorGrayScaleIPNode : public IPNode
    {
    public:
        ColorGrayScaleIPNode(const std::string& name, const NodeDefinition* def,
                             IPGraph* graph, GroupIPNode* group = 0);

        virtual ~ColorGrayScaleIPNode();

        virtual IPImage* evaluate(const Context&);

    private:
        IntProperty* m_active;
    };

} // namespace IPCore
#endif // __IPCore__ColorGrayScaleIPNode__h__
