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
#include <MuQt6/QApplicationType.h>
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
#include <MuQt6/QWidgetType.h>
#include <MuQt6/QPaletteType.h>
#include <MuQt6/QPointType.h>
#include <MuQt6/QFontType.h>

namespace Mu {
using namespace std;

//----------------------------------------------------------------------
//  INHERITABLE TYPE IMPLEMENTATION

// destructor
MuQt_QApplication::~MuQt_QApplication()
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

bool MuQt_QApplication::notify(QObject * receiver, QEvent * e) 
{
    if (!_env) return QApplication::notify(receiver, e);
    MuLangContext* c = (MuLangContext*)_env->context();
    const MemberFunction* F0 = _baseType->_func[0];
    const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
    if (F != F0) 
    {
        Function::ArgumentVector args(3);
        args[0] = Value(Pointer(_obj));
        args[1] = Value(makeinstance<QObjectType>(c,receiver,"qt.QObject"));
        args[2] = Value(makeqpointer<QEventType>(c,e,"qt.QEvent"));
        Value rval = _env->call(F, args);
        return (bool)(rval._bool);
    }
    else
    {
        return QApplication::notify(receiver, e);
    }
}

bool MuQt_QApplication::event(QEvent * e) 
{
    if (!_env) return QApplication::event(e);
    MuLangContext* c = (MuLangContext*)_env->context();
    const MemberFunction* F0 = _baseType->_func[1];
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
        return QApplication::event(e);
    }
}



//----------------------------------------------------------------------
//  Mu Type CONSTRUCTORS

QApplicationType::QApplicationType(Context* c, const char* name, Class* super, Class* super2)
: Class(c, name, vectorOf2(super, super2))
{
}

QApplicationType::~QApplicationType()
{
}

//----------------------------------------------------------------------
//  PRE-COMPILED FUNCTIONS

static Pointer
QApplication_QApplication_QObject(Thread& NODE_THREAD, Pointer obj)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    ClassInstance* widget = reinterpret_cast<ClassInstance*>(obj);

    if (!widget)
    {
        return 0;
    }
    else if (QApplication* w = object<QApplication>(widget))
    {
        QApplicationType* type = 
            c->findSymbolOfTypeByQualifiedName<QApplicationType>(c->internName("qt.QApplication"), false);
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
    NODE_RETURN( QApplication_QApplication_QObject(NODE_THREAD, NODE_ARG(0, Pointer)) );
}

bool qt_QApplication_notify_bool_QApplication_QObject_QEvent(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_receiver, Pointer param_e)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QApplication* arg0 = object<QApplication>(param_this);
    QObject * arg1 = object<QObject>(param_receiver);
    QEvent * arg2 = getqpointer<QEventType>(param_e);
    return isMuQtObject(arg0) ? arg0->QApplication::notify(arg1, arg2) : arg0->notify(arg1, arg2);
}

bool qt_QApplication_event_bool_QApplication_QEvent(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_e)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QApplication* arg0 = object<QApplication>(param_this);
    QEvent * arg1 = getqpointer<QEventType>(param_e);
    return isMuQtObject(arg0) ? ((MuQt_QApplication*)arg0)->event_pub_parent(arg1) : ((MuQt_QApplication*)arg0)->event_pub(arg1);
}

Pointer qt_QApplication_activeModalWidget_QWidget(Mu::Thread& NODE_THREAD)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    return makeinstance<QWidgetType>(c, QApplication::activeModalWidget(), "qt.QWidget");
}

Pointer qt_QApplication_activePopupWidget_QWidget(Mu::Thread& NODE_THREAD)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    return makeinstance<QWidgetType>(c, QApplication::activePopupWidget(), "qt.QWidget");
}

Pointer qt_QApplication_activeWindow_QWidget(Mu::Thread& NODE_THREAD)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    return makeinstance<QWidgetType>(c, QApplication::activeWindow(), "qt.QWidget");
}

void qt_QApplication_alert_void_QWidget_int(Mu::Thread& NODE_THREAD, Pointer param_widget, int param_msec)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QWidget * arg0 = object<QWidget>(param_widget);
    int arg1 = (int)(param_msec);
    QApplication::alert(arg0, arg1);
}

void qt_QApplication_beep_void(Mu::Thread& NODE_THREAD)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QApplication::beep();
}

int qt_QApplication_exec_int(Mu::Thread& NODE_THREAD)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    return QApplication::exec();
}

Pointer qt_QApplication_focusWidget_QWidget(Mu::Thread& NODE_THREAD)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    return makeinstance<QWidgetType>(c, QApplication::focusWidget(), "qt.QWidget");
}

Pointer qt_QApplication_font_QFont(Mu::Thread& NODE_THREAD)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    return makeqtype<QFontType>(c,QApplication::font(),"qt.QFont");
}

Pointer qt_QApplication_font_QFont_QWidget(Mu::Thread& NODE_THREAD, Pointer param_widget)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QWidget * arg0 = object<QWidget>(param_widget);
    return makeqtype<QFontType>(c,QApplication::font(arg0),"qt.QFont");
}

bool qt_QApplication_isEffectEnabled_bool_int(Mu::Thread& NODE_THREAD, int param_effect)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    Qt::UIEffect arg0 = (Qt::UIEffect)(param_effect);
    return QApplication::isEffectEnabled(arg0);
}

Pointer qt_QApplication_palette_QPalette_QWidget(Mu::Thread& NODE_THREAD, Pointer param_widget)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QWidget * arg0 = object<QWidget>(param_widget);
    return makeqtype<QPaletteType>(c,QApplication::palette(arg0),"qt.QPalette");
}

void qt_QApplication_setEffectEnabled_void_int_bool(Mu::Thread& NODE_THREAD, int param_effect, bool param_enable)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    Qt::UIEffect arg0 = (Qt::UIEffect)(param_effect);
    bool arg1 = (bool)(param_enable);
    QApplication::setEffectEnabled(arg0, arg1);
}

Pointer qt_QApplication_topLevelAt_QWidget_QPoint(Mu::Thread& NODE_THREAD, Pointer param_point)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QPoint  arg0 = getqtype<QPointType>(param_point);
    return makeinstance<QWidgetType>(c, QApplication::topLevelAt(arg0), "qt.QWidget");
}

Pointer qt_QApplication_topLevelAt_QWidget_int_int(Mu::Thread& NODE_THREAD, int param_x, int param_y)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    int arg0 = (int)(param_x);
    int arg1 = (int)(param_y);
    return makeinstance<QWidgetType>(c, QApplication::topLevelAt(arg0, arg1), "qt.QWidget");
}

Pointer qt_QApplication_widgetAt_QWidget_QPoint(Mu::Thread& NODE_THREAD, Pointer param_point)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QPoint  arg0 = getqtype<QPointType>(param_point);
    return makeinstance<QWidgetType>(c, QApplication::widgetAt(arg0), "qt.QWidget");
}

Pointer qt_QApplication_widgetAt_QWidget_int_int(Mu::Thread& NODE_THREAD, int param_x, int param_y)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    int arg0 = (int)(param_x);
    int arg1 = (int)(param_y);
    return makeinstance<QWidgetType>(c, QApplication::widgetAt(arg0, arg1), "qt.QWidget");
}


static NODE_IMPLEMENTATION(_n_notify0, bool)
{
    NODE_RETURN(qt_QApplication_notify_bool_QApplication_QObject_QEvent(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer), NODE_ARG(2, Pointer)));
}

static NODE_IMPLEMENTATION(_n_event0, bool)
{
    NODE_RETURN(qt_QApplication_event_bool_QApplication_QEvent(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}

static NODE_IMPLEMENTATION(_n_activeModalWidget0, Pointer)
{
    NODE_RETURN(qt_QApplication_activeModalWidget_QWidget(NODE_THREAD));
}

static NODE_IMPLEMENTATION(_n_activePopupWidget0, Pointer)
{
    NODE_RETURN(qt_QApplication_activePopupWidget_QWidget(NODE_THREAD));
}

static NODE_IMPLEMENTATION(_n_activeWindow0, Pointer)
{
    NODE_RETURN(qt_QApplication_activeWindow_QWidget(NODE_THREAD));
}

static NODE_IMPLEMENTATION(_n_alert0, void)
{
    qt_QApplication_alert_void_QWidget_int(NODE_THREAD, NODE_ARG(0, Pointer), NODE_ARG(1, int));
}

static NODE_IMPLEMENTATION(_n_beep0, void)
{
    qt_QApplication_beep_void(NODE_THREAD);
}

static NODE_IMPLEMENTATION(_n_exec0, int)
{
    NODE_RETURN(qt_QApplication_exec_int(NODE_THREAD));
}

static NODE_IMPLEMENTATION(_n_focusWidget0, Pointer)
{
    NODE_RETURN(qt_QApplication_focusWidget_QWidget(NODE_THREAD));
}

static NODE_IMPLEMENTATION(_n_font0, Pointer)
{
    NODE_RETURN(qt_QApplication_font_QFont(NODE_THREAD));
}

static NODE_IMPLEMENTATION(_n_font1, Pointer)
{
    NODE_RETURN(qt_QApplication_font_QFont_QWidget(NODE_THREAD, NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_isEffectEnabled0, bool)
{
    NODE_RETURN(qt_QApplication_isEffectEnabled_bool_int(NODE_THREAD, NODE_ARG(0, int)));
}

static NODE_IMPLEMENTATION(_n_palette0, Pointer)
{
    NODE_RETURN(qt_QApplication_palette_QPalette_QWidget(NODE_THREAD, NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_setEffectEnabled0, void)
{
    qt_QApplication_setEffectEnabled_void_int_bool(NODE_THREAD, NODE_ARG(0, int), NODE_ARG(1, bool));
}

static NODE_IMPLEMENTATION(_n_topLevelAt0, Pointer)
{
    NODE_RETURN(qt_QApplication_topLevelAt_QWidget_QPoint(NODE_THREAD, NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_topLevelAt1, Pointer)
{
    NODE_RETURN(qt_QApplication_topLevelAt_QWidget_int_int(NODE_THREAD, NODE_ARG(0, int), NODE_ARG(1, int)));
}

static NODE_IMPLEMENTATION(_n_widgetAt0, Pointer)
{
    NODE_RETURN(qt_QApplication_widgetAt_QWidget_QPoint(NODE_THREAD, NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_widgetAt1, Pointer)
{
    NODE_RETURN(qt_QApplication_widgetAt_QWidget_int_int(NODE_THREAD, NODE_ARG(0, int), NODE_ARG(1, int)));
}


//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

static char** global_argv = 0;

static NODE_IMPLEMENTATION(_n_QApplication0, Pointer)
{
    MuLangContext* c     = static_cast<MuLangContext*>(NODE_THREAD.context());
    ClassInstance* inst  = NODE_ARG_OBJECT(0, ClassInstance);
    DynamicArray*  args  = NODE_ARG_OBJECT(1, DynamicArray);

    int n = args->size();
    global_argv = new char* [n+1];
    global_argv[n] = 0;

    for (size_t i = 0; i < n; i++)
    {
        Mu::UTF8String utf8 = args->element<StringType::String*>(i)->utf8();
        global_argv[i] = strdup(utf8.c_str());
    }

    setobject(inst, new QApplication(n, global_argv));
    NODE_RETURN(inst);
}

void
QApplicationType::load()
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
                            Compiled, QApplication_QApplication_QObject,
                            Return, ftn,
                            Parameters,
                            new Param(c, "object", "qt.QObject"),
                            End),

               EndArguments );

addSymbols(
    // enums
    // member functions
    // MISSING: QApplication (QApplication; QApplication this, "int &" argc, "char * *" argv)
    // PROP: autoSipEnabled (bool; QApplication this)
    // PROP: styleSheet (string; QApplication this)
    _func[0] = new MemberFunction(c, "notify", _n_notify0, None, Compiled, qt_QApplication_notify_bool_QApplication_QObject_QEvent, Return, "bool", Parameters, new Param(c, "this", "qt.QApplication"), new Param(c, "receiver", "qt.QObject"), new Param(c, "e", "qt.QEvent"), End),
    _func[1] = new MemberFunction(c, "event", _n_event0, None, Compiled, qt_QApplication_event_bool_QApplication_QEvent, Return, "bool", Parameters, new Param(c, "this", "qt.QApplication"), new Param(c, "e", "qt.QEvent"), End),
    // static functions
    new Function(c, "activeModalWidget", _n_activeModalWidget0, None, Compiled, qt_QApplication_activeModalWidget_QWidget, Return, "qt.QWidget", End),
    new Function(c, "activePopupWidget", _n_activePopupWidget0, None, Compiled, qt_QApplication_activePopupWidget_QWidget, Return, "qt.QWidget", End),
    new Function(c, "activeWindow", _n_activeWindow0, None, Compiled, qt_QApplication_activeWindow_QWidget, Return, "qt.QWidget", End),
    new Function(c, "alert", _n_alert0, None, Compiled, qt_QApplication_alert_void_QWidget_int, Return, "void", Parameters, new Param(c, "widget", "qt.QWidget"), new Param(c, "msec", "int", Value((int)0)), End),
    // MISSING: allWidgets ("QWidgetList"; )
    new Function(c, "beep", _n_beep0, None, Compiled, qt_QApplication_beep_void, Return, "void", End),
    new Function(c, "exec", _n_exec0, None, Compiled, qt_QApplication_exec_int, Return, "int", End),
    new Function(c, "focusWidget", _n_focusWidget0, None, Compiled, qt_QApplication_focusWidget_QWidget, Return, "qt.QWidget", End),
    new Function(c, "font", _n_font0, None, Compiled, qt_QApplication_font_QFont, Return, "qt.QFont", End),
    new Function(c, "font", _n_font1, None, Compiled, qt_QApplication_font_QFont_QWidget, Return, "qt.QFont", Parameters, new Param(c, "widget", "qt.QWidget"), End),
    // MISSING: font (QFont; "const char *" className)
    new Function(c, "isEffectEnabled", _n_isEffectEnabled0, None, Compiled, qt_QApplication_isEffectEnabled_bool_int, Return, "bool", Parameters, new Param(c, "effect", "int"), End),
    new Function(c, "palette", _n_palette0, None, Compiled, qt_QApplication_palette_QPalette_QWidget, Return, "qt.QPalette", Parameters, new Param(c, "widget", "qt.QWidget"), End),
    // MISSING: palette (QPalette; "const char *" className)
    new Function(c, "setEffectEnabled", _n_setEffectEnabled0, None, Compiled, qt_QApplication_setEffectEnabled_void_int_bool, Return, "void", Parameters, new Param(c, "effect", "int"), new Param(c, "enable", "bool"), End),
    // MISSING: setFont (void; QFont font, "const char *" className)
    // MISSING: setPalette (void; QPalette palette, "const char *" className)
    // MISSING: setStyle (void; "QStyle *" style)
    // MISSING: setStyle ("QStyle *"; string style)
    // MISSING: style ("QStyle *"; )
    new Function(c, "topLevelAt", _n_topLevelAt0, None, Compiled, qt_QApplication_topLevelAt_QWidget_QPoint, Return, "qt.QWidget", Parameters, new Param(c, "point", "qt.QPoint"), End),
    new Function(c, "topLevelAt", _n_topLevelAt1, None, Compiled, qt_QApplication_topLevelAt_QWidget_int_int, Return, "qt.QWidget", Parameters, new Param(c, "x", "int"), new Param(c, "y", "int"), End),
    // MISSING: topLevelWidgets ("QWidgetList"; )
    new Function(c, "widgetAt", _n_widgetAt0, None, Compiled, qt_QApplication_widgetAt_QWidget_QPoint, Return, "qt.QWidget", Parameters, new Param(c, "point", "qt.QPoint"), End),
    new Function(c, "widgetAt", _n_widgetAt1, None, Compiled, qt_QApplication_widgetAt_QWidget_int_int, Return, "qt.QWidget", Parameters, new Param(c, "x", "int"), new Param(c, "y", "int"), End),
    EndArguments);
globalScope()->addSymbols(
    EndArguments);
scope()->addSymbols(
    EndArguments);

//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

c->arrayType(c->stringType(), 1, 0);

addSymbols(

           new Function(c, "QApplication", _n_QApplication0, None,
                        Return, "qt.QApplication",
                        Parameters,
                        new Param(c, "this", "qt.QApplication"),
                        new Param(c, "args", "string[]"),
                        End),
           
           EndArguments);

    const char** propExclusions = 0;

    populate(this, QApplication::staticMetaObject, propExclusions);
}

} // Mu
