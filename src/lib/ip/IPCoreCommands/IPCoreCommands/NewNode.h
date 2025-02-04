//
//  Copyright (c) 2014 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCoreCommands__NewNode__h__
#define __IPCoreCommands__NewNode__h__
#include <TwkApp/Command.h>
#include <IPCore/IPGraph.h>
#include <iostream>

namespace IPCore
{
    namespace Commands
    {

        //
        //  NewNode
        //
        //  Create a new node from a type name using IPGraph::newNode(). The
        //  undo simply deletes the node using IPGraph::deleteNode() which
        //  will signal. redo will create a new node so the pointer value will
        //  change via undo/redo.
        //

        class NewNodeInfo : public TwkApp::CommandInfo
        {
        public:
            typedef TwkApp::Command Command;

            NewNodeInfo();
            virtual ~NewNodeInfo();
            virtual Command* newCommand() const;
        };

        class NewNode : public TwkApp::Command
        {
        public:
            NewNode(const NewNodeInfo*);
            virtual ~NewNode();

            void setArgs(IPGraph* graph, const std::string& type,
                         const std::string& name);

            virtual void doit();
            virtual void undo();

            template <typename T> T* node() const
            {
                return dynamic_cast<T*>(m_node);
            }

        protected:
            NewNode(const TwkApp::CommandInfo*);

        private:
            IPGraph* m_graph;
            std::string m_type;
            std::string m_name;
            IPNode* m_node;
        };

    } // namespace Commands
} // namespace IPCore

#endif // __IPCoreCommands__NewNode__h__
