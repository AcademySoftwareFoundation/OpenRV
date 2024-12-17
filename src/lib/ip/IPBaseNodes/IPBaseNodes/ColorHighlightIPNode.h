//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPCore__ColorHighlightIPNode__h__
#define __IPCore__ColorHighlightIPNode__h__
#include <IPCore/IPNode.h>
#include <IPCore/LUTIPNode.h>

namespace IPCore
{

    //
    // operate in gamma space
    //

    class ColorHighlightIPNode : public IPNode
    {
    public:
        ColorHighlightIPNode(const std::string& name, const NodeDefinition* def,
                             IPGraph* graph, GroupIPNode* group = 0);

        virtual ~ColorHighlightIPNode();

        virtual IPImage* evaluate(const Context&);

    private:
        IntProperty* m_active;
        FloatProperty* m_colorHighlight;
    };

} // namespace IPCore
#endif // __IPCore__ColorHighlightIPNode__h__
