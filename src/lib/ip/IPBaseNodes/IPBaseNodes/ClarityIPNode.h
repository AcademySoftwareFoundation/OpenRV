//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPCore__ClarityIPNode__h__
#define __IPCore__ClarityIPNode__h__
#include <IPCore/IPNode.h>
#include <TwkFB/FrameBuffer.h>

namespace IPCore
{

    //
    //  clarity is a midtone local contrast adjustment
    //  with a relatively bigger (50 pixel) radius on the highpass filter
    //  inspired by Mac Holbert's Midtone Contrast
    //

    class ClarityIPNode : public IPNode
    {
    public:
        ClarityIPNode(const std::string& name, const NodeDefinition* def,
                      IPGraph* graph, GroupIPNode* group = 0);

        virtual ~ClarityIPNode();

        virtual IPImage* evaluate(const Context&);

    private:
        IntProperty* m_active;
        FloatProperty* m_amount;
        FloatProperty* m_radius;
    };

} // namespace IPCore
#endif // __IPCore__ClarityIPNode__h__
