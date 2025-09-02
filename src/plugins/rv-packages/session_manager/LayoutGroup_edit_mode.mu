//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

module: LayoutGroup_edit_mode 
{
use rvtypes;
use commands;
use session_manager;
use math;
use math_util;
use rvui;
use gl;
use glyph;
use app_utils;
use io;
use system;
use app_utils;
use extra_commands;
use qt;

class: LayoutGroupEditMode : MinorMode
{
    QWidget   _ui;
    QComboBox _modeCombo;
    QSlider   _spacingSlider;
    QLineEdit _gridRowsLineEdit;
    QLineEdit _gridColumnsLineEdit;

    method: auxFilePath (string; string name)
    {
        io.path.join(supportPath("session_manager", "session_manager"), name);
    }

    method: layoutMode (string;)
    {
        let modeProp = "#RVLayoutGroup.layout.mode";

        try
        {
            return getStringProperty(modeProp).front();
        }
        catch (...)
        {
            return "";
        }
    }

    method: setLayoutMode (void; string mode)
    {
        let modeProp = "#RVLayoutGroup.layout.mode";
        setStringProperty(modeProp, string[] {mode}, true);
    }

    method: setSpacing (void; float value)
    {
        let prop = "#RVLayoutGroup.layout.spacing";
        setFloatProperty(prop, float[] {value}, true);
    }

    method: setGridRowsColumns (void; int rows, int columns)
    {
        let prop = "#RVLayoutGroup.layout.";
        setIntProperty(prop + "gridRows",    int[] {rows},    true);
        setIntProperty(prop + "gridColumns", int[] {columns}, true);

        setLayoutMode ("grid");
    }

    method: updateUI (void;)
    {
        if (_ui eq nil) return;

        try
        {
            case (layoutMode())
            {
                "packed"    -> { _modeCombo.setCurrentIndex(0); }
                "packed2"   -> { _modeCombo.setCurrentIndex(1); }
                "row"       -> { _modeCombo.setCurrentIndex(2); }
                "column"    -> { _modeCombo.setCurrentIndex(3); }
                "grid"      -> { _modeCombo.setCurrentIndex(4); }
                "manual"    -> { _modeCombo.setCurrentIndex(5); }
                _           -> { _modeCombo.setCurrentIndex(6); }
            }

            float sp = getFloatProperty("#RVLayoutGroup.layout.spacing").front();
            _spacingSlider.setValue( int((clamp(sp, 0.5, 1.0) * 2.0 - 1.0) * 999.0) );

            int r = getIntProperty("#RVLayoutGroup.layout.gridRows").front();
            _gridRowsLineEdit.setText("%d" % r);

            int c = getIntProperty("#RVLayoutGroup.layout.gridColumns").front();
            _gridColumnsLineEdit.setText("%d" % c);
        }
        catch (...)
        {
            _modeCombo.setCurrentIndex(0);
        }
    }

    method: propertyChanged (void; Event event)
    {
        let prop  = event.contents(),
            parts = prop.split("."),
            node  = parts[0],
            comp  = parts[1],
            name  = parts[2];

        if (comp == "layout" && _ui neq nil)
        {
            case (name)
            {
                "mode"        -> { updateUI(); redraw(); }
                "spacing"     -> { updateUI(); redraw(); }
                "gridRows"    -> { updateUI(); redraw(); }
                "gridColumns" -> { updateUI(); redraw(); }
                _             -> {;}
            }
        }

        event.reject();
    }

    method: spacingSliderChangedSlot (void; int value)
    {
        setSpacing(float(value) / 999.0 / 2.0 + 0.5);
    }

    method: gridRowsChangedSlot (void;)
    {
        let newRows = int(_gridRowsLineEdit.text());

        setGridRowsColumns(newRows, 0);
        redraw();
    }

    method: gridColumnsChangedSlot (void;)
    {
        let newColumns = int(_gridColumnsLineEdit.text());

        setGridRowsColumns(0, newColumns);
        redraw();
    }

    method: modeComboChangedSlot (void; int index)
    {
        case (index)
        {
            0 -> { layoutPacked(); }
            1 -> { layoutPacked2(); }
            2 -> { layoutInRow(); }
            3 -> { layoutInColumn(); }
            4 -> { layoutInGrid(); }
            5 -> { layoutManually(); }
            _ -> { layoutStatic(); }
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
                _ui                  = loadUIFile(manager.auxFilePath("layout.ui"), m);
                _modeCombo           = _ui.findChild("modeCombo");
                _spacingSlider       = _ui.findChild("spacingSlider");
                _gridRowsLineEdit    = _ui.findChild("gridRowsLineEdit");
                _gridColumnsLineEdit = _ui.findChild("gridColumnsLineEdit");

                manager.addEditor("Layout", _ui);
                connect(_modeCombo, QComboBox.currentIndexChanged, modeComboChangedSlot);
                connect(_spacingSlider, QSlider.sliderMoved, spacingSliderChangedSlot);
                connect(_gridRowsLineEdit, QLineEdit.editingFinished, gridRowsChangedSlot);
                connect(_gridColumnsLineEdit, QLineEdit.editingFinished, gridColumnsChangedSlot);
            }

            updateUI();
            manager.useEditor("Layout");
        }

        event.reject();
    }

    method: layoutInRow (void;) 
    { 
        setLayoutMode("row"); 
        activateTransformMode(false); 
    }

    method: layoutInColumn (void;) 
    { 
        setLayoutMode("column");
        activateTransformMode(false); 
    }

    method: layoutPacked (void;) 
    { 
        setLayoutMode("packed");
        activateTransformMode(false); 
    }

    method: layoutInGrid (void;) 
    { 
        setLayoutMode("grid");
        activateTransformMode(false); 
    }

    method: layoutPacked2 (void;) 
    { 
        setLayoutMode("packed2");
        activateTransformMode(false); 
    }

    method: layoutManually (void;) 
    { 
        setLayoutMode("manual");
        activateTransformMode(true);
    }

    method: layoutStatic (void;) 
    { 
        setLayoutMode("static");
        activateTransformMode(false);
    }

    method: layoutPackedEvent (void; Event event) { layoutPacked(); }
    method: layoutPacked2Event (void; Event event) { layoutPacked2(); }
    method: layoutInRowEvent (void; Event event) { layoutInRow(); }
    method: layoutInColumnEvent (void; Event event) { layoutInColumn(); }
    method: layoutInGridEvent (void; Event event) { layoutInGrid(); }
    method: layoutManuallyEvent (void; Event event) { layoutManually(); }
    method: layoutStaticEvent (void; Event event) { layoutStatic(); }


    method: activateTransformMode (void; bool on)
    {
        use mode_manager;
        State state = data();
        mode_manager.ModeManagerMode mm = state.modeManager;
        let entry = mm.findModeEntry("transform_manip");
        mm.activateEntry(entry, on);
    }

    method: activateUI (void; bool on)
    {
        State state = data();
        mode_manager.ModeManagerMode mm = state.modeManager;

        for_each (mode; ["Stack_edit_mode", "Composite_edit_mode"])
        {
            let entry = mm.findModeEntry(mode);
            mm.activateEntry(entry, on);
        }
    }

    method: deactivate (void;) 
    { 
        activateUI(false); 
        activateTransformMode(false);
    }

    method: activate (void;)
    {
        activateUI(true);
        activateTransformMode(layoutMode() == "manual");
    }

    method: isLayoutMode ((int;); string name)
    {
        \: (int;)
        {
            if (filterLiveReviewEvents()) {
                return DisabledMenuState;
            }
            if this.layoutMode() == name then CheckedMenuState else UncheckedMenuState;
        };
    }

    method: LayoutGroupEditMode (LayoutGroupEditMode; string name)
    {
        init(name,
             [ ("session-manager-load-ui", loadUI, "Load UI into Session Manager"),
               ("graph-state-change", propertyChanged,  "Maybe update session UI")],
             nil,
             Menu {
                 {"Layout", Menu {
                     {"Layout Method", noop, nil, disabledItem},
                     {"    Packed", layoutPackedEvent, nil, isLayoutMode("packed")},
                     {"    Packed With Fluid Layout", layoutPacked2Event, nil, isLayoutMode("packed2")},
                     {"    Row", layoutInRowEvent, nil, isLayoutMode("row")},
                     {"    Column", layoutInColumnEvent, nil, isLayoutMode("column")},
                     {"    Grid", layoutInGridEvent, nil, isLayoutMode("grid")},
                     {"    Manual", layoutManuallyEvent, nil, isLayoutMode("manual")},
                     {"    Static", layoutStaticEvent, nil, isLayoutMode("static")},
                     }
                 }},
             "a"
             );

        activateTransformMode(layoutMode() == "manual");
    }

}

\: createMode (Mode;)
{
    return LayoutGroupEditMode("LayoutGroup_edit_mode");
}
}
