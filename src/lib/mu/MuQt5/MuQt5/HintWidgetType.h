//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __MuQt__HintWidgetType__h__
#define __MuQt__HintWidgetType__h__
#include <iostream>
#include <Mu/Class.h>
#include <QtGui/QtGui>
#include <QtNetwork/QtNetwork>

namespace Mu
{

    class HintWidgetType : public Class
    {
    public:
        //
        //  Types
        //

        struct Struct
        {
        };

        //
        //  Constructors
        //

        HintWidgetType(Context* context, const char* name,
                       Class* superClass = 0);
        virtual ~HintWidgetType();

        //
        //  Class API
        //

        virtual void load();
    };

} // namespace Mu

#endif // __MuQt__HintWidgetType__h__
