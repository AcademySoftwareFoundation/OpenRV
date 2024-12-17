//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPCore__NoiseReductionIPNode__h__
#define __IPCore__NoiseReductionIPNode__h__
#include <IPCore/IPNode.h>
#include <TwkFB/FrameBuffer.h>

namespace IPCore
{

    //
    //  Noise Reduction
    //

    class NoiseReductionIPNode : public IPNode
    {
    public:
        typedef std::vector<FrameBuffer*> FrameBufferVector;

        NoiseReductionIPNode(const std::string& name, const NodeDefinition* def,
                             IPGraph* graph, GroupIPNode* group = 0);

        virtual ~NoiseReductionIPNode();

        virtual IPImage* evaluate(const Context&);

    private:
        IntProperty* m_active;
        FloatProperty* m_amount;
        FloatProperty* m_threshold;
        FloatProperty* m_radius;
    };

} // namespace IPCore
#endif // __IPCore__NoiseReductionIPNode__h__
