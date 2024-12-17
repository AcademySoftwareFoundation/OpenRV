//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPCore__ColorSaturationIPNode__h__
#define __IPCore__ColorSaturationIPNode__h__
#include <IPCore/IPNode.h>
#include <IPCore/LUTIPNode.h>

namespace IPCore
{

    //
    // operating in gamma space
    //

    class ColorSaturationIPNode : public IPNode
    {
    public:
        ColorSaturationIPNode(const std::string& name,
                              const NodeDefinition* def, IPGraph* graph,
                              GroupIPNode* group = 0);

        virtual ~ColorSaturationIPNode();

        virtual IPImage* evaluate(const Context&);

    private:
        FloatProperty* m_colorSaturation;
        IntProperty* m_active;
    };

} // namespace IPCore
#endif // __IPCore__ColorSaturationIPNode__h__
