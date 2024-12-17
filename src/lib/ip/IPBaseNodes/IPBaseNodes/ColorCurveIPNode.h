//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPCore__ColorCurveIPNode__h__
#define __IPCore__ColorCurveIPNode__h__
#include <IPCore/IPNode.h>
#include <IPCore/LUTIPNode.h>

namespace IPCore
{

    //
    // this can do contrast
    // operate in gamma space
    //

    class ColorCurveIPNode : public IPNode
    {
    public:
        ColorCurveIPNode(const std::string& name, const NodeDefinition* def,
                         IPGraph* graph, GroupIPNode* group = 0);

        virtual ~ColorCurveIPNode();

        virtual IPImage* evaluate(const Context&);

    private:
        IntProperty* m_active;
        FloatProperty* m_colorContrast;
    };

} // namespace IPCore
#endif // __IPCore__ColorCurveIPNode__h__
