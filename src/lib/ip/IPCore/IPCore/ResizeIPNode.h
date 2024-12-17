//******************************************************************************
//  Copyright (c) 2013 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPBaseNodes__ResizeIPNode__h__
#define __IPBaseNodes__ResizeIPNode__h__
#include <iostream>
#include <IPCore/IPNode.h>

namespace IPCore
{

    //
    //  ResizeIPNode
    //
    //  Handles both up and down sampling.
    //
    //  If node.useContext == 1 then it will use the incoming viewWidth
    //  and viewHeight.
    //

    class ResizeIPNode : public IPNode
    {
    public:
        ResizeIPNode(const std::string& name, const NodeDefinition* def,
                     IPGraph* graph, GroupIPNode* group = 0);

        virtual ~ResizeIPNode();
        virtual IPImage* evaluate(const Context&);
        virtual ImageStructureInfo imageStructureInfo(const Context&) const;

        bool isActive();
        void setActive(bool);

    private:
        IntProperty* m_upSampling;
        IntProperty* m_upQuality;
        IntProperty* m_outWidth;
        IntProperty* m_outHeight;
        IntProperty* m_useContext;
        IntProperty* m_active;
    };

} // namespace IPCore

#endif // __IPBaseNodes__ResizeIPNode__h__
