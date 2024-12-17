//
//  Copyright (c) 2014 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCoreCommands__WrapExistingNode__h__
#define __IPCoreCommands__WrapExistingNode__h__
#include <TwkApp/Command.h>
#include <IPCore/IPGraph.h>
#include <iostream>

namespace IPCore
{
    namespace Commands
    {

        //
        //  WrapExistingNode
        //
        //  Wraps undo/redo around an existing node. This is useful after
        //  pasting for example or a session file merge when you want the new
        //  nodes (created by the file) to be wrapped as undoable.
        //

        class WrapExistingNodeInfo : public TwkApp::CommandInfo
        {
        public:
            typedef TwkApp::Command Command;

            WrapExistingNodeInfo();
            virtual ~WrapExistingNodeInfo();
            virtual Command* newCommand() const;
        };

        class WrapExistingNode : public TwkApp::Command
        {
        public:
            WrapExistingNode(const WrapExistingNodeInfo*);
            virtual ~WrapExistingNode();

            void setArgs(IPNode* node);
            virtual void doit();
            virtual void undo();
            virtual void redo();

            template <typename T> T* node() const
            {
                return dynamic_cast<T*>(m_node);
            }

        protected:
            WrapExistingNode(const TwkApp::CommandInfo*);

        private:
            IPGraph* m_graph;
            IPNode* m_node;
        };

    } // namespace Commands
} // namespace IPCore

#endif // __IPCoreCommands__WrapExistingNode__h__
