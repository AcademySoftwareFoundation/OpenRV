//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <QtNetwork/QtNetwork>
#include <MuQt5/qtUtils.h>
#include <MuQt5/HintWidgetType.h>
#include <MuQt5/QWidgetType.h>
#include <Mu/BaseFunctions.h>
#include <Mu/ClassInstance.h>
#include <Mu/Function.h>
#include <Mu/MemberFunction.h>
#include <Mu/MemberVariable.h>
#include <Mu/Node.h>
#include <Mu/ParameterVariable.h>
#include <Mu/ReferenceType.h>
#include <Mu/Value.h>
#include <MuLang/MuLangContext.h>
#include <MuLang/StringType.h>

#include <MuQt5/HintWidget.h>

namespace Mu
{
    using namespace std;

    HintWidgetType::HintWidgetType(Context* c, const char* name, Class* super)
        : Class(c, name, super)
    {
    }

    HintWidgetType::~HintWidgetType() {}

    //----------------------------------------------------------------------
    //  PRE-COMPILED FUNCTIONS

    static Pointer HintWidget_HintWidget(Thread& NODE_THREAD, Pointer obj,
                                         Pointer tuple, Pointer p)
    {
        ClassInstance* self = reinterpret_cast<ClassInstance*>(obj);
        ClassInstance* parent = reinterpret_cast<ClassInstance*>(p);
        ClassInstance* i = reinterpret_cast<ClassInstance*>(tuple);
        const int* fields = i->data<int>();
        QWidget* parentWidget = parent ? object<QWidget>(parent) : 0;
        setobject(self,
                  new HintWidget(parentWidget, QSize(fields[0], fields[1])));
        return obj;
    }

    static void setWidget_void_HintWidget_Widget(Thread& NODE_THREAD,
                                                 Pointer obj, Pointer wobj)
    {
        ClassInstance* self = reinterpret_cast<ClassInstance*>(obj);
        ClassInstance* other = reinterpret_cast<ClassInstance*>(wobj);
        HintWidget* w = object<HintWidget>(self);
        QWidget* wo = object<QWidget>(other);
        w->setWidget(wo);
    }

    static void setContentSize_void_HintWidget_tuple(Thread& NODE_THREAD,
                                                     Pointer obj, Pointer tuple)
    {
        ClassInstance* self = reinterpret_cast<ClassInstance*>(obj);
        HintWidget* w = object<HintWidget>(self);
        ClassInstance* i = reinterpret_cast<ClassInstance*>(tuple);
        const int* fields = i->data<int>();
        w->setContentSize(QSize(fields[0], fields[1]));
    }

    //----------------------------------------------------------------------
    //  INTERPRETER FUNCTIONS

    static NODE_IMPLEMENTATION(construct, Pointer)
    {
        NODE_RETURN(HintWidget_HintWidget(NODE_THREAD, NODE_ARG(0, Pointer),
                                          NODE_ARG(1, Pointer),
                                          NODE_ARG(2, Pointer)));
    }

    static NODE_IMPLEMENTATION(setWidget, void)
    {
        setWidget_void_HintWidget_Widget(NODE_THREAD, NODE_ARG(0, Pointer),
                                         NODE_ARG(1, Pointer));
    }

    static NODE_IMPLEMENTATION(setContentSize, void)
    {
        setContentSize_void_HintWidget_tuple(NODE_THREAD, NODE_ARG(0, Pointer),
                                             NODE_ARG(1, Pointer));
    }

    void HintWidgetType::load()
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

        Context::TypeVector types(2);
        types[0] = c->intType();
        types[1] = c->intType();
        TupleType* tt = c->tupleType(types);

        scope()->addSymbols(new ReferenceType(c, rtn, this),

                            new Function(c, tn, BaseFunctions::dereference,
                                         Cast, Return, ftn, Args, frtn, End),

                            EndArguments);

        addSymbols(
            new Function(c, "__allocate", BaseFunctions::classAllocate, None,
                         Return, ftn, End),

            new Function(
                c, tn, construct, None, Compiled, HintWidget_HintWidget, Return,
                ftn, Parameters, new Param(c, "this", ftn),
                new Param(c, "size", "(int,int)"),
                new Param(c, "parent", "qt.QWidget", Value(Pointer(0))), End),

            new Function(c, "setWidget", Mu::setWidget, None, Compiled,
                         setWidget_void_HintWidget_Widget, Return, "void",
                         Parameters, new Param(c, "this", ftn),
                         new Param(c, "widget", "qt.QWidget"), End),

            new Function(c, "setContentSize", Mu::setContentSize, None,
                         Compiled, setContentSize_void_HintWidget_tuple, Return,
                         "void", Parameters, new Param(c, "this", ftn),
                         new Param(c, "size", "(int,int)"), End),

            EndArguments);
    }

} // namespace Mu
