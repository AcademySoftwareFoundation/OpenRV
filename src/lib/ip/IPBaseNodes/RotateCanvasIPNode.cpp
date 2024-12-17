//
//  Copyright (c) 2013 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPBaseNodes/RotateCanvasIPNode.h>
#include <IPCore/ShaderCommon.h>
#include <IPCore/IPProperty.h>
#include <TwkMath/Vec2.h>

#include <math.h>

namespace IPCore
{
    using namespace std;
    using namespace TwkMath;

    RotateCanvasIPNode::RotateCanvasIPNode(const std::string& name,
                                           const NodeDefinition* def,
                                           IPGraph* graph, GroupIPNode* group)
        : IPNode(name, def, graph, group)
    {
        PropertyInfo* noSave = new PropertyInfo(PropertyInfo::NotPersistent);

        m_active = declareProperty<IntProperty>("node.active", 1);
        m_rotate = declareProperty<IntProperty>("node.rotate", 0);
    }

    RotateCanvasIPNode::~RotateCanvasIPNode() {}

    IPImage* RotateCanvasIPNode::evaluate(const Context& context)
    {
        if (inputs().empty())
            return IPImage::newNoImage(this, "No Input");

        const ImageStructureInfo info =
            inputs()[0]->imageStructureInfo(context);

        Context c = context;
        c.allowInteractiveResize = false;

        IPImage* image = IPNode::evaluate(c);
        if (!image)
            return IPImage::newNoImage(this, "No Input");
        if (image->isNoImage() || propertyValue<IntProperty>(m_active, 1) == 0)
            return image;

        const int rotate = propertyValue<IntProperty>(m_rotate, 0);
        if (rotate == 0)
            return image;

        // image size and texture matrix
        size_t fbWidth = image->fb ? image->fb->uncropWidth() : image->width;
        size_t fbHeight = image->fb ? image->fb->uncropHeight() : image->height;
        if (rotate == Rotate90)
        {
            float r = (float)fbWidth / fbHeight;
            image->textureMatrix.set(0, r, 0, -1.0f / r, 0, fbHeight, 0, 0,
                                     1.0f);
        }
        else if (rotate == Rotate180)
        {
            image->textureMatrix.set(-1.0f, 0, fbWidth, 0, -1.0f, fbHeight, 0,
                                     0, 1.0f);
        }
        else if (rotate == Rotate270)
        {
            float r = (float)fbWidth / fbHeight;
            image->textureMatrix.set(0, -r, fbWidth, 1.0f / r, 0, 0, 0, 0,
                                     1.0f);
        }
        if (rotate == Rotate90 || rotate == Rotate270)
        {
            size_t tmp = image->width;
            image->width = image->height;
            image->height = tmp;
        }

        IPImage* image2 =
            new IPImage(this, IPImage::BlendRenderType, image->width,
                        image->height, 1.0, IPImage::IntermediateBuffer);
        image2->children = image;
        image2->shaderExpr = Shader::newSourceRGBA(image2);
        return image2;
    }

    IPNode::ImageStructureInfo
    RotateCanvasIPNode::imageStructureInfo(const Context& context) const
    {
        Context c = context;
        c.allowInteractiveResize = false;

        if (inputs().empty())
        {
            return ImageStructureInfo(1, 1);
        }
        else
        {
            ImageStructureInfo info = inputs()[0]->imageStructureInfo(c);

            if (propertyValue<IntProperty>(m_active, 1) == 0)
            {
                return info;
            }
            else
            {
                const int rotate = propertyValue<IntProperty>(m_rotate, 0);
                if (rotate == Rotate90 || rotate == Rotate180)
                {
                    size_t t = info.width;
                    info.width = info.height;
                    info.height = t;
                }
                return info;
            }
        }
    }

    void RotateCanvasIPNode::propertyChanged(const Property* p)
    {
        IPNode::propertyChanged(p);

        if (p == m_rotate || p == m_active)
        {
            propagateImageStructureChange();
        }
    }

} // namespace IPCore
