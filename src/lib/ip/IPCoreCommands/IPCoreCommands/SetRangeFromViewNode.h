//
//  Copyright (c) 2015 Autodesk.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCoreCommands__SetRangeFromViewNode__h__
#define __IPCoreCommands__SetRangeFromViewNode__h__
#include <iostream>
#include <TwkApp/Command.h>
#include <IPCore/Session.h>
#include <IPCore/IPNode.h>

namespace IPCore
{
    namespace Commands
    {

        //
        //  SetRangeFromViewNode
        //
        //  sets the range and fps on the session from the view node timing
        //

        class SetRangeFromViewNodeInfo : public TwkApp::CommandInfo
        {
        public:
            typedef TwkApp::Command Command;

            SetRangeFromViewNodeInfo(const std::string& name,
                                     TwkApp::CommandInfo::UndoType type);
            virtual ~SetRangeFromViewNodeInfo();
            virtual Command* newCommand() const;
        };

        class SetRangeFromViewNode : public TwkApp::Command
        {
        public:
            typedef IPCore::IPNode::ImageRangeInfo ImageRangeInfo;

            SetRangeFromViewNode(const SetRangeFromViewNodeInfo*);
            virtual ~SetRangeFromViewNode();

            void setArgs(IPCore::Session* session, int endOffset = 0);

            virtual void doit();
            virtual void undo();

        private:
            Session* m_session;
            int m_endOffset;
            ImageRangeInfo m_info;
        };

    } // namespace Commands
} // namespace IPCore

#endif // __IPCoreCommands__SetRangeFromViewNode__h__
