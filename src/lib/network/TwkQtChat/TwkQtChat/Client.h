//
//  Copyright (c) 2008 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __TwkQtChat__Client__h__
#define __TwkQtChat__Client__h__
#include <iostream>
#include <QtNetwork/QAbstractSocket>
#include <QtCore/QHash>
#include <QtNetwork/QHostAddress>
#include <QtCore/QSettings>
#include <TwkQtChat/Server.h>

namespace TwkQtChat
{
    class Connection;
    class DataServer;
    class DataConnection;

    class Client : public QObject
    {
        Q_OBJECT

    public:
        typedef QMultiHash<QHostAddress, Connection*> ConnectionMap;

        Client(const QString& contactName, const QString& contactApp = "rv",
               int port = 45124, bool pingpong = true, ConnectionFactory f = 0);
        ~Client();

        ///
        /// Reject a connection (usually called by newConnection() signal
        /// receiver()).
        ///

        void rejectConnection(Connection*);

        ///
        ///  Send message to a particular connected contact. The contact
        ///  name should match the actual contact name (not the expected
        ///  one).

        void sendMessage(const QString& contact, const QString& message);

        ///
        /// Send binary data to connected contact. The interp string
        /// should tell the other end how to deal with it.
        ///

        void sendData(const QString& contact, const QString& interp,
                      const QByteArray& data);

        ///
        /// Send message to all connected contacts
        ///

        void broadcastMessage(const QString& message);

        ///
        /// Formats a message as an event message and sends to particular
        /// connected contacts.
        ///

        void sendEvent(const QString& contact, const QString& event,
                       const QString& target, const QString& message,
                       bool rsvp = false);

        ///
        /// Formats a message as an event message and sends to all
        /// connected contacts.
        ///

        void broadcastEvent(const QString& event, const QString& target,
                            const QString& message, bool rsvp = false);

        ///
        /// The local contact name (the name of this client)
        ///

        QString& contactName() { return m_contactName; }

        ///
        /// identifierName() is contactName() + "@" + machine name
        ///

        QString identifierName() const;

        ///
        /// Create specific connection to a contact at a machine. If the
        /// actual contact on the machine differs from the expected passed
        /// in name a new contact will be formed.
        ///

        void connectTo(const QString& name, const QString& host,
                       int port = 45124);
        void connectTo(const QString& name, const QHostAddress& ip,
                       int port = 45124);

        ///
        /// Test for existing connection to machine
        ///

        bool hasConnection(const QHostAddress& senderIp,
                           int senderPort = -1) const;

        ///
        /// Disconnect from existing connection
        ///

        void disconnectFrom(const QString& contact);

        ///
        /// Return if a connection of a contact is incoming
        ///
        bool isIncoming(const QString& contact);

        ///
        /// Wait for a message from contact. Returns true if it received
        /// one and false if it timed out.
        ///

        bool waitForMessage(const QString& contact);

        ///
        /// Wait for send to occur or timeout. The function returns true
        /// if a send occured. Use this if you't return to an event loop
        /// and you want to make sure a previously sent data/message goes
        /// out immediately.
        ///

        bool waitForSend(const QString& contact);

        ///
        /// Send a polite disconnect request message (so that the remote
        /// contact knows you didn't crash).
        ///

        void signOff(const QString& contact);

        bool online();

        int serverPort() { return (m_server) ? m_server->serverPort() : 0; }

    signals:
        void newMessage(const QString& from, const QString& message);
        void newData(const QString& from, const QString& interp,
                     const QByteArray& data);
        void newContact(const QString& contact);
        void contactLeft(const QString& contact);
        void requestConnection(TwkQtChat::Connection*);
        void connectionFailed(QAbstractSocket::SocketError);
        void contactError(const QString& contact, const QString& host,
                          const QString& error);

    private slots:
        void newConnection(Connection* connection);
        void connectionError(QAbstractSocket::SocketError socketError);
        void disconnected();
        void readyForUse();
        void requestGreeting();

    private:
        void removeConnection(Connection* connection);
        Connection* connectionByName(const QString&) const;

    private:
        QString m_contactName;
        QString m_contactApp;
        Server* m_server;
        ConnectionMap m_connectionMap;
        DataServer* m_dataServer;
        bool m_pingpong;

        ConnectionFactory m_connectionFactory;
    };

} // namespace TwkQtChat

#endif // __TwkQtChat__Client__h__
