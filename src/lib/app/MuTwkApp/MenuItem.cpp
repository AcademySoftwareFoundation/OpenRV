//******************************************************************************
// Copyright (c) 2003 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <MuTwkApp/MenuItem.h>
#include <Mu/BaseFunctions.h>
#include <Mu/Function.h>
#include <Mu/MemberVariable.h>
#include <Mu/Module.h>
#include <Mu/ParameterVariable.h>
#include <Mu/MuProcess.h>
#include <Mu/ReferenceType.h>
#include <Mu/Thread.h>
#include <MuLang/MuLangContext.h>
#include <string.h>
#include <string>

namespace TwkApp
{
    using namespace Mu;
    using namespace std;

    //----------------------------------------------------------------------

    MenuItem::MenuItem(Context* c, const char* name, Class* super)
        : Class(c, name, super)
    {
    }

    MenuItem::~MenuItem() {}

    void MenuItem::load()
    {
        USING_MU_FUNCTION_SYMBOLS;

        Symbol* s = scope();
        MuLangContext* context = (MuLangContext*)globalModule()->context();
        Context* c = context;

        const char* className = "MenuItem";
        const char* refName = "MenuItem&";

        string tname = s->name().c_str();
        tname += ".";
        tname += className;
        // string tname = "widget";

        string rname = tname + "&";

        const char* tn = tname.c_str();
        const char* rn = rname.c_str();

        const char* mi = "MenuItem[]";
        const char* ft = "(void;Event)";
        const char* bft = "(int;)";

        s->addSymbols(new ReferenceType(c, refName, this),

                      new Function(c, className, BaseFunctions::dereference,
                                   Cast, Return, tn, Args, rn, End),

                      EndArguments);

        globalScope()->addSymbols(

            new Function(c, "=", BaseFunctions::assign, AsOp, Return, rn, Args,
                         rn, tn, End),

            EndArguments);

        //
        //  Layout of struct
        //

        context->functionType(ft);
        context->functionType(bft);
        context->arrayType(this, 1, 0);

        addSymbols(
            new MemberVariable(c, "label", "string"),
            new MemberVariable(c, "actionHook", ft),
            new MemberVariable(c, "key", "string"),
            new MemberVariable(c, "stateHook", bft),
            new MemberVariable(c, "subMenu", mi),

            new Function(c, "__allocate", BaseFunctions::classAllocate,
                         Function::None, Function::Return, tn, Function::End),

            new Function(
                c, className, MenuItem::construct, None, Return, tn, Parameters,
                new ParameterVariable(c, "this", tn),
                new ParameterVariable(c, "label", "string"),
                new ParameterVariable(c, "actionHook", ft),
                new ParameterVariable(c, "key", "string", Value(Pointer(0))),
                new ParameterVariable(c, "stateHook", bft, Value(Pointer(0))),
                new ParameterVariable(c, "subMenu", mi, Value(Pointer(0))),
                End),

            new Function(c, className, MenuItem::construct2, None, Return, tn,
                         Parameters, new ParameterVariable(c, "this", tn),
                         new ParameterVariable(c, "label", "string"),
                         new ParameterVariable(c, "subMenu", mi), End),

            EndArguments);
    }

    NODE_IMPLEMENTATION(MenuItem::construct, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const Class* c = static_cast<const MenuItem*>(NODE_THIS.type());

        ClassInstance* o = NODE_ARG_OBJECT(0, ClassInstance);
        StringType::String* label = NODE_ARG_OBJECT(1, StringType::String);
        FunctionObject* fobj = NODE_ARG_OBJECT(2, FunctionObject);
        StringType::String* key = NODE_ARG_OBJECT(3, StringType::String);
        FunctionObject* sobj = NODE_ARG_OBJECT(4, FunctionObject);
        DynamicArray* array = NODE_ARG_OBJECT(5, DynamicArray);

        Struct* s = o->data<Struct>();

        s->label = label;
        s->key = key;
        s->actionCB = fobj;
        s->stateCB = sobj;
        s->subMenu = array;

        NODE_RETURN(Pointer(o));
    }

    NODE_IMPLEMENTATION(MenuItem::construct2, Pointer)
    {
        Process* p = NODE_THREAD.process();
        const Class* c = static_cast<const MenuItem*>(NODE_THIS.type());

        ClassInstance* o = NODE_ARG_OBJECT(0, ClassInstance);
        StringType::String* label = NODE_ARG_OBJECT(1, StringType::String);
        DynamicArray* array = NODE_ARG_OBJECT(2, DynamicArray);
        Struct* s = o->data<Struct>();

        s->label = label;
        s->key = 0;
        s->actionCB = 0;
        s->stateCB = 0;
        s->subMenu = array;

        NODE_RETURN(Pointer(o));
    }

} // namespace TwkApp
