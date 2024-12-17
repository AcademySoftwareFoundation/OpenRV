//
//  Copyright (c) 2013 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPBaseNodes/PrimaryConvertIPNode.h>
#include <TwkFB/Operations.h>
#include <IPCore/ShaderCommon.h>
#include <TwkMath/Vec2.h>
#include <ImathVec.h>
#include <ImathMatrix.h>
#include <ImfChromaticities.h>

namespace IPCore
{
    using namespace std;
    using namespace TwkMath;

    PrimaryConvertIPNode::PrimaryConvertIPNode(const std::string& name,
                                               const NodeDefinition* def,
                                               IPGraph* graph,
                                               GroupIPNode* group)
        : IPNode(name, def, graph, group)
    {
        m_activeProperty = declareProperty<IntProperty>("node.active", 0);

        // IlmfImf default chromaticities; i.e. rec709.
        Imf::Chromaticities cDefault;

        // Default in chromaticities
        m_inChromaticitiesRed = declareProperty<Vec2fProperty>(
            "inChromaticities.red", Vec2f(cDefault.red.x, cDefault.red.y));
        m_inChromaticitiesGreen = declareProperty<Vec2fProperty>(
            "inChromaticities.green",
            Vec2f(cDefault.green.x, cDefault.green.y));
        m_inChromaticitiesBlue = declareProperty<Vec2fProperty>(
            "inChromaticities.blue", Vec2f(cDefault.blue.x, cDefault.blue.y));
        m_inChromaticitiesWhite = declareProperty<Vec2fProperty>(
            "inChromaticities.white",
            Vec2f(cDefault.white.x, cDefault.white.y));

        // Default out chromaticities
        m_outChromaticitiesRed = declareProperty<Vec2fProperty>(
            "outChromaticities.red", Vec2f(cDefault.red.x, cDefault.red.y));
        m_outChromaticitiesGreen = declareProperty<Vec2fProperty>(
            "outChromaticities.green",
            Vec2f(cDefault.green.x, cDefault.green.y));
        m_outChromaticitiesBlue = declareProperty<Vec2fProperty>(
            "outChromaticities.blue", Vec2f(cDefault.blue.x, cDefault.blue.y));
        m_outChromaticitiesWhite = declareProperty<Vec2fProperty>(
            "outChromaticities.white",
            Vec2f(cDefault.white.x, cDefault.white.y));

        // Default in and out illuminant white points.
        m_inIlluminantWhite = declareProperty<Vec2fProperty>(
            "illuminantAdaptation.inIlluminantWhite",
            Vec2f(cDefault.white.x, cDefault.white.y));
        m_outIlluminantWhite = declareProperty<Vec2fProperty>(
            "illuminantAdaptation.outIlluminantWhite",
            Vec2f(cDefault.white.x, cDefault.white.y));

        // Default adapt is true.
        m_useBradfordTransform = declareProperty<IntProperty>(
            "illuminantAdaptation.useBradfordTransform", 1);
    }

    PrimaryConvertIPNode::~PrimaryConvertIPNode() {}

    bool PrimaryConvertIPNode::active() const
    {
        return (m_activeProperty->front() ? true : false);
    }

    IPImage* PrimaryConvertIPNode::evaluate(const Context& context)
    {
        IPImage* image = IPNode::evaluate(context);
        if (!image)
            return IPImage::newNoImage(this, "No Input");
        if (image->isNoImage() || !active())
            return image;

        const Vec2f inChromaR = m_inChromaticitiesRed->front();
        const Vec2f inChromaG = m_inChromaticitiesGreen->front();
        const Vec2f inChromaB = m_inChromaticitiesBlue->front();
        const Vec2f inChromaW = m_inChromaticitiesWhite->front();

        const Vec2f outChromaR = m_outChromaticitiesRed->front();
        const Vec2f outChromaG = m_outChromaticitiesGreen->front();
        const Vec2f outChromaB = m_outChromaticitiesBlue->front();
        const Vec2f outChromaW = m_outChromaticitiesWhite->front();

        const Vec2f inIlluminant = m_inIlluminantWhite->front();
        const Vec2f outIlluminant = m_outIlluminantWhite->front();

        bool adapt =
            (m_useBradfordTransform->front() ? (inIlluminant != outIlluminant)
                                             : false);

        //
        // Do conversion iff any of the in's do not match out's.
        //
        const bool doConvert =
            ((inChromaR != outChromaR) || (inChromaG != outChromaG)
             || (inChromaB != outChromaB) || (inChromaW != outChromaW)
             || adapt);

        if (doConvert)
        {
            Imf::Chromaticities c0(Imath::V2f(inChromaR.x, inChromaR.y),
                                   Imath::V2f(inChromaG.x, inChromaG.y),
                                   Imath::V2f(inChromaB.x, inChromaB.y),
                                   Imath::V2f(inChromaW.x, inChromaW.y));

            Imf::Chromaticities c1(Imath::V2f(outChromaR.x, outChromaR.y),
                                   Imath::V2f(outChromaG.x, outChromaG.y),
                                   Imath::V2f(outChromaB.x, outChromaB.y),
                                   Imath::V2f(outChromaW.x, outChromaW.y));

            Imath::M44f A;

            // pinched from ImfAcesFile's ACES color conversion
            if (adapt)
            {
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
            }

            A = Imf::RGBtoXYZ(c0, 1.0) * A * Imf::XYZtoRGB(c1, 1.0);
            A.transpose(); // Transpose for passing into shader.
            TwkMath::Mat44f conversionMatrix;
            memcpy(&conversionMatrix, &A, sizeof(float) * 16);

            image->shaderExpr =
                Shader::newColorMatrix(image->shaderExpr, conversionMatrix);
        }

        return image;
    }

} // namespace IPCore
