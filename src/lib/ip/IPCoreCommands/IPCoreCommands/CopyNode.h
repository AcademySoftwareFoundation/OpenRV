//
//  Copyright (c) 2014 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCoreCommands__CopyNode__h__
#define __IPCoreCommands__CopyNode__h__
#include <TwkApp/Command.h>
#include <IPCore/IPGraph.h>
#include <iostream>
#include <IPCoreCommands/NewNode.h>

namespace IPCore
{
    namespace Commands
    {

        //
        //  CopyNode
        //
        //  Calls NewNode command then copies the props
        //

        class CopyNodeInfo : public TwkApp::CommandInfo
        {
        public:
            typedef TwkApp::Command Command;

            CopyNodeInfo();
            virtual ~CopyNodeInfo();
            virtual Command* newCommand() const;
        };

        class CopyNode : public NewNode
        {
        public:
            CopyNode(const CopyNodeInfo*);
            virtual ~CopyNode();

            void setArgs(IPGraph* graph, IPNode* node, const std::string& name);

            virtual void doit();

        private:
            IPNode* m_copyNode;
        };

    } // namespace Commands
} // namespace IPCore

#endif // __IPCoreCommands__CopyNode__h__
