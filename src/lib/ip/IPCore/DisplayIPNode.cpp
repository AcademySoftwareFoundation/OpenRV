//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <IPCore/DisplayIPNode.h>
#include <IPCore/Exception.h>
#include <IPCore/ShaderCommon.h>
#include <ImfRgbaYca.h>
#include <ImfChromaticities.h>
#include <ImathVec.h>
#include <TwkMath/Function.h>
#include <TwkMath/MatrixColor.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stl_ext/string_algo.h>
#include <TwkFB/Operations.h>
#include <IPCore/IPProperty.h>
#include <IPCore/DispTransform2DIPNode.h>
#include <IPCore/NodeDefinition.h>

namespace IPCore
{
    using namespace std;
    using namespace TwkContainer;
    using namespace TwkMath;

    static inline Imath::V2f convert(Vec2f v) { return Imath::V2f(v.x, v.y); }

    static inline Imath::V3f convert(Vec3f v)
    {
        return Imath::V3f(v.x, v.y, v.z);
    }

    static inline Vec2f convert(Imath::V2f v) { return Vec2f(v.x, v.y); }

    static inline Vec3f convert(Imath::V3f v) { return Vec3f(v.x, v.y, v.z); }

    static inline Mat44f convert(const Imath::M44f& m)
    {
        return reinterpret_cast<const Mat44f*>(&m)->transposed();
    }

    DisplayIPNode::DisplayIPNode(const std::string& name,
                                 const NodeDefinition* def, IPGraph* g,
                                 GroupIPNode* group)
        : LUTIPNode(name, def, g, group)
    {
        setMaxInputs(1);
        setHasLinearTransform(true); // fits to aspect

        m_channelOrder =
            declareProperty<StringProperty>("color.channelOrder", "RGBA");
        m_channelFlood = declareProperty<IntProperty>("color.channelFlood", 0);
        m_premult = declareProperty<IntProperty>("color.premult", 0);
        m_gamma = declareProperty<FloatProperty>("color.gamma", 1.0f);
        m_srgb = declareProperty<IntProperty>("color.sRGB", 0);
        m_rec709 = declareProperty<IntProperty>("color.Rec709", 0);
        m_brightness = declareProperty<FloatProperty>("color.brightness", 0.0f);
        m_outOfRange = declareProperty<IntProperty>("color.outOfRange", 0);
        m_dither = declareProperty<IntProperty>("color.dither", 0);
        m_ditherLast = declareProperty<IntProperty>("color.ditherLast", 1);
        m_active = declareProperty<IntProperty>("color.active", 1);

        //
        //  default to rec709 primaries
        //

        m_chromaActive =
            declareProperty<IntProperty>("chromaticities.active", 0);
        m_adoptedNeutral =
            declareProperty<IntProperty>("chromaticities.adoptedNeutral", 1);
        m_white = declareProperty<Vec2fProperty>("chromaticities.white",
                                                 Vec2f(0.3127f, 0.3290f));
        m_red = declareProperty<Vec2fProperty>("chromaticities.red",
                                               Vec2f(0.640f, 0.330f));
        m_green = declareProperty<Vec2fProperty>("chromaticities.green",
                                                 Vec2f(0.3f, 0.6f));
        m_blue = declareProperty<Vec2fProperty>("chromaticities.blue",
                                                Vec2f(0.15f, 0.06f));
        m_neutral = declareProperty<Vec2fProperty>("chromaticities.neutral",
                                                   Vec2f(0.3127f, 0.3290f));

        m_overrideColorspace = 0;

        if (definition()->intValue("defaults.overridableColorspace", 0) != 0)
        {
            //
            //  Since this is a string property we need to protect against
            //  simultaneous reading/writing by causing the graph to shutdown
            //  when changing it. The info here tells the PropertyEditor to
            //  do that.
            //

            PropertyInfo* info = new PropertyInfo(
                PropertyInfo::Persistent | PropertyInfo::RequiresGraphEdit, 1);

            m_overrideColorspace = declareProperty<StringProperty>(
                "color.overrideColorspace", "", info);
        }
    }

    DisplayIPNode::~DisplayIPNode() {}

    struct AssignDisplayState
    {
        AssignDisplayState(string overrideColorspace_, bool linear2sRGB_,
                           bool linear2Rec709_, float displayGamma_,
                           bool outOfRange_, size_t dither_, bool ditherLast_,
                           size_t seed_, const IPImage::Matrix& C_,
                           const IPImage::Matrix& Ca_,
                           const IPImage::Matrix& B_, DisplayIPNode* node_,
                           const IPNode::Context& context_)
            : overrideColorspace(overrideColorspace_)
            , linear2sRGB(linear2sRGB_)
            , linear2Rec709(linear2Rec709_)
            , displayGamma(displayGamma_)
            , outOfRange(outOfRange_)
            , dither(dither_)
            , ditherLast(ditherLast_)
            , seed(seed_)
            , C(C_)
            , Ca(Ca_)
            , B(B_)
            , node(node_)
            , context(context_)
        {
        }

        string overrideColorspace;
        bool linear2sRGB;
        bool linear2Rec709;
        float displayGamma;
        bool outOfRange;
        IPImage::Matrix C;
        IPImage::Matrix Ca;
        IPImage::Matrix B;
        size_t dither;
        bool ditherLast;
        size_t seed;
        DisplayIPNode* node;
        const IPNode::Context& context;

        void operator()(IPImage* i)
        {
            if (Ca != Mat44f())
                i->shaderExpr = Shader::newColorMatrix4D(i->shaderExpr, Ca);
            if (C != Mat44f())
                i->shaderExpr = Shader::newColorMatrix(i->shaderExpr, C);

            if (dither != 0.0f && !ditherLast)
                i->shaderExpr =
                    Shader::newDither(i, i->shaderExpr, dither, seed);

            node->addLUTPipeline(context, i);

            if (overrideColorspace.empty())
            {
                if (linear2sRGB)
                    i->shaderExpr = Shader::newColorLinearToSRGB(i->shaderExpr);
                if (linear2Rec709)
                    i->shaderExpr =
                        Shader::newColorLinearToRec709(i->shaderExpr);
                if (displayGamma != 1.0)
                    i->shaderExpr = Shader::newColorGamma(
                        i->shaderExpr, Vec3f(1.0 / displayGamma));
            }
            else
            {
                if (overrideColorspace == TwkFB::ColorSpace::sRGB())
                {
                    i->shaderExpr = Shader::newColorLinearToSRGB(i->shaderExpr);
                }
                else if (overrideColorspace == TwkFB::ColorSpace::Rec709())
                {
                    i->shaderExpr =
                        Shader::newColorLinearToRec709(i->shaderExpr);
                }
                else if (overrideColorspace == TwkFB::ColorSpace::CineonLog())
                {
                    const float refBlack = 95.0;
                    const float refWhite = 685.0;

                    i->shaderExpr = Shader::newColorLinearToCineonLog(
                        i->shaderExpr, refBlack, refWhite);
                }
                else if (overrideColorspace == TwkFB::ColorSpace::RedLog())
                {
                    i->shaderExpr =
                        Shader::newColorLinearToRedLog(i->shaderExpr);
                }
                else if (overrideColorspace == TwkFB::ColorSpace::RedLogFilm())
                {
                    // Red Log Film is Cineon Log. See ACES v1.0.1 distribution.
                    const float refBlack = 95.0;
                    const float refWhite = 685.0;

                    i->shaderExpr = Shader::newColorLinearToCineonLog(
                        i->shaderExpr, refBlack, refWhite);
                }
                else if (overrideColorspace == TwkFB::ColorSpace::ArriLogC())
                {
                    TwkFB::LogCTransformParams params;

                    TwkFB::getLogCCurveParams(params, i->fb);

                    i->shaderExpr = Shader::newColorLinearLogC(
                        i->shaderExpr,
                        params.LogCBlackSignal,    // pbs
                        params.LogCEncodingOffset, // eo
                        params.LogCEncodingGain,   // eg
                        params.LogCGraySignal,     // gs
                        params.LogCBlackOffset,    // bo
                        params.LogCLinearSlope,    // ls
                        params.LogCLinearOffset,   // lo
                        params.LogCCutPoint);      // linear to logc cutoff
                }
                else if (overrideColorspace == TwkFB::ColorSpace::ACES())
                {
                    typedef Chromaticities<float> CIECoords;

                    Imf::Chromaticities chr709;
                    CIECoords cc = CIECoords::ACES();

                    Mat44f A;
                    Imf::Chromaticities chr(convert(cc.red), convert(cc.green),
                                            convert(cc.blue),
                                            convert(cc.white));

                    Vec2f cneutral = cc.white;
                    TwkFB::colorSpaceConversionMatrix(
                        (const float*)&chr709, (const float*)&chr,
                        (const float*)&chr709.white, (const float*)&cneutral,
                        true, (float*)&A);

                    i->shaderExpr = Shader::newColorMatrix(i->shaderExpr, A);
                }
                else if (overrideColorspace == TwkFB::ColorSpace::Linear())
                {
                    ; //  If "Linear", do nothing
                }
                else
                {
                    cerr << "ERROR: unknown overriding color space name '"
                         << overrideColorspace << "'" << endl;
                }
            }

            if (outOfRange)
                i->shaderExpr = Shader::newColorOutOfRange(i->shaderExpr);

            if (dither != 0.0f && ditherLast)
                i->shaderExpr =
                    Shader::newDither(i, i->shaderExpr, dither, seed);

            if (B != Mat44f())
                i->shaderExpr = Shader::newColorMatrix(i->shaderExpr, B);
        }
    };

    IPImage* DisplayIPNode::evaluate(const Context& context)
    {
        int frame = context.frame;

        IPImage* root = IPNode::evaluate(context);

        if (!root)
            return IPImage::newNoImage(this, "No Input");
        if (root->isBlank() || root->isNoImage())
            return root;
        if (propertyValue(m_active, 1) == 0)
            return root;

        bool linear2sRGB = propertyValue(m_srgb, 0) != 0;
        bool linear2Rec709 = propertyValue(m_rec709, 0) != 0;
        float displayGamma = propertyValue(m_gamma, 1.0f);
        bool outOfRange = propertyValue(m_outOfRange, 0) != 0;
        int dither = propertyValue(m_dither, 0);
        bool ditherLast = propertyValue(m_ditherLast, 1) != 0;
        string bgtype = "none";
        Vec3f bgcolor = Vec3f(0.18);
        bool chromaActive = propertyValue(m_chromaActive, 0) != 0;

        Mat44f C, Ca, B;

        string order = propertyValue(m_channelOrder, "");
        int flood = propertyValue(m_channelFlood, 0);

        string overrideColorspace = propertyValue(m_overrideColorspace, "");

        if (order != "")
        {
            Mat44f M(0.0);

            for (int i = 0; i < order.size() && i < 4; i++)
            {
                switch (order[i])
                {
                case 'R':
                    M(i, 0) = 1.0;
                    break;
                case 'G':
                    M(i, 1) = 1.0;
                    break;
                case 'B':
                    M(i, 2) = 1.0;
                    break;
                case 'A':
                    M(i, 3) = 1.0;
                    break;
                case '1':
                    M(i, 0) = 1.0, M(i, 1) = 1.0, M(i, 2) = 1.0, M(i, 3) = 1.0;
                    break;
                default:
                case '0':
                    M(i, 0) = 0.0, M(i, 1) = 0.0, M(i, 2) = 0.0, M(i, 3) = 0.0;
                    break;
                }
            }

            Ca = M * Ca;
        }

        if (flood != 0)
        {
            int ch = flood;

            Mat44f M = Rec709FullRangeRGBToYUV8<float>();
            const float rw709 = M.m00;
            const float gw709 = M.m01;
            const float bw709 = M.m02;

            Mat44f F;

            switch (ch)
            {
            case 1:
                F = Mat44f(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1);
                break;
            case 2:
                F = Mat44f(0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1);
                break;
            case 3:
                F = Mat44f(0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1);
                break;
            case 4:
                F = Mat44f(0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1);
                break;
            case 5:
                F = Mat44f(rw709, gw709, bw709, 0, rw709, gw709, bw709, 0,
                           rw709, gw709, bw709, 0, 0, 0, 0, 1);
                break;
            default:
                // nothing
                break;
            }

            Ca = F * Ca;
        }

        if (chromaActive)
        {
            Vec2fProperty* vp;
            Vec2f white709 = Vec2f(0.3127f, 0.3290f);
            Vec2f red709 = Vec2f(0.6400f, 0.3300f);
            Vec2f green709 = Vec2f(0.3000f, 0.6000f);
            Vec2f blue709 = Vec2f(0.1500f, 0.0600f);

            Vec2f cwhite = propertyValue(m_white, white709);
            Vec2f cred = propertyValue(m_red, red709);
            Vec2f cgreen = propertyValue(m_green, green709);
            Vec2f cblue = propertyValue(m_blue, blue709);
            Vec2f cneutral = propertyValue(m_neutral, cwhite);

            Imf::Chromaticities chr709(convert(red709), convert(green709),
                                       convert(blue709), convert(white709));

            Imf::Chromaticities chr(convert(cred), convert(cgreen),
                                    convert(cblue), convert(cwhite));

            Mat44f M;

            TwkFB::colorSpaceConversionMatrix(
                (const float*)&chr709, (const float*)&chr,
                (const float*)&chr709.white, (const float*)&cneutral,
                m_adoptedNeutral, (float*)&M);

            C = M * C;
        }

        //
        //  Check for user matrix
        //

        if (Mat44fProperty* dmatrix = property<Mat44fProperty>("color.matrix"))
        {
            if (dmatrix->size() > 0)
            {
                C = dmatrix->front() * C;
            }
        }

        if (m_matrixOutputRGBA)
        {
            m_matrixOutputRGBA->resize(1);
            m_matrixOutputRGBA->front() = C;
        }

        float brightness = propertyValue(m_brightness, 0.0f);

        if (brightness != 0.0)
        {
            const float e = ::pow((double)2.0, (double)brightness);
            Mat44f E;

            E.makeScale(Vec3f(e, e, e));
            B = E * B;
        }

        AssignDisplayState assign(
            overrideColorspace, linear2sRGB, linear2Rec709, displayGamma,
            outOfRange, dither, ditherLast,
            frame + DispTransform2DIPNode::transformHash(), C, Ca, B, this,
            context);

        IPImage* nroot =
            new IPImage(this, IPImage::BlendRenderType, context.viewWidth,
                        context.viewHeight, 1.0, IPImage::IntermediateBuffer,
                        IPImage::FloatDataType);

        root->fitToAspect(nroot->displayAspect());
        nroot->appendChild(root);
        nroot->useBackground = true;
        nroot->shaderExpr = Shader::newSourceRGBA(nroot);
        assign(nroot);

        nroot->resourceUsage =
            nroot->shaderExpr->computeResourceUsageRecursive();

        return nroot;
    }

} // namespace IPCore
