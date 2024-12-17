//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <RvApp/FileSpaceLinearizeIPNode.h>
#include <IPCore/Exception.h>
#include <IPCore/GroupIPNode.h>
#include <IPCore/ShaderCommon.h>
#include <ImathVec.h>
#include <ImfChromaticities.h>
#include <ImfRgbaYca.h>
#include <TwkFB/Operations.h>
#include <TwkMath/Function.h>
#include <TwkMath/Iostream.h>
#include <TwkMath/Vec3.h>
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

    //
    //  LUTS:
    //
    //  Luminance LUT:          type == "Luminance", dimensions == 1
    //

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

#define NO_FLOAT_3D_TEXTURES

    FileSpaceLinearizeIPNode::FileSpaceLinearizeIPNode(
        const std::string& name, const NodeDefinition* def, IPGraph* graph,
        GroupIPNode* group)
        : LUTIPNode(name, def, graph, group)
    {
        setMaxInputs(1);

        m_colorAlphaType = declareProperty<IntProperty>("color.alphaType", 0);
        m_colorLogType = declareProperty<IntProperty>("color.logtype", 0);
        m_cineonWhite =
            declareProperty<IntProperty>("cineon.whiteCodeValue", 0);
        m_cineonBlack =
            declareProperty<IntProperty>("cineon.blackCodeValue", 0);
        m_cineonBreakPoint =
            declareProperty<IntProperty>("cineon.breakPointValue", 0);
        m_colorYUV = declareProperty<IntProperty>("color.YUV", 0);

        // XXX we may have to always write these to file, since if they have
        // been set to "Linear" for some reason (IE we have a linear DPX file or
        // somethign), then when we load that session file, source_setup will
        // think they need to be configured (since they were not set in file).

        m_colorsRGB2Linear =
            declareProperty<IntProperty>("color.sRGB2linear", 0);
        m_colorRec7092Linear =
            declareProperty<IntProperty>("color.Rec709ToLinear", 0);
        m_colorFileGamma =
            declareProperty<FloatProperty>("color.fileGamma", 1.0f);
        m_colorActive = declareProperty<IntProperty>("color.active", 1);
        m_colorIgnoreChromaticities =
            declareProperty<IntProperty>("color.ignoreChromaticities", 0);
        m_CDLactive = declareProperty<IntProperty>("CDL.active", 0);
        m_CDLslope = declareProperty<Vec3fProperty>("CDL.slope", Vec3f(1.0f));
        m_CDLoffset = declareProperty<Vec3fProperty>("CDL.offset", Vec3f(0.0f));
        m_CDLpower = declareProperty<Vec3fProperty>("CDL.power", Vec3f(1.0f));
        m_CDLsaturation =
            declareProperty<FloatProperty>("CDL.saturation", 1.0f);
        m_CDLnoclamp = declareProperty<IntProperty>("CDL.noClamp", 0);
    }

    FileSpaceLinearizeIPNode::~FileSpaceLinearizeIPNode() {}

    void FileSpaceLinearizeIPNode::setLogLin(int t)
    {
        setProperty(m_colorLogType, t);
    }

    void FileSpaceLinearizeIPNode::setSRGB(int t)
    {
        setProperty(m_colorsRGB2Linear, t);
    }

    void FileSpaceLinearizeIPNode::setRec709(int t)
    {
        setProperty(m_colorRec7092Linear, t);
    }

    void FileSpaceLinearizeIPNode::setFileGamma(float y)
    {
        setProperty(m_colorFileGamma, y);
    }

    IPImage* FileSpaceLinearizeIPNode::evaluate(const Context& context)
    {
        IPImage* head = IPNode::evaluate(context);

        if (!head)
            return IPImage::newNoImage(this, "No Input");

        if (!head->shaderExpr && head->children && head->noIntermediate)
        {
            // If the head node has no shaderExpr, has children and does not
            // support intermediate renders, we must explicitly loop over the
            // children and apply the linearization logic to each of them. This
            // usually happens in the context of a tiled-source.
            size_t i = 0;
            for (IPImage* child = head->children; child;
                 child = child->next, i++)
            {
                evaluateOne(child, context);
            }
        }
        else
        {
            //
            //  If input to this node is a blend, prepare head for shaderExpr
            //  mods, etc.
            //
            convertBlendRenderTypeToIntermediate(head);
            evaluateOne(head, context);
        }

        return head;
    }

    void FileSpaceLinearizeIPNode::evaluateOne(IPImage* img,
                                               const Context& context)
    {
        Vec2f cred;
        Vec2f cgreen;
        Vec2f cblue;
        Vec2f cwhite;
        Vec2f cneutral;
        bool rec709 = true;
        bool xyzMatrix = false;
        Mat44f RGBXYZ;

        //
        //  The intent of this CDL operation is for it to happen in "File
        //  Space", so do it before anything else.
        //

        const bool active = propertyValue<IntProperty>(m_colorActive, 1) != 0;
        const bool CDLactive = propertyValue<IntProperty>(m_CDLactive, 1) != 0;

        Vec3f CDL_offset =
            propertyValue<Vec3fProperty>(m_CDLoffset, Vec3f(0.0f));
        Vec3f CDL_slope = propertyValue<Vec3fProperty>(m_CDLslope, Vec3f(1.0f));
        Vec3f CDL_power = propertyValue<Vec3fProperty>(m_CDLpower, Vec3f(1.0f));
        float CDL_saturation =
            propertyValue<FloatProperty>(m_CDLsaturation, 1.0f);

        if (CDLactive
            && (CDL_offset != Vec3f(0.0f) || CDL_slope != Vec3f(1.0f)
                || CDL_power != Vec3f(1.0f) || CDL_saturation != 1.0f))
        {
            const bool noClamp =
                (propertyValue<IntProperty>(m_CDLnoclamp, 0) != 0);

            img->shaderExpr =
                Shader::newColorCDL(img->shaderExpr, CDL_slope, CDL_offset,
                                    CDL_power, CDL_saturation, noClamp);
        }

        if (img->fb && img->fb->hasRGBtoXYZMatrix())
        {
            RGBXYZ =
                img->fb->attribute<Mat44f>(TwkFB::ColorSpace::RGBtoXYZMatrix());
            xyzMatrix = true;
            rec709 = false;

            cwhite = Vec2f(0.3127f, 0.3290f);
            cred = Vec2f(0.6400f, 0.3300f);
            cgreen = Vec2f(0.3000f, 0.6000f);
            cblue = Vec2f(0.1500f, 0.0600f);
        }
        else if (img->fb && img->fb->hasPrimaries())
        {
            cwhite =
                img->fb->attribute<Vec2f>(TwkFB::ColorSpace::WhitePrimary());
            cred = img->fb->attribute<Vec2f>(TwkFB::ColorSpace::RedPrimary());
            cgreen =
                img->fb->attribute<Vec2f>(TwkFB::ColorSpace::GreenPrimary());
            cblue = img->fb->attribute<Vec2f>(TwkFB::ColorSpace::BluePrimary());
            rec709 = false;
        }
        else
        {
            cwhite = Vec2f(0.3127f, 0.3290f);
            cred = Vec2f(0.6400f, 0.3300f);
            cgreen = Vec2f(0.3000f, 0.6000f);
            cblue = Vec2f(0.1500f, 0.0600f);
        }

        if (img->fb && img->fb->hasAdoptedNeutral())
        {
            cneutral =
                img->fb->attribute<Vec2f>(TwkFB::ColorSpace::AdoptedNeutral());
            rec709 = false;
        }
        else
        {
            cneutral = cwhite;
        }

        Mat44f M = Rec709FullRangeRGBToYUV8<float>();
        const float rw709 = M.m00;
        const float gw709 = M.m01;
        const float bw709 = M.m02;

        bool alreadyUnpremulted = img->unpremulted;

        //
        //  Allow alpha type setting to be used regardless of color.active value
        //

        if (IntProperty* atype = m_colorAlphaType)
        {
            if (atype->front() == 0)
            {
                if (img->fb)
                {
                    if (const TwkFB::FBAttribute* a =
                            img->fb->findAttribute("AlphaType"))
                    {
                        if (const TwkFB::StringAttribute* sa =
                                dynamic_cast<const TwkFB::StringAttribute*>(a))
                        {
                            alreadyUnpremulted =
                                sa->value() == "Unpremultiplied";
                        }
                    }
                }
            }
            else
            {
                alreadyUnpremulted = atype->front() == 2;
            }
        }

        img->unpremulted = alreadyUnpremulted;

        if (active)
        {
            if (IntProperty* loglin = m_colorLogType)
            {
                int logtype = loglin->front();

                if (logtype == 1) // Kodak Cineon Log
                {
                    double cinblack = 95;

                    if (IntProperty* black = m_cineonBlack)
                    {
                        if (!black->empty() && black->front() != 0)
                        {
                            cinblack = black->front();
                        }
                        else
                        {
                            if (img->fb)
                            {
                                if (const TwkFB::FloatAttribute* fattr =
                                        dynamic_cast<
                                            const TwkFB::FloatAttribute*>(
                                            img->fb->findAttribute(
                                                TwkFB::ColorSpace::
                                                    BlackPoint())))
                                {
                                    float ftmp = fattr->value();
                                    if (ftmp >= 1.0f)
                                    {
                                        cinblack = (double)ftmp;
                                    }
                                }
                            }
                        }
                    }

                    double cinwhite = 685;
                    if (IntProperty* white = m_cineonWhite)
                    {
                        if (!white->empty() && white->front() != 0)
                        {
                            cinwhite = white->front();
                        }
                        else
                        {
                            if (img->fb)
                            {
                                if (const TwkFB::FloatAttribute* fattr =
                                        dynamic_cast<
                                            const TwkFB::FloatAttribute*>(
                                            img->fb->findAttribute(
                                                TwkFB::ColorSpace::
                                                    WhitePoint())))
                                {
                                    float ftmp = fattr->value();
                                    if (ftmp >= 1.0f)
                                    {
                                        cinwhite = (double)ftmp;
                                    }
                                }
                            }
                        }
                    }

                    double cinbreakpoint = cinwhite;
                    if (IntProperty* breakpoint = m_cineonBreakPoint)
                    {
                        if (!breakpoint->empty() && breakpoint->front() != 0)
                        {
                            cinbreakpoint = breakpoint->front();
                        }
                        else
                        {
                            if (img->fb)
                            {
                                if (const TwkFB::FloatAttribute* fattr =
                                        dynamic_cast<
                                            const TwkFB::FloatAttribute*>(
                                            img->fb->findAttribute(
                                                TwkFB::ColorSpace::
                                                    BreakPoint())))
                                {
                                    float ftmp = fattr->value();
                                    if (ftmp >= 1.0f)
                                    {
                                        cinbreakpoint = (double)ftmp;
                                    }
                                }
                            }
                        }
                    }

                    img->shaderExpr = Shader::newColorCineonLogToLinear(
                        img->shaderExpr, cinblack, cinwhite,
                        (cinwhite - cinbreakpoint));
                }
                else if (logtype == 2)
                {
                    img->shaderExpr =
                        Shader::newColorViperLogToLinear(img->shaderExpr);
                }
                else if ((logtype == 3) || (logtype == 4)) // logC or logC_Film
                {
                    if (logtype == 4) //  logC_Film
                    {
                        //
                        // Undo the Film Style Matrix; this is done prior to
                        // linearizing the logC.
                        //
                        img->shaderExpr = Shader::newColorMatrix(
                            img->shaderExpr,
                            ArriFilmStyleInverseMatrix<float>());
                    }

                    TwkFB::LogCTransformParams params;

                    TwkFB::getLogCCurveParams(params, img->fb);

                    img->shaderExpr = Shader::newColorLogCLinear(
                        img->shaderExpr,
                        params.LogCBlackSignal,     // pbs
                        params.LogCEncodingOffset,  // eo
                        params.LogCEncodingGain,    // eg
                        params.LogCGraySignal,      // gs
                        params.LogCBlackOffset,     // bo
                        params.LogCLinearSlope,     // ls
                        params.LogCLinearOffset,    // lo
                        params.LogCLinearCutPoint); // logc to linear cutoff
                }
                else if (logtype == 6)
                {
                    img->shaderExpr =
                        Shader::newColorRedLogToLinear(img->shaderExpr);
                }
                else if (logtype == 7) // RedLogFilm
                {
                    // Red Log Film is Cineon Log
                    double cinblack = 95;
                    double cinwhite = 685;
                    double cinbreakpoint = cinwhite;

                    img->shaderExpr = Shader::newColorCineonLogToLinear(
                        img->shaderExpr, cinblack, cinwhite,
                        (cinwhite - cinbreakpoint));
                }
            }
        }

        //
        //  FILE lut
        //

        addLUTPipeline(context, img);

        if (active)
        {
            if (IntProperty* srgb = m_colorsRGB2Linear)
            {
                if (srgb->size() && srgb->front())
                {
                    img->shaderExpr =
                        Shader::newColorSRGBToLinear(img->shaderExpr);
                }
            }

            if (IntProperty* rec7092L = m_colorRec7092Linear)
            {
                if (rec7092L->size() && rec7092L->front())
                {
                    img->shaderExpr =
                        Shader::newColorRec709ToLinear(img->shaderExpr);
                }
            }

            if (FloatProperty* fileGamma = m_colorFileGamma)
            {
                Vec3f g = Vec3f(fileGamma->front());

                if (g != Vec3f(1.0))
                {
                    // if (g.x != 0.f) g.x = 1.0f / g.x;
                    // else g.x = Math<float>::max();

                    // if (g.y != 0.f) g.y = 1.0f / g.y;
                    // else g.y = Math<float>::max();

                    // if (g.z != 0.f) g.z = 1.0f / g.z;
                    // else g.z = Math<float>::max();

                    img->shaderExpr = Shader::newColorGamma(img->shaderExpr, g);
                }
            }
        }

        Mat44f C;
        typedef TwkFB::TypedFBAttribute<Vec2f> V2fAttr;

        if (!rec709)
        {
            if (IntProperty* ignoreColorSpace = m_colorIgnoreChromaticities)
            {
                if (ignoreColorSpace->front() == 0)
                {
                    Imf::Chromaticities chr709;
                    if (xyzMatrix)
                    {
                        Mat44f A;
                        TwkFB::colorSpaceConversionMatrix(
                            (const float*)&chr709, (const float*)&chr709,
                            (const float*)&cneutral,
                            (const float*)&chr709.white, true, (float*)&A);

                        Mat44f m1 =
                            convert(XYZtoRGB(Imf::Chromaticities(), 1.0));
                        C = m1 * A * RGBXYZ * C;
                    }
                    else
                    {
                        Imf::Chromaticities chr(convert(cred), convert(cgreen),
                                                convert(cblue),
                                                convert(cwhite));
                        TwkFB::colorSpaceConversionMatrix(
                            (const float*)&chr, (const float*)&chr709,
                            (const float*)&cneutral,
                            (const float*)&chr709.white, true, (float*)&C);
                    }
                }
            }
        }

        // We should investigate the real impact of this check.
        if (img->fb)
        {
            if (const TwkFB::FloatAttribute* lsa =
                    dynamic_cast<const TwkFB::FloatAttribute*>(
                        img->fb->findAttribute(
                            TwkFB::ColorSpace::LinearScale())))
            {
                Mat44f S;
                Vec3f scl(lsa->value());
                S.makeScale(scl);
                C = S * C;
            }
        }

        if (IntProperty* yuv = m_colorYUV)
        {
            if (yuv->size() && yuv->front())
            {
                // RGB -> YUV
                // [0,1] -> [[0,1], [-.5,.5], [-.5,.5]]
                //
                // [ [      0.299,           0.587,           0.114,       0  ]
                //   [ -0.168735891648, -0.331264108352,       0.5,        0. ]
                //   [       0.5,       -0.418687589158, -0.0813124108417, 0. ]
                //   [        0,               0,               0,         1  ]
                //   ]
                //
                // inverse:
                // [ [ 1.,     2.8e-13,          1.402,      0. ]
                //   [ 1., -0.344136286201, -0.714136286201, 0. ]
                //   [ 1.,      1.772,        5.4932e-13,    0. ]
                //   [ 0.,       0.,              0.,        1. ] ]
                //
                // [ [ 1, 0, 0,  0   ]
                //   [ 0, 1, 0, -0.5 ]
                //   [ 0, 0, 1, -0.5 ]
                //   [ 0, 0, 0,  1   ] ]
                //

                Mat44f T(1., 0, 1.402, 0., 1., -0.344136286201, -0.714136286201,
                         0., 1., 1.772, 0, 0., 0., 0., 0., 1.);

                C = T * C;
            }
        }

        //
        //  XXX I think maybe this is wrong.  The color changes applied here are
        //  not "artistic" and you would expect them to operate on the RGB
        //  values regardless of the alpha.  I think.
        //
        //  Yes, wrong - removing this for now.
        //

        bool unpremult =
            false; // (C != Mat44f() && active) && !alreadyUnpremulted;
        bool willPremult = unpremult || alreadyUnpremulted;

        if (unpremult)
        {
            // add unpremult shader
            img->shaderExpr = Shader::newColorUnpremult(img->shaderExpr);
        }
        else if (willPremult)
        {
            // add stash alpha shader
        }

        if (C != Mat44f() && active)
        {
            img->shaderExpr = Shader::newColorMatrix(img->shaderExpr, C);
        }

        if (willPremult)
        {
            // premult shader
            if (unpremult)
            {
                // We have done an earlier unpremult shader op because there was
                // a color operation to perform.
                img->shaderExpr = Shader::newColorPremultLight(img->shaderExpr);
            }
            else
            {
                img->shaderExpr = Shader::newColorPremult(img->shaderExpr);
            }
        }
    }

} // namespace IPCore
