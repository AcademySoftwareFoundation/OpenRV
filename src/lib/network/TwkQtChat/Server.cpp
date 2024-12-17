//
//  Copyright (c) 2008 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <TwkQtChat/Server.h>
#include <TwkQtChat/Connection.h>
#include <QtNetwork/QtNetwork>
#include <QtNetwork/QTcpSocket>

namespace TwkQtChat
{
    using namespace std;

    Server::Server(QObject* parent, int port, ConnectionFactory fact)
        : QTcpServer(parent)
        , m_connectionFactory(fact)
    {
        int myPort = 0;
        int fallbackCount = 10;
        if (getenv("TWK_SERVER_PORT_FALLBACK_COUNT"))
        {
            fallbackCount = atoi(getenv("TWK_SERVER_PORT_FALLBACK_COUNT"));
        }

        for (int p = port; p <= port + fallbackCount; ++p)
        {
            QTcpSocket probe;
            probe.connectToHost(QHostAddress("127.0.0.1"), p);
            if (probe.waitForConnected(300) && probe.isValid())
            {
                probe.disconnectFromHost();
            }
            else
            {
                myPort = p;
                break;
            }
        }
        if (myPort)
        {
            // Disable application-wide proxy if any for this QTcpServer
            setProxy(QNetworkProxy::NoProxy);

            if (!listen(QHostAddress::Any, myPort))
            {
                cerr << "ERROR: RvNetwork could not listen to the network port "
                     << port << " - " << errorString().toStdString() << endl;
            }
        }
        else
        {
            cerr << "ERROR: RvNetwork: cannot find a free network port, tried "
                 << port << "-" << port + fallbackCount << endl;
        }
    }

    void Server::incomingConnection(qintptr socketDescriptor)
    {
        Connection* connection = 0;
        if (m_connectionFactory)
            connection = (*m_connectionFactory)(this, true);
        else
            connection = new Connection(this, true);

        connection->setSocketDescriptor(socketDescriptor);
        emit newConnection(connection);
    }

} // namespace TwkQtChat
