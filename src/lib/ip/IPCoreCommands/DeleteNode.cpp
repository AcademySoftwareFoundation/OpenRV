//
//  Copyright (c) 2014 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCoreCommands/DeleteNode.h>
#include <IPCore/IPGraph.h>
#include <IPCore/NodeManager.h>
#include <IPCore/NodeDefinition.h>

namespace IPCore
{
    namespace Commands
    {
        using namespace std;

        DeleteNodeInfo::DeleteNodeInfo()
            : CommandInfo("deleteNode")
        {
        }

        DeleteNodeInfo::~DeleteNodeInfo() {}

        TwkApp::Command* DeleteNodeInfo::newCommand() const
        {
            return new DeleteNode(this);
        }

        //----------------------------------------------------------------------

        DeleteNode::DeleteNode(const DeleteNodeInfo* info)
            : Command(info)
            , m_graph(0)
            , m_node(0)
        {
        }

        DeleteNode::~DeleteNode()
        {
            //
            //  Because of the timing of undo/redo we can't really tell if
            //  we're in the undo or redo state when this destructor gets
            //  called (i.e. if the command is on the redo stack which is
            //  begin cleared because of another instance of a delete
            //  command).
            //
            //  So to prevent accidently deleting the node we never store the
            //  node pointer in the redo direction
            //

            if (m_node && !m_node->graph())
            {
                m_node->undoDeref();
            }
        }

        void DeleteNode::setArgs(IPGraph* graph, const string& name)
        {
            m_graph = graph;
            m_name = name;

            if (!(m_node = m_graph->findNode(m_name)))
            {
                TWK_THROW_EXC_STREAM("DeleteNode: unknown node " << m_name);
            }
            else if (m_node->group())
            {
                TWK_THROW_EXC_STREAM("DeleteNode: cannot delete "
                                     << m_name
                                     << " because its a group memeber");
            }

            if (m_node)
                m_node->undoRef();
        }

        void DeleteNode::doit()
        {
            if (!m_node)
                setArgs(m_graph, m_name);
            m_graph->isolateNode(m_node);
        }

        void DeleteNode::undo()
        {
            m_graph->restoreIsolatedNode(m_node);
            m_node = 0;
        }

    } // namespace Commands
} // namespace IPCore
