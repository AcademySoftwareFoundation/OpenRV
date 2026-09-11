//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __MuQt__SignalSpy__h__
#define __MuQt__SignalSpy__h__
#include <iostream>
#include <Mu/FunctionObject.h>
#include <Mu/Thread.h>

//
//  SignalSpy hijacks qt_metacall (see SignalSpy.cpp) to route a Qt signal to a
//  Mu function, which requires being a QObject.
//
//  Up to Qt 6.5 it derived from QSignalSpy, which was itself a QObject and
//  also resolved and connected the signal for us. As of Qt 6.8 QSignalSpy is
//  no longer a QObject (it derives only from QList<QList<QVariant>> and works
//  through a private QSignalSpyPrivate), and its "args" member became private,
//  so there is nothing left to hook into.
//
//  Rather than change behaviour on the Qt versions where the original works,
//  keep the QSignalSpy implementation for Qt < 6.8 (VFX platform 2024/2025,
//  Qt 6.5.3) and derive from QObject only where we have to.
//
//  RV_MUQT_SIGNALSPY_OWN_CONNECT is defined by MuQt6/CMakeLists.txt when Qt is
//  6.8 or newer, and is passed to both the compiler and moc. Do NOT switch on
//  QT_VERSION here: moc does not evaluate it, so moc would pick a different
//  base class than the compiler and generate a meta object for the wrong one.
//
#include <QtCore/QList>

#ifdef RV_MUQT_SIGNALSPY_OWN_CONNECT
#include <QtCore/QMetaMethod>
#include <QtCore/QObject>
#else
#include <QtTest/QtTest>
#endif

namespace Mu
{

#ifdef RV_MUQT_SIGNALSPY_OWN_CONNECT
    class SignalSpy : public QObject
#else
    class SignalSpy : public QSignalSpy
#endif
    {
        Q_OBJECT

    public:
        enum Types
        {
            UnknownArg,
            IntArg,
            StringArg,
            BoolArg,
            PointArg,
            ObjectArg,
            ActionArg,
            ColorArg,
            TreeItemArg,
            ListItemArg,
            TableItemArg,
            StandardItemArg,
            ModelIndexArg,
            ItemSelectionArg,
            UrlArg,
            VariantArg
        };

        SignalSpy(QObject*, const char* signal, const Function* F, Process* p);

        virtual ~SignalSpy();

        int original_qt_metacall(QMetaObject::Call, int, void**);

#ifdef RV_MUQT_SIGNALSPY_OWN_CONNECT
        //  True if the signal was found and connected. Only meaningful for the
        //  QObject-based implementation; QSignalSpy has its own isValid().
        bool isValid() const { return _valid; }

    private:
        //  Resolves signal and connects it to this object. Returns false if
        //  the signal could not be found or connected.
        bool connectToSignal(QObject* sender, const char* signal);
#endif

    private:
        //  Metatype ids of the connected signal's arguments, needed to turn the
        //  void** qt_metacall() receives into QVariants. Defined out of line in
        //  SignalSpy.cpp: for Qt < 6.8 it returns QSignalSpy::args, which is
        //  only reachable there.
        const QList<int>& signalArgTypes() const;

    private:
        const Function* _F;
        Process* _process;
        const CallEnvironment* _env;
        std::vector<Types> _argTypes;
#ifdef RV_MUQT_SIGNALSPY_OWN_CONNECT
        //  Metatype ids of the connected signal's arguments. This replaces
        //  QSignalSpy::args, which is private as of Qt 6.8.
        QList<int> _signalArgTypes;
        bool _valid;
#endif
    };

} // namespace Mu

#endif // __MuQt__SignalSpy__h__
