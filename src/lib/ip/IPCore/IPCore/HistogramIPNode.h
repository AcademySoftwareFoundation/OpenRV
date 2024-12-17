//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPCore__HistogramIPNode__h__
#define __IPCore__HistogramIPNode__h__
#include <IPCore/IPNode.h>
#include <TwkFB/FrameBuffer.h>

namespace IPCore
{

    //
    //  Histogram 256 bins. above 1 is counted as 1, below 0 is counted as 0
    //  Use openCL for now
    //

    class HistogramIPNode : public IPNode
    {
    public:
        HistogramIPNode(const std::string& name, const NodeDefinition* def,
                        IPGraph* graph, GroupIPNode* group = 0);

        virtual ~HistogramIPNode();

        virtual IPImage* evaluate(const Context&);

    private:
        IntProperty* m_active;
        IntProperty* m_height;
    };

} // namespace IPCore
#endif // __IPCore__HistogramIPNode__h__
