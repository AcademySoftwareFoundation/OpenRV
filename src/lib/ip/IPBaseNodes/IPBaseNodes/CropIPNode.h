//******************************************************************************
//  Copyright (c) 2013 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPBaseNodes__CropIPNode__h__
#define __IPBaseNodes__CropIPNode__h__
#include <iostream>
#include <IPCore/IPNode.h>

namespace IPCore
{

    //
    //  CropIPNode
    //
    //  On hardware crop
    //

    class CropIPNode : public IPNode
    {
    public:
        struct CropSize
        {
            CropSize()
                : width(0)
                , height(0)
                , xorigin(0)
                , yorigin(0)
            {
            }

            CropSize(int a, int b, int c, int d)
                : width(a)
                , height(b)
                , xorigin(c)
                , yorigin(d)
            {
            }

            int width;
            int height;
            int xorigin;
            int yorigin;
        };

        CropIPNode(const std::string& name, const NodeDefinition* def,
                   IPGraph* graph, GroupIPNode* group = 0);

        virtual ~CropIPNode();
        virtual IPImage* evaluate(const Context&);
        virtual ImageStructureInfo imageStructureInfo(const Context&) const;

        virtual void propertyChanged(const Property*);

    private:
        IntProperty* m_active;
        IntProperty* m_manip;
        IntProperty* m_baseWidth;
        IntProperty* m_baseHeight;
        IntProperty* m_left;
        IntProperty* m_right;
        IntProperty* m_top;
        IntProperty* m_bottom;
    };

} // namespace IPCore

#endif // __IPBaseNodes__CropIPNode__h__
