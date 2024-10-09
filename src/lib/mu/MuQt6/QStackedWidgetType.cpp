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
#include <MuQt6/QStackedWidgetType.h>
#include <QtGui/QtGui>
#include <QtWidgets/QtWidgets>
#include <QtSvg/QtSvg>
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
#include <MuQt6/QSizeType.h>
#include <MuQt6/QPaintEventType.h>
#include <MuQt6/QWidgetType.h>
#include <MuQt6/QEventType.h>

namespace Mu {
using namespace std;

//----------------------------------------------------------------------
//  INHERITABLE TYPE IMPLEMENTATION

// destructor
MuQt_QStackedWidget::~MuQt_QStackedWidget()
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

MuQt_QStackedWidget::MuQt_QStackedWidget(Pointer muobj, const CallEnvironment* ce, QWidget * parent) 
 : QStackedWidget(parent)
{
    _env = ce;
    _obj = reinterpret_cast<ClassInstance*>(muobj);
    _obj->retainExternal();
    MuLangContext* c = (MuLangContext*)_env->context();
    _baseType = c->findSymbolOfTypeByQualifiedName<QStackedWidgetType>(c->internName("qt.QStackedWidget"));
}

bool MuQt_QStackedWidget::event(QEvent * e) 
{
    if (!_env) return QStackedWidget::event(e);
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
        return QStackedWidget::event(e);
    }
}

QSize MuQt_QStackedWidget::sizeHint() const
{
    if (!_env) return QStackedWidget::sizeHint();
    MuLangContext* c = (MuLangContext*)_env->context();
    const MemberFunction* F0 = _baseType->_func[1];
    const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
    if (F != F0) 
    {
        Function::ArgumentVector args(1);
        args[0] = Value(Pointer(_obj));
        Value rval = _env->call(F, args);
        return getqtype<QSizeType>(rval._Pointer);
    }
    else
    {
        return QStackedWidget::sizeHint();
    }
}

void MuQt_QStackedWidget::changeEvent(QEvent * ev) 
{
    if (!_env) { QStackedWidget::changeEvent(ev); return; }
    MuLangContext* c = (MuLangContext*)_env->context();
    const MemberFunction* F0 = _baseType->_func[2];
    const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
    if (F != F0) 
    {
        Function::ArgumentVector args(2);
        args[0] = Value(Pointer(_obj));
        args[1] = Value(makeqpointer<QEventType>(c,ev,"qt.QEvent"));
        Value rval = _env->call(F, args);
    }
    else
    {
        QStackedWidget::changeEvent(ev);
    }
}

void MuQt_QStackedWidget::paintEvent(QPaintEvent * _p14) 
{
    if (!_env) { QStackedWidget::paintEvent(_p14); return; }
    MuLangContext* c = (MuLangContext*)_env->context();
    const MemberFunction* F0 = _baseType->_func[3];
    const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
    if (F != F0) 
    {
        Function::ArgumentVector args(2);
        args[0] = Value(Pointer(_obj));
        args[1] = Value(makeqpointer<QPaintEventType>(c,_p14,"qt.QPaintEvent"));
        Value rval = _env->call(F, args);
    }
    else
    {
        QStackedWidget::paintEvent(_p14);
    }
}



//----------------------------------------------------------------------
//  Mu Type CONSTRUCTORS

QStackedWidgetType::QStackedWidgetType(Context* c, const char* name, Class* super, Class* super2)
: Class(c, name, vectorOf2(super, super2))
{
}

QStackedWidgetType::~QStackedWidgetType()
{
}

//----------------------------------------------------------------------
//  PRE-COMPILED FUNCTIONS

static Pointer
QStackedWidget_QStackedWidget_QObject(Thread& NODE_THREAD, Pointer obj)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    ClassInstance* widget = reinterpret_cast<ClassInstance*>(obj);

    if (!widget)
    {
        return 0;
    }
    else if (QStackedWidget* w = object<QStackedWidget>(widget))
    {
        QStackedWidgetType* type = 
            c->findSymbolOfTypeByQualifiedName<QStackedWidgetType>(c->internName("qt.QStackedWidget"), false);
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
    NODE_RETURN( QStackedWidget_QStackedWidget_QObject(NODE_THREAD, NODE_ARG(0, Pointer)) );
}

Pointer qt_QStackedWidget_QStackedWidget_QStackedWidget_QStackedWidget_QWidget(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_parent)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QWidget * arg1 = object<QWidget>(param_parent);
    setobject(param_this, new MuQt_QStackedWidget(param_this, NODE_THREAD.process()->callEnv(), arg1));
    return param_this;
}

int qt_QStackedWidget_addWidget_int_QStackedWidget_QWidget(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_widget)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QStackedWidget* arg0 = object<QStackedWidget>(param_this);
    QWidget * arg1 = object<QWidget>(param_widget);
    return arg0->addWidget(arg1);
}

Pointer qt_QStackedWidget_currentWidget_QWidget_QStackedWidget(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QStackedWidget* arg0 = object<QStackedWidget>(param_this);
    return makeinstance<QWidgetType>(c, arg0->currentWidget(), "qt.QWidget");
}

int qt_QStackedWidget_indexOf_int_QStackedWidget_QWidget(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_widget)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QStackedWidget* arg0 = object<QStackedWidget>(param_this);
    const QWidget * arg1 = object<QWidget>(param_widget);
    return arg0->indexOf(arg1);
}

int qt_QStackedWidget_insertWidget_int_QStackedWidget_int_QWidget(Mu::Thread& NODE_THREAD, Pointer param_this, int param_index, Pointer param_widget)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QStackedWidget* arg0 = object<QStackedWidget>(param_this);
    int arg1 = (int)(param_index);
    QWidget * arg2 = object<QWidget>(param_widget);
    return arg0->insertWidget(arg1, arg2);
}

void qt_QStackedWidget_removeWidget_void_QStackedWidget_QWidget(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_widget)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QStackedWidget* arg0 = object<QStackedWidget>(param_this);
    QWidget * arg1 = object<QWidget>(param_widget);
    arg0->removeWidget(arg1);
}

Pointer qt_QStackedWidget_widget_QWidget_QStackedWidget_int(Mu::Thread& NODE_THREAD, Pointer param_this, int param_index)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QStackedWidget* arg0 = object<QStackedWidget>(param_this);
    int arg1 = (int)(param_index);
    return makeinstance<QWidgetType>(c, arg0->widget(arg1), "qt.QWidget");
}

bool qt_QStackedWidget_event_bool_QStackedWidget_QEvent(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_e)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QStackedWidget* arg0 = object<QStackedWidget>(param_this);
    QEvent * arg1 = getqpointer<QEventType>(param_e);
    return isMuQtObject(arg0) ? ((MuQt_QStackedWidget*)arg0)->event_pub_parent(arg1) : ((MuQt_QStackedWidget*)arg0)->event_pub(arg1);
}

Pointer qt_QStackedWidget_sizeHint_QSize_QStackedWidget(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QStackedWidget* arg0 = object<QStackedWidget>(param_this);
    return isMuQtObject(arg0) ? makeqtype<QSizeType>(c,arg0->QStackedWidget::sizeHint(),"qt.QSize") : makeqtype<QSizeType>(c,arg0->sizeHint(),"qt.QSize");
}

void qt_QStackedWidget_changeEvent_void_QStackedWidget_QEvent(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_ev)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QStackedWidget* arg0 = object<QStackedWidget>(param_this);
    QEvent * arg1 = getqpointer<QEventType>(param_ev);
    if (isMuQtObject(arg0)) ((MuQt_QStackedWidget*)arg0)->changeEvent_pub_parent(arg1);
    else ((MuQt_QStackedWidget*)arg0)->changeEvent_pub(arg1);
}

void qt_QStackedWidget_paintEvent_void_QStackedWidget_QPaintEvent(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param__p14)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QStackedWidget* arg0 = object<QStackedWidget>(param_this);
    QPaintEvent * arg1 = getqpointer<QPaintEventType>(param__p14);
    if (isMuQtObject(arg0)) ((MuQt_QStackedWidget*)arg0)->paintEvent_pub_parent(arg1);
    else ((MuQt_QStackedWidget*)arg0)->paintEvent_pub(arg1);
}


static NODE_IMPLEMENTATION(_n_QStackedWidget0, Pointer)
{
    NODE_RETURN(qt_QStackedWidget_QStackedWidget_QStackedWidget_QStackedWidget_QWidget(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}

static NODE_IMPLEMENTATION(_n_addWidget0, int)
{
    NODE_RETURN(qt_QStackedWidget_addWidget_int_QStackedWidget_QWidget(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}

static NODE_IMPLEMENTATION(_n_currentWidget0, Pointer)
{
    NODE_RETURN(qt_QStackedWidget_currentWidget_QWidget_QStackedWidget(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_indexOf0, int)
{
    NODE_RETURN(qt_QStackedWidget_indexOf_int_QStackedWidget_QWidget(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}

static NODE_IMPLEMENTATION(_n_insertWidget0, int)
{
    NODE_RETURN(qt_QStackedWidget_insertWidget_int_QStackedWidget_int_QWidget(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int), NODE_ARG(2, Pointer)));
}

static NODE_IMPLEMENTATION(_n_removeWidget0, void)
{
    qt_QStackedWidget_removeWidget_void_QStackedWidget_QWidget(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
}

static NODE_IMPLEMENTATION(_n_widget0, Pointer)
{
    NODE_RETURN(qt_QStackedWidget_widget_QWidget_QStackedWidget_int(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int)));
}

static NODE_IMPLEMENTATION(_n_event0, bool)
{
    NODE_RETURN(qt_QStackedWidget_event_bool_QStackedWidget_QEvent(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}

static NODE_IMPLEMENTATION(_n_sizeHint0, Pointer)
{
    NODE_RETURN(qt_QStackedWidget_sizeHint_QSize_QStackedWidget(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_changeEvent0, void)
{
    qt_QStackedWidget_changeEvent_void_QStackedWidget_QEvent(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
}

static NODE_IMPLEMENTATION(_n_paintEvent0, void)
{
    qt_QStackedWidget_paintEvent_void_QStackedWidget_QPaintEvent(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
}



void
QStackedWidgetType::load()
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
                            Compiled, QStackedWidget_QStackedWidget_QObject,
                            Return, ftn,
                            Parameters,
                            new Param(c, "object", "qt.QObject"),
                            End),

               EndArguments );

addSymbols(
    // enums
    // member functions
    new Function(c, "QStackedWidget", _n_QStackedWidget0, None, Compiled, qt_QStackedWidget_QStackedWidget_QStackedWidget_QStackedWidget_QWidget, Return, "qt.QStackedWidget", Parameters, new Param(c, "this", "qt.QStackedWidget"), new Param(c, "parent", "qt.QWidget"), End),
    new Function(c, "addWidget", _n_addWidget0, None, Compiled, qt_QStackedWidget_addWidget_int_QStackedWidget_QWidget, Return, "int", Parameters, new Param(c, "this", "qt.QStackedWidget"), new Param(c, "widget", "qt.QWidget"), End),
    // PROP: count (int; QStackedWidget this)
    // PROP: currentIndex (int; QStackedWidget this)
    new Function(c, "currentWidget", _n_currentWidget0, None, Compiled, qt_QStackedWidget_currentWidget_QWidget_QStackedWidget, Return, "qt.QWidget", Parameters, new Param(c, "this", "qt.QStackedWidget"), End),
    new Function(c, "indexOf", _n_indexOf0, None, Compiled, qt_QStackedWidget_indexOf_int_QStackedWidget_QWidget, Return, "int", Parameters, new Param(c, "this", "qt.QStackedWidget"), new Param(c, "widget", "qt.QWidget"), End),
    new Function(c, "insertWidget", _n_insertWidget0, None, Compiled, qt_QStackedWidget_insertWidget_int_QStackedWidget_int_QWidget, Return, "int", Parameters, new Param(c, "this", "qt.QStackedWidget"), new Param(c, "index", "int"), new Param(c, "widget", "qt.QWidget"), End),
    new Function(c, "removeWidget", _n_removeWidget0, None, Compiled, qt_QStackedWidget_removeWidget_void_QStackedWidget_QWidget, Return, "void", Parameters, new Param(c, "this", "qt.QStackedWidget"), new Param(c, "widget", "qt.QWidget"), End),
    new Function(c, "widget", _n_widget0, None, Compiled, qt_QStackedWidget_widget_QWidget_QStackedWidget_int, Return, "qt.QWidget", Parameters, new Param(c, "this", "qt.QStackedWidget"), new Param(c, "index", "int"), End),
    _func[0] = new MemberFunction(c, "event", _n_event0, None, Compiled, qt_QStackedWidget_event_bool_QStackedWidget_QEvent, Return, "bool", Parameters, new Param(c, "this", "qt.QStackedWidget"), new Param(c, "e", "qt.QEvent"), End),
    _func[1] = new MemberFunction(c, "sizeHint", _n_sizeHint0, None, Compiled, qt_QStackedWidget_sizeHint_QSize_QStackedWidget, Return, "qt.QSize", Parameters, new Param(c, "this", "qt.QStackedWidget"), End),
    // MISSING: initStyleOption (void; QStackedWidget this, "QStyleOptionFrame *" option) // protected
    _func[2] = new MemberFunction(c, "changeEvent", _n_changeEvent0, None, Compiled, qt_QStackedWidget_changeEvent_void_QStackedWidget_QEvent, Return, "void", Parameters, new Param(c, "this", "qt.QStackedWidget"), new Param(c, "ev", "qt.QEvent"), End),
    _func[3] = new MemberFunction(c, "paintEvent", _n_paintEvent0, None, Compiled, qt_QStackedWidget_paintEvent_void_QStackedWidget_QPaintEvent, Return, "void", Parameters, new Param(c, "this", "qt.QStackedWidget"), new Param(c, "_p14", "qt.QPaintEvent"), End),
    // static functions
    EndArguments);
globalScope()->addSymbols(
    EndArguments);
scope()->addSymbols(
    EndArguments);


    const char** propExclusions = 0;

    populate(this, QStackedWidget::staticMetaObject, propExclusions);
}

} // Mu
