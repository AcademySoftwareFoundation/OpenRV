//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

module: SourceGroup_edit_mode 
{
use rvtypes;
use commands;
use rvui;
use gl;
use glyph;
use app_utils;
use io;
use system;
use app_utils;
use extra_commands;
use qt;
use session_manager;
use math;

class: SourceGroupEditMode : MinorMode
{
    bool        _locked;
    QWidget     _ui;
    QSpinBox    _cutInEdit;
    QSpinBox    _cutOutEdit;
    QCheckBox   _syncCheckBox;
    QPushButton _resetButton;

    method: auxFilePath (string; string name)
    {
        io.path.join(supportPath("session_manager", "session_manager"), name);
    }
    
    method: syncGuiInOut (bool; )
    {
        let p = "#RVFileSource.cut.syncGui";

        if (propertyExists(p)) return (getIntProperty(p).front() != 0);

        return true;
    }

    method: reset (void;)
    {
        _locked = true;
        try 
        {
            if (syncGuiInOut()) 
            {
                setInPoint (frameStart());
                setOutPoint(frameEnd());
            }
            set("#RVFileSource.cut.in",  -int.max);
            set("#RVFileSource.cut.out",  int.max);
        }
        catch (...) { ; }
        _locked = false;
        updateUI();
        redraw();
    }

    method: updateUI (void;)
    {
        if (_ui eq nil) return;

        _locked = true;

        try
        {
            let in     = getIntProperty("#RVFileSource.cut.in").front(),
                out    = getIntProperty("#RVFileSource.cut.out").front();

            _cutInEdit.setValue(in);
            _cutOutEdit.setValue(if (out !=  int.max) then out else -int.max);

            _syncCheckBox.setCheckState (if syncGuiInOut() then Qt.Checked else Qt.Unchecked);
        }
        catch (...)
        {
            ;// The session may have been cleared.
        }
        _locked = false;
    }

    method: resetSlot (void; bool checked) { reset(); }

    method: syncSlot (void; bool checked)
    {
        if (_locked) return;

        let p = "#RVFileSource.cut.syncGui";

        set (p, if (checked) then 1 else 0);
        if (checked) updateFromProps();
        updateUI();
    }

    method: toggleSync (void; Event event)
    {
        syncSlot (!syncGuiInOut());
    }

    method: changedSlot ((void; int); string prop)
    {
        \: (void; int v)
        {
            if (!this._locked && v != -int.max)
            {
                if (v < frameStart()) return;
                if (v > frameEnd())   return;

                if (prop == "in"  && v > outPoint()) return; 
                if (prop == "out" && v < inPoint())  return;

                this._locked = true;

                set("#RVFileSource.cut." + prop, v);

                try {
                    if (syncGuiInOut() && prop == "in")  setInPoint(v);
                    if (syncGuiInOut() && prop == "out") setOutPoint(v);
                } catch (...) {;}
                this._locked = false;
            }
            redraw();
        };
    }
    method: finishedSlot ((void; ); string prop)
    {
        \: (void; )
        {
            let v = if (prop == "in") then this._cutInEdit.value() else this._cutOutEdit.value();

            if (v != -int.max)
            {
                if (v < frameStart()) v = frameStart();
                if (v > frameEnd())   v = frameEnd();

                if (prop == "in"  && v > outPoint()) v = outPoint(); 
                if (prop == "out" && v < inPoint())  v = inPoint(); 

                this._locked = true;

                if (prop == "in")  _cutInEdit.setValue(v);
                if (prop == "out") _cutOutEdit.setValue(v);

                set("#RVFileSource.cut." + prop, v);

                try {
                    if (syncGuiInOut() && prop == "in")  setInPoint(v);
                    if (syncGuiInOut() && prop == "out") setOutPoint(v);
                } catch (...) {;}
                this._locked = false;
            }
            redraw();
        };
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
                _ui            = loadUIFile(manager.auxFilePath("source.ui"), m);

                _cutInEdit    = _ui.findChild("cutInEdit");
                _cutInEdit.setRange(-int.max, int.max);
                _cutInEdit.setSpecialValueText(" ");

                _cutOutEdit   = _ui.findChild("cutOutEdit");
                _cutOutEdit.setRange(-int.max, int.max);
                _cutOutEdit.setSpecialValueText(" ");

                _resetButton  = _ui.findChild("resetButton");
                _syncCheckBox = _ui.findChild("syncCheckBox");

                manager.addEditor("Source", _ui);

                connect(_resetButton, QPushButton.clicked, resetSlot);

                connect(_cutInEdit,  QSpinBox.editingFinished, finishedSlot("in"));
                connect(_cutOutEdit, QSpinBox.editingFinished, finishedSlot("out"));

                connect(_cutInEdit,  QSpinBox.valueChanged, changedSlot("in"));
                connect(_cutOutEdit, QSpinBox.valueChanged, changedSlot("out"));

                connect(_syncCheckBox, QCheckBox.clicked, syncSlot);
            }
            
            updateUI();
            manager.useEditor("Source");
        }
    }

    method: propertyChanged (void; Event event)
    {
        let prop  = event.contents(),
            parts = prop.split("."),
            node  = parts[0];

        if (!_locked && nodeType(node) == "RVFileSource")  
        { 
            updateUI(); 
            if (syncGuiInOut()) updateFromProps();
        }
        event.reject();
    }

    method: cutInPrompt (string; )
    {
        let v = getIntProperty("#RVFileSource.cut.in").front();

        if (v == -int.max) return "Set Source In Point:";
        else return "Set Source In Point (current=%d):" % v;
    }

    method: cutOutPrompt (string; )
    {
        let v = getIntProperty("#RVFileSource.cut.out").front();

        if (v == int.max) return "Set Source Out Point:";
        else return "Set Source Out Point (current=%d):" % v;
    }

    method: setCutValue (void; string prop, string text)
    {
        set("#RVFileSource.cut." + prop, int(text));
        redraw();
    }
    
    method: resetCut (void; Event event) { reset(); }

    method: newInPoint (void; Event event)
    {
        let p = "#RVFileSource.cut.in";

        if (!_locked && syncGuiInOut() && propertyExists(p)) set(p, inPoint());

        event.reject();
    }

    method: newOutPoint (void; Event event)
    {
        let p = "#RVFileSource.cut.out";

        if (!_locked && syncGuiInOut() && propertyExists(p)) set(p, outPoint());

        event.reject();
    }

    method: updateFromProps (void;)
    {
        _locked = true;
        try
        {
            let in  = getIntProperty ("#RVFileSource.cut.in").front(),
                out = getIntProperty ("#RVFileSource.cut.out").front();

            in  = min (max (in,  frameStart()), frameEnd());
            out = min (max (out, frameStart()), frameEnd());
            setInPoint (in);
            setOutPoint(out);
        }
        catch (...) {;}
        _locked = false;
    }

    method: activate (void; )
    {
        if (syncGuiInOut()) updateFromProps();

        MinorMode.activate (this);
    }

    method: syncState (int; )
    {
        if (filterLiveReviewEvents())
        {
            return DisabledMenuState;
        }
    
        if (syncGuiInOut()) then CheckedMenuState else UncheckedMenuState;
    }

    method: sourceMenuState (int; )
    {
        if (filterLiveReviewEvents())
        {
            return DisabledMenuState;
        }

        return NeutralMenuState;
    }

    method: SourceGroupEditMode (SourceGroupEditMode; string name)
    {
        let setCutInMode  = startTextEntryMode(cutInPrompt,  setCutValue("in", )),
            setCutOutMode = startTextEntryMode(cutOutPrompt, setCutValue("out", ));

        init(name,  // this is init from session_manager (its new style)
            nil,
            [("new-in-point",  newInPoint,  "Update In Point"),
             ("new-out-point", newOutPoint, "Update Out Point"),
             ("session-manager-load-ui", loadUI, "Load UI into Session Manager"),
             ("graph-state-change", propertyChanged,  "Maybe update session UI")],
            Menu {
                {"Source", Menu {
                    {"Set Source Cut In ...",  setCutInMode,  nil, sourceMenuState},
                    {"Set Source Cut Out ...", setCutOutMode,  nil, sourceMenuState},
                    {"Clear Source Cut In/Out", resetCut,  nil, sourceMenuState},
                    {"Sync GUI With Source Cut In/Out", toggleSync, nil, syncState},
                    }
                }
            },
            nil);

        _locked = false;
    }
}

\: createMode (Mode;)
{
    return SourceGroupEditMode("SourceGroup_edit_mode");
}
}
