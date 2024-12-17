//
//  Copyright (c) 2013 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCore/ResizeIPNode.h>
#include <IPCore/ShaderCommon.h>
#include <TwkMath/Vec2.h>

#include <math.h>

namespace IPCore
{
    using namespace std;
    using namespace TwkMath;

    ResizeIPNode::ResizeIPNode(const std::string& name,
                               const NodeDefinition* def, IPGraph* graph,
                               GroupIPNode* group)
        : IPNode(name, def, graph, group)
    {
        m_active = declareProperty<IntProperty>("node.active", 1);
        m_upQuality = declareProperty<IntProperty>("node.upsamplingQuality", 1);
        m_outWidth = declareProperty<IntProperty>("node.outWidth", 1280);
        m_outHeight = declareProperty<IntProperty>("node.outHeight", 720);
        m_useContext = declareProperty<IntProperty>("node.useContext", 0);
    }

    ResizeIPNode::~ResizeIPNode() {}

    bool ResizeIPNode::isActive() { return propertyValue(m_active, 0); }

    void ResizeIPNode::setActive(bool b) { setProperty(m_active, b ? 1 : 0); }

    IPImage* ResizeIPNode::evaluate(const Context& context)
    {
        if (propertyValue(m_active, 0) == 0)
        {
            return IPNode::evaluate(context);
        }

        Context c = context;

        int outWidth = propertyValue(m_outWidth, 1280);
        int outHeight = propertyValue(m_outHeight, 720);
        bool useContext = propertyValue(m_useContext, 0);

        if (context.allowInteractiveResize && useContext)
        {
            outWidth = context.viewWidth;
            outHeight = context.viewHeight;
        }

        c.allowInteractiveResize = false;

        IPImage* image = IPNode::evaluate(c);
        if (!image)
            return IPImage::newNoImage(this, "No Input");
        if (image->isNoImage())
            return image;

        if (outWidth <= 0 || outHeight <= 0)
            return image;

        const bool upSampling =
            outWidth > image->width && outHeight > image->height;
        if (upSampling)
        {
            // for now only do 2x upsampling
            const bool upQuality = propertyValue(m_upQuality, 0) == 1;
            float scale = min(2.0f, (float)min(outWidth / image->width,
                                               outHeight / image->height));
            if (scale < 2.0)
                return image;
            if (!image->shaderExpr && !image->mergeExpr)
            {
                if (!image->children || image->children->next)
                    return image; // should never happen

                IPImage* child = image->children;
                image->children = NULL;
                IPImage* dimage = image;
                delete dimage;
                image = child;
            }

            //
            //  NOTE: purposefully applying the horizontal and vertical
            //  filters one after the other without an intermediate
            //  because the use case is for resizing between [1,4]
            //  scale. In other words: the two filters will result in X^2
            //  samples being asked for in general instead of 2X
            //
            //  For upscaling >4 a different type of algorithm should be
            //  used.
            //

            IPImage* insertImage2 = new IPImage(
                this, IPImage::BlendRenderType, image->width * 2,
                image->height * 2, 1.0, IPImage::IntermediateBuffer);
            insertImage2->shaderExpr = Shader::newSourceRGBA(insertImage2);
            image->shaderExpr =
                newUpSampleHorizontal(image, image->shaderExpr, upQuality);
            image->shaderExpr =
                newUpSampleVertical(image, image->shaderExpr, upQuality);
            insertImage2->appendChild(image);

            return insertImage2;
        }
        else // down sampling
        {
            if (!image->shaderExpr && !image->mergeExpr)
            {
                if (!image->children || image->children->next)
                    return image; // should never happen

                IPImage* child = image->children;
                image->children = NULL;
                IPImage* dimage = image;
                delete dimage;
                image = child;
            }

            int scale = max(image->width / outWidth, image->height / outHeight);
            // NOTE: TODO: figure out how to manage the downsize when the scale
            // is big (like 20) for now, if scale is great than 4, we will just
            // do nothing
            if (scale <= 1)
                return image;

            if (scale > 4 && image->width >= 2048)
                scale = 4;

            const size_t newWidth = image->width / scale;
            const size_t newHeight = image->height / scale;

            IPImage* insertImage2 =
                new IPImage(this, IPImage::BlendRenderType, newWidth, newHeight,
                            1.0, IPImage::IntermediateBuffer);
            insertImage2->shaderExpr = Shader::newSourceRGBA(insertImage2);
            image->shaderExpr = newDownSample(image, image->shaderExpr, scale);
            insertImage2->appendChild(image);
            return insertImage2;
        }
        return image;
    }

    IPNode::ImageStructureInfo
    ResizeIPNode::imageStructureInfo(const Context& context) const
    {
        if (propertyValue(m_active, 0) == 0)
        {
            return IPNode::imageStructureInfo(context);
        }

        Context c = context;
        c.allowInteractiveResize = false;

        if (inputs().empty())
        {
            return ImageStructureInfo(1, 1);
        }
        else
        {
            int outWidth = propertyValue(m_outWidth, 1280);
            int outHeight = propertyValue(m_outHeight, 720);
            bool useContext = propertyValue(m_useContext, 0);

            if (context.allowInteractiveResize && useContext)
            {
                outWidth = context.viewWidth;
                outHeight = context.viewHeight;
            }

            ImageStructureInfo info(outWidth, outHeight);
            return info;
        }
    }

} // namespace IPCore
