//
//  Copyright (c) 2014 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCoreCommands/SetViewNode.h>
#include <IPCore/IPGraph.h>

namespace IPCore
{
    namespace Commands
    {
        using namespace std;

        SetViewNodeInfo::SetViewNodeInfo()
            : CommandInfo("setViewNode")
        {
        }

        SetViewNodeInfo::~SetViewNodeInfo() {}

        TwkApp::Command* SetViewNodeInfo::newCommand() const
        {
            return new SetViewNode(this);
        }

        //----------------------------------------------------------------------

        SetViewNode::SetViewNode(const SetViewNodeInfo* info)
            : Command(info)
            , m_graph(0)
        {
        }

        SetViewNode::~SetViewNode()
        {
            //
            // don't delete the cached pointers! this command does not own
            // them the graph does!
            //
        }

        void SetViewNode::setArgs(IPGraph* graph, const string& name)
        {
            m_graph = graph;
            m_name = name;

            if (!m_graph->findNode(m_name))
            {
                TWK_THROW_EXC_STREAM("SetViewNode: unknown node " << m_name);
            }
        }

        void SetViewNode::doit()
        {
            IPNode* current = m_graph->viewNode();
            IPNode* viewNode = m_graph->findNode(m_name);
            m_name = current->name();
            m_graph->setViewNode(viewNode);
        }

        void SetViewNode::undo() { doit(); }

    } // namespace Commands
} // namespace IPCore
