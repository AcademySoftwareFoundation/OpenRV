//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <IPBaseNodes/ColorShadowIPNode.h>
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

    ColorShadowIPNode::ColorShadowIPNode(const std::string& name,
                                         const NodeDefinition* def,
                                         IPGraph* graph, GroupIPNode* group)
        : IPNode(name, def, graph, group)
    {
        setMaxInputs(1);

        Property::Info* info = new Property::Info();
        info->setPersistent(false);

        m_active = declareProperty<IntProperty>("color.active", 1);
        m_colorShadow = declareProperty<FloatProperty>("color.shadow", 0.0f);
    }

    ColorShadowIPNode::~ColorShadowIPNode() {}

    IPImage* ColorShadowIPNode::evaluate(const Context& context)
    {

        int frame = context.frame;
        IPImage* head = IPNode::evaluate(context);
        if (!head)
            return IPImage::newNoImage(this, "No Input");
        IPImage* img = head;

        bool active = m_active ? m_active->front() : true;
        if (!active)
            return img;

        ////////////////////// Shadow ////////////////////////
        // this goes into YCbCr space, and come back
        float shadow = m_colorShadow ? m_colorShadow->front() : 0;
        float shadowPoint = 0.45;
        if (shadow)
        {
            img->shaderExpr = Shader::newColorSRGBYCbCr(img->shaderExpr);

            // 0->0, shadowPoint->shadowPoint. at shadowPoint
            float down = 0.25 + shadow * 0.01 * 0.05; // 20-30%
            Vec2f p1(shadowPoint, shadowPoint);
            Vec2f p2(0, 0);
            Vec2f p3(0.25, down);
            Mat44f m;
            m[0][0] = 0;
            m[0][1] = 0;
            m[0][2] = 0;
            m[0][3] = 1;
            m[1][0] = 0.25 * 0.25 * 0.25;
            m[1][1] = 0.25 * 0.25;
            m[1][2] = 0.25;
            m[1][3] = 1;
            m[2][0] = shadowPoint * shadowPoint * shadowPoint;
            m[2][1] = shadowPoint * shadowPoint;
            m[2][2] = shadowPoint;
            m[2][3] = 1;

            // enforce derivative 1 at shadowPoint
            m[3][0] = 3 * shadowPoint * shadowPoint;
            m[3][1] = 2 * shadowPoint;
            m[3][2] = 1;
            m[3][3] = 0;

            Mat44f mInv = m.inverted();

            Vec4f coeff;
            coeff[0] =
                mInv[0][0] * 0 + mInv[0][1] * down + mInv[0][2] * shadowPoint;
            coeff[1] =
                mInv[1][0] * 0 + mInv[1][1] * down + mInv[1][2] * shadowPoint;
            coeff[2] =
                mInv[2][0] * 0 + mInv[2][1] * down + mInv[2][2] * shadowPoint;
            coeff[3] =
                mInv[3][0] * 0 + mInv[3][1] * down + mInv[3][2] * shadowPoint;

            coeff[0] += mInv[0][3];
            coeff[1] += mInv[1][3];
            coeff[2] += mInv[2][3];
            coeff[3] += mInv[3][3];

            img->shaderExpr =
                Shader::newColorShadowonY(img->shaderExpr, coeff, shadowPoint);

            img->shaderExpr = Shader::newColorYCbCrSRGB(img->shaderExpr);
        }

        return img;
    }

} // namespace IPCore
