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
#include <MuQt6/QDialType.h>
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
#include <MuQt6/QEventType.h>
#include <MuQt6/QMouseEventType.h>
#include <MuQt6/QPaintEventType.h>
#include <MuQt6/QKeyEventType.h>
#include <MuQt6/QResizeEventType.h>
#include <MuQt6/QTimerEventType.h>
#include <MuQt6/QWheelEventType.h>

namespace Mu
{
    using namespace std;

    //----------------------------------------------------------------------
    //  INHERITABLE TYPE IMPLEMENTATION

    // destructor
    MuQt_QDial::~MuQt_QDial()
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

    MuQt_QDial::MuQt_QDial(Pointer muobj, const CallEnvironment* ce,
                           QWidget* parent)
        : QDial(parent)
    {
        _env = ce;
        _obj = reinterpret_cast<ClassInstance*>(muobj);
        _obj->retainExternal();
        MuLangContext* c = (MuLangContext*)_env->context();
        _baseType = c->findSymbolOfTypeByQualifiedName<QDialType>(
            c->internName("qt.QDial"));
    }

    QSize MuQt_QDial::minimumSizeHint() const
    {
        if (!_env)
            return QDial::minimumSizeHint();
        MuLangContext* c = (MuLangContext*)_env->context();
        const MemberFunction* F0 = _baseType->_func[0];
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
            return QDial::minimumSizeHint();
        }
    }

    QSize MuQt_QDial::sizeHint() const
    {
        if (!_env)
            return QDial::sizeHint();
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
            return QDial::sizeHint();
        }
    }

    bool MuQt_QDial::event(QEvent* e)
    {
        if (!_env)
            return QDial::event(e);
        MuLangContext* c = (MuLangContext*)_env->context();
        const MemberFunction* F0 = _baseType->_func[2];
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
            return QDial::event(e);
        }
    }

    void MuQt_QDial::mouseMoveEvent(QMouseEvent* e)
    {
        if (!_env)
        {
            QDial::mouseMoveEvent(e);
            return;
        }
        MuLangContext* c = (MuLangContext*)_env->context();
        const MemberFunction* F0 = _baseType->_func[3];
        const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
        if (F != F0)
        {
            Function::ArgumentVector args(2);
            args[0] = Value(Pointer(_obj));
            args[1] =
                Value(makeqpointer<QMouseEventType>(c, e, "qt.QMouseEvent"));
            Value rval = _env->call(F, args);
        }
        else
        {
            QDial::mouseMoveEvent(e);
        }
    }

    void MuQt_QDial::mousePressEvent(QMouseEvent* e)
    {
        if (!_env)
        {
            QDial::mousePressEvent(e);
            return;
        }
        MuLangContext* c = (MuLangContext*)_env->context();
        const MemberFunction* F0 = _baseType->_func[4];
        const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
        if (F != F0)
        {
            Function::ArgumentVector args(2);
            args[0] = Value(Pointer(_obj));
            args[1] =
                Value(makeqpointer<QMouseEventType>(c, e, "qt.QMouseEvent"));
            Value rval = _env->call(F, args);
        }
        else
        {
            QDial::mousePressEvent(e);
        }
    }

    void MuQt_QDial::mouseReleaseEvent(QMouseEvent* e)
    {
        if (!_env)
        {
            QDial::mouseReleaseEvent(e);
            return;
        }
        MuLangContext* c = (MuLangContext*)_env->context();
        const MemberFunction* F0 = _baseType->_func[5];
        const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
        if (F != F0)
        {
            Function::ArgumentVector args(2);
            args[0] = Value(Pointer(_obj));
            args[1] =
                Value(makeqpointer<QMouseEventType>(c, e, "qt.QMouseEvent"));
            Value rval = _env->call(F, args);
        }
        else
        {
            QDial::mouseReleaseEvent(e);
        }
    }

    void MuQt_QDial::paintEvent(QPaintEvent* pe)
    {
        if (!_env)
        {
            QDial::paintEvent(pe);
            return;
        }
        MuLangContext* c = (MuLangContext*)_env->context();
        const MemberFunction* F0 = _baseType->_func[6];
        const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
        if (F != F0)
        {
            Function::ArgumentVector args(2);
            args[0] = Value(Pointer(_obj));
            args[1] =
                Value(makeqpointer<QPaintEventType>(c, pe, "qt.QPaintEvent"));
            Value rval = _env->call(F, args);
        }
        else
        {
            QDial::paintEvent(pe);
        }
    }

    void MuQt_QDial::resizeEvent(QResizeEvent* e)
    {
        if (!_env)
        {
            QDial::resizeEvent(e);
            return;
        }
        MuLangContext* c = (MuLangContext*)_env->context();
        const MemberFunction* F0 = _baseType->_func[7];
        const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
        if (F != F0)
        {
            Function::ArgumentVector args(2);
            args[0] = Value(Pointer(_obj));
            args[1] =
                Value(makeqpointer<QResizeEventType>(c, e, "qt.QResizeEvent"));
            Value rval = _env->call(F, args);
        }
        else
        {
            QDial::resizeEvent(e);
        }
    }

    void MuQt_QDial::changeEvent(QEvent* ev)
    {
        if (!_env)
        {
            QDial::changeEvent(ev);
            return;
        }
        MuLangContext* c = (MuLangContext*)_env->context();
        const MemberFunction* F0 = _baseType->_func[8];
        const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
        if (F != F0)
        {
            Function::ArgumentVector args(2);
            args[0] = Value(Pointer(_obj));
            args[1] = Value(makeqpointer<QEventType>(c, ev, "qt.QEvent"));
            Value rval = _env->call(F, args);
        }
        else
        {
            QDial::changeEvent(ev);
        }
    }

    void MuQt_QDial::keyPressEvent(QKeyEvent* ev)
    {
        if (!_env)
        {
            QDial::keyPressEvent(ev);
            return;
        }
        MuLangContext* c = (MuLangContext*)_env->context();
        const MemberFunction* F0 = _baseType->_func[9];
        const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
        if (F != F0)
        {
            Function::ArgumentVector args(2);
            args[0] = Value(Pointer(_obj));
            args[1] = Value(makeqpointer<QKeyEventType>(c, ev, "qt.QKeyEvent"));
            Value rval = _env->call(F, args);
        }
        else
        {
            QDial::keyPressEvent(ev);
        }
    }

    void MuQt_QDial::timerEvent(QTimerEvent* e)
    {
        if (!_env)
        {
            QDial::timerEvent(e);
            return;
        }
        MuLangContext* c = (MuLangContext*)_env->context();
        const MemberFunction* F0 = _baseType->_func[10];
        const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
        if (F != F0)
        {
            Function::ArgumentVector args(2);
            args[0] = Value(Pointer(_obj));
            args[1] =
                Value(makeqpointer<QTimerEventType>(c, e, "qt.QTimerEvent"));
            Value rval = _env->call(F, args);
        }
        else
        {
            QDial::timerEvent(e);
        }
    }

    void MuQt_QDial::wheelEvent(QWheelEvent* e)
    {
        if (!_env)
        {
            QDial::wheelEvent(e);
            return;
        }
        MuLangContext* c = (MuLangContext*)_env->context();
        const MemberFunction* F0 = _baseType->_func[11];
        const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
        if (F != F0)
        {
            Function::ArgumentVector args(2);
            args[0] = Value(Pointer(_obj));
            args[1] =
                Value(makeqpointer<QWheelEventType>(c, e, "qt.QWheelEvent"));
            Value rval = _env->call(F, args);
        }
        else
        {
            QDial::wheelEvent(e);
        }
    }

    //----------------------------------------------------------------------
    //  Mu Type CONSTRUCTORS

    QDialType::QDialType(Context* c, const char* name, Class* super,
                         Class* super2)
        : Class(c, name, vectorOf2(super, super2))
    {
    }

    QDialType::~QDialType() {}

    //----------------------------------------------------------------------
    //  PRE-COMPILED FUNCTIONS

    static Pointer QDial_QDial_QObject(Thread& NODE_THREAD, Pointer obj)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        ClassInstance* widget = reinterpret_cast<ClassInstance*>(obj);

        if (!widget)
        {
            return 0;
        }
        else if (QDial* w = object<QDial>(widget))
        {
            QDialType* type = c->findSymbolOfTypeByQualifiedName<QDialType>(
                c->internName("qt.QDial"), false);
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
        NODE_RETURN(QDial_QDial_QObject(NODE_THREAD, NODE_ARG(0, Pointer)));
    }

    Pointer qt_QDial_QDial_QDial_QDial_QWidget(Mu::Thread& NODE_THREAD,
                                               Pointer param_this,
                                               Pointer param_parent)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QWidget* arg1 = object<QWidget>(param_parent);
        setobject(
            param_this,
            new MuQt_QDial(param_this, NODE_THREAD.process()->callEnv(), arg1));
        return param_this;
    }

    Pointer qt_QDial_minimumSizeHint_QSize_QDial(Mu::Thread& NODE_THREAD,
                                                 Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QDial* arg0 = object<QDial>(param_this);
        return isMuQtObject(arg0)
                   ? makeqtype<QSizeType>(c, arg0->QDial::minimumSizeHint(),
                                          "qt.QSize")
                   : makeqtype<QSizeType>(c, arg0->minimumSizeHint(),
                                          "qt.QSize");
    }

    Pointer qt_QDial_sizeHint_QSize_QDial(Mu::Thread& NODE_THREAD,
                                          Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QDial* arg0 = object<QDial>(param_this);
        return isMuQtObject(arg0)
                   ? makeqtype<QSizeType>(c, arg0->QDial::sizeHint(),
                                          "qt.QSize")
                   : makeqtype<QSizeType>(c, arg0->sizeHint(), "qt.QSize");
    }

    bool qt_QDial_event_bool_QDial_QEvent(Mu::Thread& NODE_THREAD,
                                          Pointer param_this, Pointer param_e)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QDial* arg0 = object<QDial>(param_this);
        QEvent* arg1 = getqpointer<QEventType>(param_e);
        return isMuQtObject(arg0) ? ((MuQt_QDial*)arg0)->event_pub_parent(arg1)
                                  : ((MuQt_QDial*)arg0)->event_pub(arg1);
    }

    void qt_QDial_mouseMoveEvent_void_QDial_QMouseEvent(Mu::Thread& NODE_THREAD,
                                                        Pointer param_this,
                                                        Pointer param_e)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QDial* arg0 = object<QDial>(param_this);
        QMouseEvent* arg1 = getqpointer<QMouseEventType>(param_e);
        if (isMuQtObject(arg0))
            ((MuQt_QDial*)arg0)->mouseMoveEvent_pub_parent(arg1);
        else
            ((MuQt_QDial*)arg0)->mouseMoveEvent_pub(arg1);
    }

    void qt_QDial_mousePressEvent_void_QDial_QMouseEvent(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_e)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QDial* arg0 = object<QDial>(param_this);
        QMouseEvent* arg1 = getqpointer<QMouseEventType>(param_e);
        if (isMuQtObject(arg0))
            ((MuQt_QDial*)arg0)->mousePressEvent_pub_parent(arg1);
        else
            ((MuQt_QDial*)arg0)->mousePressEvent_pub(arg1);
    }

    void qt_QDial_mouseReleaseEvent_void_QDial_QMouseEvent(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_e)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QDial* arg0 = object<QDial>(param_this);
        QMouseEvent* arg1 = getqpointer<QMouseEventType>(param_e);
        if (isMuQtObject(arg0))
            ((MuQt_QDial*)arg0)->mouseReleaseEvent_pub_parent(arg1);
        else
            ((MuQt_QDial*)arg0)->mouseReleaseEvent_pub(arg1);
    }

    void qt_QDial_paintEvent_void_QDial_QPaintEvent(Mu::Thread& NODE_THREAD,
                                                    Pointer param_this,
                                                    Pointer param_pe)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QDial* arg0 = object<QDial>(param_this);
        QPaintEvent* arg1 = getqpointer<QPaintEventType>(param_pe);
        if (isMuQtObject(arg0))
            ((MuQt_QDial*)arg0)->paintEvent_pub_parent(arg1);
        else
            ((MuQt_QDial*)arg0)->paintEvent_pub(arg1);
    }

    void qt_QDial_resizeEvent_void_QDial_QResizeEvent(Mu::Thread& NODE_THREAD,
                                                      Pointer param_this,
                                                      Pointer param_e)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QDial* arg0 = object<QDial>(param_this);
        QResizeEvent* arg1 = getqpointer<QResizeEventType>(param_e);
        if (isMuQtObject(arg0))
            ((MuQt_QDial*)arg0)->resizeEvent_pub_parent(arg1);
        else
            ((MuQt_QDial*)arg0)->resizeEvent_pub(arg1);
    }

    void qt_QDial_changeEvent_void_QDial_QEvent(Mu::Thread& NODE_THREAD,
                                                Pointer param_this,
                                                Pointer param_ev)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QDial* arg0 = object<QDial>(param_this);
        QEvent* arg1 = getqpointer<QEventType>(param_ev);
        if (isMuQtObject(arg0))
            ((MuQt_QDial*)arg0)->changeEvent_pub_parent(arg1);
        else
            ((MuQt_QDial*)arg0)->changeEvent_pub(arg1);
    }

    void qt_QDial_keyPressEvent_void_QDial_QKeyEvent(Mu::Thread& NODE_THREAD,
                                                     Pointer param_this,
                                                     Pointer param_ev)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QDial* arg0 = object<QDial>(param_this);
        QKeyEvent* arg1 = getqpointer<QKeyEventType>(param_ev);
        if (isMuQtObject(arg0))
            ((MuQt_QDial*)arg0)->keyPressEvent_pub_parent(arg1);
        else
            ((MuQt_QDial*)arg0)->keyPressEvent_pub(arg1);
    }

    void qt_QDial_timerEvent_void_QDial_QTimerEvent(Mu::Thread& NODE_THREAD,
                                                    Pointer param_this,
                                                    Pointer param_e)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QDial* arg0 = object<QDial>(param_this);
        QTimerEvent* arg1 = getqpointer<QTimerEventType>(param_e);
        if (isMuQtObject(arg0))
            ((MuQt_QDial*)arg0)->timerEvent_pub_parent(arg1);
        else
            ((MuQt_QDial*)arg0)->timerEvent_pub(arg1);
    }

    void qt_QDial_wheelEvent_void_QDial_QWheelEvent(Mu::Thread& NODE_THREAD,
                                                    Pointer param_this,
                                                    Pointer param_e)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QDial* arg0 = object<QDial>(param_this);
        QWheelEvent* arg1 = getqpointer<QWheelEventType>(param_e);
        if (isMuQtObject(arg0))
            ((MuQt_QDial*)arg0)->wheelEvent_pub_parent(arg1);
        else
            ((MuQt_QDial*)arg0)->wheelEvent_pub(arg1);
    }

    static NODE_IMPLEMENTATION(_n_QDial0, Pointer)
    {
        NODE_RETURN(qt_QDial_QDial_QDial_QDial_QWidget(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_minimumSizeHint0, Pointer)
    {
        NODE_RETURN(qt_QDial_minimumSizeHint_QSize_QDial(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_sizeHint0, Pointer)
    {
        NODE_RETURN(qt_QDial_sizeHint_QSize_QDial(NODE_THREAD,
                                                  NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_event0, bool)
    {
        NODE_RETURN(qt_QDial_event_bool_QDial_QEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_mouseMoveEvent0, void)
    {
        qt_QDial_mouseMoveEvent_void_QDial_QMouseEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_mousePressEvent0, void)
    {
        qt_QDial_mousePressEvent_void_QDial_QMouseEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_mouseReleaseEvent0, void)
    {
        qt_QDial_mouseReleaseEvent_void_QDial_QMouseEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_paintEvent0, void)
    {
        qt_QDial_paintEvent_void_QDial_QPaintEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_resizeEvent0, void)
    {
        qt_QDial_resizeEvent_void_QDial_QResizeEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_changeEvent0, void)
    {
        qt_QDial_changeEvent_void_QDial_QEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_keyPressEvent0, void)
    {
        qt_QDial_keyPressEvent_void_QDial_QKeyEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_timerEvent0, void)
    {
        qt_QDial_timerEvent_void_QDial_QTimerEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_wheelEvent0, void)
    {
        qt_QDial_wheelEvent_void_QDial_QWheelEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    void QDialType::load()
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
                                QDial_QDial_QObject, Return, ftn, Parameters,
                                new Param(c, "object", "qt.QObject"), End),

                   EndArguments);

        addSymbols(
            // enums
            // member functions
            new Function(c, "QDial", _n_QDial0, None, Compiled,
                         qt_QDial_QDial_QDial_QDial_QWidget, Return, "qt.QDial",
                         Parameters, new Param(c, "this", "qt.QDial"),
                         new Param(c, "parent", "qt.QWidget"), End),
            // PROP: notchSize (int; QDial this)
            // PROP: notchTarget (double; QDial this)
            // PROP: notchesVisible (bool; QDial this)
            // PROP: setNotchTarget (void; QDial this, double target)
            // PROP: wrapping (bool; QDial this)
            _func[0] = new MemberFunction(
                c, "minimumSizeHint", _n_minimumSizeHint0, None, Compiled,
                qt_QDial_minimumSizeHint_QSize_QDial, Return, "qt.QSize",
                Parameters, new Param(c, "this", "qt.QDial"), End),
            _func[1] = new MemberFunction(
                c, "sizeHint", _n_sizeHint0, None, Compiled,
                qt_QDial_sizeHint_QSize_QDial, Return, "qt.QSize", Parameters,
                new Param(c, "this", "qt.QDial"), End),
            // MISSING: initStyleOption (void; QDial this, "QStyleOptionSlider
            // *" option) // protected
            _func[2] = new MemberFunction(c, "event", _n_event0, None, Compiled,
                                          qt_QDial_event_bool_QDial_QEvent,
                                          Return, "bool", Parameters,
                                          new Param(c, "this", "qt.QDial"),
                                          new Param(c, "e", "qt.QEvent"), End),
            _func[3] = new MemberFunction(
                c, "mouseMoveEvent", _n_mouseMoveEvent0, None, Compiled,
                qt_QDial_mouseMoveEvent_void_QDial_QMouseEvent, Return, "void",
                Parameters, new Param(c, "this", "qt.QDial"),
                new Param(c, "e", "qt.QMouseEvent"), End),
            _func[4] = new MemberFunction(
                c, "mousePressEvent", _n_mousePressEvent0, None, Compiled,
                qt_QDial_mousePressEvent_void_QDial_QMouseEvent, Return, "void",
                Parameters, new Param(c, "this", "qt.QDial"),
                new Param(c, "e", "qt.QMouseEvent"), End),
            _func[5] = new MemberFunction(
                c, "mouseReleaseEvent", _n_mouseReleaseEvent0, None, Compiled,
                qt_QDial_mouseReleaseEvent_void_QDial_QMouseEvent, Return,
                "void", Parameters, new Param(c, "this", "qt.QDial"),
                new Param(c, "e", "qt.QMouseEvent"), End),
            _func[6] = new MemberFunction(
                c, "paintEvent", _n_paintEvent0, None, Compiled,
                qt_QDial_paintEvent_void_QDial_QPaintEvent, Return, "void",
                Parameters, new Param(c, "this", "qt.QDial"),
                new Param(c, "pe", "qt.QPaintEvent"), End),
            _func[7] = new MemberFunction(
                c, "resizeEvent", _n_resizeEvent0, None, Compiled,
                qt_QDial_resizeEvent_void_QDial_QResizeEvent, Return, "void",
                Parameters, new Param(c, "this", "qt.QDial"),
                new Param(c, "e", "qt.QResizeEvent"), End),
            _func[8] = new MemberFunction(
                c, "changeEvent", _n_changeEvent0, None, Compiled,
                qt_QDial_changeEvent_void_QDial_QEvent, Return, "void",
                Parameters, new Param(c, "this", "qt.QDial"),
                new Param(c, "ev", "qt.QEvent"), End),
            _func[9] = new MemberFunction(
                c, "keyPressEvent", _n_keyPressEvent0, None, Compiled,
                qt_QDial_keyPressEvent_void_QDial_QKeyEvent, Return, "void",
                Parameters, new Param(c, "this", "qt.QDial"),
                new Param(c, "ev", "qt.QKeyEvent"), End),
            _func[10] = new MemberFunction(
                c, "timerEvent", _n_timerEvent0, None, Compiled,
                qt_QDial_timerEvent_void_QDial_QTimerEvent, Return, "void",
                Parameters, new Param(c, "this", "qt.QDial"),
                new Param(c, "e", "qt.QTimerEvent"), End),
            _func[11] = new MemberFunction(
                c, "wheelEvent", _n_wheelEvent0, None, Compiled,
                qt_QDial_wheelEvent_void_QDial_QWheelEvent, Return, "void",
                Parameters, new Param(c, "this", "qt.QDial"),
                new Param(c, "e", "qt.QWheelEvent"), End),
            // static functions
            EndArguments);
        globalScope()->addSymbols(EndArguments);
        scope()->addSymbols(EndArguments);

        const char** propExclusions = 0;

        populate(this, QDial::staticMetaObject, propExclusions);
    }

} // namespace Mu
