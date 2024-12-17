//*****************************************************************************
// Copyright (c) 2019 Autodesk, Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//*****************************************************************************

#ifndef __MuQt5__QWebEngineCookieStoreType__h__
#define __MuQt5__QWebEngineCookieStoreType__h__
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
    class MuQt_QWebEngineCookieStore;

    //
    //  NOTE: file generated by qt2mu.py
    //

    class QWebEngineCookieStoreType : public Class
    {
    public:
        typedef MuQt_QWebEngineCookieStore MuQtType;
        typedef QWebEngineCookieStore QtType;

        //
        //  Constructors
        //

        QWebEngineCookieStoreType(Context* context, const char* name,
                                  Class* superClass = 0,
                                  Class* superClass2 = 0);

        virtual ~QWebEngineCookieStoreType();

        static bool isInheritable() { return true; }

        static inline ClassInstance* cachedInstance(const MuQtType*);

        //
        //  Class API
        //

        virtual void load();

        MemberFunction* _func[4];
    };

    // Inheritable object

    class MuQt_QWebEngineCookieStore : public QWebEngineCookieStore
    {
    public:
        virtual ~MuQt_QWebEngineCookieStore();
        virtual bool event(QEvent* e);
        virtual bool eventFilter(QObject* watched, QEvent* event);

    protected:
        virtual void customEvent(QEvent* event);
        virtual void timerEvent(QTimerEvent* event);

    public:
        void customEvent_pub(QEvent* event) { customEvent(event); }

        void customEvent_pub_parent(QEvent* event)
        {
            QWebEngineCookieStore::customEvent(event);
        }

        void timerEvent_pub(QTimerEvent* event) { timerEvent(event); }

        void timerEvent_pub_parent(QTimerEvent* event)
        {
            QWebEngineCookieStore::timerEvent(event);
        }

    public:
        const QWebEngineCookieStoreType* _baseType;
        ClassInstance* _obj;
        const CallEnvironment* _env;
    };

    inline ClassInstance* QWebEngineCookieStoreType::cachedInstance(
        const QWebEngineCookieStoreType::MuQtType* obj)
    {
        return obj->_obj;
    }

} // namespace Mu

#endif // __MuQt__QWebEngineCookieStoreType__h__
