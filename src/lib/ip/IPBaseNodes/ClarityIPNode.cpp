//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <IPBaseNodes/ClarityIPNode.h>
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

    ClarityIPNode::ClarityIPNode(const std::string& name,
                                 const NodeDefinition* def, IPGraph* graph,
                                 GroupIPNode* group)
        : IPNode(name, def, graph, group)
    {
        m_active = declareProperty<IntProperty>("node.active", 1);
        m_amount = declareProperty<FloatProperty>("node.amount", 0.0f);
        m_radius = declareProperty<FloatProperty>("node.radius", 20.0f);
    }

    ClarityIPNode::~ClarityIPNode() {}

    IPImage* ClarityIPNode::evaluate(const Context& context)
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
        if (!amount)
            return image;
        // amount is roughly -0.5 -> 1.5
        if (amount >= 0)
            amount /= 70;
        else
            amount /= 200;

        float r, s;
        r = m_radius->front();
        if (r <= 0)
            return image;

        IPImage* image2 = IPNode::evaluate(context);

        IPImage* downSized =
            new IPImage(this, IPImage::BlendRenderType, width / 2, height / 2,
                        1.0, IPImage::IntermediateBuffer);
        downSized->shaderExpr = Shader::newSourceRGBA(downSized);
        downSized->shaderExpr =
            newDownSample(downSized, downSized->shaderExpr, 2.0);
        downSized->appendChild(image);

        // both are reference copies of the m_gaussFB
        IPImage* gauss =
            Shader::applyFastGaussianFilter(this, downSized, (size_t)r);

        // use gaussian result and the original to create result
        IPImage* result = new IPImage(this, IPImage::MergeRenderType, width,
                                      height, 1.0, IPImage::IntermediateBuffer);

        IPImageVector images;
        IPImageSet modifiedImages;
        images.push_back(image2);
        images.push_back(gauss);

        convertBlendRenderTypeToIntermediate(images, modifiedImages);
        Shader::ExpressionVector inExpressions;
        balanceResourceUsage(IPNode::accumulate, images, modifiedImages, 8, 8,
                             81);
        assembleMergeExpressions(result, images, modifiedImages, false,
                                 inExpressions);

        result->mergeExpr =
            Shader::newFilterClarity(result, inExpressions, amount);
        result->shaderExpr = Shader::newSourceRGBA(result);
        result->appendChildren(images);

        return result;
    }

} // namespace IPCore
