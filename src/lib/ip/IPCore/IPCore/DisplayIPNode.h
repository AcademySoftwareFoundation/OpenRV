//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPCore__DisplayIPNode__h__
#define __IPCore__DisplayIPNode__h__
#include <IPCore/IPNode.h>
#include <IPCore/LUTIPNode.h>

namespace IPCore
{

    /// Provides display gamma, brightness, and display LUTs to the renderer

    ///
    /// DisplayIPNode like ColorIPNode handles two LUTs -- a 3D LUT and a
    /// channel LUT. Display gamma, and relative brightness are also
    /// handled by this node.
    ///

    class DisplayIPNode : public LUTIPNode
    {
    public:
        //
        //  Types
        //
        friend class AssignDisplayState;

        //
        //  Constructors
        //

        DisplayIPNode(const std::string& name, const NodeDefinition* def,
                      IPGraph*, GroupIPNode* group = 0);
        virtual ~DisplayIPNode();

        virtual IPImage* evaluate(const Context&);

    private:
        StringProperty* m_channelOrder;
        IntProperty* m_channelFlood;
        IntProperty* m_premult;
        FloatProperty* m_gamma;
        IntProperty* m_srgb;
        IntProperty* m_rec709;
        FloatProperty* m_brightness;
        IntProperty* m_outOfRange;
        IntProperty* m_dither;
        IntProperty* m_ditherLast;
        IntProperty* m_active;
        IntProperty* m_chromaActive;
        IntProperty* m_adoptedNeutral;
        Vec2fProperty* m_white;
        Vec2fProperty* m_red;
        Vec2fProperty* m_green;
        Vec2fProperty* m_blue;
        Vec2fProperty* m_neutral;
        StringProperty* m_overrideColorspace;
    };

} // namespace IPCore

#endif // __IPCore__DisplayIPNode__h__
