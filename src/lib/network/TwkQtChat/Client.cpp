//
//  Copyright (c) 2008 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <TwkQtChat/Client.h>
#include <TwkQtChat/Connection.h>
#include <QtNetwork/QtNetwork>
#include <QtCore/QtCore>

#if 0
#define DB_ICON 0x01
#define DB_ALL 0xff

//  #define DB_LEVEL        (DB_ALL & (~ DB_ICON))
#define DB_LEVEL DB_ALL

#define DB(x) cerr << "Client: " << x << endl
#define DBL(level, x)     \
    if (level & DB_LEVEL) \
    cerr << "Client: " << x << endl
#else
#define DB(x)
#define DBL(level, x)
#endif

namespace TwkQtChat
{
    using namespace std;

    Client::Client(const QString& name, const QString& appName, int port,
                   bool pingpong, ConnectionFactory fact)
        : m_pingpong(pingpong)
        , m_contactName(name)
        , m_contactApp(appName)
        , m_connectionFactory(fact)
    {
        //
        //  Most clients (who only want to communicate with RV) have no need of
        //  a Server, since they will not be listening for connections.  They
        //  can use a port arg of 0.
        //
        if (port)
        {
            m_server = new Server(0, port, fact);

            if (!m_server->isListening())
                return;

            QObject::connect(m_server, SIGNAL(newConnection(Connection*)), this,
                             SLOT(newConnection(Connection*)));
        }
        else
            m_server = 0;
    }

    Client::~Client()
    {
        QList<Connection*> values = m_connectionMap.values();
        for (int i = 0; i < values.size(); i++)
            removeConnection(values[i]);
        delete m_server;
    }

    void Client::broadcastMessage(const QString& message)
    {
        if (message.isEmpty())
            return;

        QList<Connection*> connections = m_connectionMap.values();
        foreach (Connection* connection, connections)
            connection->sendMessage(message);
    }

    void Client::sendMessage(const QString& who, const QString& message)
    {
        if (message.isEmpty())
            return;

        QList<Connection*> connections = m_connectionMap.values();

        foreach (Connection* c, connections)
        {
            if (c->remoteContactName() == who && c->isValid())
                c->sendMessage(message);
        }
    }

    void Client::sendData(const QString& who, const QString& interp,
                          const QByteArray& data)
    {
        if (interp.isEmpty() || data.isEmpty())
            return;

        QList<Connection*> connections = m_connectionMap.values();

        foreach (Connection* c, connections)
        {
            if (c->remoteContactName() == who && c->isValid())
            {
                c->sendData(interp, data);
            }
        }
    }

    void Client::broadcastEvent(const QString& eventName, const QString& target,
                                const QString& text, bool rsvp)
    {
        QString type = (rsvp) ? "RETURNEVENT " : "EVENT ";
        QString message = type + eventName + " " + target + " " + text;
        broadcastMessage(message);
    }

    void Client::sendEvent(const QString& who, const QString& eventName,
                           const QString& target, const QString& text,
                           bool rsvp)
    {
        QString type = (rsvp) ? "RETURNEVENT " : "EVENT ";
        QString message = type + eventName + " " + target + " " + text;
        sendMessage(who, message);
    }

    bool Client::waitForMessage(const QString& who)
    {
        QList<Connection*> connections = m_connectionMap.values();

        foreach (Connection* c, connections)
        {
            if (c->remoteContactName() == who && c->isValid())
            {
                return c->waitForReadyRead();
            }
        }

        return false;
    }

    bool Client::waitForSend(const QString& who)
    {
        QList<Connection*> connections = m_connectionMap.values();

        foreach (Connection* c, connections)
        {
            if (c->remoteContactName() == who && c->isValid())
            {
                return c->waitForBytesWritten();
            }
        }

        return false;
    }

    QString Client::identifierName() const
    {
        QString name = QHostInfo::localHostName();
        if (name.endsWith(".local"))
            name.chop(6);
        return m_contactName + "@" + name;
    }

    bool Client::hasConnection(const QHostAddress& senderIp,
                               int senderPort) const
    {
        if (senderPort == -1)
            return m_connectionMap.contains(senderIp);

        if (!m_connectionMap.contains(senderIp))
            return false;

        QList<Connection*> connections = m_connectionMap.values(senderIp);
        foreach (Connection* connection, connections)
        {
            if (connection->peerPort() == senderPort)
                return true;
        }

        return false;
    }

    void Client::connectTo(const QString& name, const QHostAddress& ip,
                           int port)
    {
        if (!hasConnection(ip, port))
        {
            Connection* c = 0;
            if (m_connectionFactory)
                c = (*m_connectionFactory)(this, true);
            else
                c = new Connection(this, true);

            newConnection(c);
            c->connectToContact(name, ip, port);
        }
        else
        {
            DB("Client::connectTo connection with same IP and port exists");
        }
    }

    void Client::connectTo(const QString& name, const QString& host, int port)
    {
        QHostInfo info = QHostInfo::fromName(host);

        if (info.error() == QHostInfo::NoError)
        {
            connectTo(name, info.addresses()[0], port);
        }
        else
        {
            QHostAddress addr(name);

            if (addr.protocol() != QAbstractSocket::UnknownNetworkLayerProtocol)
            {
                connectTo(name, addr, port);
            }
            else
            {
                // throw here?
            }
        }
    }

    void Client::newConnection(Connection* connection)
    {
        connection->setGreetingMessage(m_contactName, m_contactApp);

        connect(connection, SIGNAL(error(QAbstractSocket::SocketError)), this,
                SLOT(connectionError(QAbstractSocket::SocketError)));
        connect(connection, SIGNAL(disconnected()), this, SLOT(disconnected()));
        connect(connection, SIGNAL(readyForUse()), this, SLOT(readyForUse()));
        connect(connection, SIGNAL(requestGreeting()), this,
                SLOT(requestGreeting()));
    }

    void Client::disconnectFrom(const QString& contact)
    {
        DB("Client::disconnectFrom " << contact.toUtf8().constData());
        if (Connection* c = connectionByName(contact))
        {
            DB("    found connection");
            c->disconnectFromHost();
        }
    }

    Connection* Client::connectionByName(const QString& contact) const
    {
        for (ConnectionMap::const_iterator i = m_connectionMap.constBegin();
             i != m_connectionMap.constEnd(); ++i)
        {
            Connection* c = *i;
            if (c->remoteContactName() == contact)
                return c;
        }

        return 0;
    }

    bool Client::isIncoming(const QString& contact)
    {
        DB("Client::isIncoming " << contact.toUtf8().constData());
        if (Connection* c = connectionByName(contact))
        {
            DB("    found connection. Incoming:" << c->incoming());
            return c->incoming();
        }
        return false;
    }

    void Client::rejectConnection(Connection* c)
    {
        c->abort();
        m_connectionMap.remove(c->peerAddress(), c);
    }

    void Client::requestGreeting()
    {
        if (Connection* c = dynamic_cast<Connection*>(sender()))
        {
            emit requestConnection(c);
        }
    }

    void Client::readyForUse()
    {
        DB("Client::readyForUse");
        Connection* connection = qobject_cast<Connection*>(sender());
        if (!connection
            || hasConnection(connection->peerAddress(), connection->peerPort())
            || connectionByName(connection->remoteContactName()))
            return;

        connect(connection, SIGNAL(newMessage(const QString&, const QString&)),
                this, SIGNAL(newMessage(const QString&, const QString&)));

        connect(
            connection,
            SIGNAL(newData(const QString&, const QString&, const QByteArray&)),
            this,
            SIGNAL(newData(const QString&, const QString&, const QByteArray&)));

        m_connectionMap.insert(connection->peerAddress(), connection);
        QString contact = connection->remoteContactName();
        if (!m_pingpong)
            connection->pingPongControl(false);
        if (!contact.isEmpty())
            emit newContact(contact);
    }

    void Client::disconnected()
    {
        if (Connection* connection = qobject_cast<Connection*>(sender()))
            removeConnection(connection);
    }

    void Client::connectionError(QAbstractSocket::SocketError err)
    {
        if (Connection* connection = qobject_cast<Connection*>(sender()))
        {
            DB("connectionError state " << connection->state() << " incoming "
                                        << connection->incoming());
            //
            //  Ignore errors on incoming connections before greeting
            //
            if (connection->incoming()
                && connection->state() == Connection::WaitingForGreeting)
                return;

            QString msg = connection->errorString();
            QString cname = connection->remoteContactName();

            if (cname == "unknown")
                cname = connection->expectedContactName();

            emit contactError(
                cname,
                // cname + "@" + connection->expectedAddress().toString(),
                connection->expectedAddress().toString(), msg);

            QString name = connection->remoteContactName();
            emit contactLeft(name);

            removeConnection(connection);
        }
    }

    void Client::removeConnection(Connection* connection)
    {
        DB("Client::removeConnection dupe " << connection->duplicate());
        if (m_connectionMap.contains(connection->peerAddress()))
        {
            DB("    removing connection from map");
            m_connectionMap.remove(connection->peerAddress(), connection);

            if (!connection->duplicate()
                && !connection->remoteContactName().isEmpty())
            {
                emit contactLeft(connection->remoteContactName());
            }
        }

        connection->deleteLater();
    }

    void Client::signOff(const QString& contact)
    {
        sendMessage(contact, "DISCONNECT");
        waitForSend(contact);
    }

    bool Client::online()
    {
        return (m_server) ? m_server->isListening() : false;
    }

} // namespace TwkQtChat
