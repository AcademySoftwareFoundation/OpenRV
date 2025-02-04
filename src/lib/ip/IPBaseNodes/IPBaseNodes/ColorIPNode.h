//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPGraph__ColorIPNode__h__
#define __IPGraph__ColorIPNode__h__
#include <IPCore/IPNode.h>
#include <IPCore/LUTIPNode.h>

namespace IPCore
{

    //
    //  ColorIPNode
    //
    //  Does gamma + linear color transforms. Does not include
    //  nonlinear->linear transforms conceptually. See LinearizeIPNode for
    //  that.
    //
    //  NOTE: this class inherits from LUTIPNode in order to implement a
    //  luminance LUT. It does not allow loading LUTs from files, etc. and does
    //  not allow OCIO control. That is done by LinearizeIPNode
    //

    class ColorIPNode : public LUTIPNode
    {
    public:
        ColorIPNode(const std::string& name, const NodeDefinition* def,
                    IPGraph* graph, GroupIPNode* group = 0);

        virtual ~ColorIPNode();

        virtual IPImage* evaluate(const Context&);
        virtual void propertyChanged(const Property* property);
        virtual void readCompleted(const std::string&, unsigned int);

        void generateLUT();
        void setLogLin(int);
        void setSRGB(int);
        void setRec709(int);
        void setGamma(float);
        void setFileGamma(float);
        void setExposure(float);

    private:
        void evaluateOne(IPImage* img, const Context& context);

    private:
        FrameBuffer* m_lumLUTfb;
        IntProperty* m_colorInvert;
        Vec3fProperty* m_colorGamma;
        StringProperty* m_colorLUT;
        Vec3fProperty* m_colorOffset;
        Vec3fProperty* m_colorScale;
        Vec3fProperty* m_colorExposure;
        Vec3fProperty* m_colorContrast;
        FloatProperty* m_colorSaturation;
        IntProperty* m_colorNormalize;
        FloatProperty* m_colorHue;
        IntProperty* m_colorActive;
        IntProperty* m_colorUnpremult;
        IntProperty* m_CDLactive;
        StringProperty* m_CDLcolorspace;
        Vec3fProperty* m_CDLslope;
        Vec3fProperty* m_CDLoffset;
        Vec3fProperty* m_CDLpower;
        FloatProperty* m_CDLsaturation;
        IntProperty* m_CDLnoclamp;
        FloatProperty* m_lumlutLUT;
        FloatProperty* m_lumlutMax;
        IntProperty* m_lumlutSize;
        StringProperty* m_lumlutName;
        IntProperty* m_lumlutActive;
        IntProperty* m_lumlutOutputSize;
        StringProperty* m_lumlutOutputType;
        HalfProperty* m_lumlutOutputLUT;
        Mat44fProperty* m_matrixOutputRGBA;
    };

} // namespace IPCore

#endif // __IPGraph__ColorIPNode__h__
