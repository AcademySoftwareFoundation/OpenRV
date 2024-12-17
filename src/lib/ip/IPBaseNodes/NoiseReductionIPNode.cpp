//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <IPBaseNodes/NoiseReductionIPNode.h>
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

    NoiseReductionIPNode::NoiseReductionIPNode(const std::string& name,
                                               const NodeDefinition* def,
                                               IPGraph* graph,
                                               GroupIPNode* group)
        : IPNode(name, def, graph, group)
    {
        m_active = declareProperty<IntProperty>("node.active", 1);
        m_amount = declareProperty<FloatProperty>("node.amount", 0.0f);
        m_radius = declareProperty<FloatProperty>("node.radius", 0.0f);
        m_threshold = declareProperty<FloatProperty>("node.threshold", 5.0f);
    }

    NoiseReductionIPNode::~NoiseReductionIPNode() {}

    IPImage* NoiseReductionIPNode::evaluate(const Context& context)
    {

        int frame = context.frame;
        IPImage* image = IPNode::evaluate(context);
        if (!image)
            return IPImage::newNoImage(this, "No Input");
        if (m_active && !m_active->front())
            return image;

        size_t width = image->width;
        size_t height = image->height;
        float amount = m_amount ? m_amount->front() : 0;
        amount /= 100;
        amount = min(1.0f, max(0.0f, amount));
        // from ui, 0 means no changes, 1 means max change
        amount = 1 - amount; // now 1 means no changes, 0 means max change
        if (amount == 1)
            return image;

        float threshold = m_threshold ? m_threshold->front() : 5.0;
        threshold /= 255;
        float s, r;
        if (FloatProperty* radius = m_radius)
        {
            r = radius->front();
            if (r <= 0)
                return image;
        }

        IPImage* image2 = IPNode::evaluate(context);

        IPImage* gaussHorizontal = NULL;
        if (r <= 5)
            gaussHorizontal =
                Shader::applyGaussianFilter(this, image, (size_t)r);
        else
            gaussHorizontal =
                Shader::applyFastGaussianFilter(this, image, (size_t)r);

        // use gaussian result and the original to create result
        IPImage* result = new IPImage(this, IPImage::MergeRenderType, width,
                                      height, 1.0, IPImage::IntermediateBuffer);

        IPImageVector images;
        IPImageSet modifiedImages;
        images.push_back(image2);
        images.push_back(gaussHorizontal);

        convertBlendRenderTypeToIntermediate(images, modifiedImages);
        Shader::ExpressionVector inExpressions;
        balanceResourceUsage(IPNode::accumulate, images, modifiedImages, 8, 8,
                             81);
        assembleMergeExpressions(result, images, modifiedImages, false,
                                 inExpressions);

        result->mergeExpr = Shader::newFilterNoiseReduction(
            result, inExpressions, amount, threshold);
        result->shaderExpr = Shader::newSourceRGBA(result);
        result->appendChildren(images);

        return result;
    }

} // namespace IPCore
