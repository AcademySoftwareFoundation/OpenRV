//
//  Copyright (c) 2017 Autodesk.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __TwkQtCoreUtil__BoostBridge__h__
#define __TwkQtCoreUtil__BoostBridge__h__
#include <QtCore/QCoreApplication>
#include <QtCore/QEvent>
#include <iostream>

namespace TwkQtCoreUtil
{

    //
    //    QThread t;
    //    QObject o;
    //    o.moveToThread(&t);
    //
    //    postToQObject([&]{ o.setObjectName("hello"); }, &o); // Execute in
    //    given object's thread
    //      -or-
    //    postToQObject(std::bind(&QObject::setObjectName, &o, "hello"), &o);
    //

    template <typename F>
    static void postToQObject(F&& fun, QObject* obj = qApp)
    {
        //
        //  Creates a queued connection with a dummy object and the incoming
        //  object. When the scope exits the dummy destroyed signal is emitted
        //  to the other object. The queued connection ensures that the signal
        //  is handled by the thread that owns the object via its event
        //  handler.
        //

        QObject src;
        QObject::connect(&src, &QObject::destroyed, obj, std::forward<F>(fun),
                         Qt::QueuedConnection);
    }

} // namespace TwkQtCoreUtil

#endif // __TwkQtCoreUtil__ThreadTools__h__
