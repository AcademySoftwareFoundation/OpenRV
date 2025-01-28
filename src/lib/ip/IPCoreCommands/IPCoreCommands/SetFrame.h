//
//  Copyright (c) 2014 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __IPCoreCommands__SetFrame__h__
#define __IPCoreCommands__SetFrame__h__
#include <TwkApp/Command.h>
#include <IPCore/Session.h>
#include <iostream>

namespace IPCore
{
    namespace Commands
    {

        //
        //  SetFrame
        //
        //  Set the current frame.
        //

        class SetFrameInfo : public TwkApp::CommandInfo
        {
        public:
            typedef TwkApp::Command Command;

            SetFrameInfo(const std::string& name,
                         TwkApp::CommandInfo::UndoType type);
            virtual ~SetFrameInfo();
            virtual Command* newCommand() const;
        };

        class SetFrame : public TwkApp::Command
        {
        public:
            SetFrame(const SetFrameInfo*);
            virtual ~SetFrame();

            void setArgs(IPCore::Session* session, int frame);

            virtual void doit();

        private:
            Session* m_session;
            int m_frame;
        };

    } // namespace Commands
} // namespace IPCore

#endif // __IPCoreCommands__SetFrame__h__
