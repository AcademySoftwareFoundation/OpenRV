//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPGraph__YCToRGBIPNode__h__
#define __IPGraph__YCToRGBIPNode__h__
#include <IPCore/IPNode.h>

namespace IPCore
{

    //
    //  YCToRGBIPNode
    //
    //  Convert from Y+Chrona to RGB
    //

    class YCToRGBIPNode : public IPNode
    {
    public:
        YCToRGBIPNode(const std::string& name, const NodeDefinition* def,
                      IPGraph* graph, GroupIPNode* group = 0);

        virtual ~YCToRGBIPNode();

        virtual IPImage* evaluate(const Context&);

        void setConversion(const std::string&);

    private:
        IntProperty* m_active;
        StringProperty* m_alphaTypeName;
        StringProperty* m_conversionName;
    };

} // namespace IPCore

#endif // __IPGraph__YCToRGBIPNode__h__
