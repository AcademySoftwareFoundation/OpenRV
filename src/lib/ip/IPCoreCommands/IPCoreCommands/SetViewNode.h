//
//  Copyright (c) 2014 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCoreCommands__SetViewNode__h__
#define __IPCoreCommands__SetViewNode__h__
#include <TwkApp/Command.h>
#include <IPCore/IPGraph.h>
#include <iostream>

namespace IPCore
{
    namespace Commands
    {

        //
        //  SetViewNode
        //
        //  Change the view node state on IPGraph. doit() toggles the viewNode
        //  state so undo/redo both call it.
        //

        class SetViewNodeInfo : public TwkApp::CommandInfo
        {
        public:
            typedef TwkApp::Command Command;

            SetViewNodeInfo();
            virtual ~SetViewNodeInfo();
            virtual Command* newCommand() const;
        };

        class SetViewNode : public TwkApp::Command
        {
        public:
            SetViewNode(const SetViewNodeInfo*);
            virtual ~SetViewNode();

            void setArgs(IPGraph* graph, const std::string& name);

            virtual void doit();
            virtual void undo();

        private:
            IPGraph* m_graph;
            std::string m_name;
        };

    } // namespace Commands
} // namespace IPCore

#endif // __IPCoreCommands__SetViewNode__h__
