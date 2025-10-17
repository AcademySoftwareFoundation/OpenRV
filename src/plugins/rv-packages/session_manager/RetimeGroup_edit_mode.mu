//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

module: RetimeGroup_edit_mode 
{
use qt;
use rvtypes;
use commands;
use rvui;
use gl;
use glyph;
use app_utils;
use io;
use system;
use extra_commands;
use session_manager;

class: RetimeGroupEditMode : MinorMode
{
    QWidget     _ui;
    QLineEdit   _fpsEdit;
    QLineEdit   _voffsetEdit;
    QLineEdit   _aoffsetEdit;
    QLineEdit   _vscaleEdit;
    QLineEdit   _ascaleEdit;
    QPushButton _reverseButton;
    QPushButton _resetButton;

    method: auxFilePath (string; string name)
    {
        io.path.join(supportPath("session_manager", "session_manager"), name);
    }

    method: reset (void;)
    {
        set("#RVRetime.visual.scale", 1.0);
        set("#RVRetime.visual.offset", 0.0);
        set("#RVRetime.audio.scale", 1.0);
        set("#RVRetime.audio.offset", 0.0);
        redraw();
    }

    method: reverse (void;)
    {
        let len = (frameEnd() - frameStart()),
            scl = getFloatProperty("#RVRetime.visual.scale").front();

        if (scl < 0)
        {
            set("#RVRetime.visual.scale", 1.0);
            set("#RVRetime.visual.offset", 0);
            set("#RVRetime.audio.scale", 1.0);
            set("#RVRetime.audio.offset", 0);
        }
        else
        {
            set("#RVRetime.visual.scale", -1.0);
            set("#RVRetime.visual.offset", float(-len));
            set("#RVRetime.audio.scale", 1.0);
            set("#RVRetime.audio.offset", 0);
        }
        
        redraw();
    }

    method: updateUI (void;)
    {
        if (_ui eq nil) return;

        let fps     = getFloatProperty("#RVRetime.output.fps").front(),
            vscale  = getFloatProperty("#RVRetime.visual.scale").front(),
            ascale  = getFloatProperty("#RVRetime.audio.scale").front(),
            voffset = getFloatProperty("#RVRetime.visual.offset").front(),
            aoffset = getFloatProperty("#RVRetime.audio.offset").front(),
            speed   = (3.0 + vscale) / 6.0;

        _fpsEdit.setText("%g" % fps);
        _vscaleEdit.setText("%g" % vscale);
        _ascaleEdit.setText("%g" % ascale);
        _voffsetEdit.setText("%g" % voffset);
        _aoffsetEdit.setText("%g" % aoffset);
    }

    method: resetSlot (void; bool checked) { reset(); }
    method: reverseSlot (void; bool checked) { reverse(); }

    method: editSlot ((void;); QLineEdit lineEdit, string prop)
    {
        \: (void;)
        {
            let v = float(lineEdit.text());
            set("#RVRetime" + prop, v);
            if (prop == ".output.fps") setFPS(v);
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
                _ui            = loadUIFile(manager.auxFilePath("retime.ui"), m);
                _fpsEdit       = _ui.findChild("fpsEdit");
                _ascaleEdit    = _ui.findChild("ascaleEdit");
                _vscaleEdit    = _ui.findChild("vscaleEdit");
                _aoffsetEdit   = _ui.findChild("aoffsetEdit");
                _voffsetEdit   = _ui.findChild("voffsetEdit");
                _resetButton   = _ui.findChild("resetButton");
                _reverseButton = _ui.findChild("reverseButton");

                manager.addEditor("Retime", _ui);

                connect(_resetButton, QPushButton.clicked, resetSlot);
                connect(_reverseButton, QPushButton.clicked, reverseSlot);

                for_each (edit; [(_fpsEdit, ".output.fps"),
                                 (_ascaleEdit, ".audio.scale"),
                                 (_vscaleEdit, ".visual.scale"),
                                 (_aoffsetEdit, ".audio.offset"),
                                 (_voffsetEdit, ".visual.offset")])
                {
                    connect(edit._0, 
                            QLineEdit.editingFinished,
                            editSlot(edit._0, edit._1));
                }
            }

            updateUI();
            manager.useEditor("Retime");
        }
    }

    method: propertyChanged (void; Event event)
    {
        let prop  = event.contents(),
            parts = prop.split("."),
            node  = parts[0],
            comp  = parts[1],
            name  = parts[2];

        if (nodeType(node) == "RVRetime") updateUI();
        event.reject();
    }

    method: factorPrompt (string; string fmt, bool invert)
    {
        let factor = getFloatProperty("#RVRetime.visual.scale").front();
        fmt % (if invert then 1.0 / factor else factor);
    }

    method: slowDownPrompt (string;) 
    { 
        factorPrompt("Slow Down by Factor (current=%g):", true); 
    }

    method: speedUpPrompt (string;) 
    { 
        factorPrompt("Speed Up by Factor (current=%g):", false); 
    }

    method: setFactorValue (void; string text, bool invert)
    {
        let factor = if invert then 1.0 / float(text) else float(text);
        set("#RVRetime.visual.scale", factor);
        redraw();
    }


    method: fpsPrompt (string;)
    {
        "Convert to FPS (current=%g):" % getFloatProperty("#RVRetime.output.fps").front();
    }

    method: setConvertFPS (void; string text)
    {
        let newFPS = float(text);
        set("#RVRetime.output.fps", newFPS);
        setFPS(newFPS);
    }

    method: convertToFPS (void; Event event, float newFPS)
    {
        for_each (src; sourcesRendered())
        {
            set("#RVRetime.output.fps", newFPS);
        }

        setFPS(newFPS);
    }


    method: resetTiming (void; Event event) { reset(); }
    method: reverseTiming (void; Event event) { reverse(); }

    method: RetimeGroupEditMode (RetimeGroupEditMode; string name)
    {
        let editVScale     = startParameterMode("#RVRetime.visual.scale", 0.05, 1.0),
            editVOffset    = startParameterMode("#RVRetime.visual.offset", 0.05, 0.0),
            editAScale     = startParameterMode("#RVRetime.audio.scale", 0.05, 1.0),
            editAOffset    = startParameterMode("#RVRetime.audio.offset", 0.05, 0.0),
            slowDownFactor = startTextEntryMode(slowDownPrompt, setFactorValue(,true)),
            speedUpFactor  = startTextEntryMode(speedUpPrompt, setFactorValue(,false)),
            editFPS        = startTextEntryMode(fpsPrompt, setConvertFPS);

        init(name,  // this is init from session_manager (its new style)
             nil,
             [("session-manager-load-ui", loadUI, "Load UI into Session Manager"),
              ("graph-state-change", propertyChanged,  "Maybe update session UI")],
             newMenu(MenuItem[] {
                 subMenu("Retime", MenuItem[] {
                     subMenu("Convert to FPS", MenuItem[] {
                             menuItem("24", "", "", convertToFPS(,24), enabledItem),
                             menuItem("25", "", "", convertToFPS(,25), enabledItem),
                             menuItem("23.98", "", "", convertToFPS(,23.98), enabledItem),
                             menuItem("30", "", "", convertToFPS(,30), enabledItem),
                             menuItem("29.97", "", "", convertToFPS(,29.97), enabledItem),
                             menuSeparator(),
                             menuItem("Custom...", "", "", editFPS, enabledItem)
                     }),
                     menuSeparator(),
                     menuItem("Slow Down by Factor...", "", "", slowDownFactor, enabledItem),
                     menuItem("Speed Up By Factor...", "", "", speedUpFactor, enabledItem),
                     menuItem("Reverse", "", "", reverseTiming, enabledItem),
                     menuSeparator(),
                     menuText("Edit Raw"),
                     menuItem("    Visual Scale...", "", "", editVScale, enabledItem),
                     menuItem("    Visual Offset...", "", "", editVOffset, enabledItem),
                     menuItem("    Audio Scale...", "", "", editAScale, enabledItem),
                     menuItem("    Audio Offset...", "", "", editAOffset, enabledItem),
                     menuSeparator(),
                     menuItem("Reset Timing", "", "", resetTiming, enabledItem)
                 })
             }),
             nil);
    }
}

\: createMode (Mode;)
{
    return RetimeGroupEditMode("RetimeGroup_edit_mode");
}
}
