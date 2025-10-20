//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

module: StackGroup_edit_mode 
{
use rvtypes;
use commands;
use rvui;
use qt;
use session_manager;
use extra_commands;
use gl;
use glyph;
use app_utils;
use io;
use system;
use app_utils;

class: StackGroupEditMode : MinorMode
{
    method: auxFilePath (string; string name)
    {
        io.path.join(supportPath("session_manager", "session_manager"), name);
    }

    method: activateUI (void; bool on)
    {
        State state = data();
        mode_manager.ModeManagerMode mm = state.modeManager;

        for_each (mode; ["Composite_edit_mode",
                         "Stack_edit_mode"])
        {
            mm.activateEntry(mm.findModeEntry(mode), on);
        }

        let p = viewNode() + ".ui.wipes",
            wipe = state.wipe;

        //
        //  Note on toggleWipe vs wipe.toggle.  toggleWipe resets
        //  the wipes and turns them off (sets the ui.wipes flag to
        //  0).  wipe.toggle just makes the mode inactive, so that
        //  the wipes will be in the same state when you return to
        //  this view.
        //
        if (on)
        {
            if (propertyExists(p))
            { 
                let wipeon = getIntProperty(p).front() == 1;

                if (wipeon)
                {
                    if (wipe eq nil || !wipe._active) toggleWipe();
                }
                else
                {
                    if (wipe neq nil && wipe._active) wipe.toggle();
                }
            }
            else
            {
                if (wipe neq nil && wipe._active) wipe.toggle();
            }
        }
        else if (wipe neq nil && wipe._active)
        {
            wipe.toggle();
        }
    }

    method: activate (void;) { activateUI(true); }
    method: deactivate (void;) { activateUI(false); }

    method: propertyChanged (void; Event event)
    {
        let prop  = event.contents(),
            parts = prop.split("."),
            node  = parts[0],
            comp  = parts[1],
            name  = parts[2];

        //
        //  If a UI name changes we need to update the tree 
        //

        if ((comp == "ui" && name == "wipes") ||
            (comp == "timing" && name == "retimeToOutput"))
        {
            activateUI(true);
            redraw();
        }

        event.reject();
    }

    method: StackGroupEditMode (StackGroupEditMode; string name)
    {
        init(name,  // this is init from session_manager (its new style)
             nil,
             [("graph-state-change", propertyChanged,  "Maybe update session UI")],
             newMenu(MenuItem[] {
                 subMenu("Stack", MenuItem[] {
                     // Empty submenu - no items
                 })
             }),
             nil);
    }
}

\: createMode (Mode;)
{
    return StackGroupEditMode("StackGroup_edit_mode");
}
}
