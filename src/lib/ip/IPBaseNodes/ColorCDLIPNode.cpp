//******************************************************************************
// Copyright (c) 2014 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <IPBaseNodes/ColorCDLIPNode.h>
#include <IPCore/Exception.h>
#include <IPCore/GroupIPNode.h>
#include <IPCore/ShaderCommon.h>
#include <IPCore/NodeDefinition.h>
#include <TwkMath/Function.h>
#include <TwkMath/Vec3.h>
#include <TwkMath/Iostream.h>
#include <TwkMath/MatrixColor.h>
#include <CDL/cdl_utils.h>

namespace IPCore
{
    using namespace std;
    using namespace TwkContainer;
    using namespace TwkMath;

    ColorCDLIPNode::ColorCDLIPNode(const std::string& name,
                                   const NodeDefinition* def, IPGraph* graph,
                                   GroupIPNode* group)
        : IPNode(name, def, graph, group)
    {
        setMaxInputs(1);

        Property::Info* info = new Property::Info();
        info->setPersistent(false);

        m_active = declareProperty<IntProperty>("node.active", 1);
        m_file = declareProperty<StringProperty>("node.file", "");
        m_colorspace = declareProperty<StringProperty>(
            "node.colorspace",
            definition()->stringValue("defaults.colorspace", "rec709"));
        m_slope = declareProperty<Vec3fProperty>("node.slope",
                                                 Vec3f(1.0f, 1.0f, 1.0f));
        m_offset = declareProperty<Vec3fProperty>("node.offset", Vec3f(0.0f));
        m_power = declareProperty<Vec3fProperty>("node.power",
                                                 Vec3f(1.0f, 1.0f, 1.0f));
        m_saturation = declareProperty<FloatProperty>("node.saturation", 1.0f);
        m_noclamp = declareProperty<IntProperty>("node.noClamp", 1);
    }

    ColorCDLIPNode::~ColorCDLIPNode() {}

    void ColorCDLIPNode::copyNode(const IPNode* node)
    {
        IPNode::copy(node);
        if (node->definition() == definition())
            updateProperties();
    }

    void ColorCDLIPNode::propertyChanged(const Property* property)
    {
        IPNode::propertyChanged(property);
        if (property == m_file)
            updateProperties();
    }

    void ColorCDLIPNode::readCompleted(const std::string& type,
                                       unsigned int version)
    {
        IPNode::readCompleted(type, version);
        updateProperties();
    }

    void ColorCDLIPNode::updateProperties()
    {
        LockGuard lock(m_updateMutex);

        if (m_file->size() <= 0 || m_file->front() == "")
            return;

        CDL::ColorCorrectionCollection ccc = CDL::readCDL(m_file->front());
        if (ccc.size() > 1)
        {
            cout << "WARNING: Found more than one cdl in " << m_file->front()
                 << ". Using first one found." << endl;
        }
        if (ccc.size() > 0)
        {
            CDL::ColorCorrection cc = ccc.front();

            m_slope->front() = cc.slope;
            m_offset->front() = cc.offset;
            m_power->front() = cc.power;
            m_saturation->front() = cc.saturation;
        }
    }

    IPImage* ColorCDLIPNode::evaluate(const Context& context)
    {
        int frame = context.frame;
        IPImage* head = IPNode::evaluate(context);
        if (!head)
            return IPImage::newNoImage(this, "No Input");
        IPImage* img = head;

        bool active = m_active ? m_active->front() : true;
        if (!active)
            return img;

        Vec3f slope(1.0f);
        Vec3f offset(0.0f);
        Vec3f power(1.0f);
        float saturation = 1.0f;

        if (Vec3fProperty* ip = m_slope)
        {
            slope = ip->front();
        }

        if (Vec3fProperty* ip = m_offset)
        {
            offset = ip->front();
        }

        if (Vec3fProperty* ip = m_power)
        {
            power = ip->front();
        }

        if (FloatProperty* ip = m_saturation)
        {
            saturation = ip->front();
        }

        if (offset != Vec3f(0.0f) || slope != Vec3f(1.0f)
            || power != Vec3f(1.0f) || saturation != 1.0f)
        {
            bool noClamp = false;
            if (IntProperty* ip = m_noclamp)
                noClamp = ip->front();

            const string colorspace =
                m_colorspace ? m_colorspace->front() : "rec709";

            const bool isACESLog = (colorspace == "aceslog");

            if (isACESLog || (colorspace == "aces"))
            {
                img->shaderExpr = Shader::newColorCDLForACES(
                    img->shaderExpr, slope, offset, power, saturation, noClamp,
                    isACESLog);
            }
            else
            {
                img->shaderExpr = Shader::newColorCDL(
                    img->shaderExpr, slope, offset, power, saturation, noClamp);
            }
        }

        return img;
    }

} // namespace IPCore
