//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <IPBaseNodes/ColorExposureIPNode.h>
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

#define NO_FLOAT_3D_TEXTURES

    ColorExposureIPNode::ColorExposureIPNode(const std::string& name,
                                             const NodeDefinition* def,
                                             IPGraph* graph, GroupIPNode* group)
        : IPNode(name, def, graph, group)
    {
        setMaxInputs(1);

        Property::Info* info = new Property::Info();
        info->setPersistent(false);

        m_active = declareProperty<IntProperty>("color.active", 1);
        m_colorExposure =
            declareProperty<FloatProperty>("color.exposure", 0.0f);
    }

    ColorExposureIPNode::~ColorExposureIPNode() {}

    IPImage* ColorExposureIPNode::evaluate(const Context& context)
    {

        int frame = context.frame;
        IPImage* head = IPNode::evaluate(context);
        if (!head)
            return IPImage::newNoImage(this, "No Input");
        IPImage* img = head;

        bool active = m_active ? m_active->front() : true;

        if (!active)
            return img;

        if (FloatProperty* exposure = m_colorExposure)
        {
            if (exposure->front() != 0.0f)
            {
                float e = exposure->front();
                float e0 = ::pow(2.0, double(e));
                Mat44f E;
                E.makeScale(Vec3f(e0, e0, e0));
                img->shaderExpr = Shader::newColorMatrix(img->shaderExpr, E);
            }
        }

        return img;
    }

} // namespace IPCore
