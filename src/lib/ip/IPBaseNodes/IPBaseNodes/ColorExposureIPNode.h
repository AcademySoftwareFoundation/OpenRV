//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPCore__ColorExposureIPNode__h__
#define __IPCore__ColorExposureIPNode__h__
#include <IPCore/IPNode.h>

namespace IPCore
{

    class ColorExposureIPNode : public IPNode
    {
    public:
        ColorExposureIPNode(const std::string& name, const NodeDefinition* def,
                            IPGraph* graph, GroupIPNode* group = 0);

        virtual ~ColorExposureIPNode();

        virtual IPImage* evaluate(const Context&);

    private:
        IntProperty* m_active;
        FloatProperty* m_colorExposure;
    };

} // namespace IPCore
#endif // __IPCore__ColorExposureIPNode__h__
