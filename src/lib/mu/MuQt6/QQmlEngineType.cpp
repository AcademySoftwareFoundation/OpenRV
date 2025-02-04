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
#include <MuQt6/QQmlEngineType.h>
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
#include <MuQt6/QQmlContextType.h>
#include <MuQt6/QUrlType.h>
#include <MuQt6/QEventType.h>

namespace Mu
{
    using namespace std;

    //----------------------------------------------------------------------
    //  INHERITABLE TYPE IMPLEMENTATION

    // destructor
    MuQt_QQmlEngine::~MuQt_QQmlEngine()
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

    MuQt_QQmlEngine::MuQt_QQmlEngine(Pointer muobj, const CallEnvironment* ce,
                                     QObject* parent)
        : QQmlEngine(parent)
    {
        _env = ce;
        _obj = reinterpret_cast<ClassInstance*>(muobj);
        _obj->retainExternal();
        MuLangContext* c = (MuLangContext*)_env->context();
        _baseType = c->findSymbolOfTypeByQualifiedName<QQmlEngineType>(
            c->internName("qt.QQmlEngine"));
    }

    bool MuQt_QQmlEngine::event(QEvent* e)
    {
        if (!_env)
            return QQmlEngine::event(e);
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
            return QQmlEngine::event(e);
        }
    }

    //----------------------------------------------------------------------
    //  Mu Type CONSTRUCTORS

    QQmlEngineType::QQmlEngineType(Context* c, const char* name, Class* super,
                                   Class* super2)
        : Class(c, name, vectorOf2(super, super2))
    {
    }

    QQmlEngineType::~QQmlEngineType() {}

    //----------------------------------------------------------------------
    //  PRE-COMPILED FUNCTIONS

    static Pointer QQmlEngine_QQmlEngine_QObject(Thread& NODE_THREAD,
                                                 Pointer obj)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        ClassInstance* widget = reinterpret_cast<ClassInstance*>(obj);

        if (!widget)
        {
            return 0;
        }
        else if (QQmlEngine* w = object<QQmlEngine>(widget))
        {
            QQmlEngineType* type =
                c->findSymbolOfTypeByQualifiedName<QQmlEngineType>(
                    c->internName("qt.QQmlEngine"), false);
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
            QQmlEngine_QQmlEngine_QObject(NODE_THREAD, NODE_ARG(0, Pointer)));
    }

    Pointer qt_QQmlEngine_QQmlEngine_QQmlEngine_QQmlEngine_QObject(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_parent)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QObject* arg1 = object<QObject>(param_parent);
        setobject(param_this,
                  new MuQt_QQmlEngine(param_this,
                                      NODE_THREAD.process()->callEnv(), arg1));
        return param_this;
    }

    void qt_QQmlEngine_addImportPath_void_QQmlEngine_string(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_path)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QQmlEngine* arg0 = object<QQmlEngine>(param_this);
        const QString arg1 = qstring(param_path);
        arg0->addImportPath(arg1);
    }

    void qt_QQmlEngine_addPluginPath_void_QQmlEngine_string(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_path)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QQmlEngine* arg0 = object<QQmlEngine>(param_this);
        const QString arg1 = qstring(param_path);
        arg0->addPluginPath(arg1);
    }

    Pointer qt_QQmlEngine_baseUrl_QUrl_QQmlEngine(Mu::Thread& NODE_THREAD,
                                                  Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QQmlEngine* arg0 = object<QQmlEngine>(param_this);
        return makeqtype<QUrlType>(c, arg0->baseUrl(), "qt.QUrl");
    }

    void
    qt_QQmlEngine_clearComponentCache_void_QQmlEngine(Mu::Thread& NODE_THREAD,
                                                      Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QQmlEngine* arg0 = object<QQmlEngine>(param_this);
        arg0->clearComponentCache();
    }

    void qt_QQmlEngine_clearSingletons_void_QQmlEngine(Mu::Thread& NODE_THREAD,
                                                       Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QQmlEngine* arg0 = object<QQmlEngine>(param_this);
        arg0->clearSingletons();
    }

    Pointer qt_QQmlEngine_importPathList_stringBSB_ESB__QQmlEngine(
        Mu::Thread& NODE_THREAD, Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QQmlEngine* arg0 = object<QQmlEngine>(param_this);
        return makestringlist(c, arg0->importPathList());
    }

    Pointer qt_QQmlEngine_interceptUrl_QUrl_QQmlEngine_QUrl_int(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_url,
        int param_type)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QQmlEngine* arg0 = object<QQmlEngine>(param_this);
        const QUrl arg1 = getqtype<QUrlType>(param_url);
        QQmlAbstractUrlInterceptor::DataType arg2 =
            (QQmlAbstractUrlInterceptor::DataType)(param_type);
        return makeqtype<QUrlType>(c, arg0->interceptUrl(arg1, arg2),
                                   "qt.QUrl");
    }

    Pointer qt_QQmlEngine_networkAccessManager_QNetworkAccessManager_QQmlEngine(
        Mu::Thread& NODE_THREAD, Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QQmlEngine* arg0 = object<QQmlEngine>(param_this);
        return makeinstance<QNetworkAccessManagerType>(
            c, arg0->networkAccessManager(), "qt.QNetworkAccessManager");
    }

    Pointer
    qt_QQmlEngine_offlineStorageDatabaseFilePath_string_QQmlEngine_string(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_databaseName)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QQmlEngine* arg0 = object<QQmlEngine>(param_this);
        const QString arg1 = qstring(param_databaseName);
        return makestring(c, arg0->offlineStorageDatabaseFilePath(arg1));
    }

    bool qt_QQmlEngine_outputWarningsToStandardError_bool_QQmlEngine(
        Mu::Thread& NODE_THREAD, Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QQmlEngine* arg0 = object<QQmlEngine>(param_this);
        return arg0->outputWarningsToStandardError();
    }

    Pointer qt_QQmlEngine_pluginPathList_stringBSB_ESB__QQmlEngine(
        Mu::Thread& NODE_THREAD, Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QQmlEngine* arg0 = object<QQmlEngine>(param_this);
        return makestringlist(c, arg0->pluginPathList());
    }

    void qt_QQmlEngine_removeImageProvider_void_QQmlEngine_string(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_providerId)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QQmlEngine* arg0 = object<QQmlEngine>(param_this);
        const QString arg1 = qstring(param_providerId);
        arg0->removeImageProvider(arg1);
    }

    Pointer
    qt_QQmlEngine_rootContext_QQmlContext_QQmlEngine(Mu::Thread& NODE_THREAD,
                                                     Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QQmlEngine* arg0 = object<QQmlEngine>(param_this);
        return makeinstance<QQmlContextType>(c, arg0->rootContext(),
                                             "qt.QQmlContext");
    }

    void qt_QQmlEngine_setBaseUrl_void_QQmlEngine_QUrl(Mu::Thread& NODE_THREAD,
                                                       Pointer param_this,
                                                       Pointer param_url)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QQmlEngine* arg0 = object<QQmlEngine>(param_this);
        const QUrl arg1 = getqtype<QUrlType>(param_url);
        arg0->setBaseUrl(arg1);
    }

    void qt_QQmlEngine_setImportPathList_void_QQmlEngine_stringBSB_ESB_(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_paths)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QQmlEngine* arg0 = object<QQmlEngine>(param_this);
        const QStringList arg1 = qstringlist(param_paths);
        arg0->setImportPathList(arg1);
    }

    void qt_QQmlEngine_setOutputWarningsToStandardError_void_QQmlEngine_bool(
        Mu::Thread& NODE_THREAD, Pointer param_this, bool param_enabled)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QQmlEngine* arg0 = object<QQmlEngine>(param_this);
        bool arg1 = (bool)(param_enabled);
        arg0->setOutputWarningsToStandardError(arg1);
    }

    void qt_QQmlEngine_setPluginPathList_void_QQmlEngine_stringBSB_ESB_(
        Mu::Thread& NODE_THREAD, Pointer param_this, Pointer param_paths)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QQmlEngine* arg0 = object<QQmlEngine>(param_this);
        const QStringList arg1 = qstringlist(param_paths);
        arg0->setPluginPathList(arg1);
    }

    void
    qt_QQmlEngine_trimComponentCache_void_QQmlEngine(Mu::Thread& NODE_THREAD,
                                                     Pointer param_this)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QQmlEngine* arg0 = object<QQmlEngine>(param_this);
        arg0->trimComponentCache();
    }

    bool qt_QQmlEngine_event_bool_QQmlEngine_QEvent(Mu::Thread& NODE_THREAD,
                                                    Pointer param_this,
                                                    Pointer param_e)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QQmlEngine* arg0 = object<QQmlEngine>(param_this);
        QEvent* arg1 = getqpointer<QEventType>(param_e);
        return isMuQtObject(arg0)
                   ? ((MuQt_QQmlEngine*)arg0)->event_pub_parent(arg1)
                   : ((MuQt_QQmlEngine*)arg0)->event_pub(arg1);
    }

    Pointer
    qt_QQmlEngine_contextForObject_QQmlContext_QObject(Mu::Thread& NODE_THREAD,
                                                       Pointer param_object)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        const QObject* arg0 = object<QObject>(param_object);
        return makeinstance<QQmlContextType>(
            c, QQmlEngine::contextForObject(arg0), "qt.QQmlContext");
    }

    void qt_QQmlEngine_setContextForObject_void_QObject_QQmlContext(
        Mu::Thread& NODE_THREAD, Pointer param_object, Pointer param_context)
    {
        MuLangContext* c = static_cast<MuLangContext*>(NODE_THREAD.context());
        QObject* arg0 = object<QObject>(param_object);
        QQmlContext* arg1 = object<QQmlContext>(param_context);
        QQmlEngine::setContextForObject(arg0, arg1);
    }

    static NODE_IMPLEMENTATION(_n_QQmlEngine0, Pointer)
    {
        NODE_RETURN(qt_QQmlEngine_QQmlEngine_QQmlEngine_QQmlEngine_QObject(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_addImportPath0, void)
    {
        qt_QQmlEngine_addImportPath_void_QQmlEngine_string(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_addPluginPath0, void)
    {
        qt_QQmlEngine_addPluginPath_void_QQmlEngine_string(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_baseUrl0, Pointer)
    {
        NODE_RETURN(qt_QQmlEngine_baseUrl_QUrl_QQmlEngine(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_clearComponentCache0, void)
    {
        qt_QQmlEngine_clearComponentCache_void_QQmlEngine(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_clearSingletons0, void)
    {
        qt_QQmlEngine_clearSingletons_void_QQmlEngine(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_importPathList0, Pointer)
    {
        NODE_RETURN(qt_QQmlEngine_importPathList_stringBSB_ESB__QQmlEngine(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_interceptUrl0, Pointer)
    {
        NODE_RETURN(qt_QQmlEngine_interceptUrl_QUrl_QQmlEngine_QUrl_int(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer),
            NODE_ARG(2, int)));
    }

    static NODE_IMPLEMENTATION(_n_networkAccessManager0, Pointer)
    {
        NODE_RETURN(
            qt_QQmlEngine_networkAccessManager_QNetworkAccessManager_QQmlEngine(
                NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_offlineStorageDatabaseFilePath0, Pointer)
    {
        NODE_RETURN(
            qt_QQmlEngine_offlineStorageDatabaseFilePath_string_QQmlEngine_string(
                NODE_THREAD, NONNIL_NODE_ARG(0, Pointer),
                NODE_ARG(1, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_outputWarningsToStandardError0, bool)
    {
        NODE_RETURN(qt_QQmlEngine_outputWarningsToStandardError_bool_QQmlEngine(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_pluginPathList0, Pointer)
    {
        NODE_RETURN(qt_QQmlEngine_pluginPathList_stringBSB_ESB__QQmlEngine(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_removeImageProvider0, void)
    {
        qt_QQmlEngine_removeImageProvider_void_QQmlEngine_string(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_rootContext0, Pointer)
    {
        NODE_RETURN(qt_QQmlEngine_rootContext_QQmlContext_QQmlEngine(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_setBaseUrl0, void)
    {
        qt_QQmlEngine_setBaseUrl_void_QQmlEngine_QUrl(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_setImportPathList0, void)
    {
        qt_QQmlEngine_setImportPathList_void_QQmlEngine_stringBSB_ESB_(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_setOutputWarningsToStandardError0, void)
    {
        qt_QQmlEngine_setOutputWarningsToStandardError_void_QQmlEngine_bool(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, bool));
    }

    static NODE_IMPLEMENTATION(_n_setPluginPathList0, void)
    {
        qt_QQmlEngine_setPluginPathList_void_QQmlEngine_stringBSB_ESB_(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_trimComponentCache0, void)
    {
        qt_QQmlEngine_trimComponentCache_void_QQmlEngine(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer));
    }

    static NODE_IMPLEMENTATION(_n_event0, bool)
    {
        NODE_RETURN(qt_QQmlEngine_event_bool_QQmlEngine_QEvent(
            NODE_THREAD, NONNIL_NODE_ARG(0, Pointer), NODE_ARG(1, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_contextForObject0, Pointer)
    {
        NODE_RETURN(qt_QQmlEngine_contextForObject_QQmlContext_QObject(
            NODE_THREAD, NODE_ARG(0, Pointer)));
    }

    static NODE_IMPLEMENTATION(_n_setContextForObject0, void)
    {
        qt_QQmlEngine_setContextForObject_void_QObject_QQmlContext(
            NODE_THREAD, NODE_ARG(0, Pointer), NODE_ARG(1, Pointer));
    }

    void QQmlEngineType::load()
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
                                QQmlEngine_QQmlEngine_QObject, Return, ftn,
                                Parameters,
                                new Param(c, "object", "qt.QObject"), End),

                   EndArguments);

        addSymbols(
            // enums
            // member functions
            new Function(c, "QQmlEngine", _n_QQmlEngine0, None, Compiled,
                         qt_QQmlEngine_QQmlEngine_QQmlEngine_QQmlEngine_QObject,
                         Return, "qt.QQmlEngine", Parameters,
                         new Param(c, "this", "qt.QQmlEngine"),
                         new Param(c, "parent", "qt.QObject"), End),
            // MISSING: addImageProvider (void; QQmlEngine this, string
            // providerId, "QQmlImageProviderBase *" provider)
            new Function(c, "addImportPath", _n_addImportPath0, None, Compiled,
                         qt_QQmlEngine_addImportPath_void_QQmlEngine_string,
                         Return, "void", Parameters,
                         new Param(c, "this", "qt.QQmlEngine"),
                         new Param(c, "path", "string"), End),
            new Function(c, "addPluginPath", _n_addPluginPath0, None, Compiled,
                         qt_QQmlEngine_addPluginPath_void_QQmlEngine_string,
                         Return, "void", Parameters,
                         new Param(c, "this", "qt.QQmlEngine"),
                         new Param(c, "path", "string"), End),
            // MISSING: addUrlInterceptor (void; QQmlEngine this,
            // "QQmlAbstractUrlInterceptor *" urlInterceptor)
            new Function(c, "baseUrl", _n_baseUrl0, None, Compiled,
                         qt_QQmlEngine_baseUrl_QUrl_QQmlEngine, Return,
                         "qt.QUrl", Parameters,
                         new Param(c, "this", "qt.QQmlEngine"), End),
            new Function(c, "clearComponentCache", _n_clearComponentCache0,
                         None, Compiled,
                         qt_QQmlEngine_clearComponentCache_void_QQmlEngine,
                         Return, "void", Parameters,
                         new Param(c, "this", "qt.QQmlEngine"), End),
            new Function(
                c, "clearSingletons", _n_clearSingletons0, None, Compiled,
                qt_QQmlEngine_clearSingletons_void_QQmlEngine, Return, "void",
                Parameters, new Param(c, "this", "qt.QQmlEngine"), End),
            // MISSING: imageProvider ("QQmlImageProviderBase *"; QQmlEngine
            // this, string providerId)
            new Function(c, "importPathList", _n_importPathList0, None,
                         Compiled,
                         qt_QQmlEngine_importPathList_stringBSB_ESB__QQmlEngine,
                         Return, "string[]", Parameters,
                         new Param(c, "this", "qt.QQmlEngine"), End),
            // MISSING: incubationController ("QQmlIncubationController *";
            // QQmlEngine this)
            new Function(c, "interceptUrl", _n_interceptUrl0, None, Compiled,
                         qt_QQmlEngine_interceptUrl_QUrl_QQmlEngine_QUrl_int,
                         Return, "qt.QUrl", Parameters,
                         new Param(c, "this", "qt.QQmlEngine"),
                         new Param(c, "url", "qt.QUrl"),
                         new Param(c, "type", "int"), End),
            new Function(
                c, "networkAccessManager", _n_networkAccessManager0, None,
                Compiled,
                qt_QQmlEngine_networkAccessManager_QNetworkAccessManager_QQmlEngine,
                Return, "qt.QNetworkAccessManager", Parameters,
                new Param(c, "this", "qt.QQmlEngine"), End),
            // MISSING: networkAccessManagerFactory
            // ("QQmlNetworkAccessManagerFactory *"; QQmlEngine this)
            new Function(
                c, "offlineStorageDatabaseFilePath",
                _n_offlineStorageDatabaseFilePath0, None, Compiled,
                qt_QQmlEngine_offlineStorageDatabaseFilePath_string_QQmlEngine_string,
                Return, "string", Parameters,
                new Param(c, "this", "qt.QQmlEngine"),
                new Param(c, "databaseName", "string"), End),
            // PROP: offlineStoragePath (string; QQmlEngine this)
            new Function(
                c, "outputWarningsToStandardError",
                _n_outputWarningsToStandardError0, None, Compiled,
                qt_QQmlEngine_outputWarningsToStandardError_bool_QQmlEngine,
                Return, "bool", Parameters,
                new Param(c, "this", "qt.QQmlEngine"), End),
            new Function(c, "pluginPathList", _n_pluginPathList0, None,
                         Compiled,
                         qt_QQmlEngine_pluginPathList_stringBSB_ESB__QQmlEngine,
                         Return, "string[]", Parameters,
                         new Param(c, "this", "qt.QQmlEngine"), End),
            new Function(
                c, "removeImageProvider", _n_removeImageProvider0, None,
                Compiled,
                qt_QQmlEngine_removeImageProvider_void_QQmlEngine_string,
                Return, "void", Parameters,
                new Param(c, "this", "qt.QQmlEngine"),
                new Param(c, "providerId", "string"), End),
            // MISSING: removeUrlInterceptor (void; QQmlEngine this,
            // "QQmlAbstractUrlInterceptor *" urlInterceptor)
            new Function(c, "rootContext", _n_rootContext0, None, Compiled,
                         qt_QQmlEngine_rootContext_QQmlContext_QQmlEngine,
                         Return, "qt.QQmlContext", Parameters,
                         new Param(c, "this", "qt.QQmlEngine"), End),
            new Function(c, "setBaseUrl", _n_setBaseUrl0, None, Compiled,
                         qt_QQmlEngine_setBaseUrl_void_QQmlEngine_QUrl, Return,
                         "void", Parameters,
                         new Param(c, "this", "qt.QQmlEngine"),
                         new Param(c, "url", "qt.QUrl"), End),
            new Function(
                c, "setImportPathList", _n_setImportPathList0, None, Compiled,
                qt_QQmlEngine_setImportPathList_void_QQmlEngine_stringBSB_ESB_,
                Return, "void", Parameters,
                new Param(c, "this", "qt.QQmlEngine"),
                new Param(c, "paths", "string[]"), End),
            // MISSING: setIncubationController (void; QQmlEngine this,
            // "QQmlIncubationController *" controller) MISSING:
            // setNetworkAccessManagerFactory (void; QQmlEngine this,
            // "QQmlNetworkAccessManagerFactory *" factory) PROP:
            // setOfflineStoragePath (void; QQmlEngine this, string dir)
            new Function(
                c, "setOutputWarningsToStandardError",
                _n_setOutputWarningsToStandardError0, None, Compiled,
                qt_QQmlEngine_setOutputWarningsToStandardError_void_QQmlEngine_bool,
                Return, "void", Parameters,
                new Param(c, "this", "qt.QQmlEngine"),
                new Param(c, "enabled", "bool"), End),
            new Function(
                c, "setPluginPathList", _n_setPluginPathList0, None, Compiled,
                qt_QQmlEngine_setPluginPathList_void_QQmlEngine_stringBSB_ESB_,
                Return, "void", Parameters,
                new Param(c, "this", "qt.QQmlEngine"),
                new Param(c, "paths", "string[]"), End),
            // MISSING: singletonInstance ("T"; QQmlEngine this, int qmlTypeId)
            // MISSING: singletonInstance ("T"; QQmlEngine this,
            // "QAnyStringView" uri, "QAnyStringView" typeName)
            new Function(
                c, "trimComponentCache", _n_trimComponentCache0, None, Compiled,
                qt_QQmlEngine_trimComponentCache_void_QQmlEngine, Return,
                "void", Parameters, new Param(c, "this", "qt.QQmlEngine"), End),
            // MISSING: urlInterceptors ("QList<QQmlAbstractUrlInterceptor * >";
            // QQmlEngine this)
            _func[0] = new MemberFunction(
                c, "event", _n_event0, None, Compiled,
                qt_QQmlEngine_event_bool_QQmlEngine_QEvent, Return, "bool",
                Parameters, new Param(c, "this", "qt.QQmlEngine"),
                new Param(c, "e", "qt.QEvent"), End),
            // static functions
            new Function(c, "contextForObject", _n_contextForObject0, None,
                         Compiled,
                         qt_QQmlEngine_contextForObject_QQmlContext_QObject,
                         Return, "qt.QQmlContext", Parameters,
                         new Param(c, "object", "qt.QObject"), End),
            new Function(
                c, "setContextForObject", _n_setContextForObject0, None,
                Compiled,
                qt_QQmlEngine_setContextForObject_void_QObject_QQmlContext,
                Return, "void", Parameters,
                new Param(c, "object", "qt.QObject"),
                new Param(c, "context", "qt.QQmlContext"), End),
            EndArguments);
        globalScope()->addSymbols(EndArguments);
        scope()->addSymbols(EndArguments);

        const char** propExclusions = 0;

        populate(this, QQmlEngine::staticMetaObject, propExclusions);
    }

} // namespace Mu
