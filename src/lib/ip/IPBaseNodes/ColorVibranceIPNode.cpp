//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <IPBaseNodes/ColorVibranceIPNode.h>
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

    ColorVibranceIPNode::ColorVibranceIPNode(const std::string& name,
                                             const NodeDefinition* def,
                                             IPGraph* graph, GroupIPNode* group)
        : IPNode(name, def, graph, group)
    {
        setMaxInputs(1);

        Property::Info* info = new Property::Info();
        info->setPersistent(false);

        m_active = declareProperty<IntProperty>("color.active", 1);
        m_ColorVibrance =
            declareProperty<FloatProperty>("color.vibrance", 0.0f);
    }

    ColorVibranceIPNode::~ColorVibranceIPNode() {}

    IPImage* ColorVibranceIPNode::evaluate(const Context& context)
    {

        int frame = context.frame;
        IPImage* head = IPNode::evaluate(context);
        if (!head)
            return IPImage::newNoImage(this, "No Input");
        IPImage* img = head;

        bool active = m_active ? m_active->front() : true;
        if (!active)
            return img;

        if (m_ColorVibrance)
        {
            float s = m_ColorVibrance->front();
            if (s != 0.0)
            {
                // s will be [0, 2.25] where s = 1 means identity
                if (s < 0.0)
                    s = 1.0 + s / 100;
                else
                    s = s / 80 + 1.0;

                Mat44f M = Rec709FullRangeRGBToYUV8<float>();
                const float rw709 = M.m00;
                const float gw709 = M.m01;
                const float bw709 = M.m02;

                Vec3f rgb709(rw709, gw709, bw709);
                img->shaderExpr =
                    Shader::newColorVibrance(img->shaderExpr, rgb709, s);
            }
        }

        return img;
    }

} // namespace IPCore
