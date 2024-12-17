//
//  Copyright (c) 2013 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCore/OutputGroupIPNode.h>

namespace IPCore
{
    using namespace std;

    OutputGroupIPNode::OutputGroupIPNode(const std::string& name,
                                         const NodeDefinition* def,
                                         IPGraph* graph, GroupIPNode* group)
        : DisplayGroupIPNode(name, def, graph, group)
    {
        setWritable(true);

        m_active = declareProperty<IntProperty>("output.active", 1);
        m_width = declareProperty<IntProperty>("output.width", 0);
        m_height = declareProperty<IntProperty>("output.height", 0);
        m_dataType =
            declareProperty<StringProperty>("output.dataType", "uint8");
        m_pixelAspect =
            declareProperty<FloatProperty>("output.pixelAspect", 1.0f);
    }

    OutputGroupIPNode::~OutputGroupIPNode() {}

    IPImage* OutputGroupIPNode::evaluate(const Context& context)
    {
        return DisplayGroupIPNode::evaluate(context);
    }

    bool OutputGroupIPNode::isActive() const { return m_active->front() != 0; }

} // namespace IPCore
