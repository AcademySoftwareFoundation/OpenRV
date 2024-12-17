//
//  Copyright (c) 2014 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCoreCommands/WrapExistingNode.h>
#include <IPCore/IPGraph.h>
#include <IPCore/NodeManager.h>
#include <IPCore/NodeDefinition.h>

namespace IPCore
{
    namespace Commands
    {
        using namespace std;

        WrapExistingNodeInfo::WrapExistingNodeInfo()
            : CommandInfo("wrapExistingNode")
        {
        }

        WrapExistingNodeInfo::~WrapExistingNodeInfo() {}

        TwkApp::Command* WrapExistingNodeInfo::newCommand() const
        {
            return new WrapExistingNode(this);
        }

        //----------------------------------------------------------------------

        WrapExistingNode::WrapExistingNode(const WrapExistingNodeInfo* info)
            : Command(info)
            , m_graph(0)
            , m_node(0)
        {
        }

        WrapExistingNode::WrapExistingNode(const TwkApp::CommandInfo* info)
            : Command(info)
            , m_graph(0)
            , m_node(0)
        {
        }

        WrapExistingNode::~WrapExistingNode()
        {
            //
            // don't delete the cached pointers! this command does not own
            // them the graph does!
            //

            if (m_node && !m_node->graph())
            {
                m_node->undoDeref();
            }
        }

        void WrapExistingNode::setArgs(IPNode* node)
        {
            m_graph = node->graph();
            m_node = node;
            m_node->undoRef();
        }

        void WrapExistingNode::doit()
        {
            // nothing
        }

        void WrapExistingNode::undo() { m_graph->isolateNode(m_node); }

        void WrapExistingNode::redo() { m_graph->restoreIsolatedNode(m_node); }

    } // namespace Commands
} // namespace IPCore
