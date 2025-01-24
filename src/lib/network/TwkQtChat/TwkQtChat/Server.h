//
//  Copyright (c) 2008 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __TwkQtChat__Server__h__
#define __TwkQtChat__Server__h__
#include <QtNetwork/QTcpServer>
#include <TwkQtChat/Connection.h>
#include <iostream>

namespace TwkQtChat
{
    class Connection;

    class Server : public QTcpServer
    {
        Q_OBJECT

    public:
        Server(QObject* parent = 0, int port = 45124,
               ConnectionFactory fact = 0);

    signals:
        void newConnection(Connection* connection);

    protected:
        void incomingConnection(qintptr socketDescriptor) override;

    private:
        ConnectionFactory m_connectionFactory;
    };

} // namespace TwkQtChat

#endif // __TwkQtChat__Server__h__
