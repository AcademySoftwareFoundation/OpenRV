//
//  Copyright (c) 2014 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCoreCommands/CopyNode.h>
#include <IPCore/IPGraph.h>
#include <IPCore/NodeManager.h>
#include <IPCore/NodeDefinition.h>

namespace IPCore
{
    namespace Commands
    {
        using namespace std;

        CopyNodeInfo::CopyNodeInfo()
            : CommandInfo("copyNode")
        {
        }

        CopyNodeInfo::~CopyNodeInfo() {}

        TwkApp::Command* CopyNodeInfo::newCommand() const
        {
            return new CopyNode(this);
        }

        //----------------------------------------------------------------------

        CopyNode::CopyNode(const CopyNodeInfo* info)
            : NewNode(info)
        {
        }

        CopyNode::~CopyNode() {}

        void CopyNode::setArgs(IPGraph* graph, IPNode* node, const string& name)
        {
            m_copyNode = node;

            try
            {
                NewNode::setArgs(graph, node->protocol(), name);
            }
            catch (std::exception& exc)
            {
                TWK_THROW_EXC_STREAM("CopyNode: failed to create copy of node "
                                     << node->name());
            }
        }

        void CopyNode::doit()
        {
            const bool firstTime = !node<IPNode>();

            try
            {
                NewNode::doit();
            }
            catch (std::exception& exc)
            {
                TWK_THROW_EXC_STREAM("CopyNode: failed to create copy of node "
                                     << m_copyNode->name());
            }

            if (firstTime)
            {
                if (IPNode* n = node<IPNode>())
                    n->copyNode(m_copyNode);
            }
        }

    } // namespace Commands
} // namespace IPCore
