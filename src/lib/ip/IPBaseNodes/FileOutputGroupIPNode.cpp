//
//  Copyright (c) 2014 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPBaseNodes/FileOutputGroupIPNode.h>
#include <IPBaseNodes/RetimeIPNode.h>
#include <IPCore/NodeDefinition.h>
#include <IPCore/PipelineGroupIPNode.h>

namespace IPCore
{
    using namespace std;

    FileOutputGroupIPNode::FileOutputGroupIPNode(const std::string& name,
                                                 const NodeDefinition* def,
                                                 IPGraph* graph,
                                                 GroupIPNode* group)
        : OutputGroupIPNode(name, def, graph, group)
    {
        m_filename =
            declareProperty<StringProperty>("output.filename", "out.mov");
        m_fps = declareProperty<FloatProperty>("output.fps", 24.0f);
        m_channels = declareProperty<StringProperty>("output.channels", "RGBA");
        m_timeRange = declareProperty<StringProperty>("output.timeRange", "");

        //
        //  Add a Retime node as the root so that the FPS of the output
        //  node is whatever the user specified in m_fps
        //

        string retimeType = def->stringValue("retimeType", "Retime");

        m_retimeNode = newMemberNodeOfType<RetimeIPNode>(retimeType, "retime");
        m_retimeNode->setInputs1(displayPipelineNode());
        m_retimeNode->setProperty<FloatProperty>("output.fps",
                                                 propertyValue(m_fps, 24.0f));
        setRoot(m_retimeNode);
    }

    FileOutputGroupIPNode::~FileOutputGroupIPNode() {}

    IPImage* FileOutputGroupIPNode::evaluate(const Context& context)
    {
        return OutputGroupIPNode::evaluate(context);
    }

    void FileOutputGroupIPNode::propertyChanged(const Property* p)
    {
        if (p == m_fps)
        {
            m_retimeNode->setProperty(m_retimeNode->outputFPSProperty(),
                                      propertyValue(m_fps, 24.0f));
        }

        IPNode::propertyChanged(p);
    }

} // namespace IPCore
