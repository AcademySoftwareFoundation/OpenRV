//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <IPCore/HistogramIPNode.h>
#include <IPCore/Exception.h>
#include <IPCore/ShaderCommon.h>
#include <TwkMath/Function.h>
#include <TwkMath/Vec3.h>
#include <TwkMath/Iostream.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stl_ext/string_algo.h>
#include <TwkFB/FrameBuffer.h>

namespace IPCore
{
    using namespace std;
    using namespace TwkContainer;
    using namespace TwkMath;
    using namespace TwkFB;

    HistogramIPNode::HistogramIPNode(const std::string& name,
                                     const NodeDefinition* def, IPGraph* graph,
                                     GroupIPNode* group)
        : IPNode(name, def, graph, group)
    {
        m_active = declareProperty<IntProperty>("node.active", 1);
        m_height = declareProperty<IntProperty>("node.height", 100);
    }

    HistogramIPNode::~HistogramIPNode() {}

    IPImage* HistogramIPNode::evaluate(const Context& context)
    {

        int frame = context.frame;
        IPImage* image = IPNode::evaluate(context);
        if (!image)
            return IPImage::newNoImage(this, "No Input");
        if (m_active && !m_active->front())
            return image;

        IPImage* newImage = image;
        if (!image->shaderExpr && !image->mergeExpr)
        {
            assert(image->children && !image->children->next);
            newImage = image->children;
            image->children = NULL;
            delete image;
            image = NULL;
        }
        newImage->shaderExpr =
            Shader::newColorLinearToSRGB(newImage->shaderExpr);

        IPImage* image2 = NULL;
        size_t scale = max(newImage->width / 300, newImage->height / 300);
        if (scale > 1)
        {
            const size_t newWidth = newImage->width / scale;
            const size_t newHeight = newImage->height / scale;

            IPImage* smallImage =
                new IPImage(this, IPImage::BlendRenderType, newWidth, newHeight,
                            1.0, IPImage::IntermediateBuffer);
            smallImage->shaderExpr = Shader::newSourceRGBA(smallImage);
            smallImage->appendChild(newImage);
            image2 = smallImage;
        }
        else
        {
            // this is the case where the image has a fb, and cannot be
            // converted by convertBlend func
            IPImage* insertImage =
                new IPImage(this, IPImage::BlendRenderType, newImage->width,
                            newImage->height, 1.0, IPImage::IntermediateBuffer);
            insertImage->shaderExpr = Shader::newSourceRGBA(insertImage);
            insertImage->appendChild(newImage);
            image2 = insertImage;
        }

        IPImage* histo =
            new IPImage(this, IPImage::BlendRenderType, 256, 1, 1.0,
                        IPImage::DataBuffer, IPImage::FloatDataType);
        histo->setHistogram(true);
        histo->appendChild(image2);
        histo->shaderExpr = Shader::newSourceRGBA(histo);

        // this image will have a shaderExpr that uses the histogram result to
        // render a histogram
        size_t width = 256;
        size_t height = m_height ? m_height->front() : 100;

        IPImage* result = new IPImage(this, IPImage::MergeRenderType, width,
                                      height, 1.0, IPImage::IntermediateBuffer);

        IPImageVector images;
        IPImageSet modifiedImages;
        images.push_back(histo);
        convertBlendRenderTypeToIntermediate(images, modifiedImages);
        Shader::ExpressionVector inExpressions;
        assembleMergeExpressions(result, images, modifiedImages, false,
                                 inExpressions);

        result->shaderExpr = Shader::newSourceRGBA(result);
        result->mergeExpr = Shader::newHistogram(result, inExpressions);
        result->appendChild(histo);

        return result;
    }

} // namespace IPCore
