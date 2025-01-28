//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <IPBaseNodes/YCToRGBIPNode.h>
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

    YCToRGBIPNode::YCToRGBIPNode(const std::string& name,
                                 const NodeDefinition* def, IPGraph* graph,
                                 GroupIPNode* group)
        : IPNode(name, def, graph, group)
        , m_conversionName(0)
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
        m_conversionName = declareProperty<StringProperty>(
            "color.YUVconversion", "File", info);
    }

    YCToRGBIPNode::~YCToRGBIPNode() {}

    void YCToRGBIPNode::setConversion(const std::string& s)
    {
        setProperty(m_conversionName, s);
    }

    IPImage* YCToRGBIPNode::evaluate(const Context& context)
    {
        const bool active = propertyValue<IntProperty>(m_active, 1) != 0;

        int frame = context.frame;
        IPImage* img = IPNode::evaluate(context);
        if (!img)
            return IPImage::newNoImage(this, "No Input");

        TwkFB::FrameBuffer* fb = img->fb;
        string conversion = propertyValue(m_conversionName, "File");

        if (fb
            && (fb->isYUV() || fb->isYUVPlanar() || fb->isYUVBiPlanar()
                || fb->dataType() == TwkFB::FrameBuffer::PACKED_Cb8_Y8_Cr8_Y8
                || fb->dataType() == TwkFB::FrameBuffer::PACKED_Y8_Cb8_Y8_Cr8)
            && conversion != "File")
        {
            //
            //  If necessary, modify the incoming YUV->RGB conversion
            //
            //  NOTE: this is assuming that no other shader functions have
            //  been applied ontop of the source shader. if there have
            //  been this will clobber them.
            //

            string colorName;
            string rangeName;

            if (conversion == "ITU Rec.601")
            {
                colorName = TwkFB::ColorSpace::Rec601();
                rangeName = TwkFB::ColorSpace::VideoRange();
            }
            else if (conversion == "ITU Rec.601 Full Range")
            {
                colorName = TwkFB::ColorSpace::Rec601();
                rangeName = TwkFB::ColorSpace::FullRange();
            }
            else if (conversion == "ITU Rec.709")
            {
                colorName = TwkFB::ColorSpace::Rec709();
                rangeName = TwkFB::ColorSpace::VideoRange();
            }
            else if (conversion == "ITU Rec.709 Full Range")
            {
                colorName = TwkFB::ColorSpace::Rec709();
                rangeName = TwkFB::ColorSpace::FullRange();
            }
            else if (conversion == "ITU Rec.2020")
            {
                colorName = TwkFB::ColorSpace::Rec2020();
                rangeName = TwkFB::ColorSpace::VideoRange();
            }
            else if (conversion == "ITU Rec.2020 Full Range")
            {
                colorName = TwkFB::ColorSpace::Rec2020();
                rangeName = TwkFB::ColorSpace::FullRange();
            }
            else
            {
                // leave it
            }

            if (colorName != ""
                && (colorName != fb->conversion() || rangeName != fb->range()))
            {
                Mat44f M;
                TwkFB::getYUVtoRGBMatrix(M, colorName, rangeName);

                switch (fb->numPlanes())
                {
                case 1:
                    delete img->shaderExpr;
                    img->shaderExpr = Shader::newSourceYVYU(img, M);
                    break;
                case 2:
                    delete img->shaderExpr;
                    img->shaderExpr = Shader::newSourcePlanar2YUV(img, M);
                    break;
                case 3:
                    delete img->shaderExpr;
                    img->shaderExpr = Shader::newSourcePlanarYUV(img, M);
                    break;
                case 4:
                    delete img->shaderExpr;
                    img->shaderExpr = Shader::newSourcePlanarYUVA(img, M);
                    break;
                default:
                    // nothing
                    break;
                }
            }
        }

        return img;
    }

} // namespace IPCore
