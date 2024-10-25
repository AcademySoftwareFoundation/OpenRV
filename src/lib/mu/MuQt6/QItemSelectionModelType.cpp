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
#include <MuQt6/QItemSelectionModelType.h>
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
#include <MuQt6/QObjectType.h>
#include <MuQt6/QEventType.h>
#include <MuQt6/QModelIndexType.h>
#include <MuQt6/QItemSelectionType.h>
#include <MuQt6/QTimerEventType.h>
#include <MuQt6/QAbstractItemModelType.h>

namespace Mu {
using namespace std;

//----------------------------------------------------------------------
//  INHERITABLE TYPE IMPLEMENTATION

// destructor
MuQt_QItemSelectionModel::~MuQt_QItemSelectionModel()
{
    if (_obj)
    {
        *_obj->data<Pointer>() = Pointer(0);
        _obj->releaseExternal();
    }
    _obj = 0;
    _env = 0;
    _baseType = 0;
}

MuQt_QItemSelectionModel::MuQt_QItemSelectionModel(Pointer muobj, const CallEnvironment* ce, QAbstractItemModel * model) 
 : QItemSelectionModel(model)
{
    _env = ce;
    _obj = reinterpret_cast<ClassInstance*>(muobj);
    _obj->retainExternal();
    MuLangContext* c = (MuLangContext*)_env->context();
    _baseType = c->findSymbolOfTypeByQualifiedName<QItemSelectionModelType>(c->internName("qt.QItemSelectionModel"));
}

MuQt_QItemSelectionModel::MuQt_QItemSelectionModel(Pointer muobj, const CallEnvironment* ce, QAbstractItemModel * model, QObject * parent) 
 : QItemSelectionModel(model, parent)
{
    _env = ce;
    _obj = reinterpret_cast<ClassInstance*>(muobj);
    _obj->retainExternal();
    MuLangContext* c = (MuLangContext*)_env->context();
    _baseType = c->findSymbolOfTypeByQualifiedName<QItemSelectionModelType>(c->internName("qt.QItemSelectionModel"));
}

bool MuQt_QItemSelectionModel::event(QEvent * e) 
{
    if (!_env) return QItemSelectionModel::event(e);
    MuLangContext* c = (MuLangContext*)_env->context();
    const MemberFunction* F0 = _baseType->_func[0];
    const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
    if (F != F0) 
    {
        Function::ArgumentVector args(2);
        args[0] = Value(Pointer(_obj));
        args[1] = Value(makeqpointer<QEventType>(c,e,"qt.QEvent"));
        Value rval = _env->call(F, args);
        return (bool)(rval._bool);
    }
    else
    {
        return QItemSelectionModel::event(e);
    }
}

bool MuQt_QItemSelectionModel::eventFilter(QObject * watched, QEvent * event) 
{
    if (!_env) return QItemSelectionModel::eventFilter(watched, event);
    MuLangContext* c = (MuLangContext*)_env->context();
    const MemberFunction* F0 = _baseType->_func[1];
    const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
    if (F != F0) 
    {
        Function::ArgumentVector args(3);
        args[0] = Value(Pointer(_obj));
        args[1] = Value(makeinstance<QObjectType>(c,watched,"qt.QObject"));
        args[2] = Value(makeqpointer<QEventType>(c,event,"qt.QEvent"));
        Value rval = _env->call(F, args);
        return (bool)(rval._bool);
    }
    else
    {
        return QItemSelectionModel::eventFilter(watched, event);
    }
}

void MuQt_QItemSelectionModel::customEvent(QEvent * event) 
{
    if (!_env) { QItemSelectionModel::customEvent(event); return; }
    MuLangContext* c = (MuLangContext*)_env->context();
    const MemberFunction* F0 = _baseType->_func[2];
    const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
    if (F != F0) 
    {
        Function::ArgumentVector args(2);
        args[0] = Value(Pointer(_obj));
        args[1] = Value(makeqpointer<QEventType>(c,event,"qt.QEvent"));
        Value rval = _env->call(F, args);
    }
    else
    {
        QItemSelectionModel::customEvent(event);
    }
}

void MuQt_QItemSelectionModel::timerEvent(QTimerEvent * event) 
{
    if (!_env) { QItemSelectionModel::timerEvent(event); return; }
    MuLangContext* c = (MuLangContext*)_env->context();
    const MemberFunction* F0 = _baseType->_func[3];
    const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
    if (F != F0) 
    {
        Function::ArgumentVector args(2);
        args[0] = Value(Pointer(_obj));
        args[1] = Value(makeqpointer<QTimerEventType>(c,event,"qt.QTimerEvent"));
        Value rval = _env->call(F, args);
    }
    else
    {
        QItemSelectionModel::timerEvent(event);
    }
}



//----------------------------------------------------------------------
//  Mu Type CONSTRUCTORS

QItemSelectionModelType::QItemSelectionModelType(Context* c, const char* name, Class* super, Class* super2)
: Class(c, name, vectorOf2(super, super2))
{
}

QItemSelectionModelType::~QItemSelectionModelType()
{
}

//----------------------------------------------------------------------
//  PRE-COMPILED FUNCTIONS

static Pointer
QItemSelectionModel_QItemSelectionModel_QObject(Thread& NODE_THREAD, Pointer obj)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    ClassInstance* widget = reinterpret_cast<ClassInstance*>(obj);

    if (!widget)
    {
        return 0;
    }
    else if (QItemSelectionModel* w = object<QItemSelectionModel>(widget))
    {
        QItemSelectionModelType* type = 
            c->findSymbolOfTypeByQualifiedName<QItemSelectionModelType>(c->internName("qt.QItemSelectionModel"), false);
        ClassInstance* o = ClassInstance::allocate(type);
        setobject(o, w);
        return o;
    }
    else
    {
        throw BadCastException();
    }
}

static NODE_IMPLEMENTATION(castFromObject, Pointer)
{
    NODE_RETURN( QItemSelectionModel_QItemSelectionModel_QObject(NODE_THREAD, NODE_ARG(0, Pointer)) );
}

Pointer qt_QItemSelectionModel_QItemSelectionModel_QItemSelectionModel_QItemSelectionModel_QAbstractItemModel(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_model)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QAbstractItemModel * arg1 = object<QAbstractItemModel>(param_model);
    setobject(param_this, new MuQt_QItemSelectionModel(param_this, NODE_THREAD.process()->callEnv(), arg1));
    return param_this;
}

Pointer qt_QItemSelectionModel_QItemSelectionModel_QItemSelectionModel_QItemSelectionModel_QAbstractItemModel_QObject(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_model, Pointer param_parent)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QAbstractItemModel * arg1 = object<QAbstractItemModel>(param_model);
    QObject * arg2 = object<QObject>(param_parent);
    setobject(param_this, new MuQt_QItemSelectionModel(param_this, NODE_THREAD.process()->callEnv(), arg1, arg2));
    return param_this;
}

bool qt_QItemSelectionModel_columnIntersectsSelection_bool_QItemSelectionModel_int_QModelIndex(Mu::Thread& NODE_THREAD, Pointer param_this, int param_column, Pointer param_parent)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QItemSelectionModel* arg0 = object<QItemSelectionModel>(param_this);
    int arg1 = (int)(param_column);
    const QModelIndex  arg2 = getqtype<QModelIndexType>(param_parent);
    return arg0->columnIntersectsSelection(arg1, arg2);
}

Pointer qt_QItemSelectionModel_currentIndex_QModelIndex_QItemSelectionModel(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QItemSelectionModel* arg0 = object<QItemSelectionModel>(param_this);
    return makeqtype<QModelIndexType>(c,arg0->currentIndex(),"qt.QModelIndex");
}

bool qt_QItemSelectionModel_hasSelection_bool_QItemSelectionModel(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QItemSelectionModel* arg0 = object<QItemSelectionModel>(param_this);
    return arg0->hasSelection();
}

bool qt_QItemSelectionModel_isColumnSelected_bool_QItemSelectionModel_int_QModelIndex(Mu::Thread& NODE_THREAD, Pointer param_this, int param_column, Pointer param_parent)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QItemSelectionModel* arg0 = object<QItemSelectionModel>(param_this);
    int arg1 = (int)(param_column);
    const QModelIndex  arg2 = getqtype<QModelIndexType>(param_parent);
    return arg0->isColumnSelected(arg1, arg2);
}

bool qt_QItemSelectionModel_isRowSelected_bool_QItemSelectionModel_int_QModelIndex(Mu::Thread& NODE_THREAD, Pointer param_this, int param_row, Pointer param_parent)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QItemSelectionModel* arg0 = object<QItemSelectionModel>(param_this);
    int arg1 = (int)(param_row);
    const QModelIndex  arg2 = getqtype<QModelIndexType>(param_parent);
    return arg0->isRowSelected(arg1, arg2);
}

bool qt_QItemSelectionModel_isSelected_bool_QItemSelectionModel_QModelIndex(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_index)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QItemSelectionModel* arg0 = object<QItemSelectionModel>(param_this);
    const QModelIndex  arg1 = getqtype<QModelIndexType>(param_index);
    return arg0->isSelected(arg1);
}

Pointer qt_QItemSelectionModel_model_QAbstractItemModel_QItemSelectionModel(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QItemSelectionModel* arg0 = object<QItemSelectionModel>(param_this);
    return makeinstance<QAbstractItemModelType>(c, arg0->model(), "qt.QAbstractItemModel");
}

bool qt_QItemSelectionModel_rowIntersectsSelection_bool_QItemSelectionModel_int_QModelIndex(Mu::Thread& NODE_THREAD, Pointer param_this, int param_row, Pointer param_parent)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QItemSelectionModel* arg0 = object<QItemSelectionModel>(param_this);
    int arg1 = (int)(param_row);
    const QModelIndex  arg2 = getqtype<QModelIndexType>(param_parent);
    return arg0->rowIntersectsSelection(arg1, arg2);
}

Pointer qt_QItemSelectionModel_selectedColumns_qt__QModelIndexBSB_ESB__QItemSelectionModel_int(Mu::Thread& NODE_THREAD, Pointer param_this, int param_row)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QItemSelectionModel* arg0 = object<QItemSelectionModel>(param_this);
    int arg1 = (int)(param_row);
    return makeqmodelindexlist(c,arg0->selectedColumns(arg1));
}

Pointer qt_QItemSelectionModel_selectedIndexes_qt__QModelIndexBSB_ESB__QItemSelectionModel(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QItemSelectionModel* arg0 = object<QItemSelectionModel>(param_this);
    return makeqmodelindexlist(c,arg0->selectedIndexes());
}

Pointer qt_QItemSelectionModel_selectedRows_qt__QModelIndexBSB_ESB__QItemSelectionModel_int(Mu::Thread& NODE_THREAD, Pointer param_this, int param_column)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QItemSelectionModel* arg0 = object<QItemSelectionModel>(param_this);
    int arg1 = (int)(param_column);
    return makeqmodelindexlist(c,arg0->selectedRows(arg1));
}

Pointer qt_QItemSelectionModel_selection_QItemSelection_QItemSelectionModel(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QItemSelectionModel* arg0 = object<QItemSelectionModel>(param_this);
    return makeqtype<QItemSelectionType>(c,arg0->selection(),"qt.QItemSelection");
}

void qt_QItemSelectionModel_setModel_void_QItemSelectionModel_QAbstractItemModel(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_model)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QItemSelectionModel* arg0 = object<QItemSelectionModel>(param_this);
    QAbstractItemModel * arg1 = object<QAbstractItemModel>(param_model);
    arg0->setModel(arg1);
}

void qt_QItemSelectionModel_emitSelectionChanged_void_QItemSelectionModel_QItemSelection_QItemSelection(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_newSelection, Pointer param_oldSelection)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QItemSelectionModel* arg0 = object<QItemSelectionModel>(param_this);
    const QItemSelection  arg1 = getqtype<QItemSelectionType>(param_newSelection);
    const QItemSelection  arg2 = getqtype<QItemSelectionType>(param_oldSelection);
    ((MuQt_QItemSelectionModel*)arg0)->emitSelectionChanged_pub(arg1, arg2);
}

bool qt_QItemSelectionModel_event_bool_QItemSelectionModel_QEvent(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_e)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QItemSelectionModel* arg0 = object<QItemSelectionModel>(param_this);
    QEvent * arg1 = getqpointer<QEventType>(param_e);
    return isMuQtObject(arg0) ? arg0->QItemSelectionModel::event(arg1) : arg0->event(arg1);
}

bool qt_QItemSelectionModel_eventFilter_bool_QItemSelectionModel_QObject_QEvent(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_watched, Pointer param_event)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QItemSelectionModel* arg0 = object<QItemSelectionModel>(param_this);
    QObject * arg1 = object<QObject>(param_watched);
    QEvent * arg2 = getqpointer<QEventType>(param_event);
    return isMuQtObject(arg0) ? arg0->QItemSelectionModel::eventFilter(arg1, arg2) : arg0->eventFilter(arg1, arg2);
}

void qt_QItemSelectionModel_customEvent_void_QItemSelectionModel_QEvent(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_event)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QItemSelectionModel* arg0 = object<QItemSelectionModel>(param_this);
    QEvent * arg1 = getqpointer<QEventType>(param_event);
    if (isMuQtObject(arg0)) ((MuQt_QItemSelectionModel*)arg0)->customEvent_pub_parent(arg1);
    else ((MuQt_QItemSelectionModel*)arg0)->customEvent_pub(arg1);
}

void qt_QItemSelectionModel_timerEvent_void_QItemSelectionModel_QTimerEvent(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_event)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QItemSelectionModel* arg0 = object<QItemSelectionModel>(param_this);
    QTimerEvent * arg1 = getqpointer<QTimerEventType>(param_event);
    if (isMuQtObject(arg0)) ((MuQt_QItemSelectionModel*)arg0)->timerEvent_pub_parent(arg1);
    else ((MuQt_QItemSelectionModel*)arg0)->timerEvent_pub(arg1);
}


static NODE_IMPLEMENTATION(_n_QItemSelectionModel0, Pointer)
{
    NODE_RETURN(qt_QItemSelectionModel_QItemSelectionModel_QItemSelectionModel_QItemSelectionModel_QAbstractItemModel(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}

static NODE_IMPLEMENTATION(_n_QItemSelectionModel1, Pointer)
{
    NODE_RETURN(qt_QItemSelectionModel_QItemSelectionModel_QItemSelectionModel_QItemSelectionModel_QAbstractItemModel_QObject(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer), NODE_ARG(2, Pointer)));
}

static NODE_IMPLEMENTATION(_n_columnIntersectsSelection0, bool)
{
    NODE_RETURN(qt_QItemSelectionModel_columnIntersectsSelection_bool_QItemSelectionModel_int_QModelIndex(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int), NODE_ARG(2, Pointer)));
}

static NODE_IMPLEMENTATION(_n_currentIndex0, Pointer)
{
    NODE_RETURN(qt_QItemSelectionModel_currentIndex_QModelIndex_QItemSelectionModel(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_hasSelection0, bool)
{
    NODE_RETURN(qt_QItemSelectionModel_hasSelection_bool_QItemSelectionModel(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_isColumnSelected0, bool)
{
    NODE_RETURN(qt_QItemSelectionModel_isColumnSelected_bool_QItemSelectionModel_int_QModelIndex(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int), NODE_ARG(2, Pointer)));
}

static NODE_IMPLEMENTATION(_n_isRowSelected0, bool)
{
    NODE_RETURN(qt_QItemSelectionModel_isRowSelected_bool_QItemSelectionModel_int_QModelIndex(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int), NODE_ARG(2, Pointer)));
}

static NODE_IMPLEMENTATION(_n_isSelected0, bool)
{
    NODE_RETURN(qt_QItemSelectionModel_isSelected_bool_QItemSelectionModel_QModelIndex(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}

static NODE_IMPLEMENTATION(_n_model0, Pointer)
{
    NODE_RETURN(qt_QItemSelectionModel_model_QAbstractItemModel_QItemSelectionModel(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_rowIntersectsSelection0, bool)
{
    NODE_RETURN(qt_QItemSelectionModel_rowIntersectsSelection_bool_QItemSelectionModel_int_QModelIndex(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int), NODE_ARG(2, Pointer)));
}

static NODE_IMPLEMENTATION(_n_selectedColumns0, Pointer)
{
    NODE_RETURN(qt_QItemSelectionModel_selectedColumns_qt__QModelIndexBSB_ESB__QItemSelectionModel_int(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int)));
}

static NODE_IMPLEMENTATION(_n_selectedIndexes0, Pointer)
{
    NODE_RETURN(qt_QItemSelectionModel_selectedIndexes_qt__QModelIndexBSB_ESB__QItemSelectionModel(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_selectedRows0, Pointer)
{
    NODE_RETURN(qt_QItemSelectionModel_selectedRows_qt__QModelIndexBSB_ESB__QItemSelectionModel_int(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int)));
}

static NODE_IMPLEMENTATION(_n_selection0, Pointer)
{
    NODE_RETURN(qt_QItemSelectionModel_selection_QItemSelection_QItemSelectionModel(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_setModel0, void)
{
    qt_QItemSelectionModel_setModel_void_QItemSelectionModel_QAbstractItemModel(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
}

static NODE_IMPLEMENTATION(_n_emitSelectionChanged0, void)
{
    qt_QItemSelectionModel_emitSelectionChanged_void_QItemSelectionModel_QItemSelection_QItemSelection(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer), NODE_ARG(2, Pointer));
}

static NODE_IMPLEMENTATION(_n_event0, bool)
{
    NODE_RETURN(qt_QItemSelectionModel_event_bool_QItemSelectionModel_QEvent(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}

static NODE_IMPLEMENTATION(_n_eventFilter0, bool)
{
    NODE_RETURN(qt_QItemSelectionModel_eventFilter_bool_QItemSelectionModel_QObject_QEvent(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer), NODE_ARG(2, Pointer)));
}

static NODE_IMPLEMENTATION(_n_customEvent0, void)
{
    qt_QItemSelectionModel_customEvent_void_QItemSelectionModel_QEvent(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
}

static NODE_IMPLEMENTATION(_n_timerEvent0, void)
{
    qt_QItemSelectionModel_timerEvent_void_QItemSelectionModel_QTimerEvent(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
}



void
QItemSelectionModelType::load()
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


               new Function(c, tn, castFromObject, Cast,
                            Compiled, QItemSelectionModel_QItemSelectionModel_QObject,
                            Return, ftn,
                            Parameters,
                            new Param(c, "object", "qt.QObject"),
                            End),

               EndArguments );

addSymbols(
    // enums
    // member functions
    new Function(c, "QItemSelectionModel", _n_QItemSelectionModel0, None, Compiled, qt_QItemSelectionModel_QItemSelectionModel_QItemSelectionModel_QItemSelectionModel_QAbstractItemModel, Return, "qt.QItemSelectionModel", Parameters, new Param(c, "this", "qt.QItemSelectionModel"), new Param(c, "model", "qt.QAbstractItemModel"), End),
    new Function(c, "QItemSelectionModel", _n_QItemSelectionModel1, None, Compiled, qt_QItemSelectionModel_QItemSelectionModel_QItemSelectionModel_QItemSelectionModel_QAbstractItemModel_QObject, Return, "qt.QItemSelectionModel", Parameters, new Param(c, "this", "qt.QItemSelectionModel"), new Param(c, "model", "qt.QAbstractItemModel"), new Param(c, "parent", "qt.QObject"), End),
    new Function(c, "columnIntersectsSelection", _n_columnIntersectsSelection0, None, Compiled, qt_QItemSelectionModel_columnIntersectsSelection_bool_QItemSelectionModel_int_QModelIndex, Return, "bool", Parameters, new Param(c, "this", "qt.QItemSelectionModel"), new Param(c, "column", "int"), new Param(c, "parent", "qt.QModelIndex"), End),
    new Function(c, "currentIndex", _n_currentIndex0, None, Compiled, qt_QItemSelectionModel_currentIndex_QModelIndex_QItemSelectionModel, Return, "qt.QModelIndex", Parameters, new Param(c, "this", "qt.QItemSelectionModel"), End),
    new Function(c, "hasSelection", _n_hasSelection0, None, Compiled, qt_QItemSelectionModel_hasSelection_bool_QItemSelectionModel, Return, "bool", Parameters, new Param(c, "this", "qt.QItemSelectionModel"), End),
    new Function(c, "isColumnSelected", _n_isColumnSelected0, None, Compiled, qt_QItemSelectionModel_isColumnSelected_bool_QItemSelectionModel_int_QModelIndex, Return, "bool", Parameters, new Param(c, "this", "qt.QItemSelectionModel"), new Param(c, "column", "int"), new Param(c, "parent", "qt.QModelIndex"), End),
    new Function(c, "isRowSelected", _n_isRowSelected0, None, Compiled, qt_QItemSelectionModel_isRowSelected_bool_QItemSelectionModel_int_QModelIndex, Return, "bool", Parameters, new Param(c, "this", "qt.QItemSelectionModel"), new Param(c, "row", "int"), new Param(c, "parent", "qt.QModelIndex"), End),
    new Function(c, "isSelected", _n_isSelected0, None, Compiled, qt_QItemSelectionModel_isSelected_bool_QItemSelectionModel_QModelIndex, Return, "bool", Parameters, new Param(c, "this", "qt.QItemSelectionModel"), new Param(c, "index", "qt.QModelIndex"), End),
    new Function(c, "model", _n_model0, None, Compiled, qt_QItemSelectionModel_model_QAbstractItemModel_QItemSelectionModel, Return, "qt.QAbstractItemModel", Parameters, new Param(c, "this", "qt.QItemSelectionModel"), End),
    // MISSING: model (QAbstractItemModel; QItemSelectionModel this)
    new Function(c, "rowIntersectsSelection", _n_rowIntersectsSelection0, None, Compiled, qt_QItemSelectionModel_rowIntersectsSelection_bool_QItemSelectionModel_int_QModelIndex, Return, "bool", Parameters, new Param(c, "this", "qt.QItemSelectionModel"), new Param(c, "row", "int"), new Param(c, "parent", "qt.QModelIndex"), End),
    new Function(c, "selectedColumns", _n_selectedColumns0, None, Compiled, qt_QItemSelectionModel_selectedColumns_qt__QModelIndexBSB_ESB__QItemSelectionModel_int, Return, "qt.QModelIndex[]", Parameters, new Param(c, "this", "qt.QItemSelectionModel"), new Param(c, "row", "int", Value((int)0)), End),
    new Function(c, "selectedIndexes", _n_selectedIndexes0, None, Compiled, qt_QItemSelectionModel_selectedIndexes_qt__QModelIndexBSB_ESB__QItemSelectionModel, Return, "qt.QModelIndex[]", Parameters, new Param(c, "this", "qt.QItemSelectionModel"), End),
    new Function(c, "selectedRows", _n_selectedRows0, None, Compiled, qt_QItemSelectionModel_selectedRows_qt__QModelIndexBSB_ESB__QItemSelectionModel_int, Return, "qt.QModelIndex[]", Parameters, new Param(c, "this", "qt.QItemSelectionModel"), new Param(c, "column", "int", Value((int)0)), End),
    new Function(c, "selection", _n_selection0, None, Compiled, qt_QItemSelectionModel_selection_QItemSelection_QItemSelectionModel, Return, "qt.QItemSelection", Parameters, new Param(c, "this", "qt.QItemSelectionModel"), End),
    new Function(c, "setModel", _n_setModel0, None, Compiled, qt_QItemSelectionModel_setModel_void_QItemSelectionModel_QAbstractItemModel, Return, "void", Parameters, new Param(c, "this", "qt.QItemSelectionModel"), new Param(c, "model", "qt.QAbstractItemModel"), End),
    new Function(c, "emitSelectionChanged", _n_emitSelectionChanged0, None, Compiled, qt_QItemSelectionModel_emitSelectionChanged_void_QItemSelectionModel_QItemSelection_QItemSelection, Return, "void", Parameters, new Param(c, "this", "qt.QItemSelectionModel"), new Param(c, "newSelection", "qt.QItemSelection"), new Param(c, "oldSelection", "qt.QItemSelection"), End),
    _func[0] = new MemberFunction(c, "event", _n_event0, None, Compiled, qt_QItemSelectionModel_event_bool_QItemSelectionModel_QEvent, Return, "bool", Parameters, new Param(c, "this", "qt.QItemSelectionModel"), new Param(c, "e", "qt.QEvent"), End),
    _func[1] = new MemberFunction(c, "eventFilter", _n_eventFilter0, None, Compiled, qt_QItemSelectionModel_eventFilter_bool_QItemSelectionModel_QObject_QEvent, Return, "bool", Parameters, new Param(c, "this", "qt.QItemSelectionModel"), new Param(c, "watched", "qt.QObject"), new Param(c, "event", "qt.QEvent"), End),
    // MISSING: metaObject ("const QMetaObject *"; QItemSelectionModel this)
    // MISSING: childEvent (void; QItemSelectionModel this, "QChildEvent *" event) // protected
    // MISSING: connectNotify (void; QItemSelectionModel this, "const QMetaMethod &" signal) // protected
    _func[2] = new MemberFunction(c, "customEvent", _n_customEvent0, None, Compiled, qt_QItemSelectionModel_customEvent_void_QItemSelectionModel_QEvent, Return, "void", Parameters, new Param(c, "this", "qt.QItemSelectionModel"), new Param(c, "event", "qt.QEvent"), End),
    // MISSING: disconnectNotify (void; QItemSelectionModel this, "const QMetaMethod &" signal) // protected
    _func[3] = new MemberFunction(c, "timerEvent", _n_timerEvent0, None, Compiled, qt_QItemSelectionModel_timerEvent_void_QItemSelectionModel_QTimerEvent, Return, "void", Parameters, new Param(c, "this", "qt.QItemSelectionModel"), new Param(c, "event", "qt.QTimerEvent"), End),
    // static functions
    EndArguments);
globalScope()->addSymbols(
    EndArguments);
scope()->addSymbols(
    EndArguments);


    const char** propExclusions = 0;

    populate(this, QItemSelectionModel::staticMetaObject, propExclusions);
}

} // Mu
