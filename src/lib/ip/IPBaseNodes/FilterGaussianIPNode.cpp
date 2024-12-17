//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <IPBaseNodes/FilterGaussianIPNode.h>
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
    using namespace TwkFB;

    FilterGaussianIPNode::FilterGaussianIPNode(const std::string& name,
                                               const NodeDefinition* def,
                                               IPGraph* graph,
                                               GroupIPNode* group)
        : IPNode(name, def, graph, group)
    {
        m_sigma = declareProperty<FloatProperty>("node.sigma",
                                                 1.0 / 33.0); // roughly r^2 / 3
        m_radius = declareProperty<FloatProperty>("node.radius", 10.0f);
    }

    FilterGaussianIPNode::~FilterGaussianIPNode() {}

    IPImage* FilterGaussianIPNode::evaluate(const Context& context)
    {

        int frame = context.frame;
        IPImage* image = IPNode::evaluate(context);
        if (!image)
            return IPImage::newNoImage(this, "No Input");

        float radius = m_radius ? m_radius->front() : 10;
        float sigma = m_sigma ? m_sigma->front() : 1.0 / 33.0;
        size_t width = image->width;
        size_t height = image->height;

        IPImage* gaussHorizontal = NULL;
        if (radius <= 10)
            gaussHorizontal =
                Shader::applyGaussianFilter(this, image, (size_t)radius, sigma);
        else
            gaussHorizontal =
                Shader::applyFastGaussianFilter(this, image, (size_t)radius);

        return gaussHorizontal;
    }

} // namespace IPCore
