//*****************************************************************************/
// Copyright (c) 2021 Autodesk, Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//*****************************************************************************/

//
//  Implements the legacy RV Sync related menu options that are scattered 
//  throughout various RV menus.
//  Not loading the RV Sync package will automatically hide these RV Sync 
//  related menu options.
//

module: sync_mode {

require commands;
require extra_commands;
require rvtypes;
require rvui;
use app_utils;

class: SyncMode : rvtypes.MinorMode
{ 
    method: SyncMode(SyncMode; string name)
    {
        init(name,
            nil,
            nil,
            rvtypes.Menu {
                {"Edit",
                    rvtypes.Menu {
                        {"Copy Sync Session URL", copySessionUrl, nil, nil},
                    }
                },
                {"Tools",
                    rvtypes.Menu {
                        {"Sync With Connected RVs", ~extra_commands.toggleSync, nil, syncWithConnectedRVsState},
                    }
                },
            }
        );
    }

    method: copySessionUrl (void; Event ev)
    {
        string url = "rvlink:// -network -networkConnect %s %d -flags syncPullFirst" %
                 (commands.myNetworkHost(), commands.myNetworkPort());
        
        string simpleTitle = "RV Sync Session with %s" % commands.myNetworkHost();
        commands.putUrlOnClipboard (url, simpleTitle);
    }

    method: syncWithConnectedRVsState (int;)
    {
        rvtypes.State state = commands.data();

        if (commands.remoteNetworkStatus() == commands.NetworkStatusOn && !commands.remoteConnections().empty())
        {
            return if state.sync neq nil && state.sync.isActive() 
                then commands.CheckedMenuState
                else commands.UncheckedMenuState;
        }
        else
        {
            return commands.DisabledMenuState;
        }
    }
}

\: createMode (rvtypes.Mode;)
{
    return SyncMode("sync-mode");
}

\: theMode (SyncMode; )
{
    SyncMode m = rvui.minorModeFromName("sync-mode");

    if (m eq nil)
    {
        print("WARNING: RV Sync module is not currently loaded.\n" + 
              "Please make sure the RV Sync Package is enabled in the RV Preferences/Packages\n");
    }

    return m;
}
}
