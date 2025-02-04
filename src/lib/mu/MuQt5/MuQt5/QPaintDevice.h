//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __MuQt__QPaintDevice__h__
#define __MuQt__QPaintDevice__h__
#include <iostream>
#include <Mu/Class.h>
#include <QtGui/QtGui>

namespace Mu
{

    class QPaintDeviceType : public Class
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

        QPaintDeviceType(Context* context, const char* name,
                         Class* superClass = 0);
        virtual ~QPaintDeviceType();

        //
        //  Class API
        //

        virtual void load();
    };

} // namespace Mu

#endif // __MuQt__QPaintDevice__h__
