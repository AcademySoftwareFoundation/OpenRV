//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

module: SwitchGroup_edit_mode 
{
use rvtypes;
use commands;
use rvui;
use session_manager;
use extra_commands;
use app_utils;

class: SwitchGroupEditMode : MinorMode
{
    method: activateUI (void; bool on)
    {
        State state = data();
        mode_manager.ModeManagerMode mm = state.modeManager;

        for_each (mode; ["Switch_edit_mode"])
        {
            mm.activateEntry(mm.findModeEntry(mode), on);
        }
    }

    method: activate (void;) { activateUI(true); }
    method: deactivate (void;) { activateUI(false); }

    method: SwitchGroupEditMode (SwitchGroupEditMode; string name)
    {
        init(name,  // this is init from session_manager (its new style)
             nil,
             nil,
             nil,
             nil);
    }
}

\: createMode (Mode;)
{
    return SwitchGroupEditMode("SwitchGroup_edit_mode");
}
}
