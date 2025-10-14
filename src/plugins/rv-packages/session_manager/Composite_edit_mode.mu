//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

module: Composite_edit_mode 
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

class: CompositeEditMode : MinorMode
{
    QWidget _ui;
    QComboBox _comboBox;
    
    method: auxFilePath (string; string name)
    {
        io.path.join(supportPath("session_manager", "session_manager"), name);
    }

    method: setOp (void; int index)
    {
        string name = "over";

        case (index)
        {
            0 -> { name = "over"; }
            1 -> { name = "add"; }
            //  2 -> { name = "dissolve"; }
            2 -> { name = "difference"; }
            3 -> { name = "-difference"; }
            4 -> { name = "replace"; }
            5 -> { name = "topmost"; }
        }

        set("#RVStack.composite.type", name);
        redraw();
    }

    method: setOpEvent (void; Event event, int index)
    {
        setOp(index);
    }

    method: updateUI (void;)
    {
        if (_ui eq nil) return;

        int index = 0;
        
        case (getStringProperty("#RVStack.composite.type").front())
        {
            "over"        -> { index = 0; }
            "add"         -> { index = 1; }
            //  "dissolve"    -> { index = 2; }
            "difference"  -> { index = 2; }
            "-difference" -> { index = 3; }
            "replace"     -> { index = 4; }
            "topmost"     -> { index = 5; }
            _             -> { index = 6; }
        }

        _comboBox.setCurrentIndex(index);
    }

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

        if (comp == "composite")
        {
            if (name == "type") updateUI();
        }

        event.reject();
    }

    method: loadUI (void; Event event)
    {
        State state = data();

        if (state.sessionManager neq nil)
        {
            SessionManagerMode manager = state.sessionManager;
            let m = mainWindowWidget();

            if (_ui eq nil)
            {
                _ui = loadUIFile(manager.auxFilePath("composite.ui"), m);
                _comboBox = _ui.findChild("comboBox");
                manager.addEditor("Composite Function", _ui);
                connect(_comboBox, QComboBox.currentIndexChanged, setOp);
            }

            updateUI();
            manager.useEditor("Composite Function");
        }

        event.reject();
    }

    method: opState ((int;); string n)
    {
        \: (int;)
        {
            if (filterLiveReviewEvents()) {
                return DisabledMenuState;
            }

            let op = getStringProperty("#RVStack.composite.type").front();
            if op == n then CheckedMenuState else UncheckedMenuState;
        };
    }

    method: CompositeEditMode (CompositeEditMode; string name)
    {
        init(name,  // this is init from session_manager (its new style)
             nil,
             [("session-manager-load-ui", loadUI, "Load UI into Session Manager"),
              ("graph-state-change", propertyChanged,  "Maybe update session UI")],
             newMenu(MenuItem[] {
                 subMenu("Stack", MenuItem[] {
                     menuText("Composite Operation"),
                     menuItem("   Over", "", "viewmode_category", setOpEvent(,0), opState("over")),
                     menuItem("   Add", "", "viewmode_category", setOpEvent(,1), opState("add")),
                     menuItem("   Dissolve", "", "viewmode_category", setOpEvent(,2), opState("dissolve")),
                     menuItem("   Difference", "", "viewmode_category", setOpEvent(,2), opState("difference")),
                     menuItem("   Inverted Difference", "", "viewmode_category", setOpEvent(,3), opState("-difference")),
                     menuItem("   Replace", "", "viewmode_category", setOpEvent(,4), opState("replace")),
                     menuItem("   Topmost", "", "viewmode_category", setOpEvent(,5), opState("topmost")),
                     menuSeparator(),
                     menuItem("Cycle Forward", "", "viewmode_category", cycleStackForward, isStackMode),
                     menuItem("Cycle Backward", "", "viewmode_category", cycleStackBackward, isStackMode)
                 })
             }),
             "b");
    }
}

\: createMode (Mode;)
{
    return CompositeEditMode("Composite_edit_mode");
}
}
