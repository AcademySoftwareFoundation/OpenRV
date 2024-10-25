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

#ifndef __MuQt5__QInputEventType__h__
#define __MuQt5__QInputEventType__h__
#include <iostream>
#include <Mu/Class.h>

namespace Mu {

class QInputEventType : public Class
{
  public:
    //
    //  Types
    //

    typedef QInputEvent ValueType;

    struct Struct
    {
        QInputEvent* object;
    };

    //
    //  Constructors
    //

    QInputEventType(Context* context, const char* name, Class* superClass = 0);
    virtual ~QInputEventType();

    //
    //  Class API
    //

    virtual void load();
};

} // Mu

#endif // __MuQt5__QInputEventType__h__
