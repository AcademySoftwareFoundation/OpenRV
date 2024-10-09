//*****************************************************************************
// Copyright (c) 2024 Autodesk, Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//*****************************************************************************

// IMPORTANT: This file (not the template) is auto-generated by qt2mu.py script.
//            The prefered way to do a fix is to handrolled it or modify the qt2mu.py script.
//            If it is not possible, manual editing is ok but it could be lost in future generations.

#ifndef __MuQt6__QApplicationType__h__
#define __MuQt6__QApplicationType__h__
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
#include <MuQt6/Bridge.h>

namespace Mu {
class MuQt_QApplication;

//
//  NOTE: file generated by qt2mu.py
//

class QApplicationType : public Class
{
  public:

    typedef MuQt_QApplication MuQtType;
    typedef QApplication QtType;

    //
    //  Constructors
    //

    QApplicationType(Context* context, 
           const char* name,
           Class* superClass = 0,
           Class* superClass2 = 0);

    virtual ~QApplicationType();

    static bool isInheritable() { return true; }
    static inline ClassInstance* cachedInstance(const MuQtType*);

    //
    //  Class API
    //

    virtual void load();

    MemberFunction* _func[2];
};

// Inheritable object

class MuQt_QApplication : public QApplication
{
  public:
    virtual ~MuQt_QApplication();
    virtual bool notify(QObject * receiver, QEvent * e) ;
  protected:
    virtual bool event(QEvent * e) ;
  public:
    bool event_pub(QEvent * e)  { return event(e); }
    bool event_pub_parent(QEvent * e)  { return QApplication::event(e); }
  public:
    const QApplicationType* _baseType;
    ClassInstance* _obj;
    const CallEnvironment* _env;
};

inline ClassInstance* QApplicationType::cachedInstance(const QApplicationType::MuQtType* obj) { return obj->_obj; }

} // Mu

#endif // __MuQt__QApplicationType__h__
