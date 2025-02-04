//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPCore__Transform2DIPNode__h__
#define __IPCore__Transform2DIPNode__h__
#include <IPCore/IPNode.h>
#include <TwkMovie/Movie.h>
#include <algorithm>

namespace IPCore
{

    //
    //  class Transform2DIPNode
    //
    //  Converts inputs into formats that can be used by the ImageRenderer
    //

    class Transform2DIPNode : public IPNode
    {
    public:
        Transform2DIPNode(const std::string& name, const NodeDefinition* def,
                          IPGraph* graph, GroupIPNode* group = 0);

        virtual ~Transform2DIPNode();

        virtual IPImage* evaluate(const Context&);
        virtual IPImageID* evaluateIdentifier(const Context&);
        virtual Matrix localMatrix(const Context&) const;

        void setFlip(bool);
        void setFlop(bool);

        void setAdaptiveResampling(bool b) { m_adaptiveResampling = b; }

    protected:
        void init();

    private:
        bool m_adaptiveResampling;
        Component* m_tag;
        IntProperty* m_flip;
        IntProperty* m_flop;
        FloatProperty* m_rotate;
        Vec2fProperty* m_translate;
        Vec2fProperty* m_scale;
        IntProperty* m_active;
        FloatProperty* m_visibleBox;
        FloatProperty* m_leftVisibleBox;
        FloatProperty* m_rightVisibleBox;
        FloatProperty* m_topVisibleBox;
        FloatProperty* m_bottomVisibleBox;
    };

} // namespace IPCore

#endif // __IPCore__Transform2DIPNode__h__
