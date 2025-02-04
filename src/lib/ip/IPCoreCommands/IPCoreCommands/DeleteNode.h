//
//  Copyright (c) 2014 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCoreCommands__DeleteNode__h__
#define __IPCoreCommands__DeleteNode__h__
#include <TwkApp/Command.h>
#include <IPCore/IPGraph.h>
#include <iostream>

namespace IPCore
{
    namespace Commands
    {

        //
        //  DeleteNode
        //
        //  Calls IPGraph::isolateNode() and IPGraph::restoreIsolatedNode() to
        //  "freeze dry" a node and take it out of the graph and make it easy
        //  to restore later. Unlike NewProperty deletion maintains the node
        //  pointer (but don't assume that).
        //

        class DeleteNodeInfo : public TwkApp::CommandInfo
        {
        public:
            typedef TwkApp::Command Command;

            DeleteNodeInfo();
            virtual ~DeleteNodeInfo();
            virtual Command* newCommand() const;
        };

        class DeleteNode : public TwkApp::Command
        {
        public:
            DeleteNode(const DeleteNodeInfo*);
            virtual ~DeleteNode();

            void setArgs(IPGraph* graph, const std::string& name);

            virtual void doit();
            virtual void undo();

        private:
            IPGraph* m_graph;
            IPNode* m_node;
            std::string m_name;
        };

    } // namespace Commands
} // namespace IPCore

#endif // __IPCoreCommands__DeleteNode__h__
