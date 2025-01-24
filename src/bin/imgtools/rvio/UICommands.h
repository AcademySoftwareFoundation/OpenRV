//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __RVIO__UICommands__h__
#define __RVIO__UICommands__h__

#include <Mu/Mu.h>
#include <MuLang/MuLang.h>
#include <MuTwkApp/MuInterface.h>

namespace RVIO
{

    void initUICommands(Mu::MuLangContext*);

    NODE_DECLARATION(resizeFit, void);
    NODE_DECLARATION(setViewSize, void);
    NODE_DECLARATION(popupMenu, void);
    NODE_DECLARATION(popupMenuAtPoint, void);
    NODE_DECLARATION(setWindowTitle, void);
    NODE_DECLARATION(center, void);
    NODE_DECLARATION(close, void);
    NODE_DECLARATION(toggleMenuBar, void);
    NODE_DECLARATION(isMenuBarVisible, bool);
    NODE_DECLARATION(openMediaFileDialog, Mu::Pointer);
    NODE_DECLARATION(openFileDialog, Mu::Pointer);
    NODE_DECLARATION(saveFileDialog, Mu::Pointer);
    NODE_DECLARATION(setCursor, void);
    NODE_DECLARATION(alertPanel, int);
    NODE_DECLARATION(stereoSupported, bool);
    NODE_DECLARATION(watchFile, void);
    NODE_DECLARATION(showConsole, void);
    NODE_DECLARATION(isConsoleVisible, bool);
    NODE_DECLARATION(remoteSendMessage, void);
    NODE_DECLARATION(remoteSendEvent, void);
    NODE_DECLARATION(remoteApplications, Mu::Pointer);
    NODE_DECLARATION(remoteConnections, Mu::Pointer);
    NODE_DECLARATION(remoteContacts, Mu::Pointer);
    NODE_DECLARATION(remoteLocalContactName, Mu::Pointer);
    NODE_DECLARATION(remoteConnect, void);
    NODE_DECLARATION(remoteDisconnect, void);
    NODE_DECLARATION(remoteNetwork, void);
    NODE_DECLARATION(remoteNetworkStatus, int);
    NODE_DECLARATION(writeSetting, void);
    NODE_DECLARATION(readSetting, Mu::Pointer);
    NODE_DECLARATION(httpGet, void);
    NODE_DECLARATION(httpPost, void);
    NODE_DECLARATION(sessionFromUrl, void);
    NODE_DECLARATION(putUrlOnClipboard, void);
    NODE_DECLARATION(myNetworkPort, int);
    NODE_DECLARATION(encodePassword, Mu::Pointer);
    NODE_DECLARATION(decodePassword, Mu::Pointer);
    NODE_DECLARATION(cacheDir, Mu::Pointer);
    NODE_DECLARATION(mainWindowWidget, Mu::Pointer);
    NODE_DECLARATION(networkAccessManager, Mu::Pointer);
    NODE_DECLARATION(javascriptMuExport, void);
    NODE_DECLARATION(openUrl, void);
    NODE_DECLARATION(myNetworkHost, Mu::Pointer);

} // namespace RVIO

#endif // __RV__UICommands__h__
