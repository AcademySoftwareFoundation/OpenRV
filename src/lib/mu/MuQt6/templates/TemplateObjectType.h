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

#ifndef __MuQt6__$TType__h__
#define __MuQt6__$TType__h__
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
    {
        % % muqtForwardDeclaration % %
    }

    class $TType : public Class
    {
    public:
        {
            % % typeDeclarations % %
        }
        typedef MuQt_$T MuQtType;
        typedef $T QtType;

        //
        //  Constructors
        //

        $TType(Context* context, const char* name, Class* superClass = 0,
               Class* superClass2 = 0);

        virtual ~$TType();

        {
            % % isInheritableFunc % %
        }
        static inline ClassInstance* cachedInstance(const MuQtType*);

        //
        //  Class API
        //

        virtual void load();

        {
            % % virtualArray % %
        }
    };

    {
        % % nativeMuQtClass % %
    }
    {
        % % cachedInstanceFunc % %
    }

} // namespace Mu

#endif // __MuQt__$TType__h__
