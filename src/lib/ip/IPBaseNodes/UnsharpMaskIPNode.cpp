//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <IPBaseNodes/UnsharpMaskIPNode.h>
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

    UnsharpMaskIPNode::UnsharpMaskIPNode(const std::string& name,
                                         const NodeDefinition* def,
                                         IPGraph* graph, GroupIPNode* group)
        : IPNode(name, def, graph, group)
    {
        m_active = declareProperty<IntProperty>("node.active", 1);
        m_amount = declareProperty<FloatProperty>("node.amount", 1.0f);
        m_threshold = declareProperty<FloatProperty>("node.threshold", 5.0f);
        m_unsharpRadius =
            declareProperty<FloatProperty>("node.unsharpRadius", 5.0f);
    }

    UnsharpMaskIPNode::~UnsharpMaskIPNode() {}

    IPImage* UnsharpMaskIPNode::evaluate(const Context& context)
    {

        int frame = context.frame;
        IPImage* image = IPNode::evaluate(context);
        if (!image)
            return IPImage::newNoImage(this, "No Input");
        if (m_active && !m_active->front())
            return image;

        size_t width = image->width;
        size_t height = image->height;
        float amount = m_amount ? m_amount->front() : 2;
        if (amount <= 1.0)
            return image;

        float threshold = m_threshold ? m_threshold->front() : 5;
        threshold /= 255;

        float unsharpS, unsharpR;
        if (FloatProperty* unsharpRadius = m_unsharpRadius)
        {
            unsharpR = unsharpRadius->front();
            if (unsharpR <= 0)
                return image;
        }

        IPImage* image2 = IPNode::evaluate(context);

        IPImage* gauss = NULL;

        if (unsharpR <= 5)
            gauss = Shader::applyGaussianFilter(this, image, (size_t)unsharpR);
        else
            gauss =
                Shader::applyFastGaussianFilter(this, image, (size_t)unsharpR);

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

        result->mergeExpr = Shader::newFilterUnsharpMask(result, inExpressions,
                                                         amount, threshold);
        result->shaderExpr = Shader::newSourceRGBA(result);
        result->appendChildren(images);

        return result;
    }

} // namespace IPCore
