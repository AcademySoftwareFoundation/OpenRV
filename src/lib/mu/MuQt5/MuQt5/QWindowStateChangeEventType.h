//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __MuQt5__QWindowStateChangeEventType__h__
#define __MuQt5__QWindowStateChangeEventType__h__
#include <iostream>
#include <Mu/Class.h>

namespace Mu
{

    //
    //  NOTE: file generated by qt2mu.py
    //

    class QWindowStateChangeEventType : public Class
    {
    public:
        //
        //  Types
        //

        typedef QWindowStateChangeEvent ValueType;

        struct Struct
        {
            QWindowStateChangeEvent* object;
        };

        //
        //  Constructors
        //

        QWindowStateChangeEventType(Context* context, const char* name,
                                    Class* superClass = 0);
        virtual ~QWindowStateChangeEventType();

        //
        //  Class API
        //

        virtual void load();
    };

} // namespace Mu

#endif // __MuQt5__QWindowStateChangeEventType__h__
