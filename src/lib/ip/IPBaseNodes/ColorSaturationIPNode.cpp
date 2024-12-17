//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <IPBaseNodes/ColorSaturationIPNode.h>
#include <IPCore/Exception.h>
#include <IPCore/GroupIPNode.h>
#include <IPCore/ShaderCommon.h>
#include <TwkMath/Function.h>
#include <TwkMath/Vec3.h>
#include <TwkMath/Iostream.h>
#include <TwkMath/MatrixColor.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stl_ext/string_algo.h>

namespace IPCore
{
    using namespace std;
    using namespace TwkContainer;
    using namespace TwkMath;

    ColorSaturationIPNode::ColorSaturationIPNode(const std::string& name,
                                                 const NodeDefinition* def,
                                                 IPGraph* graph,
                                                 GroupIPNode* group)
        : IPNode(name, def, graph, group)
    {
        setMaxInputs(1);

        Property::Info* info = new Property::Info();
        info->setPersistent(false);

        m_active = declareProperty<IntProperty>("color.active", 1);
        m_colorSaturation =
            declareProperty<FloatProperty>("color.saturation", 1.0f);
    }

    ColorSaturationIPNode::~ColorSaturationIPNode() {}

    IPImage* ColorSaturationIPNode::evaluate(const Context& context)
    {

        int frame = context.frame;
        IPImage* head = IPNode::evaluate(context);
        if (!head)
            return IPImage::newNoImage(this, "No Input");
        IPImage* img = head;

        bool active = m_active ? m_active->front() : true;
        if (!active)
            return img;

        if (m_colorSaturation)
        {
            float s = m_colorSaturation->front();
            if (s != 1.0)
            {
                Mat44f M = Rec709FullRangeRGBToYUV8<float>();
                const float rw709 = M.m00;
                const float gw709 = M.m01;
                const float bw709 = M.m02;

                const float a = (1.0 - s) * rw709 + s;
                const float b = (1.0 - s) * rw709;
                const float c = (1.0 - s) * rw709;
                const float d = (1.0 - s) * gw709;
                const float e = (1.0 - s) * gw709 + s;
                const float f = (1.0 - s) * gw709;
                const float g = (1.0 - s) * bw709;
                const float h = (1.0 - s) * bw709;
                const float i = (1.0 - s) * bw709 + s;

                Mat44f S(a, d, g, 0, b, e, h, 0, c, f, i, 0, 0, 0, 0, 1);

                img->shaderExpr = Shader::newColorMatrix(img->shaderExpr, S);
            }
        }

        return img;
    }

} // namespace IPCore
