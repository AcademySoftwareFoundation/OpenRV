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
#include <MuQt6/QLocalSocketType.h>
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

namespace Mu {
using namespace std;

//----------------------------------------------------------------------
//  INHERITABLE TYPE IMPLEMENTATION

// destructor
MuQt_QLocalSocket::~MuQt_QLocalSocket()
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

MuQt_QLocalSocket::MuQt_QLocalSocket(Pointer muobj, const CallEnvironment* ce, QObject * parent) 
 : QLocalSocket(parent)
{
    _env = ce;
    _obj = reinterpret_cast<ClassInstance*>(muobj);
    _obj->retainExternal();
    MuLangContext* c = (MuLangContext*)_env->context();
    _baseType = c->findSymbolOfTypeByQualifiedName<QLocalSocketType>(c->internName("qt.QLocalSocket"));
}

qint64 MuQt_QLocalSocket::bytesAvailable() const
{
    if (!_env) return QLocalSocket::bytesAvailable();
    MuLangContext* c = (MuLangContext*)_env->context();
    const MemberFunction* F0 = _baseType->_func[0];
    const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
    if (F != F0) 
    {
        Function::ArgumentVector args(1);
        args[0] = Value(Pointer(_obj));
        Value rval = _env->call(F, args);
        return (int64)(rval._int64);
    }
    else
    {
        return QLocalSocket::bytesAvailable();
    }
}

qint64 MuQt_QLocalSocket::bytesToWrite() const
{
    if (!_env) return QLocalSocket::bytesToWrite();
    MuLangContext* c = (MuLangContext*)_env->context();
    const MemberFunction* F0 = _baseType->_func[1];
    const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
    if (F != F0) 
    {
        Function::ArgumentVector args(1);
        args[0] = Value(Pointer(_obj));
        Value rval = _env->call(F, args);
        return (int64)(rval._int64);
    }
    else
    {
        return QLocalSocket::bytesToWrite();
    }
}

bool MuQt_QLocalSocket::canReadLine() const
{
    if (!_env) return QLocalSocket::canReadLine();
    MuLangContext* c = (MuLangContext*)_env->context();
    const MemberFunction* F0 = _baseType->_func[2];
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
        return QLocalSocket::canReadLine();
    }
}

void MuQt_QLocalSocket::close() 
{
    if (!_env) { QLocalSocket::close(); return; }
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
        QLocalSocket::close();
    }
}

bool MuQt_QLocalSocket::isSequential() const
{
    if (!_env) return QLocalSocket::isSequential();
    MuLangContext* c = (MuLangContext*)_env->context();
    const MemberFunction* F0 = _baseType->_func[4];
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
        return QLocalSocket::isSequential();
    }
}

bool MuQt_QLocalSocket::open(QIODeviceBase::OpenMode openMode) 
{
    if (!_env) return QLocalSocket::open(openMode);
    MuLangContext* c = (MuLangContext*)_env->context();
    const MemberFunction* F0 = _baseType->_func[5];
    const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
    if (F != F0) 
    {
        Function::ArgumentVector args(2);
        args[0] = Value(Pointer(_obj));
        args[1] = Value(int(openMode));
        Value rval = _env->call(F, args);
        return (bool)(rval._bool);
    }
    else
    {
        return QLocalSocket::open(openMode);
    }
}

bool MuQt_QLocalSocket::waitForBytesWritten(int msecs) 
{
    if (!_env) return QLocalSocket::waitForBytesWritten(msecs);
    MuLangContext* c = (MuLangContext*)_env->context();
    const MemberFunction* F0 = _baseType->_func[6];
    const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
    if (F != F0) 
    {
        Function::ArgumentVector args(2);
        args[0] = Value(Pointer(_obj));
        args[1] = Value(msecs);
        Value rval = _env->call(F, args);
        return (bool)(rval._bool);
    }
    else
    {
        return QLocalSocket::waitForBytesWritten(msecs);
    }
}

bool MuQt_QLocalSocket::waitForReadyRead(int msecs) 
{
    if (!_env) return QLocalSocket::waitForReadyRead(msecs);
    MuLangContext* c = (MuLangContext*)_env->context();
    const MemberFunction* F0 = _baseType->_func[7];
    const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
    if (F != F0) 
    {
        Function::ArgumentVector args(2);
        args[0] = Value(Pointer(_obj));
        args[1] = Value(msecs);
        Value rval = _env->call(F, args);
        return (bool)(rval._bool);
    }
    else
    {
        return QLocalSocket::waitForReadyRead(msecs);
    }
}

qint64 MuQt_QLocalSocket::skipData(qint64 maxSize) 
{
    if (!_env) return QLocalSocket::skipData(maxSize);
    MuLangContext* c = (MuLangContext*)_env->context();
    const MemberFunction* F0 = _baseType->_func[8];
    const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
    if (F != F0) 
    {
        Function::ArgumentVector args(2);
        args[0] = Value(Pointer(_obj));
        args[1] = Value(maxSize);
        Value rval = _env->call(F, args);
        return (int64)(rval._int64);
    }
    else
    {
        return QLocalSocket::skipData(maxSize);
    }
}

bool MuQt_QLocalSocket::atEnd() const
{
    if (!_env) return QLocalSocket::atEnd();
    MuLangContext* c = (MuLangContext*)_env->context();
    const MemberFunction* F0 = _baseType->_func[9];
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
        return QLocalSocket::atEnd();
    }
}

qint64 MuQt_QLocalSocket::pos() const
{
    if (!_env) return QLocalSocket::pos();
    MuLangContext* c = (MuLangContext*)_env->context();
    const MemberFunction* F0 = _baseType->_func[10];
    const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
    if (F != F0) 
    {
        Function::ArgumentVector args(1);
        args[0] = Value(Pointer(_obj));
        Value rval = _env->call(F, args);
        return (int64)(rval._int64);
    }
    else
    {
        return QLocalSocket::pos();
    }
}

bool MuQt_QLocalSocket::reset() 
{
    if (!_env) return QLocalSocket::reset();
    MuLangContext* c = (MuLangContext*)_env->context();
    const MemberFunction* F0 = _baseType->_func[11];
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
        return QLocalSocket::reset();
    }
}

bool MuQt_QLocalSocket::seek(qint64 pos) 
{
    if (!_env) return QLocalSocket::seek(pos);
    MuLangContext* c = (MuLangContext*)_env->context();
    const MemberFunction* F0 = _baseType->_func[12];
    const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
    if (F != F0) 
    {
        Function::ArgumentVector args(2);
        args[0] = Value(Pointer(_obj));
        args[1] = Value(pos);
        Value rval = _env->call(F, args);
        return (bool)(rval._bool);
    }
    else
    {
        return QLocalSocket::seek(pos);
    }
}

qint64 MuQt_QLocalSocket::size() const
{
    if (!_env) return QLocalSocket::size();
    MuLangContext* c = (MuLangContext*)_env->context();
    const MemberFunction* F0 = _baseType->_func[13];
    const MemberFunction* F = _obj->classType()->dynamicLookup(F0);
    if (F != F0) 
    {
        Function::ArgumentVector args(1);
        args[0] = Value(Pointer(_obj));
        Value rval = _env->call(F, args);
        return (int64)(rval._int64);
    }
    else
    {
        return QLocalSocket::size();
    }
}



//----------------------------------------------------------------------
//  Mu Type CONSTRUCTORS

QLocalSocketType::QLocalSocketType(Context* c, const char* name, Class* super, Class* super2)
: Class(c, name, vectorOf2(super, super2))
{
}

QLocalSocketType::~QLocalSocketType()
{
}

//----------------------------------------------------------------------
//  PRE-COMPILED FUNCTIONS

static Pointer
QLocalSocket_QLocalSocket_QObject(Thread& NODE_THREAD, Pointer obj)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    ClassInstance* widget = reinterpret_cast<ClassInstance*>(obj);

    if (!widget)
    {
        return 0;
    }
    else if (QLocalSocket* w = object<QLocalSocket>(widget))
    {
        QLocalSocketType* type = 
            c->findSymbolOfTypeByQualifiedName<QLocalSocketType>(c->internName("qt.QLocalSocket"), false);
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
    NODE_RETURN( QLocalSocket_QLocalSocket_QObject(NODE_THREAD, NODE_ARG(0, Pointer)) );
}

Pointer qt_QLocalSocket_QLocalSocket_QLocalSocket_QLocalSocket_QObject(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_parent)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QObject * arg1 = object<QObject>(param_parent);
    setobject(param_this, new MuQt_QLocalSocket(param_this, NODE_THREAD.process()->callEnv(), arg1));
    return param_this;
}

void qt_QLocalSocket_abort_void_QLocalSocket(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QLocalSocket* arg0 = object<QLocalSocket>(param_this);
    arg0->abort();
}

void qt_QLocalSocket_connectToServer_void_QLocalSocket_int(Mu::Thread& NODE_THREAD, Pointer param_this, int param_openMode)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QLocalSocket* arg0 = object<QLocalSocket>(param_this);
    QIODeviceBase::OpenMode arg1 = (QIODeviceBase::OpenMode)(param_openMode);
    arg0->connectToServer(arg1);
}

void qt_QLocalSocket_connectToServer_void_QLocalSocket_string_int(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_name, int param_openMode)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QLocalSocket* arg0 = object<QLocalSocket>(param_this);
    const QString  arg1 = qstring(param_name);
    QIODeviceBase::OpenMode arg2 = (QIODeviceBase::OpenMode)(param_openMode);
    arg0->connectToServer(arg1, arg2);
}

void qt_QLocalSocket_disconnectFromServer_void_QLocalSocket(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QLocalSocket* arg0 = object<QLocalSocket>(param_this);
    arg0->disconnectFromServer();
}

int qt_QLocalSocket_error_int_QLocalSocket(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QLocalSocket* arg0 = object<QLocalSocket>(param_this);
    return int(arg0->error());
}

bool qt_QLocalSocket_flush_bool_QLocalSocket(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QLocalSocket* arg0 = object<QLocalSocket>(param_this);
    return arg0->flush();
}

Pointer qt_QLocalSocket_fullServerName_string_QLocalSocket(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QLocalSocket* arg0 = object<QLocalSocket>(param_this);
    return makestring(c,arg0->fullServerName());
}

bool qt_QLocalSocket_isValid_bool_QLocalSocket(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QLocalSocket* arg0 = object<QLocalSocket>(param_this);
    return arg0->isValid();
}

int64 qt_QLocalSocket_readBufferSize_int64_QLocalSocket(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QLocalSocket* arg0 = object<QLocalSocket>(param_this);
    return arg0->readBufferSize();
}

Pointer qt_QLocalSocket_serverName_string_QLocalSocket(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QLocalSocket* arg0 = object<QLocalSocket>(param_this);
    return makestring(c,arg0->serverName());
}

void qt_QLocalSocket_setReadBufferSize_void_QLocalSocket_int64(Mu::Thread& NODE_THREAD, Pointer param_this, int64 param_size)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QLocalSocket* arg0 = object<QLocalSocket>(param_this);
    qint64 arg1 = (int64)(param_size);
    arg0->setReadBufferSize(arg1);
}

void qt_QLocalSocket_setServerName_void_QLocalSocket_string(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_name)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QLocalSocket* arg0 = object<QLocalSocket>(param_this);
    const QString  arg1 = qstring(param_name);
    arg0->setServerName(arg1);
}

int qt_QLocalSocket_state_int_QLocalSocket(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QLocalSocket* arg0 = object<QLocalSocket>(param_this);
    return int(arg0->state());
}

bool qt_QLocalSocket_waitForConnected_bool_QLocalSocket_int(Mu::Thread& NODE_THREAD, Pointer param_this, int param_msecs)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QLocalSocket* arg0 = object<QLocalSocket>(param_this);
    int arg1 = (int)(param_msecs);
    return arg0->waitForConnected(arg1);
}

bool qt_QLocalSocket_waitForDisconnected_bool_QLocalSocket_int(Mu::Thread& NODE_THREAD, Pointer param_this, int param_msecs)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QLocalSocket* arg0 = object<QLocalSocket>(param_this);
    int arg1 = (int)(param_msecs);
    return arg0->waitForDisconnected(arg1);
}

int64 qt_QLocalSocket_bytesAvailable_int64_QLocalSocket(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QLocalSocket* arg0 = object<QLocalSocket>(param_this);
    return isMuQtObject(arg0) ? arg0->QLocalSocket::bytesAvailable() : arg0->bytesAvailable();
}

int64 qt_QLocalSocket_bytesToWrite_int64_QLocalSocket(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QLocalSocket* arg0 = object<QLocalSocket>(param_this);
    return isMuQtObject(arg0) ? arg0->QLocalSocket::bytesToWrite() : arg0->bytesToWrite();
}

bool qt_QLocalSocket_canReadLine_bool_QLocalSocket(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QLocalSocket* arg0 = object<QLocalSocket>(param_this);
    return isMuQtObject(arg0) ? arg0->QLocalSocket::canReadLine() : arg0->canReadLine();
}

void qt_QLocalSocket_close_void_QLocalSocket(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QLocalSocket* arg0 = object<QLocalSocket>(param_this);
    if (isMuQtObject(arg0)) arg0->QLocalSocket::close();
    else arg0->close();
}

bool qt_QLocalSocket_isSequential_bool_QLocalSocket(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QLocalSocket* arg0 = object<QLocalSocket>(param_this);
    return isMuQtObject(arg0) ? arg0->QLocalSocket::isSequential() : arg0->isSequential();
}

bool qt_QLocalSocket_open_bool_QLocalSocket_int(Mu::Thread& NODE_THREAD, Pointer param_this, int param_openMode)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QLocalSocket* arg0 = object<QLocalSocket>(param_this);
    QIODeviceBase::OpenMode arg1 = (QIODeviceBase::OpenMode)(param_openMode);
    return isMuQtObject(arg0) ? arg0->QLocalSocket::open(arg1) : arg0->open(arg1);
}

bool qt_QLocalSocket_waitForBytesWritten_bool_QLocalSocket_int(Mu::Thread& NODE_THREAD, Pointer param_this, int param_msecs)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QLocalSocket* arg0 = object<QLocalSocket>(param_this);
    int arg1 = (int)(param_msecs);
    return isMuQtObject(arg0) ? arg0->QLocalSocket::waitForBytesWritten(arg1) : arg0->waitForBytesWritten(arg1);
}

bool qt_QLocalSocket_waitForReadyRead_bool_QLocalSocket_int(Mu::Thread& NODE_THREAD, Pointer param_this, int param_msecs)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QLocalSocket* arg0 = object<QLocalSocket>(param_this);
    int arg1 = (int)(param_msecs);
    return isMuQtObject(arg0) ? arg0->QLocalSocket::waitForReadyRead(arg1) : arg0->waitForReadyRead(arg1);
}

int64 qt_QLocalSocket_skipData_int64_QLocalSocket_int64(Mu::Thread& NODE_THREAD, Pointer param_this, int64 param_maxSize)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QLocalSocket* arg0 = object<QLocalSocket>(param_this);
    qint64 arg1 = (int64)(param_maxSize);
    return isMuQtObject(arg0) ? ((MuQt_QLocalSocket*)arg0)->skipData_pub_parent(arg1) : ((MuQt_QLocalSocket*)arg0)->skipData_pub(arg1);
}

bool qt_QLocalSocket_atEnd_bool_QLocalSocket(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QLocalSocket* arg0 = object<QLocalSocket>(param_this);
    return isMuQtObject(arg0) ? arg0->QLocalSocket::atEnd() : arg0->atEnd();
}

int64 qt_QLocalSocket_pos_int64_QLocalSocket(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QLocalSocket* arg0 = object<QLocalSocket>(param_this);
    return isMuQtObject(arg0) ? arg0->QLocalSocket::pos() : arg0->pos();
}

bool qt_QLocalSocket_reset_bool_QLocalSocket(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QLocalSocket* arg0 = object<QLocalSocket>(param_this);
    return isMuQtObject(arg0) ? arg0->QLocalSocket::reset() : arg0->reset();
}

bool qt_QLocalSocket_seek_bool_QLocalSocket_int64(Mu::Thread& NODE_THREAD, Pointer param_this, int64 param_pos)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QLocalSocket* arg0 = object<QLocalSocket>(param_this);
    qint64 arg1 = (int64)(param_pos);
    return isMuQtObject(arg0) ? arg0->QLocalSocket::seek(arg1) : arg0->seek(arg1);
}

int64 qt_QLocalSocket_size_int64_QLocalSocket(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QLocalSocket* arg0 = object<QLocalSocket>(param_this);
    return isMuQtObject(arg0) ? arg0->QLocalSocket::size() : arg0->size();
}


static NODE_IMPLEMENTATION(_n_QLocalSocket0, Pointer)
{
    NODE_RETURN(qt_QLocalSocket_QLocalSocket_QLocalSocket_QLocalSocket_QObject(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}

static NODE_IMPLEMENTATION(_n_abort0, void)
{
    qt_QLocalSocket_abort_void_QLocalSocket(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer));
}

static NODE_IMPLEMENTATION(_n_connectToServer0, void)
{
    qt_QLocalSocket_connectToServer_void_QLocalSocket_int(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int));
}

static NODE_IMPLEMENTATION(_n_connectToServer1, void)
{
    qt_QLocalSocket_connectToServer_void_QLocalSocket_string_int(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer), NODE_ARG(2, int));
}

static NODE_IMPLEMENTATION(_n_disconnectFromServer0, void)
{
    qt_QLocalSocket_disconnectFromServer_void_QLocalSocket(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer));
}

static NODE_IMPLEMENTATION(_n_error0, int)
{
    NODE_RETURN(qt_QLocalSocket_error_int_QLocalSocket(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_flush0, bool)
{
    NODE_RETURN(qt_QLocalSocket_flush_bool_QLocalSocket(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_fullServerName0, Pointer)
{
    NODE_RETURN(qt_QLocalSocket_fullServerName_string_QLocalSocket(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_isValid0, bool)
{
    NODE_RETURN(qt_QLocalSocket_isValid_bool_QLocalSocket(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_readBufferSize0, int64)
{
    NODE_RETURN(qt_QLocalSocket_readBufferSize_int64_QLocalSocket(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_serverName0, Pointer)
{
    NODE_RETURN(qt_QLocalSocket_serverName_string_QLocalSocket(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_setReadBufferSize0, void)
{
    qt_QLocalSocket_setReadBufferSize_void_QLocalSocket_int64(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int64));
}

static NODE_IMPLEMENTATION(_n_setServerName0, void)
{
    qt_QLocalSocket_setServerName_void_QLocalSocket_string(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
}

static NODE_IMPLEMENTATION(_n_state0, int)
{
    NODE_RETURN(qt_QLocalSocket_state_int_QLocalSocket(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_waitForConnected0, bool)
{
    NODE_RETURN(qt_QLocalSocket_waitForConnected_bool_QLocalSocket_int(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int)));
}

static NODE_IMPLEMENTATION(_n_waitForDisconnected0, bool)
{
    NODE_RETURN(qt_QLocalSocket_waitForDisconnected_bool_QLocalSocket_int(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int)));
}

static NODE_IMPLEMENTATION(_n_bytesAvailable0, int64)
{
    NODE_RETURN(qt_QLocalSocket_bytesAvailable_int64_QLocalSocket(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_bytesToWrite0, int64)
{
    NODE_RETURN(qt_QLocalSocket_bytesToWrite_int64_QLocalSocket(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_canReadLine0, bool)
{
    NODE_RETURN(qt_QLocalSocket_canReadLine_bool_QLocalSocket(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_close0, void)
{
    qt_QLocalSocket_close_void_QLocalSocket(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer));
}

static NODE_IMPLEMENTATION(_n_isSequential0, bool)
{
    NODE_RETURN(qt_QLocalSocket_isSequential_bool_QLocalSocket(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_open0, bool)
{
    NODE_RETURN(qt_QLocalSocket_open_bool_QLocalSocket_int(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int)));
}

static NODE_IMPLEMENTATION(_n_waitForBytesWritten0, bool)
{
    NODE_RETURN(qt_QLocalSocket_waitForBytesWritten_bool_QLocalSocket_int(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int)));
}

static NODE_IMPLEMENTATION(_n_waitForReadyRead0, bool)
{
    NODE_RETURN(qt_QLocalSocket_waitForReadyRead_bool_QLocalSocket_int(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int)));
}

static NODE_IMPLEMENTATION(_n_skipData0, int64)
{
    NODE_RETURN(qt_QLocalSocket_skipData_int64_QLocalSocket_int64(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int64)));
}

static NODE_IMPLEMENTATION(_n_atEnd0, bool)
{
    NODE_RETURN(qt_QLocalSocket_atEnd_bool_QLocalSocket(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_pos0, int64)
{
    NODE_RETURN(qt_QLocalSocket_pos_int64_QLocalSocket(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_reset0, bool)
{
    NODE_RETURN(qt_QLocalSocket_reset_bool_QLocalSocket(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_seek0, bool)
{
    NODE_RETURN(qt_QLocalSocket_seek_bool_QLocalSocket_int64(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int64)));
}

static NODE_IMPLEMENTATION(_n_size0, int64)
{
    NODE_RETURN(qt_QLocalSocket_size_int64_QLocalSocket(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}



void
QLocalSocketType::load()
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
                            Compiled, QLocalSocket_QLocalSocket_QObject,
                            Return, ftn,
                            Parameters,
                            new Param(c, "object", "qt.QObject"),
                            End),

               EndArguments );

addSymbols(
    // enums
    new Alias(c, "LocalSocketError", "int"),
      new SymbolicConstant(c, "ConnectionRefusedError", "int", Value(int(QLocalSocket::ConnectionRefusedError))),
      new SymbolicConstant(c, "PeerClosedError", "int", Value(int(QLocalSocket::PeerClosedError))),
      new SymbolicConstant(c, "ServerNotFoundError", "int", Value(int(QLocalSocket::ServerNotFoundError))),
      new SymbolicConstant(c, "SocketAccessError", "int", Value(int(QLocalSocket::SocketAccessError))),
      new SymbolicConstant(c, "SocketResourceError", "int", Value(int(QLocalSocket::SocketResourceError))),
      new SymbolicConstant(c, "SocketTimeoutError", "int", Value(int(QLocalSocket::SocketTimeoutError))),
      new SymbolicConstant(c, "DatagramTooLargeError", "int", Value(int(QLocalSocket::DatagramTooLargeError))),
      new SymbolicConstant(c, "ConnectionError", "int", Value(int(QLocalSocket::ConnectionError))),
      new SymbolicConstant(c, "UnsupportedSocketOperationError", "int", Value(int(QLocalSocket::UnsupportedSocketOperationError))),
      new SymbolicConstant(c, "OperationError", "int", Value(int(QLocalSocket::OperationError))),
      new SymbolicConstant(c, "UnknownSocketError", "int", Value(int(QLocalSocket::UnknownSocketError))),
    new Alias(c, "LocalSocketState", "int"),
      new SymbolicConstant(c, "UnconnectedState", "int", Value(int(QLocalSocket::UnconnectedState))),
      new SymbolicConstant(c, "ConnectingState", "int", Value(int(QLocalSocket::ConnectingState))),
      new SymbolicConstant(c, "ConnectedState", "int", Value(int(QLocalSocket::ConnectedState))),
      new SymbolicConstant(c, "ClosingState", "int", Value(int(QLocalSocket::ClosingState))),
    // member functions
    new Function(c, "QLocalSocket", _n_QLocalSocket0, None, Compiled, qt_QLocalSocket_QLocalSocket_QLocalSocket_QLocalSocket_QObject, Return, "qt.QLocalSocket", Parameters, new Param(c, "this", "qt.QLocalSocket"), new Param(c, "parent", "qt.QObject"), End),
    new Function(c, "abort", _n_abort0, None, Compiled, qt_QLocalSocket_abort_void_QLocalSocket, Return, "void", Parameters, new Param(c, "this", "qt.QLocalSocket"), End),
    // PROP: bindableSocketOptions (flags QBindable<QLocalSocket::SocketOptions>; QLocalSocket this)
    new Function(c, "connectToServer", _n_connectToServer0, None, Compiled, qt_QLocalSocket_connectToServer_void_QLocalSocket_int, Return, "void", Parameters, new Param(c, "this", "qt.QLocalSocket"), new Param(c, "openMode", "int", Value((int)QIODeviceBase::ReadWrite)), End),
    new Function(c, "connectToServer", _n_connectToServer1, None, Compiled, qt_QLocalSocket_connectToServer_void_QLocalSocket_string_int, Return, "void", Parameters, new Param(c, "this", "qt.QLocalSocket"), new Param(c, "name", "string"), new Param(c, "openMode", "int", Value((int)QIODeviceBase::ReadWrite)), End),
    new Function(c, "disconnectFromServer", _n_disconnectFromServer0, None, Compiled, qt_QLocalSocket_disconnectFromServer_void_QLocalSocket, Return, "void", Parameters, new Param(c, "this", "qt.QLocalSocket"), End),
    new Function(c, "error", _n_error0, None, Compiled, qt_QLocalSocket_error_int_QLocalSocket, Return, "int", Parameters, new Param(c, "this", "qt.QLocalSocket"), End),
    new Function(c, "flush", _n_flush0, None, Compiled, qt_QLocalSocket_flush_bool_QLocalSocket, Return, "bool", Parameters, new Param(c, "this", "qt.QLocalSocket"), End),
    new Function(c, "fullServerName", _n_fullServerName0, None, Compiled, qt_QLocalSocket_fullServerName_string_QLocalSocket, Return, "string", Parameters, new Param(c, "this", "qt.QLocalSocket"), End),
    new Function(c, "isValid", _n_isValid0, None, Compiled, qt_QLocalSocket_isValid_bool_QLocalSocket, Return, "bool", Parameters, new Param(c, "this", "qt.QLocalSocket"), End),
    new Function(c, "readBufferSize", _n_readBufferSize0, None, Compiled, qt_QLocalSocket_readBufferSize_int64_QLocalSocket, Return, "int64", Parameters, new Param(c, "this", "qt.QLocalSocket"), End),
    new Function(c, "serverName", _n_serverName0, None, Compiled, qt_QLocalSocket_serverName_string_QLocalSocket, Return, "string", Parameters, new Param(c, "this", "qt.QLocalSocket"), End),
    new Function(c, "setReadBufferSize", _n_setReadBufferSize0, None, Compiled, qt_QLocalSocket_setReadBufferSize_void_QLocalSocket_int64, Return, "void", Parameters, new Param(c, "this", "qt.QLocalSocket"), new Param(c, "size", "int64"), End),
    new Function(c, "setServerName", _n_setServerName0, None, Compiled, qt_QLocalSocket_setServerName_void_QLocalSocket_string, Return, "void", Parameters, new Param(c, "this", "qt.QLocalSocket"), new Param(c, "name", "string"), End),
    // MISSING: setSocketDescriptor (bool; QLocalSocket this, "qintptr" socketDescriptor, flags QLocalSocket::LocalSocketState socketState, flags QIODeviceBase::OpenMode openMode)
    // PROP: setSocketOptions (void; QLocalSocket this, flags QLocalSocket::SocketOptions option)
    // MISSING: socketDescriptor ("qintptr"; QLocalSocket this)
    // PROP: socketOptions (flags QLocalSocket::SocketOptions; QLocalSocket this)
    new Function(c, "state", _n_state0, None, Compiled, qt_QLocalSocket_state_int_QLocalSocket, Return, "int", Parameters, new Param(c, "this", "qt.QLocalSocket"), End),
    new Function(c, "waitForConnected", _n_waitForConnected0, None, Compiled, qt_QLocalSocket_waitForConnected_bool_QLocalSocket_int, Return, "bool", Parameters, new Param(c, "this", "qt.QLocalSocket"), new Param(c, "msecs", "int", Value((int)30000)), End),
    new Function(c, "waitForDisconnected", _n_waitForDisconnected0, None, Compiled, qt_QLocalSocket_waitForDisconnected_bool_QLocalSocket_int, Return, "bool", Parameters, new Param(c, "this", "qt.QLocalSocket"), new Param(c, "msecs", "int", Value((int)30000)), End),
    _func[0] = new MemberFunction(c, "bytesAvailable", _n_bytesAvailable0, None, Compiled, qt_QLocalSocket_bytesAvailable_int64_QLocalSocket, Return, "int64", Parameters, new Param(c, "this", "qt.QLocalSocket"), End),
    _func[1] = new MemberFunction(c, "bytesToWrite", _n_bytesToWrite0, None, Compiled, qt_QLocalSocket_bytesToWrite_int64_QLocalSocket, Return, "int64", Parameters, new Param(c, "this", "qt.QLocalSocket"), End),
    _func[2] = new MemberFunction(c, "canReadLine", _n_canReadLine0, None, Compiled, qt_QLocalSocket_canReadLine_bool_QLocalSocket, Return, "bool", Parameters, new Param(c, "this", "qt.QLocalSocket"), End),
    _func[3] = new MemberFunction(c, "close", _n_close0, None, Compiled, qt_QLocalSocket_close_void_QLocalSocket, Return, "void", Parameters, new Param(c, "this", "qt.QLocalSocket"), End),
    _func[4] = new MemberFunction(c, "isSequential", _n_isSequential0, None, Compiled, qt_QLocalSocket_isSequential_bool_QLocalSocket, Return, "bool", Parameters, new Param(c, "this", "qt.QLocalSocket"), End),
    _func[5] = new MemberFunction(c, "open", _n_open0, None, Compiled, qt_QLocalSocket_open_bool_QLocalSocket_int, Return, "bool", Parameters, new Param(c, "this", "qt.QLocalSocket"), new Param(c, "openMode", "int", Value((int)QIODeviceBase::ReadWrite)), End),
    _func[6] = new MemberFunction(c, "waitForBytesWritten", _n_waitForBytesWritten0, None, Compiled, qt_QLocalSocket_waitForBytesWritten_bool_QLocalSocket_int, Return, "bool", Parameters, new Param(c, "this", "qt.QLocalSocket"), new Param(c, "msecs", "int", Value((int)30000)), End),
    _func[7] = new MemberFunction(c, "waitForReadyRead", _n_waitForReadyRead0, None, Compiled, qt_QLocalSocket_waitForReadyRead_bool_QLocalSocket_int, Return, "bool", Parameters, new Param(c, "this", "qt.QLocalSocket"), new Param(c, "msecs", "int", Value((int)30000)), End),
    // MISSING: readData (int64; QLocalSocket this, "char *" data, int64 c) // protected
    // MISSING: readLineData (int64; QLocalSocket this, "char *" data, int64 maxSize) // protected
    _func[8] = new MemberFunction(c, "skipData", _n_skipData0, None, Compiled, qt_QLocalSocket_skipData_int64_QLocalSocket_int64, Return, "int64", Parameters, new Param(c, "this", "qt.QLocalSocket"), new Param(c, "maxSize", "int64"), End),
    // MISSING: writeData (int64; QLocalSocket this, "const char *" data, int64 c) // protected
    _func[9] = new MemberFunction(c, "atEnd", _n_atEnd0, None, Compiled, qt_QLocalSocket_atEnd_bool_QLocalSocket, Return, "bool", Parameters, new Param(c, "this", "qt.QLocalSocket"), End),
    _func[10] = new MemberFunction(c, "pos", _n_pos0, None, Compiled, qt_QLocalSocket_pos_int64_QLocalSocket, Return, "int64", Parameters, new Param(c, "this", "qt.QLocalSocket"), End),
    _func[11] = new MemberFunction(c, "reset", _n_reset0, None, Compiled, qt_QLocalSocket_reset_bool_QLocalSocket, Return, "bool", Parameters, new Param(c, "this", "qt.QLocalSocket"), End),
    _func[12] = new MemberFunction(c, "seek", _n_seek0, None, Compiled, qt_QLocalSocket_seek_bool_QLocalSocket_int64, Return, "bool", Parameters, new Param(c, "this", "qt.QLocalSocket"), new Param(c, "pos", "int64"), End),
    _func[13] = new MemberFunction(c, "size", _n_size0, None, Compiled, qt_QLocalSocket_size_int64_QLocalSocket, Return, "int64", Parameters, new Param(c, "this", "qt.QLocalSocket"), End),
    // static functions
    EndArguments);
globalScope()->addSymbols(
    EndArguments);
scope()->addSymbols(
    EndArguments);


    const char** propExclusions = 0;

    populate(this, QLocalSocket::staticMetaObject, propExclusions);
}

} // Mu
