//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkApp__EventNode__h__
#define __TwkApp__EventNode__h__
#include <TwkApp/Event.h>
#include <TwkUtil/Timer.h>
#include <vector>
#include <string>

namespace TwkApp
{

    ///
    /// class EventNode
    ///
    /// This class holds an EventTable object in which events can be bound.
    ///

    class EventNode
    {
    public:
        typedef std::vector<EventNode*> EventNodes;
        typedef TwkUtil::Timer Timer;

        enum Result
        {
            EventAccept,
            EventAcceptAndContinue,
            EventIgnored,
            Error
        };

        EventNode(const char* name);
        virtual ~EventNode();

        const std::string& name() const { return m_name; }

        void setEventNodeName(std::string n) { m_name = n; }

        //
        //  The event propagation tree is constructed by listening to
        //  events from another EventNode.
        //

        virtual void listenTo(EventNode*);

        //
        //  Events are propagated along the EventNode tree. The entry
        //  point is sendEvent(). Typically, the Event object will be
        //  created on the stack and passed to sendEvent(). You should
        //  call this function around a try{} block.
        //

        virtual Result sendEvent(const Event&);

        //
        //  If your node wishes to receive an events, override this
        //  function. This function should return true if the event
        //  propagation along the tree should be terminated (eaten) by
        //  receiver. Its ok to throw out of here.
        //

        virtual Result receiveEvent(const Event&);

        //
        //  Breaks all connections with the node
        //

        virtual void breakConnection(EventNode*);

        //
        //  Each EventNode has a timer. When events are sent from an
        //  EventNode, they receive a time stamp indicating when the
        //  event was sent relative to the creation of the EventNode
        //

        const Timer& timer() const { return m_timer; }

    protected:
        //
        //  Event propagation occurs via propogateEvent(). This function
        //  is called by sendEvent() at the event source.
        //

        virtual Result propagateEvent(const Event&);

    private:
        std::string m_name;
        EventNodes m_listeners;
        EventNodes m_senders;
        static Timer m_timer;
    };

} // namespace TwkApp

#endif // __TwkApp__EventNode__h__
