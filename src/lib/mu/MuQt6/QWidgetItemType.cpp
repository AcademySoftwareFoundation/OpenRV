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
#include <MuQt6/QWidgetItemType.h>
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
#include <MuQt6/QSizeType.h>
#include <MuQt6/QRectType.h>
#include <MuQt6/QWidgetType.h>
#include <MuQt6/QLayoutType.h>

namespace Mu {
using namespace std;

//----------------------------------------------------------------------
//  INHERITABLE TYPE IMPLEMENTATION

// destructor
MuQt_QWidgetItem::~MuQt_QWidgetItem()
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

MuQt_QWidgetItem::MuQt_QWidgetItem(Pointer muobj, const CallEnvironment* ce, QWidget * widget) 
 : QWidgetItem(widget)
{
    _env = ce;
    _obj = reinterpret_cast<ClassInstance*>(muobj);
    _obj->retainExternal();
    MuLangContext* c = (MuLangContext*)_env->context();
    _baseType = c->findSymbolOfTypeByQualifiedName<QWidgetItemType>(c->internName("qt.QWidgetItem"));
}

QSizePolicy::ControlTypes MuQt_QWidgetItem::controlTypes() const
{
    if (!_env) return QWidgetItem::controlTypes();
    MuLangContext* c = (MuLangContext*)_env->context();
    const MemberFunction* F0 = _baseType->_func[0];
    const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
    if (F != F0) 
    {
        Function::ArgumentVector args(1);
        args[0] = Value(Pointer(_obj));
        Value rval = _env->call(F, args);
        return (QSizePolicy::ControlTypes)(rval._int);
    }
    else
    {
        return QWidgetItem::controlTypes();
    }
}

Qt::Orientations MuQt_QWidgetItem::expandingDirections() const
{
    if (!_env) return QWidgetItem::expandingDirections();
    MuLangContext* c = (MuLangContext*)_env->context();
    const MemberFunction* F0 = _baseType->_func[1];
    const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
    if (F != F0) 
    {
        Function::ArgumentVector args(1);
        args[0] = Value(Pointer(_obj));
        Value rval = _env->call(F, args);
        return (Qt::Orientations)(rval._int);
    }
    else
    {
        return QWidgetItem::expandingDirections();
    }
}

QRect MuQt_QWidgetItem::geometry() const
{
    if (!_env) return QWidgetItem::geometry();
    MuLangContext* c = (MuLangContext*)_env->context();
    const MemberFunction* F0 = _baseType->_func[2];
    const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
    if (F != F0) 
    {
        Function::ArgumentVector args(1);
        args[0] = Value(Pointer(_obj));
        Value rval = _env->call(F, args);
        return getqtype<QRectType>(rval._Pointer);
    }
    else
    {
        return QWidgetItem::geometry();
    }
}

bool MuQt_QWidgetItem::hasHeightForWidth() const
{
    if (!_env) return QWidgetItem::hasHeightForWidth();
    MuLangContext* c = (MuLangContext*)_env->context();
    const MemberFunction* F0 = _baseType->_func[3];
    const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
    if (F != F0) 
    {
        Function::ArgumentVector args(1);
        args[0] = Value(Pointer(_obj));
        Value rval = _env->call(F, args);
        return (bool)(rval._bool);
    }
    else
    {
        return QWidgetItem::hasHeightForWidth();
    }
}

int MuQt_QWidgetItem::heightForWidth(int w) const
{
    if (!_env) return QWidgetItem::heightForWidth(w);
    MuLangContext* c = (MuLangContext*)_env->context();
    const MemberFunction* F0 = _baseType->_func[4];
    const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
    if (F != F0) 
    {
        Function::ArgumentVector args(2);
        args[0] = Value(Pointer(_obj));
        args[1] = Value(w);
        Value rval = _env->call(F, args);
        return (int)(rval._int);
    }
    else
    {
        return QWidgetItem::heightForWidth(w);
    }
}

bool MuQt_QWidgetItem::isEmpty() const
{
    if (!_env) return QWidgetItem::isEmpty();
    MuLangContext* c = (MuLangContext*)_env->context();
    const MemberFunction* F0 = _baseType->_func[5];
    const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
    if (F != F0) 
    {
        Function::ArgumentVector args(1);
        args[0] = Value(Pointer(_obj));
        Value rval = _env->call(F, args);
        return (bool)(rval._bool);
    }
    else
    {
        return QWidgetItem::isEmpty();
    }
}

QSize MuQt_QWidgetItem::maximumSize() const
{
    if (!_env) return QWidgetItem::maximumSize();
    MuLangContext* c = (MuLangContext*)_env->context();
    const MemberFunction* F0 = _baseType->_func[6];
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
        return QWidgetItem::maximumSize();
    }
}

QSize MuQt_QWidgetItem::minimumSize() const
{
    if (!_env) return QWidgetItem::minimumSize();
    MuLangContext* c = (MuLangContext*)_env->context();
    const MemberFunction* F0 = _baseType->_func[7];
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
        return QWidgetItem::minimumSize();
    }
}

void MuQt_QWidgetItem::setGeometry(const QRect & rect) 
{
    if (!_env) { QWidgetItem::setGeometry(rect); return; }
    MuLangContext* c = (MuLangContext*)_env->context();
    const MemberFunction* F0 = _baseType->_func[8];
    const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
    if (F != F0) 
    {
        Function::ArgumentVector args(2);
        args[0] = Value(Pointer(_obj));
        args[1] = Value(makeqtype<QRectType>(c,rect,"qt.QRect"));
        Value rval = _env->call(F, args);
    }
    else
    {
        QWidgetItem::setGeometry(rect);
    }
}

QSize MuQt_QWidgetItem::sizeHint() const
{
    if (!_env) return QWidgetItem::sizeHint();
    MuLangContext* c = (MuLangContext*)_env->context();
    const MemberFunction* F0 = _baseType->_func[9];
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
        return QWidgetItem::sizeHint();
    }
}

QWidget * MuQt_QWidgetItem::widget() const
{
    if (!_env) return QWidgetItem::widget();
    MuLangContext* c = (MuLangContext*)_env->context();
    const MemberFunction* F0 = _baseType->_func[10];
    const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
    if (F != F0) 
    {
        Function::ArgumentVector args(1);
        args[0] = Value(Pointer(_obj));
        Value rval = _env->call(F, args);
        return object<QWidget>(rval._Pointer);
    }
    else
    {
        return QWidgetItem::widget();
    }
}

void MuQt_QWidgetItem::invalidate() 
{
    if (!_env) { QWidgetItem::invalidate(); return; }
    MuLangContext* c = (MuLangContext*)_env->context();
    const MemberFunction* F0 = _baseType->_func[11];
    const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
    if (F != F0) 
    {
        Function::ArgumentVector args(1);
        args[0] = Value(Pointer(_obj));
        Value rval = _env->call(F, args);
    }
    else
    {
        QWidgetItem::invalidate();
    }
}

QLayout * MuQt_QWidgetItem::layout() 
{
    if (!_env) return QWidgetItem::layout();
    MuLangContext* c = (MuLangContext*)_env->context();
    const MemberFunction* F0 = _baseType->_func[12];
    const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
    if (F != F0) 
    {
        Function::ArgumentVector args(1);
        args[0] = Value(Pointer(_obj));
        Value rval = _env->call(F, args);
        return object<QLayout>(rval._Pointer);
    }
    else
    {
        return QWidgetItem::layout();
    }
}

int MuQt_QWidgetItem::minimumHeightForWidth(int w) const
{
    if (!_env) return QWidgetItem::minimumHeightForWidth(w);
    MuLangContext* c = (MuLangContext*)_env->context();
    const MemberFunction* F0 = _baseType->_func[13];
    const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
    if (F != F0) 
    {
        Function::ArgumentVector args(2);
        args[0] = Value(Pointer(_obj));
        args[1] = Value(w);
        Value rval = _env->call(F, args);
        return (int)(rval._int);
    }
    else
    {
        return QWidgetItem::minimumHeightForWidth(w);
    }
}



//----------------------------------------------------------------------
//  Mu Type CONSTRUCTORS

QWidgetItemType::QWidgetItemType(Context* c, const char* name, Class* super)
    : Class(c, name, super)
{
}

QWidgetItemType::~QWidgetItemType()
{
}

//----------------------------------------------------------------------
//  PRE-COMPILED FUNCTIONS

static Pointer
QWidgetItem_QWidgetItem_QLayoutItem(Thread& NODE_THREAD, Pointer obj)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    ClassInstance* item = reinterpret_cast<ClassInstance*>(obj);

    if (!item)
    {
        return 0;
    }
    else if (QWidgetItem* i = layoutitem<QWidgetItem>(item))
    {
        QWidgetItemType* type = 
            c->findSymbolOfTypeByQualifiedName<QWidgetItemType>(c->internName("qt.QWidgetItem"), false);
        ClassInstance* o = ClassInstance::allocate(type);
        setlayoutitem(o, i);
        return o;
    }
    else
    {
        throw BadCastException();
    }
}

static NODE_IMPLEMENTATION(castFromLayoutItem, Pointer)
{
    NODE_RETURN( QWidgetItem_QWidgetItem_QLayoutItem(NODE_THREAD, NODE_ARG(0, Pointer)) );
}

Pointer qt_QWidgetItem_QWidgetItem_QWidgetItem_QWidgetItem_QWidget(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_widget)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QWidget * arg1 = object<QWidget>(param_widget);
    setlayoutitem(param_this, new MuQt_QWidgetItem(param_this, NODE_THREAD.process()->callEnv(), arg1));
    return param_this;
}

int qt_QWidgetItem_controlTypes_int_QWidgetItem(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QWidgetItem* arg0 = layoutitem<QWidgetItem>(param_this);
    return isMuQtLayoutItem(arg0) ? int(arg0->QWidgetItem::controlTypes()) : int(arg0->controlTypes());
}

int qt_QWidgetItem_expandingDirections_int_QWidgetItem(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QWidgetItem* arg0 = layoutitem<QWidgetItem>(param_this);
    return isMuQtLayoutItem(arg0) ? int(arg0->QWidgetItem::expandingDirections()) : int(arg0->expandingDirections());
}

Pointer qt_QWidgetItem_geometry_QRect_QWidgetItem(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QWidgetItem* arg0 = layoutitem<QWidgetItem>(param_this);
    return isMuQtLayoutItem(arg0) ? makeqtype<QRectType>(c,arg0->QWidgetItem::geometry(),"qt.QRect") : makeqtype<QRectType>(c,arg0->geometry(),"qt.QRect");
}

bool qt_QWidgetItem_hasHeightForWidth_bool_QWidgetItem(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QWidgetItem* arg0 = layoutitem<QWidgetItem>(param_this);
    return isMuQtLayoutItem(arg0) ? arg0->QWidgetItem::hasHeightForWidth() : arg0->hasHeightForWidth();
}

int qt_QWidgetItem_heightForWidth_int_QWidgetItem_int(Mu::Thread& NODE_THREAD, Pointer param_this, int param_w)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QWidgetItem* arg0 = layoutitem<QWidgetItem>(param_this);
    int arg1 = (int)(param_w);
    return isMuQtLayoutItem(arg0) ? arg0->QWidgetItem::heightForWidth(arg1) : arg0->heightForWidth(arg1);
}

bool qt_QWidgetItem_isEmpty_bool_QWidgetItem(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QWidgetItem* arg0 = layoutitem<QWidgetItem>(param_this);
    return isMuQtLayoutItem(arg0) ? arg0->QWidgetItem::isEmpty() : arg0->isEmpty();
}

Pointer qt_QWidgetItem_maximumSize_QSize_QWidgetItem(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QWidgetItem* arg0 = layoutitem<QWidgetItem>(param_this);
    return isMuQtLayoutItem(arg0) ? makeqtype<QSizeType>(c,arg0->QWidgetItem::maximumSize(),"qt.QSize") : makeqtype<QSizeType>(c,arg0->maximumSize(),"qt.QSize");
}

Pointer qt_QWidgetItem_minimumSize_QSize_QWidgetItem(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QWidgetItem* arg0 = layoutitem<QWidgetItem>(param_this);
    return isMuQtLayoutItem(arg0) ? makeqtype<QSizeType>(c,arg0->QWidgetItem::minimumSize(),"qt.QSize") : makeqtype<QSizeType>(c,arg0->minimumSize(),"qt.QSize");
}

void qt_QWidgetItem_setGeometry_void_QWidgetItem_QRect(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_rect)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QWidgetItem* arg0 = layoutitem<QWidgetItem>(param_this);
    const QRect  arg1 = getqtype<QRectType>(param_rect);
    if (isMuQtLayoutItem(arg0)) arg0->QWidgetItem::setGeometry(arg1);
    else arg0->setGeometry(arg1);
}

Pointer qt_QWidgetItem_sizeHint_QSize_QWidgetItem(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QWidgetItem* arg0 = layoutitem<QWidgetItem>(param_this);
    return isMuQtLayoutItem(arg0) ? makeqtype<QSizeType>(c,arg0->QWidgetItem::sizeHint(),"qt.QSize") : makeqtype<QSizeType>(c,arg0->sizeHint(),"qt.QSize");
}

Pointer qt_QWidgetItem_widget_QWidget_QWidgetItem(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QWidgetItem* arg0 = layoutitem<QWidgetItem>(param_this);
    return isMuQtLayoutItem(arg0) ? makeinstance<QWidgetType>(c, arg0->QWidgetItem::widget(), "qt.QWidget") : makeinstance<QWidgetType>(c, arg0->widget(), "qt.QWidget");
}

void qt_QWidgetItem_invalidate_void_QWidgetItem(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QWidgetItem* arg0 = layoutitem<QWidgetItem>(param_this);
    if (isMuQtLayoutItem(arg0)) arg0->QWidgetItem::invalidate();
    else arg0->invalidate();
}

Pointer qt_QWidgetItem_layout_QLayout_QWidgetItem(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QWidgetItem* arg0 = layoutitem<QWidgetItem>(param_this);
    return isMuQtLayoutItem(arg0) ? makeinstance<QLayoutType>(c, arg0->QWidgetItem::layout(), "qt.QLayout") : makeinstance<QLayoutType>(c, arg0->layout(), "qt.QLayout");
}

int qt_QWidgetItem_minimumHeightForWidth_int_QWidgetItem_int(Mu::Thread& NODE_THREAD, Pointer param_this, int param_w)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QWidgetItem* arg0 = layoutitem<QWidgetItem>(param_this);
    int arg1 = (int)(param_w);
    return isMuQtLayoutItem(arg0) ? arg0->QWidgetItem::minimumHeightForWidth(arg1) : arg0->minimumHeightForWidth(arg1);
}


static NODE_IMPLEMENTATION(_n_QWidgetItem0, Pointer)
{
    NODE_RETURN(qt_QWidgetItem_QWidgetItem_QWidgetItem_QWidgetItem_QWidget(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}

static NODE_IMPLEMENTATION(_n_controlTypes0, int)
{
    NODE_RETURN(qt_QWidgetItem_controlTypes_int_QWidgetItem(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_expandingDirections0, int)
{
    NODE_RETURN(qt_QWidgetItem_expandingDirections_int_QWidgetItem(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_geometry0, Pointer)
{
    NODE_RETURN(qt_QWidgetItem_geometry_QRect_QWidgetItem(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_hasHeightForWidth0, bool)
{
    NODE_RETURN(qt_QWidgetItem_hasHeightForWidth_bool_QWidgetItem(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_heightForWidth0, int)
{
    NODE_RETURN(qt_QWidgetItem_heightForWidth_int_QWidgetItem_int(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int)));
}

static NODE_IMPLEMENTATION(_n_isEmpty0, bool)
{
    NODE_RETURN(qt_QWidgetItem_isEmpty_bool_QWidgetItem(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_maximumSize0, Pointer)
{
    NODE_RETURN(qt_QWidgetItem_maximumSize_QSize_QWidgetItem(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_minimumSize0, Pointer)
{
    NODE_RETURN(qt_QWidgetItem_minimumSize_QSize_QWidgetItem(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_setGeometry0, void)
{
    qt_QWidgetItem_setGeometry_void_QWidgetItem_QRect(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
}

static NODE_IMPLEMENTATION(_n_sizeHint0, Pointer)
{
    NODE_RETURN(qt_QWidgetItem_sizeHint_QSize_QWidgetItem(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_widget0, Pointer)
{
    NODE_RETURN(qt_QWidgetItem_widget_QWidget_QWidgetItem(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_invalidate0, void)
{
    qt_QWidgetItem_invalidate_void_QWidgetItem(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer));
}

static NODE_IMPLEMENTATION(_n_layout0, Pointer)
{
    NODE_RETURN(qt_QWidgetItem_layout_QLayout_QWidgetItem(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_minimumHeightForWidth0, int)
{
    NODE_RETURN(qt_QWidgetItem_minimumHeightForWidth_int_QWidgetItem_int(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int)));
}



void
QWidgetItemType::load()
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


               new Function(c, tn, castFromLayoutItem, Cast,
                            Compiled, QWidgetItem_QWidgetItem_QLayoutItem,
                            Return, ftn,
                            Parameters,
                            new Param(c, "layoutItem", "qt.QLayoutItem"),
                            End),

               EndArguments );

addSymbols(
    // enums
    // member functions
    new Function(c, "QWidgetItem", _n_QWidgetItem0, None, Compiled, qt_QWidgetItem_QWidgetItem_QWidgetItem_QWidgetItem_QWidget, Return, "qt.QWidgetItem", Parameters, new Param(c, "this", "qt.QWidgetItem"), new Param(c, "widget", "qt.QWidget"), End),
    _func[0] = new MemberFunction(c, "controlTypes", _n_controlTypes0, None, Compiled, qt_QWidgetItem_controlTypes_int_QWidgetItem, Return, "int", Parameters, new Param(c, "this", "qt.QWidgetItem"), End),
    _func[1] = new MemberFunction(c, "expandingDirections", _n_expandingDirections0, None, Compiled, qt_QWidgetItem_expandingDirections_int_QWidgetItem, Return, "int", Parameters, new Param(c, "this", "qt.QWidgetItem"), End),
    _func[2] = new MemberFunction(c, "geometry", _n_geometry0, None, Compiled, qt_QWidgetItem_geometry_QRect_QWidgetItem, Return, "qt.QRect", Parameters, new Param(c, "this", "qt.QWidgetItem"), End),
    _func[3] = new MemberFunction(c, "hasHeightForWidth", _n_hasHeightForWidth0, None, Compiled, qt_QWidgetItem_hasHeightForWidth_bool_QWidgetItem, Return, "bool", Parameters, new Param(c, "this", "qt.QWidgetItem"), End),
    _func[4] = new MemberFunction(c, "heightForWidth", _n_heightForWidth0, None, Compiled, qt_QWidgetItem_heightForWidth_int_QWidgetItem_int, Return, "int", Parameters, new Param(c, "this", "qt.QWidgetItem"), new Param(c, "w", "int"), End),
    _func[5] = new MemberFunction(c, "isEmpty", _n_isEmpty0, None, Compiled, qt_QWidgetItem_isEmpty_bool_QWidgetItem, Return, "bool", Parameters, new Param(c, "this", "qt.QWidgetItem"), End),
    _func[6] = new MemberFunction(c, "maximumSize", _n_maximumSize0, None, Compiled, qt_QWidgetItem_maximumSize_QSize_QWidgetItem, Return, "qt.QSize", Parameters, new Param(c, "this", "qt.QWidgetItem"), End),
    _func[7] = new MemberFunction(c, "minimumSize", _n_minimumSize0, None, Compiled, qt_QWidgetItem_minimumSize_QSize_QWidgetItem, Return, "qt.QSize", Parameters, new Param(c, "this", "qt.QWidgetItem"), End),
    _func[8] = new MemberFunction(c, "setGeometry", _n_setGeometry0, None, Compiled, qt_QWidgetItem_setGeometry_void_QWidgetItem_QRect, Return, "void", Parameters, new Param(c, "this", "qt.QWidgetItem"), new Param(c, "rect", "qt.QRect"), End),
    _func[9] = new MemberFunction(c, "sizeHint", _n_sizeHint0, None, Compiled, qt_QWidgetItem_sizeHint_QSize_QWidgetItem, Return, "qt.QSize", Parameters, new Param(c, "this", "qt.QWidgetItem"), End),
    _func[10] = new MemberFunction(c, "widget", _n_widget0, None, Compiled, qt_QWidgetItem_widget_QWidget_QWidgetItem, Return, "qt.QWidget", Parameters, new Param(c, "this", "qt.QWidgetItem"), End),
    _func[11] = new MemberFunction(c, "invalidate", _n_invalidate0, None, Compiled, qt_QWidgetItem_invalidate_void_QWidgetItem, Return, "void", Parameters, new Param(c, "this", "qt.QWidgetItem"), End),
    _func[12] = new MemberFunction(c, "layout", _n_layout0, None, Compiled, qt_QWidgetItem_layout_QLayout_QWidgetItem, Return, "qt.QLayout", Parameters, new Param(c, "this", "qt.QWidgetItem"), End),
    _func[13] = new MemberFunction(c, "minimumHeightForWidth", _n_minimumHeightForWidth0, None, Compiled, qt_QWidgetItem_minimumHeightForWidth_int_QWidgetItem_int, Return, "int", Parameters, new Param(c, "this", "qt.QWidgetItem"), new Param(c, "w", "int"), End),
    // MISSING: spacerItem ("QSpacerItem *"; QWidgetItem this)
    // static functions
    EndArguments);
globalScope()->addSymbols(
    EndArguments);
scope()->addSymbols(
    EndArguments);

}

} // Mu
