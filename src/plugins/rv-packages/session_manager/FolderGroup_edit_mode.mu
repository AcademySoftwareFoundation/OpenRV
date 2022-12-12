//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

module: FolderGroup_edit_mode 
{
use rvtypes;
use commands;
use rvui;
use session_manager;
use extra_commands;
use app_utils;
use qt;

class: FolderGroupEditMode : MinorMode
{
    QWidget _ui;
    QComboBox _viewTypeCombo;

    method: activateUI (void; bool on)
    {
        State state = data();
        mode_manager.ModeManagerMode mm = state.modeManager;
        let currentType = getStringProperty("#RVFolderGroup.mode.viewType").front();

        [string] modes;

        case (currentType)
        {
            "switch" -> { modes = ["Switch_edit_mode"]; }
            "layout" -> { modes = ["LayoutGroup_edit_mode"]; }
            "stack" -> { modes = ["StackGroup_edit_mode"]; }
            _ -> { modes = ["LayoutGroup_edit_mode"]; }
        }

        for_each (mode; modes)
        {
            mm.activateEntry(mm.findModeEntry(mode), on);
        }
    }

    method: setViewType (void; int index)
    {
        string currentType = getStringProperty("#RVFolderGroup.mode.viewType").front();
        let newtype = _viewTypeCombo.itemData(index, Qt.UserRole).toString();

        if (newtype != currentType)
        {
            activateUI(false);
            set("#RVFolderGroup.mode.viewType", newtype);
            redraw();
            activateUI(true);

            State state = data();

            if (state.sessionManager neq nil)
            {
                SessionManagerMode manager = state.sessionManager;
                manager.reloadEditorTab();
            }
        }
    }

    method: updateUI (void;)
    {
        let vnode = viewNode(),
            vnodeExists = vnode neq nil;

        if (_ui eq nil || !vnodeExists) return;

        try
        {
            let vtype = getStringProperty("#RVFolderGroup.mode.viewType").front();
            int index = 0;
            
            case (vtype)
            {
                "switch" -> { index = 0; }
                "layout" -> { index = 1; }
                "stack"  -> { index = 2; }
                _        -> { index = 1; }
            }

            _viewTypeCombo.setCurrentIndex(index);
        }
        catch (...)
        {
            ; // nothing
        }
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
                _ui            = loadUIFile(manager.auxFilePath("folder.ui"), m);
                _viewTypeCombo = _ui.findChild("viewTypeCombo");

                _viewTypeCombo.clear();
                _viewTypeCombo.addItem("Switch", QVariant("switch"));
                _viewTypeCombo.addItem("Layout", QVariant("layout"));
                _viewTypeCombo.addItem("Stack", QVariant("stack"));

                connect(_viewTypeCombo, QComboBox.currentIndexChanged, setViewType);
                manager.addEditor("Folder View", _ui);
            }

            updateUI();
            manager.useEditor("Folder View");
        }

        event.reject();
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

        if (comp == "mode" && name == "viewType")
        {
            updateUI();
        }

        event.reject();
    }

    method: FolderGroupEditMode (FolderGroupEditMode; string name)
    {
        init(name,  // this is init from session_manager (its new style)
             nil,
             [("session-manager-load-ui", loadUI, "Load UI into Session Manager"),
              ("graph-state-change", propertyChanged,  "Maybe update session UI")],
             nil,
             nil);
    }
}

\: createMode (Mode;)
{
    return FolderGroupEditMode("FolderGroup_edit_mode");
}
}
