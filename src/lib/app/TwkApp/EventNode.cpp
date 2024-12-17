//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkApp/EventNode.h>
#include <stl_ext/stl_ext_algo.h>

namespace TwkApp
{
    using namespace stl_ext;

    EventNode::Timer EventNode::m_timer;

    EventNode::EventNode(const char* name)
        : m_name(name)
    {
        //  std::cerr << "EventNode '" << name << "'" << std::endl;
    }

    EventNode::~EventNode()
    {
        while (m_listeners.size())
            breakConnection(m_listeners.front());
        while (m_senders.size())
            breakConnection(m_senders.front());
    }

    void EventNode::listenTo(EventNode* node)
    {
        node->m_listeners.push_back(this);
        m_senders.push_back(node);
    }

    void EventNode::breakConnection(EventNode* node)
    {
        remove_unsorted(node->m_senders, this);
        remove_unsorted(node->m_listeners, this);
        remove_unsorted(m_listeners, node);
        remove_unsorted(m_senders, node);
    }

    EventNode::Result EventNode::receiveEvent(const Event&)
    {
        //
        //  Default is to simply let the event continue
        //

        return EventIgnored;
    }

    EventNode::Result EventNode::sendEvent(const Event& event)
    {
        if (!m_timer.isRunning())
            m_timer.start();
        event.m_timeStamp = m_timer.elapsed();
        return propagateEvent(event);
    }

    EventNode::Result EventNode::propagateEvent(const Event& event)
    {
        //
        //  If the receiver accepts the event, then stop
        //
        // std::cerr << "propagateEvent '" << event.name() << "' from '" <<
        // m_name << "'" << std::endl;

        if (receiveEvent(event) == EventAccept)
        {
            return EventAccept;
        }

        Result rval = EventIgnored;

        for (int i = 0; i < m_listeners.size(); i++)
        {
            EventNode* n = m_listeners[i];

            switch (n->propagateEvent(event))
            {
            case EventAccept:
                return EventAccept;
            case Error:
                return Error;
            case EventAcceptAndContinue:
                rval = EventAcceptAndContinue;
            case EventIgnored:
                break;
            }
        }

        return rval;
    }

} // namespace TwkApp
