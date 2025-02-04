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
#include <MuQt6/QRadioButtonType.h>
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
#include <MuQt6/QPointType.h>
#include <MuQt6/QTimerEventType.h>

namespace Mu
{
    using namespace std;

    //----------------------------------------------------------------------
    //  INHERITABLE TYPE IMPLEMENTATION

    // destructor
    MuQt_QRadioButton::~MuQt_QRadioButton()
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

    MuQt_QRadioButton::MuQt_QRadioButton(Pointer muobj,
                                         const CallEnvironment* ce,
                                         QWidget* parent)
        : QRadioButton(parent)
    {
        _env = ce;
        _obj = reinterpret_cast<ClassInstance*>(muobj);
        _obj->retainExternal();
        MuLangContext* c = (MuLangContext*)_env->context();
        _baseType = c->findSymbolOfTypeByQualifiedName<QRadioButtonType>(
            c->internName("qt.QRadioButton"));
    }

    MuQt_QRadioButton::MuQt_QRadioButton(Pointer muobj,
                                         const CallEnvironment* ce,
                                         const QString& text, QWidget* parent)
        : QRadioButton(text, parent)
    {
        _env = ce;
        _obj = reinterpret_cast<ClassInstance*>(muobj);
        _obj->retainExternal();
        MuLangContext* c = (MuLangContext*)_env->context();
        _baseType = c->findSymbolOfTypeByQualifiedName<QRadioButtonType>(
            c->internName("qt.QRadioButton"));
    }

    QSize MuQt_QRadioButton::minimumSizeHint() const
    {
        if (!_env)
            return QRadioButton::minimumSizeHint();
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
            return QRadioButton::minimumSizeHint();
        }
    }

    QSize MuQt_QRadioButton::sizeHint() const
    {
        if (!_env)
            return QRadioButton::sizeHint();
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
            return QRadioButton::sizeHint();
        }
    }

    bool MuQt_QRadioButton::event(QEvent* e)
    {
        if (!_env)
            return QRadioButton::event(e);
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
            return QRadioButton::event(e);
        }
    }

    bool MuQt_QRadioButton::hitButton(const QPoint& pos) const
    {
        if (!_env)
            return QRadioButton::hitButton(pos);
        MuLangContext* c = (MuLangContext*)_env->context();
        const MemberFunction* F0 = _baseType->_func[3];
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
            return QRadioButton::hitButton(pos);
        }
    }

    void MuQt_QRadioButton::mouseMoveEvent(QMouseEvent* e)
    {
        if (!_env)
        {
            QRadioButton::mouseMoveEvent(e);
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
            QRadioButton::mouseMoveEvent(e);
        }
    }

    void MuQt_QRadioButton::paintEvent(QPaintEvent* _p14)
    {
        if (!_env)
        {
            QRadioButton::paintEvent(_p14);
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
                Value(makeqpointer<QPaintEventType>(c, _p14, "qt.QPaintEvent"));
            Value rval = _env->call(F, args);
        }
        else
        {
            QRadioButton::paintEvent(_p14);
        }
    }

    void MuQt_QRadioButton::checkStateSet()
    {
        if (!_env)
        {
            QRadioButton::checkStateSet();
            return;
        }
        MuLangContext* c = (MuLangContext*)_env->context();
        const MemberFunction* F0 = _baseType->_func[6];
        const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
        if (F != F0)
        {
            Function::ArgumentVector args(1);
            args[0] = Value(Pointer(_obj));
            Value rval = _env->call(F, args);
        }
        else
        {
            QRadioButton::checkStateSet();
        }
    }

    void MuQt_QRadioButton::nextCheckState()
    {
        if (!_env)
        {
            QRadioButton::nextCheckState();
            return;
        }
        MuLangContext* c = (MuLangContext*)_env->context();
        const MemberFunction* F0 = _baseType->_func[7];
        const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
        if (F != F0)
        {
            Function::ArgumentVector args(1);
            args[0] = Value(Pointer(_obj));
            Value rval = _env->call(F, args);
        }
        else
        {
            QRadioButton::nextCheckState();
        }
    }

    void MuQt_QRadioButton::changeEvent(QEvent* e)
    {
        if (!_env)
        {
            QRadioButton::changeEvent(e);
            return;
        }
        MuLangContext* c = (MuLangContext*)_env->context();
        const MemberFunction* F0 = _baseType->_func[8];
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
            QRadioButton::changeEvent(e);
        }
    }

    void MuQt_QRadioButton::focusInEvent(QFocusEvent* e)
    {
        if (!_env)
        {
            QRadioButton::focusInEvent(e);
            return;
        }
        MuLangContext* c = (MuLangContext*)_env->context();
        const MemberFunction* F0 = _baseType->_func[9];
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
            QRadioButton::focusInEvent(e);
        }
    }

    void MuQt_QRadioButton::focusOutEvent(QFocusEvent* e)
    {
        if (!_env)
        {
            QRadioButton::focusOutEvent(e);
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
                Value(makeqpointer<QFocusEventType>(c, e, "qt.QFocusEvent"));
            Value rval = _env->call(F, args);
        }
        else
        {
            QRadioButton::focusOutEvent(e);
        }
    }

    void MuQt_QRadioButton::keyPressEvent(QKeyEvent* e)
    {
        if (!_env)
        {
            QRadioButton::keyPressEvent(e);
            return;
        }
        MuLangContext* c = (MuLangContext*)_env->context();
        const MemberFunction* F0 = _baseType->_func[11];
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
            QRadioButton::keyPressEvent(e);
        }
    }

    void MuQt_QRadioButton::keyReleaseEvent(QKeyEvent* e)
    {
        if (!_env)
        {
            QRadioButton::keyReleaseEvent(e);
            return;
        }
        MuLangContext* c = (MuLangContext*)_env->context();
        const MemberFunction* F0 = _baseType->_func[12];
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
            QRadioButton::keyReleaseEvent(e);
        }
    }

    void MuQt_QRadioButton::mousePressEvent(QMouseEvent* e)
    {
        if (!_env)
        {
            QRadioButton::mousePressEvent(e);
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
                Value(makeqpointer<QMouseEventType>(c, e, "qt.QMouseEvent"));
            Value rval = _env->call(F, args);
        }
        else
        {
            QRadioButton::mousePressEvent(e);
        }
    }

    void MuQt_QRadioButton::mouseReleaseEvent(QMouseEvent* e)
    {
        if (!_env)
        {
            QRadioButton::mouseReleaseEvent(e);
            return;
        }
        MuLangContext* c = (MuLangContext*)_env->context();
        const MemberFunction* F0 = _baseType->_func[14];
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
            QRadioButton::mouseReleaseEvent(e);
        }
    }

    void MuQt_QRadioButton::timerEvent(QTimerEvent* e)
    {
        if (!_env)
        {
            QRadioButton::timerEvent(e);
            return;
        }
        MuLangContext* c = (MuLangContext*)_env->context();
        const MemberFunction* F0 = _baseType->_func[15];
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
            QRadioButton::timerEvent(e);
        }
    }

    //----------------------------------------------------------------------
    //  Mu Type CONSTRUCTORS

    QRadioButtonType::QRadioButtonType(Context* c, const char* name,
                                       Class* super, Class* super2)
        : Class(c, name, vectorOf2(super, super2))
    {
    }

    QRadioButtonType::~QRadioButtonType() {}

    //----------------------------------------------------------------------
    //  PRE-COMPILED FUNCTIONS

    static Pointer QRadioButton_QRadioButton_QObject(Thread& NODE_THREAD,
                                                     Pointer obj)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        ClassInstance* widget = reinterpret_cast<ClassInstance*>(obj);

        if (!widget)
        {
            return 0;
        }
        else if (QRadioButton* w = object<QRadioButton>(widget))
        {
            QRadioButtonType* type =
                c->findSymbolOfTypeByQualifiedName<QRadioButtonType>(
                    c->internName("qt.QRadioButton"), false);
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
        NODE_RETURN(QRadioButton_QRadioButton_QObject(NODE_THREAD,
                                                      NODE_ARG(0, Pointer)));
    }

    Pointer qt_QRadioButton_QRadioButton_QRadioButton_QRadioButton_QWidget(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_parent)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QWidget* arg1 = object<QWidget>(param_parent);
        setobject(param_this,
                  new MuQt_QRadioButton(
                      param_this, NODE_THREAD.process()->callEnv(), arg1));
        return param_this;
    }

    Pointer
    qt_QRadioButton_QRadioButton_QRadioButton_QRadioButton_string_QWidget(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_text,
        Pointer param_parent)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        const QString arg1 = qstring(param_text);
        QWidget* arg2 = object<QWidget>(param_parent);
        setobject(param_this, new MuQt_QRadioButton(
                                  param_this, NODE_THREAD.process()->callEnv(),
                                  arg1, arg2));
        return param_this;
    }

    Pointer
    qt_QRadioButton_minimumSizeHint_QSize_QRadioButton(Mu::Thread& NODE_THREAD,
                                                       Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QRadioButton* arg0 = object<QRadioButton>(param_this);
        return isMuQtObject(arg0)
                   ? makeqtype<QSizeType>(
                         c, arg0->QRadioButton::minimumSizeHint(), "qt.QSize")
                   : makeqtype<QSizeType>(c, arg0->minimumSizeHint(),
                                          "qt.QSize");
    }

    Pointer qt_QRadioButton_sizeHint_QSize_QRadioButton(Mu::Thread& NODE_THREAD,
                                                        Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QRadioButton* arg0 = object<QRadioButton>(param_this);
        return isMuQtObject(arg0)
                   ? makeqtype<QSizeType>(c, arg0->QRadioButton::sizeHint(),
                                          "qt.QSize")
                   : makeqtype<QSizeType>(c, arg0->sizeHint(), "qt.QSize");
    }

    bool qt_QRadioButton_event_bool_QRadioButton_QEvent(Mu::Thread& NODE_THREAD,
                                                        Pointer param_this,
                                                        Pointer param_e)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QRadioButton* arg0 = object<QRadioButton>(param_this);
        QEvent* arg1 = getqpointer<QEventType>(param_e);
        return isMuQtObject(arg0)
                   ? ((MuQt_QRadioButton*)arg0)->event_pub_parent(arg1)
                   : ((MuQt_QRadioButton*)arg0)->event_pub(arg1);
    }

    bool qt_QRadioButton_hitButton_bool_QRadioButton_QPoint(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_pos)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QRadioButton* arg0 = object<QRadioButton>(param_this);
        const QPoint arg1 = getqtype<QPointType>(param_pos);
        return isMuQtObject(arg0)
                   ? ((MuQt_QRadioButton*)arg0)->hitButton_pub_parent(arg1)
                   : ((MuQt_QRadioButton*)arg0)->hitButton_pub(arg1);
    }

    void qt_QRadioButton_mouseMoveEvent_void_QRadioButton_QMouseEvent(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_e)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QRadioButton* arg0 = object<QRadioButton>(param_this);
        QMouseEvent* arg1 = getqpointer<QMouseEventType>(param_e);
        if (isMuQtObject(arg0))
            ((MuQt_QRadioButton*)arg0)->mouseMoveEvent_pub_parent(arg1);
        else
            ((MuQt_QRadioButton*)arg0)->mouseMoveEvent_pub(arg1);
    }

    void qt_QRadioButton_paintEvent_void_QRadioButton_QPaintEvent(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param__p14)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QRadioButton* arg0 = object<QRadioButton>(param_this);
        QPaintEvent* arg1 = getqpointer<QPaintEventType>(param__p14);
        if (isMuQtObject(arg0))
            ((MuQt_QRadioButton*)arg0)->paintEvent_pub_parent(arg1);
        else
            ((MuQt_QRadioButton*)arg0)->paintEvent_pub(arg1);
    }

    void
    qt_QRadioButton_checkStateSet_void_QRadioButton(Mu::Thread& NODE_THREAD,
                                                    Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QRadioButton* arg0 = object<QRadioButton>(param_this);
        if (isMuQtObject(arg0))
            ((MuQt_QRadioButton*)arg0)->checkStateSet_pub_parent();
        else
            ((MuQt_QRadioButton*)arg0)->checkStateSet_pub();
    }

    void
    qt_QRadioButton_nextCheckState_void_QRadioButton(Mu::Thread& NODE_THREAD,
                                                     Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QRadioButton* arg0 = object<QRadioButton>(param_this);
        if (isMuQtObject(arg0))
            ((MuQt_QRadioButton*)arg0)->nextCheckState_pub_parent();
        else
            ((MuQt_QRadioButton*)arg0)->nextCheckState_pub();
    }

    void qt_QRadioButton_changeEvent_void_QRadioButton_QEvent(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_e)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QRadioButton* arg0 = object<QRadioButton>(param_this);
        QEvent* arg1 = getqpointer<QEventType>(param_e);
        if (isMuQtObject(arg0))
            ((MuQt_QRadioButton*)arg0)->changeEvent_pub_parent(arg1);
        else
            ((MuQt_QRadioButton*)arg0)->changeEvent_pub(arg1);
    }

    void qt_QRadioButton_focusInEvent_void_QRadioButton_QFocusEvent(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_e)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QRadioButton* arg0 = object<QRadioButton>(param_this);
        QFocusEvent* arg1 = getqpointer<QFocusEventType>(param_e);
        if (isMuQtObject(arg0))
            ((MuQt_QRadioButton*)arg0)->focusInEvent_pub_parent(arg1);
        else
            ((MuQt_QRadioButton*)arg0)->focusInEvent_pub(arg1);
    }

    void qt_QRadioButton_focusOutEvent_void_QRadioButton_QFocusEvent(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_e)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QRadioButton* arg0 = object<QRadioButton>(param_this);
        QFocusEvent* arg1 = getqpointer<QFocusEventType>(param_e);
        if (isMuQtObject(arg0))
            ((MuQt_QRadioButton*)arg0)->focusOutEvent_pub_parent(arg1);
        else
            ((MuQt_QRadioButton*)arg0)->focusOutEvent_pub(arg1);
    }

    void qt_QRadioButton_keyPressEvent_void_QRadioButton_QKeyEvent(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_e)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QRadioButton* arg0 = object<QRadioButton>(param_this);
        QKeyEvent* arg1 = getqpointer<QKeyEventType>(param_e);
        if (isMuQtObject(arg0))
            ((MuQt_QRadioButton*)arg0)->keyPressEvent_pub_parent(arg1);
        else
            ((MuQt_QRadioButton*)arg0)->keyPressEvent_pub(arg1);
    }

    void qt_QRadioButton_keyReleaseEvent_void_QRadioButton_QKeyEvent(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_e)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QRadioButton* arg0 = object<QRadioButton>(param_this);
        QKeyEvent* arg1 = getqpointer<QKeyEventType>(param_e);
        if (isMuQtObject(arg0))
            ((MuQt_QRadioButton*)arg0)->keyReleaseEvent_pub_parent(arg1);
        else
            ((MuQt_QRadioButton*)arg0)->keyReleaseEvent_pub(arg1);
    }

    void qt_QRadioButton_mousePressEvent_void_QRadioButton_QMouseEvent(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_e)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QRadioButton* arg0 = object<QRadioButton>(param_this);
        QMouseEvent* arg1 = getqpointer<QMouseEventType>(param_e);
        if (isMuQtObject(arg0))
            ((MuQt_QRadioButton*)arg0)->mousePressEvent_pub_parent(arg1);
        else
            ((MuQt_QRadioButton*)arg0)->mousePressEvent_pub(arg1);
    }

    void qt_QRadioButton_mouseReleaseEvent_void_QRadioButton_QMouseEvent(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_e)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QRadioButton* arg0 = object<QRadioButton>(param_this);
        QMouseEvent* arg1 = getqpointer<QMouseEventType>(param_e);
        if (isMuQtObject(arg0))
            ((MuQt_QRadioButton*)arg0)->mouseReleaseEvent_pub_parent(arg1);
        else
            ((MuQt_QRadioButton*)arg0)->mouseReleaseEvent_pub(arg1);
    }

    void qt_QRadioButton_timerEvent_void_QRadioButton_QTimerEvent(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_e)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QRadioButton* arg0 = object<QRadioButton>(param_this);
        QTimerEvent* arg1 = getqpointer<QTimerEventType>(param_e);
        if (isMuQtObject(arg0))
            ((MuQt_QRadioButton*)arg0)->timerEvent_pub_parent(arg1);
        else
            ((MuQt_QRadioButton*)arg0)->timerEvent_pub(arg1);
    }

    static NODE_IMPLEMENTATION(_n_QRadioButton0, Pointer)
    {
        NODE_RETURN(
            qt_QRadioButton_QRadioButton_QRadioButton_QRadioButton_QWidget(
                NODE_THREAD, NONNIL_NODE_ARG(0, Pointer),
                NODE_ARG(1, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_QRadioButton1, Pointer)
    {
        NODE_RETURN(
            qt_QRadioButton_QRadioButton_QRadioButton_QRadioButton_string_QWidget(
                NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer),
                NODE_ARG(2, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_minimumSizeHint0, Pointer)
    {
        NODE_RETURN(qt_QRadioButton_minimumSizeHint_QSize_QRadioButton(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_sizeHint0, Pointer)
    {
        NODE_RETURN(qt_QRadioButton_sizeHint_QSize_QRadioButton(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_event0, bool)
    {
        NODE_RETURN(qt_QRadioButton_event_bool_QRadioButton_QEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_hitButton0, bool)
    {
        NODE_RETURN(qt_QRadioButton_hitButton_bool_QRadioButton_QPoint(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_mouseMoveEvent0, void)
    {
        qt_QRadioButton_mouseMoveEvent_void_QRadioButton_QMouseEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_paintEvent0, void)
    {
        qt_QRadioButton_paintEvent_void_QRadioButton_QPaintEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_checkStateSet0, void)
    {
        qt_QRadioButton_checkStateSet_void_QRadioButton(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_nextCheckState0, void)
    {
        qt_QRadioButton_nextCheckState_void_QRadioButton(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_changeEvent0, void)
    {
        qt_QRadioButton_changeEvent_void_QRadioButton_QEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_focusInEvent0, void)
    {
        qt_QRadioButton_focusInEvent_void_QRadioButton_QFocusEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_focusOutEvent0, void)
    {
        qt_QRadioButton_focusOutEvent_void_QRadioButton_QFocusEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_keyPressEvent0, void)
    {
        qt_QRadioButton_keyPressEvent_void_QRadioButton_QKeyEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_keyReleaseEvent0, void)
    {
        qt_QRadioButton_keyReleaseEvent_void_QRadioButton_QKeyEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_mousePressEvent0, void)
    {
        qt_QRadioButton_mousePressEvent_void_QRadioButton_QMouseEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_mouseReleaseEvent0, void)
    {
        qt_QRadioButton_mouseReleaseEvent_void_QRadioButton_QMouseEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_timerEvent0, void)
    {
        qt_QRadioButton_timerEvent_void_QRadioButton_QTimerEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    void QRadioButtonType::load()
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
                                QRadioButton_QRadioButton_QObject, Return, ftn,
                                Parameters,
                                new Param(c, "object", "qt.QObject"), End),

                   EndArguments);

        addSymbols(
            // enums
            // member functions
            new Function(
                c, "QRadioButton", _n_QRadioButton0, None, Compiled,
                qt_QRadioButton_QRadioButton_QRadioButton_QRadioButton_QWidget,
                Return, "qt.QRadioButton", Parameters,
                new Param(c, "this", "qt.QRadioButton"),
                new Param(c, "parent", "qt.QWidget"), End),
            new Function(
                c, "QRadioButton", _n_QRadioButton1, None, Compiled,
                qt_QRadioButton_QRadioButton_QRadioButton_QRadioButton_string_QWidget,
                Return, "qt.QRadioButton", Parameters,
                new Param(c, "this", "qt.QRadioButton"),
                new Param(c, "text", "string"),
                new Param(c, "parent", "qt.QWidget"), End),
            _func[0] = new MemberFunction(
                c, "minimumSizeHint", _n_minimumSizeHint0, None, Compiled,
                qt_QRadioButton_minimumSizeHint_QSize_QRadioButton, Return,
                "qt.QSize", Parameters, new Param(c, "this", "qt.QRadioButton"),
                End),
            _func[1] = new MemberFunction(
                c, "sizeHint", _n_sizeHint0, None, Compiled,
                qt_QRadioButton_sizeHint_QSize_QRadioButton, Return, "qt.QSize",
                Parameters, new Param(c, "this", "qt.QRadioButton"), End),
            // MISSING: initStyleOption (void; QRadioButton this,
            // "QStyleOptionButton *" option) // protected
            _func[2] = new MemberFunction(
                c, "event", _n_event0, None, Compiled,
                qt_QRadioButton_event_bool_QRadioButton_QEvent, Return, "bool",
                Parameters, new Param(c, "this", "qt.QRadioButton"),
                new Param(c, "e", "qt.QEvent"), End),
            _func[3] = new MemberFunction(
                c, "hitButton", _n_hitButton0, None, Compiled,
                qt_QRadioButton_hitButton_bool_QRadioButton_QPoint, Return,
                "bool", Parameters, new Param(c, "this", "qt.QRadioButton"),
                new Param(c, "pos", "qt.QPoint"), End),
            _func[4] = new MemberFunction(
                c, "mouseMoveEvent", _n_mouseMoveEvent0, None, Compiled,
                qt_QRadioButton_mouseMoveEvent_void_QRadioButton_QMouseEvent,
                Return, "void", Parameters,
                new Param(c, "this", "qt.QRadioButton"),
                new Param(c, "e", "qt.QMouseEvent"), End),
            _func[5] = new MemberFunction(
                c, "paintEvent", _n_paintEvent0, None, Compiled,
                qt_QRadioButton_paintEvent_void_QRadioButton_QPaintEvent,
                Return, "void", Parameters,
                new Param(c, "this", "qt.QRadioButton"),
                new Param(c, "_p14", "qt.QPaintEvent"), End),
            _func[6] = new MemberFunction(
                c, "checkStateSet", _n_checkStateSet0, None, Compiled,
                qt_QRadioButton_checkStateSet_void_QRadioButton, Return, "void",
                Parameters, new Param(c, "this", "qt.QRadioButton"), End),
            _func[7] = new MemberFunction(
                c, "nextCheckState", _n_nextCheckState0, None, Compiled,
                qt_QRadioButton_nextCheckState_void_QRadioButton, Return,
                "void", Parameters, new Param(c, "this", "qt.QRadioButton"),
                End),
            _func[8] = new MemberFunction(
                c, "changeEvent", _n_changeEvent0, None, Compiled,
                qt_QRadioButton_changeEvent_void_QRadioButton_QEvent, Return,
                "void", Parameters, new Param(c, "this", "qt.QRadioButton"),
                new Param(c, "e", "qt.QEvent"), End),
            _func[9] = new MemberFunction(
                c, "focusInEvent", _n_focusInEvent0, None, Compiled,
                qt_QRadioButton_focusInEvent_void_QRadioButton_QFocusEvent,
                Return, "void", Parameters,
                new Param(c, "this", "qt.QRadioButton"),
                new Param(c, "e", "qt.QFocusEvent"), End),
            _func[10] = new MemberFunction(
                c, "focusOutEvent", _n_focusOutEvent0, None, Compiled,
                qt_QRadioButton_focusOutEvent_void_QRadioButton_QFocusEvent,
                Return, "void", Parameters,
                new Param(c, "this", "qt.QRadioButton"),
                new Param(c, "e", "qt.QFocusEvent"), End),
            _func[11] = new MemberFunction(
                c, "keyPressEvent", _n_keyPressEvent0, None, Compiled,
                qt_QRadioButton_keyPressEvent_void_QRadioButton_QKeyEvent,
                Return, "void", Parameters,
                new Param(c, "this", "qt.QRadioButton"),
                new Param(c, "e", "qt.QKeyEvent"), End),
            _func[12] = new MemberFunction(
                c, "keyReleaseEvent", _n_keyReleaseEvent0, None, Compiled,
                qt_QRadioButton_keyReleaseEvent_void_QRadioButton_QKeyEvent,
                Return, "void", Parameters,
                new Param(c, "this", "qt.QRadioButton"),
                new Param(c, "e", "qt.QKeyEvent"), End),
            _func[13] = new MemberFunction(
                c, "mousePressEvent", _n_mousePressEvent0, None, Compiled,
                qt_QRadioButton_mousePressEvent_void_QRadioButton_QMouseEvent,
                Return, "void", Parameters,
                new Param(c, "this", "qt.QRadioButton"),
                new Param(c, "e", "qt.QMouseEvent"), End),
            _func[14] = new MemberFunction(
                c, "mouseReleaseEvent", _n_mouseReleaseEvent0, None, Compiled,
                qt_QRadioButton_mouseReleaseEvent_void_QRadioButton_QMouseEvent,
                Return, "void", Parameters,
                new Param(c, "this", "qt.QRadioButton"),
                new Param(c, "e", "qt.QMouseEvent"), End),
            _func[15] = new MemberFunction(
                c, "timerEvent", _n_timerEvent0, None, Compiled,
                qt_QRadioButton_timerEvent_void_QRadioButton_QTimerEvent,
                Return, "void", Parameters,
                new Param(c, "this", "qt.QRadioButton"),
                new Param(c, "e", "qt.QTimerEvent"), End),
            // static functions
            EndArguments);
        globalScope()->addSymbols(EndArguments);
        scope()->addSymbols(EndArguments);

        const char** propExclusions = 0;

        populate(this, QRadioButton::staticMetaObject, propExclusions);
    }

} // namespace Mu
