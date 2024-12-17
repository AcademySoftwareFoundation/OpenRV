//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPCore__FilterGaussianIPNode__h__
#define __IPCore__FilterGaussianIPNode__h__
#include <IPCore/IPNode.h>
#include <TwkFB/FrameBuffer.h>

namespace IPCore
{

    // separable gaussian filter
    class FilterGaussianIPNode : public IPNode
    {
    public:
        FilterGaussianIPNode(const std::string& name, const NodeDefinition* def,
                             IPGraph* graph, GroupIPNode* group = 0);

        virtual ~FilterGaussianIPNode();

        virtual IPImage* evaluate(const Context&);

    private:
        FloatProperty* m_sigma; // this is 2 * sigma * sigma in the equation
        FloatProperty* m_radius;
    };

} // namespace IPCore
#endif // __IPCore__FilterGaussianIPNode__h__
