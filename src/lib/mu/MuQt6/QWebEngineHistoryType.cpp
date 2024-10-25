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
#include <MuQt6/QWebEngineHistoryType.h>
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
#include <MuQt6/QTimerEventType.h>
#include <MuQt6/QObjectType.h>
#include <MuQt6/QEventType.h>

namespace Mu {
using namespace std;

//----------------------------------------------------------------------
//  INHERITABLE TYPE IMPLEMENTATION

bool MuQt_QWebEngineHistory::event(QEvent * e) 
{
    if (!_env) return QWebEngineHistory::event(e);
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
        return QWebEngineHistory::event(e);
    }
}

bool MuQt_QWebEngineHistory::eventFilter(QObject * watched, QEvent * event) 
{
    if (!_env) return QWebEngineHistory::eventFilter(watched, event);
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
        return QWebEngineHistory::eventFilter(watched, event);
    }
}

void MuQt_QWebEngineHistory::customEvent(QEvent * event) 
{
    if (!_env) { QWebEngineHistory::customEvent(event); return; }
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
        QWebEngineHistory::customEvent(event);
    }
}

void MuQt_QWebEngineHistory::timerEvent(QTimerEvent * event) 
{
    if (!_env) { QWebEngineHistory::timerEvent(event); return; }
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
        QWebEngineHistory::timerEvent(event);
    }
}



//----------------------------------------------------------------------
//  Mu Type CONSTRUCTORS

QWebEngineHistoryType::QWebEngineHistoryType(Context* c, const char* name, Class* super, Class* super2)
: Class(c, name, vectorOf2(super, super2))
{
}

QWebEngineHistoryType::~QWebEngineHistoryType()
{
}

//----------------------------------------------------------------------
//  PRE-COMPILED FUNCTIONS

static Pointer
QWebEngineHistory_QWebEngineHistory_QObject(Thread& NODE_THREAD, Pointer obj)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    ClassInstance* widget = reinterpret_cast<ClassInstance*>(obj);

    if (!widget)
    {
        return 0;
    }
    else if (QWebEngineHistory* w = object<QWebEngineHistory>(widget))
    {
        QWebEngineHistoryType* type = 
            c->findSymbolOfTypeByQualifiedName<QWebEngineHistoryType>(c->internName("qt.QWebEngineHistory"), false);
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
    NODE_RETURN( QWebEngineHistory_QWebEngineHistory_QObject(NODE_THREAD, NODE_ARG(0, Pointer)) );
}

void qt_QWebEngineHistory_back_void_QWebEngineHistory(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QWebEngineHistory* arg0 = object<QWebEngineHistory>(param_this);
    arg0->back();
}

bool qt_QWebEngineHistory_canGoBack_bool_QWebEngineHistory(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QWebEngineHistory* arg0 = object<QWebEngineHistory>(param_this);
    return arg0->canGoBack();
}

bool qt_QWebEngineHistory_canGoForward_bool_QWebEngineHistory(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QWebEngineHistory* arg0 = object<QWebEngineHistory>(param_this);
    return arg0->canGoForward();
}

void qt_QWebEngineHistory_clear_void_QWebEngineHistory(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QWebEngineHistory* arg0 = object<QWebEngineHistory>(param_this);
    arg0->clear();
}

int qt_QWebEngineHistory_count_int_QWebEngineHistory(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QWebEngineHistory* arg0 = object<QWebEngineHistory>(param_this);
    return arg0->count();
}

int qt_QWebEngineHistory_currentItemIndex_int_QWebEngineHistory(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QWebEngineHistory* arg0 = object<QWebEngineHistory>(param_this);
    return arg0->currentItemIndex();
}

void qt_QWebEngineHistory_forward_void_QWebEngineHistory(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QWebEngineHistory* arg0 = object<QWebEngineHistory>(param_this);
    arg0->forward();
}

bool qt_QWebEngineHistory_event_bool_QWebEngineHistory_QEvent(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_e)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QWebEngineHistory* arg0 = object<QWebEngineHistory>(param_this);
    QEvent * arg1 = getqpointer<QEventType>(param_e);
    return isMuQtObject(arg0) ? arg0->QWebEngineHistory::event(arg1) : arg0->event(arg1);
}

bool qt_QWebEngineHistory_eventFilter_bool_QWebEngineHistory_QObject_QEvent(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_watched, Pointer param_event)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QWebEngineHistory* arg0 = object<QWebEngineHistory>(param_this);
    QObject * arg1 = object<QObject>(param_watched);
    QEvent * arg2 = getqpointer<QEventType>(param_event);
    return isMuQtObject(arg0) ? arg0->QWebEngineHistory::eventFilter(arg1, arg2) : arg0->eventFilter(arg1, arg2);
}

void qt_QWebEngineHistory_customEvent_void_QWebEngineHistory_QEvent(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_event)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QWebEngineHistory* arg0 = object<QWebEngineHistory>(param_this);
    QEvent * arg1 = getqpointer<QEventType>(param_event);
    if (isMuQtObject(arg0)) ((MuQt_QWebEngineHistory*)arg0)->customEvent_pub_parent(arg1);
    else ((MuQt_QWebEngineHistory*)arg0)->customEvent_pub(arg1);
}

void qt_QWebEngineHistory_timerEvent_void_QWebEngineHistory_QTimerEvent(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_event)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QWebEngineHistory* arg0 = object<QWebEngineHistory>(param_this);
    QTimerEvent * arg1 = getqpointer<QTimerEventType>(param_event);
    if (isMuQtObject(arg0)) ((MuQt_QWebEngineHistory*)arg0)->timerEvent_pub_parent(arg1);
    else ((MuQt_QWebEngineHistory*)arg0)->timerEvent_pub(arg1);
}


static NODE_IMPLEMENTATION(_n_back0, void)
{
    qt_QWebEngineHistory_back_void_QWebEngineHistory(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer));
}

static NODE_IMPLEMENTATION(_n_canGoBack0, bool)
{
    NODE_RETURN(qt_QWebEngineHistory_canGoBack_bool_QWebEngineHistory(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_canGoForward0, bool)
{
    NODE_RETURN(qt_QWebEngineHistory_canGoForward_bool_QWebEngineHistory(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_clear0, void)
{
    qt_QWebEngineHistory_clear_void_QWebEngineHistory(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer));
}

static NODE_IMPLEMENTATION(_n_count0, int)
{
    NODE_RETURN(qt_QWebEngineHistory_count_int_QWebEngineHistory(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_currentItemIndex0, int)
{
    NODE_RETURN(qt_QWebEngineHistory_currentItemIndex_int_QWebEngineHistory(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_forward0, void)
{
    qt_QWebEngineHistory_forward_void_QWebEngineHistory(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer));
}

static NODE_IMPLEMENTATION(_n_event0, bool)
{
    NODE_RETURN(qt_QWebEngineHistory_event_bool_QWebEngineHistory_QEvent(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}

static NODE_IMPLEMENTATION(_n_eventFilter0, bool)
{
    NODE_RETURN(qt_QWebEngineHistory_eventFilter_bool_QWebEngineHistory_QObject_QEvent(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer), NODE_ARG(2, Pointer)));
}

static NODE_IMPLEMENTATION(_n_customEvent0, void)
{
    qt_QWebEngineHistory_customEvent_void_QWebEngineHistory_QEvent(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
}

static NODE_IMPLEMENTATION(_n_timerEvent0, void)
{
    qt_QWebEngineHistory_timerEvent_void_QWebEngineHistory_QTimerEvent(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
}



void
QWebEngineHistoryType::load()
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
                            Compiled, QWebEngineHistory_QWebEngineHistory_QObject,
                            Return, ftn,
                            Parameters,
                            new Param(c, "object", "qt.QObject"),
                            End),

               EndArguments );

addSymbols(
    // enums
    // member functions
    new Function(c, "back", _n_back0, None, Compiled, qt_QWebEngineHistory_back_void_QWebEngineHistory, Return, "void", Parameters, new Param(c, "this", "qt.QWebEngineHistory"), End),
    // MISSING: backItems ("QList<QWebEngineHistoryItem>"; QWebEngineHistory this, int maxItems)
    // MISSING: backItemsModel ("QWebEngineHistoryModel *"; QWebEngineHistory this)
    new Function(c, "canGoBack", _n_canGoBack0, None, Compiled, qt_QWebEngineHistory_canGoBack_bool_QWebEngineHistory, Return, "bool", Parameters, new Param(c, "this", "qt.QWebEngineHistory"), End),
    new Function(c, "canGoForward", _n_canGoForward0, None, Compiled, qt_QWebEngineHistory_canGoForward_bool_QWebEngineHistory, Return, "bool", Parameters, new Param(c, "this", "qt.QWebEngineHistory"), End),
    new Function(c, "clear", _n_clear0, None, Compiled, qt_QWebEngineHistory_clear_void_QWebEngineHistory, Return, "void", Parameters, new Param(c, "this", "qt.QWebEngineHistory"), End),
    new Function(c, "count", _n_count0, None, Compiled, qt_QWebEngineHistory_count_int_QWebEngineHistory, Return, "int", Parameters, new Param(c, "this", "qt.QWebEngineHistory"), End),
    new Function(c, "currentItemIndex", _n_currentItemIndex0, None, Compiled, qt_QWebEngineHistory_currentItemIndex_int_QWebEngineHistory, Return, "int", Parameters, new Param(c, "this", "qt.QWebEngineHistory"), End),
    new Function(c, "forward", _n_forward0, None, Compiled, qt_QWebEngineHistory_forward_void_QWebEngineHistory, Return, "void", Parameters, new Param(c, "this", "qt.QWebEngineHistory"), End),
    // MISSING: forwardItems ("QList<QWebEngineHistoryItem>"; QWebEngineHistory this, int maxItems)
    // MISSING: forwardItemsModel ("QWebEngineHistoryModel *"; QWebEngineHistory this)
    // MISSING: goToItem (void; QWebEngineHistory this, "const QWebEngineHistoryItem &" item)
    // MISSING: items ("QList<QWebEngineHistoryItem>"; QWebEngineHistory this)
    // MISSING: itemsModel ("QWebEngineHistoryModel *"; QWebEngineHistory this)
    _func[0] = new MemberFunction(c, "event", _n_event0, None, Compiled, qt_QWebEngineHistory_event_bool_QWebEngineHistory_QEvent, Return, "bool", Parameters, new Param(c, "this", "qt.QWebEngineHistory"), new Param(c, "e", "qt.QEvent"), End),
    _func[1] = new MemberFunction(c, "eventFilter", _n_eventFilter0, None, Compiled, qt_QWebEngineHistory_eventFilter_bool_QWebEngineHistory_QObject_QEvent, Return, "bool", Parameters, new Param(c, "this", "qt.QWebEngineHistory"), new Param(c, "watched", "qt.QObject"), new Param(c, "event", "qt.QEvent"), End),
    // MISSING: metaObject ("const QMetaObject *"; QWebEngineHistory this)
    // MISSING: childEvent (void; QWebEngineHistory this, "QChildEvent *" event) // protected
    // MISSING: connectNotify (void; QWebEngineHistory this, "const QMetaMethod &" signal) // protected
    _func[2] = new MemberFunction(c, "customEvent", _n_customEvent0, None, Compiled, qt_QWebEngineHistory_customEvent_void_QWebEngineHistory_QEvent, Return, "void", Parameters, new Param(c, "this", "qt.QWebEngineHistory"), new Param(c, "event", "qt.QEvent"), End),
    // MISSING: disconnectNotify (void; QWebEngineHistory this, "const QMetaMethod &" signal) // protected
    _func[3] = new MemberFunction(c, "timerEvent", _n_timerEvent0, None, Compiled, qt_QWebEngineHistory_timerEvent_void_QWebEngineHistory_QTimerEvent, Return, "void", Parameters, new Param(c, "this", "qt.QWebEngineHistory"), new Param(c, "event", "qt.QTimerEvent"), End),
    // static functions
    EndArguments);
globalScope()->addSymbols(
    EndArguments);
scope()->addSymbols(
    EndArguments);


    const char** propExclusions = 0;

    populate(this, QWebEngineHistory::staticMetaObject, propExclusions);
}

} // Mu
