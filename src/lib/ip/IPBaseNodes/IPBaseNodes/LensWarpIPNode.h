//******************************************************************************
//  Copyright (c) 2013 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPBaseNodes__LensWarpIPNode__h__
#define __IPBaseNodes__LensWarpIPNode__h__
#include <iostream>
#include <IPCore/IPNode.h>

namespace IPCore
{

    //
    //  LensWarpIPNode
    //
    //  Handles both warp and unwarp cases. Also handles pixel aspect
    //  ratio.
    //

    class LensWarpIPNode : public IPNode
    {
    public:
        LensWarpIPNode(const std::string& name, const NodeDefinition* def,
                       IPGraph* graph, GroupIPNode* group = 0);

        virtual ~LensWarpIPNode();
        virtual IPImage* evaluate(const Context&);
        virtual ImageStructureInfo imageStructureInfo(const Context&) const;
        virtual void readCompleted(const std::string&, unsigned int);
        virtual void propertyChanged(const Property*);

        float pixelAspect() const;
        bool active() const;

    private:
        IntProperty* m_activeProperty;

        FloatProperty* m_pixelAspect;

        StringProperty* m_model;

        // Radial distortion coefficients
        FloatProperty* m_k1;
        FloatProperty* m_k2;
        FloatProperty* m_k3;
        FloatProperty* m_d;
        // Tangential distortion coefficients
        FloatProperty* m_p1;
        FloatProperty* m_p2;
        // Distortion center and offset
        Vec2fProperty* m_center;
        Vec2fProperty* m_offset;
        // Focallength in x and y
        FloatProperty* m_fx;
        FloatProperty* m_fy;
        // Crop Ratio in x and y
        FloatProperty* m_cropRatioX;
        FloatProperty* m_cropRatioY;

        // 3de4 anamorphic params
        FloatProperty* m_cx02;
        FloatProperty* m_cy02;
        FloatProperty* m_cx22;
        FloatProperty* m_cy22;

        FloatProperty* m_cx04;
        FloatProperty* m_cy04;
        FloatProperty* m_cx24;
        FloatProperty* m_cy24;
        FloatProperty* m_cx44;
        FloatProperty* m_cy44;

        FloatProperty* m_cx06;
        FloatProperty* m_cy06;
        FloatProperty* m_cx26;
        FloatProperty* m_cy26;
        FloatProperty* m_cx46;
        FloatProperty* m_cy46;
        FloatProperty* m_cx66;
        FloatProperty* m_cy66;
    };

} // namespace IPCore

#endif // __IPBaseNodes__LensWarpIPNode__h__
