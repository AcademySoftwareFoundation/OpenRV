//******************************************************************************
//  Copyright (c) 2013 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPBaseNodes__RotateCanvasIPNode__h__
#define __IPBaseNodes__RotateCanvasIPNode__h__
#include <iostream>
#include <IPCore/IPNode.h>

namespace IPCore
{

    class RotateCanvasIPNode : public IPNode
    {
    public:
        enum RotateMode
        {
            NoRotate,
            Rotate90,
            Rotate180,
            Rotate270
        };

        RotateCanvasIPNode(const std::string& name, const NodeDefinition* def,
                           IPGraph* graph, GroupIPNode* group = 0);

        virtual ~RotateCanvasIPNode();
        virtual IPImage* evaluate(const Context&);
        virtual ImageStructureInfo imageStructureInfo(const Context&) const;

        virtual void propertyChanged(const Property*);

    private:
        IntProperty* m_active;
        IntProperty* m_rotate;
    };

} // namespace IPCore

#endif // __IPBaseNodes__RotateCanvasIPNode__h__
