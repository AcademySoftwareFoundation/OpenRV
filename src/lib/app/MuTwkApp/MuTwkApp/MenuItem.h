//******************************************************************************
// Copyright (c) 2003 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __MuTwkApp__MenuItem__h__
#define __MuTwkApp__MenuItem__h__
#include <Mu/Class.h>
#include <Mu/ClassInstance.h>
#include <MuLang/DynamicArray.h>
#include <MuLang/StringType.h>
#include <Mu/FunctionObject.h>
#include <vector>

namespace TwkApp
{
    using namespace Mu;

    //
    //  MenuItem is the base of class hierarchy for UI.
    //

    class MenuItem : public Class
    {
    public:
        //
        //  ClassInstance Structure
        //

        struct Struct
        {
            StringType::String* label;
            FunctionObject* actionCB;
            StringType::String* key;
            FunctionObject* stateCB;
            DynamicArray* subMenu;
        };

        //
        //  Constructors
        //

        MenuItem(Context* c, const char* name, Class* super = 0);
        ~MenuItem();

        //
        //	Symbol API
        //

        virtual void load();

        //
        //	Constant
        //

        static NODE_DECLARATION(construct, Pointer);
        static NODE_DECLARATION(construct2, Pointer);
    };

} // namespace TwkApp

#endif // __MuTwkApp__MenuItem__h__
