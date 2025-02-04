//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <MuQt5/qtUtils.h>
#include <MuQt5/QHelpEventType.h>
#include <QtGui/QtGui>
#include <QtWidgets/QtWidgets>
#include <QtSvg/QtSvg>
#include <QtNetwork/QtNetwork>
#include <MuQt5/QWidgetType.h>
#include <MuQt5/QActionType.h>
#include <MuQt5/QIconType.h>
#include <Mu/BaseFunctions.h>
#include <Mu/Alias.h>
#include <Mu/SymbolicConstant.h>
#include <Mu/Thread.h>
#include <Mu/ClassInstance.h>
#include <Mu/Function.h>
#include <Mu/MemberFunction.h>
#include <Mu/MemberVariable.h>
#include <Mu/Node.h>
#include <Mu/Exception.h>
#include <Mu/ParameterVariable.h>
#include <Mu/ReferenceType.h>
#include <Mu/Value.h>
#include <MuLang/MuLangContext.h>
#include <MuLang/StringType.h>
#include <MuQt5/QPointType.h>

//
//  NOTE: this file was automatically generated by qt2mu.py
//

namespace Mu
{
    using namespace std;

    QHelpEventType::QHelpEventType(Context* c, const char* name, Class* super)
        : Class(c, name, super)
    {
    }

    QHelpEventType::~QHelpEventType() {}

    //----------------------------------------------------------------------
    //  PRE-COMPILED FUNCTIONS

    Pointer qt_QHelpEvent_globalPos_QPoint_QHelpEvent(Mu::Thread& NODE_THREAD,
                                                      Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QHelpEvent* arg0 = getqpointer<QHelpEventType>(param_this);
        return makeqtype<QPointType>(c, arg0->globalPos(), "qt.QPoint");
    }

    int qt_QHelpEvent_globalX_int_QHelpEvent(Mu::Thread& NODE_THREAD,
                                             Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QHelpEvent* arg0 = getqpointer<QHelpEventType>(param_this);
        return arg0->globalX();
    }

    int qt_QHelpEvent_globalY_int_QHelpEvent(Mu::Thread& NODE_THREAD,
                                             Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QHelpEvent* arg0 = getqpointer<QHelpEventType>(param_this);
        return arg0->globalY();
    }

    Pointer qt_QHelpEvent_pos_QPoint_QHelpEvent(Mu::Thread& NODE_THREAD,
                                                Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QHelpEvent* arg0 = getqpointer<QHelpEventType>(param_this);
        return makeqtype<QPointType>(c, arg0->pos(), "qt.QPoint");
    }

    int qt_QHelpEvent_x_int_QHelpEvent(Mu::Thread& NODE_THREAD,
                                       Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QHelpEvent* arg0 = getqpointer<QHelpEventType>(param_this);
        return arg0->x();
    }

    int qt_QHelpEvent_y_int_QHelpEvent(Mu::Thread& NODE_THREAD,
                                       Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QHelpEvent* arg0 = getqpointer<QHelpEventType>(param_this);
        return arg0->y();
    }

    static NODE_IMPLEMENTATION(_n_globalPos0, Pointer)
    {
        NODE_RETURN(qt_QHelpEvent_globalPos_QPoint_QHelpEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_globalX0, int)
    {
        NODE_RETURN(qt_QHelpEvent_globalX_int_QHelpEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_globalY0, int)
    {
        NODE_RETURN(qt_QHelpEvent_globalY_int_QHelpEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_pos0, Pointer)
    {
        NODE_RETURN(qt_QHelpEvent_pos_QPoint_QHelpEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_x0, int)
    {
        NODE_RETURN(qt_QHelpEvent_x_int_QHelpEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_y0, int)
    {
        NODE_RETURN(qt_QHelpEvent_y_int_QHelpEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    void QHelpEventType::load()
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

                   new MemberVariable(c, "native", "qt.NativeObject"),

                   EndArguments);

        addSymbols(EndArguments);

        addSymbols(
            // enums
            // member functions
            // MISSING: QHelpEvent (QHelpEvent; QHelpEvent this, "Type" type,
            // QPoint pos, QPoint globalPos)
            new Function(c, "globalPos", _n_globalPos0, None, Compiled,
                         qt_QHelpEvent_globalPos_QPoint_QHelpEvent, Return,
                         "qt.QPoint", Parameters,
                         new Param(c, "this", "qt.QHelpEvent"), End),
            new Function(c, "globalX", _n_globalX0, None, Compiled,
                         qt_QHelpEvent_globalX_int_QHelpEvent, Return, "int",
                         Parameters, new Param(c, "this", "qt.QHelpEvent"),
                         End),
            new Function(c, "globalY", _n_globalY0, None, Compiled,
                         qt_QHelpEvent_globalY_int_QHelpEvent, Return, "int",
                         Parameters, new Param(c, "this", "qt.QHelpEvent"),
                         End),
            new Function(c, "pos", _n_pos0, None, Compiled,
                         qt_QHelpEvent_pos_QPoint_QHelpEvent, Return,
                         "qt.QPoint", Parameters,
                         new Param(c, "this", "qt.QHelpEvent"), End),
            new Function(c, "x", _n_x0, None, Compiled,
                         qt_QHelpEvent_x_int_QHelpEvent, Return, "int",
                         Parameters, new Param(c, "this", "qt.QHelpEvent"),
                         End),
            new Function(c, "y", _n_y0, None, Compiled,
                         qt_QHelpEvent_y_int_QHelpEvent, Return, "int",
                         Parameters, new Param(c, "this", "qt.QHelpEvent"),
                         End),
            // static functions
            EndArguments);
        globalScope()->addSymbols(EndArguments);
        scope()->addSymbols(EndArguments);
    }

} // namespace Mu
