//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <IPBaseNodes/ColorIPNode.h>
#include <IPCore/Exception.h>
#include <IPCore/GroupIPNode.h>
#include <IPCore/ShaderCommon.h>
#include <ImfRgbaYca.h>
#include <ImfChromaticities.h>
#include <ImathVec.h>
#include <TwkMath/Function.h>
#include <TwkMath/Vec3.h>
#include <TwkMath/Iostream.h>
#include <TwkMath/MatrixColor.h>
#include <TwkMath/Chromaticities.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stl_ext/string_algo.h>
#include <TwkFB/Operations.h>

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

    ColorIPNode::ColorIPNode(const std::string& name, const NodeDefinition* def,
                             IPGraph* graph, GroupIPNode* group)
        : LUTIPNode(name, def, graph, group)
    {
        setMaxInputs(1);

        m_lumLUTfb = 0;

        Property::Info* info = new Property::Info();
        info->setPersistent(false);

        m_colorInvert = declareProperty<IntProperty>("color.invert", 0);
        m_colorGamma = declareProperty<Vec3fProperty>("color.gamma",
                                                      Vec3f(1.0f, 1.0f, 1.0f));
        m_colorLUT = declareProperty<StringProperty>("color.lut", "default");
        m_colorOffset =
            declareProperty<Vec3fProperty>("color.offset", Vec3f(0.0f));
        m_colorScale = declareProperty<Vec3fProperty>("color.scale",
                                                      Vec3f(1.0f, 1.0f, 1.0f));
        m_colorExposure =
            declareProperty<Vec3fProperty>("color.exposure", Vec3f(0.0f));
        m_colorContrast =
            declareProperty<Vec3fProperty>("color.contrast", Vec3f(0.0f));
        m_colorSaturation =
            declareProperty<FloatProperty>("color.saturation", 1.0f);
        m_colorNormalize = declareProperty<IntProperty>("color.normalize", 0);
        m_colorHue = declareProperty<FloatProperty>("color.hue", 0.0f);
        m_colorActive = declareProperty<IntProperty>("color.active", 1);
        m_colorUnpremult = declareProperty<IntProperty>("color.unpremult", 0);
        m_CDLactive = declareProperty<IntProperty>("CDL.active", 0);
        m_CDLcolorspace =
            declareProperty<StringProperty>("CDL.colorspace", "rec709");
        m_CDLslope = declareProperty<Vec3fProperty>("CDL.slope",
                                                    Vec3f(1.0f, 1.0f, 1.0f));
        m_CDLoffset = declareProperty<Vec3fProperty>("CDL.offset", Vec3f(0.0f));
        m_CDLpower = declareProperty<Vec3fProperty>("CDL.power",
                                                    Vec3f(1.0f, 1.0f, 1.0f));
        m_CDLsaturation =
            declareProperty<FloatProperty>("CDL.saturation", 1.0f);
        m_CDLnoclamp = declareProperty<IntProperty>("CDL.noClamp", 0);
        m_lumlutLUT = declareProperty<FloatProperty>("luminanceLUT.lut");
        m_lumlutMax = declareProperty<FloatProperty>("luminanceLUT.max", 1.0f);
        m_lumlutSize = declareProperty<IntProperty>("luminanceLUT.size", 0);
        m_lumlutName = declareProperty<StringProperty>("luminanceLUT.name", "");
        m_lumlutActive = declareProperty<IntProperty>("luminanceLUT.active", 0);
        m_lumlutOutputSize =
            declareProperty<IntProperty>("luminanceLUT:output.size", 256);
        m_lumlutOutputType = declareProperty<StringProperty>(
            "luminanceLUT:output.type", "Luminance", info);
        m_lumlutOutputLUT =
            declareProperty<HalfProperty>("luminanceLUT:output.lut", info);
        m_matrixOutputRGBA = declareProperty<Mat44fProperty>(
            "matrix:output.RGBA", Mat44f(), info);
    }

    ColorIPNode::~ColorIPNode() { delete m_lumLUTfb; }

    void ColorIPNode::setGamma(float y)
    {
        m_colorGamma->resize(1);
        m_colorGamma->front() = y;
    }

    void ColorIPNode::setExposure(float e)
    {
        m_colorExposure->resize(1);
        m_colorExposure->front() = e;
    }

    inline float lerp1DLUT(FloatProperty* lut, float range, size_t c, float v)
    {
        const float n = lut->size() / 3 - 1;
        const size_t i0 = size_t(v * n);
        const size_t i1 = size_t(i0 >= n ? n : i0 + 1);
        const float d = v * n - float(i0);

        const float v0 = (*lut)[i0 * 3 + c];
        const float v1 = (*lut)[i1 * 3 + c];

        return lerp(v0, v1, d);
    }

    void ColorIPNode::readCompleted(const std::string& type,
                                    unsigned int version)
    {
        IPNode::readCompleted(type, version);

        if (const IntProperty* ip = m_lumlutActive)
        {
            if (!ip->empty() && ip->front())
                generateLUT();
        }
    }

    void ColorIPNode::propertyChanged(const Property* property)
    {
        IPNode::propertyChanged(property);

        if (property == m_lumlutLUT || property == m_lumlutSize)
            generateLUT();
    }

    void ColorIPNode::generateLUT()
    {
        delete m_lumLUTfb;
        m_lumLUTfb = 0;

        //
        //  Generate a LUT for the log conversion and gamma
        //

        IntProperty* outSize = m_lumlutOutputSize;
        HalfProperty* out = m_lumlutOutputLUT;
        FloatProperty* lut = m_lumlutLUT;
        FloatProperty* range = m_lumlutMax;
        IntProperty* sizes = m_lumlutSize;

        if (!lut || lut->empty())
            return;

        if (!outSize->size() || outSize->front() == 0)
        {
            outSize->resize(1);
            (*outSize)[0] = 256;
        }

        size_t n = outSize->front();
        const float n2 = lut->size() - 1;
        unsigned int h = n;

        out->resize(n * 3);

        const float lmax = range->size() ? range->front() : 1.0;

        for (int i = 0; i < n; i++)
        {
            const float cindex = float(i) / float(n - 1);
            Vec3f c = Vec3f(cindex);

            c.x = lerp1DLUT(lut, lmax, 0, c.x);
            c.y = lerp1DLUT(lut, lmax, 1, c.y);
            c.z = lerp1DLUT(lut, lmax, 2, c.z);

            (*out)[i * 3 + 0] = c.x;
            (*out)[i * 3 + 1] = c.y;
            (*out)[i * 3 + 2] = c.z;

            h ^= char(c.x * 255) | char(c.y * 255) << 8 | char(c.z * 255) << 16;
            h = h << 8 | h >> 24;
        }

        m_lumLUTfb = new FrameBuffer(n, 1, 3, FrameBuffer::HALF,
                                     (unsigned char*)&out->front(), 0,
                                     FrameBuffer::NATURAL, false);

        m_lumLUTfb->idstream() << h << ":" << size_t(m_lumLUTfb);
    }

    IPImage* ColorIPNode::evaluate(const Context& context)
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

    void ColorIPNode::evaluateOne(IPImage* img, const Context& context)
    {
        Vec2f cred;
        Vec2f cgreen;
        Vec2f cblue;
        Vec2f cwhite;
        Vec2f cneutral;
        bool rec709 = true;
        bool xyzMatrix = false;
        Mat44f RGBXYZ;

        const bool active = propertyValue<IntProperty>(m_colorActive, 1) != 0;
        const bool CDLactive = propertyValue<IntProperty>(m_CDLactive, 1) != 0;
        const bool unpremultProp =
            propertyValue<IntProperty>(m_colorUnpremult, 0) != 0;

        Mat44f C;
        typedef TwkFB::TypedFBAttribute<Vec2f> V2fAttr;

        if (IntProperty* n = m_colorNormalize)
        {
            if (n->front() == 1 && img->fb)
            {
                if (TwkFB::FBAttribute* a =
                        img->fb->findAttribute("ColorBounds"))
                {
                    if (V2fAttr* va = dynamic_cast<V2fAttr*>(a))
                    {
                        const Vec2f v = va->value();
                        Mat44f T;
                        Mat44f S;
                        const float d = 1.0 / (v.y - v.x);
                        T.makeTranslation(Vec3f(-v.x, -v.x, -v.x));
                        S.makeScale(Vec3f(d, d, d));
                        C = T * S * C;
                    }
                }
            }
        }

        if (IntProperty* invert = m_colorInvert)
        {
            if (invert->size() && invert->front())
            {
                Mat44f I(-1, 0, 0, 1, 0, -1, 0, 1, 0, 0, -1, 1, 1, 1, 1, 0);

                C = I * C;
            }
        }

        //
        //  Install resize resampler
        //

        bool hasGamma = false;
        Vec3f gammaValue = Vec3f(1.0f);

        if (Vec3fProperty* gamma = m_colorGamma)
        {
            Vec3f g = gamma->front();

            if (g != Vec3f(1.0))
            {
                hasGamma = true;

                if (g.x != 0.f)
                    g.x = 1.0f / g.x;
                else
                    g.x = Math<float>::max();

                if (g.y != 0.f)
                    g.y = 1.0f / g.y;
                else
                    g.y = Math<float>::max();

                if (g.z != 0.f)
                    g.z = 1.0f / g.z;
                else
                    g.z = Math<float>::max();

                gammaValue = g;
            }
        }

        if (Vec3fProperty* offset = m_colorOffset)
        {
            if (offset->front() != Vec3f(0.0f))
            {
                Mat44f T;
                T.makeTranslation(offset->front());
                C = T * C;
            }
        }

        if (Vec3fProperty* scale = m_colorScale)
        {
            if (scale->front() != Vec3f(1.0f, 1.0f, 1.0f))
            {
                Mat44f S;
                S.makeScale(scale->front());
                C = S * C;
            }
        }

        if (Vec3fProperty* exposure = m_colorExposure)
        {
            if (exposure->front() != Vec3f(0.0f))
            {
                Vec3f e = exposure->front();
                float e0 = ::pow(2.0, double(e.x));
                float e1 = ::pow(2.0, double(e.y));
                float e2 = ::pow(2.0, double(e.z));
                Mat44f E;

                E.makeScale(Vec3f(e0, e1, e2));
                C = E * C;
            }
        }

        if (Vec3fProperty* contrast = m_colorContrast)
        {
            const Vec3f c = contrast->front();

            const Vec3f s = (Vec3f(1.0) + c);
            const Vec3f d = -c * 0.5;

            Mat44f M(s.x, 0, 0, d.x, 0, s.y, 0, d.y, 0, 0, s.z, d.z, 0, 0, 0,
                     1);

            C = M * C;
        }

        Mat44f M = Rec709FullRangeRGBToYUV8<float>();
        const float rw709 = M.m00;
        const float gw709 = M.m01;
        const float bw709 = M.m02;

        if (FloatProperty* hue = m_colorHue)
        {
            float h = hue->front();

            if (h != 0.0f)
            {
                Mat44f R, Rz;

                const Vec3f d =
                    normalize(cross(Vec3f(1, 1, 1), Vec3f(0, 0, 1)));
                const float a = angleBetween(Vec3f(1, 1, 1), Vec3f(0, 0, 1));

                R.makeRotation(d, a);
                Mat44f iR = R;
                iR.invert();

                Vec3f l = R * Vec3f(rw709, gw709, bw709);
                const float xz = l.x / l.z;
                const float yz = l.y / l.z;

                Mat44f Zsh(1, 0, 0, 0, 0, 1, 0, 0, xz, yz, 1, 0, 0, 0, 0, 1);

                Rz.makeRotation(Vec3f(0, 0, 1), h);

                Mat44f iZsh(1, 0, 0, 0, 0, 1, 0, 0, -xz, -yz, 1, 0, 0, 0, 0, 1);

                C = iR * iZsh * Rz * Zsh * R * C;
                // C = iR * Rz * R * C;
            }
        }

        if (FloatProperty* saturation = m_colorSaturation)
        {
            float s = saturation->front();

            if (s != 1.0)
            {
                const float a = (1.0 - s) * rw709 + s;
                const float b = (1.0 - s) * rw709;
                const float c = (1.0 - s) * rw709;
                const float d = (1.0 - s) * gw709;
                const float e = (1.0 - s) * gw709 + s;
                const float f = (1.0 - s) * gw709;
                const float g = (1.0 - s) * bw709;
                const float h = (1.0 - s) * bw709;
                const float i = (1.0 - s) * bw709 + s;

                Mat44f S(a, d, g, 0, b, e, h, 0, c, f, i, 0, 0, 0, 0, 1);

                C = S * C;
            }
        }

        //
        //  CDL
        //
        //
        Vec3f CDL_slope(1.0f);
        Vec3f CDL_offset(0.0f);
        Vec3f CDL_power(1.0f);
        float CDL_saturation = 1.0f;

        if (CDLactive)
        {
            if (Vec3fProperty* slope = m_CDLslope)
            {
                CDL_slope = slope->front();
            }

            if (Vec3fProperty* offset = m_CDLoffset)
            {
                CDL_offset = offset->front();
            }

            if (Vec3fProperty* power = m_CDLpower)
            {
                CDL_power = power->front();
            }

            if (FloatProperty* saturation = m_CDLsaturation)
            {
                CDL_saturation = saturation->front();
            }
        }

        bool useCDL =
            CDLactive
            && (CDL_offset != Vec3f(0.0f) || CDL_slope != Vec3f(1.0f)
                || CDL_power != Vec3f(1.0f) || CDL_saturation != 1.0f);

        bool unpremult =
            unpremultProp
            && ((C != Mat44f() && active) || useCDL || (hasGamma && active));
        bool willPremult = unpremult;

        if (unpremult)
        {
            // add unpremult shader
            img->shaderExpr = Shader::newColorUnpremult(img->shaderExpr);
        }
        else if (willPremult)
        {
            // add stash alpha shader
        }

        // luminance LUT
        if (IntProperty* ip = m_lumlutActive)
        {
            if (ip->front() && m_lumLUTfb)
            {
                FrameBuffer* lumaLUT = m_lumLUTfb->referenceCopy();
                img->shaderExpr =
                    Shader::newColorLuminanceLUT(img->shaderExpr, lumaLUT);
            }
        }

        if (gammaValue != Vec3f(1.0f) && active)
        {
            img->shaderExpr =
                Shader::newColorGamma(img->shaderExpr, gammaValue);
        }

        if (C != Mat44f() && active)
        {
            img->shaderExpr = Shader::newColorMatrix(img->shaderExpr, C);
        }

        if (CDLactive
            && (CDL_offset != Vec3f(0.0f) || CDL_slope != Vec3f(1.0f)
                || CDL_power != Vec3f(1.0f) || CDL_saturation != 1.0f))
        {
            bool noClamp = false;
            if (IntProperty* ip = m_CDLnoclamp)
                noClamp = ip->front();

            const string CDL_colorspace =
                m_CDLcolorspace ? m_CDLcolorspace->front() : "rec709";

            const bool isACESLog = (CDL_colorspace == "aceslog");

            if (isACESLog || (CDL_colorspace == "aces"))
            {
                img->shaderExpr = Shader::newColorCDLForACES(
                    img->shaderExpr, CDL_slope, CDL_offset, CDL_power,
                    CDL_saturation, noClamp, isACESLog);
            }
            else
            {
                img->shaderExpr =
                    Shader::newColorCDL(img->shaderExpr, CDL_slope, CDL_offset,
                                        CDL_power, CDL_saturation, noClamp);
            }
        }

        if (willPremult)
        {
            // We use the ColorPremultLight shader which does not premultiply
            // the RGB channel values if the alpha value is zero.
            img->shaderExpr = Shader::newColorPremultLight(img->shaderExpr);
        }

        if (Mat44fProperty* Mo = m_matrixOutputRGBA)
        {
            Mo->resize(1);
            Mo->front() = C;
        }
    }

} // namespace IPCore
