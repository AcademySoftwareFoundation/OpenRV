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
#include <MuQt6/QInputEventType.h>
#include <QtGui/QtGui>
#include <QtWidgets/QtWidgets>
#include <QtSvg/QtSvg>
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

namespace Mu {
using namespace std;

QInputEventType::QInputEventType(Context* c, const char* name, Class* super)
    : Class(c, name, super)
{
}

QInputEventType::~QInputEventType()
{
}

//----------------------------------------------------------------------
//  PRE-COMPILED FUNCTIONS

int qt_QInputEvent_deviceType_int_QInputEvent(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QInputEvent * arg0 = getqpointer<QInputEventType>(param_this);
    return int(arg0->deviceType());
}

int qt_QInputEvent_modifiers_int_QInputEvent(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QInputEvent * arg0 = getqpointer<QInputEventType>(param_this);
    return int(arg0->modifiers());
}


static NODE_IMPLEMENTATION(_n_deviceType0, int)
{
    NODE_RETURN(qt_QInputEvent_deviceType_int_QInputEvent(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_modifiers0, int)
{
    NODE_RETURN(qt_QInputEvent_modifiers_int_QInputEvent(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}



void
QInputEventType::load()
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
    // MISSING: device ("const QInputDevice *"; QInputEvent this)
    new Function(c, "deviceType", _n_deviceType0, None, Compiled, qt_QInputEvent_deviceType_int_QInputEvent, Return, "int", Parameters, new Param(c, "this", "qt.QInputEvent"), End),
    new Function(c, "modifiers", _n_modifiers0, None, Compiled, qt_QInputEvent_modifiers_int_QInputEvent, Return, "int", Parameters, new Param(c, "this", "qt.QInputEvent"), End),
    // MISSING: timestamp ("quint64"; QInputEvent this)
    // static functions
    EndArguments);
globalScope()->addSymbols(
    EndArguments);
scope()->addSymbols(
    EndArguments);

}

} // Mu
