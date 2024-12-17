//
//  Copyright (c) 2008 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __TwkQtChat__Connection__h__
#define __TwkQtChat__Connection__h__
#include <iostream>
#include <QtNetwork/QHostAddress>
#include <QtCore/QString>
#include <QtNetwork/QTcpSocket>
#include <QtCore/QTime>
#include <QtCore/QTimer>

namespace TwkQtChat
{
    static const int MaxBufferSize = 1024000;

    class Connection : public QTcpSocket
    {
        Q_OBJECT

    public:
        enum ConnectionState
        {
            WaitingForGreeting,
            ReadingGreeting,
            ReadyForUse
        };

        enum DataType
        {
            PlainText,
            RawData,
            Ping,
            Pong,
            Greeting,
            NewGreeting,
            PingPongControl,
            Undefined
        };

        Connection(QObject* parent, bool pingpong);
        ~Connection();

        virtual const QString& remoteName() const;
        virtual const QString& remoteApp() const;
        virtual const QString& remoteContactName() const;
        const QString& expectedContactName() const;
        const QHostAddress& expectedAddress() const;

        void setGreetingMessage(const QString& name, const QString& app = "rv");
        virtual bool sendMessage(const QString& message);

        virtual bool sendData(const QString& interp, const QByteArray& data);

        void connectToContact(const QString& name, const QHostAddress& ip,
                              quint16 port, OpenMode openMode = ReadWrite);

        virtual bool isLocal() { return m_local; };

        void pingPongControl(bool ppOn);

        bool duplicate() { return m_duplicate; };

        void setDuplicate(bool d) { m_duplicate = d; }

        ConnectionState state() { return m_state; }

        bool incoming() { return m_incoming; }

    signals:
        void readyForUse();
        void newMessage(const QString& from, const QString& message);
        void newData(const QString& from, const QString& interp,
                     const QByteArray& data);
        void requestGreeting();

    protected:
        void timerEvent(QTimerEvent* timerEvent);

    protected slots:
        void processReadyRead();
        void sendPing();
        void sendGreetingMessage();

    private:
        int readDataIntoBuffer(int maxSize = MaxBufferSize);
        int dataLengthForCurrentDataType();
        bool readProtocolHeader();
        bool hasEnoughData();
        void processData();
        void startPingPong();
        void stopPingPong();

        QString m_greetingName;
        QString m_greetingApp;
        QString m_remoteName;
        QString m_remoteApp;
        QString m_remoteContactName;
        QString m_expectedContactName;
        QHostAddress m_expectedAddress;
        QTimer m_pingTimer;
        QTime m_pongTime;
        QByteArray m_buffer;
        ConnectionState m_state;
        DataType m_currentDataType;
        QString m_dataInterp;
        int m_numBytesForCurrentDataType;
        int m_transferTimerId;
        bool m_isGreetingMessageSent;
        bool m_local;
        bool m_pingpong;
        bool m_duplicate;
        bool m_incoming;
    };

    typedef Connection* (*ConnectionFactory)(QObject*, bool);

} // namespace TwkQtChat

#endif // __TwkQtChat__Connection__h__
