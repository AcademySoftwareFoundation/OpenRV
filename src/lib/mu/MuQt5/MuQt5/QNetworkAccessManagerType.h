//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __MuQt5__QNetworkAccessManagerType__h__
#define __MuQt5__QNetworkAccessManagerType__h__
#include <iostream>
#include <Mu/Class.h>
#include <Mu/MuProcess.h>
#include <QtCore/QtCore>
#include <QtGui/QtGui>
#include <QtWidgets/QtWidgets>
#include <QtNetwork/QtNetwork>
#include <QtWebEngine/QtWebEngine>
#include <QtWebEngineWidgets/QtWebEngineWidgets>
#include <QtQml/QtQml>
#include <QtQuick/QtQuick>
#include <QtQuickWidgets/QtQuickWidgets>
#include <QtSvg/QtSvg>
#include <MuQt5/Bridge.h>

namespace Mu
{
    class MuQt_QNetworkAccessManager;

    //
    //  NOTE: file generated by qt2mu.py
    //

    class QNetworkAccessManagerType : public Class
    {
    public:
        typedef MuQt_QNetworkAccessManager MuQtType;
        typedef QNetworkAccessManager QtType;

        //
        //  Constructors
        //

        QNetworkAccessManagerType(Context* context, const char* name,
                                  Class* superClass = 0,
                                  Class* superClass2 = 0);

        virtual ~QNetworkAccessManagerType();

        static bool isInheritable() { return true; }

        static inline ClassInstance* cachedInstance(const MuQtType*);

        //
        //  Class API
        //

        virtual void load();

        MemberFunction* _func[4];
    };

    // Inheritable object

    class MuQt_QNetworkAccessManager : public QNetworkAccessManager
    {
    public:
        virtual ~MuQt_QNetworkAccessManager();
        MuQt_QNetworkAccessManager(Pointer muobj, const CallEnvironment*,
                                   QObject* parent);
        virtual bool event(QEvent* e);
        virtual bool eventFilter(QObject* watched, QEvent* event);

    protected:
        virtual void customEvent(QEvent* event);
        virtual void timerEvent(QTimerEvent* event);

    public:
        void customEvent_pub(QEvent* event) { customEvent(event); }

        void customEvent_pub_parent(QEvent* event)
        {
            QNetworkAccessManager::customEvent(event);
        }

        void timerEvent_pub(QTimerEvent* event) { timerEvent(event); }

        void timerEvent_pub_parent(QTimerEvent* event)
        {
            QNetworkAccessManager::timerEvent(event);
        }

    public:
        const QNetworkAccessManagerType* _baseType;
        ClassInstance* _obj;
        const CallEnvironment* _env;
    };

    inline ClassInstance* QNetworkAccessManagerType::cachedInstance(
        const QNetworkAccessManagerType::MuQtType* obj)
    {
        return obj->_obj;
    }

} // namespace Mu

#endif // __MuQt__QNetworkAccessManagerType__h__
