//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <IPBaseNodes/ColorTemperatureIPNode.h>
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
#include <ImathVec.h>
#include <ImathMatrix.h>
#include <ImfChromaticities.h>

namespace IPCore
{
    using namespace std;
    using namespace TwkContainer;
    using namespace TwkMath;

    ColorTemperatureIPNode::ColorTemperatureIPNode(const std::string& name,
                                                   const NodeDefinition* def,
                                                   IPGraph* graph,
                                                   GroupIPNode* group)
        : IPNode(name, def, graph, group)
    {
        setMaxInputs(1);

        Property::Info* info = new Property::Info();
        info->setPersistent(false);

        m_inWhitePrimary = declareProperty<Vec2fProperty>(
            "color.inWhitePrimary", Vec2f(0.3457, 0.3585));
        m_inTemperature =
            declareProperty<FloatProperty>("color.inTemperature", 6500.0f);
        m_outTemperature =
            declareProperty<FloatProperty>("color.outTemperature", 6500.0f);
        m_active = declareProperty<IntProperty>("color.active", 1);
        m_method = declareProperty<IntProperty>("color.method", 2);
    }

    ColorTemperatureIPNode::~ColorTemperatureIPNode() {}

    static Vec3f temperatureToRGB(float t)
    {

        t = max(1000.0f, min(40000.0f, t));
        t /= 100;
        float r, g, b;
        // r
        if (t <= 66)
            r = 255;
        else
        {
            float tempT = t - 60;
            r = 329.698727446 * pow(tempT, -0.1332047592f);
            r = max(0.0f, min(255.0f, r));
        }

        // g
        if (t <= 66)
        {
            g = 99.4708025861 * log(t) - 161.1195681661;
        }
        else
        {
            float tempT = t - 60;
            g = 288.1221695283 * pow(tempT, -0.0755148492f);
        }
        g = max(0.0f, min(255.0f, g));

        // b
        if (t <= 19)
            b = 0;
        else if (t < 66)
        {
            float tempT = t - 10;
            b = 138.5177312231 * log(tempT) - 305.0447927307;
            b = max(0.0f, min(255.0f, b));
        }
        else
            b = 255;

        r /= 255;
        g /= 255;
        b /= 255;
        return Vec3f(r, g, b);
    }

    static Vec3f computeOffset(float inV, float outV)
    {
        if (inV == outV)
            return Vec3f(0, 0, 0);

        Vec3f in3 = temperatureToRGB(inV);
        Vec3f out3 = temperatureToRGB(outV);
        float inl = 0.299 * in3.x + 0.587 * in3.y + 0.114 * in3.z;
        float outl = 0.299 * out3.x + 0.587 * out3.y + 0.114 * out3.z;
        in3 /= inl;
        out3 /= outl;

        // 0 -> 390
        inV = max(1000.0f, min(40000.0f, inV));
        inV /= 100;
        inV -= 10;
        outV = max(1000.0f, min(40000.0f, outV));
        outV /= 100;
        outV -= 10;

        return in3 - out3;
    }

    static void KelvinToXY(float kelvin, Vec2f& xy)
    {
        kelvin = min(25000.0f, max(4000.0f, kelvin));
        kelvin /= 1000;
        if (kelvin > 7) // 7k - 25k
        {
            xy.x = -4.607 / (kelvin * kelvin * kelvin)
                   + 2.9678 / (kelvin * kelvin) + 0.09911 / kelvin + 0.244063;
        }
        else // 4k - 7k
        {
            xy.x = -2.0064 / (kelvin * kelvin * kelvin)
                   + 1.9018 / (kelvin * kelvin) + 0.24748 / kelvin + 0.23704;
        }

        xy.y = -3 * xy.x * xy.x + 2.87 * xy.x - 0.275;
    }

#if 0
static void XYToKelvin(const Vec2f& xy, float& kelvin)
{
    float n = (xy.x - 0.332) / (0.1858 - xy.y);
    kelvin = 449 * n * n * n + 3525 * n * n + 6823.3 * n + 5520.33;

}
#endif

    IPImage* ColorTemperatureIPNode::evaluate(const Context& context)
    {
        int frame = context.frame;
        IPImage* head = IPNode::evaluate(context);
        if (!head)
            return IPImage::newNoImage(this, "No Input");
        IPImage* img = head;

        bool active = m_active ? m_active->front() : true;

        if (active)
        {
            float outTemperature = m_outTemperature->front();
            float inTemperature = m_inTemperature->front();
            int method = m_method->front();

            if (method == TANNER)
            {
                float blend = 0.5;
                if (inTemperature == outTemperature)
                    return img;
                Vec3f c = temperatureToRGB(outTemperature);
                img->shaderExpr = Shader::newColorBlendWithConstant(
                    img->shaderExpr, Vec4f(c.x, c.y, c.z, blend));
            }
            else if (method == DANIELLE)
            {
                float blend = 0.5;
                if (inTemperature == outTemperature)
                    return img;
                Vec3f c = computeOffset(inTemperature, outTemperature);
                img->shaderExpr = Shader::newColorTemperatureOffset(
                    img->shaderExpr, Vec4f(c.x, c.y, c.z, blend));
            }
            else
            {
                // find x,y
                Vec2f inIlluminant, outIlluminant;

                if (false)
                {
                    inIlluminant = m_inWhitePrimary->front();
                }
                else
                {
                    float inTemperature = m_inTemperature->front();
                    KelvinToXY(inTemperature, inIlluminant);
                }

                KelvinToXY(outTemperature, outIlluminant);

                if (inIlluminant == outIlluminant)
                    return img;

                Vec2f temp;
                temp = inIlluminant;
                inIlluminant = outIlluminant;
                outIlluminant = temp;

                // Bradford Transform
                Imf::Chromaticities c0;
                Imath::M44f A;
                static const Imath::M44f B(
                    0.895100, -0.750200, 0.038900, 0.000000, 0.266400, 1.713500,
                    -0.068500, 0.000000, -0.161400, 0.036700, 1.029600,
                    0.000000, 0.000000, 0.000000, 0.000000, 1.000000);

                static const Imath::M44f BI(
                    0.986993, 0.432305, -0.008529, 0.000000, -0.147054,
                    0.518360, 0.040043, 0.000000, 0.159963, 0.049291, 0.968487,
                    0.000000, 0.000000, 0.000000, 0.000000, 1.000000);

                float ix = inIlluminant.x;
                float iy = inIlluminant.y;
                Imath::V3f inN(ix / iy, 1, (1 - ix - iy) / iy);

                float ox = outIlluminant.x;
                float oy = outIlluminant.y;
                Imath::V3f outN(ox / oy, 1, (1 - ox - oy) / oy);

                Imath::V3f ratio((outN * B) / (inN * B));

                Imath::M44f R(ratio[0], 0, 0, 0, 0, ratio[1], 0, 0, 0, 0,
                              ratio[2], 0, 0, 0, 0, 1);

                A = B * R * BI;

                A = Imf::RGBtoXYZ(c0, 1.0) * A * Imf::XYZtoRGB(c0, 1.0);
                A.transpose(); // Transpose for passing into shader.
                TwkMath::Mat44f conversionMatrix;
                memcpy(&conversionMatrix, &A, sizeof(float) * 16);

                img->shaderExpr =
                    Shader::newColorMatrix(img->shaderExpr, conversionMatrix);
            }
        }

        return img;
    }

} // namespace IPCore
