//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <TwkUtil/Notifier.h>
#include <stdio.h>
#include <algorithm>
#include <string>
#include <iostream>
#include <TwkUtil/Notifier.h>
#include <stl_ext/stl_ext_algo.h>

#define DEBUG_MESSAGE_LOCKING 0

namespace TwkUtil
{
    using namespace std;

#ifdef PLATFORM_WINDOWS
    //
    //  Explicitly instantiate
    //
    template class Notifier::TypedMessageData<std::string>;
#endif

    Notifier::Message::Message(const std::string& text, Notifier::MessageId id)
        : m_text(text)
        , m_id(id)
        , m_debug(false)
        , m_lock(0)
    {
    }

    Notifier::Message::~Message()
    {
        //  Messages are never destroyed.  We require this since multiple
        //  threads may access them through the m_messages list.

        cerr << "ERROR: Notifier::Message destroyed '" << m_text << "'" << endl;
    }

    //----------------------------------------------------------------------

    Notifier::MessageData::~MessageData() {}

    //----------------------------------------------------------------------

    Notifier::MessageVector Notifier::m_messages;
    Notifier::Message* Notifier::m_destructMessage = 0;
    Notifier::NotifierVector Notifier::m_needsGarbageCollection;
    bool Notifier::m_init = true;
    pthread_mutex_t Notifier::m_messageMutex;

    void Notifier::staticInit()
    {
        if (m_init)
        {
#if DEBUG_MESSAGE_LOCKING
            pthread_mutexattr_t attr;

            pthread_mutexattr_init(&attr);
            pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
            pthread_mutex_init(&m_messageMutex, &attr);
#else
            pthread_mutex_init(&m_messageMutex, 0);
#endif

            m_init = false;
        }
    }

    Notifier::Notifier() { staticInit(); }

    Notifier::~Notifier()
    {
        //
        //  Notifier recipients that you're about to die. They need to
        //  remove you from any notification lists you're in.
        //

        for (int i = 0; i < m_destructRecipients.size(); i++)
        {
            Notifier* N = m_destructRecipients[i];
            N->destructReceiveInternal(this);
            N->removeNotification(this);
        }

        stl_ext::delete_contents(m_nodes);
        m_nodes.clear();
        m_destructRecipients.clear();

        // cout << "deleted " << name() << endl;
    }

    void Notifier::lockMessages()
    {
        int err = pthread_mutex_lock(&m_messageMutex);

#if DEBUG_MESSAGE_LOCKING
        if (err != 0)
            cerr << "ERROR: Notifier::lockMessages() failed, err: "
                 << strerror(err) << endl;
#endif
    }

    void Notifier::unlockMessages()
    {
        int err = pthread_mutex_unlock(&m_messageMutex);

#if DEBUG_MESSAGE_LOCKING
        if (err != 0)
            cerr << "ERROR: Notifier::unlockMessages() failed, err: "
                 << strerror(err) << endl;
#endif
    }

    std::string Notifier::name() const
    {
        char temp[256];
        sprintf(temp, "Notifier@%p", this);
        return temp;
    }

    void Notifier::destructReceiveInternal(Notifier* N)
    {
        //
        // This is called when Notifier N is being destroyed.  we want to
        // remove any Nodes that have been created then call the virtual
        // destructReceive()
        //

        bool found = false;

        do
        {
            NotifierVector::iterator i = find(m_destructRecipients.begin(),
                                              m_destructRecipients.end(), N);

            if (i != m_destructRecipients.end())
            {
                m_destructRecipients.erase(i);
            }
        } while (found);

        removeNotification(N);
        destructReceive(N);
    }

    void Notifier::destructReceive(Notifier* N) {}

    void Notifier::initialize()
    {
        lockMessages();

        if (m_messages.size() == 0)
        {
            unlockMessages();
            registerMessage("Uninitialized");
            registerMessage("Destruct");
            lockMessages();
            m_destructMessage = m_messages[1];
            unlockMessages();
        }
        else
        {
            unlockMessages();
        }
    }

    Notifier::Message* Notifier::findMessage(MessageId id)
    {
        lockMessages();

        if (m_messages.size() > id)
        {
            Message* m = m_messages[id];
            unlockMessages();
            return m;
        }
        else
        {
            if (m_messages.size() == 0)
            {
                unlockMessages();
                initialize();
                return findMessage(id);
            }

            unlockMessages();
            return 0;
        }
    }

    Notifier::Message* Notifier::findMessage(const std::string& text)
    {
        lockMessages();

        for (int i = 0; i < m_messages.size(); i++)
        {
            if (m_messages[i]->m_text == text)
            {
                Message* m = m_messages[i];
                unlockMessages();
                return m;
            }
        }

        unlockMessages();
        return 0;
    }

    std::string Notifier::stringForMessageId(MessageId id)
    {
        if (Message* m = findMessage(id))
        {
            return m->m_text;
        }

        return "--unknown id--";
    }

    Notifier::MessageId Notifier::registerMessage(const std::string& text)
    {
        if (Message* m = findMessage(text))
        {
            MessageId id = m->m_id;
            return id;
        }
        else
        {
            lockMessages();
            Message* mm = new Message(text, m_messages.size());
            m_messages.push_back(mm);
            unlockMessages();
            return mm->m_id;
        }
    }

    void Notifier::debugMessage(MessageId id, bool state)
    {
        if (Message* message = findMessage(id))
        {
            message->m_debug = state;
        }
    }

    Notifier::Node* Notifier::findNode(Message* message) const
    {
        if (m_nodes.size() == 0)
            return 0;

        //
        //	Over time, this function bubble sorts the messages according
        //	to frequency of use. At any one time, they are not strictly in
        //	that order, only approximately. The last "most frequent"
        //	message will be on top.
        //

        if (m_nodes.front()->m_message == message)
        {
            Node* n = m_nodes.front();
            return n;
        }

        for (int i = 1; i < m_nodes.size(); i++)
        {
            if (m_nodes[i]->m_message == message)
            {
                std::swap(m_nodes[i], m_nodes[i - 1]);
                Node* n = m_nodes[i - 1];
                return n;
            }
        }

        return 0;
    }

    Notifier::Node* Notifier::findNode(Notifier* N) const
    {
        for (int i = 0; i < m_nodes.size(); i++)
        {
            if (stl_ext::exists(m_nodes[i]->m_recipients, N))
            {
                return m_nodes[i];
            }
        }

        return 0;
    }

    bool Notifier::addNotification(Notifier* N, MessageId id)
    {
        if (Message* message = findMessage(id))
        {
            return addNotification(N, message, true);
        }
        else
        {
            std::cerr << "Invalid Message in Notifier::addNotification "
                      << "[" << id << "]" << std::endl
                      << std::flush;

            return false;
        }
    }

    bool Notifier::addNotification(Notifier* N, Message* message,
                                   bool createIfNone)
    {
        Node* node = findNode(message);

        if (!node)
        {
            if (createIfNone)
            {
                node = new Node;
                node->m_message = message;
                m_nodes.push_back(node);
            }
            else
            {
                return false;
            }
        }

        node->m_recipients.push_back(N);

        // This is the way we fixed it on my machine
        N->m_destructRecipients.push_back(this);
        // AJG - JIM'S LOOKING INTO THIS - the easy fix
        m_destructRecipients.push_back(N);

        return true;
    }

    void Notifier::removeNotification(Notifier* N, MessageId id)
    {
        if (Message* message = findMessage(id))
        {
            if (Node* node = findNode(message))
            {
                stl_ext::remove(node->m_recipients, N);

                if (node->m_recipients.empty())
                {
                    m_needsGarbageCollection.push_back(this);
                }

                //
                //   Look to see if we have any other connections with
                //   this notifier if not, remove the destruct
                //   notification
                //

                if (!findNode(N))
                {
                    stl_ext::remove(N->m_destructRecipients, this);
                }
            }
        }
    }

    void Notifier::removeNotification(Notifier* N)
    {
        Node* node;

        while (node = findNode(N))
        {
            stl_ext::remove(node->m_recipients, N);

            if (node->m_recipients.empty())
            {
                m_needsGarbageCollection.push_back(this);
            }
        }
    }

    void Notifier::runGarbageCollector()
    {
        int i;

        for (i = 0; i < m_nodes.size(); i++)
        {
            Node* node = m_nodes[i];

            if (node->m_recipients.empty())
            {
                stl_ext::remove(m_nodes, node);
                delete node;
                i--;
            }
        }
    }

    bool Notifier::receive(Notifier*, Notifier*, MessageId, MessageData*)
    {
        return true;
    }

    void Notifier::receiveInternal(MessageId, MessageData*) {}

    void Notifier::sendWithData(MessageId id, MessageData* data)
    {
        //
        //  public send merely calls the internal send() function
        //  which traverses all Nodes.
        //

        if (Message* message = findMessage(id))
        {
            receiveInternal(id, data);
            send(this, message, data);
        }
        else
        {
            std::cerr << "Notifier \"" << name() << "\" can't send message id "
                      << id << " because it is invalid" << std::endl
                      << std::flush;
        }
    }

    void Notifier::send(Notifier* originator, Notifier::Message* message,
                        MessageData* data)
    {
        if (Node* node = findNode(message))
        {
            if (!node->m_lock)
            {
                node->m_lock = true;

                for (int i = 0, s = node->m_recipients.size(); i < s; i++)
                {
                    Notifier* receiver = node->m_recipients[i];

                    if (receiver != originator)
                    {
                        if (message->m_debug)
                        {
                            receiver->debugReceive(originator, this,
                                                   message->m_id, data);
                        }

                        if (receiver->receive(originator, this, message->m_id,
                                              data))
                        {
                            receiver->send(originator, message, data);
                        }
                    }
                }

                node->m_lock = false;
            }
        }
    }

    //----------------------------------------------------------------------

    bool Notifier::debugReceive(Notifier*, Notifier* N, MessageId id,
                                MessageData* data)
    {
        if (Message* message = findMessage(id))
        {
            std::cerr << N->name() << " sent \"" << message->m_text
                      << "\" message to " << name() << std::endl
                      << std::flush;

            return true;
        }
        else
        {
            std::cerr << "Notifier \"" << N->name()
                      << "\"  sent garbage message" << std::endl
                      << std::flush;
            return false;
        }
    }

} // namespace TwkUtil
