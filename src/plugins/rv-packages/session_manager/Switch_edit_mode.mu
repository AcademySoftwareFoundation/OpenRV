//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

module: Switch_edit_mode 
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

class: SwitchEditMode : MinorMode
{
    QWidget _ui;
    QCheckBox _alignCheckBox;
    QCheckBox _useCutInfoCheckBox;
    QCheckBox _autoSizeCheckBox;
    QComboBox _selectedInputCombo;
    QLineEdit _outputWidthEdit;
    QLineEdit _outputHeightEdit;
    bool      _uiInFlux;
    
    
    method: auxFilePath (string; string name)
    {
        io.path.join(supportPath("session_manager", "session_manager"), name);
    }

    method: updateUI (void;)
    {
        let vnode = viewNode(),
            vnodeExists = vnode neq nil;

        if (_ui eq nil || !vnodeExists) return;

        _uiInFlux = true;

        try
        {
            let a     = getIntProperty("#RVSwitch.mode.alignStartFrames").front(),
                u     = getIntProperty("#RVSwitch.mode.useCutInfo").front(),
                c     = getStringProperty("#RVSwitch.output.input").front(),
                asize = getIntProperty("#RVSwitch.output.autoSize").front(),
                size  = getIntProperty("#RVSwitch.output.size");

            _alignCheckBox.setCheckState(if a == 0 then Qt.Unchecked else Qt.Checked);
            _useCutInfoCheckBox.setCheckState(if u == 0 then Qt.Unchecked else Qt.Checked);
            _autoSizeCheckBox.setCheckState(if asize == 0 then Qt.Unchecked else Qt.Checked);

            _selectedInputCombo.clear();

            int selectedIndex = 0;
            let inputs = nodeConnections(viewNode(), false)._0;

            for_index (i; inputs)
            {
                _selectedInputCombo.addItem(uiName(inputs[i]), QVariant(inputs[i]));
                if (inputs[i] == c) selectedIndex = i;
            }

            _selectedInputCombo.setCurrentIndex(selectedIndex);

            _outputWidthEdit.setEnabled(asize == 0);
            _outputHeightEdit.setEnabled(asize == 0);

            _outputWidthEdit.setText("%d" % size.front());
            _outputHeightEdit.setText("%d" % size.back());
        }
        catch (...)
        {
            ;
        }

        redraw();
        _uiInFlux = false;
    }

    method: updateUIEvent (void; Event event)
    {
        event.reject();
        updateUI();
    }

    method: propertyChanged (void; Event event)
    {
        let prop  = event.contents(),
            parts = prop.split("."),
            node  = parts[0],
            comp  = parts[1],
            name  = parts[2];

        if (comp == "mode" || comp == "output")
        {
            if (name == "alignStartFrames" ||
                name == "useCutInfo" ||
                name == "input" ||
                name == "size" ||
                name == "autoSize")
            {
                if (_ui neq nil) updateUI();
            }
        }

        event.reject();
    }

    method: checkBoxSlot (void; int state, string name)
    {
        set(name, if state == Qt.Checked then 1 else 0);
    }

    method: updateMenu (void;)
    {
        setMenu(menu());
    }

    method: setSelectedInput (void; int index)
    {
        if (_uiInFlux) return;

        string currentName = getStringProperty("#RVSwitch.output.input").front();
        string name;
        
        if (index >= 0 && index < _selectedInputCombo.count())
        {
            name = _selectedInputCombo.itemData(index, Qt.UserRole).toString();
        }

        if (name != currentName)
        {
            set("#RVSwitch.output.input", name);
            redraw();
        }
    }

    method: widthChanged (void;)
    {
        let val = float(_outputWidthEdit.text()),
            prop = getIntProperty("#RVSwitch.output.size");

        setIntProperty("#RVSwitch.output.size", int[] {val, prop.back()});
        redraw();
    }

    method: heightChanged (void;)
    {
        let val = float(_outputHeightEdit.text()),
            prop = getIntProperty("#RVSwitch.output.size");

        setIntProperty("#RVSwitch.output.size", int[] {prop.front(), val});
        redraw();
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
                _ui                      = loadUIFile(manager.auxFilePath("switch.ui"), m);
                _alignCheckBox           = _ui.findChild("alignCheckBox");
                _useCutInfoCheckBox      = _ui.findChild("useCutInfoCheckBox");
                _autoSizeCheckBox        = _ui.findChild("autoSizeCheckBox");
                _selectedInputCombo      = _ui.findChild("selectedInputCombo");
                _outputWidthEdit         = _ui.findChild("outputWidthEdit");
                _outputHeightEdit        = _ui.findChild("outputHeightEdit");

                manager.addEditor("Switch", _ui);

                connect(_alignCheckBox, 
                        QCheckBox.stateChanged,
                        checkBoxSlot(,"#RVSwitch.mode.alignStartFrames"));

                connect(_useCutInfoCheckBox, 
                        QCheckBox.stateChanged, 
                        checkBoxSlot(,"#RVSwitch.mode.useCutInfo"));

                connect(_autoSizeCheckBox, 
                        QCheckBox.stateChanged, 
                        checkBoxSlot(,"#RVSwitch.output.autoSize"));

                connect(_selectedInputCombo, QComboBox.currentIndexChanged, setSelectedInput);
                connect(_outputWidthEdit, QLineEdit.editingFinished, widthChanged);
                connect(_outputHeightEdit, QLineEdit.editingFinished, heightChanged);
            }

            updateUI();
            manager.useEditor("Switch");
        }

        event.reject();
    }

    method: alignStartFrames (void; Event event) 
    { 
        let p = "#RVSwitch.mode.alignStartFrames",
            a = getIntProperty(p).front();
        set(p, if a != 0 then 0 else 1);
    }

    method: useCutInfo (void; Event event) 
    { 
        let p = "#RVSwitch.mode.useCutInfo",
            a = getIntProperty(p).front();
        set(p, if a != 0 then 0 else 1);
    }

    method: stateFunc ((int;); string name)
    {
        \: (int;)
        {
            let p = getIntProperty("#RVSwitch.mode.%s" % name).front();
            if p == 0 then UncheckedMenuState else CheckedMenuState;
        };
    }

    method: retimeState (int;)
    {
        let p = getIntProperty("#View.timing.retimeInputs").front();
        if p == 0 then UncheckedMenuState else CheckedMenuState;
    }

    method: menu (Menu;)
    {
        Menu {
            {"Switch", Menu {
                {"Align Start Frames", alignStartFrames, nil, stateFunc("alignStartFrames")},
                {"Use Source Cut Info", useCutInfo, nil, stateFunc("useCutInfo")}
            }
        }};
    }

    method: activate (void;)
    {
        //setMenu(menu());
        ;
    }

    method: SwitchEditMode (SwitchEditMode; string name)
    {
        _uiInFlux = false;

        init(name,  // this is init from session_manager (its new style)
             nil,
             [("session-manager-load-ui", loadUI, "Load UI into Session Manager"),
              ("range-changed", updateUIEvent, "Update UI"),
              ("image-structure-change", updateUIEvent, "Update UI"),
              ("graph-state-change", propertyChanged,  "Maybe update session UI")],
             menu(),
             "z0");
    }
}

\: createMode (Mode;)
{
    return SwitchEditMode("Switch_edit_mode");
}

}
