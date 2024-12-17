//
//  Copyright (c) 2013 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPBaseNodes/LensWarpIPNode.h>
#include <IPCore/ShaderCommon.h>
#include <TwkMath/Vec2.h>

#include <math.h>

namespace IPCore
{
    using namespace std;
    using namespace TwkMath;

    LensWarpIPNode::LensWarpIPNode(const std::string& name,
                                   const NodeDefinition* def, IPGraph* graph,
                                   GroupIPNode* group)
        : IPNode(name, def, graph, group)
    {
        m_activeProperty = declareProperty<IntProperty>("node.active", 1);

        m_pixelAspect =
            declareProperty<FloatProperty>("warp.pixelAspectRatio", 0.0f);

        // Options are: "brown", "opencv", "pfbarrel",
        // "nuke", "tde4_ldp_anamorphic_deg_6", "adobe"
        m_model = declareProperty<StringProperty>("warp.model", "brown");

        /* Brown, OpenCV, PFBarrel, Adobe, Nuke model Params */
        // Radial distortion coefficients
        m_k1 = declareProperty<FloatProperty>("warp.k1", 0.0f);
        m_k2 = declareProperty<FloatProperty>("warp.k2", 0.0f);
        m_k3 = declareProperty<FloatProperty>("warp.k3", 0.0f);
        m_d = declareProperty<FloatProperty>("warp.d", 1.0f);
        // Tangential distortion coefficients
        m_p1 = declareProperty<FloatProperty>("warp.p1", 0.0f);
        m_p2 = declareProperty<FloatProperty>("warp.p2", 0.0f);
        // Distortion Center and Offset
        m_center = declareProperty<Vec2fProperty>("warp.center", Vec2f(0.5f));
        m_offset = declareProperty<Vec2fProperty>("warp.offset", Vec2f(0.0f));

        // Focallength in x and y
        m_fx = declareProperty<FloatProperty>("warp.fx", 1.0f);
        m_fy = declareProperty<FloatProperty>("warp.fy", 1.0f);

        // Crop Ratio in x and y
        m_cropRatioX = declareProperty<FloatProperty>("warp.cropRatioX", 1.0f);
        m_cropRatioY = declareProperty<FloatProperty>("warp.cropRatioY", 1.0f);

        // 3de4 anamorphic params
        m_cx02 = declareProperty<FloatProperty>("warp.cx02", 0.0f);
        m_cy02 = declareProperty<FloatProperty>("warp.cy02", 0.0f);
        m_cx22 = declareProperty<FloatProperty>("warp.cx22", 0.0f);
        m_cy22 = declareProperty<FloatProperty>("warp.cy22", 0.0f);

        m_cx04 = declareProperty<FloatProperty>("warp.cx04", 0.0f);
        m_cy04 = declareProperty<FloatProperty>("warp.cy04", 0.0f);
        m_cx24 = declareProperty<FloatProperty>("warp.cx24", 0.0f);
        m_cy24 = declareProperty<FloatProperty>("warp.cy24", 0.0f);
        m_cx44 = declareProperty<FloatProperty>("warp.cx44", 0.0f);
        m_cy44 = declareProperty<FloatProperty>("warp.cy44", 0.0f);

        m_cx06 = declareProperty<FloatProperty>("warp.cx06", 0.0f);
        m_cy06 = declareProperty<FloatProperty>("warp.cy06", 0.0f);
        m_cx26 = declareProperty<FloatProperty>("warp.cx26", 0.0f);
        m_cy26 = declareProperty<FloatProperty>("warp.cy26", 0.0f);
        m_cx46 = declareProperty<FloatProperty>("warp.cx46", 0.0f);
        m_cy46 = declareProperty<FloatProperty>("warp.cy46", 0.0f);
        m_cx66 = declareProperty<FloatProperty>("warp.cx66", 0.0f);
        m_cy66 = declareProperty<FloatProperty>("warp.cy66", 0.0f);
    }

    LensWarpIPNode::~LensWarpIPNode() {}

    float LensWarpIPNode::pixelAspect() const
    {
        if (active())
        {
            return m_pixelAspect->front();
        }
        else
        {
            return 0.0f;
        }
    }

    bool LensWarpIPNode::active() const
    {
        return (m_activeProperty->front() ? true : false);
    }

    IPImage* LensWarpIPNode::evaluate(const Context& context)
    {
        IPImage* image = IPNode::evaluate(context);
        if (!image)
            return IPImage::newNoImage(this, "No Input");
        // There might be a way to support an incoming IPImage
        // without a shaderExpr.
        if (image->isNoImage() || !image->shaderExpr)
            return image;
        if (!active())
        {
            image->pixelAspect = 0.0;
            return image;
        }

        const float pa = pixelAspect();
        const string model = m_model->front();

        if (pa != 0.0f)
        {

            //
            // We look at the incoming pixel aspect because there is a case
            // where you have an pixelAspect embedded in the image, then
            // want to change it to be 1.
            // Consider changing the pixelAspect to 1.0 for the following
            // images:
            //   1.  width > height, embedded pixelRatio 2.0
            //   2.  width > height, embedded pixelRatio 0.5
            //   3.  height > width , embedded pixelRatio 2.0
            //   3.  height > width , embedded pixelRatio 0.5
            //

            if (pa > 1.0f || (pa == 1.0f && image->pixelAspect >= 1.0))
            {
                image->width = image->fb
                                   ? image->fb->uncropWidth() * pa
                                   : image->width / image->pixelAspect * pa;
            }
            if (pa < 1.0f || (pa == 1.0f && image->pixelAspect < 1.0))
            {
                image->height = image->fb
                                    ? image->fb->uncropHeight() / pa
                                    : image->height * image->pixelAspect / pa;
            }

            // Pixel Aspect may be changed down the line,
            // we keep the original value needed for computation
            image->initPixelAspect = image->pixelAspect;
            image->pixelAspect = pa;
        }

        //
        //  We may need to do some work here if the lens warp does any
        //  kind of complex sampling in the future. I.e. do the same
        //  thing that the FilterIPInstanceNode does in its evaluate
        //  for example. (convert incoming blending, etc)
        //

        if (model == "brown")
        {
            const float k1 = m_k1->front();
            const float k2 = m_k2->front();
            const float k3 = m_k3->front();
            const float d = m_d->front();
            const float p1 = m_p1->front();
            const float p2 = m_p2->front();
            const Vec2f center = m_center->front();
            const Vec2f offset = m_offset->front();
            const float fx = m_fx->front();
            const float fy = m_fy->front();
            const float cropRatioX = m_cropRatioX->front();
            const float cropRatioY = m_cropRatioY->front();

            image->shaderExpr = Shader::newLensWarp(
                image, image->shaderExpr, k1, k2, k3, d, p1, p2,
                center + offset, Vec2f(fx, fy), Vec2f(cropRatioX, cropRatioY));
        }
        else if (model == "opencv")
        {
            const float k1 = m_k1->front();
            const float k2 = m_k2->front();
            const float d = m_d->front();
            const float p1 = m_p1->front();
            const float p2 = m_p2->front();
            const Vec2f center = m_center->front();
            const float fx = m_fx->front();
            const float fy = m_fy->front();
            const float cropRatioX = m_cropRatioX->front();
            const float cropRatioY = m_cropRatioY->front();

            image->shaderExpr = Shader::newLensWarp(
                image, image->shaderExpr, k1, k2, 0.0f, d, p1, p2, center,
                Vec2f(fx, fy), Vec2f(cropRatioX, cropRatioY));
        }
        else if (model == "pfbarrel")
        {
            const float k1 = m_k1->front();
            const float k2 = m_k2->front();
            const float d = m_d->front();
            const Vec2f center = m_center->front();
            const float cropRatioX = m_cropRatioX->front();
            const float cropRatioY = m_cropRatioY->front();

            // For PFBarrel; we set the  fx and fy to
            // change the normalization length of the brown
            // shader from half the image's 'width' to half the image's
            // 'diagonal'.
            const float YXRatio = (float)image->height / (float)image->width;
            const float fx = 0.5f * sqrtf(1.0f + YXRatio * YXRatio);
            const float fy = fx;

            image->shaderExpr = Shader::newLensWarp(
                image, image->shaderExpr, k1, k2, 0.0f, d, 0.0f, 0.0f, center,
                Vec2f(fx, fy), Vec2f(cropRatioX, cropRatioY));
        }
        else if (model
                 == "tde4_ldp_anamorphic_deg_6") // 3DE4 Anamorphic Degree 6
        {
            const float cx02 = m_cx02->front(); // Distortion Degree 2
            const float cy02 = m_cy02->front(); // Distortion Degree 2
            const float cx22 = m_cx22->front(); // Distortion Degree 2
            const float cy22 = m_cy22->front(); // Distortion Degree 2

            const float cx04 = m_cx04->front(); // Distortion Degree 4
            const float cy04 = m_cy04->front(); // Distortion Degree 4
            const float cx24 = m_cx24->front(); // Distortion Degree 4
            const float cy24 = m_cy24->front(); // Distortion Degree 4
            const float cx44 = m_cx44->front(); // Distortion Degree 4
            const float cy44 = m_cy44->front(); // Distortion Degree 4

            const float cx06 = m_cx06->front(); // Distortion Degree 6
            const float cy06 = m_cy06->front(); // Distortion Degree 6
            const float cx26 = m_cx26->front(); // Distortion Degree 6
            const float cy26 = m_cy26->front(); // Distortion Degree 6
            const float cx46 = m_cx46->front(); // Distortion Degree 6
            const float cy46 = m_cy46->front(); // Distortion Degree 6
            const float cx66 = m_cx66->front(); // Distortion Degree 6
            const float cy66 = m_cy66->front(); // Distortion Degree 6

            const Vec2f center = m_center->front(); // Lens Center
            const Vec2f offset = m_offset->front(); // Lens Center Offset
            const float fx = m_fx->front();
            const float fy = m_fy->front();
            const float cropRatioX = m_cropRatioX->front();
            const float cropRatioY = m_cropRatioY->front();

            image->shaderExpr = Shader::newLensWarp3DE4AnamorphicDegree6(
                image, image->shaderExpr, Vec2f(cx02, cy02), Vec2f(cx22, cy22),
                Vec2f(cx04, cy04), Vec2f(cx24, cy24), Vec2f(cx44, cy44),
                Vec2f(cx06, cy06), Vec2f(cx26, cy26), Vec2f(cx46, cy46),
                Vec2f(cx66, cy66), center - offset, Vec2f(fx, fy),
                Vec2f(cropRatioX, cropRatioY));
        }
        else if (model == "adobe") // Adobe LCP
        {
            const float k1 = m_k1->front(); // stCamera: RadialDistortParam1
            const float k2 = m_k2->front(); // stCamera: RadialDistortParam2
            const float k3 = m_k3->front(); // stCamera: RadialDistortParam3
            const float d =
                0.99f; // This is a mystery; but it matches Lightroom.
            const float p1 = m_p1->front(); // stCamera: TangentialDistortParam1
            const float p2 = m_p2->front(); // stCamera: TangentialDistortParam2
            Vec2f center =
                m_center
                    ->front(); // stCamera:ImageXCenter stCamera:ImageYCenter
            const float fx = m_fx->front(); // stCamera:FocalLengthX
            const float fy = m_fy->front(); // stCamera:FocalLengthY
            const float cropRatioX = m_cropRatioX->front();
            const float cropRatioY = m_cropRatioY->front();

            image->shaderExpr = Shader::newLensWarp(
                image, image->shaderExpr, k1, k2, k3, d, p1, p2, center,
                Vec2f(fx, fy), Vec2f(cropRatioX, cropRatioY));
        }
        else if (model == "nuke") // IN PROGRESS;  needs to be figured out?
        {
            const float k1 = m_k1->front();
            const float k2 = m_k2->front();
            const float d = m_d->front();
            Vec2f center = m_center->front();
            // center.y *= -1;
            const float fx = m_fx->front();
            const float fy = m_fy->front();
            const float cropRatioX = m_cropRatioX->front();
            const float cropRatioY = m_cropRatioY->front();

            image->shaderExpr = Shader::newLensWarp(
                image, image->shaderExpr, k1, k2, 0.0f, d, 0.0f, 0.0f, center,
                Vec2f(fx, fy), Vec2f(cropRatioX, cropRatioY));
        }
        else if (model
                 == "rv4.0.10") // Legacy brown model for rv version <= 4.0.10
        {
            const float k1 = -m_k1->front();
            const float k2 = -m_k2->front();
            const float k3 = -m_k3->front();
            const float d = m_d->front();
            const float p1 = -m_p2->front();
            const float p2 = -m_p1->front();
            const Vec2f center = m_center->front();
            const Vec2f offset = 0.5f * m_offset->front();
            const float fx = m_fx->front();
            const float fy = m_fy->front();
            const float cropRatioX = 0.5f * m_cropRatioX->front();
            const float cropRatioY = 0.5f * m_cropRatioY->front();

            image->shaderExpr = Shader::newLensWarp(
                image, image->shaderExpr, k1, k2, k3, d, p1, p2,
                center + offset, Vec2f(fx, fy), Vec2f(cropRatioX, cropRatioY));
        }

        return image;
    }

    IPNode::ImageStructureInfo
    LensWarpIPNode::imageStructureInfo(const Context& context) const
    {
        ImageStructureInfo i = IPNode::imageStructureInfo(context);

        const float pa = pixelAspect();
        if (pa != 0.0f)
        {
            if (pa > 1.0f)
                i.width = i.width / i.pixelAspect * pa;
            if (pa < 1.0f)
                i.height = i.height * i.pixelAspect / pa;
            if (pa == 1.0f)
            {
                if (i.pixelAspect > 1.0f)
                    i.width = i.width / i.pixelAspect;
                if (i.pixelAspect < 1.0f)
                    i.height = i.height * i.pixelAspect;
            }
        }
        return i;
    }

    void LensWarpIPNode::readCompleted(const std::string& typeName,
                                       unsigned int version)
    {
        if (active() && pixelAspect() != 0.0f)
        {
            propagateImageStructureChange();
        }
    }

    void LensWarpIPNode::propertyChanged(const Property* prop)
    {
        if (prop == m_pixelAspect)
        {
            propagateImageStructureChange();
        }
        IPNode::propertyChanged(prop);
    }

} // namespace IPCore
