//
//  Copyright (c) 2015 Autodesk.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCoreCommands/SetRangeFromViewNode.h>

namespace IPCore
{
    namespace Commands
    {
        using namespace std;

        SetRangeFromViewNodeInfo::SetRangeFromViewNodeInfo(
            const string& n, TwkApp::CommandInfo::UndoType t)
            : CommandInfo(n, t)
        {
        }

        SetRangeFromViewNodeInfo::~SetRangeFromViewNodeInfo() {}

        TwkApp::Command* SetRangeFromViewNodeInfo::newCommand() const
        {
            return new SetRangeFromViewNode(this);
        }

        //----------------------------------------------------------------------

        SetRangeFromViewNode::SetRangeFromViewNode(
            const SetRangeFromViewNodeInfo* info)
            : Command(info)
            , m_session(0)
        {
        }

        SetRangeFromViewNode::~SetRangeFromViewNode() {}

        void SetRangeFromViewNode::setArgs(Session* session, int endOffset)
        {
            m_session = session;
            m_endOffset = endOffset;
        }

        void SetRangeFromViewNode::doit()
        {
            IPNode* viewNode = m_session->graph().viewNode();
            m_info = viewNode->imageRangeInfo();
            m_session->setFPS(m_info.fps);
            m_session->setRangeStart(m_info.start);
            m_session->setRangeEnd(m_info.end + m_endOffset);
            m_session->setInPoint(m_info.start);
            m_session->setOutPoint(m_info.end + m_endOffset);
        }

        void SetRangeFromViewNode::undo()
        {
            IPNode* viewNode = m_session->graph().viewNode();
            m_session->setFPS(m_info.fps);
            m_session->setRangeStart(m_info.start);
            m_session->setRangeEnd(m_info.end + m_endOffset);
            m_session->setInPoint(m_info.start);
            m_session->setOutPoint(m_info.end + m_endOffset);
        }

    } // namespace Commands
} // namespace IPCore
