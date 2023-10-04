//******************************************************************************
// Copyright (c) 2001-2005 Tweak Inc. All rights reserved.
// 
// SPDX-License-Identifier: Apache-2.0
// 
//******************************************************************************

#ifdef PLATFORM_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <TwkGLF/GL.h>
#include <TwkGLF/GLVBO.h>
#include <TwkGLF/GLPipeline.h>
#include <TwkGLF/GLState.h>
#endif
#endif

#include <RvCommon/MediaFileTypes.h>
#include <RvCommon/RvApplication.h>
#include <RvCommon/RvConsoleWindow.h>
#include <RvCommon/RvDocument.h>
#include <RvCommon/RvFileDialog.h>
#include <RvCommon/RvNetworkDialog.h>
#include <RvCommon/StreamConnection.h>
#include <RvCommon/RvTopViewToolBar.h>
#include <RvCommon/RvBottomViewToolBar.h>
#include <RvCommon/RvProfileManager.h>
#include <RvCommon/RvPreferences.h>
#include <RvCommon/RvWebManager.h>
#include <RvCommon/MuUICommands.h>
#include <IPCore/Session.h>
#include <TwkQtBase/QtUtil.h>
#include <TwkDeploy/Deploy.h>
#include <Mu/List.h>
#include <Mu/TupleType.h>
#include <Mu/VariantInstance.h>
#include <Mu/VariantTagType.h>
#include <MuLang/StringType.h>
#include <MuTwkApp/EventType.h>
#include <MuTwkApp/SettingsValueType.h>
#include <QtCore/QtCore>
#include <QtGui/QtGui>
#include <QtWidgets/QFileIconProvider>

#include <MuQt5/QNetworkAccessManagerType.h>
#include <MuQt5/QTabWidgetType.h>
#include <MuQt5/QToolBarType.h>
#include <MuQt5/qtUtils.h>
#include <MuQt5/QUrlType.h>


#include <TwkQtCoreUtil/QtConvert.h>
#include <RvApp/Options.h>
#include <TwkApp/Event.h>
#include <TwkQtChat/Client.h>
#include <TwkUtil/PathConform.h>
#include <TwkUtil/sgcHop.h>
#include <TwkUtil/User.h>
#include <TwkUtil/File.h>
#include <assert.h>
#include <RvCommon/GLView.h> // WINDOWS NEEDS THIS LAST
//#include <RvCommon/SequenceFileEngine.h>
#ifdef PLATFORM_WINDOWS
#undef _NTDDK_
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#define WINVER 0x0501 // both combined stand for Windows XP
#endif
//#include <windows.h>
//#include <lm.h>
#include <sddl.h>
#endif

//----------------------------------------------------------------------
//  Related to MainWindow UI
//  May want to move into separate module/library later on
//----------------------------------------------------------------------
#include <MuQt5/qtModule.h>
#include <MuQt5/QActionType.h>
#include <MuQt5/QWidgetType.h>
#include <MuQt5/QObjectType.h>
#include <MuQt5/QMainWindowType.h>
#include <MuQt5/QWebEnginePageType.h>

#include <RvCommon/RvJavaScriptObject.h>

//#ifdef PLATFORM_DARWIN
//#define USE_NATIVE_DIALOGS
//#endif

namespace Rv {

using namespace TwkApp;
using namespace Mu;
using namespace std;
using namespace TwkUtil;
using namespace IPCore;
using namespace TwkQtCoreUtil;

static void throwBadArgumentException(const Mu::Node& node,
                                      Mu::Thread& thread,
                                      const Mu::String& msg)
{
    ostringstream str;
    const Mu::MuLangContext* context =
        static_cast<const Mu::MuLangContext*>(thread.context());
    ExceptionType::Exception *e =
        new ExceptionType::Exception(context->exceptionType());
    str << "in " << node.symbol()->fullyQualifiedName() << ": " << msg;
    e->string() += str.str().c_str();
    thread.setException(e);
    throw BadArgumentException(thread, e);
}

void initUICommands()
{
    USING_MU_FUNCTION_SYMBOLS;
    MuLangContext* context = muContext();
    Context* c = context;
    Symbol* root = context->globalScope();
    Name cname = c->internName("commands");
    Mu::Module* commands = root->findSymbolOfType<Mu::Module>(cname);

    root->addSymbol(new qtModule(c, "qt"));

    Context::TypeVector types;
    types.push_back(context->stringType());
    types.push_back(context->stringType());
    context->listType(context->tupleType(types));

    commands->addSymbols(new Function(c, "resizeFit", resizeFit, None,
                                      Return, "void",
                                      End),

                         new Function(c, "setViewSize", setViewSize, None,
                                      Return, "void",
                                      Parameters,
                                      new Param(c, "width", "int"),
                                      new Param(c, "height", "int"),
                                      End),

                         new Function(c, "startupResize", startupResize, None,
                                      Return, "bool",
                                      End),

                         new Function(c, "popupMenu", popupMenu, None,
                                      Return, "void",
                                      Parameters,
                                      new Param(c, "event", "Event"),
                                      new Param(c, "menu", "MenuItem[]", Value(Pointer(0))),
                                      End),

                         new Function(c, "popupMenuAtPoint", popupMenuAtPoint, None,
                                      Return, "void",
                                      Parameters,
                                      new Param(c, "x", "int"),
                                      new Param(c, "y", "int"),
                                      new Param(c, "menu", "MenuItem[]", Value(Pointer(0))),
                                      End),

                         new Function(c, "setWindowTitle", setWindowTitle, None,
                                      Return, "void",
                                      Parameters,
                                      new Param(c, "title", "string"),
                                      End),

                         new Function(c, "center", center, None,
                                      Return, "void",
                                      End),

                         new Function(c, "close", close, None,
                                      Return, "void",
                                      End),

                         new Function(c, "toggleMenuBar", toggleMenuBar, None,
                                      Return, "void",
                                      End),

                         new Function(c, "isMenuBarVisible", isMenuBarVisible, None,
                                      Return, "bool",
                                      End),

                         new SymbolicConstant(c, "OneExistingFile", "int", Value(int(RvFileDialog::OneExistingFile))),
                         new SymbolicConstant(c, "ManyExistingFiles", "int", Value(int(RvFileDialog::ManyExistingFiles))),
                         new SymbolicConstant(c, "ManyExistingFilesAndDirectories", "int", Value(int(RvFileDialog::ManyExistingFilesAndDirectories))),
                         new SymbolicConstant(c, "OneFileName", "int", Value(int(RvFileDialog::OneFileName))),
                         new SymbolicConstant(c, "OneDirectory", "int", Value(int(RvFileDialog::OneDirectory))),

                         new Function(c, "openMediaFileDialog", openMediaFileDialog,
                                      None,
                                      Return, "string[]",
                                      Parameters,
                                      new Param(c, "associated", "bool"),
                                      new Param(c, "selectType", "int"),
                                      new Param(c, "filter", "string", Value(Pointer(0))),
                                      new Param(c, "defaultPath", "string", Value(Pointer(0))),
                                      new Param(c, "label", "string", Value(Pointer(0))),
                                      End),

                         new Function(c, "openFileDialog", openFileDialog,
                                      None,
                                      Return, "string[]",
                                      Parameters,
                                      new Param(c, "associated", "bool"),
                                      new Param(c, "multiple", "bool", Value(false)),
                                      new Param(c, "directory", "bool", Value(false)),
                                      new Param(c, "filter", "string", Value(Pointer(0))),
                                      new Param(c, "defaultPath", "string", Value(Pointer(0))),
                                      End),

                         new Function(c, "saveFileDialog", saveFileDialog,
                                      None,
                                      Return, "string",
                                      Parameters,
                                      new Param(c, "associated", "bool"),
                                      new Param(c, "filter", "string", Value(Pointer(0))),
                                      new Param(c, "defaultPath", "string", Value(Pointer(0))),
                                      new Param(c, "directory", "bool", Value(false)),
                                      End),

                         new SymbolicConstant(c, "CursorNone", "int", Value(int(Qt::BlankCursor))),
                         new SymbolicConstant(c, "CursorArrow", "int", Value(int(Qt::ArrowCursor))),
                         new SymbolicConstant(c, "CursorDefault", "int", Value(int(Qt::ArrowCursor))),

                         new Function(c, "setCursor", setCursor, None,
                                      Return, "void",
                                      Parameters,
                                      new Param(c, "cursorType", "int"),
                                      End),

                         new SymbolicConstant(c, "InfoAlert", "int", Value(0)),
                         new SymbolicConstant(c, "WarningAlert", "int", Value(1)),
                         new SymbolicConstant(c, "ErrorAlert", "int", Value(2)),

                         new Function(c, "stereoSupported", stereoSupported, None,
                                      Return, "bool",
                                      End),

                         new Function(c, "alertPanel", alertPanel, None,
                                      Return, "int",
                                      Parameters,
                                      new Param(c, "associated", "bool"),
                                      new Param(c, "type", "int"),
                                      new Param(c, "title", "string"),
                                      new Param(c, "message", "string"),
                                      new Param(c, "button0", "string"),
                                      new Param(c, "button1", "string"),
                                      new Param(c, "button2", "string"),
                                      End),

                         new Function(c, "watchFile", watchFile, None,
                                      Return, "void",
                                      Parameters,
                                      new Param(c, "filename", "string"),
                                      new Param(c, "watch", "bool"),
                                      End),

                         new Function(c, "showNetworkDialog", showNetworkDialog, None,
                                      Return, "void",
                                      End),

                         new Function(c, "showConsole", showConsole, None,
                                      Return, "void",
                                      End),

                         new Function(c, "isConsoleVisible", isConsoleVisible, None,
                                      Return, "bool",
                                      End),
                         // network

                         new Function(c, "remoteSendMessage", remoteSendMessage, None,
                                      Return, "void",
                                      Parameters,
                                      new Param(c, "message", "string"),
                                      new Param(c, "recipients", "string[]", Value(Pointer(0))),
                                      End),

                         new Function(c, "remoteSendEvent", remoteSendEvent, None,
                                      Return, "void",
                                      Parameters,
                                      new Param(c, "event", "string"),
                                      new Param(c, "target", "string"),
                                      new Param(c, "contents", "string"),
                                      new Param(c, "recipients", "string[]", Value(Pointer(0))),
                                      End),

                         new Function(c, "remoteSendDataEvent", remoteSendDataEvent, None,
                                      Return, "void",
                                      Parameters,
                                      new Param(c, "event", "string"),
                                      new Param(c, "target", "string"),
                                      new Param(c, "interp", "string"),
                                      new Param(c, "data", "byte[]"),
                                      new Param(c, "recipients", "string[]"),
                                      End),

                         new Function(c, "remoteConnections", remoteConnections, None,
                                      Return, "string[]",
                                      End),

                         new Function(c, "remoteApplications", remoteApplications, None,
                                      Return, "string[]",
                                      End),

                         new Function(c, "remoteContacts", remoteContacts, None,
                                      Return, "string[]",
                                      End),

                         new Function(c, "remoteLocalContactName", remoteLocalContactName, None,
                                      Return, "string",
                                      End),

                         new Function(c, "setRemoteLocalContactName", setRemoteLocalContactName, None,
                                      Return, "string",
                                      Parameters,
                                      new Param(c, "name", "string"),
                                      End),

                         new Function(c, "remoteConnect", remoteConnect, None,
                                      Return, "void",
                                      Parameters,
                                      new Param(c, "name", "string"),
                                      new Param(c, "host", "string"),
                                      new Param(c, "port", "int", Value(0)),
                                      End),

                         new Function(c, "remoteDisconnect", remoteDisconnect, None,
                                      Return, "void",
                                      Parameters,
                                      new Param(c, "remoteContact", "string"),
                                      End),

                         new Function(c, "remoteNetwork", remoteNetwork, None,
                                      Return, "void",
                                      Parameters,
                                      new Param(c, "on", "bool"),
                                      End),

                         new Function(c, "remoteConnectionIsIncoming", remoteConnectionIsIncoming, None,
                                      Return, "bool",
                                      Parameters,
                                      new Param(c, "connection", "string"),
                                      End),

                         new Function(c, "spoofConnectionStream", spoofConnectionStream, None,
                                      Return, "void",
                                      Parameters,
                                      new Param(c, "streamFile", "string"),
                                      new Param(c, "timeScale", "float"),
                                      new Param(c, "verbose", "bool", Value(false)),
                                      End),

                         new SymbolicConstant(c, "NetworkStatusOn", "int", Value(1)),
                         new SymbolicConstant(c, "NetworkStatusOff", "int", Value(0)),

                         new Function(c, "remoteNetworkStatus", remoteNetworkStatus, None,
                                      Return, "int",
                                      End),

                         new SymbolicConstant(c, "NetworkPermissionAsk",   "int", Value(int(RvNetworkDialog::AskConnect))),
                         new SymbolicConstant(c, "NetworkPermissionAllow", "int", Value(int(RvNetworkDialog::AllowConnect))),
                         new SymbolicConstant(c, "NetworkPermissionDeny",  "int", Value(int(RvNetworkDialog::DenyConnect))),

                         new Function(c, "remoteDefaultPermission", remoteDefaultPermission, None,
                                      Return, "int",
                                      End),

                         new Function(c, "setRemoteDefaultPermission", setRemoteDefaultPermission, None,
                                      Return, "void",
                                      Parameters,
                                      new Param(c, "permission", "int"),
                                      End),

                         new Function(c, "writeSetting", writeSetting, None,
                                      Return, "void",
                                      Parameters,
                                      new Param(c, "group", "string"),
                                      new Param(c, "name", "string"),
                                      new Param(c, "value", "SettingsValue"),
                                      End),

                         new Function(c, "readSetting", readSetting, None,
                                      Return, "SettingsValue",
                                      Parameters,
                                      new Param(c, "group", "string"),
                                      new Param(c, "name", "string"),
                                      new Param(c, "defaultValue", "SettingsValue"),
                                      End),

                         new Function(c, "httpGet", httpGet, None,
                                      Return, "void",
                                      Parameters,
                                      new Param(c, "url", "string"),
                                      new Param(c, "headers", "[(string,string)]"),
                                      new Param(c, "replyEvent", "string"),
                                      new Param(c, "authenticationEvent", "string", Value(Pointer(0))),
                                      new Param(c, "progressEvent", "string", Value(Pointer(0))),
                                      new Param(c, "ignoreSslErrors", "bool", Value(false)),
                                      new Param(c, "urlIsEncoded", "bool", Value(false)),
                                      End),

                         new Function(c, "httpPost", httpPostString, None,
                                      Return, "void",
                                      Parameters,
                                      new Param(c, "url", "string"),
                                      new Param(c, "headers", "[(string,string)]"),
                                      new Param(c, "postString", "string"),
                                      new Param(c, "replyEvent", "string"),
                                      new Param(c, "authenticationEvent", "string", Value(Pointer(0))),
                                      new Param(c, "progressEvent", "string", Value(Pointer(0))),
                                      new Param(c, "ignoreSslErrors", "bool", Value(false)),
                                      new Param(c, "urlIsEncoded", "bool", Value(false)),
                                      End),

                         new Function(c, "httpPost", httpPostData, None,
                                      Return, "void",
                                      Parameters,
                                      new Param(c, "url", "string"),
                                      new Param(c, "headers", "[(string,string)]"),
                                      new Param(c, "postData", "byte[]"),
                                      new Param(c, "replyEvent", "string"),
                                      new Param(c, "authenticationEvent", "string", Value(Pointer(0))),
                                      new Param(c, "progressEvent", "string", Value(Pointer(0))),
                                      new Param(c, "ignoreSslErrors", "bool", Value(false)),
                                      new Param(c, "urlIsEncoded", "bool", Value(false)),
                                      End),

                         new Function(c, "httpPut", httpPutString, None,
                                      Return, "void",
                                      Parameters,
                                      new Param(c, "url", "string"),
                                      new Param(c, "headers", "[(string,string)]"),
                                      new Param(c, "putString", "string"),
                                      new Param(c, "replyEvent", "string"),
                                      new Param(c, "authenticationEvent", "string", Value(Pointer(0))),
                                      new Param(c, "progressEvent", "string", Value(Pointer(0))),
                                      new Param(c, "ignoreSslErrors", "bool", Value(false)),
                                      new Param(c, "urlIsEncoded", "bool", Value(false)),
                                      End),

                         new Function(c, "httpPut", httpPutData, None,
                                      Return, "void",
                                      Parameters,
                                      new Param(c, "url", "string"),
                                      new Param(c, "headers", "[(string,string)]"),
                                      new Param(c, "putData", "byte[]"),
                                      new Param(c, "replyEvent", "string"),
                                      new Param(c, "authenticationEvent", "string", Value(Pointer(0))),
                                      new Param(c, "progressEvent", "string", Value(Pointer(0))),
                                      new Param(c, "ignoreSslErrors", "bool", Value(false)),
                                      new Param(c, "urlIsEncoded", "bool", Value(false)),
                                      End),

                         new Function(c, "mainWindowWidget", mainWindowWidget, None,
                                      Return, "qt.QMainWindow",
                                      End),

                         new Function(c, "mainViewWidget", mainViewWidget, None,
                                      Return, "qt.QWidget",
                                      End),

                         new Function(c, "prefTabWidget", prefTabWidget, None,
                                      Return, "qt.QTabWidget",
                                      End),

                         new Function(c, "sessionBottomToolBar", sessionBottomToolBar, None,
                                      Return, "qt.QToolBar",
                                      End),

                         new Function(c, "networkAccessManager", networkAccessManager, None,
                                      Return, "qt.QNetworkAccessManager",
                                      End),

                         new Function(c, "javascriptMuExport", javascriptMuExport, None,
                                      Return, "void",
                                      Parameters,
                                      new Param(c, "frame", "qt.QWebEnginePage"),
                                      End),

                         new Function(c, "sessionFromUrl", sessionFromUrl, None,
                                      Return, "void",
                                      Parameters,
                                      new Param(c, "url", "string"),
                                      End),

                         new Function(c, "putUrlOnClipboard", putUrlOnClipboard, None,
                                      Return, "void",
                                      Parameters,
                                      new Param(c, "url", "string"),
                                      new Param(c, "title", "string"),
                                      new Param(c, "doEncode", "bool", Value(true)),
                                      End),

                         new Function(c, "myNetworkPort", myNetworkPort, None,
                                      Return, "int",
                                      End),

                         new Function(c, "myNetworkHost", myNetworkHost, None,
                                      Return, "string",
                                      End),

                         new Function(c, "encodePassword", encodePassword,
                                      None,
                                      Return, "string",
                                      Parameters,
                                      new Param(c, "password", "string"),
                                      End),

                         new Function(c, "decodePassword", decodePassword,
                                      None,
                                      Return, "string",
                                      Parameters,
                                      new Param(c, "password", "string"),
                                      End),

                         new Function(c, "cacheDir", cacheDir,
                                      None,
                                      Return, "string",
                                      End),

                         new Function(c, "openUrl", openUrl,
                                      None,
                                      Return, "void",
                                      Parameters,
                                      new Param(c, "url", "string"),
                                      End),

                         new Function(c, "openUrlFromUrl", openUrlFromUrl, None,
                                      Return, "void",
                                      Parameters,
                                      new Param(c, "url", "qt.QUrl"),
                                      End),

                         new Function(c, "queryDriverAttribute", queryDriverAttribute,
                                      None,
                                      Return, "string",
                                      Parameters,
                                      new Param(c, "attribute", "string"),
                                      End),

                         new Function(c, "setDriverAttribute", setDriverAttribute,
                                      None,
                                      Return, "void",
                                      Parameters,
                                      new Param(c, "attribute", "string"),
                                      new Param(c, "value", "string"),
                                      End),

                         new Function(c, "setPresentationMode", setPresentationMode,
                                      None,
                                      Return, "void",
                                      Parameters,
                                      new Param(c, "value", "bool"),
                                      End),

                         new Function(c, "presentationMode", presentationMode, None,
                                      Return, "bool",
                                      End),

                         new Function(c, "packageListFromSetting", packageListFromSetting,
                                      None,
                                      Return, "string[]",
                                      Parameters,
                                      new Param(c, "settingName", "string"),
                                      End),

                         new Function(c, "showTopViewToolbar", showTopViewToolbar, None,
                                      Return, "void",
                                      Parameters,
                                      new Param(c, "show", "bool"),
                                      End),

                         new Function(c, "showBottomViewToolbar", showBottomViewToolbar, None,
                                      Return, "void",
                                      Parameters,
                                      new Param(c, "show", "bool"),
                                      End),

                         new Function(c, "isTopViewToolbarVisible", isTopViewToolbarVisible, None,
                                      Return, "bool",
                                      End),

                         new Function(c, "isBottomViewToolbarVisible", isBottomViewToolbarVisible, None,
                                      Return, "bool",
                                      End),

                         new Function(c, "editNodeSource", editNodeSource, None,
                                      Return, "void",
                                      Parameters,
                                      new Param(c, "nodeName", "string"),
                                      End),

                         new Function(c, "editProfiles", editProfiles, None,
                                      Return, "void",
                                      End),

                         new Function(c, "validateShotgunToken", validateShotgunToken, None,
                                      Return, "string",
                                      Parameters,
                                      new Param(c, "port", "int", Value(-1)),
                                      new Param(c, "tag", "string", Value(Pointer(0))),
                                      End),

                         new Function(c, "launchTLI", launchTLI, None,
                                      Return, "void",
                                      End),

                         new Function(c, "rvioSetup", rvioSetup, None,
                                      Return, "void",
                                      End),

                         EndArguments);
}

NODE_IMPLEMENTATION(queryDriverAttribute, Pointer)
{
    Process*                  p       = NODE_THREAD.process();
    MuLangContext*            context = static_cast<MuLangContext*>(p->context());
    const StringType::String* attr    = NODE_ARG_OBJECT(0, StringType::String);
    const StringType*         stype   = context->stringType();

    string v = RvApplication::queryDriverAttribute(attr->c_str());
    NODE_RETURN(stype->allocate(v));
}

NODE_IMPLEMENTATION(setDriverAttribute, void)
{
    const StringType::String* attr = NODE_ARG_OBJECT(0, StringType::String);
    const StringType::String* val = NODE_ARG_OBJECT(1, StringType::String);
    RvApplication::setDriverAttribute(attr->c_str(), val->c_str());
}

NODE_IMPLEMENTATION(resizeFit, void)
{
    RvSession *s = RvSession::currentRvSession();
    assert( s );

    RvDocument *rvDoc = (RvDocument *)s->opaquePointer();
    assert( rvDoc );

    rvDoc->resizeToFit();
    s->setUserHasSetViewSize (true);
}

NODE_IMPLEMENTATION(setViewSize, void)
{
    RvSession *s = RvSession::currentRvSession();
    RvDocument *rvDoc = (RvDocument *)s->opaquePointer();
    int w = NODE_ARG(0, int);
    int h = NODE_ARG(1, int);

    rvDoc->resizeView(w, h);
    s->setUserHasSetViewSize (true);
}

NODE_IMPLEMENTATION(startupResize, bool)
{
    Session *s = Session::currentSession();
    RvDocument *rvDoc = (RvDocument *)s->opaquePointer();
    NODE_RETURN(rvDoc->startupResize());
}

static void
popupMenuInternal (DynamicArray* array, QPoint& location)
{
    Session *s = Session::currentSession();
    RvDocument *rvDoc = (RvDocument *)s->opaquePointer();

    s->receivingEvents(false);

    QPoint p = rvDoc->view()->mapToGlobal(location);

    if (array)
    {
        if (TwkApp::Menu* m = createTwkAppMenu("temp", array))
        {
            rvDoc->popupMenu(m, p);
        }
    }
    else
    {
        rvDoc->mainPopup()->popup(p);
    }
}

NODE_IMPLEMENTATION(popupMenu, void)
{
    Session *s = Session::currentSession();
    RvDocument *rvDoc = (RvDocument *)s->opaquePointer();

    TwkApp::EventType::EventInstance* e =
        NODE_ARG_OBJECT(0, TwkApp::EventType::EventInstance);

    DynamicArray* array = NODE_ARG_OBJECT(1, DynamicArray);
    QPoint lp;

    if (const TwkApp::PointerEvent* pevent =
        dynamic_cast<const TwkApp::PointerEvent*>(e->event))
    {
        lp = QPoint(pevent->x(), rvDoc->view()->height() - pevent->y() - 1);
    }
    else
    {
        lp = QPoint(0, rvDoc->view()->height() - 1);
    }

    popupMenuInternal (array, lp);
}

NODE_IMPLEMENTATION(popupMenuAtPoint, void)
{
    Session *s = Session::currentSession();
    RvDocument *rvDoc = (RvDocument *)s->opaquePointer();
    int x = NODE_ARG(0, int);
    int y = NODE_ARG(1, int);
    DynamicArray* array = NODE_ARG_OBJECT(2, DynamicArray);

    QPoint lp (x, rvDoc->view()->height() - y - 1);

    popupMenuInternal (array, lp);
}

NODE_IMPLEMENTATION(setWindowTitle, void)
{
    const StringType::String *title = NODE_ARG_OBJECT(0, StringType::String);

    Session *s = Session::currentSession();
    assert( s );

    RvDocument *rvDoc = (RvDocument *)s->opaquePointer();
    assert( rvDoc );

    rvDoc->setWindowTitle(UTF8::qconvert(title->c_str()));
}

NODE_IMPLEMENTATION(center, void)
{
    Session *s = Session::currentSession();
    assert( s );

    RvDocument *rvDoc = (RvDocument *)s->opaquePointer();
    assert( rvDoc );

    rvDoc->resizeToFit(true);
    rvDoc->center();
}

NODE_IMPLEMENTATION(close, void)
{
    Session *s = Session::currentSession();
    assert( s );

    RvDocument *rvDoc = (RvDocument *)s->opaquePointer();
    assert( rvDoc );

    rvDoc->close();
}

NODE_IMPLEMENTATION(toggleMenuBar, void)
{
    Session *s = Session::currentSession();
    assert( s );
    RvDocument *rvDoc = (RvDocument *)s->opaquePointer();
    rvDoc->toggleMenuBar();
}

NODE_IMPLEMENTATION(isMenuBarVisible, bool)
{
    Session *s = Session::currentSession();
    assert( s );
    RvDocument *rvDoc = (RvDocument *)s->opaquePointer();
    NODE_RETURN(rvDoc->menuBarShown());
}

static map<Session*,RvFileDialog*> sessionToMediaDialog;

NODE_IMPLEMENTATION(openMediaFileDialog, Pointer)
{
    HOP_PROF_FUNC();

    Session*            s         = Session::currentSession();
    RvDocument*         rvDoc     = (RvDocument *)s->opaquePointer();
    Process*            p         = NODE_THREAD.process();
    bool                sheet     = NODE_ARG(0, bool);
    int                 mode      = NODE_ARG(1, int);
    StringType::String* filter    = NODE_ARG_OBJECT(2, StringType::String);
    StringType::String* path      = NODE_ARG_OBJECT(3, StringType::String);
    StringType::String* label     = NODE_ARG_OBJECT(4, StringType::String);

    const DynamicArrayType* atype = static_cast<const DynamicArrayType*>(NODE_THIS.type());
    const StringType* stype = static_cast<const StringType*>(atype->elementType());

    FileTypeTraits* traits = 0;
    bool hasSinglePair = false;

    if (filter)
    {
        QStringList parts;
        QList<QPair<QString,QString> > pairs;
        parts = UTF8::qconvert(filter->c_str()).split("|");

        if (parts.size() == 1)
        {
            QString v = parts.front();

            if (v != "*" && v != "")
            {
                pairs.push_back(qMakePair(v, v));
            }
        }
        else
        {
            for (size_t i = 0; i < parts.size(); i+=2)
            {
                pairs.push_back(qMakePair(parts[i], parts[i+1]));
            }
        }

        if (pairs.size() == 0)
        {
            traits = new MediaFileTypes(true, false);
        }
        else
        {
            traits = new MediaFileTypes(true, false, pairs);
        }

        hasSinglePair = (pairs.size() == 1);
    }
    else
    {
        traits = new MediaFileTypes(true, false);
    }

    if (sessionToMediaDialog.find(s) == sessionToMediaDialog.end())
    {
        sessionToMediaDialog[s] = new RvFileDialog(rvDoc,
                                                   new MediaFileTypes(true, false),
                                                   RvFileDialog::OpenFileRole,
                                                   sheet ?  Qt::Sheet : Qt::Dialog,
                                                   "MediaFileDialog");
    }

    RvFileDialog& dialog = *sessionToMediaDialog[s];

    if (path) dialog.setDirectory(UTF8::qconvert(path->c_str()));
    dialog.setTitleLabel(UTF8::qconvert(label->c_str()));
    dialog.setFileMode((RvFileDialog::FileMode)mode);
    dialog.setFileTypeTraits(traits);
    if (hasSinglePair) dialog.setFileTypeIndex(2);

    rvDoc->setDocumentDisabled(false, true);
    bool result = dialog.exec();
    rvDoc->view()->setFocus(Qt::OtherFocusReason);
    rvDoc->setDocumentDisabled(false, false);

    if (result)
    {
        QStringList files;
        files = dialog.selectedFiles();

        DynamicArray* array = new DynamicArray(atype, 1);
        array->resize(files.size());

        for (int i=0, size = files.size(); i < size; i++)
        {
            string v = pathConform(UTF8::qconvert(files.at(i)));
            array->element<StringType::String*>(i) = stype->allocate(v);
        }

        NODE_RETURN((Pointer)array);
    }
    else
    {
        MuLangContext* context = static_cast<MuLangContext*>(p->context());
        ExceptionType::Exception *e =
            new ExceptionType::Exception(context->exceptionType());
        e->string() += "operation cancelled";
        NODE_THREAD.setException(e);
        ProgramException exc(NODE_THREAD, e);
        throw exc;
    }

    NODE_RETURN((Pointer)0);
}

/*
 * XXX in progress (see below).
 *
class MyIconProvider : public QFileIconProvider
{
  public:
    virtual QString type (const QFileInfo& info) { return QFileIconProvider::File; };
};
*/

static map<Session*,RvFileDialog*> sessionToOpenDialog;

NODE_IMPLEMENTATION(openFileDialog, Pointer)
{
    Session*            s         = Session::currentSession();
    RvDocument*         rvDoc     = (RvDocument *)s->opaquePointer();
    Process*            p         = NODE_THREAD.process();
    bool                sheet     = NODE_ARG(0, bool);
    bool                multi     = NODE_ARG(1, bool);
    bool                directory = NODE_ARG(2, bool);
    StringType::String* filter    = NODE_ARG_OBJECT(3, StringType::String);
    StringType::String* path      = NODE_ARG_OBJECT(4, StringType::String);

    const DynamicArrayType* atype = static_cast<const DynamicArrayType*>(NODE_THIS.type());
    const StringType* stype = static_cast<const StringType*>(atype->elementType());

    FileTypeTraits* traits = 0;
    bool hasSinglePair = false;

    if (filter)
    {
        QStringList parts;
        QList<QPair<QString,QString> > pairs;
        parts = UTF8::qconvert(filter->c_str()).split("|");

        if (parts.size() == 1)
        {
            QString v = parts.front();

            if (v != "*" && v != "")
            {
                pairs.push_back(qMakePair(v, v));
            }
        }
        else
        {
            for (size_t i = 0; i < parts.size(); i+=2)
            {
                pairs.push_back(qMakePair(parts[i], parts[i+1]));
            }
        }

        traits = new MediaFileTypes(true, false, pairs);

        hasSinglePair = (pairs.size() == 1);
    }
    else
    {
        traits = new MediaFileTypes(true, false);
    }

    RvFileDialog* d = 0;

    if (sessionToOpenDialog.find(s) == sessionToOpenDialog.end())
    {
        sessionToOpenDialog[s] = new RvFileDialog(rvDoc,
                                                  traits,
                                                  RvFileDialog::OpenFileRole,
                                                  sheet ?  Qt::Sheet : Qt::Dialog,
                                                  "OpenFileDialog");
    }
    else
    {
        sessionToOpenDialog[s]->setFileTypeTraits(traits);
    }

    RvFileDialog& dialog = *sessionToOpenDialog[s];

    if (hasSinglePair) dialog.setFileTypeIndex(2);

    dialog.setFileMode(multi ? RvFileDialog::ManyExistingFiles :
        ((directory) ? RvFileDialog::OneDirectoryName : RvFileDialog::OneFileName));

    if (path) dialog.setDirectory(UTF8::qconvert(path->c_str()));
    dialog.setRole(RvFileDialog::OpenFileRole);
    dialog.setTitleLabel(multi ? QString("Open Files") : QString("Open File"));
    dialog.setViewMode(RvFileDialog::DetailedFileView);
    dialog.lockViewMode(true);

    rvDoc->setDocumentDisabled(false, true);
    bool result = dialog.exec();
    rvDoc->view()->setFocus(Qt::OtherFocusReason);
    rvDoc->setDocumentDisabled(false, false);

    if (result)
    {
        QStringList files;
        files = dialog.selectedFiles();

        DynamicArray* array = new DynamicArray(atype, 1);
        array->resize(files.size());

        for (int i=0, size = files.size(); i < size; i++)
        {
            string v = pathConform(UTF8::qconvert(files.at(i)));
            array->element<StringType::String*>(i) = stype->allocate(v);
        }

        NODE_RETURN((Pointer)array);
    }
    else
    {
        MuLangContext* context = static_cast<MuLangContext*>(p->context());
        ExceptionType::Exception *e =
            new ExceptionType::Exception(context->exceptionType());
        e->string() += "operation cancelled";
        NODE_THREAD.setException(e);
        ProgramException exc(NODE_THREAD, e);
        throw exc;
    }

    NODE_RETURN((Pointer)0);
}

static map<Session*,RvFileDialog*> sessionToSaveDialog;

NODE_IMPLEMENTATION(saveFileDialog, Pointer)
{
    Session*            s      = Session::currentSession();
    RvDocument*         rvDoc  = (RvDocument *)s->opaquePointer();
    Process*            p      = NODE_THREAD.process();
    bool                sheet  = NODE_ARG(0, bool);
    StringType::String* filter = NODE_ARG_OBJECT(1, StringType::String);
    StringType::String* path   = NODE_ARG_OBJECT(2, StringType::String);
    bool                directory = NODE_ARG(3, bool);

    const StringType* stype =
        static_cast<const StringType*>(NODE_THIS.type());

    FileTypeTraits* traits = 0;
    bool hasSinglePair = false;

    if (filter)
    {
        QStringList parts;
        QList<QPair<QString,QString> > pairs;
        parts = UTF8::qconvert(filter->c_str()).split("|");

        if (parts.size() == 1)
        {
            QString v = parts.front();

            if (v != "*" && v != "")
            {
                pairs.push_back(qMakePair(v, v));
            }
        }
        else
        {
            for (size_t i = 0; i < parts.size(); i+=2)
            {
                pairs.push_back(qMakePair(parts[i], parts[i+1]));
            }
        }

        traits = new MediaFileTypes(false, true, pairs);

        hasSinglePair = (pairs.size() == 1);
    }
    else
    {
        traits = new MediaFileTypes(false, true);
    }

    if (sessionToSaveDialog.find(s) == sessionToSaveDialog.end())
    {
        sessionToSaveDialog[s] = new RvFileDialog(rvDoc,
                                                  traits,
                                                  RvFileDialog::SaveFileRole,
                                                  sheet ?  Qt::Sheet : Qt::Dialog,
                                                  "SaveFileDialog");
    }
    else
    {
        sessionToSaveDialog[s]->setFileTypeTraits(traits);
    }

    RvFileDialog& dialog = *sessionToSaveDialog[s];

    dialog.setFileMode((directory) ?
            RvFileDialog::OneDirectoryName : RvFileDialog::OneFileName);

    if (hasSinglePair) dialog.setFileTypeIndex(2);
    if (path) dialog.setDirectory(UTF8::qconvert(path->c_str()));
    dialog.setRole(RvFileDialog::SaveFileRole);
    dialog.setTitleLabel(QString("Save to File"));
    dialog.setViewMode(RvFileDialog::DetailedFileView);
    dialog.lockViewMode(true);

    string v = "";
    do
    {
        rvDoc->setDocumentDisabled(false, true);
        bool result = dialog.exec();
        rvDoc->view()->setFocus(Qt::OtherFocusReason);
        rvDoc->setDocumentDisabled(false, false);

        if (result)
        {
            QStringList files;
            files = dialog.selectedFiles();

            v = pathConform(UTF8::qconvert(files.at(0)));
            QFileInfo info(UTF8::qconvert(v.c_str()));

            //
            //  QFileInfo says a non-existant file is not writable, so have to check the directory
            //  in that case.
            //
            const bool isDirWritable = TwkUtil::isWritable(UTF8::qconvert(info.absolutePath()).c_str());
            const bool isFileWritable = TwkUtil::isWritable(v.c_str());
	    if ((!info.exists() && !isDirWritable) || (info.exists() && !isFileWritable))
	    {
		QString message = QString("File '") + UTF8::qconvert(v.c_str()) +
			"' is not writable; please check the permissions or choose another location.";
		QMessageBox confirm(QMessageBox::Warning,
				"Permissions",
				message,
				QMessageBox::NoButton, rvDoc, Qt::Sheet);

		QPushButton* q1 = confirm.addButton("OK", QMessageBox::AcceptRole);
		confirm.setIcon(QMessageBox::Question);
		confirm.exec();
		v = "";
	    }
	    else
            if (info.exists())
            {
		QString message = QString("File '") + UTF8::qconvert(v.c_str()) + "' exists; overwrite ?";
		QMessageBox confirm(QMessageBox::Warning,
				"Overwrite",
				message,
				QMessageBox::NoButton, rvDoc, Qt::Sheet);

		QPushButton* q1 = confirm.addButton("Overwrite", QMessageBox::AcceptRole);
		QPushButton* q2 = confirm.addButton("Cancel", QMessageBox::RejectRole);
		confirm.setIcon(QMessageBox::Question);
		confirm.exec();
		if (confirm.clickedButton() != q1) v = "";
            }
        }
        else
        {
            MuLangContext* context = static_cast<MuLangContext*>(p->context());
            ExceptionType::Exception *e =
                new ExceptionType::Exception(context->exceptionType());
            e->string() += "operation cancelled";
            NODE_THREAD.setException(e);
            ProgramException exc(NODE_THREAD, e);
            throw exc;
        }
    }
    while (v.empty());

    NODE_RETURN(stype->allocate(v));
}


NODE_IMPLEMENTATION(setCursor, void)
{
    Session* s = Session::currentSession();
    RvDocument *rvDoc = (RvDocument *)s->opaquePointer();
    rvDoc->view()->setCursor(QCursor(Qt::CursorShape(NODE_ARG(0,int))));
}

NODE_IMPLEMENTATION(alertPanel, int)
{
    Session*                  s     = Session::currentSession();
    RvDocument*               doc   = (RvDocument*)s->opaquePointer();
    Process*                  p     = NODE_THREAD.process();
    bool                      sheet = NODE_ARG(0, bool);
    int                       type  = NODE_ARG(1, int);
    const StringType::String* title = NODE_ARG_OBJECT(2, StringType::String);
    const StringType::String* msg   = NODE_ARG_OBJECT(3, StringType::String);
    const StringType::String* b1    = NODE_ARG_OBJECT(4, StringType::String);
    const StringType::String* b2    = NODE_ARG_OBJECT(5, StringType::String);
    const StringType::String* b3    = NODE_ARG_OBJECT(6, StringType::String);

    QMessageBox box(doc);
    QString temp = UTF8::qconvert(title->c_str());

    if (msg && *msg != *title)
    {
        temp += "\n\n";
        temp += UTF8::qconvert(msg->c_str());
    }

    box.setWindowTitle(UTF8::qconvert(title->c_str()));
    box.setText(temp);
    //box.setDetailedText(QString(msg->c_str()));

#ifdef PLATFORM_DARWIN
    if (sheet) box.setWindowModality(Qt::WindowModal);
#else
    box.setWindowModality(Qt::WindowModal);
#endif

    QPushButton* q1 = box.addButton(UTF8::qconvert(b1->c_str()), QMessageBox::AcceptRole);
    QPushButton* q2 = b2 ? box.addButton(UTF8::qconvert(b2->c_str()), QMessageBox::RejectRole) : 0;
    QPushButton* q3 = b3 ? box.addButton(UTF8::qconvert(b3->c_str()), QMessageBox::ApplyRole) : 0;

    switch (type)
    {
      case 0:
          // Info
          box.setIcon(QMessageBox::Information);
          break;

      case 1:
          // Warning
          box.setIcon(QMessageBox::Warning);
          break;

      case 2:
          // Error
          box.setIcon(QMessageBox::Critical);
          break;
    }

    doc->setDocumentDisabled(true, true);
    box.exec();
    doc->setDocumentDisabled(false);

    int result = 0;

    if      (box.clickedButton() == q1 && b1) result = 0;
    else if (box.clickedButton() == q2 && b2) result = 1;
    else if (box.clickedButton() == q3 && b3) result = 2;

    doc->view()->setFocus(Qt::OtherFocusReason);
    NODE_RETURN(result);
}

NODE_IMPLEMENTATION(stereoSupported, bool)
{
    Session*    s   = Session::currentSession();
    RvDocument* doc = (RvDocument*)s->opaquePointer();

    NODE_RETURN(true);
}

NODE_IMPLEMENTATION(watchFile, void)
{
    Session*    s   = Session::currentSession();
    RvDocument* doc = (RvDocument*)s->opaquePointer();
    StringType::String* file = NODE_ARG_OBJECT(0, StringType::String);
    bool watch = NODE_ARG(1, bool);

    if (watch)
    {
        doc->addWatchFile(file->c_str());
    }
    else
    {
        doc->removeWatchFile(file->c_str());
    }
}

NODE_IMPLEMENTATION(showNetworkDialog, void)
{
    RvApp()->networkWindow()->show();
    RvApp()->networkWindow()->raise();
}

NODE_IMPLEMENTATION(showConsole, void)
{
    RvApp()->console()->show();
    RvApp()->console()->raise();
}

NODE_IMPLEMENTATION(isConsoleVisible, bool)
{
    NODE_RETURN(RvApp()->console()->isVisible());
}

NODE_IMPLEMENTATION(remoteSendMessage, void)
{
    StringType::String* msg        = NODE_ARG_OBJECT(0, StringType::String);
    DynamicArray*       recipients = NODE_ARG_OBJECT(1, DynamicArray);

    if (!msg) throw NilArgumentException(NODE_THREAD);

    if (TwkQtChat::Client* client = RvApp()->networkWindow()->client())
    {
        if (recipients)
        {
            for (int i=0; i < recipients->size(); i++)
            {
                if (StringType::String* s = recipients->element<StringType::String*>(i))
                {
                    client->sendMessage(s->c_str(), msg->c_str());
                }
            }
        }
        else
        {
            client->broadcastMessage(msg->c_str());
        }
    }
}

NODE_IMPLEMENTATION(remoteSendEvent, void)
{
    StringType::String* event      = NODE_ARG_OBJECT(0, StringType::String);
    StringType::String* target     = NODE_ARG_OBJECT(1, StringType::String);
    StringType::String* msg        = NODE_ARG_OBJECT(2, StringType::String);
    DynamicArray*       recipients = NODE_ARG_OBJECT(3, DynamicArray);

    if (!msg || !target || !event) throw NilArgumentException(NODE_THREAD);

    if (TwkQtChat::Client* client = RvApp()->networkWindow()->client())
    {
        if (recipients)
        {
            for (int i=0; i < recipients->size(); i++)
            {
                if (StringType::String* s = recipients->element<StringType::String*>(i))
                {
                    client->sendEvent(s->c_str(),
                                      event->c_str(),
                                      target->c_str(),
                                      msg->c_str());
                }
            }
        }
        else
        {
            client->broadcastEvent(event->c_str(),
                                   target->c_str(),
                                   msg->c_str());
        }
    }
}

NODE_IMPLEMENTATION(remoteSendDataEvent, void)
{
    StringType::String* event  = NODE_ARG_OBJECT(0, StringType::String);
    StringType::String* target = NODE_ARG_OBJECT(1, StringType::String);
    StringType::String* interp = NODE_ARG_OBJECT(2, StringType::String);
    DynamicArray* array        = NODE_ARG_OBJECT(3, DynamicArray);
    DynamicArray* recipients   = NODE_ARG_OBJECT(4, DynamicArray);

    if (!array || !interp || !target || !event ) throw NilArgumentException(NODE_THREAD);

    string newInterp = string("DATAEVENT(") + event->c_str() + "," + target->c_str() + "," + interp->c_str() + ")";

    const unsigned char* datap = array->data<unsigned char>();
    QByteArray qarray((const char *) datap, array->size());

    if (TwkQtChat::Client* client = RvApp()->networkWindow()->client())
    {
        if (recipients)
        {
            for (int i=0; i < recipients->size(); i++)
            {
                if (StringType::String* s = recipients->element<StringType::String*>(i))
                {
                    client->sendData(s->c_str(), newInterp.c_str(), qarray);
                }
            }
        }
    }
}

NODE_IMPLEMENTATION(remoteConnections, Pointer)
{
    Session*                s           = Session::currentSession();
    Process*                p           = NODE_THREAD.process();
    const DynamicArrayType* atype       = static_cast<const DynamicArrayType*>(NODE_THIS.type());
    const StringType*       stype       = static_cast<const StringType*>(atype->elementType());
    DynamicArray*           array       = new DynamicArray(atype, 1);
    RvNetworkDialog*        d           = RvApp()->networkWindow();
    vector<string>          connections = d->connections();
    vector<string>          sessions    = d->sessions();
    vector<string>          myConnections;

    for (int i=0; i < connections.size(); i++)
    {
        if (sessions[i] == s->name()) myConnections.push_back(connections[i]);
    }

    array->resize(myConnections.size());
    for (int i=0; i < myConnections.size(); i++)
    {
        array->element<StringType::String*>(i) = stype->allocate(myConnections[i]);
    }

    NODE_RETURN(array);
}

NODE_IMPLEMENTATION(remoteApplications, Pointer)
{
    Session*                s           = Session::currentSession();
    Process*                p           = NODE_THREAD.process();
    const DynamicArrayType* atype       = static_cast<const DynamicArrayType*>(NODE_THIS.type());
    const StringType*       stype       = static_cast<const StringType*>(atype->elementType());
    DynamicArray*           array       = new DynamicArray(atype, 1);
    RvNetworkDialog*        d           = RvApp()->networkWindow();
    vector<string>          apps        = d->applications();

    array->resize(apps.size());

    for (int i=0; i < apps.size(); i++)
    {
        array->element<StringType::String*>(i) = stype->allocate(apps[i]);
    }

    NODE_RETURN(array);
}

NODE_IMPLEMENTATION(remoteContacts, Pointer)
{
    Session*                s        = Session::currentSession();
    Process*                p        = NODE_THREAD.process();
    const DynamicArrayType* atype    = static_cast<const DynamicArrayType*>(NODE_THIS.type());
    const StringType*       stype    = static_cast<const StringType*>(atype->elementType());
    DynamicArray*           array    = new DynamicArray(atype, 1);
    RvNetworkDialog*        d        = RvApp()->networkWindow();
    vector<string>          contacts = d->contacts();

    array->resize(contacts.size());

    for (int i=0; i < contacts.size(); i++)
    {
        array->element<StringType::String*>(i) = stype->allocate(contacts[i]);
    }

    NODE_RETURN(array);
}

NODE_IMPLEMENTATION(remoteLocalContactName, Pointer)
{
    const StringType* stype = static_cast<const StringType*>(NODE_THIS.type());

    NODE_RETURN(stype->allocate(RvApp()->networkWindow()->localContactName().c_str()));
}

NODE_IMPLEMENTATION(setRemoteLocalContactName, void)
{
    const StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);

    if (!name) throw NilArgumentException(NODE_THREAD);

    RvApp()->networkWindow()->setLocalContactName(name->c_str());
}

NODE_IMPLEMENTATION(remoteConnect, void)
{
    const StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
    const StringType::String* host = NODE_ARG_OBJECT(1, StringType::String);
    int                       port = NODE_ARG(2, int);

    if (TwkQtChat::Client* client = RvApp()->networkWindow()->client())
    {
        if (port)
        {
            client->connectTo(name->c_str(), host->c_str(), port);
        }
        else
        {
            client->connectTo(name->c_str(), host->c_str());
        }
    }
}

NODE_IMPLEMENTATION(remoteDisconnect, void)
{
    StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);

    if (TwkQtChat::Client* client = RvApp()->networkWindow()->client())
    {
        client->disconnectFrom(name->c_str());
    }
}

NODE_IMPLEMENTATION(remoteNetwork, void)
{
    bool on = NODE_ARG(0, bool);
    RvNetworkDialog* d = RvApp()->networkWindow();

    if (on)
    {
        if (!d->client()) d->toggleServer();
    }
    else
    {
        if (d->client()) d->toggleServer();
    }
}

NODE_IMPLEMENTATION(remoteConnectionIsIncoming, bool)
{
    StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);

    if (auto client = RvApp()->networkWindow()->client())
    {
        return client->isIncoming(name->c_str());
    }

    return false; // network is not started
}

NODE_IMPLEMENTATION(spoofConnectionStream, void)
{
    StringType::String* name = NODE_ARG_OBJECT(0, StringType::String);
    float timeScale = NODE_ARG(1, float);
    bool  verbose   = NODE_ARG(2, bool);

    RvNetworkDialog* d = RvApp()->networkWindow();

    d->spoofConnectionStream (UTF8::qconvert(name->c_str()), timeScale, verbose);
}

NODE_IMPLEMENTATION(remoteNetworkStatus, int)
{
    RvNetworkDialog* d = RvApp()->networkWindow();
    NODE_RETURN(d->client() ? 1 : 0);
}

NODE_IMPLEMENTATION(remoteDefaultPermission, int)
{
    NODE_RETURN(RvApp()->networkWindow()->defaultPermission());
}

NODE_IMPLEMENTATION(setRemoteDefaultPermission, void)
{
    int perm = NODE_ARG(0, int);

    NODE_RETURN(RvApp()->networkWindow()->setDefaultPermission(perm));
}

NODE_IMPLEMENTATION(writeSetting, void)
{
    StringType::String*      group = NODE_ARG_OBJECT(0, StringType::String);
    StringType::String*      name  = NODE_ARG_OBJECT(1, StringType::String);
    VariantInstance*         vobj  = NODE_ARG_OBJECT(2, VariantInstance);
    const VariantTagType*    tt    = vobj->tagType();
    const SettingsValueType* vtype = static_cast<const SettingsValueType*>(tt->variantType());

    QVariant value;

    switch (vtype->valueType(vobj))
    {
      case SettingsValueType::NoType:
          break;
      case SettingsValueType::FloatType:
          value.setValue(double(*vobj->data<float>()));
          break;
      case SettingsValueType::IntType:
          value.setValue(*vobj->data<int>());
          break;
      case SettingsValueType::StringType:
          value.setValue(UTF8::qconvert(vobj->data<StringType::String>()->c_str()));
          break;
      case SettingsValueType::BoolType:
          value.setValue(*vobj->data<bool>());
          break;
      case SettingsValueType::StringArrayType:
          {
              DynamicArray* array = vobj->data<DynamicArray>();
              QStringList list;

              for (size_t i = 0; i < array->size(); i++)
              {
                  StringType::String* s = array->element<StringType::String*>(i);
                  list.push_back(s->c_str());
              }

              value.setValue(list);
          }
          break;
      case SettingsValueType::IntArrayType:
          {
              DynamicArray* array = vobj->data<DynamicArray>();
              QList<QVariant> list;

              for (size_t i = 0; i < array->size(); i++)
              {
                  list.push_back(QVariant(array->element<int>(i)));
              }

              value.setValue(list);
          }
          break;
      case SettingsValueType::FloatArrayType:
          {
              DynamicArray* array = vobj->data<DynamicArray>();
              QList<QVariant> list;

              for (size_t i = 0; i < array->size(); i++)
              {
                  list.push_back(QVariant(double(array->element<float>(i))));
              }

              value.setValue(list);
          }
          break;
    }

    RV_QSETTINGS;

    settings.beginGroup(UTF8::qconvert(group->c_str()));
    settings.setValue(UTF8::qconvert(name->c_str()), value);
    settings.endGroup();
}

static QVariant::Type
settingsTypeToQVariantType(SettingsValueType::ValueType t)
{
    switch (t)
    {
      case SettingsValueType::FloatType: return QVariant::Double;
      case SettingsValueType::IntType: return QVariant::Int;
      case SettingsValueType::StringType: return QVariant::String;
      case SettingsValueType::BoolType: return QVariant::Bool;
      case SettingsValueType::FloatArrayType: return QVariant::List;// same as in
      case SettingsValueType::IntArrayType: return QVariant::List;  // same as float
      case SettingsValueType::StringArrayType: return QVariant::StringList;
      default:
          break;
    }

    return QVariant::List;
}

NODE_IMPLEMENTATION(readSetting, Pointer)
{
    Process*            p          = NODE_THREAD.process();
    MuLangContext*      c          = static_cast<MuLangContext*>(p->context());
    StringType::String* group      = NODE_ARG_OBJECT(0, StringType::String);
    StringType::String* name       = NODE_ARG_OBJECT(1, StringType::String);
    VariantInstance*    defaultObj = NODE_ARG_OBJECT(2, VariantInstance);
    const StringType*   stype      = static_cast<const StringType*>(name->type());

    RV_QSETTINGS;

    settings.beginGroup(UTF8::qconvert(group->c_str()));
    if (!settings.contains(UTF8::qconvert(name->c_str())))
    {
        settings.endGroup();
        NODE_RETURN(defaultObj);
    }
    QVariant value = settings.value(UTF8::qconvert(name->c_str()));
    settings.endGroup();

    const SettingsValueType* vtype =
        static_cast<const SettingsValueType*>(NODE_THIS.type());

    VariantInstance* vobj = 0;
    QVariant::Type type = defaultObj ? settingsTypeToQVariantType(vtype->valueType(defaultObj))
                                     : value.type();

    switch (type)
    {
      case QVariant::Bool:
          vobj = VariantInstance::allocate(vtype->boolType());
          *vobj->data<bool>() = value.toBool();
          break;
      case QVariant::Int:
          vobj = VariantInstance::allocate(vtype->intType());
          *vobj->data<int>() = value.toInt();
          break;
      case QVariant::Double:
          vobj = VariantInstance::allocate(vtype->floatType());
          *vobj->data<float>() = value.toDouble();
          break;
      case QVariant::String:
          {
              vobj = VariantInstance::allocate(vtype->stringType());
              StringType::String* s = vobj->data<StringType::String>();
              s->set(value.toString().toUtf8().constData());
          }
          break;
      case QVariant::StringList:
          {
              //
              //    Creating it will also create the tagged object it
              //    contains. So we don't make a new DynamicArray we just
              //    get the one that was created.
              //
              vobj = VariantInstance::allocate(vtype->stringArrayType());
              DynamicArray* array = vobj->data<DynamicArray>();

              QStringList list = value.toStringList();
              array->resize(list.size());

              for (size_t i=0; i < array->size(); i++)
              {
                  array->element<StringType::String*>(i) =
                      stype->allocate(list[i].toUtf8().constData());
              }
          }
          break;
      case QVariant::List:
          {
              QVariantList list = value.toList();

              if (list.empty()) throwBadArgumentException(NODE_THIS,
                                                          NODE_THREAD,
                                                          "Bad value in settings file");

              QVariant one = list.front();
              bool isfloat = vtype->valueType(defaultObj) == SettingsValueType::FloatArrayType;

              vobj = VariantInstance::allocate(isfloat
                                               ? vtype->floatArrayType()
                                               : vtype->intArrayType());

              DynamicArray* array = vobj->data<DynamicArray>();
              array->resize(list.size());

              for (size_t i=0; i < array->size(); i++)
              {
                  if (isfloat)
                  {
                      array->element<float>(i) = float(list[i].toDouble());
                  }
                  else
                  {
                      array->element<int>(i) = list[i].toInt();
                  }
              }
          }
          break;
      default:
          break;
    }

    NODE_RETURN(vobj);
}

struct StringPairTupleStruct
{
    Mu::StringType::String* s0;
    Mu::StringType::String* s1;
};

NODE_IMPLEMENTATION(httpGet, void)
{
    Process*            p          = NODE_THREAD.process();
    MuLangContext*      c          = static_cast<MuLangContext*>(p->context());
    Session*            s          = Session::currentSession();
    StringType::String* url        = NODE_ARG_OBJECT(0, StringType::String);
    ClassInstance*      hlist      = NODE_ARG_OBJECT(1, ClassInstance);
    StringType::String* replyEvent = NODE_ARG_OBJECT(2, StringType::String);
    StringType::String* authEvent  = NODE_ARG_OBJECT(3, StringType::String);
    StringType::String* progEvent  = NODE_ARG_OBJECT(4, StringType::String);
    bool                ignore     = NODE_ARG(5, bool);
    bool                urlIsEncoded = NODE_ARG(6, bool);

    RvWebManager::HeaderList headers;

    for (List list(p, hlist); !list.isNil(); list++)
    {
        if (ClassInstance* spair = list.value<ClassInstance*>())
        {
            const StringPairTupleStruct* sp = spair->data<StringPairTupleStruct>();
            if (!sp->s0 || !sp->s1) throwBadArgumentException(NODE_THIS, NODE_THREAD, "Bad header argument");

            headers.push_back(QPair<QString,QString>(UTF8::qconvert(sp->s0->c_str()),
                                                     UTF8::qconvert(sp->s1->c_str())));
        }
    }

    if (RvWebManager* m = RvApp()->webManager())
    {
        m->httpGet(UTF8::qconvert(url->c_str()),
                   headers,
                   s,
                   UTF8::qconvert(replyEvent ? replyEvent->c_str() : ""),
                   UTF8::qconvert(authEvent ? authEvent->c_str() : ""),
                   UTF8::qconvert(progEvent ? progEvent->c_str() : ""),
                   ignore,
                   urlIsEncoded);
    }
}

NODE_IMPLEMENTATION(httpPostString, void)
{
    Process*            p          = NODE_THREAD.process();
    MuLangContext*      c          = static_cast<MuLangContext*>(p->context());
    Session*            s          = Session::currentSession();
    StringType::String* url        = NODE_ARG_OBJECT(0, StringType::String);
    ClassInstance*      hlist      = NODE_ARG_OBJECT(1, ClassInstance);
    StringType::String* postString = NODE_ARG_OBJECT(2, StringType::String);
    StringType::String* replyEvent = NODE_ARG_OBJECT(3, StringType::String);
    StringType::String* authEvent  = NODE_ARG_OBJECT(4, StringType::String);
    StringType::String* progEvent  = NODE_ARG_OBJECT(5, StringType::String);
    bool                ignore     = NODE_ARG(6, bool);
    bool                urlIsEncoded = NODE_ARG(7, bool);

    RvWebManager::HeaderList headers;

    for (List list(p, hlist); !list.isNil(); list++)
    {
        if (ClassInstance* spair = list.value<ClassInstance*>())
        {
            const StringPairTupleStruct* sp = spair->data<StringPairTupleStruct>();
            if (!sp->s0 || !sp->s1) throwBadArgumentException(NODE_THIS, NODE_THREAD, "Bad header argument");

            headers.push_back(QPair<QString,QString>(UTF8::qconvert(sp->s0->c_str()),
                                                     UTF8::qconvert(sp->s1->c_str())));
        }
    }

    if (RvWebManager* m = RvApp()->webManager())
    {
        m->httpPost(url->c_str(),
                    headers,
                    postString ? QByteArray(postString->c_str()) : QByteArray(),
                    s,
                    UTF8::qconvert(replyEvent ? replyEvent->c_str() : ""),
                    UTF8::qconvert(authEvent ? authEvent->c_str() : ""),
                    UTF8::qconvert(progEvent ? progEvent->c_str() : ""),
                    ignore,
                    urlIsEncoded);
    }
}

NODE_IMPLEMENTATION(httpPostData, void)
{
    Process*            p          = NODE_THREAD.process();
    MuLangContext*      c          = static_cast<MuLangContext*>(p->context());
    Session*            s          = Session::currentSession();
    StringType::String* url        = NODE_ARG_OBJECT(0, StringType::String);
    ClassInstance*      hlist      = NODE_ARG_OBJECT(1, ClassInstance);
    DynamicArray*       postData   = NODE_ARG_OBJECT(2, DynamicArray);
    StringType::String* replyEvent = NODE_ARG_OBJECT(3, StringType::String);
    StringType::String* authEvent  = NODE_ARG_OBJECT(4, StringType::String);
    StringType::String* progEvent  = NODE_ARG_OBJECT(5, StringType::String);
    bool                ignore     = NODE_ARG(6, bool);
    bool                urlIsEncoded = NODE_ARG(7, bool);

    RvWebManager::HeaderList headers;

    for (List list(p, hlist); !list.isNil(); list++)
    {
        if (ClassInstance* spair = list.value<ClassInstance*>())
        {
            const StringPairTupleStruct* sp = spair->data<StringPairTupleStruct>();
            if (!sp->s0 || !sp->s1) throwBadArgumentException(NODE_THIS, NODE_THREAD, "Bad header argument");

            headers.push_back(QPair<QString,QString>(UTF8::qconvert(sp->s0->c_str()),
                                                     UTF8::qconvert(sp->s1->c_str())));
        }
    }

    if (RvWebManager* m = RvApp()->webManager())
    {
        const unsigned char* datap = (postData) ? postData->data<unsigned char>() : 0;

        m->httpPost(url->c_str(),
                    headers,
                    postData ? QByteArray((const char *) datap, postData->size()) : QByteArray(),
                    s,
                    UTF8::qconvert(replyEvent ? replyEvent->c_str() : ""),
                    UTF8::qconvert(authEvent ? authEvent->c_str() : ""),
                    UTF8::qconvert(progEvent ? progEvent->c_str() : ""),
                    ignore,
                    urlIsEncoded);
    }
}

NODE_IMPLEMENTATION(httpPutString, void)
{
    Process*            p          = NODE_THREAD.process();
    MuLangContext*      c          = static_cast<MuLangContext*>(p->context());
    Session*            s          = Session::currentSession();
    StringType::String* url        = NODE_ARG_OBJECT(0, StringType::String);
    ClassInstance*      hlist      = NODE_ARG_OBJECT(1, ClassInstance);
    StringType::String* putString  = NODE_ARG_OBJECT(2, StringType::String);
    StringType::String* replyEvent = NODE_ARG_OBJECT(3, StringType::String);
    StringType::String* authEvent  = NODE_ARG_OBJECT(4, StringType::String);
    StringType::String* progEvent  = NODE_ARG_OBJECT(5, StringType::String);
    bool                ignore     = NODE_ARG(6, bool);
    bool                urlIsEncoded = NODE_ARG(7, bool);

    RvWebManager::HeaderList headers;

    for (List list(p, hlist); !list.isNil(); list++)
    {
        if (ClassInstance* spair = list.value<ClassInstance*>())
        {
            const StringPairTupleStruct* sp = spair->data<StringPairTupleStruct>();
            if (!sp->s0 || !sp->s1) throwBadArgumentException(NODE_THIS, NODE_THREAD, "Bad header argument");

            headers.push_back(QPair<QString,QString>(UTF8::qconvert(sp->s0->c_str()),
                                                     UTF8::qconvert(sp->s1->c_str())));
        }
    }

    if (RvWebManager* m = RvApp()->webManager())
    {
        m->httpPut( UTF8::qconvert(url->c_str()),
                    headers,
                    putString ? QByteArray(putString->c_str()) : QByteArray(),
                    s,
                    UTF8::qconvert(replyEvent ? replyEvent->c_str() : ""),
                    UTF8::qconvert(authEvent ? authEvent->c_str() : ""),
                    UTF8::qconvert(progEvent ? progEvent->c_str() : ""),
                    ignore,
                    urlIsEncoded);
    }
}

NODE_IMPLEMENTATION(httpPutData, void)
{
    Process*            p            = NODE_THREAD.process();
    MuLangContext*      c            = static_cast<MuLangContext*>(p->context());
    Session*            s            = Session::currentSession();
    StringType::String* url          = NODE_ARG_OBJECT(0, StringType::String);
    ClassInstance*      hlist        = NODE_ARG_OBJECT(1, ClassInstance);
    DynamicArray*       putData      = NODE_ARG_OBJECT(2, DynamicArray);
    StringType::String* replyEvent   = NODE_ARG_OBJECT(3, StringType::String);
    StringType::String* authEvent    = NODE_ARG_OBJECT(4, StringType::String);
    StringType::String* progEvent    = NODE_ARG_OBJECT(5, StringType::String);
    bool                ignore       = NODE_ARG(6, bool);
    bool                urlIsEncoded = NODE_ARG(7, bool);

    RvWebManager::HeaderList headers;

    for (List list(p, hlist); !list.isNil(); list++)
    {
        if (ClassInstance* spair = list.value<ClassInstance*>())
        {
            const StringPairTupleStruct* sp = spair->data<StringPairTupleStruct>();
            if (!sp->s0 || !sp->s1) throwBadArgumentException(NODE_THIS, NODE_THREAD, "Bad header argument");

            headers.push_back(QPair<QString,QString>(UTF8::qconvert(sp->s0->c_str()),
                                                     UTF8::qconvert(sp->s1->c_str())));
        }
    }

    if (RvWebManager* m = RvApp()->webManager())
    {
        const unsigned char* datap = (putData) ? putData->data<unsigned char>() : 0;

        m->httpPut( UTF8::qconvert(url->c_str()),
                    headers,
                    putData ? QByteArray((const char *) datap, putData->size()) : QByteArray(),
                    s,
                    UTF8::qconvert(replyEvent ? replyEvent->c_str() : ""),
                    UTF8::qconvert(authEvent ? authEvent->c_str() : ""),
                    UTF8::qconvert(progEvent ? progEvent->c_str() : ""),
                    ignore,
                    urlIsEncoded);
    }
}

NODE_IMPLEMENTATION(mainWindowWidget, Pointer)
{
    Process*       p   = NODE_THREAD.process();
    MuLangContext* c   = static_cast<MuLangContext*>(p->context());
    Session*       s   = Session::currentSession();
    RvDocument*    doc = reinterpret_cast<RvDocument*>(s->opaquePointer());

    const QMainWindowType* type =
        c->findSymbolOfTypeByQualifiedName
            <QMainWindowType>(c->internName("qt.QMainWindow"), false);

    NODE_RETURN(makeinstance(type, static_cast<QMainWindow*>(doc)));
}

NODE_IMPLEMENTATION(mainViewWidget, Pointer)
{
    Process*       p   = NODE_THREAD.process();
    MuLangContext* c   = static_cast<MuLangContext*>(p->context());
    Session*       s   = Session::currentSession();
    RvDocument*    doc = reinterpret_cast<RvDocument*>(s->opaquePointer());
    QWidget*       w   = doc->view();

    const QWidgetType* type =
        c->findSymbolOfTypeByQualifiedName
            <QWidgetType>(c->internName("qt.QWidget"), false);

    NODE_RETURN(makeinstance(type, static_cast<QWidget*>(w)));
}

NODE_IMPLEMENTATION(prefTabWidget, Pointer)
{
    Process*       p   = NODE_THREAD.process();
    MuLangContext* c   = static_cast<MuLangContext*>(p->context());
    Session*       s   = Session::currentSession();

    QTabWidget* w = RvApp()->prefDialog()->tabWidget();

    const QTabWidgetType* type =
        c->findSymbolOfTypeByQualifiedName
            <QTabWidgetType>(c->internName("qt.QTabWidget"), false);

    NODE_RETURN(makeinstance(type, w));
}

NODE_IMPLEMENTATION(sessionBottomToolBar, Pointer)
{
    Process*       p   = NODE_THREAD.process();
    MuLangContext* c   = static_cast<MuLangContext*>(p->context());
    Session*       s   = Session::currentSession();
    RvDocument*    doc = reinterpret_cast<RvDocument*>(s->opaquePointer());

    QToolBar*      toolBar = doc->bottomViewToolBar();

    const QToolBarType* type =
        c->findSymbolOfTypeByQualifiedName
            <QToolBarType>(c->internName("qt.QToolBar"), false);

    NODE_RETURN(makeinstance(type, toolBar));
}

NODE_IMPLEMENTATION(networkAccessManager, Pointer)
{
    Process*       p = NODE_THREAD.process();
    MuLangContext* c = static_cast<MuLangContext*>(p->context());
    Session*       s = Session::currentSession();
    RvWebManager*  m = RvApp()->webManager();

    const QNetworkAccessManagerType* type =
        c->findSymbolOfTypeByQualifiedName
            <QNetworkAccessManagerType>(c->internName("qt.QNetworkAccessManager"), false);

    NODE_RETURN(makeinstance(type, static_cast<QNetworkAccessManager*>(m->netManager())));
}

NODE_IMPLEMENTATION(javascriptMuExport, void)
{
    Process*       p   = NODE_THREAD.process();
    MuLangContext* c   = static_cast<MuLangContext*>(p->context());
    Session*       s   = Session::currentSession();
    RvDocument*    doc = reinterpret_cast<RvDocument*>(s->opaquePointer());

    QWebEnginePage* frame = Mu::object<QWebEnginePage>(NODE_ARG_OBJECT(0, ClassInstance));

    RvJavaScriptObject* obj = new RvJavaScriptObject(doc, frame);
}

NODE_IMPLEMENTATION(sessionFromUrl, void)
{
    StringType::String* url = NODE_ARG_OBJECT(0, StringType::String);

    if (!url) throw NilArgumentException(NODE_THREAD);

    RvApp()->sessionFromUrl(url->c_str());
}


NODE_IMPLEMENTATION(putUrlOnClipboard, void)
{
    StringType::String* url = NODE_ARG_OBJECT(0, StringType::String);
    StringType::String* title = NODE_ARG_OBJECT(1, StringType::String);
    bool                doEncode = NODE_ARG(2, bool);

    if (!url) throw NilArgumentException(NODE_THREAD);

    RvApp()->putUrlOnClipboard(url->c_str(),
                               title->c_str(),
                               doEncode);

    /*
    NO WORKY

    QUrl qurl;
    qurl.setScheme("rvlink");
    qurl.setUrl (url->c_str(), QUrl::TolerantMode);
    cerr << "made QUrl from '" << url->c_str() << "', valid " << qurl.isValid() << endl;
    QList<QUrl> qlist;
    qlist.append (qurl);

    //  XXX memory leak, but a tiny one.
    QMimeData *qmd = new QMimeData();

    qmd->setUrls(qlist);
    qmd->setText(url->c_str());

    QApplication::clipboard()->setMimeData(qmd);
    */
}

NODE_IMPLEMENTATION(myNetworkPort, int)
{
    NODE_RETURN(RvApp()->networkWindow()->myPort());
}

NODE_IMPLEMENTATION(myNetworkHost, Pointer)
{
    const StringType* stype = static_cast<const StringType*>(NODE_THIS.type());
    StringType::String* str = 0;

    Options& opts = Options::sharedOptions();
    if (opts.networkHost)
    {
        str = stype->allocate(opts.networkHost);
    }
    else
    {
        QString host   = QHostInfo::localHostName();
        //
        //  On max os x 10.6, the returned "local" name is actually
        //  the fully-qualified domain name.  So trim it.
        //
        QStringList parts = host.split(".");
        host = parts[0];

        QString domain = QHostInfo::localDomainName();
        if (domain.size()) host = host + "." + domain;

        str = stype->allocate(UTF8::qconvert(host));
    }
    NODE_RETURN(str);
}

//
//  Wing-dinging 32-bit encryption !
//

NODE_IMPLEMENTATION(encodePassword, Pointer)
{
    StringType::String* pass = NODE_ARG_OBJECT(0, StringType::String);
    const StringType* stype = static_cast<const StringType*>(NODE_THIS.type());

    string enc = TwkQtBase::encode(pass->c_str());

    NODE_RETURN(stype->allocate(enc.c_str()));
}

NODE_IMPLEMENTATION(decodePassword, Pointer)
{
    StringType::String* pass = NODE_ARG_OBJECT(0, StringType::String);
    const StringType* stype = static_cast<const StringType*>(NODE_THIS.type());

    if (!pass)
    {
        throwBadArgumentException(NODE_THIS, NODE_THREAD, "Null password could not be decoded.");
    }

    string dec = TwkQtBase::decode(pass->c_str());

    NODE_RETURN(stype->allocate(dec.c_str()));
}

NODE_IMPLEMENTATION(cacheDir, Pointer)
{
    const StringType* stype = static_cast<const StringType*>(NODE_THIS.type());

    QStringList cacheLocations = QStandardPaths::standardLocations(QStandardPaths::CacheLocation);
    QDir cacheDir(cacheLocations.front());

    if (!cacheDir.exists()) cacheDir.mkpath(cacheDir.absolutePath());

    NODE_RETURN(stype->allocate(cacheDir.absolutePath().toUtf8().constData()));
}

namespace {

void
openUrlWithDesktopServices (const Mu::Node& node, Mu::Thread& thread, QUrl& qurl)
{
    #ifdef PLATFORM_LINUX
        string pathS;
        const char* path =  getenv("LD_LIBRARY_PATH");
        if (path) pathS = path;
        unsetenv ("LD_LIBRARY_PATH");
    #endif

    if (qurl.isValid()) QDesktopServices::openUrl(qurl);
    else throwBadArgumentException(node, thread, "QUrl argument is not valid");

    #ifdef PLATFORM_LINUX
        if (path) setenv ("LD_LIBRARY_PATH", pathS.c_str(), 1);
    #endif
}

}

NODE_IMPLEMENTATION(openUrl, void)
{
    StringType::String* url = NODE_ARG_OBJECT(0, StringType::String);
    QUrl qurl(url->c_str());

    openUrlWithDesktopServices(NODE_THIS, NODE_THREAD, qurl);
}

NODE_IMPLEMENTATION(openUrlFromUrl, void)
{
    Process*       p   = NODE_THREAD.process();
    MuLangContext* c   = static_cast<MuLangContext*>(p->context());

    Pointer qp = NODE_ARG(0, Pointer);

    if (!qp) throwBadArgumentException(NODE_THIS, NODE_THREAD, "Nil QUrl argument");

    QUrl& qurl = getqtype<QUrlType>(qp);

    openUrlWithDesktopServices(NODE_THIS, NODE_THREAD, qurl);
}

NODE_IMPLEMENTATION(setPresentationMode, void)
{
    bool value = NODE_ARG(0, bool);

    //
    //  Turning presentation mode on/off may mean that
    //  we want to cache different frame buffers, so
    //  kick cache to be sure we recache if necessary.
    //
    Session* s = Session::currentSession();
    Session::CachingMode mode = s->cachingMode();
    s->setCaching(Session::NeverCache);

    if (RvApp()->documents().size() == 1)
    {
        RvApp()->setPresentationMode(value);
    }
    else if (value)
    {
        Session* s = Session::currentSession();
        RvDocument *rvDoc = (RvDocument *)s->opaquePointer();

        QString message = "Cannot start presentation mode when multiple sessions are active";
        QMessageBox confirm(QMessageBox::Warning,
                            "Presentation Mode",
                            message,
                            QMessageBox::NoButton, rvDoc, Qt::Sheet);

        QPushButton* q1 = confirm.addButton("Ok", QMessageBox::AcceptRole);
        confirm.setIcon(QMessageBox::Warning);
        confirm.exec();
    }

    s->setCaching(mode);
}

NODE_IMPLEMENTATION(presentationMode, bool)
{
    NODE_RETURN(RvApp()->isInPresentationMode());
}

NODE_IMPLEMENTATION(packageListFromSetting, Pointer)
{
    StringType::String*     setting = NODE_ARG_OBJECT(0, StringType::String);
    const DynamicArrayType* atype   = static_cast<const DynamicArrayType*>(NODE_THIS.type());
    const StringType*       stype   = static_cast<const StringType*>(atype->elementType());
    DynamicArray*           array   = new DynamicArray(atype, 1);

    if (!setting) throwBadArgumentException(NODE_THIS, NODE_THREAD, "Nil setting name.");

    RV_QSETTINGS;

    settings.beginGroup("ModeManager");

    QStringList l = Rv::PackageManager::swapAppDir(settings.value (setting->c_str(), QStringList()).toStringList(), false);

    settings.endGroup();

    array->resize (l.size());
    for (int i = 0; i < l.size(); i++)
    {
        array->element<StringType::String*>(i) = stype->allocate(UTF8::qconvert(l[i]));
    }

    NODE_RETURN(array);
}

NODE_IMPLEMENTATION(showTopViewToolbar, void)
{
    Session*    s     = Session::currentSession();
    RvDocument* doc   = reinterpret_cast<RvDocument*>(s->opaquePointer());
    bool        value = NODE_ARG(0, bool);
    doc->topViewToolBar()->makeActive(value);
}

NODE_IMPLEMENTATION(isTopViewToolbarVisible, bool)
{
    Session*    s   = Session::currentSession();
    RvDocument* doc = reinterpret_cast<RvDocument*>(s->opaquePointer());
    NODE_RETURN(doc->topViewToolBar()->isVisible());
}

NODE_IMPLEMENTATION(showBottomViewToolbar, void)
{
    Session*    s     = Session::currentSession();
    RvDocument* doc   = reinterpret_cast<RvDocument*>(s->opaquePointer());
    bool        value = NODE_ARG(0, bool);
    doc->bottomViewToolBar()->makeActive(value);
}

NODE_IMPLEMENTATION(isBottomViewToolbarVisible, bool)
{
    Session*    s   = Session::currentSession();
    RvDocument* doc = reinterpret_cast<RvDocument*>(s->opaquePointer());
    NODE_RETURN(doc->bottomViewToolBar()->isVisible());
}

NODE_IMPLEMENTATION(editNodeSource, void)
{
    Session*            s    = Session::currentSession();
    RvDocument*         doc  = reinterpret_cast<RvDocument*>(s->opaquePointer());
    StringType::String* node = NODE_ARG_OBJECT(0, StringType::String);

    if (node)
    {
        doc->editSourceNode(node->c_str());
    }
    else
    {
        throwBadArgumentException(NODE_THIS, NODE_THREAD, "valid nodeName required");
    }
}

NODE_IMPLEMENTATION(editProfiles, void)
{
    RvApp()->profileManager()->show();
}

NODE_IMPLEMENTATION(validateShotgunToken, Mu::Pointer)
{
    int                 index = NODE_ARG(0, int);
    StringType::String* tag   = NODE_ARG_OBJECT(1, StringType::String);

    Process*                  p       = NODE_THREAD.process();
    MuLangContext*            context = static_cast<MuLangContext*>(p->context());
    const StringType*         stype   = context->stringType();

    // Note: This is no longer relevant in RV Open Source but kept to maintain
    // backward compatibility.
    string err = "";

    NODE_RETURN(stype->allocate(err));
}

NODE_IMPLEMENTATION(launchTLI, void)
{
    // Note: This is no longer relevant in RV Open Source but kept to maintain
    // backward compatibility.
}

NODE_IMPLEMENTATION(rvioSetup, void)
{
    // Note: This is no longer relevant in RV Open Source but kept to maintain
    // backward compatibility.
}

} // Rv namespace
