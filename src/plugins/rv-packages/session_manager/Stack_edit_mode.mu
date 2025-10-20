//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

module: Stack_edit_mode 
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

class: StackEditMode : MinorMode
{
    QWidget _ui;
    QCheckBox _alignCheckBox;
    QCheckBox _strictRangesCheckBox;
    QCheckBox _useCutInfoCheckBox;
    QCheckBox _retimeCheckBox;
    QCheckBox _autoSizeCheckBox;
    QCheckBox _interactiveSizeCheckBox;
    QComboBox _chosenAudioInputCombo;
    QLineEdit _outputFPSEdit;
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
            let a     = getIntProperty("#RVStack.mode.alignStartFrames").front(),
                st    = getIntProperty("#RVStack.mode.strictFrameRanges").front(),
                u     = getIntProperty("#RVStack.mode.useCutInfo").front(),
                c     = getStringProperty("#RVStack.output.chosenAudioInput").front(),
                asize = getIntProperty("#RVStack.output.autoSize").front(),
                size  = getIntProperty("#RVStack.output.size"),
                fps   = getFloatProperty("#RVStack.output.fps").front(),
                isize = getIntProperty("#RVStack.output.interactiveSize").front();

            _alignCheckBox.setCheckState(if a == 0 then Qt.Unchecked else Qt.Checked);
            _strictRangesCheckBox.setCheckState(if st == 0 then Qt.Unchecked else Qt.Checked);
            _useCutInfoCheckBox.setCheckState(if u == 0 then Qt.Unchecked else Qt.Checked);
            _autoSizeCheckBox.setCheckState(if asize == 0 then Qt.Unchecked else Qt.Checked);
            _interactiveSizeCheckBox.setCheckState(if isize == 0 then Qt.Unchecked else Qt.Checked);

            _chosenAudioInputCombo.clear();
            _chosenAudioInputCombo.addItem("All Inputs Mixed", QVariant(".all."));
            _chosenAudioInputCombo.addItem("First Input Only", QVariant(".first."));
            _chosenAudioInputCombo.addItem("First Visible Input", QVariant(".topmost."));

            let chosenIndex = 0,
                inputs = nodeConnections(viewNode(), false)._0;

            if (c == ".first.") chosenIndex = 1;
            if (c == ".topmost.") chosenIndex = 2;
            for_index (i; inputs)
            {
                _chosenAudioInputCombo.addItem(uiName(inputs[i]), QVariant(inputs[i]));
                //
                //  i+3 because we used the first three slots for "play
                //  everything" and "play first only" and "play first visible"
                //
                if (inputs[i] == c) chosenIndex = i+3;
            }
            _chosenAudioInputCombo.setCurrentIndex(chosenIndex);


            _outputWidthEdit.setEnabled(asize == 0);
            _outputHeightEdit.setEnabled(asize == 0);

            _outputFPSEdit.setText("%g" % fps);
            _outputWidthEdit.setText("%d" % size.front());
            _outputHeightEdit.setText("%d" % size.back());
        
            if (vnodeExists)
            {
                let retimeProp = "#View.timing.retimeInputs";

                if (propertyExists(retimeProp))
                {
                    _retimeCheckBox.setCheckState(if getIntProperty(retimeProp).front() == 1 
                                                  then Qt.Checked 
                                                      else Qt.Unchecked);
                }
                else
                {
                    //print("no prop\n");
                    ;
                }
            }
            else
            {
                print("no vnode\n");
            }
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
                name == "strictFrameRanges" ||
                name == "useCutInfo" ||
                name == "chosenAudioInput" ||
                name == "size" ||
                name == "autoSize" ||
                name == "fps" ||
                name == "interactiveSize")
            {
                if (_ui neq nil) updateUI();
            }
        }

        event.reject();
    }

    method: checkBoxSlot (void; int state, string name)
    {
	let v    = getIntProperty (name)[0],
	    newV = (if state == Qt.Checked then 1 else 0);

        if (v != newV) set(name, newV);
    }

    method: updateMenu (void;)
    {
        setMenu(menu());
    }

    method: setChosenAudioInput (void; int index)
    {
        if (_uiInFlux) return;

        string currentName = getStringProperty("#RVStack.output.chosenAudioInput").front(),
            name = ".all.";

        if (index >= 0 && index < _chosenAudioInputCombo.count())
        {
            name = _chosenAudioInputCombo.itemData(index, Qt.UserRole).toString();
        }

        if (name != currentName)
        {
            set("#RVStack.output.chosenAudioInput", name);
            redraw();
        }
    }

    method: fpsChanged (void;)
    {
        let newFPS = float(_outputFPSEdit.text());

        try
        {
            set("#RVStack.output.fps", newFPS);
            setFPS(newFPS);
        }
        catch (...)
        {
            ; //nothing
        }

        redraw();
    }

    method: widthChanged (void;)
    {
        let val = float(_outputWidthEdit.text()),
            prop = getIntProperty("#RVStack.output.size");

        setIntProperty("#RVStack.output.size", int[] {val, prop.back()});
        redraw();
    }

    method: heightChanged (void;)
    {
        let val = float(_outputHeightEdit.text()),
            prop = getIntProperty("#RVStack.output.size");

        setIntProperty("#RVStack.output.size", int[] {prop.front(), val});
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
                _ui                      = loadUIFile(manager.auxFilePath("stack.ui"), m);
                _alignCheckBox           = _ui.findChild("alignCheckBox");
                _strictRangesCheckBox    = _ui.findChild("strictRangesCheckBox");
                _useCutInfoCheckBox      = _ui.findChild("useCutInfoCheckBox");
                _retimeCheckBox          = _ui.findChild("retimeInputsCheckBox");
                _autoSizeCheckBox        = _ui.findChild("autoSizeCheckBox");
                _chosenAudioInputCombo   = _ui.findChild("chosenAudioInputCombo");
                _outputFPSEdit           = _ui.findChild("outputFPSEdit");
                _outputWidthEdit         = _ui.findChild("outputWidthEdit");
                _outputHeightEdit        = _ui.findChild("outputHeightEdit");
                _interactiveSizeCheckBox = _ui.findChild("interactiveResizeCheckBox");

                manager.addEditor("Stack", _ui);

                connect(_alignCheckBox, 
                        QCheckBox.stateChanged,
                        checkBoxSlot(,"#RVStack.mode.alignStartFrames"));

                connect(_strictRangesCheckBox, 
                        QCheckBox.stateChanged,
                        checkBoxSlot(,"#RVStack.mode.strictFrameRanges"));

                connect(_useCutInfoCheckBox, 
                        QCheckBox.stateChanged, 
                        checkBoxSlot(,"#RVStack.mode.useCutInfo"));

                connect(_autoSizeCheckBox, 
                        QCheckBox.stateChanged, 
                        checkBoxSlot(,"#RVStack.output.autoSize"));

                connect(_retimeCheckBox, 
                        QCheckBox.stateChanged, 
                        checkBoxSlot(,"#View.timing.retimeInputs"));

                connect(_interactiveSizeCheckBox, 
                        QCheckBox.stateChanged, 
                        checkBoxSlot(,"#RVStack.output.interactiveSize"));

                connect(_chosenAudioInputCombo, QComboBox.currentIndexChanged, setChosenAudioInput);
                connect(_outputFPSEdit, QLineEdit.editingFinished, fpsChanged);
                connect(_outputWidthEdit, QLineEdit.editingFinished, widthChanged);
                connect(_outputHeightEdit, QLineEdit.editingFinished, heightChanged);
            }

            updateUI();
            manager.useEditor("Stack");
        }

        event.reject();
    }

    method: alignStartFrames (void; Event event) 
    { 
        let p = "#RVStack.mode.alignStartFrames",
            a = getIntProperty(p).front();
        set(p, if a != 0 then 0 else 1);
    }

    method: strictFrameRanges (void; Event event) 
    { 
        let p = "#RVStack.mode.strictFrameRanges",
            s = getIntProperty(p).front();
        set(p, if s != 0 then 0 else 1);
    }

    method: useCutInfo (void; Event event) 
    { 
        let p = "#RVStack.mode.useCutInfo",
            a = getIntProperty(p).front();
        set(p, if a != 0 then 0 else 1);
    }

    method: stateFunc ((int;); string name)
    {
        \: (int;)
        {
            let p = getIntProperty("#RVStack.mode.%s" % name).front();
            if p == 0 then UncheckedMenuState else CheckedMenuState;
        };
    }

    method: retimeState (int;)
    {
        let p = getIntProperty("#View.timing.retimeInputs").front();
        if p == 0 then UncheckedMenuState else CheckedMenuState;
    }

    method: autoRetimeInputs (void; Event event)
    {
        let p = "#View.timing.retimeInputs",
            a = getIntProperty(p).front();
        set(p, if a != 0 then 0 else 1);
    }

    method: menu (Menu;)
    {
        let n = viewNode(),
            t = nodeType(n),
            name = if t == "RVLayoutGroup" then "Layout" else "Stack";

        newMenu(MenuItem[] {
            subMenu(name, MenuItem[] {
                menuSeparator(),
                menuItem("Align Start Frames", "", "", alignStartFrames, stateFunc("alignStartFrames")),
                menuItem("Use Source Cut Info", "", "", useCutInfo, stateFunc("useCutInfo")),
                menuItem("Automatically Retime Inputs", "", "", autoRetimeInputs, retimeState),
                menuItem("Use Strict Frame Ranges", "", "", strictFrameRanges, stateFunc("strictFrameRanges"))
            })
        });
    }

    method: activate (void;)
    {
        //setMenu(menu());
        ;
    }

    method: StackEditMode (StackEditMode; string name)
    {
        _uiInFlux = false;

        init(name,  // this is init from session_manager (its new style)
             nil,
             [("session-manager-load-ui", loadUI, "Load UI into Session Manager"),
              ("range-changed", updateUIEvent, "Update UI"),
              ("image-structure-change", updateUIEvent, "Update UI"),
              ("graph-state-change", propertyChanged,  "Maybe update session UI")],
             nil, //menu(),
             "z");
    }
}

\: createMode (Mode;)
{
    return StackEditMode("Stack_edit_mode");
}

}
