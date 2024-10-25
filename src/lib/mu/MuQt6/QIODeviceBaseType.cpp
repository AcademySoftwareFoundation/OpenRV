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
#include <MuQt6/QIODeviceBaseType.h>
#include <QtGui/QtGui>
#include <QtWidgets/QtWidgets>
#include <QtSvg/QtSvg>
#include <QSvgWidget>
#include <QtNetwork/QtNetwork>
#include <MuQt6/QWidgetType.h>
#include <MuQt6/QActionType.h>
#include <MuQt6/QIconType.h>
#include <Mu/BaseFunctions.h>
#include <Mu/Thread.h>
#include <Mu/Alias.h>
#include <Mu/SymbolicConstant.h>
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

//----------------------------------------------------------------------
//  INHERITABLE TYPE IMPLEMENTATION POTATO



//----------------------------------------------------------------------
//  Mu Type CONSTRUCTORS

QIODeviceBaseType::QIODeviceBaseType(Context* c, const char* name, Class* super)
    : Class(c, name, super)
{
}

QIODeviceBaseType::~QIODeviceBaseType()
{
}

//----------------------------------------------------------------------
//  PRE-COMPILED FUNCTIONS

// static Pointer
// QIODeviceBase_QIODeviceBase_QIODeviceBase(Thread& NODE_THREAD, Pointer obj)
// {
//     MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
//     ClassInstance* item = reinterpret_cast<ClassInstance*>(obj);

//     if (!item)
//     {
//         return 0;
//     }
//     else if (QIODeviceBase* i = iodevicebase<QIODeviceBase>(item))
//     {
//         QIODeviceBaseType* type = 
//             c->findSymbolOfTypeByQualifiedName<QIODeviceBaseType>(c->internName("qt.QIODeviceBase"), false);
//         ClassInstance* o = ClassInstance::allocate(type);
//         setiodevicebase(o, i);
//         return o;
//     }
//     else
//     {
//         throw BadCastException();
//     }
// }

// static NODE_IMPLEMENTATION(castFromLayoutItem, Pointer)
// {
//     NODE_RETURN( QIODeviceBase_QIODeviceBase_QIODeviceBase(NODE_THREAD, NODE_ARG(0, Pointer)) );
// }




void
QIODeviceBaseType::load()
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


               EndArguments );

addSymbols(
    // enums
    new Alias(c, "OpenModeFlag", "int"),
    new Alias(c, "OpenMode", "int"),
      new SymbolicConstant(c, "NotOpen", "int", Value(int(QIODeviceBase::NotOpen))),
      new SymbolicConstant(c, "ReadOnly", "int", Value(int(QIODeviceBase::ReadOnly))),
      new SymbolicConstant(c, "WriteOnly", "int", Value(int(QIODeviceBase::WriteOnly))),
      new SymbolicConstant(c, "ReadWrite", "int", Value(int(QIODeviceBase::ReadWrite))),
      new SymbolicConstant(c, "Append", "int", Value(int(QIODeviceBase::Append))),
      new SymbolicConstant(c, "Truncate", "int", Value(int(QIODeviceBase::Truncate))),
      new SymbolicConstant(c, "Text", "int", Value(int(QIODeviceBase::Text))),
      new SymbolicConstant(c, "Unbuffered", "int", Value(int(QIODeviceBase::Unbuffered))),
      new SymbolicConstant(c, "NewOnly", "int", Value(int(QIODeviceBase::NewOnly))),
      new SymbolicConstant(c, "ExistingOnly", "int", Value(int(QIODeviceBase::ExistingOnly))),
    // member functions
    // static functions
    EndArguments);
globalScope()->addSymbols(
    EndArguments);
scope()->addSymbols(
    EndArguments);

}

} // Mu
