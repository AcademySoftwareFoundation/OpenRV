//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPCore__UnsharpMaskIPNode__h__
#define __IPCore__UnsharpMaskIPNode__h__
#include <IPCore/IPNode.h>
#include <TwkFB/FrameBuffer.h>

namespace IPCore
{

    //
    //  Unsharp mask: a small radius operation. mostly looking for edges.
    //
    //

    class UnsharpMaskIPNode : public IPNode
    {
    public:
        UnsharpMaskIPNode(const std::string& name, const NodeDefinition* def,
                          IPGraph* graph, GroupIPNode* group = 0);

        virtual ~UnsharpMaskIPNode();

        virtual IPImage* evaluate(const Context&);

    private:
        IntProperty* m_active;
        FloatProperty* m_amount;
        FloatProperty* m_threshold;
        FloatProperty* m_unsharpRadius;
    };

} // namespace IPCore
#endif // __IPCore__UnsharpMaskIPNode__h__
