//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <IPBaseNodes/ColorHighlightIPNode.h>
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

    ColorHighlightIPNode::ColorHighlightIPNode(const std::string& name,
                                               const NodeDefinition* def,
                                               IPGraph* graph,
                                               GroupIPNode* group)
        : IPNode(name, def, graph, group)
    {
        setMaxInputs(1);

        Property::Info* info = new Property::Info();
        info->setPersistent(false);

        m_active = declareProperty<IntProperty>("color.active", 1);
        m_colorHighlight =
            declareProperty<FloatProperty>("color.highlight", 0.0f);
    }

    ColorHighlightIPNode::~ColorHighlightIPNode() {}

    IPImage* ColorHighlightIPNode::evaluate(const Context& context)
    {

        int frame = context.frame;
        IPImage* head = IPNode::evaluate(context);
        if (!head)
            return IPImage::newNoImage(this, "No Input");
        IPImage* img = head;

        bool active = m_active ? m_active->front() : true;
        if (!active)
            return img;

        ////////////////////// Highlight ////////////////////////
        // this goes into YCbCr space, and come back
        float highlight = m_colorHighlight ? m_colorHighlight->front() : 0;
        float highlightPoint = 0.5;
        if (highlight)
        {
            img->shaderExpr = Shader::newColorSRGBYCbCr(img->shaderExpr);

            // highpoint->highpoint (derivative = 1), 0.75->highlight, 1->1
            float up = 0.75 + highlight * 0.01 * 0.1; // 65-85%
            Vec2f p1(highlightPoint, highlightPoint);
            Vec2f p2(1, 1);
            Vec2f p3(0.75, up);
            Mat44f m;
            m[0][0] = 1;
            m[0][1] = 1;
            m[0][2] = 1;
            m[0][3] = 1;
            m[1][0] = 0.75 * 0.75 * 0.75;
            m[1][1] = 0.75 * 0.75;
            m[1][2] = 0.75;
            m[1][3] = 1;
            m[2][0] = highlightPoint * highlightPoint * highlightPoint;
            m[2][1] = highlightPoint * highlightPoint;
            m[2][2] = highlightPoint;
            m[2][3] = 1;

            // derivative at highlight is 1
            m[3][0] = 3 * highlightPoint * highlightPoint;
            m[3][1] = 2 * highlightPoint;
            m[3][2] = 1;
            m[3][3] = 0;

            Mat44f mInv = m.inverted();

            Vec4f coeff;
            coeff[0] =
                mInv[0][0] * 1 + mInv[0][1] * up + mInv[0][2] * highlightPoint;
            coeff[1] =
                mInv[1][0] * 1 + mInv[1][1] * up + mInv[1][2] * highlightPoint;
            coeff[2] =
                mInv[2][0] * 1 + mInv[2][1] * up + mInv[2][2] * highlightPoint;
            coeff[3] =
                mInv[3][0] * 1 + mInv[3][1] * up + mInv[3][2] * highlightPoint;

            coeff[0] += mInv[0][3];
            coeff[1] += mInv[1][3];
            coeff[2] += mInv[2][3];
            coeff[3] += mInv[3][3];

            img->shaderExpr = Shader::newColorHighlightonY(
                img->shaderExpr, coeff, highlightPoint);

            img->shaderExpr = Shader::newColorYCbCrSRGB(img->shaderExpr);
        }

        return img;
    }

} // namespace IPCore
