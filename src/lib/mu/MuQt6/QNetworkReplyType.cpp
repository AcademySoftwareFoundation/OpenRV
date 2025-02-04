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
#include <MuQt6/QNetworkReplyType.h>
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
#include <MuQt6/QNetworkAccessManagerType.h>
#include <MuQt6/QUrlType.h>
#include <MuQt6/QByteArrayType.h>
#include <MuQt6/QVariantType.h>

namespace Mu
{
    using namespace std;

    //----------------------------------------------------------------------
    //  INHERITABLE TYPE IMPLEMENTATION

    //----------------------------------------------------------------------
    //  Mu Type CONSTRUCTORS

    QNetworkReplyType::QNetworkReplyType(Context* c, const char* name,
                                         Class* super, Class* super2)
        : Class(c, name, vectorOf2(super, super2))
    {
    }

    QNetworkReplyType::~QNetworkReplyType() {}

    //----------------------------------------------------------------------
    //  PRE-COMPILED FUNCTIONS

    static Pointer QNetworkReply_QNetworkReply_QObject(Thread& NODE_THREAD,
                                                       Pointer obj)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        ClassInstance* widget = reinterpret_cast<ClassInstance*>(obj);

        if (!widget)
        {
            return 0;
        }
        else if (QNetworkReply* w = object<QNetworkReply>(widget))
        {
            QNetworkReplyType* type =
                c->findSymbolOfTypeByQualifiedName<QNetworkReplyType>(
                    c->internName("qt.QNetworkReply"), false);
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
        NODE_RETURN(QNetworkReply_QNetworkReply_QObject(NODE_THREAD,
                                                        NODE_ARG(0, Pointer)));
    }

    Pointer qt_QNetworkReply_attribute_QVariant_QNetworkReply_int(
        Mu::Thread& NODE_THREAD, Pointer param_this, int param_code)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QNetworkReply* arg0 = object<QNetworkReply>(param_this);
        QNetworkRequest::Attribute arg1 =
            (QNetworkRequest::Attribute)(param_code);
        return makeqtype<QVariantType>(c, arg0->attribute(arg1), "qt.QVariant");
    }

    int qt_QNetworkReply_error_int_QNetworkReply(Mu::Thread& NODE_THREAD,
                                                 Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QNetworkReply* arg0 = object<QNetworkReply>(param_this);
        return int(arg0->error());
    }

    bool qt_QNetworkReply_hasRawHeader_bool_QNetworkReply_QByteArray(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_headerName)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QNetworkReply* arg0 = object<QNetworkReply>(param_this);
        const QByteArray arg1 = getqtype<QByteArrayType>(param_headerName);
        return arg0->hasRawHeader(arg1);
    }

    Pointer qt_QNetworkReply_header_QVariant_QNetworkReply_int(
        Mu::Thread& NODE_THREAD, Pointer param_this, int param_header_)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QNetworkReply* arg0 = object<QNetworkReply>(param_this);
        QNetworkRequest::KnownHeaders arg1 =
            (QNetworkRequest::KnownHeaders)(param_header_);
        return makeqtype<QVariantType>(c, arg0->header(arg1), "qt.QVariant");
    }

    bool qt_QNetworkReply_isFinished_bool_QNetworkReply(Mu::Thread& NODE_THREAD,
                                                        Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QNetworkReply* arg0 = object<QNetworkReply>(param_this);
        return arg0->isFinished();
    }

    bool qt_QNetworkReply_isRunning_bool_QNetworkReply(Mu::Thread& NODE_THREAD,
                                                       Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QNetworkReply* arg0 = object<QNetworkReply>(param_this);
        return arg0->isRunning();
    }

    Pointer qt_QNetworkReply_manager_QNetworkAccessManager_QNetworkReply(
        Mu::Thread& NODE_THREAD, Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QNetworkReply* arg0 = object<QNetworkReply>(param_this);
        return makeinstance<QNetworkAccessManagerType>(
            c, arg0->manager(), "qt.QNetworkAccessManager");
    }

    int qt_QNetworkReply_operation_int_QNetworkReply(Mu::Thread& NODE_THREAD,
                                                     Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QNetworkReply* arg0 = object<QNetworkReply>(param_this);
        return int(arg0->operation());
    }

    Pointer qt_QNetworkReply_rawHeader_QByteArray_QNetworkReply_QByteArray(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_headerName)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QNetworkReply* arg0 = object<QNetworkReply>(param_this);
        const QByteArray arg1 = getqtype<QByteArrayType>(param_headerName);
        return makeqtype<QByteArrayType>(c, arg0->rawHeader(arg1),
                                         "qt.QByteArray");
    }

    int64
    qt_QNetworkReply_readBufferSize_int64_QNetworkReply(Mu::Thread& NODE_THREAD,
                                                        Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QNetworkReply* arg0 = object<QNetworkReply>(param_this);
        return arg0->readBufferSize();
    }

    void qt_QNetworkReply_setReadBufferSize_void_QNetworkReply_int64(
        Mu::Thread& NODE_THREAD, Pointer param_this, int64 param_size)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QNetworkReply* arg0 = object<QNetworkReply>(param_this);
        qint64 arg1 = (int64)(param_size);
        arg0->setReadBufferSize(arg1);
    }

    Pointer qt_QNetworkReply_url_QUrl_QNetworkReply(Mu::Thread& NODE_THREAD,
                                                    Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QNetworkReply* arg0 = object<QNetworkReply>(param_this);
        return makeqtype<QUrlType>(c, arg0->url(), "qt.QUrl");
    }

    void qt_QNetworkReply_close_void_QNetworkReply(Mu::Thread& NODE_THREAD,
                                                   Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QNetworkReply* arg0 = object<QNetworkReply>(param_this);
        arg0->close();
    }

    bool qt_QNetworkReply_atEnd_bool_QNetworkReply(Mu::Thread& NODE_THREAD,
                                                   Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QNetworkReply* arg0 = object<QNetworkReply>(param_this);
        return arg0->atEnd();
    }

    int64
    qt_QNetworkReply_bytesAvailable_int64_QNetworkReply(Mu::Thread& NODE_THREAD,
                                                        Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QNetworkReply* arg0 = object<QNetworkReply>(param_this);
        return arg0->bytesAvailable();
    }

    int64
    qt_QNetworkReply_bytesToWrite_int64_QNetworkReply(Mu::Thread& NODE_THREAD,
                                                      Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QNetworkReply* arg0 = object<QNetworkReply>(param_this);
        return arg0->bytesToWrite();
    }

    bool
    qt_QNetworkReply_canReadLine_bool_QNetworkReply(Mu::Thread& NODE_THREAD,
                                                    Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QNetworkReply* arg0 = object<QNetworkReply>(param_this);
        return arg0->canReadLine();
    }

    bool
    qt_QNetworkReply_isSequential_bool_QNetworkReply(Mu::Thread& NODE_THREAD,
                                                     Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QNetworkReply* arg0 = object<QNetworkReply>(param_this);
        return arg0->isSequential();
    }

    bool qt_QNetworkReply_open_bool_QNetworkReply_int(Mu::Thread& NODE_THREAD,
                                                      Pointer param_this,
                                                      int param_mode)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QNetworkReply* arg0 = object<QNetworkReply>(param_this);
        QIODeviceBase::OpenMode arg1 = (QIODeviceBase::OpenMode)(param_mode);
        return arg0->open(arg1);
    }

    int64 qt_QNetworkReply_pos_int64_QNetworkReply(Mu::Thread& NODE_THREAD,
                                                   Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QNetworkReply* arg0 = object<QNetworkReply>(param_this);
        return arg0->pos();
    }

    bool qt_QNetworkReply_reset_bool_QNetworkReply(Mu::Thread& NODE_THREAD,
                                                   Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QNetworkReply* arg0 = object<QNetworkReply>(param_this);
        return arg0->reset();
    }

    bool qt_QNetworkReply_seek_bool_QNetworkReply_int64(Mu::Thread& NODE_THREAD,
                                                        Pointer param_this,
                                                        int64 param_pos)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QNetworkReply* arg0 = object<QNetworkReply>(param_this);
        qint64 arg1 = (int64)(param_pos);
        return arg0->seek(arg1);
    }

    int64 qt_QNetworkReply_size_int64_QNetworkReply(Mu::Thread& NODE_THREAD,
                                                    Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QNetworkReply* arg0 = object<QNetworkReply>(param_this);
        return arg0->size();
    }

    bool qt_QNetworkReply_waitForBytesWritten_bool_QNetworkReply_int(
        Mu::Thread& NODE_THREAD, Pointer param_this, int param_msecs)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QNetworkReply* arg0 = object<QNetworkReply>(param_this);
        int arg1 = (int)(param_msecs);
        return arg0->waitForBytesWritten(arg1);
    }

    bool qt_QNetworkReply_waitForReadyRead_bool_QNetworkReply_int(
        Mu::Thread& NODE_THREAD, Pointer param_this, int param_msecs)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QNetworkReply* arg0 = object<QNetworkReply>(param_this);
        int arg1 = (int)(param_msecs);
        return arg0->waitForReadyRead(arg1);
    }

    static NODE_IMPLEMENTATION(_n_attribute0, Pointer)
    {
        NODE_RETURN(qt_QNetworkReply_attribute_QVariant_QNetworkReply_int(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int)));
    }

    static NODE_IMPLEMENTATION(_n_error0, int)
    {
        NODE_RETURN(qt_QNetworkReply_error_int_QNetworkReply(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_hasRawHeader0, bool)
    {
        NODE_RETURN(qt_QNetworkReply_hasRawHeader_bool_QNetworkReply_QByteArray(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_header0, Pointer)
    {
        NODE_RETURN(qt_QNetworkReply_header_QVariant_QNetworkReply_int(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int)));
    }

    static NODE_IMPLEMENTATION(_n_isFinished0, bool)
    {
        NODE_RETURN(qt_QNetworkReply_isFinished_bool_QNetworkReply(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_isRunning0, bool)
    {
        NODE_RETURN(qt_QNetworkReply_isRunning_bool_QNetworkReply(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_manager0, Pointer)
    {
        NODE_RETURN(
            qt_QNetworkReply_manager_QNetworkAccessManager_QNetworkReply(
                NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_operation0, int)
    {
        NODE_RETURN(qt_QNetworkReply_operation_int_QNetworkReply(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_rawHeader0, Pointer)
    {
        NODE_RETURN(
            qt_QNetworkReply_rawHeader_QByteArray_QNetworkReply_QByteArray(
                NODE_THREAD, NONNIL_NODE_ARG(0, Pointer),
                NODE_ARG(1, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_readBufferSize0, int64)
    {
        NODE_RETURN(qt_QNetworkReply_readBufferSize_int64_QNetworkReply(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_setReadBufferSize0, void)
    {
        qt_QNetworkReply_setReadBufferSize_void_QNetworkReply_int64(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int64));
    }

    static NODE_IMPLEMENTATION(_n_url0, Pointer)
    {
        NODE_RETURN(qt_QNetworkReply_url_QUrl_QNetworkReply(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_close0, void)
    {
        qt_QNetworkReply_close_void_QNetworkReply(NODE_THREAD,
                                                  NONNIL_NODE_ARG(0, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_atEnd0, bool)
    {
        NODE_RETURN(qt_QNetworkReply_atEnd_bool_QNetworkReply(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_bytesAvailable0, int64)
    {
        NODE_RETURN(qt_QNetworkReply_bytesAvailable_int64_QNetworkReply(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_bytesToWrite0, int64)
    {
        NODE_RETURN(qt_QNetworkReply_bytesToWrite_int64_QNetworkReply(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_canReadLine0, bool)
    {
        NODE_RETURN(qt_QNetworkReply_canReadLine_bool_QNetworkReply(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_isSequential0, bool)
    {
        NODE_RETURN(qt_QNetworkReply_isSequential_bool_QNetworkReply(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_open0, bool)
    {
        NODE_RETURN(qt_QNetworkReply_open_bool_QNetworkReply_int(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int)));
    }

    static NODE_IMPLEMENTATION(_n_pos0, int64)
    {
        NODE_RETURN(qt_QNetworkReply_pos_int64_QNetworkReply(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_reset0, bool)
    {
        NODE_RETURN(qt_QNetworkReply_reset_bool_QNetworkReply(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_seek0, bool)
    {
        NODE_RETURN(qt_QNetworkReply_seek_bool_QNetworkReply_int64(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int64)));
    }

    static NODE_IMPLEMENTATION(_n_size0, int64)
    {
        NODE_RETURN(qt_QNetworkReply_size_int64_QNetworkReply(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_waitForBytesWritten0, bool)
    {
        NODE_RETURN(qt_QNetworkReply_waitForBytesWritten_bool_QNetworkReply_int(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int)));
    }

    static NODE_IMPLEMENTATION(_n_waitForReadyRead0, bool)
    {
        NODE_RETURN(qt_QNetworkReply_waitForReadyRead_bool_QNetworkReply_int(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int)));
    }

    void QNetworkReplyType::load()
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
                                QNetworkReply_QNetworkReply_QObject, Return,
                                ftn, Parameters,
                                new Param(c, "object", "qt.QObject"), End),

                   EndArguments);

        addSymbols(
            // enums
            // member functions
            new Function(c, "attribute", _n_attribute0, None, Compiled,
                         qt_QNetworkReply_attribute_QVariant_QNetworkReply_int,
                         Return, "qt.QVariant", Parameters,
                         new Param(c, "this", "qt.QNetworkReply"),
                         new Param(c, "code", "int"), End),
            new Function(c, "error", _n_error0, None, Compiled,
                         qt_QNetworkReply_error_int_QNetworkReply, Return,
                         "int", Parameters,
                         new Param(c, "this", "qt.QNetworkReply"), End),
            new Function(
                c, "hasRawHeader", _n_hasRawHeader0, None, Compiled,
                qt_QNetworkReply_hasRawHeader_bool_QNetworkReply_QByteArray,
                Return, "bool", Parameters,
                new Param(c, "this", "qt.QNetworkReply"),
                new Param(c, "headerName", "qt.QByteArray"), End),
            new Function(c, "header", _n_header0, None, Compiled,
                         qt_QNetworkReply_header_QVariant_QNetworkReply_int,
                         Return, "qt.QVariant", Parameters,
                         new Param(c, "this", "qt.QNetworkReply"),
                         new Param(c, "header_", "int"), End),
            // MISSING: ignoreSslErrors (void; QNetworkReply this, "const
            // QList<QSslError> &" errors)
            new Function(c, "isFinished", _n_isFinished0, None, Compiled,
                         qt_QNetworkReply_isFinished_bool_QNetworkReply, Return,
                         "bool", Parameters,
                         new Param(c, "this", "qt.QNetworkReply"), End),
            new Function(c, "isRunning", _n_isRunning0, None, Compiled,
                         qt_QNetworkReply_isRunning_bool_QNetworkReply, Return,
                         "bool", Parameters,
                         new Param(c, "this", "qt.QNetworkReply"), End),
            new Function(
                c, "manager", _n_manager0, None, Compiled,
                qt_QNetworkReply_manager_QNetworkAccessManager_QNetworkReply,
                Return, "qt.QNetworkAccessManager", Parameters,
                new Param(c, "this", "qt.QNetworkReply"), End),
            new Function(c, "operation", _n_operation0, None, Compiled,
                         qt_QNetworkReply_operation_int_QNetworkReply, Return,
                         "int", Parameters,
                         new Param(c, "this", "qt.QNetworkReply"), End),
            new Function(
                c, "rawHeader", _n_rawHeader0, None, Compiled,
                qt_QNetworkReply_rawHeader_QByteArray_QNetworkReply_QByteArray,
                Return, "qt.QByteArray", Parameters,
                new Param(c, "this", "qt.QNetworkReply"),
                new Param(c, "headerName", "qt.QByteArray"), End),
            // MISSING: rawHeaderList ("QList<QByteArray>"; QNetworkReply this)
            new Function(c, "readBufferSize", _n_readBufferSize0, None,
                         Compiled,
                         qt_QNetworkReply_readBufferSize_int64_QNetworkReply,
                         Return, "int64", Parameters,
                         new Param(c, "this", "qt.QNetworkReply"), End),
            // MISSING: request ("QNetworkRequest"; QNetworkReply this)
            new MemberFunction(
                c, "setReadBufferSize", _n_setReadBufferSize0, None, Compiled,
                qt_QNetworkReply_setReadBufferSize_void_QNetworkReply_int64,
                Return, "void", Parameters,
                new Param(c, "this", "qt.QNetworkReply"),
                new Param(c, "size", "int64"), End),
            // MISSING: setSslConfiguration (void; QNetworkReply this, "const
            // QSslConfiguration &" config) MISSING: sslConfiguration
            // ("QSslConfiguration"; QNetworkReply this)
            new Function(c, "url", _n_url0, None, Compiled,
                         qt_QNetworkReply_url_QUrl_QNetworkReply, Return,
                         "qt.QUrl", Parameters,
                         new Param(c, "this", "qt.QNetworkReply"), End),
            new MemberFunction(c, "close", _n_close0, None, Compiled,
                               qt_QNetworkReply_close_void_QNetworkReply,
                               Return, "void", Parameters,
                               new Param(c, "this", "qt.QNetworkReply"), End),
            // NOT INHERITABLE PROTECTED: QNetworkReply (QNetworkReply;
            // QNetworkReply this, QObject parent) // protected MISSING:
            // ignoreSslErrorsImplementation (void; QNetworkReply this, "const
            // QList<QSslError> &" errors) // protected NOT INHERITABLE
            // PROTECTED: setAttribute (void; QNetworkReply this, flags
            // QNetworkRequest::Attribute code, QVariant value) // protected NOT
            // INHERITABLE PROTECTED: setError (void; QNetworkReply this, flags
            // QNetworkReply::NetworkError errorCode, string errorString) //
            // protected NOT INHERITABLE PROTECTED: setFinished (void;
            // QNetworkReply this, bool finished) // protected NOT INHERITABLE
            // PROTECTED: setHeader (void; QNetworkReply this, flags
            // QNetworkRequest::KnownHeaders header, QVariant value) //
            // protected NOT INHERITABLE PROTECTED: setOperation (void;
            // QNetworkReply this, flags QNetworkAccessManager::Operation
            // operation) // protected NOT INHERITABLE PROTECTED: setRawHeader
            // (void; QNetworkReply this, QByteArray headerName, QByteArray
            // value) // protected MISSING: setRequest (void; QNetworkReply
            // this, "const QNetworkRequest &" request) // protected MISSING:
            // setSslConfigurationImplementation (void; QNetworkReply this,
            // "const QSslConfiguration &" configuration) // protected NOT
            // INHERITABLE PROTECTED: setUrl (void; QNetworkReply this, QUrl
            // url) // protected MISSING: sslConfigurationImplementation (void;
            // QNetworkReply this, "QSslConfiguration &" configuration) //
            // protected
            new MemberFunction(c, "atEnd", _n_atEnd0, None, Compiled,
                               qt_QNetworkReply_atEnd_bool_QNetworkReply,
                               Return, "bool", Parameters,
                               new Param(c, "this", "qt.QNetworkReply"), End),
            new MemberFunction(
                c, "bytesAvailable", _n_bytesAvailable0, None, Compiled,
                qt_QNetworkReply_bytesAvailable_int64_QNetworkReply, Return,
                "int64", Parameters, new Param(c, "this", "qt.QNetworkReply"),
                End),
            new MemberFunction(
                c, "bytesToWrite", _n_bytesToWrite0, None, Compiled,
                qt_QNetworkReply_bytesToWrite_int64_QNetworkReply, Return,
                "int64", Parameters, new Param(c, "this", "qt.QNetworkReply"),
                End),
            new MemberFunction(
                c, "canReadLine", _n_canReadLine0, None, Compiled,
                qt_QNetworkReply_canReadLine_bool_QNetworkReply, Return, "bool",
                Parameters, new Param(c, "this", "qt.QNetworkReply"), End),
            new MemberFunction(c, "isSequential", _n_isSequential0, None,
                               Compiled,
                               qt_QNetworkReply_isSequential_bool_QNetworkReply,
                               Return, "bool", Parameters,
                               new Param(c, "this", "qt.QNetworkReply"), End),
            new MemberFunction(c, "open", _n_open0, None, Compiled,
                               qt_QNetworkReply_open_bool_QNetworkReply_int,
                               Return, "bool", Parameters,
                               new Param(c, "this", "qt.QNetworkReply"),
                               new Param(c, "mode", "int"), End),
            new MemberFunction(c, "pos", _n_pos0, None, Compiled,
                               qt_QNetworkReply_pos_int64_QNetworkReply, Return,
                               "int64", Parameters,
                               new Param(c, "this", "qt.QNetworkReply"), End),
            new MemberFunction(c, "reset", _n_reset0, None, Compiled,
                               qt_QNetworkReply_reset_bool_QNetworkReply,
                               Return, "bool", Parameters,
                               new Param(c, "this", "qt.QNetworkReply"), End),
            new MemberFunction(c, "seek", _n_seek0, None, Compiled,
                               qt_QNetworkReply_seek_bool_QNetworkReply_int64,
                               Return, "bool", Parameters,
                               new Param(c, "this", "qt.QNetworkReply"),
                               new Param(c, "pos", "int64"), End),
            new MemberFunction(c, "size", _n_size0, None, Compiled,
                               qt_QNetworkReply_size_int64_QNetworkReply,
                               Return, "int64", Parameters,
                               new Param(c, "this", "qt.QNetworkReply"), End),
            new MemberFunction(
                c, "waitForBytesWritten", _n_waitForBytesWritten0, None,
                Compiled,
                qt_QNetworkReply_waitForBytesWritten_bool_QNetworkReply_int,
                Return, "bool", Parameters,
                new Param(c, "this", "qt.QNetworkReply"),
                new Param(c, "msecs", "int"), End),
            new MemberFunction(
                c, "waitForReadyRead", _n_waitForReadyRead0, None, Compiled,
                qt_QNetworkReply_waitForReadyRead_bool_QNetworkReply_int,
                Return, "bool", Parameters,
                new Param(c, "this", "qt.QNetworkReply"),
                new Param(c, "msecs", "int"), End),
            // MISSING: readData (int64; QNetworkReply this, "char *" data,
            // int64 maxSize) // protected MISSING: readLineData (int64;
            // QNetworkReply this, "char *" data, int64 maxSize) // protected
            // NOT INHERITABLE PROTECTED: skipData (int64; QNetworkReply this,
            // int64 maxSize) // protected MISSING: writeData (int64;
            // QNetworkReply this, "const char *" data, int64 maxSize) //
            // protected static functions
            EndArguments);
        globalScope()->addSymbols(EndArguments);
        scope()->addSymbols(EndArguments);

        const char** propExclusions = 0;

        populate(this, QNetworkReply::staticMetaObject, propExclusions);
    }

} // namespace Mu
