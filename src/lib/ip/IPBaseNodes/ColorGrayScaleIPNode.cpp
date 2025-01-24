//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <IPBaseNodes/ColorGrayScaleIPNode.h>
#include <IPCore/Exception.h>
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

    ColorGrayScaleIPNode::ColorGrayScaleIPNode(const std::string& name,
                                               const NodeDefinition* def,
                                               IPGraph* graph,
                                               GroupIPNode* group)
        : IPNode(name, def, graph, group)
    {
        setMaxInputs(1);
        m_active = declareProperty<IntProperty>("node.active", 0);
    }

    ColorGrayScaleIPNode::~ColorGrayScaleIPNode() {}

    IPImage* ColorGrayScaleIPNode::evaluate(const Context& context)
    {

        int frame = context.frame;
        IPImage* head = IPNode::evaluate(context);
        if (!head)
            return IPImage::newNoImage(this, "No Input");
        IPImage* img = head;

        bool active = m_active ? m_active->front() : true;

        if (!active)
            return img;

        img->shaderExpr = Shader::newColorLineartoGray(img->shaderExpr);

        return img;
    }

} // namespace IPCore
