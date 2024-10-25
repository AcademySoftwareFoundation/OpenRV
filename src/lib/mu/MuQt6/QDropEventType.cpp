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

#include <MuQt6/qtUtils.h>
#include <MuQt6/QDropEventType.h>
#include <QtGui/QtGui>
#include <QtWidgets/QtWidgets>
#include <QtSvg/QtSvg>
#include <QSvgWidget>
#include <QtNetwork/QtNetwork>
#include <MuQt6/QWidgetType.h>
#include <MuQt6/QActionType.h>
#include <MuQt6/QIconType.h>
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
#include <MuQt6/QPointFType.h>
#include <MuQt6/QMimeDataType.h>
#include <MuQt6/QObjectType.h>

namespace Mu {
using namespace std;

QDropEventType::QDropEventType(Context* c, const char* name, Class* super)
    : Class(c, name, super)
{
}

QDropEventType::~QDropEventType()
{
}

//----------------------------------------------------------------------
//  PRE-COMPILED FUNCTIONS

Pointer qt_QDropEvent_QDropEvent_QDropEvent_QDropEvent_QPointF_int_QMimeData_int_int_int(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_pos, int param_actions, Pointer param_data, int param_buttons, int param_modifiers, int param_type)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QPointF  arg1 = getqtype<QPointFType>(param_pos);
    Qt::DropActions arg2 = (Qt::DropActions)(param_actions);
    const QMimeData * arg3 = object<QMimeData>(param_data);
    Qt::MouseButtons arg4 = (Qt::MouseButtons)(param_buttons);
    Qt::KeyboardModifiers arg5 = (Qt::KeyboardModifiers)(param_modifiers);
    QEvent::Type arg6 = (QEvent::Type)(param_type);
    setqpointer<QDropEventType>(param_this,new QDropEvent(arg1, arg2, arg3, arg4, arg5, arg6));
    return param_this;
}

void qt_QDropEvent_acceptProposedAction_void_QDropEvent(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QDropEvent * arg0 = getqpointer<QDropEventType>(param_this);
    arg0->acceptProposedAction();
    setqpointer<QDropEventType>(param_this,arg0);
}

int qt_QDropEvent_buttons_int_QDropEvent(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QDropEvent * arg0 = getqpointer<QDropEventType>(param_this);
    return int(arg0->buttons());
}

int qt_QDropEvent_dropAction_int_QDropEvent(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QDropEvent * arg0 = getqpointer<QDropEventType>(param_this);
    return int(arg0->dropAction());
}

Pointer qt_QDropEvent_mimeData_QMimeData_QDropEvent(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QDropEvent * arg0 = getqpointer<QDropEventType>(param_this);
    return makeinstance<QMimeDataType>(c,arg0->mimeData(),"qt.QMimeData");
}

int qt_QDropEvent_modifiers_int_QDropEvent(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QDropEvent * arg0 = getqpointer<QDropEventType>(param_this);
    return int(arg0->modifiers());
}

Pointer qt_QDropEvent_position_QPointF_QDropEvent(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QDropEvent * arg0 = getqpointer<QDropEventType>(param_this);
    return makeqtype<QPointFType>(c,arg0->position(),"qt.QPointF");
}

int qt_QDropEvent_possibleActions_int_QDropEvent(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QDropEvent * arg0 = getqpointer<QDropEventType>(param_this);
    return int(arg0->possibleActions());
}

int qt_QDropEvent_proposedAction_int_QDropEvent(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QDropEvent * arg0 = getqpointer<QDropEventType>(param_this);
    return int(arg0->proposedAction());
}

void qt_QDropEvent_setDropAction_void_QDropEvent_int(Mu::Thread& NODE_THREAD, Pointer param_this, int param_action)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QDropEvent * arg0 = getqpointer<QDropEventType>(param_this);
    Qt::DropAction arg1 = (Qt::DropAction)(param_action);
    arg0->setDropAction(arg1);
    setqpointer<QDropEventType>(param_this,arg0);
}

Pointer qt_QDropEvent_source_QObject_QDropEvent(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QDropEvent * arg0 = getqpointer<QDropEventType>(param_this);
    return makeinstance<QObjectType>(c,arg0->source(),"qt.QObject");
}


static NODE_IMPLEMENTATION(_n_QDropEvent0, Pointer)
{
    NODE_RETURN(qt_QDropEvent_QDropEvent_QDropEvent_QDropEvent_QPointF_int_QMimeData_int_int_int(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer), NODE_ARG(2, int), NODE_ARG(3, Pointer), NODE_ARG(4, int), NODE_ARG(5, int), NODE_ARG(6, int)));
}

static NODE_IMPLEMENTATION(_n_acceptProposedAction0, void)
{
    qt_QDropEvent_acceptProposedAction_void_QDropEvent(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer));
}

static NODE_IMPLEMENTATION(_n_buttons0, int)
{
    NODE_RETURN(qt_QDropEvent_buttons_int_QDropEvent(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_dropAction0, int)
{
    NODE_RETURN(qt_QDropEvent_dropAction_int_QDropEvent(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_mimeData0, Pointer)
{
    NODE_RETURN(qt_QDropEvent_mimeData_QMimeData_QDropEvent(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_modifiers0, int)
{
    NODE_RETURN(qt_QDropEvent_modifiers_int_QDropEvent(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_position0, Pointer)
{
    NODE_RETURN(qt_QDropEvent_position_QPointF_QDropEvent(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_possibleActions0, int)
{
    NODE_RETURN(qt_QDropEvent_possibleActions_int_QDropEvent(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_proposedAction0, int)
{
    NODE_RETURN(qt_QDropEvent_proposedAction_int_QDropEvent(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_setDropAction0, void)
{
    qt_QDropEvent_setDropAction_void_QDropEvent_int(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int));
}

static NODE_IMPLEMENTATION(_n_source0, Pointer)
{
    NODE_RETURN(qt_QDropEvent_source_QObject_QDropEvent(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}



void
QDropEventType::load()
{
    USING_MU_FUNCTION_SYMBOLS;
    MuLangContext* c = static_cast<MuLangContext*>(context());
    Module* global = globalModule();
    
    const string typeName        = name();
    const string refTypeName     = typeName + "&";
    const string fullTypeName    = fullyQualifiedName();
    const string fullRefTypeName = fullTypeName + "&";
    const char*  tn              = typeName.c_str();
    const char*  ftn             = fullTypeName.c_str();
    const char*  rtn             = refTypeName.c_str();
    const char*  frtn            = fullRefTypeName.c_str();

    scope()->addSymbols(new ReferenceType(c, rtn, this),

                        new Function(c, tn, BaseFunctions::dereference, Cast,
                                     Return, ftn,
                                     Args, frtn, End),

                        EndArguments);
    
    addSymbols(new Function(c, "__allocate", BaseFunctions::classAllocate, None,
                            Return, ftn,
                            End),

               new MemberVariable(c, "native", "qt.NativeObject"),

               EndArguments );


addSymbols(
    EndArguments);

addSymbols(
    // enums
    // member functions
    new Function(c, "QDropEvent", _n_QDropEvent0, None, Compiled, qt_QDropEvent_QDropEvent_QDropEvent_QDropEvent_QPointF_int_QMimeData_int_int_int, Return, "qt.QDropEvent", Parameters, new Param(c, "this", "qt.QDropEvent"), new Param(c, "pos", "qt.QPointF"), new Param(c, "actions", "int"), new Param(c, "data", "qt.QMimeData"), new Param(c, "buttons", "int"), new Param(c, "modifiers", "int"), new Param(c, "type", "int", Value((int)QEvent::Drop)), End),
    new Function(c, "acceptProposedAction", _n_acceptProposedAction0, None, Compiled, qt_QDropEvent_acceptProposedAction_void_QDropEvent, Return, "void", Parameters, new Param(c, "this", "qt.QDropEvent"), End),
    new Function(c, "buttons", _n_buttons0, None, Compiled, qt_QDropEvent_buttons_int_QDropEvent, Return, "int", Parameters, new Param(c, "this", "qt.QDropEvent"), End),
    new Function(c, "dropAction", _n_dropAction0, None, Compiled, qt_QDropEvent_dropAction_int_QDropEvent, Return, "int", Parameters, new Param(c, "this", "qt.QDropEvent"), End),
    new Function(c, "mimeData", _n_mimeData0, None, Compiled, qt_QDropEvent_mimeData_QMimeData_QDropEvent, Return, "qt.QMimeData", Parameters, new Param(c, "this", "qt.QDropEvent"), End),
    new Function(c, "modifiers", _n_modifiers0, None, Compiled, qt_QDropEvent_modifiers_int_QDropEvent, Return, "int", Parameters, new Param(c, "this", "qt.QDropEvent"), End),
    new Function(c, "position", _n_position0, None, Compiled, qt_QDropEvent_position_QPointF_QDropEvent, Return, "qt.QPointF", Parameters, new Param(c, "this", "qt.QDropEvent"), End),
    new Function(c, "possibleActions", _n_possibleActions0, None, Compiled, qt_QDropEvent_possibleActions_int_QDropEvent, Return, "int", Parameters, new Param(c, "this", "qt.QDropEvent"), End),
    new Function(c, "proposedAction", _n_proposedAction0, None, Compiled, qt_QDropEvent_proposedAction_int_QDropEvent, Return, "int", Parameters, new Param(c, "this", "qt.QDropEvent"), End),
    new Function(c, "setDropAction", _n_setDropAction0, None, Compiled, qt_QDropEvent_setDropAction_void_QDropEvent_int, Return, "void", Parameters, new Param(c, "this", "qt.QDropEvent"), new Param(c, "action", "int"), End),
    new Function(c, "source", _n_source0, None, Compiled, qt_QDropEvent_source_QObject_QDropEvent, Return, "qt.QObject", Parameters, new Param(c, "this", "qt.QDropEvent"), End),
    // static functions
    EndArguments);
globalScope()->addSymbols(
    EndArguments);
scope()->addSymbols(
    EndArguments);

}

} // Mu
