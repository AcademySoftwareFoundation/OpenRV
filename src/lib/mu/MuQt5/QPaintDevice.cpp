//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <MuQt5/Bridge.h>
#include <MuQt5/QPaintDeviceType.h>
#include <MuQt5/QWidgetType.h>
#include <MuQt5/QObjectType.h>
#include <Mu/BaseFunctions.h>
#include <Mu/ClassInstance.h>
#include <Mu/Function.h>
#include <Mu/MemberFunction.h>
#include <Mu/MemberVariable.h>
#include <Mu/Node.h>
#include <Mu/ParameterVariable.h>
#include <Mu/ReferenceType.h>
#include <Mu/Thread.h>
#include <Mu/Value.h>
#include <MuLang/MuLangContext.h>
#include <MuLang/StringType.h>

namespace Mu
{
    using namespace std;

    QPaintDeviceType::QPaintDeviceType(Context* c, const char* name,
                                       Class* super)
        : Class(c, name, super)
    {
    }

    QPaintDeviceType::~QPaintDeviceType() {}

    //----------------------------------------------------------------------
    //  PRE-COMPILED FUNCTIONS

    static Pointer QPaintDevice_QPaintDevice_QObject(Thread& NODE_THREAD,
                                                     Pointer obj, Pointer p)
    {
        ClassInstance* self = reinterpret_cast<ClassInstance*>(obj);
        ClassInstance* parent = reinterpret_cast<ClassInstance*>(p);
        QObject* parentObject = parent ? object<QObject>(parent) : 0;
        setobject(self, new QPaintDevice(parentObject));
        return obj;
    }

    static Pointer QPaintDevice_QObject(Thread& NODE_THREAD, Pointer obj)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        ClassInstance* widget = reinterpret_cast<ClassInstance*>(obj);
        QWidget* w = object<QPaintDevice>(widget);
        QPaintDeviceType* type =
            c->findSymbolOfTypeByQualifiedName<QPaintDeviceType>(
                c->internName("qt.QPaintDevice"), false);
        ClassInstance* o = ClassInstance::allocate(type);
        setobject(o, w);
        return o;
    }

    //----------------------------------------------------------------------
    //  INTERPRETER FUNCTIONS

    static NODE_IMPLEMENTATION(construct, Pointer)
    {
        NODE_RETURN(QPaintDevice_QPaintDevice_QObject(
            NODE_THREAD, NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
    }

    static NODE_IMPLEMENTATION(castFromObject, Pointer)
    {
        NODE_RETURN(QPaintDevice_QObject(NODE_THREAD, NODE_ARG(0, Pointer)));
    }

    //----------------------------------------------------------------------

    void QPaintDeviceType::load()
    {
        USING_MU_FUNCTION_SYMBOLS;
        MuLangContext* c = static_cast<MuLangContext*>(context());
        Module* global = globalModule();

        const string typeName = name();
        const string refTypeName = typeName + "&";
        const string fullTypeName = fullyQualifiedName();
        const string fullRefTypeName = fullTypeName + "&";
        const char* tn = typeName.c_str();
        const char* ftn = fullTypeName.c_str();
        const char* rtn = refTypeName.c_str();
        const char* frtn = fullRefTypeName.c_str();

        scope()->addSymbols(new ReferenceType(c, rtn, this),

                            new Function(c, tn, BaseFunctions::dereference,
                                         Cast, Return, ftn, Args, frtn, End),

                            EndArguments);

        addSymbols(new Function(c, "__allocate", BaseFunctions::classAllocate,
                                None, Return, ftn, End),

                   new Function(
                       c, tn, construct, None, Compiled,
                       QPaintDevice_QPaintDevice_QObject, Return, ftn,
                       Parameters, new Param(c, "this", ftn),
                       new Param(c, "parent", "qt.QWidget", Value(Pointer(0))),
                       End),

                   new Function(c, tn, castFromObject, Cast, Compiled,
                                QPaintDevice_QObject, Return, ftn, Parameters,
                                new Param(c, "object", "qt.QObject"), End),

                   EndArguments);

        populate(this, QPaintDevice::staticMetaObject);
    }

} // namespace Mu
