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
#include <MuQt6/QToolButtonType.h>
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
#include <MuQt6/QFocusEventType.h>
#include <MuQt6/QKeyEventType.h>
#include <MuQt6/QMenuType.h>
#include <MuQt6/QPointType.h>
#include <MuQt6/QActionType.h>
#include <MuQt6/QTimerEventType.h>

namespace Mu
{
    using namespace std;

    //----------------------------------------------------------------------
    //  INHERITABLE TYPE IMPLEMENTATION

    // destructor
    MuQt_QToolButton::~MuQt_QToolButton()
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

    MuQt_QToolButton::MuQt_QToolButton(Pointer muobj, const CallEnvironment* ce,
                                       QWidget* parent)
        : QToolButton(parent)
    {
        _env = ce;
        _obj = reinterpret_cast<ClassInstance*>(muobj);
        _obj->retainExternal();
        MuLangContext* c = (MuLangContext*)_env->context();
        _baseType = c->findSymbolOfTypeByQualifiedName<QToolButtonType>(
            c->internName("qt.QToolButton"));
    }

    QSize MuQt_QToolButton::minimumSizeHint() const
    {
        if (!_env)
            return QToolButton::minimumSizeHint();
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
            return QToolButton::minimumSizeHint();
        }
    }

    QSize MuQt_QToolButton::sizeHint() const
    {
        if (!_env)
            return QToolButton::sizeHint();
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
            return QToolButton::sizeHint();
        }
    }

    void MuQt_QToolButton::changeEvent(QEvent* e)
    {
        if (!_env)
        {
            QToolButton::changeEvent(e);
            return;
        }
        MuLangContext* c = (MuLangContext*)_env->context();
        const MemberFunction* F0 = _baseType->_func[2];
        const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
        if (F != F0)
        {
            Function::ArgumentVector args(2);
            args[0] = Value(Pointer(_obj));
            args[1] = Value(makeqpointer<QEventType>(c, e, "qt.QEvent"));
            Value rval = _env->call(F, args);
        }
        else
        {
            QToolButton::changeEvent(e);
        }
    }

    void MuQt_QToolButton::checkStateSet()
    {
        if (!_env)
        {
            QToolButton::checkStateSet();
            return;
        }
        MuLangContext* c = (MuLangContext*)_env->context();
        const MemberFunction* F0 = _baseType->_func[3];
        const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
        if (F != F0)
        {
            Function::ArgumentVector args(1);
            args[0] = Value(Pointer(_obj));
            Value rval = _env->call(F, args);
        }
        else
        {
            QToolButton::checkStateSet();
        }
    }

    bool MuQt_QToolButton::event(QEvent* event_)
    {
        if (!_env)
            return QToolButton::event(event_);
        MuLangContext* c = (MuLangContext*)_env->context();
        const MemberFunction* F0 = _baseType->_func[4];
        const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
        if (F != F0)
        {
            Function::ArgumentVector args(2);
            args[0] = Value(Pointer(_obj));
            args[1] = Value(makeqpointer<QEventType>(c, event_, "qt.QEvent"));
            Value rval = _env->call(F, args);
            return (bool)(rval._bool);
        }
        else
        {
            return QToolButton::event(event_);
        }
    }

    bool MuQt_QToolButton::hitButton(const QPoint& pos) const
    {
        if (!_env)
            return QToolButton::hitButton(pos);
        MuLangContext* c = (MuLangContext*)_env->context();
        const MemberFunction* F0 = _baseType->_func[5];
        const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
        if (F != F0)
        {
            Function::ArgumentVector args(2);
            args[0] = Value(Pointer(_obj));
            args[1] = Value(makeqtype<QPointType>(c, pos, "qt.QPoint"));
            Value rval = _env->call(F, args);
            return (bool)(rval._bool);
        }
        else
        {
            return QToolButton::hitButton(pos);
        }
    }

    void MuQt_QToolButton::leaveEvent(QEvent* e)
    {
        if (!_env)
        {
            QToolButton::leaveEvent(e);
            return;
        }
        MuLangContext* c = (MuLangContext*)_env->context();
        const MemberFunction* F0 = _baseType->_func[6];
        const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
        if (F != F0)
        {
            Function::ArgumentVector args(2);
            args[0] = Value(Pointer(_obj));
            args[1] = Value(makeqpointer<QEventType>(c, e, "qt.QEvent"));
            Value rval = _env->call(F, args);
        }
        else
        {
            QToolButton::leaveEvent(e);
        }
    }

    void MuQt_QToolButton::mousePressEvent(QMouseEvent* e)
    {
        if (!_env)
        {
            QToolButton::mousePressEvent(e);
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
                Value(makeqpointer<QMouseEventType>(c, e, "qt.QMouseEvent"));
            Value rval = _env->call(F, args);
        }
        else
        {
            QToolButton::mousePressEvent(e);
        }
    }

    void MuQt_QToolButton::mouseReleaseEvent(QMouseEvent* e)
    {
        if (!_env)
        {
            QToolButton::mouseReleaseEvent(e);
            return;
        }
        MuLangContext* c = (MuLangContext*)_env->context();
        const MemberFunction* F0 = _baseType->_func[8];
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
            QToolButton::mouseReleaseEvent(e);
        }
    }

    void MuQt_QToolButton::nextCheckState()
    {
        if (!_env)
        {
            QToolButton::nextCheckState();
            return;
        }
        MuLangContext* c = (MuLangContext*)_env->context();
        const MemberFunction* F0 = _baseType->_func[9];
        const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
        if (F != F0)
        {
            Function::ArgumentVector args(1);
            args[0] = Value(Pointer(_obj));
            Value rval = _env->call(F, args);
        }
        else
        {
            QToolButton::nextCheckState();
        }
    }

    void MuQt_QToolButton::paintEvent(QPaintEvent* event)
    {
        if (!_env)
        {
            QToolButton::paintEvent(event);
            return;
        }
        MuLangContext* c = (MuLangContext*)_env->context();
        const MemberFunction* F0 = _baseType->_func[10];
        const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
        if (F != F0)
        {
            Function::ArgumentVector args(2);
            args[0] = Value(Pointer(_obj));
            args[1] = Value(
                makeqpointer<QPaintEventType>(c, event, "qt.QPaintEvent"));
            Value rval = _env->call(F, args);
        }
        else
        {
            QToolButton::paintEvent(event);
        }
    }

    void MuQt_QToolButton::timerEvent(QTimerEvent* e)
    {
        if (!_env)
        {
            QToolButton::timerEvent(e);
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
                Value(makeqpointer<QTimerEventType>(c, e, "qt.QTimerEvent"));
            Value rval = _env->call(F, args);
        }
        else
        {
            QToolButton::timerEvent(e);
        }
    }

    void MuQt_QToolButton::focusInEvent(QFocusEvent* e)
    {
        if (!_env)
        {
            QToolButton::focusInEvent(e);
            return;
        }
        MuLangContext* c = (MuLangContext*)_env->context();
        const MemberFunction* F0 = _baseType->_func[12];
        const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
        if (F != F0)
        {
            Function::ArgumentVector args(2);
            args[0] = Value(Pointer(_obj));
            args[1] =
                Value(makeqpointer<QFocusEventType>(c, e, "qt.QFocusEvent"));
            Value rval = _env->call(F, args);
        }
        else
        {
            QToolButton::focusInEvent(e);
        }
    }

    void MuQt_QToolButton::focusOutEvent(QFocusEvent* e)
    {
        if (!_env)
        {
            QToolButton::focusOutEvent(e);
            return;
        }
        MuLangContext* c = (MuLangContext*)_env->context();
        const MemberFunction* F0 = _baseType->_func[13];
        const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
        if (F != F0)
        {
            Function::ArgumentVector args(2);
            args[0] = Value(Pointer(_obj));
            args[1] =
                Value(makeqpointer<QFocusEventType>(c, e, "qt.QFocusEvent"));
            Value rval = _env->call(F, args);
        }
        else
        {
            QToolButton::focusOutEvent(e);
        }
    }

    void MuQt_QToolButton::keyPressEvent(QKeyEvent* e)
    {
        if (!_env)
        {
            QToolButton::keyPressEvent(e);
            return;
        }
        MuLangContext* c = (MuLangContext*)_env->context();
        const MemberFunction* F0 = _baseType->_func[14];
        const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
        if (F != F0)
        {
            Function::ArgumentVector args(2);
            args[0] = Value(Pointer(_obj));
            args[1] = Value(makeqpointer<QKeyEventType>(c, e, "qt.QKeyEvent"));
            Value rval = _env->call(F, args);
        }
        else
        {
            QToolButton::keyPressEvent(e);
        }
    }

    void MuQt_QToolButton::keyReleaseEvent(QKeyEvent* e)
    {
        if (!_env)
        {
            QToolButton::keyReleaseEvent(e);
            return;
        }
        MuLangContext* c = (MuLangContext*)_env->context();
        const MemberFunction* F0 = _baseType->_func[15];
        const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
        if (F != F0)
        {
            Function::ArgumentVector args(2);
            args[0] = Value(Pointer(_obj));
            args[1] = Value(makeqpointer<QKeyEventType>(c, e, "qt.QKeyEvent"));
            Value rval = _env->call(F, args);
        }
        else
        {
            QToolButton::keyReleaseEvent(e);
        }
    }

    void MuQt_QToolButton::mouseMoveEvent(QMouseEvent* e)
    {
        if (!_env)
        {
            QToolButton::mouseMoveEvent(e);
            return;
        }
        MuLangContext* c = (MuLangContext*)_env->context();
        const MemberFunction* F0 = _baseType->_func[16];
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
            QToolButton::mouseMoveEvent(e);
        }
    }

    //----------------------------------------------------------------------
    //  Mu Type CONSTRUCTORS

    QToolButtonType::QToolButtonType(Context* c, const char* name, Class* super,
                                     Class* super2)
        : Class(c, name, vectorOf2(super, super2))
    {
    }

    QToolButtonType::~QToolButtonType() {}

    //----------------------------------------------------------------------
    //  PRE-COMPILED FUNCTIONS

    static Pointer QToolButton_QToolButton_QObject(Thread& NODE_THREAD,
                                                   Pointer obj)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        ClassInstance* widget = reinterpret_cast<ClassInstance*>(obj);

        if (!widget)
        {
            return 0;
        }
        else if (QToolButton* w = object<QToolButton>(widget))
        {
            QToolButtonType* type =
                c->findSymbolOfTypeByQualifiedName<QToolButtonType>(
                    c->internName("qt.QToolButton"), false);
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
        NODE_RETURN(
            QToolButton_QToolButton_QObject(NODE_THREAD, NODE_ARG(0, Pointer)));
    }

    Pointer qt_QToolButton_QToolButton_QToolButton_QToolButton_QWidget(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_parent)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QWidget* arg1 = object<QWidget>(param_parent);
        setobject(param_this,
                  new MuQt_QToolButton(param_this,
                                       NODE_THREAD.process()->callEnv(), arg1));
        return param_this;
    }

    Pointer
    qt_QToolButton_defaultAction_QAction_QToolButton(Mu::Thread& NODE_THREAD,
                                                     Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QToolButton* arg0 = object<QToolButton>(param_this);
        return makeinstance<QActionType>(c, arg0->defaultAction(),
                                         "qt.QAction");
    }

    Pointer qt_QToolButton_menu_QMenu_QToolButton(Mu::Thread& NODE_THREAD,
                                                  Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QToolButton* arg0 = object<QToolButton>(param_this);
        return makeinstance<QMenuType>(c, arg0->menu(), "qt.QMenu");
    }

    void qt_QToolButton_setMenu_void_QToolButton_QMenu(Mu::Thread& NODE_THREAD,
                                                       Pointer param_this,
                                                       Pointer param_menu)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QToolButton* arg0 = object<QToolButton>(param_this);
        QMenu* arg1 = object<QMenu>(param_menu);
        arg0->setMenu(arg1);
    }

    Pointer
    qt_QToolButton_minimumSizeHint_QSize_QToolButton(Mu::Thread& NODE_THREAD,
                                                     Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QToolButton* arg0 = object<QToolButton>(param_this);
        return isMuQtObject(arg0)
                   ? makeqtype<QSizeType>(
                         c, arg0->QToolButton::minimumSizeHint(), "qt.QSize")
                   : makeqtype<QSizeType>(c, arg0->minimumSizeHint(),
                                          "qt.QSize");
    }

    Pointer qt_QToolButton_sizeHint_QSize_QToolButton(Mu::Thread& NODE_THREAD,
                                                      Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QToolButton* arg0 = object<QToolButton>(param_this);
        return isMuQtObject(arg0)
                   ? makeqtype<QSizeType>(c, arg0->QToolButton::sizeHint(),
                                          "qt.QSize")
                   : makeqtype<QSizeType>(c, arg0->sizeHint(), "qt.QSize");
    }

    void qt_QToolButton_changeEvent_void_QToolButton_QEvent(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_e)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QToolButton* arg0 = object<QToolButton>(param_this);
        QEvent* arg1 = getqpointer<QEventType>(param_e);
        if (isMuQtObject(arg0))
            ((MuQt_QToolButton*)arg0)->changeEvent_pub_parent(arg1);
        else
            ((MuQt_QToolButton*)arg0)->changeEvent_pub(arg1);
    }

    void qt_QToolButton_checkStateSet_void_QToolButton(Mu::Thread& NODE_THREAD,
                                                       Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QToolButton* arg0 = object<QToolButton>(param_this);
        if (isMuQtObject(arg0))
            ((MuQt_QToolButton*)arg0)->checkStateSet_pub_parent();
        else
            ((MuQt_QToolButton*)arg0)->checkStateSet_pub();
    }

    bool qt_QToolButton_event_bool_QToolButton_QEvent(Mu::Thread& NODE_THREAD,
                                                      Pointer param_this,
                                                      Pointer param_event_)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QToolButton* arg0 = object<QToolButton>(param_this);
        QEvent* arg1 = getqpointer<QEventType>(param_event_);
        return isMuQtObject(arg0)
                   ? ((MuQt_QToolButton*)arg0)->event_pub_parent(arg1)
                   : ((MuQt_QToolButton*)arg0)->event_pub(arg1);
    }

    bool qt_QToolButton_hitButton_bool_QToolButton_QPoint(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_pos)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QToolButton* arg0 = object<QToolButton>(param_this);
        const QPoint arg1 = getqtype<QPointType>(param_pos);
        return isMuQtObject(arg0)
                   ? ((MuQt_QToolButton*)arg0)->hitButton_pub_parent(arg1)
                   : ((MuQt_QToolButton*)arg0)->hitButton_pub(arg1);
    }

    void qt_QToolButton_leaveEvent_void_QToolButton_QEvent(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_e)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QToolButton* arg0 = object<QToolButton>(param_this);
        QEvent* arg1 = getqpointer<QEventType>(param_e);
        if (isMuQtObject(arg0))
            ((MuQt_QToolButton*)arg0)->leaveEvent_pub_parent(arg1);
        else
            ((MuQt_QToolButton*)arg0)->leaveEvent_pub(arg1);
    }

    void qt_QToolButton_mousePressEvent_void_QToolButton_QMouseEvent(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_e)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QToolButton* arg0 = object<QToolButton>(param_this);
        QMouseEvent* arg1 = getqpointer<QMouseEventType>(param_e);
        if (isMuQtObject(arg0))
            ((MuQt_QToolButton*)arg0)->mousePressEvent_pub_parent(arg1);
        else
            ((MuQt_QToolButton*)arg0)->mousePressEvent_pub(arg1);
    }

    void qt_QToolButton_mouseReleaseEvent_void_QToolButton_QMouseEvent(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_e)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QToolButton* arg0 = object<QToolButton>(param_this);
        QMouseEvent* arg1 = getqpointer<QMouseEventType>(param_e);
        if (isMuQtObject(arg0))
            ((MuQt_QToolButton*)arg0)->mouseReleaseEvent_pub_parent(arg1);
        else
            ((MuQt_QToolButton*)arg0)->mouseReleaseEvent_pub(arg1);
    }

    void qt_QToolButton_nextCheckState_void_QToolButton(Mu::Thread& NODE_THREAD,
                                                        Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QToolButton* arg0 = object<QToolButton>(param_this);
        if (isMuQtObject(arg0))
            ((MuQt_QToolButton*)arg0)->nextCheckState_pub_parent();
        else
            ((MuQt_QToolButton*)arg0)->nextCheckState_pub();
    }

    void qt_QToolButton_paintEvent_void_QToolButton_QPaintEvent(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_event)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QToolButton* arg0 = object<QToolButton>(param_this);
        QPaintEvent* arg1 = getqpointer<QPaintEventType>(param_event);
        if (isMuQtObject(arg0))
            ((MuQt_QToolButton*)arg0)->paintEvent_pub_parent(arg1);
        else
            ((MuQt_QToolButton*)arg0)->paintEvent_pub(arg1);
    }

    void qt_QToolButton_timerEvent_void_QToolButton_QTimerEvent(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_e)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QToolButton* arg0 = object<QToolButton>(param_this);
        QTimerEvent* arg1 = getqpointer<QTimerEventType>(param_e);
        if (isMuQtObject(arg0))
            ((MuQt_QToolButton*)arg0)->timerEvent_pub_parent(arg1);
        else
            ((MuQt_QToolButton*)arg0)->timerEvent_pub(arg1);
    }

    void qt_QToolButton_focusInEvent_void_QToolButton_QFocusEvent(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_e)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QToolButton* arg0 = object<QToolButton>(param_this);
        QFocusEvent* arg1 = getqpointer<QFocusEventType>(param_e);
        if (isMuQtObject(arg0))
            ((MuQt_QToolButton*)arg0)->focusInEvent_pub_parent(arg1);
        else
            ((MuQt_QToolButton*)arg0)->focusInEvent_pub(arg1);
    }

    void qt_QToolButton_focusOutEvent_void_QToolButton_QFocusEvent(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_e)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QToolButton* arg0 = object<QToolButton>(param_this);
        QFocusEvent* arg1 = getqpointer<QFocusEventType>(param_e);
        if (isMuQtObject(arg0))
            ((MuQt_QToolButton*)arg0)->focusOutEvent_pub_parent(arg1);
        else
            ((MuQt_QToolButton*)arg0)->focusOutEvent_pub(arg1);
    }

    void qt_QToolButton_keyPressEvent_void_QToolButton_QKeyEvent(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_e)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QToolButton* arg0 = object<QToolButton>(param_this);
        QKeyEvent* arg1 = getqpointer<QKeyEventType>(param_e);
        if (isMuQtObject(arg0))
            ((MuQt_QToolButton*)arg0)->keyPressEvent_pub_parent(arg1);
        else
            ((MuQt_QToolButton*)arg0)->keyPressEvent_pub(arg1);
    }

    void qt_QToolButton_keyReleaseEvent_void_QToolButton_QKeyEvent(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_e)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QToolButton* arg0 = object<QToolButton>(param_this);
        QKeyEvent* arg1 = getqpointer<QKeyEventType>(param_e);
        if (isMuQtObject(arg0))
            ((MuQt_QToolButton*)arg0)->keyReleaseEvent_pub_parent(arg1);
        else
            ((MuQt_QToolButton*)arg0)->keyReleaseEvent_pub(arg1);
    }

    void qt_QToolButton_mouseMoveEvent_void_QToolButton_QMouseEvent(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_e)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QToolButton* arg0 = object<QToolButton>(param_this);
        QMouseEvent* arg1 = getqpointer<QMouseEventType>(param_e);
        if (isMuQtObject(arg0))
            ((MuQt_QToolButton*)arg0)->mouseMoveEvent_pub_parent(arg1);
        else
            ((MuQt_QToolButton*)arg0)->mouseMoveEvent_pub(arg1);
    }

    static NODE_IMPLEMENTATION(_n_QToolButton0, Pointer)
    {
        NODE_RETURN(qt_QToolButton_QToolButton_QToolButton_QToolButton_QWidget(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_defaultAction0, Pointer)
    {
        NODE_RETURN(qt_QToolButton_defaultAction_QAction_QToolButton(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_menu0, Pointer)
    {
        NODE_RETURN(qt_QToolButton_menu_QMenu_QToolButton(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_setMenu0, void)
    {
        qt_QToolButton_setMenu_void_QToolButton_QMenu(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_minimumSizeHint0, Pointer)
    {
        NODE_RETURN(qt_QToolButton_minimumSizeHint_QSize_QToolButton(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_sizeHint0, Pointer)
    {
        NODE_RETURN(qt_QToolButton_sizeHint_QSize_QToolButton(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_changeEvent0, void)
    {
        qt_QToolButton_changeEvent_void_QToolButton_QEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_checkStateSet0, void)
    {
        qt_QToolButton_checkStateSet_void_QToolButton(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_event0, bool)
    {
        NODE_RETURN(qt_QToolButton_event_bool_QToolButton_QEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_hitButton0, bool)
    {
        NODE_RETURN(qt_QToolButton_hitButton_bool_QToolButton_QPoint(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_leaveEvent0, void)
    {
        qt_QToolButton_leaveEvent_void_QToolButton_QEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_mousePressEvent0, void)
    {
        qt_QToolButton_mousePressEvent_void_QToolButton_QMouseEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_mouseReleaseEvent0, void)
    {
        qt_QToolButton_mouseReleaseEvent_void_QToolButton_QMouseEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_nextCheckState0, void)
    {
        qt_QToolButton_nextCheckState_void_QToolButton(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_paintEvent0, void)
    {
        qt_QToolButton_paintEvent_void_QToolButton_QPaintEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_timerEvent0, void)
    {
        qt_QToolButton_timerEvent_void_QToolButton_QTimerEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_focusInEvent0, void)
    {
        qt_QToolButton_focusInEvent_void_QToolButton_QFocusEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_focusOutEvent0, void)
    {
        qt_QToolButton_focusOutEvent_void_QToolButton_QFocusEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_keyPressEvent0, void)
    {
        qt_QToolButton_keyPressEvent_void_QToolButton_QKeyEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_keyReleaseEvent0, void)
    {
        qt_QToolButton_keyReleaseEvent_void_QToolButton_QKeyEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_mouseMoveEvent0, void)
    {
        qt_QToolButton_mouseMoveEvent_void_QToolButton_QMouseEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    void QToolButtonType::load()
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
                                QToolButton_QToolButton_QObject, Return, ftn,
                                Parameters,
                                new Param(c, "object", "qt.QObject"), End),

                   EndArguments);

        addSymbols(
            // enums
            // member functions
            new Function(
                c, "QToolButton", _n_QToolButton0, None, Compiled,
                qt_QToolButton_QToolButton_QToolButton_QToolButton_QWidget,
                Return, "qt.QToolButton", Parameters,
                new Param(c, "this", "qt.QToolButton"),
                new Param(c, "parent", "qt.QWidget"), End),
            // PROP: arrowType (flags Qt::ArrowType; QToolButton this)
            // PROP: autoRaise (bool; QToolButton this)
            new Function(c, "defaultAction", _n_defaultAction0, None, Compiled,
                         qt_QToolButton_defaultAction_QAction_QToolButton,
                         Return, "qt.QAction", Parameters,
                         new Param(c, "this", "qt.QToolButton"), End),
            new Function(c, "menu", _n_menu0, None, Compiled,
                         qt_QToolButton_menu_QMenu_QToolButton, Return,
                         "qt.QMenu", Parameters,
                         new Param(c, "this", "qt.QToolButton"), End),
            // PROP: popupMode (flags QToolButton::ToolButtonPopupMode;
            // QToolButton this) PROP: setArrowType (void; QToolButton this,
            // flags Qt::ArrowType type) PROP: setAutoRaise (void; QToolButton
            // this, bool enable)
            new Function(c, "setMenu", _n_setMenu0, None, Compiled,
                         qt_QToolButton_setMenu_void_QToolButton_QMenu, Return,
                         "void", Parameters,
                         new Param(c, "this", "qt.QToolButton"),
                         new Param(c, "menu", "qt.QMenu"), End),
            // PROP: setPopupMode (void; QToolButton this, flags
            // QToolButton::ToolButtonPopupMode mode) PROP: toolButtonStyle
            // (flags Qt::ToolButtonStyle; QToolButton this)
            _func[0] = new MemberFunction(
                c, "minimumSizeHint", _n_minimumSizeHint0, None, Compiled,
                qt_QToolButton_minimumSizeHint_QSize_QToolButton, Return,
                "qt.QSize", Parameters, new Param(c, "this", "qt.QToolButton"),
                End),
            _func[1] = new MemberFunction(
                c, "sizeHint", _n_sizeHint0, None, Compiled,
                qt_QToolButton_sizeHint_QSize_QToolButton, Return, "qt.QSize",
                Parameters, new Param(c, "this", "qt.QToolButton"), End),
            // MISSING: initStyleOption (void; QToolButton this,
            // "QStyleOptionToolButton *" option) // protected MISSING:
            // actionEvent (void; QToolButton this, "QActionEvent *" event) //
            // protected
            _func[2] = new MemberFunction(
                c, "changeEvent", _n_changeEvent0, None, Compiled,
                qt_QToolButton_changeEvent_void_QToolButton_QEvent, Return,
                "void", Parameters, new Param(c, "this", "qt.QToolButton"),
                new Param(c, "e", "qt.QEvent"), End),
            _func[3] = new MemberFunction(
                c, "checkStateSet", _n_checkStateSet0, None, Compiled,
                qt_QToolButton_checkStateSet_void_QToolButton, Return, "void",
                Parameters, new Param(c, "this", "qt.QToolButton"), End),
            // MISSING: enterEvent (void; QToolButton this, "QEnterEvent *" e)
            // // protected
            _func[4] = new MemberFunction(
                c, "event", _n_event0, None, Compiled,
                qt_QToolButton_event_bool_QToolButton_QEvent, Return, "bool",
                Parameters, new Param(c, "this", "qt.QToolButton"),
                new Param(c, "event_", "qt.QEvent"), End),
            _func[5] = new MemberFunction(
                c, "hitButton", _n_hitButton0, None, Compiled,
                qt_QToolButton_hitButton_bool_QToolButton_QPoint, Return,
                "bool", Parameters, new Param(c, "this", "qt.QToolButton"),
                new Param(c, "pos", "qt.QPoint"), End),
            _func[6] = new MemberFunction(
                c, "leaveEvent", _n_leaveEvent0, None, Compiled,
                qt_QToolButton_leaveEvent_void_QToolButton_QEvent, Return,
                "void", Parameters, new Param(c, "this", "qt.QToolButton"),
                new Param(c, "e", "qt.QEvent"), End),
            _func[7] = new MemberFunction(
                c, "mousePressEvent", _n_mousePressEvent0, None, Compiled,
                qt_QToolButton_mousePressEvent_void_QToolButton_QMouseEvent,
                Return, "void", Parameters,
                new Param(c, "this", "qt.QToolButton"),
                new Param(c, "e", "qt.QMouseEvent"), End),
            _func[8] = new MemberFunction(
                c, "mouseReleaseEvent", _n_mouseReleaseEvent0, None, Compiled,
                qt_QToolButton_mouseReleaseEvent_void_QToolButton_QMouseEvent,
                Return, "void", Parameters,
                new Param(c, "this", "qt.QToolButton"),
                new Param(c, "e", "qt.QMouseEvent"), End),
            _func[9] = new MemberFunction(
                c, "nextCheckState", _n_nextCheckState0, None, Compiled,
                qt_QToolButton_nextCheckState_void_QToolButton, Return, "void",
                Parameters, new Param(c, "this", "qt.QToolButton"), End),
            _func[10] = new MemberFunction(
                c, "paintEvent", _n_paintEvent0, None, Compiled,
                qt_QToolButton_paintEvent_void_QToolButton_QPaintEvent, Return,
                "void", Parameters, new Param(c, "this", "qt.QToolButton"),
                new Param(c, "event", "qt.QPaintEvent"), End),
            _func[11] = new MemberFunction(
                c, "timerEvent", _n_timerEvent0, None, Compiled,
                qt_QToolButton_timerEvent_void_QToolButton_QTimerEvent, Return,
                "void", Parameters, new Param(c, "this", "qt.QToolButton"),
                new Param(c, "e", "qt.QTimerEvent"), End),
            _func[12] = new MemberFunction(
                c, "focusInEvent", _n_focusInEvent0, None, Compiled,
                qt_QToolButton_focusInEvent_void_QToolButton_QFocusEvent,
                Return, "void", Parameters,
                new Param(c, "this", "qt.QToolButton"),
                new Param(c, "e", "qt.QFocusEvent"), End),
            _func[13] = new MemberFunction(
                c, "focusOutEvent", _n_focusOutEvent0, None, Compiled,
                qt_QToolButton_focusOutEvent_void_QToolButton_QFocusEvent,
                Return, "void", Parameters,
                new Param(c, "this", "qt.QToolButton"),
                new Param(c, "e", "qt.QFocusEvent"), End),
            _func[14] = new MemberFunction(
                c, "keyPressEvent", _n_keyPressEvent0, None, Compiled,
                qt_QToolButton_keyPressEvent_void_QToolButton_QKeyEvent, Return,
                "void", Parameters, new Param(c, "this", "qt.QToolButton"),
                new Param(c, "e", "qt.QKeyEvent"), End),
            _func[15] = new MemberFunction(
                c, "keyReleaseEvent", _n_keyReleaseEvent0, None, Compiled,
                qt_QToolButton_keyReleaseEvent_void_QToolButton_QKeyEvent,
                Return, "void", Parameters,
                new Param(c, "this", "qt.QToolButton"),
                new Param(c, "e", "qt.QKeyEvent"), End),
            _func[16] = new MemberFunction(
                c, "mouseMoveEvent", _n_mouseMoveEvent0, None, Compiled,
                qt_QToolButton_mouseMoveEvent_void_QToolButton_QMouseEvent,
                Return, "void", Parameters,
                new Param(c, "this", "qt.QToolButton"),
                new Param(c, "e", "qt.QMouseEvent"), End),
            // static functions
            EndArguments);
        globalScope()->addSymbols(EndArguments);
        scope()->addSymbols(EndArguments);

        const char** propExclusions = 0;

        populate(this, QToolButton::staticMetaObject, propExclusions);
    }

} // namespace Mu
