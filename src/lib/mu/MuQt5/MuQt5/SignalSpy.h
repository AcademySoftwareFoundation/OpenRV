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
#include <QtTest/QtTest>

namespace Mu
{

    class SignalSpy : public QSignalSpy
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

    private:
        const Function* _F;
        Process* _process;
        const CallEnvironment* _env;
        std::vector<Types> _argTypes;
    };

} // namespace Mu

#endif // __MuQt__SignalSpy__h__
