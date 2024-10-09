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
#include <MuQt6/QNetworkCookieType.h>
#include <MuQt6/QActionType.h>
#include <MuQt6/QWidgetType.h>
#include <Mu/Alias.h>
#include <Mu/BaseFunctions.h>
#include <Mu/ClassInstance.h>
#include <Mu/Exception.h>
#include <Mu/Function.h>
#include <Mu/MemberFunction.h>
#include <Mu/MemberVariable.h>
#include <Mu/Node.h>
#include <Mu/ParameterVariable.h>
#include <Mu/ReferenceType.h>
#include <Mu/SymbolicConstant.h>
#include <Mu/Thread.h>
#include <Mu/Value.h>
#include <MuLang/MuLangContext.h>
#include <MuLang/StringType.h>
#include <QtGui/QtGui>
#include <QtWidgets/QtWidgets>
#include <QtSvg/QtSvg>
#include <QtNetwork/QtNetwork>
#include <MuQt6/QDateTimeType.h>
#include <MuQt6/QUrlType.h>
#include <MuQt6/QByteArrayType.h>

namespace Mu {
using namespace std;

QNetworkCookieType::Instance::Instance(const Class* c) : ClassInstance(c)
{
}

QNetworkCookieType::QNetworkCookieType(Context* c, const char* name, Class* super)
    : Class(c, name, super)
{
}

QNetworkCookieType::~QNetworkCookieType()
{
}

static NODE_IMPLEMENTATION(__allocate, Pointer)
{
    QNetworkCookieType::Instance* i = new QNetworkCookieType::Instance((Class*)NODE_THIS.type());
    QNetworkCookieType::registerFinalizer(i);
    NODE_RETURN(i);
}

void 
QNetworkCookieType::registerFinalizer (void* o)
{
    GC_register_finalizer(o, QNetworkCookieType::finalizer, 0, 0, 0);
}

void 
QNetworkCookieType::finalizer (void* obj, void* data)
{
    QNetworkCookieType::Instance* i = reinterpret_cast<QNetworkCookieType::Instance*>(obj);
    delete i;
}

//----------------------------------------------------------------------
//  PRE-COMPILED FUNCTIONS

Pointer qt_QNetworkCookie_QNetworkCookie_QNetworkCookie_QNetworkCookie_QByteArray_QByteArray(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_name, Pointer param_value)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QByteArray  arg1 = getqtype<QByteArrayType>(param_name);
    const QByteArray  arg2 = getqtype<QByteArrayType>(param_value);
    setqtype<QNetworkCookieType>(param_this,QNetworkCookie(arg1, arg2));
    return param_this;
}

Pointer qt_QNetworkCookie_domain_string_QNetworkCookie(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QNetworkCookie& arg0 = getqtype<QNetworkCookieType>(param_this);
    return makestring(c,arg0.domain());
}

Pointer qt_QNetworkCookie_expirationDate_QDateTime_QNetworkCookie(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QNetworkCookie& arg0 = getqtype<QNetworkCookieType>(param_this);
    return makeqtype<QDateTimeType>(c,arg0.expirationDate(),"qt.QDateTime");
}

bool qt_QNetworkCookie_hasSameIdentifier_bool_QNetworkCookie_QNetworkCookie(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_other)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QNetworkCookie& arg0 = getqtype<QNetworkCookieType>(param_this);
    const QNetworkCookie  arg1 = getqtype<QNetworkCookieType>(param_other);
    return arg0.hasSameIdentifier(arg1);
}

bool qt_QNetworkCookie_isHttpOnly_bool_QNetworkCookie(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QNetworkCookie& arg0 = getqtype<QNetworkCookieType>(param_this);
    return arg0.isHttpOnly();
}

bool qt_QNetworkCookie_isSecure_bool_QNetworkCookie(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QNetworkCookie& arg0 = getqtype<QNetworkCookieType>(param_this);
    return arg0.isSecure();
}

bool qt_QNetworkCookie_isSessionCookie_bool_QNetworkCookie(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QNetworkCookie& arg0 = getqtype<QNetworkCookieType>(param_this);
    return arg0.isSessionCookie();
}

Pointer qt_QNetworkCookie_name_QByteArray_QNetworkCookie(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QNetworkCookie& arg0 = getqtype<QNetworkCookieType>(param_this);
    return makeqtype<QByteArrayType>(c,arg0.name(),"qt.QByteArray");
}

void qt_QNetworkCookie_normalize_void_QNetworkCookie_QUrl(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_url)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QNetworkCookie& arg0 = getqtype<QNetworkCookieType>(param_this);
    const QUrl  arg1 = getqtype<QUrlType>(param_url);
    arg0.normalize(arg1);
    setqtype<QNetworkCookieType>(param_this,arg0);
}

Pointer qt_QNetworkCookie_path_string_QNetworkCookie(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QNetworkCookie& arg0 = getqtype<QNetworkCookieType>(param_this);
    return makestring(c,arg0.path());
}

int qt_QNetworkCookie_sameSitePolicy_int_QNetworkCookie(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QNetworkCookie& arg0 = getqtype<QNetworkCookieType>(param_this);
    return int(arg0.sameSitePolicy());
}

void qt_QNetworkCookie_setDomain_void_QNetworkCookie_string(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_domain)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QNetworkCookie& arg0 = getqtype<QNetworkCookieType>(param_this);
    const QString  arg1 = qstring(param_domain);
    arg0.setDomain(arg1);
    setqtype<QNetworkCookieType>(param_this,arg0);
}

void qt_QNetworkCookie_setExpirationDate_void_QNetworkCookie_QDateTime(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_date)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QNetworkCookie& arg0 = getqtype<QNetworkCookieType>(param_this);
    const QDateTime  arg1 = getqtype<QDateTimeType>(param_date);
    arg0.setExpirationDate(arg1);
    setqtype<QNetworkCookieType>(param_this,arg0);
}

void qt_QNetworkCookie_setHttpOnly_void_QNetworkCookie_bool(Mu::Thread& NODE_THREAD, Pointer param_this, bool param_enable)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QNetworkCookie& arg0 = getqtype<QNetworkCookieType>(param_this);
    bool arg1 = (bool)(param_enable);
    arg0.setHttpOnly(arg1);
    setqtype<QNetworkCookieType>(param_this,arg0);
}

void qt_QNetworkCookie_setName_void_QNetworkCookie_QByteArray(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_cookieName)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QNetworkCookie& arg0 = getqtype<QNetworkCookieType>(param_this);
    const QByteArray  arg1 = getqtype<QByteArrayType>(param_cookieName);
    arg0.setName(arg1);
    setqtype<QNetworkCookieType>(param_this,arg0);
}

void qt_QNetworkCookie_setPath_void_QNetworkCookie_string(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_path)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QNetworkCookie& arg0 = getqtype<QNetworkCookieType>(param_this);
    const QString  arg1 = qstring(param_path);
    arg0.setPath(arg1);
    setqtype<QNetworkCookieType>(param_this,arg0);
}

void qt_QNetworkCookie_setSameSitePolicy_void_QNetworkCookie_int(Mu::Thread& NODE_THREAD, Pointer param_this, int param_sameSite)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QNetworkCookie& arg0 = getqtype<QNetworkCookieType>(param_this);
    QNetworkCookie::SameSite arg1 = (QNetworkCookie::SameSite)(param_sameSite);
    arg0.setSameSitePolicy(arg1);
    setqtype<QNetworkCookieType>(param_this,arg0);
}

void qt_QNetworkCookie_setSecure_void_QNetworkCookie_bool(Mu::Thread& NODE_THREAD, Pointer param_this, bool param_enable)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QNetworkCookie& arg0 = getqtype<QNetworkCookieType>(param_this);
    bool arg1 = (bool)(param_enable);
    arg0.setSecure(arg1);
    setqtype<QNetworkCookieType>(param_this,arg0);
}

void qt_QNetworkCookie_setValue_void_QNetworkCookie_QByteArray(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_value)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QNetworkCookie& arg0 = getqtype<QNetworkCookieType>(param_this);
    const QByteArray  arg1 = getqtype<QByteArrayType>(param_value);
    arg0.setValue(arg1);
    setqtype<QNetworkCookieType>(param_this,arg0);
}

void qt_QNetworkCookie_swap_void_QNetworkCookie_QNetworkCookie(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_other)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    QNetworkCookie& arg0 = getqtype<QNetworkCookieType>(param_this);
    QNetworkCookie  arg1 = getqtype<QNetworkCookieType>(param_other);
    arg0.swap(arg1);
    setqtype<QNetworkCookieType>(param_this,arg0);
}

Pointer qt_QNetworkCookie_toRawForm_QByteArray_QNetworkCookie_int(Mu::Thread& NODE_THREAD, Pointer param_this, int param_form)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QNetworkCookie& arg0 = getqtype<QNetworkCookieType>(param_this);
    QNetworkCookie::RawForm arg1 = (QNetworkCookie::RawForm)(param_form);
    return makeqtype<QByteArrayType>(c,arg0.toRawForm(arg1),"qt.QByteArray");
}

Pointer qt_QNetworkCookie_value_QByteArray_QNetworkCookie(Mu::Thread& NODE_THREAD, Pointer param_this)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QNetworkCookie& arg0 = getqtype<QNetworkCookieType>(param_this);
    return makeqtype<QByteArrayType>(c,arg0.value(),"qt.QByteArray");
}

bool qt_QNetworkCookie_operatorBang_EQ__bool_QNetworkCookie_QNetworkCookie(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_other)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QNetworkCookie& arg0 = getqtype<QNetworkCookieType>(param_this);
    const QNetworkCookie  arg1 = getqtype<QNetworkCookieType>(param_other);
    return arg0.operator!=(arg1);
}

bool qt_QNetworkCookie_operatorEQ_EQ__bool_QNetworkCookie_QNetworkCookie(Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_other)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QNetworkCookie& arg0 = getqtype<QNetworkCookieType>(param_this);
    const QNetworkCookie  arg1 = getqtype<QNetworkCookieType>(param_other);
    return arg0.operator==(arg1);
}

Pointer qt_QNetworkCookie_parseCookies_qt__QNetworkCookieBSB_ESB__QByteArray(Mu::Thread& NODE_THREAD, Pointer param_cookieString)
{
    MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
    const QByteArray  arg0 = getqtype<QByteArrayType>(param_cookieString);
    return makeqtypelist<QNetworkCookie, QNetworkCookieType>(c,QNetworkCookie::parseCookies(arg0),"qt.QNetworkCookie");
}


static NODE_IMPLEMENTATION(_n_QNetworkCookie0, Pointer)
{
    NODE_RETURN(qt_QNetworkCookie_QNetworkCookie_QNetworkCookie_QNetworkCookie_QByteArray_QByteArray(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer), NODE_ARG(2, Pointer)));
}

static NODE_IMPLEMENTATION(_n_domain0, Pointer)
{
    NODE_RETURN(qt_QNetworkCookie_domain_string_QNetworkCookie(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_expirationDate0, Pointer)
{
    NODE_RETURN(qt_QNetworkCookie_expirationDate_QDateTime_QNetworkCookie(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_hasSameIdentifier0, bool)
{
    NODE_RETURN(qt_QNetworkCookie_hasSameIdentifier_bool_QNetworkCookie_QNetworkCookie(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}

static NODE_IMPLEMENTATION(_n_isHttpOnly0, bool)
{
    NODE_RETURN(qt_QNetworkCookie_isHttpOnly_bool_QNetworkCookie(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_isSecure0, bool)
{
    NODE_RETURN(qt_QNetworkCookie_isSecure_bool_QNetworkCookie(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_isSessionCookie0, bool)
{
    NODE_RETURN(qt_QNetworkCookie_isSessionCookie_bool_QNetworkCookie(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_name0, Pointer)
{
    NODE_RETURN(qt_QNetworkCookie_name_QByteArray_QNetworkCookie(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_normalize0, void)
{
    qt_QNetworkCookie_normalize_void_QNetworkCookie_QUrl(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
}

static NODE_IMPLEMENTATION(_n_path0, Pointer)
{
    NODE_RETURN(qt_QNetworkCookie_path_string_QNetworkCookie(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_sameSitePolicy0, int)
{
    NODE_RETURN(qt_QNetworkCookie_sameSitePolicy_int_QNetworkCookie(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_setDomain0, void)
{
    qt_QNetworkCookie_setDomain_void_QNetworkCookie_string(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
}

static NODE_IMPLEMENTATION(_n_setExpirationDate0, void)
{
    qt_QNetworkCookie_setExpirationDate_void_QNetworkCookie_QDateTime(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
}

static NODE_IMPLEMENTATION(_n_setHttpOnly0, void)
{
    qt_QNetworkCookie_setHttpOnly_void_QNetworkCookie_bool(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, bool));
}

static NODE_IMPLEMENTATION(_n_setName0, void)
{
    qt_QNetworkCookie_setName_void_QNetworkCookie_QByteArray(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
}

static NODE_IMPLEMENTATION(_n_setPath0, void)
{
    qt_QNetworkCookie_setPath_void_QNetworkCookie_string(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
}

static NODE_IMPLEMENTATION(_n_setSameSitePolicy0, void)
{
    qt_QNetworkCookie_setSameSitePolicy_void_QNetworkCookie_int(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int));
}

static NODE_IMPLEMENTATION(_n_setSecure0, void)
{
    qt_QNetworkCookie_setSecure_void_QNetworkCookie_bool(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, bool));
}

static NODE_IMPLEMENTATION(_n_setValue0, void)
{
    qt_QNetworkCookie_setValue_void_QNetworkCookie_QByteArray(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
}

static NODE_IMPLEMENTATION(_n_swap0, void)
{
    qt_QNetworkCookie_swap_void_QNetworkCookie_QNetworkCookie(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
}

static NODE_IMPLEMENTATION(_n_toRawForm0, Pointer)
{
    NODE_RETURN(qt_QNetworkCookie_toRawForm_QByteArray_QNetworkCookie_int(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, int)));
}

static NODE_IMPLEMENTATION(_n_value0, Pointer)
{
    NODE_RETURN(qt_QNetworkCookie_value_QByteArray_QNetworkCookie(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
}

static NODE_IMPLEMENTATION(_n_operatorBang_EQ_0, bool)
{
    NODE_RETURN(qt_QNetworkCookie_operatorBang_EQ__bool_QNetworkCookie_QNetworkCookie(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}

static NODE_IMPLEMENTATION(_n_operatorEQ_EQ_0, bool)
{
    NODE_RETURN(qt_QNetworkCookie_operatorEQ_EQ__bool_QNetworkCookie_QNetworkCookie(NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
}

static NODE_IMPLEMENTATION(_n_parseCookies0, Pointer)
{
    NODE_RETURN(qt_QNetworkCookie_parseCookies_qt__QNetworkCookieBSB_ESB__QByteArray(NODE_THREAD, NODE_ARG(0, Pointer)));
}



void
QNetworkCookieType::load()
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
    
    addSymbols(new Function(c, "__allocate", __allocate, None,
                            Return, ftn,
                            End),

               EndArguments );

addSymbols(
    new Alias(c, "RawForm", "int"),
      new SymbolicConstant(c, "NameAndValueOnly", "int", Value(int(QNetworkCookie::NameAndValueOnly))),
      new SymbolicConstant(c, "Full", "int", Value(int(QNetworkCookie::Full))),
    new Alias(c, "SameSite", "int"),
      new SymbolicConstant(c, "Default", "int", Value(int(QNetworkCookie::SameSite::Default))),
      new SymbolicConstant(c, "None", "int", Value(int(QNetworkCookie::SameSite::None))),
      new SymbolicConstant(c, "Lax", "int", Value(int(QNetworkCookie::SameSite::Lax))),
      new SymbolicConstant(c, "Strict", "int", Value(int(QNetworkCookie::SameSite::Strict))),
    EndArguments);

addSymbols(
    // enums
    // member functions
    new Function(c, "QNetworkCookie", _n_QNetworkCookie0, None, Compiled, qt_QNetworkCookie_QNetworkCookie_QNetworkCookie_QNetworkCookie_QByteArray_QByteArray, Return, "qt.QNetworkCookie", Parameters, new Param(c, "this", "qt.QNetworkCookie"), new Param(c, "name", "qt.QByteArray"), new Param(c, "value", "qt.QByteArray"), End),
    // MISSING: QNetworkCookie (QNetworkCookie; QNetworkCookie this, QNetworkCookie other)
    new Function(c, "domain", _n_domain0, None, Compiled, qt_QNetworkCookie_domain_string_QNetworkCookie, Return, "string", Parameters, new Param(c, "this", "qt.QNetworkCookie"), End),
    new Function(c, "expirationDate", _n_expirationDate0, None, Compiled, qt_QNetworkCookie_expirationDate_QDateTime_QNetworkCookie, Return, "qt.QDateTime", Parameters, new Param(c, "this", "qt.QNetworkCookie"), End),
    new Function(c, "hasSameIdentifier", _n_hasSameIdentifier0, None, Compiled, qt_QNetworkCookie_hasSameIdentifier_bool_QNetworkCookie_QNetworkCookie, Return, "bool", Parameters, new Param(c, "this", "qt.QNetworkCookie"), new Param(c, "other", "qt.QNetworkCookie"), End),
    new Function(c, "isHttpOnly", _n_isHttpOnly0, None, Compiled, qt_QNetworkCookie_isHttpOnly_bool_QNetworkCookie, Return, "bool", Parameters, new Param(c, "this", "qt.QNetworkCookie"), End),
    new Function(c, "isSecure", _n_isSecure0, None, Compiled, qt_QNetworkCookie_isSecure_bool_QNetworkCookie, Return, "bool", Parameters, new Param(c, "this", "qt.QNetworkCookie"), End),
    new Function(c, "isSessionCookie", _n_isSessionCookie0, None, Compiled, qt_QNetworkCookie_isSessionCookie_bool_QNetworkCookie, Return, "bool", Parameters, new Param(c, "this", "qt.QNetworkCookie"), End),
    new Function(c, "name", _n_name0, None, Compiled, qt_QNetworkCookie_name_QByteArray_QNetworkCookie, Return, "qt.QByteArray", Parameters, new Param(c, "this", "qt.QNetworkCookie"), End),
    new Function(c, "normalize", _n_normalize0, None, Compiled, qt_QNetworkCookie_normalize_void_QNetworkCookie_QUrl, Return, "void", Parameters, new Param(c, "this", "qt.QNetworkCookie"), new Param(c, "url", "qt.QUrl"), End),
    new Function(c, "path", _n_path0, None, Compiled, qt_QNetworkCookie_path_string_QNetworkCookie, Return, "string", Parameters, new Param(c, "this", "qt.QNetworkCookie"), End),
    new Function(c, "sameSitePolicy", _n_sameSitePolicy0, None, Compiled, qt_QNetworkCookie_sameSitePolicy_int_QNetworkCookie, Return, "int", Parameters, new Param(c, "this", "qt.QNetworkCookie"), End),
    new Function(c, "setDomain", _n_setDomain0, None, Compiled, qt_QNetworkCookie_setDomain_void_QNetworkCookie_string, Return, "void", Parameters, new Param(c, "this", "qt.QNetworkCookie"), new Param(c, "domain", "string"), End),
    new Function(c, "setExpirationDate", _n_setExpirationDate0, None, Compiled, qt_QNetworkCookie_setExpirationDate_void_QNetworkCookie_QDateTime, Return, "void", Parameters, new Param(c, "this", "qt.QNetworkCookie"), new Param(c, "date", "qt.QDateTime"), End),
    new Function(c, "setHttpOnly", _n_setHttpOnly0, None, Compiled, qt_QNetworkCookie_setHttpOnly_void_QNetworkCookie_bool, Return, "void", Parameters, new Param(c, "this", "qt.QNetworkCookie"), new Param(c, "enable", "bool"), End),
    new Function(c, "setName", _n_setName0, None, Compiled, qt_QNetworkCookie_setName_void_QNetworkCookie_QByteArray, Return, "void", Parameters, new Param(c, "this", "qt.QNetworkCookie"), new Param(c, "cookieName", "qt.QByteArray"), End),
    new Function(c, "setPath", _n_setPath0, None, Compiled, qt_QNetworkCookie_setPath_void_QNetworkCookie_string, Return, "void", Parameters, new Param(c, "this", "qt.QNetworkCookie"), new Param(c, "path", "string"), End),
    new Function(c, "setSameSitePolicy", _n_setSameSitePolicy0, None, Compiled, qt_QNetworkCookie_setSameSitePolicy_void_QNetworkCookie_int, Return, "void", Parameters, new Param(c, "this", "qt.QNetworkCookie"), new Param(c, "sameSite", "int"), End),
    new Function(c, "setSecure", _n_setSecure0, None, Compiled, qt_QNetworkCookie_setSecure_void_QNetworkCookie_bool, Return, "void", Parameters, new Param(c, "this", "qt.QNetworkCookie"), new Param(c, "enable", "bool"), End),
    new Function(c, "setValue", _n_setValue0, None, Compiled, qt_QNetworkCookie_setValue_void_QNetworkCookie_QByteArray, Return, "void", Parameters, new Param(c, "this", "qt.QNetworkCookie"), new Param(c, "value", "qt.QByteArray"), End),
    new Function(c, "swap", _n_swap0, None, Compiled, qt_QNetworkCookie_swap_void_QNetworkCookie_QNetworkCookie, Return, "void", Parameters, new Param(c, "this", "qt.QNetworkCookie"), new Param(c, "other", "qt.QNetworkCookie"), End),
    new Function(c, "toRawForm", _n_toRawForm0, None, Compiled, qt_QNetworkCookie_toRawForm_QByteArray_QNetworkCookie_int, Return, "qt.QByteArray", Parameters, new Param(c, "this", "qt.QNetworkCookie"), new Param(c, "form", "int", Value((int)QNetworkCookie::Full)), End),
    new Function(c, "value", _n_value0, None, Compiled, qt_QNetworkCookie_value_QByteArray_QNetworkCookie, Return, "qt.QByteArray", Parameters, new Param(c, "this", "qt.QNetworkCookie"), End),
    // static functions
    new Function(c, "parseCookies", _n_parseCookies0, None, Compiled, qt_QNetworkCookie_parseCookies_qt__QNetworkCookieBSB_ESB__QByteArray, Return, "qt.QNetworkCookie[]", Parameters, new Param(c, "cookieString", "qt.QByteArray"), End),
    EndArguments);
globalScope()->addSymbols(
    new Function(c, "!=", _n_operatorBang_EQ_0, Op, Compiled, qt_QNetworkCookie_operatorBang_EQ__bool_QNetworkCookie_QNetworkCookie, Return, "bool", Parameters, new Param(c, "this", "qt.QNetworkCookie"), new Param(c, "other", "qt.QNetworkCookie"), End),
    // MISSING: = (QNetworkCookie; QNetworkCookie this, QNetworkCookie other)
    new Function(c, "==", _n_operatorEQ_EQ_0, Op, Compiled, qt_QNetworkCookie_operatorEQ_EQ__bool_QNetworkCookie_QNetworkCookie, Return, "bool", Parameters, new Param(c, "this", "qt.QNetworkCookie"), new Param(c, "other", "qt.QNetworkCookie"), End),
    EndArguments);
scope()->addSymbols(
    EndArguments);

}

} // Mu
