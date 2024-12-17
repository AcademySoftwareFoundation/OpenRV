//
//  Copyright (c) 2014 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCoreCommands/SetFrame.h>
#include <IPCore/IPGraph.h>

namespace IPCore
{
    namespace Commands
    {
        using namespace std;

        SetFrameInfo::SetFrameInfo(const string& n,
                                   TwkApp::CommandInfo::UndoType t)
            : CommandInfo(n, t)
        {
        }

        SetFrameInfo::~SetFrameInfo() {}

        TwkApp::Command* SetFrameInfo::newCommand() const
        {
            return new SetFrame(this);
        }

        //----------------------------------------------------------------------

        SetFrame::SetFrame(const SetFrameInfo* info)
            : Command(info)
            , m_session(0)
        {
        }

        SetFrame::~SetFrame() {}

        void SetFrame::setArgs(Session* session, int frame)
        {
            m_session = session;
            m_frame = frame;
        }

        void SetFrame::doit()
        {
            int f = m_frame;
            m_frame = m_session->currentFrame();
            m_session->setFrame(f);
        }

    } // namespace Commands
} // namespace IPCore
