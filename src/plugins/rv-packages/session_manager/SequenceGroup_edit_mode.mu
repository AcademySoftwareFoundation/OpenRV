//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

module: SequenceGroup_edit_mode 
{
use rvtypes;
use commands;
use rvui;
use gl;
use glyph;
use app_utils;
use io;
use system;
use extra_commands;
use app_utils;
use qt;
use session_manager;

class: SequenceGroupEditMode : MinorMode
{
    QWidget _ui;
    QCheckBox _autoEDLCheckBox;
    QCheckBox _useCutInfoCheckBox;
    QCheckBox _retimeCheckBox;
    QCheckBox _autoSizeCheckBox;
    QCheckBox _interactiveSizeCheckBox;
    QLineEdit _outputFPSEdit;
    QLineEdit _outputWidthEdit;
    QLineEdit _outputHeightEdit;
    bool      _disableUpdates;

    method: auxFilePath (string; string name)
    {
        io.path.join(supportPath("session_manager", "session_manager"), name);
    }

    method: beforeSessionRead (void; Event event)
    {
        _disableUpdates = true;
        event.reject();
    }

    method: afterSessionRead (void; Event event)
    {
        _disableUpdates = false;
        updateUI();
        event.reject();
    }


    method: updateUI (void;)
    {
        if (_ui eq nil || _disableUpdates) return;

        try { if (!propertyExists("#RVSequence.mode.autoEDL")) return; }
        catch (...) { return; }

        let a     = getIntProperty("#RVSequence.mode.autoEDL").front(),
            u     = getIntProperty("#RVSequence.mode.useCutInfo").front(),
            r     = getIntProperty("#RVSequenceGroup.timing.retimeInputs").front(),
            fps   = getFloatProperty("#RVSequence.output.fps").front(),
            asize = getIntProperty("#RVSequence.output.autoSize").front(),
            size  = getIntProperty("#RVSequence.output.size"),
            isize = getIntProperty("#RVSequence.output.interactiveSize").front();

        _outputWidthEdit.setEnabled(asize == 0 && isize == 0);
        _outputHeightEdit.setEnabled(asize == 0 && isize == 0);

        _autoEDLCheckBox.setCheckState(if a == 0 then Qt.Unchecked else Qt.Checked);
        _useCutInfoCheckBox.setCheckState(if u == 0 then Qt.Unchecked else Qt.Checked);
        _retimeCheckBox.setCheckState(if r == 0 then Qt.Unchecked else Qt.Checked);
        _autoSizeCheckBox.setCheckState(if asize == 0 then Qt.Unchecked else Qt.Checked);
        _outputFPSEdit.setText("%g" % fps);
        _outputWidthEdit.setText("%d" % size.front());
        _outputHeightEdit.setText("%d" % size.back());
        _interactiveSizeCheckBox.setCheckState(if isize == 0 then Qt.Unchecked else Qt.Checked);
    }

    method: updateUIEvent(void; Event event)
    {
        event.reject();
        updateUI();
    }

    method: fpsChanged (void;)
    {
        let newFPS = float(_outputFPSEdit.text()),
            oldFPS = getFloatProperty("#RVSequence.output.fps").front();
        if (newFPS != oldFPS)
        {
            set("#RVSequence.output.fps", newFPS);
            setFPS(newFPS);
            redraw();
        }
    }

    method: widthChanged (void;)
    {
        let val = float(_outputWidthEdit.text()),
            prop = getIntProperty("#RVSequence.output.size");

        setIntProperty("#RVSequence.output.size", int[] {val, prop.back()});
        redraw();
    }

    method: heightChanged (void;)
    {
        let val = float(_outputHeightEdit.text()),
            prop = getIntProperty("#RVSequence.output.size");

        setIntProperty("#RVSequence.output.size", int[] {prop.front(), val});
        redraw();
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

        if (comp == "mode" || comp == "output")
        {
            if (name == "autoEDL" || 
                name == "autoSize" ||
                name == "useCutInfo" ||
                name == "width" ||
                name == "fps" ||
                name == "height" ||
                name == "interactiveSize")
            {
                updateUI();
                redraw();
            }
        }

        event.reject();
    }

    method: checkBoxSlot (void; int state, string name)
    {
        int current = getIntProperty(name).front();
        int value = if state == Qt.Checked then 1 else 0;
        if (value != current) set(name, value);
    }

    method: activateUI (void;)
    {
        State state = data();

        if (state neq nil && 
            state.sessionManager neq nil)
        {
            SessionManagerMode manager = state.sessionManager;
            let m = mainWindowWidget();

            if (_ui eq nil)
            {
                _ui                      = loadUIFile(manager.auxFilePath("sequence.ui"), m);
                _autoEDLCheckBox         = _ui.findChild("autoEDLCheckBox");
                _useCutInfoCheckBox      = _ui.findChild("useCutInfoCheckBox");
                _retimeCheckBox          = _ui.findChild("retimeInputsCheckBox");
                _outputFPSEdit           = _ui.findChild("outputFPSEdit");
                _outputWidthEdit         = _ui.findChild("outputWidthEdit");
                _outputHeightEdit        = _ui.findChild("outputHeightEdit");
                _autoSizeCheckBox        = _ui.findChild("autoSizeCheckBox");
                _interactiveSizeCheckBox = _ui.findChild("interactiveResizeCheckBox");
                manager.addEditor("Sequence", _ui);

                connect(_autoEDLCheckBox, QCheckBox.stateChanged, checkBoxSlot(,"#RVSequence.mode.autoEDL"));
                connect(_useCutInfoCheckBox, QCheckBox.stateChanged, checkBoxSlot(,"#RVSequence.mode.useCutInfo"));
                connect(_autoSizeCheckBox, QCheckBox.stateChanged, checkBoxSlot(,"#RVSequence.output.autoSize"));
                connect(_retimeCheckBox, QCheckBox.stateChanged, checkBoxSlot(,"#RVSequenceGroup.timing.retimeInputs"));
                connect(_interactiveSizeCheckBox, QCheckBox.stateChanged, checkBoxSlot(,"#RVSequence.output.interactiveSize"));

                connect(_outputFPSEdit, QLineEdit.editingFinished, fpsChanged);
                connect(_outputWidthEdit, QLineEdit.editingFinished, widthChanged);
                connect(_outputHeightEdit, QLineEdit.editingFinished, heightChanged);
            }

            updateUI();
            manager.useEditor("Sequence");
        }
    }

    method: loadUI (void; Event event)
    {
        _disableUpdates = false;
        activateUI();
        event.reject();
    }

    method: activate (void;) { _disableUpdates = false; activateUI(); }

    method: autoEDL (void; Event event) 
    { 
        let p = "#RVSequence.mode.autoEDL",
            a = getIntProperty(p).front();
        set(p, if a != 0 then 0 else 1);
    }

    method: useCutInfo (void; Event event) 
    { 
        let p = "#RVSequence.mode.useCutInfo",
            a = getIntProperty(p).front();
        set(p, if a != 0 then 0 else 1);
    }
    
    method: stateFunc ((int;); string name)
    {
        \: (int;)
        {
            if (filterLiveReviewEvents()) {
                return DisabledMenuState;
            }
            let p = getIntProperty("#RVSequence.mode.%s" % name).front();
            if p == 0 then UncheckedMenuState else CheckedMenuState;
        };
    }

    method: menu (Menu;)
    {
        Menu {
            {"Sequence", Menu {
                {"_", nil, nil, nil},
                {"Auto EDL", autoEDL, nil, stateFunc("autoEDL")},
                {"Use Source Cut Info", useCutInfo, nil, stateFunc("useCutInfo")}
            }
        }};
    }

   
    method: SequenceGroupEditMode (SequenceGroupEditMode; string name)
    {
        _disableUpdates = false;

        init(name,  // this is init from session_manager (its new style)
             nil,
             [("session-manager-load-ui", loadUI, "Load UI into Session Manager"),
              ("range-changed", updateUIEvent, "Update UI on range change"),
              ("image-structure-change", updateUIEvent, "Update UI on range change"),
               ("before-session-read", beforeSessionRead, "Freeze Updates"),
               ("after-session-read", afterSessionRead, "Resume Updates"),
              ("graph-state-change", propertyChanged,  "Maybe update session UI")],
             menu(),
             nil);
    }
}

\: createMode (Mode;)
{
    return SequenceGroupEditMode("SequenceGroup_edit_mode");
}
}
