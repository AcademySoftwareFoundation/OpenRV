//
//  Copyright (c) 2013 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPBaseNodes/CropIPNode.h>
#include <IPCore/ShaderCommon.h>
#include <IPCore/IPProperty.h>
#include <TwkMath/Vec2.h>

#include <math.h>

namespace IPCore
{
    using namespace std;
    using namespace TwkMath;

    CropIPNode::CropIPNode(const std::string& name, const NodeDefinition* def,
                           IPGraph* graph, GroupIPNode* group)
        : IPNode(name, def, graph, group)
    {
        PropertyInfo* noSave = new PropertyInfo(PropertyInfo::NotPersistent);

        m_active = declareProperty<IntProperty>("node.active", 1);
        m_manip = declareProperty<IntProperty>("node.manip", 0, noSave);
        m_baseWidth = declareProperty<IntProperty>("crop.baseWidth", 1280);
        m_baseHeight = declareProperty<IntProperty>("crop.baseHeight", 720);
        m_left = declareProperty<IntProperty>("crop.left", 0);
        m_right = declareProperty<IntProperty>("crop.right", 0);
        m_bottom = declareProperty<IntProperty>("crop.bottom", 0);
        m_top = declareProperty<IntProperty>("crop.top", 0);
    }

    CropIPNode::~CropIPNode() {}

    IPImage* CropIPNode::evaluate(const Context& context)
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

        //
        //  these numbers are w.r.t. top left
        //

        const bool manip = propertyValue<IntProperty>(m_manip, 1) == 1;
        const int left = manip ? 0 : propertyValue<IntProperty>(m_left, 0);
        const int right = manip ? 0 : propertyValue<IntProperty>(m_right, 0);
        const int bottom = manip ? 0 : propertyValue<IntProperty>(m_bottom, 0);
        const int top = manip ? 0 : propertyValue<IntProperty>(m_top, 0);

        CropSize cropSize =
            CropSize(image->width - left - right, image->height - top - bottom,
                     left, bottom);

        if (((cropSize.width < 0 || cropSize.height < 0
              || (cropSize.width == image->width
                  && cropSize.height == image->height)))
            && !manip)
        {
            return image;
        }

        size_t newWidth = image->width;
        size_t newHeight = image->height;

        IPImage* image2 =
            new IPImage(this, IPImage::BlendRenderType, newWidth, newHeight,
                        1.0, IPImage::IntermediateBuffer);
        image2->children = image;
        image2->shaderExpr = Shader::newSourceRGBA(image2);
        image2->isCropped = true;
        image2->cropStartX = cropSize.xorigin;
        image2->cropStartY = cropSize.yorigin;
        image2->cropEndX = cropSize.xorigin + cropSize.width;
        image2->cropEndY = cropSize.yorigin + cropSize.height;

        size_t newWidth2 = cropSize.width;
        size_t newHeight2 = cropSize.height;

        IPImage* root =
            new IPImage(this, IPImage::BlendRenderType, newWidth2, newHeight2,
                        1.0, IPImage::IntermediateBuffer);

        root->shaderExpr = Shader::newSourceRGBA(root);
        root->appendChild(image2);
        return root;
    }

    IPNode::ImageStructureInfo
    CropIPNode::imageStructureInfo(const Context& context) const
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
                const bool manip = propertyValue<IntProperty>(m_manip, 1) == 1;
                const int left =
                    manip ? 0 : propertyValue<IntProperty>(m_left, 0);
                const int right =
                    manip ? 0 : propertyValue<IntProperty>(m_right, 0);
                const int bottom =
                    manip ? 0 : propertyValue<IntProperty>(m_bottom, 0);
                const int top =
                    manip ? 0 : propertyValue<IntProperty>(m_top, 0);

                info.width = info.width - left - right;
                info.height = info.height - top - bottom;
                return info;
            }
        }
    }

    void CropIPNode::propertyChanged(const Property* p)
    {
        IPNode::propertyChanged(p);

        if (p == m_manip || p == m_left || p == m_right || p == m_top
            || p == m_bottom || p == m_active)
        {
            propagateImageStructureChange();
        }
    }

} // namespace IPCore
