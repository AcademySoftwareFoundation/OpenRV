//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <IPBaseNodes/LinearizeIPNode.h>
#include <IPCore/Exception.h>
#include <IPCore/GroupIPNode.h>
#include <IPCore/ShaderCommon.h>
#include <IPCore/NodeDefinition.h>
#include <IPCore/IPProperty.h>
#include <ImathVec.h>
#include <TwkMath/Chromaticities.h>
#include <ImfChromaticities.h>
#include <ImfRgbaYca.h>
#include <TwkFB/Operations.h>
#include <TwkFB/FrameBuffer.h>
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
    using namespace TwkFB;

    namespace
    {
        inline Imath::V2f convert(Vec2f v) { return Imath::V2f(v.x, v.y); }

        inline Imath::V3f convert(Vec3f v) { return Imath::V3f(v.x, v.y, v.z); }

        inline Vec2f convert(Imath::V2f v) { return Vec2f(v.x, v.y); }

        inline Vec3f convert(Imath::V3f v) { return Vec3f(v.x, v.y, v.z); }

        inline Mat44f convert(const Imath::M44f& m)
        {
            return reinterpret_cast<const Mat44f*>(&m)->transposed();
        }

    } // namespace

    LinearizeIPNode::LinearizeIPNode(const std::string& name,
                                     const NodeDefinition* def, IPGraph* graph,
                                     GroupIPNode* group)
        : IPNode(name, def, graph, group)
        , m_transferName(0)
        , m_primariesName(0)
        , m_alphaTypeName(0)
        , m_active(0)
    {
        setMaxInputs(1);

        //
        //  Since these are string properties we need to protect against
        //  simultaneous reading/writing by causing the graph to shutdown
        //  when changing them. The info here tells the PropertyEditor to
        //  do that.
        //

        PropertyInfo* info = new PropertyInfo(
            PropertyInfo::Persistent | PropertyInfo::RequiresGraphEdit, 1);

        m_active = declareProperty<IntProperty>("node.active", 1);
        m_transferName =
            declareProperty<StringProperty>("color.transfer", "File", info);
        m_primariesName =
            declareProperty<StringProperty>("color.primaries", "File", info);
        m_alphaTypeName =
            declareProperty<StringProperty>("color.alphaType", "File", info);
    }

    LinearizeIPNode::~LinearizeIPNode() {}

    void LinearizeIPNode::setTransfer(const std::string& s)
    {
        setProperty(m_transferName, s);
    }

    void LinearizeIPNode::setPrimaries(const std::string& s)
    {
        setProperty(m_primariesName, s);
    }

    void LinearizeIPNode::setAlphaType(const std::string& s)
    {
        setProperty(m_alphaTypeName, s);
    }

    string LinearizeIPNode::fbPrimariesToPropertyValue(const string& p)
    {
        if (p == ColorSpace::Rec709())
            return "ITU Rec.709";
        else if (p == ColorSpace::sRGB())
            return "ITU Rec.709";
        else if (p == ColorSpace::Rec601())
            return "ITU Rec.601 525 Lines";
        else if (p == ColorSpace::Rec2020())
            return "ITU Rec.2020";
        else if (p == ColorSpace::AdobeRGB())
            return "AdobeRGB";
        else if (p == ColorSpace::SMPTE240M())
            return "SMPTE 240M";
        else if (p == ColorSpace::SMPTE_C())
            return "SMPTE-C";
        else if (p == ColorSpace::XYZ())
            return "CIE XYZ";
        else if (p == ColorSpace::P3())
            return "DCI P3";
        else if (p == ColorSpace::ArriSceneReferred())
            return "ARRI Wide Gamut Scene Referred";
        else if (p == ColorSpace::ArriWideGamut())
            return "ARRI Wide Gamut";
        else if (p == ColorSpace::ACES())
            return "ACES";
        return "File";
    }

    string LinearizeIPNode::fbTransferToPropertyValue(const string& t)
    {
        if (t == ColorSpace::CineonLog())
            return "Cineon Log";
        else if (t == ColorSpace::None())
            return "sRGB"; // really
        else if (t == ColorSpace::sRGB())
            return "sRGB";
        else if (t == ColorSpace::Rec709())
            return "ITU Rec.709";
        else if (t == ColorSpace::Rec2020())
            return "ITU Rec.2020";
        else if (t == ColorSpace::SMPTE240M())
            return "SMPTE 240M";
        else if (t == ColorSpace::ArriLogC())
            return "ARRI LogC";
        else if (t == ColorSpace::ArriLogCFilm())
            return "ARRI LogC Film";
        else if (t == ColorSpace::ViperLog())
            return "Viper Log";
        else if (t == ColorSpace::RedLog())
            return "Red Log";
        else if (t == ColorSpace::RedLogFilm())
            return "Red Log Film";
        else if (t == ColorSpace::Linear())
            return "Linear";
        else if (t == ColorSpace::Gamma18())
            return "Gamma 1.8";
        else if (t == ColorSpace::Gamma22())
            return "Gamma 2.2";
        else if (t == ColorSpace::Gamma24())
            return "Gamma 2.4";
        else if (t == ColorSpace::Gamma26())
            return "Gamma 2.6";
        return "File";
    }

    string
    LinearizeIPNode::chromaticitiesToPropertyValue(const Chromaticities& chr)
    {
        if (chr == Chromaticities::Rec709())
            return "ITU Rec.709";
        else if (chr == Chromaticities::Rec601_525())
            return "ITU Rec.601 525 Lines";
        else if (chr == Chromaticities::Rec601_625())
            return "ITU Rec.601 625 Lines";
        else if (chr == Chromaticities::Rec2020())
            return "ITU Rec.2020";
        else if (chr == Chromaticities::P3())
            return "DCI P3";
        else if (chr == Chromaticities::XYZ())
            return "CIE XYZ";
        else if (chr == Chromaticities::AdobeRGB())
            return "Adobe RGB";
        else if (chr == Chromaticities::SMPTE_C())
            return "SMPTE-C";
        else if (chr == Chromaticities::SMPTE_240M())
            return "SMPTE 240M";
        else if (chr == Chromaticities::ACES())
            return "ACES";
        else if (chr == Chromaticities::ArriSceneReferred())
            return "ARRI Wide Gamut Scene Referred";
        return "File";
    }

    IPImage* LinearizeIPNode::evaluate(const Context& context)
    {
        const bool active = propertyValue<IntProperty>(m_active, 1) != 0;

        typedef TwkMath::Chromaticities<float> CIECoords;

        int frame = context.frame;
        IPImage* img = IPNode::evaluate(context);

        if (!img)
            return IPImage::newNoImage(this, "No Input");
        if (!active)
            return img;

        TwkFB::FrameBuffer* fb = img->fb;

        string transfer = propertyValue(m_transferName, "File");
        string primaries = propertyValue(m_primariesName, "File");
        string alphaType = propertyValue(m_alphaTypeName, "File");
        CIECoords cc = CIECoords::Rec709();
        Vec2f cneutral = cc.white;
        bool changeOfBasis = false;
        bool xyzMatrix = false;
        Mat44f RGBXYZ;
        Mat44f M = Rec709FullRangeRGBToYUV8<float>();
        const float rw709 = M.m00;
        const float gw709 = M.m01;
        const float bw709 = M.m02;

        Mat44f C;
        typedef TwkFB::TypedFBAttribute<Vec2f> V2fAttr;

        //
        //  If input to this node is a blend, prepare img for shaderExpr mods,
        //  etc.
        //

        convertBlendRenderTypeToIntermediate(img);

        //
        //  Alpha Premult/Unpremulted
        //

        bool alreadyUnpremulted = alphaType == "Unpremultiplied";

        if (alphaType == "File" && fb)
        {
            if (const TwkFB::FBAttribute* a = fb->findAttribute("AlphaType"))
            {
                if (const TwkFB::StringAttribute* sa =
                        dynamic_cast<const TwkFB::StringAttribute*>(a))
                {
                    alreadyUnpremulted = sa->value() == "Unpremultiplied";
                }
            }
        }

        img->unpremulted = alreadyUnpremulted;

        //
        //  Determine file primaries if exists
        //

        if (fb && fb->hasPrimaries() && primaries == "File")
        {
        }

        if (primaries == "ITU Rec.709")
            cc = CIECoords::Rec709();
        else if (primaries == "ITU Rec.601 525 Lines")
            cc = CIECoords::Rec601_525();
        else if (primaries == "ITU Rec.601 625 Lines")
            cc = CIECoords::Rec601_625();
        else if (primaries == "ITU Rec.2020")
            cc = CIECoords::Rec2020();
        else if (primaries == "CIE XYZ")
            cc = CIECoords::XYZ();
        else if (primaries == "DCI P3")
            cc = CIECoords::P3();
        else if (primaries == "AdobeRGB")
            cc = CIECoords::AdobeRGB();
        else if (primaries == "SMPTE-C")
            cc = CIECoords::SMPTE_C();
        else if (primaries == "SMPTE 240M")
            cc = CIECoords::SMPTE_240M();
        else if (primaries == "ACES")
            cc = CIECoords::ACES();
        else if (primaries == "ARRI Wide Gamut Scene Referred")
            cc = CIECoords::ArriSceneReferred();
        else if (primaries == "ARRI Wide Gamut")
        {
            //
            //  NB: For Crank we default to the Tonemapped conversion
            //  matrix not the colorimetric conversion matrix since the
            //  user does not really have the option to choose yet.
            //

            RGBXYZ = ArriWideGamutToXYZ<float>();
            xyzMatrix = true;
            cc = CIECoords::Rec709();
        }
        else if (primaries == "RedColor")
            cc = CIECoords::RedColor();
        else if (primaries == "RedColor2")
            cc = CIECoords::RedColor2();
        else if (primaries == "RedColor3")
            cc = CIECoords::RedColor3();
        else if (primaries == "RedColor4")
            cc = CIECoords::RedColor4();
        else if (primaries == "DragonColor")
            cc = CIECoords::DragonColor();
        else if (primaries == "DragonColor2")
            cc = CIECoords::DragonColor2();
        else if (fb && primaries == "File")
        {
            string p = fb->primaryColorSpace();

            //
            //  Use the fb metadata directly
            //

            if (p == ColorSpace::Rec709())
                cc = CIECoords::Rec709();
            else if (p == ColorSpace::sRGB())
                cc = CIECoords::Rec709();
            else if (p == ColorSpace::Rec601())
                cc = CIECoords::Rec601_525();
            else if (p == ColorSpace::AdobeRGB())
                cc = CIECoords::AdobeRGB();
            else if (p == ColorSpace::SMPTE240M())
                cc = CIECoords::SMPTE_240M();
            else if (p == ColorSpace::SMPTE_C())
                cc = CIECoords::SMPTE_C();
            else if (p == ColorSpace::XYZ())
                cc = CIECoords::XYZ();
            else if (p == ColorSpace::P3())
                cc = CIECoords::P3();
            else if (p == ColorSpace::ArriSceneReferred())
                cc = CIECoords::ArriSceneReferred();
            else if (p == ColorSpace::Rec2020())
                cc = CIECoords::Rec2020();
            else if (p == ColorSpace::ACES())
                cc = CIECoords::ACES();
            else if (p == ColorSpace::RedColor())
                cc = CIECoords::RedColor();
            else if (p == ColorSpace::RedColor2())
                cc = CIECoords::RedColor2();
            else if (p == ColorSpace::RedColor3())
                cc = CIECoords::RedColor3();
            else if (p == ColorSpace::RedColor4())
                cc = CIECoords::RedColor4();
            else if (p == ColorSpace::DragonColor())
                cc = CIECoords::DragonColor();
            else if (p == ColorSpace::DragonColor2())
                cc = CIECoords::DragonColor2();
            else if (p == ColorSpace::ArriWideGamut()
                     && !fb->hasRGBtoXYZMatrix())
            {
                //
                //  If its marked as ArriWideGamut but there's no RGB->XYZ use
                //  the one we know about
                //

                RGBXYZ = ArriWideGamutToXYZ<float>();
                xyzMatrix = true;
                cc = CIECoords::Rec709();
            }
            else if (fb->hasPrimaryColorSpace() && !fb->hasPrimaries())
            {
                //
                //  In this case we don't know what the color space means so
                //  just make it 709
                //

                cc = CIECoords::Rec709();
            }
            else if (fb->hasRGBtoXYZMatrix())
            {
                RGBXYZ = fb->attribute<Mat44f>(ColorSpace::RGBtoXYZMatrix());
                xyzMatrix = true;
                cc = CIECoords::Rec709();
            }
            else if (fb->hasPrimaries())
            {
                cc.white = fb->attribute<Vec2f>(ColorSpace::WhitePrimary());
                cc.red = fb->attribute<Vec2f>(ColorSpace::RedPrimary());
                cc.green = fb->attribute<Vec2f>(ColorSpace::GreenPrimary());
                cc.blue = fb->attribute<Vec2f>(ColorSpace::BluePrimary());
            }
            else
            {
                cc = CIECoords::Rec709();
            }
        }
        else if (fb && fb->hasRGBtoXYZMatrix())
        {
            RGBXYZ = fb->attribute<Mat44f>(ColorSpace::RGBtoXYZMatrix());
            xyzMatrix = true;
            cc = CIECoords::Rec709();
        }
        else if (fb && fb->hasPrimaries())
        {
            cc.white = fb->attribute<Vec2f>(ColorSpace::WhitePrimary());
            cc.red = fb->attribute<Vec2f>(ColorSpace::RedPrimary());
            cc.green = fb->attribute<Vec2f>(ColorSpace::GreenPrimary());
            cc.blue = fb->attribute<Vec2f>(ColorSpace::BluePrimary());
        }
        else
        {
            cc = CIECoords::Rec709();
        }

        if (fb && fb->hasAdoptedNeutral())
        {
            cneutral = fb->attribute<Vec2f>(ColorSpace::AdoptedNeutral());
            changeOfBasis = true;
        }
        else
        {
            cneutral = cc.white;
            changeOfBasis = cc != CIECoords::Rec709() || xyzMatrix;
        }

        //
        //  If we've decided above that the RGB space is not Rec709 we
        //  need to do a change of basis to Rec709
        //

        if (changeOfBasis)
        {
            Imf::Chromaticities chr709;

            if (xyzMatrix)
            {
                Mat44f A;
                TwkFB::colorSpaceConversionMatrix(
                    (const float*)&chr709, (const float*)&chr709,
                    (const float*)&cneutral, (const float*)&chr709.white, true,
                    (float*)&A);

                Mat44f m1 = convert(XYZtoRGB(Imf::Chromaticities(), 1.0));
                C = m1 * A * RGBXYZ * C;
            }
            else
            {
                Imf::Chromaticities chr(convert(cc.red), convert(cc.green),
                                        convert(cc.blue), convert(cc.white));

                TwkFB::colorSpaceConversionMatrix(
                    (const float*)&chr, (const float*)&chr709,
                    (const float*)&cneutral, (const float*)&chr709.white, true,
                    (float*)&C);
            }
        }

        if (const TwkFB::FloatAttribute* lsa =
                dynamic_cast<const TwkFB::FloatAttribute*>(
                    fb->findAttribute(ColorSpace::LinearScale())))
        {
            Mat44f S;
            Vec3f scl(lsa->value());
            S.makeScale(scl);
            C = S * C;
        }

        if (C != Mat44f())
        {
            img->shaderExpr = Shader::newColorMatrix(img->shaderExpr, C);
        }

        //
        //  Transfer function
        //

        if (transfer == "Cineon Log")
        {
            double cinblack = 95;
            double cinwhite = 685;
            double cinbreakpoint = cinwhite;

            if (fb)
            {
                if (const TwkFB::FloatAttribute* fattr =
                        dynamic_cast<const TwkFB::FloatAttribute*>(
                            fb->findAttribute(ColorSpace::BlackPoint())))
                {
                    float ftmp = fattr->value();
                    if (ftmp >= 1.0f)
                        cinblack = (double)ftmp;
                }

                if (const TwkFB::FloatAttribute* fattr =
                        dynamic_cast<const TwkFB::FloatAttribute*>(
                            fb->findAttribute(ColorSpace::WhitePoint())))
                {
                    float ftmp = fattr->value();
                    if (ftmp >= 1.0f)
                        cinwhite = (double)ftmp;
                    cinbreakpoint = cinwhite;
                }

                if (const TwkFB::FloatAttribute* fattr =
                        dynamic_cast<const TwkFB::FloatAttribute*>(
                            fb->findAttribute(ColorSpace::BreakPoint())))
                {
                    float ftmp = fattr->value();
                    if (ftmp >= 1.0f)
                        cinbreakpoint = (double)ftmp;
                }
            }

            img->shaderExpr = Shader::newColorCineonLogToLinear(
                img->shaderExpr, cinblack, cinwhite,
                (cinwhite - cinbreakpoint));
        }
        else if (transfer == "ACES ADX")
        {
            img->shaderExpr = Shader::newColorACESLogToLinear(img->shaderExpr);
        }
        else if (transfer == "Viper Log")
        {
            img->shaderExpr = Shader::newColorViperLogToLinear(img->shaderExpr);
        }
        else if (transfer == "Red Log")
        {
            img->shaderExpr = Shader::newColorRedLogToLinear(img->shaderExpr);
        }
        else if (transfer == "Red Log Film")
        {
            // Red Log Film is Cineon Log; see Aces v1.0.1 distribution.
            double cinblack = 95;
            double cinwhite = 685;
            double cinbreakpoint = cinwhite;

            img->shaderExpr = Shader::newColorCineonLogToLinear(
                img->shaderExpr, cinblack, cinwhite,
                (cinwhite - cinbreakpoint));
        }
        else if ((transfer == "ARRI LogC") || (transfer == "ARRI LogC Film"))
        {
            if (transfer == "ARRI LogC Film")
            {
                //
                // Undo the Film Style Matrix; this is done prior to linearizing
                // the logC.
                //
                img->shaderExpr = Shader::newColorMatrix(
                    img->shaderExpr, ArriFilmStyleInverseMatrix<float>());
            }

            TwkFB::LogCTransformParams params;

            TwkFB::getLogCCurveParams(params, fb);

            img->shaderExpr = Shader::newColorLogCLinear(
                img->shaderExpr,
                params.LogCBlackSignal,     // pbs
                params.LogCEncodingOffset,  // eo
                params.LogCEncodingGain,    // eg
                params.LogCGraySignal,      // gs
                params.LogCBlackOffset,     // bo
                params.LogCLinearSlope,     // ls
                params.LogCLinearOffset,    // lo
                params.LogCLinearCutPoint); // // logc to linear cutoff
        }
        else if (transfer == "sRGB")
        {
            img->shaderExpr = Shader::newColorSRGBToLinear(img->shaderExpr);
        }
        else if (transfer == "ITU Rec.709")
        {
            img->shaderExpr = Shader::newColorRec709ToLinear(img->shaderExpr);
        }
        else if (transfer == "Apple Rec.709")
        {
            Vec3f g(1.961);
            img->shaderExpr = Shader::newColorGamma(img->shaderExpr, g);
        }
        else if (transfer == "ITU Rec.2020")
        {
            // add this
        }
        else if (transfer == "SMPTE 240M")
        {
            img->shaderExpr =
                Shader::newColorSMPTE240MToLinear(img->shaderExpr);
        }
        else if (transfer == "Gamma 1.8")
        {
            Vec3f g(1.8);
            img->shaderExpr = Shader::newColorGamma(img->shaderExpr, g);
        }
        else if (transfer == "Gamma 2.2")
        {
            Vec3f g(2.2);
            img->shaderExpr = Shader::newColorGamma(img->shaderExpr, g);
        }
        else if (transfer == "Gamma 2.4")
        {
            Vec3f g(2.4);
            img->shaderExpr = Shader::newColorGamma(img->shaderExpr, g);
        }
        else if (transfer == "Gamma 2.6")
        {
            Vec3f g(2.6);
            img->shaderExpr = Shader::newColorGamma(img->shaderExpr, g);
        }
        else if (transfer == "Linear")
        {
            // nothing
        }

        //
        //  Alpha
        //

        if (img->unpremulted)
        {
            img->shaderExpr = Shader::newColorPremult(img->shaderExpr);
        }

        return img;
    }

} // namespace IPCore
