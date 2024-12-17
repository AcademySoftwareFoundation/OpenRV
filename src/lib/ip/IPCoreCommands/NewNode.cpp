//
//  Copyright (c) 2014 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCoreCommands/NewNode.h>
#include <IPCore/IPGraph.h>
#include <IPCore/NodeManager.h>
#include <IPCore/NodeDefinition.h>

namespace IPCore
{
    namespace Commands
    {
        using namespace std;

        NewNodeInfo::NewNodeInfo()
            : CommandInfo("newNode")
        {
        }

        NewNodeInfo::~NewNodeInfo() {}

        TwkApp::Command* NewNodeInfo::newCommand() const
        {
            return new NewNode(this);
        }

        //----------------------------------------------------------------------

        NewNode::NewNode(const NewNodeInfo* info)
            : Command(info)
            , m_graph(0)
            , m_node(0)
        {
        }

        NewNode::NewNode(const TwkApp::CommandInfo* info)
            : Command(info)
            , m_graph(0)
            , m_node(0)
        {
        }

        NewNode::~NewNode()
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

        void NewNode::setArgs(IPGraph* graph, const string& type,
                              const string& name)
        {
            m_graph = graph;
            m_type = type;
            m_name = name;

            if (!m_graph->nodeManager()->definition(m_type))
            {
                TWK_THROW_EXC_STREAM("NewNode: unknown node type " << m_type);
            }
        }

        void NewNode::doit()
        {
            if (!m_node)
            {
                if (IPNode* node = m_graph->newNode(m_type, m_name))
                {
                    m_name = node->name();
                    m_node = node;
                    m_node->undoRef();
                }
                else
                {
                    TWK_THROW_EXC_STREAM(
                        "NewNode: failed to create node of type " << m_type);
                }
            }
            else
            {
                m_graph->restoreIsolatedNode(m_node);
            }
        }

        void NewNode::undo() { m_graph->isolateNode(m_node); }

    } // namespace Commands
} // namespace IPCore
