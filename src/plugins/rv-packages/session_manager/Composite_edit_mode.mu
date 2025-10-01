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
    QLineEdit _dissolveLineEdit;
    QLabel _dissolveLabel;
    QSlider _dissolveSlider;
    
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
            2 -> { name = "dissolve"; }
            3 -> { name = "difference"; }
            4 -> { name = "-difference"; }
            5 -> { name = "replace"; }
            6 -> { name = "topmost"; }
        }

        set("#RVStack.composite.type", name);
        redraw();
    }

    method: setOpEvent (void; Event event, int index)
    {
        setOp(index);
    }

    method: setDissolveAmount (void;)
    {
        let amountText = _dissolveLineEdit.text();
        
        try
        {
            float amount = float(amountText);
            // Clamp to valid range
            if (amount < 0.0) amount = 0.0;
            if (amount > 1.0) amount = 1.0;
            
            // Update slider to match (0.0-1.0 -> 0-100)
            _dissolveSlider.setValue(int(amount * 100.0));
            
            set("#RVStack.composite.dissolveAmount", float[] {amount});
            redraw();
        }
        catch (...)
        {
            // If parsing fails, reset to default
            _dissolveLineEdit.setText("0.5");
            _dissolveSlider.setValue(50);
            set("#RVStack.composite.dissolveAmount", float[] {0.5});
            redraw();
        }
    }

    method: setDissolveAmountFromSlider (void; int value)
    {
        // Convert slider value (0-100) to float (0.0-1.0)
        float amount = float(value) / 100.0;
        
        // Update text field
        _dissolveLineEdit.setText("%g" % amount);
        
        set("#RVStack.composite.dissolveAmount", float[] {amount});
        redraw();
    }

    method: updateUI (void;)
    {
        if (_ui eq nil) return;

        int index = 0;
        string currentType = getStringProperty("#RVStack.composite.type").front();
        
        case (currentType)
        {
            "over"        -> { index = 0; }
            "add"         -> { index = 1; }
            "dissolve"    -> { index = 2; }
            "difference"  -> { index = 3; }
            "-difference" -> { index = 4; }
            "replace"     -> { index = 5; }
            "topmost"     -> { index = 6; }
            _             -> { index = 7; }
        }

        _comboBox.setCurrentIndex(index);

        // Show/hide dissolve amount controls based on mode
        bool showDissolve = (currentType == "dissolve");
        _dissolveLineEdit.setVisible(showDissolve);
        _dissolveLabel.setVisible(showDissolve);
        _dissolveSlider.setVisible(showDissolve);
        
        // Resize UI to fit the visible controls
        _ui.adjustSize();

        // Update dissolve amount value
        if (showDissolve)
        {
            try
            {
                float[] amounts = getFloatProperty("#RVStack.composite.dissolveAmount");
                if (amounts.size() > 0)
                {
                    float amount = amounts[0];
                    _dissolveLineEdit.setText("%g" % amount);
                    _dissolveSlider.setValue(int(amount * 100.0));
                }
            }
            catch (...)
            {
                _dissolveLineEdit.setText("0.5"); // Default value
                _dissolveSlider.setValue(50);
            }
        }
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
            else if (name == "dissolveAmount") updateUI();
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
                _dissolveLineEdit = _ui.findChild("dissolveLineEdit");
                _dissolveLabel = _ui.findChild("dissolveLabel");
                _dissolveSlider = _ui.findChild("dissolveSlider");
                
                // Initially hide dissolve controls
                _dissolveLineEdit.setVisible(false);
                _dissolveLabel.setVisible(false);
                _dissolveSlider.setVisible(false);
                
                manager.addEditor("Composite Function", _ui);
                connect(_comboBox, QComboBox.currentIndexChanged, setOp);
                connect(_dissolveLineEdit, QLineEdit.editingFinished, setDissolveAmount);
                connect(_dissolveSlider, QSlider.valueChanged, setDissolveAmountFromSlider);
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
             Menu {
                 {"Stack", Menu {
                     {"Composite Operation", nil, nil, inactiveState},
                     {"   Over", setOpEvent(,0), nil, opState("over")},
                     {"   Add", setOpEvent(,1), nil, opState("add")},
                     {"   Dissolve", setOpEvent(,2), nil, opState("dissolve")},
                     {"   Difference", setOpEvent(,3), nil, opState("difference")},
                     {"   Inverted Difference", setOpEvent(,4), nil, opState("-difference")},
                     {"   Replace", setOpEvent(,5), nil, opState("replace")},
                     {"   Topmost", setOpEvent(,6), nil, opState("topmost")},
                     {"_", nil, nil, nil},
                     {"Cycle Forward", cycleStackForward, nil, isStackMode},
                     {"Cycle Backward", cycleStackBackward, nil, isStackMode}
                     }
                 }},
             "b");
    }
}

\: createMode (Mode;)
{
    return CompositeEditMode("Composite_edit_mode");
}
}
