//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPGraph__HistogramIPNode__h__
#define __IPGraph__HistogramIPNode__h__
#include <IPCore/IPNode.h>

namespace IPCore
{

    class HistogramIPNode : public IPNode
    {
    public:
        HistogramIPNode(const std::string& name, IPGraph*,
                        GroupIPNode* group = 0);
        void init();
        virtual ~HistogramIPNode();
        virtual IPImage* evaluate(const Context&);
        virtual IPImageID* evaluateIdentifier(const Context&);
        virtual void propertyChanged(const Property*);

        void computeHistorgram(IPImage*);

        std::string m_currentID;
    };

} // namespace IPCore

#endif // __IPGraph__HistogramIPNode__h__
