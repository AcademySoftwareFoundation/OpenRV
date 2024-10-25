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
#include <MuQt6/QTextOptionType.h>
#include <MuQt6/QActionType.h>
#include <MuQt6/QWidgetType.h>
#include <Mu/Alias.h>
#include <Mu/BaseFunctions.h>
#include <Mu/ClassInstance.h>
#include <Mu/Exception.h>
#include <Mu/Function.h>
#include <Mu/MemberFunction.h>
#include <Mu/MemberVariable.h>
#include <Mu/Node.h>
#include <Mu/ParameterVariable.h>
#include <Mu/ReferenceType.h>
#include <Mu/SymbolicConstant.h>
#include <Mu/Thread.h>
#include <Mu/Value.h>
#include <MuLang/MuLangContext.h>
#include <MuLang/StringType.h>
#include <QtGui/QtGui>
#include <QtWidgets/QtWidgets>
#include <QtSvg/QtSvg>
#include <QSvgWidget>
#include <QtNetwork/QtNetwork>

namespace Mu {
using namespace std;

QTextOptionType::Instance::Instance(const Class* c) : ClassInstance(c)
{
}

QTextOptionType::QTextOptionType(Context* c, const char* name, Class* super)
    : Class(c, name, super)
{
}

QTextOptionType::~QTextOptionType()
{
}

static NODE_IMPLEMENTATION(__allocate, Pointer)
{
    QTextOptionType::Instance* i = new QTextOptionType::Instance((Class*)NODE_THIS.type());
    QTextOptionType::registerFinalizer(i);
    NODE_RETURN(i);
}

void 
QTextOptionType::registerFinalizer (void* o)
{
    GC_register_finalizer(o, QTextOptionType::finalizer, 0, 0, 0);
}

void 
QTextOptionType::finalizer (void* obj, void* data)
{
    QTextOptionType::Instance* i = reinterpret_cast<QTextOptionType::Instance*>(obj);
    delete i;
}

//----------------------------------------------------------------------
//  PRE-COMPILED FUNCTIONS

Pointer qt_QTextOption_QTextOption_QTextOption_QTextOption(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    setqtype<QTextOptionType>(param_this,QTextOption());
    return param_this;
}

Pointer qt_QTextOption_QTextOption_QTextOption_QTextOption_int(Mu::Thread& NODE_THREAD, Pointer param_this, int param_alignment)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    Qt::Alignment arg1 = (Qt::Alignment)(param_alignment);
    setqtype<QTextOptionType>(param_this,QTextOption(arg1));
    return param_this;
}

int qt_QTextOption_alignment_int_QTextOption(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QTextOption arg0 = getqtype<QTextOptionType>(param_this);
    return int(arg0.alignment());
}

int qt_QTextOption_flags_int_QTextOption(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QTextOption arg0 = getqtype<QTextOptionType>(param_this);
    return int(arg0.flags());
}

void qt_QTextOption_setAlignment_void_QTextOption_int(Mu::Thread& NODE_THREAD, Pointer param_this, int param_alignment)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QTextOption arg0 = getqtype<QTextOptionType>(param_this);
    Qt::Alignment arg1 = (Qt::Alignment)(param_alignment);
    arg0.setAlignment(arg1);
    setqtype<QTextOptionType>(param_this,arg0);
}

void qt_QTextOption_setFlags_void_QTextOption_int(Mu::Thread& NODE_THREAD, Pointer param_this, int param_flags)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QTextOption arg0 = getqtype<QTextOptionType>(param_this);
    QTextOption::Flags arg1 = (QTextOption::Flags)(param_flags);
    arg0.setFlags(arg1);
    setqtype<QTextOptionType>(param_this,arg0);
}

void qt_QTextOption_setTabStopDistance_void_QTextOption_double(Mu::Thread& NODE_THREAD, Pointer param_this, double param_tabStopDistance)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QTextOption arg0 = getqtype<QTextOptionType>(param_this);
    qreal arg1 = (double)(param_tabStopDistance);
    arg0.setTabStopDistance(arg1);
    setqtype<QTextOptionType>(param_this,arg0);
}

void qt_QTextOption_setTextDirection_void_QTextOption_int(Mu::Thread& NODE_THREAD, Pointer param_this, int param_direction)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QTextOption arg0 = getqtype<QTextOptionType>(param_this);
    Qt::LayoutDirection arg1 = (Qt::LayoutDirection)(param_direction);
    arg0.setTextDirection(arg1);
    setqtype<QTextOptionType>(param_this,arg0);
}

void qt_QTextOption_setUseDesignMetrics_void_QTextOption_bool(Mu::Thread& NODE_THREAD, Pointer param_this, bool param_enable)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QTextOption arg0 = getqtype<QTextOptionType>(param_this);
    bool arg1 = (bool)(param_enable);
    arg0.setUseDesignMetrics(arg1);
    setqtype<QTextOptionType>(param_this,arg0);
}

void qt_QTextOption_setWrapMode_void_QTextOption_int(Mu::Thread& NODE_THREAD, Pointer param_this, int param_mode)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QTextOption arg0 = getqtype<QTextOptionType>(param_this);
    QTextOption::WrapMode arg1 = (QTextOption::WrapMode)(param_mode);
    arg0.setWrapMode(arg1);
    setqtype<QTextOptionType>(param_this,arg0);
}

double qt_QTextOption_tabStopDistance_double_QTextOption(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QTextOption arg0 = getqtype<QTextOptionType>(param_this);
    return arg0.tabStopDistance();
}

int qt_QTextOption_textDirection_int_QTextOption(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QTextOption arg0 = getqtype<QTextOptionType>(param_this);
    return int(arg0.textDirection());
}

bool qt_QTextOption_useDesignMetrics_bool_QTextOption(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QTextOption arg0 = getqtype<QTextOptionType>(param_this);
    return arg0.useDesignMetrics();
}

int qt_QTextOption_wrapMode_int_QTextOption(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QTextOption arg0 = getqtype<QTextOptionType>(param_this);
    return int(arg0.wrapMode());
}


static NODE_IMPLEMENTATION(_n_QTextOption0, Pointer)
{
    NODE_RETURN(qt_QTextOption_QTextOption_QTextOption_QTextOption(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_QTextOption1, Pointer)
{
    NODE_RETURN(qt_QTextOption_QTextOption_QTextOption_QTextOption_int(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int)));
}

static NODE_IMPLEMENTATION(_n_alignment0, int)
{
    NODE_RETURN(qt_QTextOption_alignment_int_QTextOption(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_flags0, int)
{
    NODE_RETURN(qt_QTextOption_flags_int_QTextOption(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_setAlignment0, void)
{
    qt_QTextOption_setAlignment_void_QTextOption_int(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int));
}

static NODE_IMPLEMENTATION(_n_setFlags0, void)
{
    qt_QTextOption_setFlags_void_QTextOption_int(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int));
}

static NODE_IMPLEMENTATION(_n_setTabStopDistance0, void)
{
    qt_QTextOption_setTabStopDistance_void_QTextOption_double(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, double));
}

static NODE_IMPLEMENTATION(_n_setTextDirection0, void)
{
    qt_QTextOption_setTextDirection_void_QTextOption_int(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int));
}

static NODE_IMPLEMENTATION(_n_setUseDesignMetrics0, void)
{
    qt_QTextOption_setUseDesignMetrics_void_QTextOption_bool(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, bool));
}

static NODE_IMPLEMENTATION(_n_setWrapMode0, void)
{
    qt_QTextOption_setWrapMode_void_QTextOption_int(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int));
}

static NODE_IMPLEMENTATION(_n_tabStopDistance0, double)
{
    NODE_RETURN(qt_QTextOption_tabStopDistance_double_QTextOption(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_textDirection0, int)
{
    NODE_RETURN(qt_QTextOption_textDirection_int_QTextOption(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_useDesignMetrics0, bool)
{
    NODE_RETURN(qt_QTextOption_useDesignMetrics_bool_QTextOption(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_wrapMode0, int)
{
    NODE_RETURN(qt_QTextOption_wrapMode_int_QTextOption(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}



void
QTextOptionType::load()
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
    
    addSymbols(new Function(c, "__allocate", __allocate, None,
                            Return, ftn,
                            End),

               EndArguments );

addSymbols(
    new Alias(c, "Flag", "int"),
    new Alias(c, "Flags", "int"),
      new SymbolicConstant(c, "IncludeTrailingSpaces", "int", Value(int(QTextOption::IncludeTrailingSpaces))),
      new SymbolicConstant(c, "ShowTabsAndSpaces", "int", Value(int(QTextOption::ShowTabsAndSpaces))),
      new SymbolicConstant(c, "ShowLineAndParagraphSeparators", "int", Value(int(QTextOption::ShowLineAndParagraphSeparators))),
      new SymbolicConstant(c, "ShowDocumentTerminator", "int", Value(int(QTextOption::ShowDocumentTerminator))),
      new SymbolicConstant(c, "AddSpaceForLineAndParagraphSeparators", "int", Value(int(QTextOption::AddSpaceForLineAndParagraphSeparators))),
      new SymbolicConstant(c, "SuppressColors", "int", Value(int(QTextOption::SuppressColors))),
    new Alias(c, "TabType", "int"),
      new SymbolicConstant(c, "LeftTab", "int", Value(int(QTextOption::LeftTab))),
      new SymbolicConstant(c, "RightTab", "int", Value(int(QTextOption::RightTab))),
      new SymbolicConstant(c, "CenterTab", "int", Value(int(QTextOption::CenterTab))),
      new SymbolicConstant(c, "DelimiterTab", "int", Value(int(QTextOption::DelimiterTab))),
    new Alias(c, "WrapMode", "int"),
      new SymbolicConstant(c, "NoWrap", "int", Value(int(QTextOption::NoWrap))),
      new SymbolicConstant(c, "WordWrap", "int", Value(int(QTextOption::WordWrap))),
      new SymbolicConstant(c, "ManualWrap", "int", Value(int(QTextOption::ManualWrap))),
      new SymbolicConstant(c, "WrapAnywhere", "int", Value(int(QTextOption::WrapAnywhere))),
      new SymbolicConstant(c, "WrapAtWordBoundaryOrAnywhere", "int", Value(int(QTextOption::WrapAtWordBoundaryOrAnywhere))),
    EndArguments);

addSymbols(
    // enums
    // member functions
    new Function(c, "QTextOption", _n_QTextOption0, None, Compiled, qt_QTextOption_QTextOption_QTextOption_QTextOption, Return, "qt.QTextOption", Parameters, new Param(c, "this", "qt.QTextOption"), End),
    new Function(c, "QTextOption", _n_QTextOption1, None, Compiled, qt_QTextOption_QTextOption_QTextOption_QTextOption_int, Return, "qt.QTextOption", Parameters, new Param(c, "this", "qt.QTextOption"), new Param(c, "alignment", "int"), End),
    // MISSING: QTextOption (QTextOption; QTextOption this, QTextOption other)
    new Function(c, "alignment", _n_alignment0, None, Compiled, qt_QTextOption_alignment_int_QTextOption, Return, "int", Parameters, new Param(c, "this", "qt.QTextOption"), End),
    new Function(c, "flags", _n_flags0, None, Compiled, qt_QTextOption_flags_int_QTextOption, Return, "int", Parameters, new Param(c, "this", "qt.QTextOption"), End),
    new Function(c, "setAlignment", _n_setAlignment0, None, Compiled, qt_QTextOption_setAlignment_void_QTextOption_int, Return, "void", Parameters, new Param(c, "this", "qt.QTextOption"), new Param(c, "alignment", "int"), End),
    new Function(c, "setFlags", _n_setFlags0, None, Compiled, qt_QTextOption_setFlags_void_QTextOption_int, Return, "void", Parameters, new Param(c, "this", "qt.QTextOption"), new Param(c, "flags", "int"), End),
    // MISSING: setTabArray (void; QTextOption this, "const QList<qreal> &" tabStops)
    new Function(c, "setTabStopDistance", _n_setTabStopDistance0, None, Compiled, qt_QTextOption_setTabStopDistance_void_QTextOption_double, Return, "void", Parameters, new Param(c, "this", "qt.QTextOption"), new Param(c, "tabStopDistance", "double"), End),
    new Function(c, "setTextDirection", _n_setTextDirection0, None, Compiled, qt_QTextOption_setTextDirection_void_QTextOption_int, Return, "void", Parameters, new Param(c, "this", "qt.QTextOption"), new Param(c, "direction", "int"), End),
    new Function(c, "setUseDesignMetrics", _n_setUseDesignMetrics0, None, Compiled, qt_QTextOption_setUseDesignMetrics_void_QTextOption_bool, Return, "void", Parameters, new Param(c, "this", "qt.QTextOption"), new Param(c, "enable", "bool"), End),
    new Function(c, "setWrapMode", _n_setWrapMode0, None, Compiled, qt_QTextOption_setWrapMode_void_QTextOption_int, Return, "void", Parameters, new Param(c, "this", "qt.QTextOption"), new Param(c, "mode", "int"), End),
    // MISSING: tabArray ("QList<qreal>"; QTextOption this)
    new Function(c, "tabStopDistance", _n_tabStopDistance0, None, Compiled, qt_QTextOption_tabStopDistance_double_QTextOption, Return, "double", Parameters, new Param(c, "this", "qt.QTextOption"), End),
    new Function(c, "textDirection", _n_textDirection0, None, Compiled, qt_QTextOption_textDirection_int_QTextOption, Return, "int", Parameters, new Param(c, "this", "qt.QTextOption"), End),
    new Function(c, "useDesignMetrics", _n_useDesignMetrics0, None, Compiled, qt_QTextOption_useDesignMetrics_bool_QTextOption, Return, "bool", Parameters, new Param(c, "this", "qt.QTextOption"), End),
    new Function(c, "wrapMode", _n_wrapMode0, None, Compiled, qt_QTextOption_wrapMode_int_QTextOption, Return, "int", Parameters, new Param(c, "this", "qt.QTextOption"), End),
    // static functions
    EndArguments);
globalScope()->addSymbols(
    // MISSING: = (QTextOption; QTextOption this, QTextOption other)
    EndArguments);
scope()->addSymbols(
    EndArguments);

}

} // Mu
