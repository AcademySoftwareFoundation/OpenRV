//******************************************************************************
// Copyright (c) 2001-2005 Tweak Inc. All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <assert.h>
#include <UICommands.h>

namespace RVIO
{

    using namespace TwkApp;
    using namespace Mu;
    using namespace std;

    void initUICommands(MuLangContext* context)
    {
        USING_MU_FUNCTION_SYMBOLS;
        MuLangContext* c = context;
        Symbol* root = context->globalScope();
        Name cname = c->internName("commands");
        Mu::Module* commands = root->findSymbolOfType<Mu::Module>(cname);

        commands->addSymbols(
            new Function(c, "resizeFit", resizeFit, None, Return, "void", End),

            new Function(c, "setViewSize", setViewSize, None, Return, "void",
                         Parameters, new Param(c, "width", "int"),
                         new Param(c, "height", "int"), End),

            new Function(c, "popupMenu", popupMenu, None, Return, "void",
                         Parameters, new Param(c, "event", "Event"),
                         new Param(c, "menu", "MenuItem[]", Value(Pointer(0))),
                         End),

            new Function(
                c, "popupMenuAtPoint", popupMenuAtPoint, None, Return, "void",
                Parameters, new Param(c, "x", "int"), new Param(c, "y", "int"),
                new Param(c, "menu", "MenuItem[]", Value(Pointer(0))), End),

            new Function(c, "setWindowTitle", setWindowTitle, None, Return,
                         "void", Parameters, new Param(c, "title", "string"),
                         End),

            new Function(c, "center", center, None, Return, "void", End),

            new Function(c, "close", close, None, Return, "void", End),

            new Function(c, "toggleMenuBar", toggleMenuBar, None, Return,
                         "void", End),

            new Function(c, "isMenuBarVisible", isMenuBarVisible, None, Return,
                         "bool", End),

            new SymbolicConstant(c, "OneExistingFile", "int", Value(0)),
            new SymbolicConstant(c, "ManyExistingFiles", "int", Value(0)),
            new SymbolicConstant(c, "ManyExistingFilesAndDirectories", "int",
                                 Value(0)),
            new SymbolicConstant(c, "OneFileName", "int", Value(0)),
            new SymbolicConstant(c, "OneDirectory", "int", Value(0)),

            new Function(c, "openMediaFileDialog", openMediaFileDialog, None,
                         Return, "string[]", Parameters,
                         new Param(c, "associated", "bool"),
                         new Param(c, "selectType", "int"),
                         new Param(c, "filter", "string", Value(0)),
                         new Param(c, "defaultPath", "string", Value(0)),
                         new Param(c, "label", "string", Value(0)), End),

            new Function(c, "openFileDialog", openFileDialog, None, Return,
                         "string[]", Parameters,
                         new Param(c, "associated", "bool"),
                         new Param(c, "multiple", "bool", Value(false)),
                         new Param(c, "directory", "bool", Value(false)),
                         new Param(c, "filter", "string", Value(0)),
                         new Param(c, "defaultPath", "string", Value(0)), End),

            new Function(c, "saveFileDialog", saveFileDialog, None, Return,
                         "string", Parameters,
                         new Param(c, "associated", "bool"),
                         new Param(c, "filter", "string", Value(0)),
                         new Param(c, "defaultPath", "string", Value(0)), End),

            new SymbolicConstant(c, "CursorNone", "int", Value(0)),
            new SymbolicConstant(c, "CursorArrow", "int", Value(2)),
            new SymbolicConstant(c, "CursorDefault", "int", Value(1)),

            new Function(c, "setCursor", setCursor, None, Return, "void",
                         Parameters, new Param(c, "cursorType", "int"), End),

            new SymbolicConstant(c, "InfoAlert", "int", Value(0)),
            new SymbolicConstant(c, "WarningAlert", "int", Value(1)),
            new SymbolicConstant(c, "ErrorAlert", "int", Value(2)),

            new Function(c, "stereoSupported", stereoSupported, None, Return,
                         "bool", End),

            new Function(c, "alertPanel", alertPanel, None, Return, "int",
                         Parameters, new Param(c, "associated", "bool"),
                         new Param(c, "type", "int"),
                         new Param(c, "title", "string"),
                         new Param(c, "message", "string"),
                         new Param(c, "button0", "string"),
                         new Param(c, "button1", "string"),
                         new Param(c, "button2", "string"), End),

            new Function(c, "watchFile", watchFile, None, Return, "void",
                         Parameters, new Param(c, "filename", "string"),
                         new Param(c, "watch", "bool"), End),

            new Function(c, "showConsole", showConsole, None, Return, "void",
                         End),

            new Function(c, "isConsoleVisible", isConsoleVisible, None, Return,
                         "bool", End),
            // network

            new Function(c, "remoteSendMessage", remoteSendMessage, None,
                         Return, "void", Parameters,
                         new Param(c, "message", "string"),
                         new Param(c, "recipients", "string[]", Value(0)), End),

            new Function(c, "remoteSendEvent", remoteSendEvent, None, Return,
                         "void", Parameters, new Param(c, "event", "string"),
                         new Param(c, "target", "string"),
                         new Param(c, "contents", "string"),
                         new Param(c, "recipients", "string[]", Value(0)), End),

            new Function(c, "remoteConnections", remoteConnections, None,
                         Return, "string[]", End),

            new Function(c, "remoteApplications", remoteApplications, None,
                         Return, "string[]", End),

            new Function(c, "remoteContacts", remoteContacts, None, Return,
                         "string[]", End),

            new Function(c, "remoteLocalContactName", remoteLocalContactName,
                         None, Return, "string", End),

            new Function(c, "remoteConnect", remoteConnect, None, Return,
                         "void", Parameters, new Param(c, "name", "string"),
                         new Param(c, "host", "string"),
                         new Param(c, "port", "int", Value(0)), End),

            new Function(c, "remoteDisconnect", remoteDisconnect, None, Return,
                         "void", Parameters,
                         new Param(c, "remoteContact", "string"), End),

            new Function(c, "remoteNetwork", remoteNetwork, None, Return,
                         "void", Parameters, new Param(c, "on", "bool"), End),

            new SymbolicConstant(c, "NetworkStatusOn", "int", Value(1)),
            new SymbolicConstant(c, "NetworkStatusOff", "int", Value(0)),

            new Function(c, "remoteNetworkStatus", remoteNetworkStatus, None,
                         Return, "int", End),

            new Function(c, "writeSetting", writeSetting, None, Return, "void",
                         Parameters, new Param(c, "group", "string"),
                         new Param(c, "name", "string"),
                         new Param(c, "value", "SettingsValue"), End),

            new Function(c, "readSetting", readSetting, None, Return,
                         "SettingsValue", Parameters,
                         new Param(c, "group", "string"),
                         new Param(c, "name", "string"),
                         new Param(c, "defaultValue", "SettingsValue"), End),

            new Function(
                c, "httpGet", httpGet, None, Return, "void", Parameters,
                new Param(c, "url", "string"),
                new Param(c, "headers", "[(string,string)]"),
                new Param(c, "replyEvent", "string"),
                new Param(c, "authenticationEvent", "string",
                          Value(Pointer(0))),
                new Param(c, "progressEvent", "string", Value(Pointer(0))),
                new Param(c, "ignoreSslErrors", "bool", Value(false)), End),

            new Function(
                c, "httpPost", httpPost, None, Return, "void", Parameters,
                new Param(c, "url", "string"),
                new Param(c, "headers", "[(string,string)]"),
                new Param(c, "postString", "string"),
                new Param(c, "replyEvent", "string"),
                new Param(c, "authenticationEvent", "string",
                          Value(Pointer(0))),
                new Param(c, "progressEvent", "string", Value(Pointer(0))),
                new Param(c, "ignoreSslErrors", "bool", Value(false)), End),

            new Function(c, "mainWindowWidget", mainWindowWidget, None, Return,
                         "qt.QMainWindow", End),

            new Function(c, "networkAccessManager", networkAccessManager, None,
                         Return, "qt.QNetworkAccessManager", End),

            new Function(c, "javascriptMuExport", javascriptMuExport, None,
                         Return, "void", Parameters,
                         new Param(c, "frame", "qt.QWebFrame"), End),

            new Function(c, "sessionFromUrl", sessionFromUrl, None, Return,
                         "void", Parameters, new Param(c, "url", "string"),
                         End),

            new Function(c, "putUrlOnClipboard", putUrlOnClipboard, None,
                         Return, "void", Parameters,
                         new Param(c, "url", "string"),
                         new Param(c, "title", "string"),
                         new Param(c, "doEncode", "bool", Value(true)), End),

            new Function(c, "myNetworkPort", myNetworkPort, None, Return, "int",
                         End),

            new Function(c, "myNetworkHost", myNetworkHost, None, Return,
                         "string", End),

            new Function(c, "encodePassword", encodePassword, None, Return,
                         "string", Parameters,
                         new Param(c, "password", "string"), End),

            new Function(c, "decodePassword", decodePassword, None, Return,
                         "string", Parameters,
                         new Param(c, "password", "string"), End),

            new Function(c, "cacheDir", cacheDir, None, Return, "string", End),

            new Function(c, "openUrl", openUrl, None, Return, "void",
                         Parameters, new Param(c, "url", "string"), End),

            EndArguments);
    }

    NODE_IMPLEMENTATION(resizeFit, void) {}

    NODE_IMPLEMENTATION(setViewSize, void) {}

    NODE_IMPLEMENTATION(popupMenu, void) {}

    NODE_IMPLEMENTATION(popupMenuAtPoint, void) {}

    NODE_IMPLEMENTATION(setWindowTitle, void) {}

    NODE_IMPLEMENTATION(center, void) {}

    NODE_IMPLEMENTATION(close, void) {}

    NODE_IMPLEMENTATION(toggleMenuBar, void) {}

    NODE_IMPLEMENTATION(isMenuBarVisible, bool) { NODE_RETURN(false); }

    NODE_IMPLEMENTATION(openMediaFileDialog, Pointer)
    {
        NODE_RETURN((Pointer)0);
    }

    NODE_IMPLEMENTATION(openFileDialog, Pointer) { NODE_RETURN((Pointer)0); }

    NODE_IMPLEMENTATION(saveFileDialog, Pointer) { NODE_RETURN((Pointer)0); }

    NODE_IMPLEMENTATION(setCursor, void) {}

    NODE_IMPLEMENTATION(alertPanel, int) { NODE_RETURN(0); }

    NODE_IMPLEMENTATION(stereoSupported, bool) { NODE_RETURN(false); }

    NODE_IMPLEMENTATION(watchFile, void) {}

    NODE_IMPLEMENTATION(showConsole, void) {}

    NODE_IMPLEMENTATION(isConsoleVisible, bool) { NODE_RETURN(false); }

    NODE_IMPLEMENTATION(remoteSendMessage, void) {}

    NODE_IMPLEMENTATION(remoteSendEvent, void) {}

    NODE_IMPLEMENTATION(remoteConnections, Pointer) { NODE_RETURN(0); }

    NODE_IMPLEMENTATION(remoteApplications, Pointer) { NODE_RETURN(0); }

    NODE_IMPLEMENTATION(remoteContacts, Pointer) { NODE_RETURN(0); }

    NODE_IMPLEMENTATION(remoteLocalContactName, Pointer) { NODE_RETURN(0); }

    NODE_IMPLEMENTATION(remoteConnect, void) {}

    NODE_IMPLEMENTATION(remoteDisconnect, void) {}

    NODE_IMPLEMENTATION(remoteNetwork, void) {}

    NODE_IMPLEMENTATION(remoteNetworkStatus, int) { NODE_RETURN(0); }

    NODE_IMPLEMENTATION(writeSetting, void) {}

    NODE_IMPLEMENTATION(readSetting, Pointer) { NODE_RETURN(0); }

    NODE_DECLARATION(httpGet, void) {}

    NODE_DECLARATION(httpPost, void) {}

    NODE_DECLARATION(sessionFromUrl, void) {}

    NODE_DECLARATION(putUrlOnClipboard, void) {}

    NODE_DECLARATION(myNetworkPort, int) { NODE_RETURN(0); }

    NODE_DECLARATION(encodePassword, Mu::Pointer) { NODE_RETURN(0); }

    NODE_DECLARATION(decodePassword, Mu::Pointer) { NODE_RETURN(0); }

    NODE_DECLARATION(cacheDir, Mu::Pointer) { NODE_RETURN(0); }

    NODE_IMPLEMENTATION(openUrl, void) {}

    NODE_DECLARATION(mainWindowWidget, Mu::Pointer) { NODE_RETURN(0); }

    NODE_DECLARATION(networkAccessManager, Mu::Pointer) { NODE_RETURN(0); }

    NODE_DECLARATION(javascriptMuExport, void) {}

    NODE_IMPLEMENTATION(myNetworkHost, Pointer) { NODE_RETURN(0); }

} //  End namespace RVIO
