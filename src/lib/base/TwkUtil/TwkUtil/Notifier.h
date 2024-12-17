//******************************************************************************
// Copyright (c) 2002 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkUtil__TwkUtilNotifier__h__
#define __TwkUtil__TwkUtilNotifier__h__
#include <sys/types.h>
#include <pthread.h>
#include <string>
#include <vector>
#include <TwkUtil/dll_defs.h>

namespace TwkUtil
{

    //
    //  class Notifier
    //
    //  The Notifier class implements a dynamic callback. Sometimes called
    //  "slots" or messages. In addition, messages propogate through
    //  networks of Notifiers.
    //

    class TWKUTIL_EXPORT Notifier
    {
    public:
        typedef size_t MessageId;

        //
        //	You can create one of these on the stack and send it along
        //	with a message. The easiest method is to use the
        //	TypedMessageData<> template class.
        //

        class TWKUTIL_EXPORT MessageData
        {
        public:
            MessageData() {}

            virtual ~MessageData();
        };

        template <class T> class TypedMessageData : public MessageData
        {
        public:
            TypedMessageData(T value)
                : data(value) {};
            T data;
        };

        typedef TypedMessageData<std::string> StringData;
        typedef bool (Notifier::*ReceiveFunction)(Notifier*, MessageId,
                                                  MessageData*);
        typedef std::vector<Notifier*> NotifierVector;

        //
        //	These two classes are used internally.
        //

        class TWKUTIL_EXPORT Message
        {
            Message() {}

            Message(const std::string& text, MessageId);
            ~Message();
            friend class Notifier;

            std::string m_text;
            MessageId m_id;
            bool m_debug;
            int m_lock;
        };

        class TWKUTIL_EXPORT Node
        {
            Node()
                : m_lock(false)
            {
            }

        public:
            ~Node() {}
            friend class Notifier;

            Message* m_message;
            NotifierVector m_recipients;
            bool m_lock;

            // void*			operator new(size_t);
            // void			operator delete(void*, size_t);
        };

        typedef std::vector<Message*> MessageVector;
        typedef std::vector<Node*> NodeVector;

    public:
        Notifier();
        virtual ~Notifier();

        //
        //	The notifier name is used primarily for debugging. You can
        //	override the function if there is a more fitting name for the
        //	object.
        //

        virtual std::string name() const;

        //
        //	Register a message of name "text". This is a global
        //	registration. If you call this function twice with the same
        //	text, it will return the same MessageId. This allows you do
        //	define the same message in many places and use them in
        //	conjunction with one another.
        //

        static MessageId registerMessage(const std::string&);

        //
        //	Mark a message as being in debug mode (or not)
        //

        static void debugMessage(MessageId, bool);

        static std::string stringForMessageId(MessageId);

        //
        //	If you wish to be notified when a message is sent from an
        //	object you need to add your self to its list of recipients.
        //

        virtual bool addNotification(Notifier*, MessageId);

        //
        //	Remove previously added notification
        //

        virtual void removeNotification(Notifier*, MessageId);

        //
        //	Remove all previously added notifications.
        //

        virtual void removeNotification(Notifier*);

        //
        //	Send a message with data
        //

        void sendWithData(MessageId, MessageData* data = 0);

        //
        //  Same as above, but transparently handles the data object. This is
        //  the usual send function for use with data.
        //

        template <typename T> void send(MessageId idf, T data)
        {
            TypedMessageData<T> m(data);
            sendWithData(idf, &m);
        }

        //
        //  Send without data
        //

        void send(MessageId idf) { sendWithData(idf, 0); }

        template <typename T> T messageData(MessageData* data)
        {
            TypedMessageData<T>* m = dynamic_cast<TypedMessageData<T>*>(data);
            assert(m);
            return m->data;
        }

        //
        //
        //

        void runGarbageCollector();

    protected:
        //
        //	These functions should be overriden; especially the receive
        //	function. This is the primary entry point for messages
        //

        virtual bool receive(Notifier*, Notifier*, MessageId, MessageData*);

        //
        //	May also be overriden, but unusual
        //

        virtual void receiveInternal(MessageId, MessageData*);
        virtual void destructReceive(Notifier*);

    private:
        static void initialize();

        //
        //	Retrieve a message by id or text
        //

        static Message* findMessage(MessageId);
        static Message* findMessage(const std::string&);

        //
        //	Internal addNotification function called by public function
        //

        bool addNotification(Notifier*, Message*, bool);

        //
        //	Called when a notifier that this has a pointer to is about to
        //	die. used for clean up.
        //

        void destructReceiveInternal(Notifier*);

        //
        //  Find a node by Message or Notifier
        //

        Node* findNode(Message*) const;
        Node* findNode(Notifier*) const;

        //
        //	Called when message debugging is on
        //

        bool debugReceive(Notifier*, Notifier*, MessageId, MessageData* d);

        //
        //	Internal send function
        //

        void send(Notifier*, Notifier::Message*, MessageData*);

    private:
        static void staticInit();
        static void lockMessages();
        static void unlockMessages();

    private:
        mutable NodeVector m_nodes;
        mutable NotifierVector m_destructRecipients;

        static pthread_mutex_t m_messageMutex;
        static bool m_init;

        static MessageVector m_messages;
        static Message* m_destructMessage;
        static int m_pathIndex;
        static NotifierVector m_needsGarbageCollection;
    };

#if 0
inline void*
Notifier::Node::operator new(size_t s)
{
    return stl_ext::block_alloc_arena::static_arena().allocate(s);
}

inline void
Notifier::Node::operator delete(void *p, size_t s)
{
    return stl_ext::block_alloc_arena::static_arena().deallocate(p,s);
}
#endif

#define NOTIFIER_MESSAGE(NAME)                  \
    static TwkUtil::Notifier::MessageId NAME(); \
    static TwkUtil::Notifier::MessageId m_##NAME;

#define NOTIFIER_MESSAGE_IMP(CLASS, NAME, TEXT)       \
    TwkUtil::Notifier::MessageId CLASS::m_##NAME = 0; \
    TwkUtil::Notifier::MessageId CLASS::NAME()        \
    {                                                 \
        if (!m_##NAME)                                \
        {                                             \
            m_##NAME = registerMessage(TEXT);         \
        }                                             \
                                                      \
        return m_##NAME;                              \
    }

#if defined(PLATFORM_WINDOWS) && !defined(TWKUTIL_BUILD)
    template class TWKUTIL_EXPORT Notifier::TypedMessageData<std::string>;
#endif

} // namespace TwkUtil

#endif // __TwkUtil__TwkUtilNotifier__h__
