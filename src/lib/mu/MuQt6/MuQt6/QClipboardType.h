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

#ifndef __MuQt6__QClipboardType__h__
#define __MuQt6__QClipboardType__h__
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

    class QClipboardType : public Class
    {
    public:
        typedef QClipboard MuQt_QClipboard;
        typedef MuQt_QClipboard MuQtType;
        typedef QClipboard QtType;

        //
        //  Constructors
        //

        QClipboardType(Context* context, const char* name,
                       Class* superClass = 0, Class* superClass2 = 0);

        virtual ~QClipboardType();

        static bool isInheritable() { return false; }

        static inline ClassInstance* cachedInstance(const MuQtType*);

        //
        //  Class API
        //

        virtual void load();

        MemberFunction* _func[4];
    };

    inline ClassInstance*
    QClipboardType::cachedInstance(const QClipboardType::MuQtType* obj)
    {
        return 0;
    }

} // namespace Mu

#endif // __MuQt__QClipboardType__h__
