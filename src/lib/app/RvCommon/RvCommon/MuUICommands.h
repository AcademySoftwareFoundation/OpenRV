//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __RV__MuUICommands__h__
#define __RV__MuUICommands__h__

#include <Mu/Mu.h>
#include <MuLang/MuLang.h>
#include <MuTwkApp/MuInterface.h>

namespace Rv
{

    void initUICommands();

    NODE_DECLARATION(resizeFit, void);
    NODE_DECLARATION(setViewSize, void);
    NODE_DECLARATION(startupResize, bool);
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
    NODE_DECLARATION(showNetworkDialog, void);
    NODE_DECLARATION(showConsole, void);
    NODE_DECLARATION(isConsoleVisible, bool);
    NODE_DECLARATION(remoteSendMessage, void);
    NODE_DECLARATION(remoteSendEvent, void);
    NODE_DECLARATION(remoteSendDataEvent, void);
    NODE_DECLARATION(remoteConnections, Mu::Pointer);
    NODE_DECLARATION(remoteApplications, Mu::Pointer);
    NODE_DECLARATION(remoteContacts, Mu::Pointer);
    NODE_DECLARATION(remoteLocalContactName, Mu::Pointer);
    NODE_DECLARATION(setRemoteLocalContactName, void);
    NODE_DECLARATION(remoteConnect, void);
    NODE_DECLARATION(remoteDisconnect, void);
    NODE_DECLARATION(remoteNetwork, void);
    NODE_DECLARATION(remoteNetworkStatus, int);
    NODE_DECLARATION(remoteDefaultPermission, int);
    NODE_DECLARATION(setRemoteDefaultPermission, void);
    NODE_DECLARATION(remoteConnectionIsIncoming, bool);
    NODE_DECLARATION(spoofConnectionStream, void);
    NODE_DECLARATION(writeSetting, void);
    NODE_DECLARATION(readSetting, Mu::Pointer);
    NODE_DECLARATION(httpGet, void);
    NODE_DECLARATION(httpPostString, void);
    NODE_DECLARATION(httpPostData, void);
    NODE_DECLARATION(httpPutString, void);
    NODE_DECLARATION(httpPutData, void);
    NODE_DECLARATION(sessionFromUrl, void);
    NODE_DECLARATION(putUrlOnClipboard, void);
    NODE_DECLARATION(myNetworkPort, int);
    NODE_DECLARATION(myNetworkHost, Mu::Pointer);
    NODE_DECLARATION(encodePassword, Mu::Pointer);
    NODE_DECLARATION(decodePassword, Mu::Pointer);
    NODE_DECLARATION(cacheDir, Mu::Pointer);
    NODE_DECLARATION(openUrl, void);
    NODE_DECLARATION(openUrlFromUrl, void);
    NODE_DECLARATION(mainWindowWidget, Mu::Pointer);
    NODE_DECLARATION(mainViewWidget, Mu::Pointer);
    NODE_DECLARATION(prefTabWidget, Mu::Pointer);
    NODE_DECLARATION(sessionBottomToolBar, Mu::Pointer);
    NODE_DECLARATION(networkAccessManager, Mu::Pointer);
    NODE_DECLARATION(queryDriverAttribute, Mu::Pointer);
    NODE_DECLARATION(setDriverAttribute, void);
    NODE_DECLARATION(setPresentationMode, void);
    NODE_DECLARATION(presentationMode, bool);
    NODE_DECLARATION(packageListFromSetting, Mu::Pointer);
    NODE_DECLARATION(showTopViewToolbar, void);
    NODE_DECLARATION(showBottomViewToolbar, void);
    NODE_DECLARATION(isTopViewToolbarVisible, bool);
    NODE_DECLARATION(isBottomViewToolbarVisible, bool);
    NODE_DECLARATION(editNodeSource, void);
    NODE_DECLARATION(editProfiles, void);
    NODE_DECLARATION(validateShotgunToken, Mu::Pointer);
    NODE_DECLARATION(launchTLI, void);
    NODE_DECLARATION(rvioSetup, void);
    NODE_DECLARATION(javascriptMuExport, void);

} // namespace Rv

#endif // __RV__MuUICommands__h__
