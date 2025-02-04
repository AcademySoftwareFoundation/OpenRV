//*****************************************************************************
// Copyright (c) 2024 Autodesk, Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//*****************************************************************************

// IMPORTANT: This file (not the template) is auto-generated by qt2mu.py script.
//            The prefered way to do a fix is to handrolled it or modify the
//            qt2mu.py script. If it is not possible, manual editing is ok but
//            it could be lost in future generations.

#ifndef __MuQt6__QAbstractSocketType__h__
#define __MuQt6__QAbstractSocketType__h__
#include <iostream>
#include <Mu/Class.h>
#include <Mu/MuProcess.h>
#include <QtCore/QtCore>
#include <QtGui/QtGui>
#include <QtWidgets/QtWidgets>
#include <QtNetwork/QtNetwork>
#include <QtWebEngineWidgets/QtWebEngineWidgets>
#include <QtQml/QtQml>
#include <QtQuick/QtQuick>
#include <QtQuickWidgets/QtQuickWidgets>
#include <QtSvg/QtSvg>
#include <QSvgWidget>
#include <MuQt6/Bridge.h>

namespace Mu
{
    class MuQt_QAbstractSocket;

    class QAbstractSocketType : public Class
    {
    public:
        typedef MuQt_QAbstractSocket MuQtType;
        typedef QAbstractSocket QtType;

        //
        //  Constructors
        //

        QAbstractSocketType(Context* context, const char* name,
                            Class* superClass = 0, Class* superClass2 = 0);

        virtual ~QAbstractSocketType();

        static bool isInheritable() { return true; }

        static inline ClassInstance* cachedInstance(const MuQtType*);

        //
        //  Class API
        //

        virtual void load();

        MemberFunction* _func[21];
    };

    // Inheritable object

    class MuQt_QAbstractSocket : public QAbstractSocket
    {
    public:
        virtual ~MuQt_QAbstractSocket();
        MuQt_QAbstractSocket(Pointer muobj, const CallEnvironment*,
                             QAbstractSocket::SocketType socketType,
                             QObject* parent);
        virtual void disconnectFromHost();
        virtual void resume();
        virtual void setReadBufferSize(qint64 size);
        virtual void setSocketOption(QAbstractSocket::SocketOption option,
                                     const QVariant& value);
        virtual QVariant socketOption(QAbstractSocket::SocketOption option);
        virtual bool waitForConnected(int msecs);
        virtual bool waitForDisconnected(int msecs);
        virtual qint64 bytesAvailable() const;
        virtual qint64 bytesToWrite() const;
        virtual void close();
        virtual bool isSequential() const;
        virtual bool waitForBytesWritten(int msecs);
        virtual bool waitForReadyRead(int msecs);

    protected:
        virtual qint64 skipData(qint64 maxSize);

    public:
        virtual bool atEnd() const;
        virtual bool canReadLine() const;
        virtual bool open(QIODeviceBase::OpenMode mode);
        virtual qint64 pos() const;
        virtual bool reset();
        virtual bool seek(qint64 pos);
        virtual qint64 size() const;

    public:
        void setLocalAddress_pub(const QHostAddress& address)
        {
            setLocalAddress(address);
        }

        void setLocalAddress_pub_parent(const QHostAddress& address)
        {
            QAbstractSocket::setLocalAddress(address);
        }

        void setPeerAddress_pub(const QHostAddress& address)
        {
            setPeerAddress(address);
        }

        void setPeerAddress_pub_parent(const QHostAddress& address)
        {
            QAbstractSocket::setPeerAddress(address);
        }

        void setPeerName_pub(const QString& name) { setPeerName(name); }

        void setPeerName_pub_parent(const QString& name)
        {
            QAbstractSocket::setPeerName(name);
        }

        void setSocketError_pub(QAbstractSocket::SocketError socketError)
        {
            setSocketError(socketError);
        }

        void setSocketError_pub_parent(QAbstractSocket::SocketError socketError)
        {
            QAbstractSocket::setSocketError(socketError);
        }

        void setSocketState_pub(QAbstractSocket::SocketState state)
        {
            setSocketState(state);
        }

        void setSocketState_pub_parent(QAbstractSocket::SocketState state)
        {
            QAbstractSocket::setSocketState(state);
        }

        qint64 skipData_pub(qint64 maxSize) { return skipData(maxSize); }

        qint64 skipData_pub_parent(qint64 maxSize)
        {
            return QAbstractSocket::skipData(maxSize);
        }

    public:
        const QAbstractSocketType* _baseType;
        ClassInstance* _obj;
        const CallEnvironment* _env;
    };

    inline ClassInstance* QAbstractSocketType::cachedInstance(
        const QAbstractSocketType::MuQtType* obj)
    {
        return obj->_obj;
    }

} // namespace Mu

#endif // __MuQt__QAbstractSocketType__h__
