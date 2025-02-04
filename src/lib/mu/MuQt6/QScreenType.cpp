//*****************************************************************************
// Copyright (c) 2024 Autodesk, Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//*****************************************************************************

// IMPORTANT: This file (not the template) is auto-generated by qt2mu.py script.
//            The prefered way to do a fix is to handrolled it or modify the
//            qt2mu.py script. If it is not possible, manual editing is ok but
//            it could be lost in future generations.

#include <MuQt6/qtUtils.h>
#include <MuQt6/QScreenType.h>
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
#include <MuQt6/QObjectType.h>
#include <MuQt6/QEventType.h>
#include <MuQt6/QRectType.h>
#include <MuQt6/QTransformType.h>
#include <MuQt6/QPointType.h>
#include <MuQt6/QTimerEventType.h>

namespace Mu
{
    using namespace std;

    //----------------------------------------------------------------------
    //  INHERITABLE TYPE IMPLEMENTATION

    // destructor
    MuQt_QScreen::~MuQt_QScreen()
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

    bool MuQt_QScreen::event(QEvent* e)
    {
        if (!_env)
            return QScreen::event(e);
        MuLangContext* c = (MuLangContext*)_env->context();
        const MemberFunction* F0 = _baseType->_func[0];
        const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
        if (F != F0)
        {
            Function::ArgumentVector args(2);
            args[0] = Value(Pointer(_obj));
            args[1] = Value(makeqpointer<QEventType>(c, e, "qt.QEvent"));
            Value rval = _env->call(F, args);
            return (bool)(rval._bool);
        }
        else
        {
            return QScreen::event(e);
        }
    }

    bool MuQt_QScreen::eventFilter(QObject* watched, QEvent* event)
    {
        if (!_env)
            return QScreen::eventFilter(watched, event);
        MuLangContext* c = (MuLangContext*)_env->context();
        const MemberFunction* F0 = _baseType->_func[1];
        const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
        if (F != F0)
        {
            Function::ArgumentVector args(3);
            args[0] = Value(Pointer(_obj));
            args[1] =
                Value(makeinstance<QObjectType>(c, watched, "qt.QObject"));
            args[2] = Value(makeqpointer<QEventType>(c, event, "qt.QEvent"));
            Value rval = _env->call(F, args);
            return (bool)(rval._bool);
        }
        else
        {
            return QScreen::eventFilter(watched, event);
        }
    }

    void MuQt_QScreen::customEvent(QEvent* event)
    {
        if (!_env)
        {
            QScreen::customEvent(event);
            return;
        }
        MuLangContext* c = (MuLangContext*)_env->context();
        const MemberFunction* F0 = _baseType->_func[2];
        const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
        if (F != F0)
        {
            Function::ArgumentVector args(2);
            args[0] = Value(Pointer(_obj));
            args[1] = Value(makeqpointer<QEventType>(c, event, "qt.QEvent"));
            Value rval = _env->call(F, args);
        }
        else
        {
            QScreen::customEvent(event);
        }
    }

    void MuQt_QScreen::timerEvent(QTimerEvent* event)
    {
        if (!_env)
        {
            QScreen::timerEvent(event);
            return;
        }
        MuLangContext* c = (MuLangContext*)_env->context();
        const MemberFunction* F0 = _baseType->_func[3];
        const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
        if (F != F0)
        {
            Function::ArgumentVector args(2);
            args[0] = Value(Pointer(_obj));
            args[1] = Value(
                makeqpointer<QTimerEventType>(c, event, "qt.QTimerEvent"));
            Value rval = _env->call(F, args);
        }
        else
        {
            QScreen::timerEvent(event);
        }
    }

    //----------------------------------------------------------------------
    //  Mu Type CONSTRUCTORS

    QScreenType::QScreenType(Context* c, const char* name, Class* super,
                             Class* super2)
        : Class(c, name, vectorOf2(super, super2))
    {
    }

    QScreenType::~QScreenType() {}

    //----------------------------------------------------------------------
    //  PRE-COMPILED FUNCTIONS

    static Pointer QScreen_QScreen_QObject(Thread& NODE_THREAD, Pointer obj)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        ClassInstance* widget = reinterpret_cast<ClassInstance*>(obj);

        if (!widget)
        {
            return 0;
        }
        else if (QScreen* w = object<QScreen>(widget))
        {
            QScreenType* type = c->findSymbolOfTypeByQualifiedName<QScreenType>(
                c->internName("qt.QScreen"), false);
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
        NODE_RETURN(QScreen_QScreen_QObject(NODE_THREAD, NODE_ARG(0, Pointer)));
    }

    int qt_QScreen_angleBetween_int_QScreen_int_int(Mu::Thread& NODE_THREAD,
                                                    Pointer param_this,
                                                    int param_a, int param_b)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QScreen* arg0 = object<QScreen>(param_this);
        Qt::ScreenOrientation arg1 = (Qt::ScreenOrientation)(param_a);
        Qt::ScreenOrientation arg2 = (Qt::ScreenOrientation)(param_b);
        return arg0->angleBetween(arg1, arg2);
    }

    Pointer qt_QScreen_availableGeometry_QRect_QScreen(Mu::Thread& NODE_THREAD,
                                                       Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QScreen* arg0 = object<QScreen>(param_this);
        return makeqtype<QRectType>(c, arg0->availableGeometry(), "qt.QRect");
    }

    bool qt_QScreen_isLandscape_bool_QScreen_int(Mu::Thread& NODE_THREAD,
                                                 Pointer param_this,
                                                 int param_o)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QScreen* arg0 = object<QScreen>(param_this);
        Qt::ScreenOrientation arg1 = (Qt::ScreenOrientation)(param_o);
        return arg0->isLandscape(arg1);
    }

    bool qt_QScreen_isPortrait_bool_QScreen_int(Mu::Thread& NODE_THREAD,
                                                Pointer param_this, int param_o)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QScreen* arg0 = object<QScreen>(param_this);
        Qt::ScreenOrientation arg1 = (Qt::ScreenOrientation)(param_o);
        return arg0->isPortrait(arg1);
    }

    Pointer qt_QScreen_mapBetween_QRect_QScreen_int_int_QRect(
        Mu::Thread& NODE_THREAD, Pointer param_this, int param_a, int param_b,
        Pointer param_rect)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QScreen* arg0 = object<QScreen>(param_this);
        Qt::ScreenOrientation arg1 = (Qt::ScreenOrientation)(param_a);
        Qt::ScreenOrientation arg2 = (Qt::ScreenOrientation)(param_b);
        const QRect arg3 = getqtype<QRectType>(param_rect);
        return makeqtype<QRectType>(c, arg0->mapBetween(arg1, arg2, arg3),
                                    "qt.QRect");
    }

    Pointer qt_QScreen_transformBetween_QTransform_QScreen_int_int_QRect(
        Mu::Thread& NODE_THREAD, Pointer param_this, int param_a, int param_b,
        Pointer param_target)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QScreen* arg0 = object<QScreen>(param_this);
        Qt::ScreenOrientation arg1 = (Qt::ScreenOrientation)(param_a);
        Qt::ScreenOrientation arg2 = (Qt::ScreenOrientation)(param_b);
        const QRect arg3 = getqtype<QRectType>(param_target);
        return makeqtype<QTransformType>(
            c, arg0->transformBetween(arg1, arg2, arg3), "qt.QTransform");
    }

    Pointer qt_QScreen_virtualGeometry_QRect_QScreen(Mu::Thread& NODE_THREAD,
                                                     Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QScreen* arg0 = object<QScreen>(param_this);
        return makeqtype<QRectType>(c, arg0->virtualGeometry(), "qt.QRect");
    }

    Pointer qt_QScreen_virtualSiblingAt_QScreen_QScreen_QPoint(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_point)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QScreen* arg0 = object<QScreen>(param_this);
        QPoint arg1 = getqtype<QPointType>(param_point);
        return makeinstance<QScreenType>(c, arg0->virtualSiblingAt(arg1),
                                         "qt.QScreen");
    }

    Pointer qt_QScreen_virtualSize_QSize_QScreen(Mu::Thread& NODE_THREAD,
                                                 Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QScreen* arg0 = object<QScreen>(param_this);
        return makeqtype<QSizeType>(c, arg0->virtualSize(), "qt.QSize");
    }

    bool qt_QScreen_event_bool_QScreen_QEvent(Mu::Thread& NODE_THREAD,
                                              Pointer param_this,
                                              Pointer param_e)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QScreen* arg0 = object<QScreen>(param_this);
        QEvent* arg1 = getqpointer<QEventType>(param_e);
        return isMuQtObject(arg0) ? arg0->QScreen::event(arg1)
                                  : arg0->event(arg1);
    }

    bool qt_QScreen_eventFilter_bool_QScreen_QObject_QEvent(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_watched,
        Pointer param_event)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QScreen* arg0 = object<QScreen>(param_this);
        QObject* arg1 = object<QObject>(param_watched);
        QEvent* arg2 = getqpointer<QEventType>(param_event);
        return isMuQtObject(arg0) ? arg0->QScreen::eventFilter(arg1, arg2)
                                  : arg0->eventFilter(arg1, arg2);
    }

    void qt_QScreen_customEvent_void_QScreen_QEvent(Mu::Thread& NODE_THREAD,
                                                    Pointer param_this,
                                                    Pointer param_event)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QScreen* arg0 = object<QScreen>(param_this);
        QEvent* arg1 = getqpointer<QEventType>(param_event);
        if (isMuQtObject(arg0))
            ((MuQt_QScreen*)arg0)->customEvent_pub_parent(arg1);
        else
            ((MuQt_QScreen*)arg0)->customEvent_pub(arg1);
    }

    void qt_QScreen_timerEvent_void_QScreen_QTimerEvent(Mu::Thread& NODE_THREAD,
                                                        Pointer param_this,
                                                        Pointer param_event)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QScreen* arg0 = object<QScreen>(param_this);
        QTimerEvent* arg1 = getqpointer<QTimerEventType>(param_event);
        if (isMuQtObject(arg0))
            ((MuQt_QScreen*)arg0)->timerEvent_pub_parent(arg1);
        else
            ((MuQt_QScreen*)arg0)->timerEvent_pub(arg1);
    }

    static NODE_IMPLEMENTATION(_n_angleBetween0, int)
    {
        NODE_RETURN(qt_QScreen_angleBetween_int_QScreen_int_int(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int),
            NODE_ARG(2, int)));
    }

    static NODE_IMPLEMENTATION(_n_availableGeometry0, Pointer)
    {
        NODE_RETURN(qt_QScreen_availableGeometry_QRect_QScreen(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_isLandscape0, bool)
    {
        NODE_RETURN(qt_QScreen_isLandscape_bool_QScreen_int(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int)));
    }

    static NODE_IMPLEMENTATION(_n_isPortrait0, bool)
    {
        NODE_RETURN(qt_QScreen_isPortrait_bool_QScreen_int(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int)));
    }

    static NODE_IMPLEMENTATION(_n_mapBetween0, Pointer)
    {
        NODE_RETURN(qt_QScreen_mapBetween_QRect_QScreen_int_int_QRect(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int),
            NODE_ARG(2, int), NODE_ARG(3, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_transformBetween0, Pointer)
    {
        NODE_RETURN(
            qt_QScreen_transformBetween_QTransform_QScreen_int_int_QRect(
                NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int),
                NODE_ARG(2, int), NODE_ARG(3, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_virtualGeometry0, Pointer)
    {
        NODE_RETURN(qt_QScreen_virtualGeometry_QRect_QScreen(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_virtualSiblingAt0, Pointer)
    {
        NODE_RETURN(qt_QScreen_virtualSiblingAt_QScreen_QScreen_QPoint(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_virtualSize0, Pointer)
    {
        NODE_RETURN(qt_QScreen_virtualSize_QSize_QScreen(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_event0, bool)
    {
        NODE_RETURN(qt_QScreen_event_bool_QScreen_QEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_eventFilter0, bool)
    {
        NODE_RETURN(qt_QScreen_eventFilter_bool_QScreen_QObject_QEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer),
            NODE_ARG(2, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_customEvent0, void)
    {
        qt_QScreen_customEvent_void_QScreen_QEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_timerEvent0, void)
    {
        qt_QScreen_timerEvent_void_QScreen_QTimerEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    void QScreenType::load()
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

                   new Function(c, tn, castFromObject, Cast, Compiled,
                                QScreen_QScreen_QObject, Return, ftn,
                                Parameters,
                                new Param(c, "object", "qt.QObject"), End),

                   EndArguments);

        addSymbols(
            // enums
            // member functions
            new Function(c, "angleBetween", _n_angleBetween0, None, Compiled,
                         qt_QScreen_angleBetween_int_QScreen_int_int, Return,
                         "int", Parameters, new Param(c, "this", "qt.QScreen"),
                         new Param(c, "a", "int"), new Param(c, "b", "int"),
                         End),
            new Function(c, "availableGeometry", _n_availableGeometry0, None,
                         Compiled, qt_QScreen_availableGeometry_QRect_QScreen,
                         Return, "qt.QRect", Parameters,
                         new Param(c, "this", "qt.QScreen"), End),
            // PROP: availableSize (QSize; QScreen this)
            // PROP: availableVirtualGeometry (QRect; QScreen this)
            // PROP: availableVirtualSize (QSize; QScreen this)
            // PROP: depth (int; QScreen this)
            // PROP: devicePixelRatio (double; QScreen this)
            // PROP: geometry (QRect; QScreen this)
            // MISSING: grabWindow (QPixmap; QScreen this, "WId" window, int x,
            // int y, int width, int height) MISSING: handle ("QPlatformScreen
            // *"; QScreen this)
            new Function(c, "isLandscape", _n_isLandscape0, None, Compiled,
                         qt_QScreen_isLandscape_bool_QScreen_int, Return,
                         "bool", Parameters, new Param(c, "this", "qt.QScreen"),
                         new Param(c, "o", "int"), End),
            new Function(c, "isPortrait", _n_isPortrait0, None, Compiled,
                         qt_QScreen_isPortrait_bool_QScreen_int, Return, "bool",
                         Parameters, new Param(c, "this", "qt.QScreen"),
                         new Param(c, "o", "int"), End),
            // PROP: logicalDotsPerInch (double; QScreen this)
            // PROP: logicalDotsPerInchX (double; QScreen this)
            // PROP: logicalDotsPerInchY (double; QScreen this)
            // PROP: manufacturer (string; QScreen this)
            new Function(c, "mapBetween", _n_mapBetween0, None, Compiled,
                         qt_QScreen_mapBetween_QRect_QScreen_int_int_QRect,
                         Return, "qt.QRect", Parameters,
                         new Param(c, "this", "qt.QScreen"),
                         new Param(c, "a", "int"), new Param(c, "b", "int"),
                         new Param(c, "rect", "qt.QRect"), End),
            // PROP: model (string; QScreen this)
            // PROP: name (string; QScreen this)
            // PROP: nativeOrientation (flags Qt::ScreenOrientation; QScreen
            // this) PROP: orientation (flags Qt::ScreenOrientation; QScreen
            // this) PROP: physicalDotsPerInch (double; QScreen this) PROP:
            // physicalDotsPerInchX (double; QScreen this) PROP:
            // physicalDotsPerInchY (double; QScreen this) MISSING: physicalSize
            // ("QSizeF"; QScreen this) PROP: primaryOrientation (flags
            // Qt::ScreenOrientation; QScreen this) PROP: refreshRate (double;
            // QScreen this) PROP: serialNumber (string; QScreen this) PROP:
            // size (QSize; QScreen this)
            new Function(
                c, "transformBetween", _n_transformBetween0, None, Compiled,
                qt_QScreen_transformBetween_QTransform_QScreen_int_int_QRect,
                Return, "qt.QTransform", Parameters,
                new Param(c, "this", "qt.QScreen"), new Param(c, "a", "int"),
                new Param(c, "b", "int"), new Param(c, "target", "qt.QRect"),
                End),
            new Function(c, "virtualGeometry", _n_virtualGeometry0, None,
                         Compiled, qt_QScreen_virtualGeometry_QRect_QScreen,
                         Return, "qt.QRect", Parameters,
                         new Param(c, "this", "qt.QScreen"), End),
            new Function(
                c, "virtualSiblingAt", _n_virtualSiblingAt0, None, Compiled,
                qt_QScreen_virtualSiblingAt_QScreen_QScreen_QPoint, Return,
                "qt.QScreen", Parameters, new Param(c, "this", "qt.QScreen"),
                new Param(c, "point", "qt.QPoint"), End),
            // MISSING: virtualSiblings ("QList<QScreen * >"; QScreen this)
            new Function(c, "virtualSize", _n_virtualSize0, None, Compiled,
                         qt_QScreen_virtualSize_QSize_QScreen, Return,
                         "qt.QSize", Parameters,
                         new Param(c, "this", "qt.QScreen"), End),
            _func[0] = new MemberFunction(c, "event", _n_event0, None, Compiled,
                                          qt_QScreen_event_bool_QScreen_QEvent,
                                          Return, "bool", Parameters,
                                          new Param(c, "this", "qt.QScreen"),
                                          new Param(c, "e", "qt.QEvent"), End),
            _func[1] = new MemberFunction(
                c, "eventFilter", _n_eventFilter0, None, Compiled,
                qt_QScreen_eventFilter_bool_QScreen_QObject_QEvent, Return,
                "bool", Parameters, new Param(c, "this", "qt.QScreen"),
                new Param(c, "watched", "qt.QObject"),
                new Param(c, "event", "qt.QEvent"), End),
            // MISSING: metaObject ("const QMetaObject *"; QScreen this)
            // MISSING: childEvent (void; QScreen this, "QChildEvent *" event)
            // // protected MISSING: connectNotify (void; QScreen this, "const
            // QMetaMethod &" signal) // protected
            _func[2] = new MemberFunction(
                c, "customEvent", _n_customEvent0, None, Compiled,
                qt_QScreen_customEvent_void_QScreen_QEvent, Return, "void",
                Parameters, new Param(c, "this", "qt.QScreen"),
                new Param(c, "event", "qt.QEvent"), End),
            // MISSING: disconnectNotify (void; QScreen this, "const QMetaMethod
            // &" signal) // protected
            _func[3] = new MemberFunction(
                c, "timerEvent", _n_timerEvent0, None, Compiled,
                qt_QScreen_timerEvent_void_QScreen_QTimerEvent, Return, "void",
                Parameters, new Param(c, "this", "qt.QScreen"),
                new Param(c, "event", "qt.QTimerEvent"), End),
            // static functions
            EndArguments);
        globalScope()->addSymbols(EndArguments);
        scope()->addSymbols(EndArguments);

        const char* propExclusions[] = {"virtualGeometry", "virtualSize", 0};

        populate(this, QScreen::staticMetaObject, propExclusions);
    }

} // namespace Mu
