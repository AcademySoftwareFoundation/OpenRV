//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <IPBaseNodes/ColorCurveIPNode.h>
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

    // http://en.wikipedia.org/wiki/Monotone_cubic_interpolation
    // strictly monotonic

    ColorCurveIPNode::ColorCurveIPNode(const std::string& name,
                                       const NodeDefinition* def,
                                       IPGraph* graph, GroupIPNode* group)
        : IPNode(name, def, graph, group)
    {
        setMaxInputs(1);

        Property::Info* info = new Property::Info();
        info->setPersistent(false);

        m_active = declareProperty<IntProperty>("color.active", 1);
        m_colorContrast =
            declareProperty<FloatProperty>("color.contrast", 0.0f);
    }

    ColorCurveIPNode::~ColorCurveIPNode() {}

    static bool meetConditions(float a1, float b1)
    {
        if ((a1 + 2 * b1 - 3 <= 0) || (2 * a1 + b1 - 3 <= 0)
            || (a1 - (2 * a1 + b1 - 3) * (2 * a1 + b1 - 3) / (a1 + b1 - 2) / 3)
                   > 0)
            return true;
        return false;
    }

    IPImage* ColorCurveIPNode::evaluate(const Context& context)
    {

        int frame = context.frame;
        IPImage* head = IPNode::evaluate(context);
        if (!head)
            return IPImage::newNoImage(this, "No Input");
        IPImage* img = head;

        bool active = m_active ? m_active->front() : true;
        if (!active)
            return img;

        ////////////////////// Contrast /////////////////////////
        // this goes into YCbCr space, and come back
        float contrast = m_colorContrast ? m_colorContrast->front() : 0;
        if (contrast)
        {
            img->shaderExpr = Shader::newColorSRGBYCbCr(img->shaderExpr);

            // compute curve
            // for the future, could do a sampling for efficiency
            float up = contrast * 0.01 * 0.20 + 0.75;
            float down = 0.25 - contrast * 0.01 * 0.20;
            // these are the four points we interpolate from
            Vec3f p1(0, 0, 1);
            Vec3f p2(0.25, down, 1);
            Vec3f p3(0.75, up, 1);
            Vec3f p4(1.0, 1.0, 1);

            float d1 = (p2.y - p1.y) / (p2.x - p1.x);
            float d2 = (p3.y - p2.y) / (p3.x - p2.x);
            float d3 = (p4.y - p3.y) / (p4.x - p3.x);

            float m1 = d1;
            float m2 = (d1 + d2) * 0.5;
            float m3 = (d2 + d3) * 0.5;
            float m4 = d3;

            float a1 = m1 / d1;
            float a2 = m2 / d2;
            float a3 = m3 / d3;
            float b1 = m2 / d1;
            float b2 = m3 / d2;
            float b3 = m4 / d3;

            if (!meetConditions(a1, b1))
            {
                float t = 3 / sqrt(a1 * a1 + b1 * b1);
                m1 = t * a1 * d1;
                m2 = t * b1 * d1;
            }
            if (!meetConditions(a2, b2))
            {
                float t = 3 / sqrt(a2 * a2 + b2 * b2);
                m2 = t * a2 * d2;
                m3 = t * b2 * d2;
            }
            if (!meetConditions(a3, b3))
            {
                float t = 3 / sqrt(a3 * a3 + b3 * b3);
                m3 = t * a3 * d3;
                m4 = t * b3 * d3;
            }

            p1.z = m1;
            p2.z = m2;
            p3.z = m3;
            p4.z = m4;

            img->shaderExpr =
                Shader::newColorCurveonY(img->shaderExpr, p1, p2, p3, p4);

            img->shaderExpr = Shader::newColorYCbCrSRGB(img->shaderExpr);
        }

        return img;
    }

} // namespace IPCore
