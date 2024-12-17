//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPCore__ColorShadowIPNode__h__
#define __IPCore__ColorShadowIPNode__h__
#include <IPCore/IPNode.h>
#include <IPCore/LUTIPNode.h>

namespace IPCore
{

    //
    // operate in gamma space
    //

    class ColorShadowIPNode : public IPNode
    {
    public:
        ColorShadowIPNode(const std::string& name, const NodeDefinition* def,
                          IPGraph* graph, GroupIPNode* group = 0);

        virtual ~ColorShadowIPNode();

        virtual IPImage* evaluate(const Context&);

    private:
        IntProperty* m_active;
        FloatProperty* m_colorShadow;
    };

} // namespace IPCore
#endif // __IPCore__ColorShadowIPNode__h__
