module: annotate_mode {
use math;
use math_util;
use rvui;
use rvtypes;
use app_utils;
use extra_commands;
use commands;
use qt;
use gl;
use glu;
require glyph;
require io;
require system;

class: DrawDockWidget : QDockWidget
{
    Mode _mode;

    method: DrawDockWidget (DrawDockWidget; QWidget parent, Mode m)
    {
        QDockWidget.QDockWidget("Draw", parent, Qt.Tool);
        _mode = m;
    }

    method: closeEvent(void; QCloseEvent event)
    {
        _mode.toggle();

        // Dont execute this event by OS. We dont want
        // that Windows closes it.
        event.ignore();
    }
}

class: AnnotateMinorMode : MinorMode
{
    //union: UIMode { Off, FreeHandDraw, FreeHandErase, TextPlacement, IconPlacement }

    //
    //  These are the direct values used by the render for each
    //  stroke. Don't confused this with the InterfaceMode below.
    //

    RenderOverMode    := 0;
    RenderEraseMode   := 1;
    RenderScaleMode   := 2;
    RenderAddMode     := 3;

    NoJoin      := 0;
    BevelJoin   := 1;
    MiterJoin   := 2;
    RoundJoin   := 3;

    NoCap       := 0;
    SquareCap   := 1;
    RoundCap    := 2;

    ColorFunc   := (Color; Color);

    union: PressureMode { None | Size | Opacity | Saturation }

    class: DrawMode
    {
        string       name;
        string       settingsName;
        QToolButton  button;
        QAction      action;
        string       eventTable;
        int          cursor;
        float        size;
        Color        color;
        int          renderMode;
        string       brushName;
        int          join;
        int          cap;
        float        maxSize;
        float        minSize;
        int          startFrame;
        int          duration;
        PressureMode pressureMode;
        DrawMode     eraserMode;
        DrawMode     penMode;
        string       sliderName;
        ColorFunc    colorFunction;
    }

    MetaEvalInfo      _currentNodeInfo;
    string            _currentNode;
    DrawMode          _currentDrawMode;
    DrawMode[]        _drawModes;
    DrawMode          _selectDrawMode;
    DrawMode          _textDrawMode;
    DrawMode          _dropperDrawMode;
    DrawMode          _penDrawMode;
    DrawMode          _airBrushDrawMode;
    DrawMode          _hardEraseDrawMode;
    DrawMode          _softEraseDrawMode;
    DrawMode          _dodgeDrawMode;
    DrawMode          _burnDrawMode;
    DrawMode          _cloneDrawMode;
    DrawMode          _smudgeDrawMode;
    string            _currentDrawObject;
    string            _drawModeTable;
    int               _debug;
    Color             _color;
    Point             _pointer;
    float             _pointerRadius;
    Point             _editPoint;
    string            _image;
    string            _user;
    string            _machine;
    bool              _linkToolColors;
    bool              _colorDialogLock;
    QPushButton       _colorButton;
    QColorDialog      _colorDialog;
    //QComboBox         _comboBox;
    //QComboBox         _pressureCombo;
    QToolButton       _undoButton;
    QToolButton       _redoButton;
    QToolButton       _clearButton;
    QToolButton       _showDrawingButton;
    QToolButton       _disabledButton;
    QToolButton       _dropperButton;
    QToolButton       _textButton;
    QToolButton       _circleBrushButton;
    QToolButton       _gaussBrushButton;
    QToolButton       _hardEraseButton;
    QToolButton       _softEraseButton;
    QToolButton       _dodgeButton;
    QToolButton       _burnButton;
    QToolButton       _cloneButton;
    QToolButton       _smudgeButton;
    QSlider           _sizeSlider;
    QSlider           _opacitySlider;
    QAction           _undoAct;
    QAction           _redoAct;
    QAction           _clearAct;
    QListWidget       _annotationsWidget;
    QTextEdit         _notesEdit;
    QDockWidget       _manageDock;
    DrawDockWidget    _drawDock;
    QToolBar          _toolBar;
    QLabel            _toolSliderLabel;
    QtColorTriangle   _colorTriangle;
    bool              _setColorLock;
    bool              _activeSampleColor;
    Color             _sampleColor;
    int               _sampleCount;

    QWidget           _managePane;
    QWidget           _drawPane;

    bool              _syncWholeStrokes;
    bool              _syncAutoStart;
    bool              _showBrush;
    bool              _scaleBrush;
    bool              _autoSave;
    bool              _storeOnSrc;
    bool              _autoMark;
    bool              _pointerGone;
    int               _dockArea;
    bool              _topLevel;

    bool              _textPlacementMode;
    char[]            _textBuffer;

    char              _cursorChar;

    int              _hideDrawPane;

    // filtering control based on time and dx/dy deltas during drag events
    // to prevent sending ridiculous number of mouse/tablet events if using
    // high sampling rate devices like some 1000hz wacoms.
    int             _dragLastMsec;
    Point           _dragLastPointer;

    \: colorToArray (float[]; Color c) { float[] {c.x, c.y, c.z, c.w}; }
    \: arrayToColor (Color; float[] a) { Color(a[0], a[1], a[2], a[3]); }
    method: encodedName (string; string name) { regex.replace("\.", _user, "+"); }
    method: decodedName (string; string name) { regex.replace("\+", _user, "."); }

    \: dodgeFunc (Color; Color c)
    {
        float s = 1.0 + c.w;
        return c * Color(s,s,s,c.w);
    }

    \: burnFunc (Color; Color c)
    {
        float s = (1.0 - c.w) * 0.75 + 0.25;
        return c * Color(s,s,s,c.w);
    }

    method: findPaintNodes (MetaEvalInfo[];)
    {
        let infos = metaEvaluate(frame(), viewNode());
        MetaEvalInfo[] nodes;

        for_each (i; infos)
        {
            if (i.nodeType == "RVPaint") nodes.push_back(i);
        }

        return nodes;
    }

    method: updateCurrentNode (void;)
    {
        try
        {
            let infos = findPaintNodes();

            if (infos eq nil)
            {
                _currentNodeInfo = nil;
                _currentNode = nil;
            }
            else if (_storeOnSrc)
            {
                //
                //  Find first source node:
                //
                let sourcePaintCount = 0;

                for_each (i; infos)
                {
                    if (nodeType(nodeGroup(i.node)) == "RVSourceGroup")
                    {
                        if (sourcePaintCount == 0)
                        {
                            _currentNodeInfo = i;
                            _currentNode = i.node;
                        }
                        ++sourcePaintCount;
                    }
                }
                if (sourcePaintCount != 1)
                {
                    _currentNodeInfo = infos.front();
                    _currentNode = _currentNodeInfo.node;
                }
            }
            else
            {
                _currentNodeInfo = infos.front();
                _currentNode = _currentNodeInfo.node;
            }
        }
        catch (...)
        {
            _currentNodeInfo = nil;
            _currentNode = nil;
        }
    }

    method: saveSettings (void;)
    {
        use SettingsValue;

        writeSetting("Annotate", "syncWholeStrokes", Bool(_syncWholeStrokes));
        writeSetting("Annotate", "showBrush", Bool(_showBrush));
        writeSetting("Annotate", "scaleBrush", Bool(_scaleBrush));
        writeSetting("Annotate", "autoSaveEnabled", Bool(_autoSave));
        writeSetting("Annotate", "storeOnSrc", Bool(_storeOnSrc));
        writeSetting("Annotate", "autoMark", Bool(_autoMark));
        writeSetting("Annotate", "linkToolColors", Bool(_linkToolColors));

        for_each (d; _drawModes)
        {
            writeSetting("Annotate",
                         d.settingsName + "_color",
                         FloatArray(colorToArray(d.color)));

            writeSetting("Annotate",
                         d.settingsName + "_size",
                         Float(d.size));
        }

        writeSetting("Annotate", "drawmode", String(_currentDrawMode.name));
        writeSetting("Annotate", "dockArea", Int(_dockArea));
        writeSetting("Annotate", "topLevel", Bool(_topLevel));
    }

    method: loadSettings (void;)
    {
        use SettingsValue;

        let Bool b0 = readSetting("Annotate", "syncWholeStrokes", Bool(false)),
            Bool b1 = readSetting("Annotate", "showBrush", Bool(true)),
            Bool b2 = readSetting("Annotate", "scaleBrush", Bool(true)),
            Bool b3 = readSetting("Annotate", "autoSaveEnabled", Bool(true)),
            Bool b4 = readSetting("Annotate", "storeOnSrc", Bool(false)),
            Bool b5 = readSetting("Annotate", "autoMark", Bool(false)),
            Bool b6 = readSetting("Annotate", "linkToolColors", Bool(false));

        _syncWholeStrokes = b0;
        _showBrush        = b1;
        _scaleBrush       = b2;
        _autoSave         = b3;
        _storeOnSrc       = b4;
        _autoMark         = b5;
        _linkToolColors   = b6;

        let String name = readSetting("Annotate", "drawmode", String("Pen"));

        for_each (d; _drawModes)
        {
            let FloatArray a = readSetting("Annotate",
                                            d.settingsName + "_color",
                                            FloatArray(float[] {1,1,1,1}));

            let Float f = readSetting("Annotate",
                                      d.settingsName + "_size",
                                      Float(0.01));

            d.color = arrayToColor(a);
            d.size  = f;

            if (d.name == name) _currentDrawMode = d;
        }

        let Int di = readSetting("Annotate", "dockArea", Int(Qt.LeftDockWidgetArea));

        _dockArea = di;

        //let Bool tl = readSetting("Annotate", "topLevel", Bool(true));
        //_topLevel = tl;
    }

    method: nextID (int; string node)
    {
        let n = "%s.paint.nextId" % node,
            i = getIntProperty(n).front() + 1;
        setIntProperty(n, int[] {i});
        i;
    }

    method: nextAnnotationID (int;)
    {
        let n = "%s.paint.nextAnnotationId" % _currentNode,
            i = getIntProperty(n).front() + 1;
        setIntProperty(n, int[] {i});
        i;
    }

    method: beginCompoundStateChange (void;)
    {
        sendInternalEvent("internal-sync-begin-accumulate");
    }

    method: endCompoundStateChange (void; bool force=false)
    {
        sendInternalEvent("internal-sync-end-accumulate");
        if (force) sendInternalEvent("internal-sync-flush");
    }

    \: toQColor (QColor; Color c)
    {
        QColor(int(c.x * 255.0), int(c.y * 255.0), int(c.z * 255.0), int(c.w * 255.0));
    }

    \: fromQColor(Color; QColor c)
    {
        Color(c.redF(), c.greenF(), c.blueF(), c.alphaF());
    }

    method: newUniqueName (string; string node, string type, int frame)
    {
        "%s.%s:%d:%d:%s" % (node, type, nextID(node), frame, encodedName(_user), _machine);
    }

    method: frameOrderName (string; string node, int frame)
    {
        "%s.frame:%d.order" % (node, frame);
    }

    method: frameOrderRedoStackName (string; string node, int frame)
    {
        "%s.frame:%d.redo" % (node, frame);
    }

    method: updateFrameDependentState (void;)
    {
        updateCurrentNode();
        undoRedoUpdate();
        //populateAnnotationList();
    }

    method: updateFrameDependentStateEvent (void; Event event)
    {
        event.reject();
        if (!isPlaying()) updateFrameDependentState();
    }

    method: deleteStroke (void; string node, string strokeName)
    {
        let n = "%s.%s" % (node, strokeName),
            props = ["%s.color" % n,
                     "%s.width" % n,
                     "%s.brush" % n,
                     "%s.debug" % n,
                     "%s.join" % n,
                     "%s.cap" % n,
                     "%s.points" % n,
                     "%s.startFrame" % n,
                     "%s.duration" % n
                    ];

        beginCompoundStateChange();

        for_each (p; props)
        {
            try
            {
                deleteProperty(p);
            }
            catch (...)
            {
                ; // just absorb it if its already deleted
            }
        }

        endCompoundStateChange();
    }

    method: newStroke (string;
                       string node,
                       int frame,
                       Color color,
                       float width,
                       string brush,
                       int join,
                       int cap,
                       int startFrame,
                       int duration,
                       int mode=RenderOverMode,
                       int debug=0)
    {
        let n          = newUniqueName(node, "pen", frame),
            colorName  = "%s.color" % n,
            widthName  = "%s.width" % n,
            brushName  = "%s.brush" % n,
            debugName  = "%s.debug" % n,
            joinName   = "%s.join" % n,
            capName    = "%s.cap" % n,
            pointsName = "%s.points" % n,
            splatName  = "%s.splat" % n,
            startFrameName = "%s.startFrame" % n,
            durationName = "%s.duration" % n,
            orderName  = frameOrderName(node, frame);

        beginCompoundStateChange();
        newProperty(colorName, FloatType, 4);
        newProperty(widthName, FloatType, 1);
        newProperty(brushName, StringType, 1);
        newProperty(pointsName, FloatType, 2);
        newProperty(debugName, IntType, 1);
        newProperty(joinName, IntType, 1);
        newProperty(capName, IntType, 1);
        newProperty(splatName, IntType, 1);
        newProperty(startFrameName, IntType, 1);
        newProperty(durationName, IntType, 1);

        if (mode != RenderOverMode)
        {
            let modeName = "%s.mode" % n;
            newProperty(modeName, IntType, 1);
            setIntProperty(modeName, int[] {mode}, true);
        }

        bool useWidth = false;

        case (_currentDrawMode.pressureMode)
        {
            Size -> { useWidth = true; }
            _ -> { useWidth = false; }
        }

        // Since this would be the first entry for width on this newly
        // created paint command it doesn't matter if we set the width
        // or insert the width into the first and only entry thus far.
        // Therefore we choose to insert because otherwise the width
        // will never be initialized over a sync session where attempts
        // to set the width (or points) are filtered out to preserve
        // bandwidth
        if (!useWidth) insertFloatProperty(widthName, float[] {width});

        setFloatProperty(colorName, float[] {color[0], color[1], color[2], color[3]}, true);
        setStringProperty(brushName, string[] {brush}, true);
        setIntProperty(joinName, int[] {join}, true);
        setIntProperty(capName, int[] {cap}, true);
        setIntProperty(debugName, int[] {debug}, true);
        setIntProperty(splatName, int[] {if brush == "gauss" then 1 else 0}, true);
        setIntProperty(startFrameName, int[] {startFrame}, true);
        setIntProperty(durationName, int[] {duration}, true);

        try
        {
            propertyInfo(orderName);
        }
        catch (...)
        {
            newProperty(orderName, StringType, 1);
        }

        insertStringProperty(orderName, string[] {n.split(".").back()});

        endCompoundStateChange();
        n;
    }

    method: addToStroke (void; Point p)
    {
        let name = "%s.points" % _currentDrawObject,
            array = getFloatProperty(name),
            s = array.size();

        if (array.size() > 1 &&
            array.back() == p.y &&
            array[array.size()-2] == p.x)
        {
            ;
        }
        else
        {
            insertFloatProperty(name, float[] {p.x, p.y});
            redraw();
        }
    }

    method: addToStrokeWithWidth (void; Point p, float width)
    {
        let pname = "%s.points" % _currentDrawObject,
            wname = "%s.width" % _currentDrawObject;

        beginCompoundStateChange();
        insertFloatProperty(pname, float[] {p.x, p.y});
        insertFloatProperty(wname, float[] {width});
        endCompoundStateChange();
        redraw();
    }

    method: newText (string;
                     string node,
                     int frame,
                     Point pos,
                     Color color,
                     float size,
                     float scale,
                     float rot,
                     int startFrame,
                     int duration,
                     string font = "",
                     string origin = "",
                     int mode=RenderOverMode,
                     int debug=0)
    {
        let n           = newUniqueName(node, "text", frame),
            colorName   = "%s.color" % n,
            sizeName    = "%s.size" % n,
            scaleName   = "%s.scale" % n,
            spacingName = "%s.spacing" % n,
            rotName     = "%s.rotation" % n,
            fontName    = "%s.font" % n,
            textName    = "%s.text" % n,
            originName  = "%s.origin" % n,
            posName     = "%s.position" % n,
            debugName   = "%s.debug" % n,
            startFrameName = "%s.startFrame" % n,
            durationName = "%s.duration" % n,
            orderName   = frameOrderName(node, frame);

        beginCompoundStateChange();
        newProperty(posName, FloatType, 2);
        newProperty(colorName, FloatType, 4);
        newProperty(spacingName, FloatType, 1);
        newProperty(sizeName, FloatType, 1);
        newProperty(scaleName, FloatType, 1);
        newProperty(rotName, FloatType, 1);
        newProperty(fontName, StringType, 1);
        newProperty(textName, StringType, 1);
        newProperty(originName, StringType, 1);
        newProperty(debugName, IntType, 1);
        newProperty(startFrameName, IntType, 1);
        newProperty(durationName, IntType, 1);

        if (mode != RenderOverMode)
        {
            let modeName = "%s.mode" % n;
            newProperty(modeName, IntType, 1);
            setIntProperty(modeName, int[] {mode}, true);
        }

        setFloatProperty(posName, float[] {pos.x, pos.y}, true);
        setFloatProperty(colorName, float[] {color[0], color[1], color[2], color[3]}, true);
        setFloatProperty(sizeName, float[] {size}, true);
        setFloatProperty(scaleName, float[] {scale}, true);
        setFloatProperty(rotName, float[] {rot}, true);
        setFloatProperty(spacingName, float[] {0.8}, true);
        setStringProperty(fontName, string[] {font}, true);
        setStringProperty(textName, string[] {""}, true);
        setStringProperty(originName, string[] {origin}, true);
        setIntProperty(debugName, int[] {debug}, true);
        setIntProperty(startFrameName, int[] {startFrame}, true);
        setIntProperty(durationName, int[] {duration}, true);

        try
        {
            propertyInfo(orderName);
        }
        catch (...)
        {
            newProperty(orderName, StringType, 1);
        }

        insertStringProperty(orderName, string[] {n.split(".").back()});

        endCompoundStateChange();
        n;
    }

    method: setText (void; char[] s)
    {
        let name = "%s.text" % _currentDrawObject,
            value = "%s" % s;

        set(name, value);
        redraw();
    }

    method: saveAnnotation (void; string description)
    {
        let comp       = "annotation:%d" % nextAnnotationID(),
            f          = frame(),
            orderName  = frameOrderName(_currentNode, f),
            redoName   = frameOrderRedoStackName(_currentNode, f),
            orderArray = getStringProperty(orderName),
            propName   = _currentNode + "." + comp + ".order",
            descName   = _currentNode + "." + comp + ".description";

        beginCompoundStateChange();
        newProperty(propName, StringType, 1);
        newProperty(descName, StringType, 1);
        setStringProperty(propName, orderArray, true);
        setStringProperty(descName, string[] {description}, true);
        endCompoundStateChange();

        populateAnnotationList();
    }

    //----------------------------------------------------------------------

    method: pointerLocation ((string, Point); Event event)
    {
        try
        {
            State state = data();
            recordPixelInfo(event);

            let pinfo = imagesAtPixel(event.pointer(), "annotate").front(),
                name  = pinfo.name,
                devicePixelRatio = devicePixelRatio(),
                ip    = event.pointer()*devicePixelRatio;

            _pointer = ip;
            _pointerRadius = mag(imageToEventSpace(name, ip, true)
                                 - imageToEventSpace(name, ip + Point(0,_currentDrawMode.size), true));
            return (name, ip);
        }
        catch (...)
        {
            return ("", Point(0,0));
        }
    }

    method: penPush (void; Event event)
    {
        updateCurrentNode();
        _currentDrawMode = _currentDrawMode.penMode;
        updateDrawModeUI();
        push(event);
    }

    method: eraserPush (void; Event event)
    {
        updateCurrentNode();

        // Save the current draw mode so we can restore it when we are done 
        // erasing as long as it is not the eraser mode itself which could
        // happen when pushing the eraser more than once
        if (_currentDrawMode.name != _currentDrawMode.eraserMode.name)
        {
            _currentDrawMode.eraserMode.penMode = _currentDrawMode;
        }

        _currentDrawMode = _currentDrawMode.eraserMode;
        updateDrawModeUI();
        push(event);
    }

    method: dropperSample (void; Event event)
    {
        State state = data();
        recordPixelInfo(event);
        if (state.pixelInfo eq nil || state.pixelInfo.empty()) return;

        let pinfo  = state.pixelInfo.front(),
            sName  = sourceNameWithoutFrame(pinfo.name),
            ip     = state.pointerPosition*devicePixelRatio();

        let pixels = framebufferPixelValue(ip.x, ip.y);
        let c = Color(pixels[0], pixels[1], pixels[2], pixels[3]);
        _sampleColor += c;
        _sampleCount++;
        setColor(_sampleColor / float(_sampleCount));
        _activeSampleColor = true;
        //let v = sourcePixelValue(sName, pinfo.px, pinfo.py);
        //setColor(v);
    }

    method: dropperStartSample (void; Event event)
    {
        updateCurrentNode();
        _sampleColor = Color(0,0,0,0);
        _sampleCount = 0;
        dropperSample(event);
    }

    method: ignoreKeyUp (void; Event event)
    {
        if (!_textPlacementMode)
        {
            event.reject();
        }
    }


    method: insertChar (void; Event event)
    {
        if (_textPlacementMode)
        {
            try
            {
                let c = char(event.key());
                _textBuffer.back() = c;
                _textBuffer.push_back(_cursorChar);
                setText(_textBuffer);
            }
            catch (exception exc)
            {
                print("insertChar CAUGHT: %s, event = %s, key = 0x%x\n" % (exc, event.name(), event.key()));
            }
        }
        else
        {
            event.reject();
        }
    }

    method: startTextPlacement (void; Event event)
    {
        updateCurrentNode();
        let (name, ip) = pointerLocation(event),
            d = _currentDrawMode;

        if (name == "") return;

        if (_textPlacementMode) commitTextInternal();

        try
        {
            let pei = eventToImageSpace(name, ip, true);

            _currentDrawObject = newText(_currentNode,
                                         _currentNodeInfo.frame, pei,
                                         d.color,
                                         d.size,
                                         1.0 / (if _scaleBrush then scale() else 1.0),
                                         0.0,
                                         _currentNodeInfo.frame,
                                         d.duration,
                                         "", "", d.renderMode, _debug);
        }
        catch (exception exc)
        {
            print("annotate_mode: exception = %s\n" % exc);
        }
        catch (...)
        {
            print("annotate_mode: UNCAUGHT EXCEPTION\n");
        }

        _textPlacementMode = true;
        _textBuffer = char[]();
        _textBuffer.push_back(_cursorChar);
        setText(_textBuffer);
    }

    method: commitTextInternal (void;)
    {
        if (_textPlacementMode && !_textBuffer.empty())
        {
            _textBuffer.pop_back();
            setText(_textBuffer);
        }

        _textPlacementMode = false;
    }

    method: commitText (void; bool reject, Event event)
    {
        commitTextInternal();
        if (reject) event.reject();
    }

    method: backwardDeleteChar (void; Event event)
    {
        if (_textPlacementMode && _textBuffer.size() > 1)
        {
            _textBuffer.pop_back();
            _textBuffer.pop_back();
            _textBuffer.push_back(_cursorChar);
            setText(_textBuffer);
        }
    }

    method: zapToChar (bool; char zapChar)
    {
        if (_textPlacementMode && !_textBuffer.empty())
        {
            if (!_textBuffer.empty()) _textBuffer.pop_back();

            while (!_textBuffer.empty())
            {
                _textBuffer.pop_back();
                if (!_textBuffer.empty() && _textBuffer.back() == zapChar) break;
            }

            _textBuffer.push_back(_cursorChar);
            setText(_textBuffer);
            return true;
        }
        else
        {
            return false;
        }
    }

    method: killLine (void; Event event)
    {
        if (!zapToChar('\n')) event.reject();
    }

    method: backwardsKillWord (void; Event event)
    {
        if (!zapToChar(' ')) event.reject();
    }

    method: insertNL (void; Event event)
    {
        if (_textPlacementMode)
        {
            _textBuffer.pop_back();
            _textBuffer.push_back('\n');
            _textBuffer.push_back(_cursorChar);
            setText(_textBuffer);
        }
        else
        {
            event.reject();
        }
    }

    method: push (void; Event event)
    {
        if (filterLiveReviewEvents()) {
            sendInternalEvent("live-review-blocked-event");
            return;
        }
        runtime.gc.disable();

        // Stop playback if we were playing.
        // First, because it makes sense duh, but also setFrame should ensure 
        // that all RV clients are sync'd at the same frame if they're in a 
        // review session.
        if (isPlaying()) {
            stop();
            setFrame(frame());
        }

        updateCurrentNode();
        let (name, ip) = pointerLocation(event),
            d = _currentDrawMode;

        if (name == "") return;

        if (_syncWholeStrokes) beginCompoundStateChange();

        let twidth = d.size / (if _scaleBrush then scale() else 1.0),
            incolor = if d.colorFunction eq nil then d.color else d.colorFunction(d.color);

        try
        {
            _currentDrawObject = newStroke(_currentNode,
                                           _currentNodeInfo.frame,
                                           incolor, twidth, d.brushName,
                                           d.join, d.cap, _currentNodeInfo.frame, d.duration, d.renderMode, _debug);
        }
        catch (exception exc)
        {
            print("annotate_mode: exception = %s\n" % exc);
        }
        catch (...)
        {
            print("annotate_mode: UNCAUGHT EXCEPTION\n");
        }

        // Init vars for drag filtering
        _dragLastPointer = ip;
        _dragLastMsec = int(QDateTime.currentMSecsSinceEpoch());

        let pei = eventToImageSpace(name, ip, true);

        bool useWidth = false;

        case (_currentDrawMode.pressureMode)
        {
            Size -> { useWidth = true;}
            _ -> { useWidth = false; }
        }

        if (useWidth)
        {
            let p = event.pressure(),
                p2 = math_util.clamp(p, .01, 1.0),
                twidth = float(_currentDrawMode.size) / (if _scaleBrush then scale() else 1.0);

            addToStrokeWithWidth(pei, twidth * p2);
        }
        else
        {
            addToStroke(pei);
        }
    }

    method: checkDragFilter (bool; Event event, Point ip)
    {
        let dx = abs(ip.x - _dragLastPointer.x);
        let dy = abs(ip.y - _dragLastPointer.y);
        let now = int(QDateTime.currentMSecsSinceEpoch());
        let dt = now - _dragLastMsec;

        let maxEventsPerSec = 60.0;
        let millisecFiltering = (1000.0 / maxEventsPerSec);

        if (dt < millisecFiltering) 
        {
            return false;
        }

        let maxDelta = 2;
        if ((dx <= maxDelta) && (dy <= maxDelta)) 
        {
            return false;
        }

        _dragLastMsec = now;
        _dragLastPointer = ip;

        return true;
    }

    method: drag (void; Event event)
    {
        if (filterLiveReviewEvents()) {
            sendInternalEvent("live-review-blocked-event");
            return;
        }
        let d = _currentDrawMode;
        _pointerGone = false;

        if (_currentDrawObject eq nil)
        {
            //
            //  This can happen on the mac when "clicking through"
            //
            push(event);
        }
        else
        {
            bool useWidth = false;

            case (d.pressureMode)
            {
                Size -> { useWidth = true;}
                _ -> { useWidth = false; }
            }

            let (name, ip) = pointerLocation(event);
            if (name == "") return;

            if (checkDragFilter(event, ip) == false)
                return;

            let pei = eventToImageSpace(name, ip, true);

            if (useWidth)
            {
                let p = event.pressure(),
                    p2 = math_util.clamp(p, .01, 1.0),
                    twidth = float(_currentDrawMode.size) / (if _scaleBrush then scale() else 1.0);

                addToStrokeWithWidth(pei, twidth * p2);
            }
            else
            {
                addToStroke(pei);
            }
        }
    }

    method: release (void; Event event)
    {
        runtime.gc.enable();
        drag(event);
        _currentDrawObject = nil;

        let f = _currentNodeInfo.frame,
            rprop= frameOrderRedoStackName(_currentNode, f);

        if (propertyExists(rprop) && propertyInfo(rprop).size > 0)
        {
            for_each (c; getStringProperty(rprop)) deleteStroke(_currentNode, c);
            setStringProperty(rprop, string[](), true);
        }

        if (_syncWholeStrokes) endCompoundStateChange();
        undoRedoUpdate();
        redraw();
    }

    method: maybeAutoMark (void; Event event)
    {
        event.reject();

        if (!_autoMark) return;

        let parts = event.contents().split("."),
            node = parts[0],
            prop = parts[2];

        if (nodeType(node) == "RVPaint" && prop == "nextId")
        {
            markFrame(frame(), true);
        }
    }

    method: pointerGone (void; Event event)
    {
        _pointerGone = true;
        event.reject();
    }

    method: pointerNotGone (void; Event event)
    {
        _pointerGone = false;
        event.reject();
    }

    method: toggleDebug (void; Event event)
    {
        _debug = if _debug == 1 then 0 else 1;

        if (_currentNode eq nil) return;

        beginCompoundStateChange();

        for_each (prop; properties(_currentNode))
        {
            if (regex("\\.debug").match(prop) && propertyInfo(prop).type == IntType)
            {
                setIntProperty(prop, int[] {_debug});
            }
        }

        endCompoundStateChange();

        redraw();
    }

    method: setBrushSize (void; float dist)
    {
        _currentDrawMode.size = dist;
        let v = (_currentDrawMode.size - _currentDrawMode.minSize) / (_currentDrawMode.maxSize - _currentDrawMode.minSize);
        _sizeSlider.setValue(int(v * 999.0));
    }

    method: setColor (void; Color c)
    {
        if (!_setColorLock)
        {
            _currentDrawMode.color = c;
            _setColorLock = true;
            let qc = toQColor(c);

            let css = "QPushButton { background-color: rgb(%d,%d,%d); }"
                % (int(c.x * 255), int(c.y * 255), int(c.z * 255));

            if (_colorButton neq nil) _colorButton.setStyleSheet(css);

            if (!_colorDialogLock && _colorDialog neq nil)
            {
                _colorDialog.setCurrentColor(qc);
            }

            if (_colorTriangle neq nil) _colorTriangle.setColor(qc);
            if (_opacitySlider neq nil) _opacitySlider.setValue(int(c.w * 255.0));

            if (_linkToolColors) for_each (mode; _drawModes) mode.color = c;

            _setColorLock = false;
        }
    }

    method: auxFilePath (string; string icon)
    {
        io.path.join(supportPath("annotate_mode", "annotate"), icon);
    }

    method: auxIcon (QIcon; string name)
    {
        QIcon(auxFilePath(name));
    }

    method: enter (void; Event event) { event.reject(); }
    method: leave (void; Event event) { event.reject(); }

    method: move (void; Event event)
    {
        if (_currentDrawMode.renderMode == -1) return;
        let (name, ip) = pointerLocation(event);
        event.reject();
        redraw();
    }

    method: editRadius (void; Event event)
    {
        if (_currentDrawMode.renderMode == -1) return;
        State state = data();
        recordPixelInfo(event);
        if (state.pixelInfo eq nil || state.pixelInfo.empty()) return;

        let pinfo  = state.pixelInfo.front(),
            ip     = state.pointerPosition,
            name   = pinfo.name,
            dist   = mag(eventToImageSpace(name, ip, true)
                         - eventToImageSpace(name, _pointer, true));

        setBrushSize(clamp(dist, _currentDrawMode.minSize, _currentDrawMode.maxSize));
        _pointerRadius = mag(imageToEventSpace(name, ip, true)
                         - imageToEventSpace(name, ip + Point(0,_currentDrawMode.size), true));
        redraw();
    }

    method: drawModeTable (void; string tableName)
    {
        if (_drawModeTable neq nil && _drawModeTable != "")
        {
            popEventTable(_drawModeTable);
        }

        _drawModeTable = tableName;

        if (_drawModeTable != "")
        {
            pushEventTable(_drawModeTable);
        }
    }

    method: setDrawModeSlot (void; bool checked, DrawMode d)
    {
        commitTextInternal();
        _textPlacementMode = false;
        _currentDrawMode = d;
        commands.setCursor(d.cursor);
        updateDrawModeUI();
        _toolSliderLabel.setText(if d.sliderName eq nil then "Opacity" else d.sliderName);
    }

    method: locationChangedSlot (void; Qt.DockWidgetArea area)
    {
        _dockArea = area;
    }

    method: topLevelChangedSlot (void; bool toplevel)
    {
        _topLevel = toplevel;
    }

    method: newSizeSlot (void; int value)
    {
        let v = value / 999.0,
            d = v * (_currentDrawMode.maxSize - _currentDrawMode.minSize) + _currentDrawMode.minSize;

        setBrushSize(d);
        redraw();
    }

    method: newOpacitySlot (void; int value)
    {
        let c = _currentDrawMode.color;
        setColor(Color(c.x, c.y, c.z, value / 255.0));
    }

    method: newColorSlot (void; QColor color, bool lock, bool alpha)
    {
        _colorDialogLock = lock;
        let cc = _currentDrawMode.color;

        try
        {
            Color c = fromQColor(color);
            setColor(if alpha then c else Color(c.x, c.y, c.z, cc.w));
        }
        catch (...)
        {
            print("caught exception in newColorSlot\n");
        }

        _colorDialogLock = false;
    }

    method: chooseColorSlot (void; bool checked)
    {
        _colorDialog.setCurrentColor(toQColor(_currentDrawMode.color));
        _colorDialog.setWindowModality(Qt.NonModal);
        _colorDialog.show();
    }

    method: undoRedoUpdate (void;)
    {
        let f = _currentNodeInfo.frame,
            uprop = frameOrderName(_currentNode, f),
            rprop= frameOrderRedoStackName(_currentNode, f),
            playing = isPlaying();

        _undoAct.setEnabled(!playing &&
                            propertyExists(uprop) &&
                            propertyInfo(uprop).size > 0);

        _redoAct.setEnabled(!playing &&
                            propertyExists(rprop) &&
                            propertyInfo(rprop).size > 0);
    }

    method: undoPaint (void; int frame)
    {
        let f = _currentNodeInfo.frame,
            upropName = frameOrderName(_currentNode, f),
            rpropName = frameOrderRedoStackName(_currentNode, f);

        if (propertyExists(upropName))
        {
            if (!propertyExists(rpropName)) newProperty(rpropName, StringType, 1);

            let u = getStringProperty(upropName),
                r = getStringProperty(rpropName);

            r.push_back(u.back());
            u.resize(u.size()-1);

            setStringProperty(upropName, u, true);
            setStringProperty(rpropName, r, true);
        }
    }

    method: redoPaint (void; int frame)
    {
        let f = _currentNodeInfo.frame,
            upropName = frameOrderName(_currentNode, f),
            rpropName = frameOrderRedoStackName(_currentNode, f);

        if (propertyExists(rpropName))
        {
            if (!propertyExists(upropName)) newProperty(upropName, StringType, 1);

            let u = getStringProperty(upropName),
                r = getStringProperty(rpropName);

            int startIndex = -1;
            int endIndex = -1;

            for (int i = 0; i < r.size(); i++)
            {
                if (r[i] == "start")
                {
                    startIndex = i;
                }
                if (r[i] == "end")
                {
                    endIndex = i;
                }
            }

            if (r.size() >= 3 && startIndex != -1 && endIndex != -1)
            {
                for (int i = startIndex + 1; i < endIndex; i++)
                {
                    u.push_back(r[i]);
                }
                r.erase(startIndex, endIndex - startIndex + 1);
            }
            else
            {
                u.push_back(r.back());
                r.resize(r.size() - 1);
            }

            setStringProperty(upropName, u, true);
            setStringProperty(rpropName, r, true);
        }
    }

    method: clearPaint (void; string node, int frame)
    {
        let upropName = frameOrderName(node, frame);
        let rpropName = frameOrderRedoStackName(node, frame);

        if (propertyExists(upropName))
        {
            if (!propertyExists(rpropName)) newProperty(rpropName, StringType, 1);

            let u = getStringProperty(upropName),
                r = getStringProperty(rpropName);

            if (u.size() > 0)
            {
                r.push_back("start");
                for (int i = 0; i < u.size(); i++)
                    r.push_back(u[i]);
                r.push_back("end");
                u.clear();
            }

            setStringProperty(upropName, u, true);
            setStringProperty(rpropName, r, true);
        }
    }

    method: populateAnnotationList (void;)
    {
        if (_annotationsWidget eq nil) return;

        let annotationRE = regex(".*\\.annotation:[0-9]+\\..*");

        _annotationsWidget.clear();
        string lastProp = "";

        for_each (prop; properties(_currentNode))
        {
            if (annotationRE.match(prop))
            {
                let parts = prop.split("."),
                    node = parts[0],
                    comp = parts[1],
                    name = parts[2];

                if (name == "order")
                {
                    let d = getStringProperty("%s.%s.description" % (node, comp));

                    if (d.empty() || d.front() == "")
                    {
                        _annotationsWidget.addItem(name);
                    }
                    else
                    {
                        _annotationsWidget.addItem(d.front());
                    }
                }
            }
        }
    }

    method: toggleStoreOnSrc (void; Event event)
    {
        _storeOnSrc = !_storeOnSrc;
    }

    method: storeOnSrc (int;)
    {
        return if _storeOnSrc then CheckedMenuState else UncheckedMenuState;
    }

    method: toggleAutoMark (void; Event event)
    {
        _autoMark = !_autoMark;
    }

    method: autoMark (int;)
    {
        return if _autoMark then CheckedMenuState else UncheckedMenuState;
    }

    method: toggleLinkToolColors (void; Event event)
    {
        _linkToolColors = !_linkToolColors;
        setColor(_currentDrawMode.color);
    }

    method: unlinkToolColors (int;)
    {
        return if !_linkToolColors then CheckedMenuState else UncheckedMenuState;
    }

    method: liveDrawingsEvent (void; Event event)
    {
        _syncWholeStrokes = !_syncWholeStrokes;
    }

    method: liveDrawingsState (int;)
    {
        return if _syncWholeStrokes then UncheckedMenuState else CheckedMenuState;
    }

    method: syncAutoStartEvent (void; Event event)
    {
        use SettingsValue;

        try
        {
            let StringArray extraModes = readSetting("Sync",
                                                     "extraModes",
                                                     StringArray(string[]{}));

            //
            //  Update (just in case)
            //

            _syncAutoStart = false;
            for_each (mode; extraModes) if (mode == _modeName) _syncAutoStart = true;
            string[] newModes;

            if (_syncAutoStart)
            {
                for_each (mode; extraModes) if (mode != _modeName) newModes.push_back(mode);
                _syncAutoStart = false;
            }
            else
            {
                newModes = extraModes;
                newModes.push_back(_modeName);
                _syncAutoStart = true;
            }

            writeSetting("Sync", "extraModes", StringArray(newModes));
        }
        catch (...)
        {
            print("BAD\n");
            ; // ignore settings errors
        }
    }

    method: isSyncAutoStart (int;)
    {
        return if _syncAutoStart then CheckedMenuState else UncheckedMenuState;
    }

    method: undoSlot (void; bool checked)
    {
        undoPaint(sourceFrame(frame()));
        updateFrameDependentState();
        redraw();
    }

    method: clearSlot (void; bool checked)
    {
        clearPaint(_currentNode, _currentNodeInfo.frame);
        updateFrameDependentState();
        redraw();
    }

    method: clearAllSlot (void; bool checked)
    {
        let answer = alertPanel(true, InfoAlert, "Clear all annotations from the current timeline?", nil, "OK", "Cancel", nil);

        if (answer != 0)
        {
            return;
        }
        else
        {
            for_each(node; nodes())
            {
                let annotatedFrames = findAnnotatedFrames(node);
                for_each(frame; annotatedFrames)
                {
                    clearPaint(node, frame);
                    clearPaint(node, sourceFrame(frame));
                }
            }

            updateFrameDependentState();
            redraw();
        }
    }

    method: redoSlot (void; bool checked)
    {
        redoPaint(sourceFrame(frame()));
        updateFrameDependentState();
        redraw();
    }

    method: prefsShow (void; Event event)
    {
        let w = prefTabWidget();
    }

    method: prefsHide (void; Event event) { ; }
    method: undoEvent (void; Event event) { undoSlot(true); }
    method: redoEvent (void; Event event) { redoSlot(true); }
    method: clearEvent (void; Event event) { clearSlot(true); }
    method: clearAllEvent (void; Event event) { clearAllSlot(true); }

    method: keyUndoEvent (void; Event event)
    {
        let f = _currentNodeInfo.frame,
            uprop = frameOrderName(_currentNode, f),
            rprop= frameOrderRedoStackName(_currentNode, f),
            playing = isPlaying();

        if (!playing && propertyExists(uprop) &&
            propertyInfo(uprop).size > 0)
        {
            undoSlot(true);
        }
    }

    method: keyRedoEvent (void; Event event)
    {
        let f = _currentNodeInfo.frame,
            uprop = frameOrderName(_currentNode, f),
            rprop= frameOrderRedoStackName(_currentNode, f),
            playing = isPlaying();

        if (!playing && propertyExists(rprop) &&
            propertyInfo(rprop).size > 0)
        {
            redoSlot(true);
        }
    }

    method: propOnState (int; string name)
    {
        if !propertyExists(name) || propertyInfo(name).size == 0
               then DisabledMenuState
               else UncheckedMenuState;
    }

    method: undoState (int;)
    {
        if isSessionEmpty() || _currentNode eq nil
             then DisabledMenuState
             else propOnState(frameOrderName(_currentNode, _currentNodeInfo.frame));
    }

    method: redoState (int;)
    {
        if isSessionEmpty() || _currentNode eq nil
             then DisabledMenuState
             else propOnState(frameOrderRedoStackName(_currentNode, _currentNodeInfo.frame));
    }

    method: nextPrevState (int;)
    {
        if isSessionEmpty() || _currentNode eq nil
             then DisabledMenuState
             else UncheckedMenuState;
    }

    method: isShowingDrawings (int;)
    {
        try
        {
            let pname = "%s.paint.show" % _currentNode,
                v = getIntProperty(pname).front();
            return if v == 0 then UncheckedMenuState else CheckedMenuState;
        }
        catch (...)
        {
            return DisabledMenuState;
        }
    }

    method: showDrawingsSlot (void; Event event)
    {
        try
        {
            let pname = "%s.paint.show" % _currentNode,
                v = getIntProperty(pname).front();
            setIntProperty(pname, int[] {if v == 0 then 1 else 0});
            redraw();
        }
        catch (...)
        {
            ;
        }
    }

    method: scaleBrushSlot (void; Event event)
    {
        _scaleBrush = !_scaleBrush;
    }

    method: isScalingBrush (int;)
    {
        return if _scaleBrush then CheckedMenuState else UncheckedMenuState;
    }

    method: showBrushSlot (void; Event event)
    {
        _showBrush = !_showBrush;
    }

    method: isShowingBrush (int;)
    {
        return if _showBrush then CheckedMenuState else UncheckedMenuState;
    }

    method: saveDefaults (void; Event event)
    {
        saveSettings();
    }

    method: autoSave (void; Event event)
    {
        use SettingsValue;
        _autoSave = !_autoSave;
        writeSetting("Annotate", "autoSaveEnabled", Bool(_autoSave));
    }

    method: isAutoSaveEnabled (int;)
    {
        return if _autoSave then CheckedMenuState else UncheckedMenuState;
    }

    method: saveAnnotationSlot (void;)
    {
        let (text, ok) = QInputDialog.getText(mainWindowWidget(),
                                              "Annotation Title",
                                              "Title:",
                                              QLineEdit.Normal,
                                              "annotation",
                                              Qt.Dialog);

        if (ok) saveAnnotation(text);
    }

    method: addAnnotationSlot (void; bool checked)
    {
        saveAnnotationSlot();
    }

    method: updateDrawModeUI (void;)
    {
        if (_currentDrawMode eq _selectDrawMode)
        {
            _hideDrawPane = _hideDrawPane + 1;
            if (_active) toggle();
        }
        else
        {
            if (!_active) toggle();
            _hideDrawPane = 0;
        }

        if (_activeSampleColor)
        {
            if (_sampleCount != 0) setColor(_sampleColor / float(_sampleCount));
            _activeSampleColor = false;
        }
        else
        {
            setColor(_currentDrawMode.color);
        }

        setBrushSize(_currentDrawMode.size);
        _currentDrawMode.button.setChecked(true);
        drawModeTable(_currentDrawMode.eventTable);
    }

    method: removeTags (void;)
    {
        //
        //  Remove anything, eventhough we only use one
        //

        let vnode = viewNode();

        if (vnode neq nil)
        {
            for_each (node; closestNodesOfType("RVPaint"))
            {
                let p = "%s.tag.annotate" % node;
                if (propertyExists(p)) deleteProperty(p);
            }
        }
    }

    method: setTags (void;)
    {
        let vnode = viewNode();

        if (vnode neq nil)
        {
            for_each (node; closestNodesOfType("RVPaint"))
            {
                let p = "%s.tag.annotate" % node;
                if (!propertyExists(p)) set(p, "");
            }
        }
    }

    method: nodeInputsChanged (void; Event event)
    {
        let node = event.contents(),
            vnode = viewNode();

        if (vnode neq nil && node == vnode)
        {
            removeTags();
            setTags();
        }

        updateCurrentNode();
        event.reject();
    }

    method: afterGraphViewChange (void; Event event)
    {
        updateCurrentNode();
        setTags();
        event.reject();
    }

    method: beforeGraphViewChange (void; Event event)
    {
        removeTags();
        event.reject();
    }

    method: shutdown (void; Event event)
    {
        if (_autoSave)
        {
            saveSettings();
        }
        commitTextInternal();
        event.reject();
    }

    method: nextAnnotatedFrame (void;)
    {
        //
        //  The frames are out of order and have possible duplicates so
        //  just scan the whole array for the closest next frame
        //

        let frames = findAnnotatedFrames(),
            currentFrame = frame();

        if (frames.empty()) return;
        int newFrame = frames.front();

        for_each (f; frames)
        {
            if (newFrame <= currentFrame || (f > currentFrame && f < newFrame)) newFrame = f;
        }

        setFrame(newFrame);
    }

    method: prevAnnotatedFrame (void;)
    {
        let frames = findAnnotatedFrames(),
            currentFrame = frame();

        if (frames.empty()) return;
        int newFrame = frames.front();

        for_each (f; frames)
        {
            if (f < currentFrame && (newFrame >= currentFrame || f > newFrame)) newFrame = f;
        }

        setFrame(newFrame);
    }

    method: nextEvent (void; Event event) { nextAnnotatedFrame(); }
    method: prevEvent (void; Event event) { prevAnnotatedFrame(); }
    method: nextSlot (void; bool b) { nextAnnotatedFrame(); }
    method: prevSlot (void; bool b) { prevAnnotatedFrame(); }

    method: onPresenterChanged(void; Event event)
    {
        if (_active && filterLiveReviewEvents())
        {
            toggle();
        }
        event.reject();
    }

    method: AnnotateMinorMode (AnnotateMinorMode; string name)
    {
        _textPlacementMode = false;
        _machine           = "computer";
        _user              = remoteLocalContactName();
        _setColorLock      = false;
        _activeSampleColor = false;
        _sampleCount       = 0;
        _showBrush         = true;
        _scaleBrush        = true;
        _pointerGone       = false;
        _syncAutoStart     = false;
        _cursorChar        = char(0x1);
        _autoSave          = true;
        _hideDrawPane      = 0;

        let m = mainWindowWidget(),
            g = QActionGroup(m);

        _colorDialog = QColorDialog(m);
        _colorDialog.setOption(QColorDialog.ShowAlphaChannel, true);
        _colorDialog.setOption(QColorDialog.DontUseNativeDialog, true);

        try
        {
            let SettingsValue.Bool tl = readSetting("Annotate",
                                                    "topLevel",
                                                    SettingsValue.Bool(true));
            _topLevel = tl;
        }
        catch (...)
        {
            ; // ignore
        }

        _drawDock   = DrawDockWidget(m,this);
        _drawPane   = loadUIFile(auxFilePath("drawpane.ui"), m);


        try
        {
            _colorButton       = _drawPane.findChild("colorButton");

            _disabledButton    = _drawPane.findChild("disabledButton");
            _dropperButton     = _drawPane.findChild("dropperButton");
            _textButton        = _drawPane.findChild("textButton");
            _circleBrushButton = _drawPane.findChild("circleButton");
            _gaussBrushButton  = _drawPane.findChild("gaussButton");
            _hardEraseButton   = _drawPane.findChild("hardEraseButton");
            _softEraseButton   = _drawPane.findChild("softEraseButton");
            _dodgeButton       = _drawPane.findChild("dodgeButton");
            _burnButton        = _drawPane.findChild("burnButton");
            _cloneButton       = _drawPane.findChild("cloneButton");
            _smudgeButton      = _drawPane.findChild("smudgeButton");

            _sizeSlider        = _drawPane.findChild("sizeSlider");
            _undoButton        = _drawPane.findChild("undoButton");
            _redoButton        = _drawPane.findChild("redoButton");
            _clearButton       = _drawPane.findChild("clearButton");
            _opacitySlider     = _drawPane.findChild("opacitySlider");
            _toolSliderLabel   = _drawPane.findChild("toolSliderLabel");
        }
        catch (exception exc)
        {
            print("ERROR: ui file exception: %s\n" % exc);
        }

        //
        //   XXX hide clone / smudge for now (no icons)
        //
        _cloneButton.hide();
        _smudgeButton.hide();

        QGroupBox cbase    = _drawPane.findChild("colorGroup");

        PressureMode defaultPMode = if (system.getenv("RV_NO_ANNOTATION_PRESSURE_SUPPORT", nil) neq nil) then
            PressureMode.None else PressureMode.Size;

        _selectDrawMode = DrawMode { "Select",
                                     "select",
                                     _disabledButton,
                                     g.addAction(auxIcon("arrow_64x64.png"), "Select/Scrub Time (Draw Disabled)"),
                                     "",
                                     CursorDefault,
                                     0,
                                     Color(0,0,0,1),
                                     RenderOverMode,
                                     "",
                                     RoundJoin,
                                     SquareCap,
                                     1,
                                     1,
                                     0.024, 0.001,
                                     PressureMode.None };

        _dropperDrawMode = DrawMode { "Sample Color",
                                      "sample",
                                     _dropperButton,
                                     g.addAction(auxIcon("dropper_48x48.png"), "Sample Color"),
                                     "dropper",
                                     Qt.CrossCursor,
                                     0,
                                     Color(0,0,0,1),
                                     RenderOverMode,
                                     "",
                                     RoundJoin,
                                     SquareCap,
                                     0.024, 0.001,
                                     1,
                                     1,
                                     PressureMode.None };

        _textDrawMode  = DrawMode { "Text",
                                      "text",
                                     _textButton,
                                     g.addAction(auxIcon("A_48x48.png"), "Text"),
                                     "textplacement",
                                     Qt.IBeamCursor,
                                     0,
                                     Color(0,0,0,1),
                                     RenderOverMode,
                                     "",
                                     RoundJoin,
                                     SquareCap,
                                     0.01, 0.0015,
                                     1,
                                     1,
                                     PressureMode.None };

        _penDrawMode = DrawMode { "Pen",
                                  "pen",
                                  _circleBrushButton,
                                  g.addAction(auxIcon("circle_64x64.png"), "Pen"),
                                  "",
                                  Qt.CrossCursor,
                                  0.003,
                                  Color(1, 1, 0, 1),
                                  RenderOverMode,
                                  "circle",
                                  RoundJoin,
                                  SquareCap,
                                  0.024, 0.001,
                                  1,
                                  1,
                                  defaultPMode };


        _airBrushDrawMode = DrawMode { "Air Brush",
                                       "airbrush",
                                       _gaussBrushButton,
                                       g.addAction(auxIcon("gauss_64x64.png"), "Air Brush"),
                                       "",
                                       Qt.CrossCursor,
                                       0.025,
                                       Color(0, 1, 0, .15),
                                       RenderOverMode,
                                       "gauss",
                                       RoundJoin,
                                       SquareCap,
                                       0.044, 0.001,
                                       1,
                                       1,
                                       defaultPMode };


        _hardEraseDrawMode = DrawMode { "Hard Erase",
                                        "harderase",
                                        _hardEraseButton,
                                        g.addAction(auxIcon("harderase_64x64.png"),
                                                    "Hard Erase"),
                                        "",
                                        Qt.CrossCursor,
                                        0.003,
                                        Color(0, 0, 0, 1),
                                        RenderEraseMode,
                                        "circle",
                                        RoundJoin,
                                        SquareCap,
                                        0.024, 0.001,
                                        1,
                                        1,
                                        PressureMode.None };

        _softEraseDrawMode = DrawMode { "Air Brush Erase",
                                        "airerase",
                                        _softEraseButton,
                                        g.addAction(auxIcon("softerase_64x64.png"),
                                                    "Air Brush Erase"),
                                        "",
                                        Qt.CrossCursor,
                                        0.025,
                                        Color(0, 0, 0, .05),
                                        RenderEraseMode,
                                        "gauss",
                                        RoundJoin,
                                        SquareCap,
                                        0.044, 0.001,
                                        1,
                                        1,
                                        PressureMode.None };

        _dodgeDrawMode     = DrawMode { "Dodge",
                                        "dodge",
                                        _dodgeButton,
                                        g.addAction(auxIcon("dodge_64x64.png"), "Dodge"),
                                        "",
                                        Qt.CrossCursor,
                                        1.0,
                                        Color(1, 1, 1, .05),
                                        RenderScaleMode,
                                        "gauss",
                                        RoundJoin,
                                        SquareCap,
                                        0.044, 0.001,
                                        1,
                                        1,
                                        PressureMode.None,
                                        _hardEraseDrawMode,
                                        nil,
                                        "Intensity",
                                        dodgeFunc };

        _burnDrawMode     = DrawMode { "Burn",
                                        "burn",
                                        _burnButton,
                                        g.addAction(auxIcon("burn_64x64.png"), "Burn"),
                                        "",
                                        Qt.CrossCursor,
                                        1.0,
                                        Color(1, 1, 1, .05),
                                        RenderScaleMode,
                                        "gauss",
                                        RoundJoin,
                                        SquareCap,
                                        0.044, 0.001,
                                        1,
                                        1,
                                        PressureMode.None,
                                        _hardEraseDrawMode,
                                        nil,
                                        "Intensity",
                                        burnFunc };

        _cloneDrawMode    = DrawMode { "Clone",
                                        "clone",
                                        _cloneButton,
                                        g.addAction(auxIcon("clone_64x64.png"), "Clone"),
                                        "",
                                        Qt.CrossCursor,
                                        0.025,
                                        Color(0, 0, 0, .05),
                                        RenderAddMode,
                                        "circle",
                                        RoundJoin,
                                        SquareCap,
                                        0.044, 0.001,
                                        1,
                                        1,
                                        PressureMode.None };

        _smudgeDrawMode   = DrawMode { "Smudge",
                                        "smudge",
                                        _smudgeButton,
                                        g.addAction(auxIcon("smudge_64x64.png"), "Smudge"),
                                        "",
                                        Qt.CrossCursor,
                                        0.025,
                                        Color(0, 0, 0, .05),
                                        RenderAddMode,
                                        "circle",
                                        RoundJoin,
                                        SquareCap,
                                        0.044, 0.001,
                                        1,
                                        1,
                                        PressureMode.None };

        _drawModes = DrawMode[] { _selectDrawMode, _penDrawMode, _airBrushDrawMode,
                                  _textDrawMode, _dropperDrawMode, _hardEraseDrawMode,
                                  _softEraseDrawMode, _dodgeDrawMode, _burnDrawMode,
                                  _cloneDrawMode, _smudgeDrawMode };

        //
        //  Load the settings
        //

        try
        {
            loadSettings();
        }
        catch (...)
        {
            ; // ignore failure
        }

        //
        //  Do some sanity checks on the settings
        //

        if (_dockArea != Qt.LeftDockWidgetArea && _dockArea != Qt.RightDockWidgetArea)
        {
            _dockArea = Qt.LeftDockWidgetArea;
        }

        if (_currentDrawMode eq nil) _currentDrawMode = _penDrawMode;

        for_each (d; _drawModes)
        {
            d.action.setCheckable(true);
            connect(d.action, QAction.triggered, setDrawModeSlot(,d));
            d.button.setDefaultAction(d.action);
            d.eraserMode = d;
            d.penMode = d;
        }

        //
        //  For when the stylus is flipped over
        //

        _penDrawMode.eraserMode      = _hardEraseDrawMode;
        _airBrushDrawMode.eraserMode = _softEraseDrawMode;
        _dodgeDrawMode.eraserMode    = _softEraseDrawMode;
        _burnDrawMode.eraserMode     = _softEraseDrawMode;

        _opacitySlider.setMaximum(255);
        _opacitySlider.setMinimum(0);
        _sizeSlider.setMaximum(999);
        _sizeSlider.setMinimum(0);

        // not finding the specific dynamic cast so we have to force
        // the layout to a QObject first
        QObject lobj = cbase.layout();
        QVBoxLayout lbox = lobj;
        _colorTriangle = QtColorTriangle(m);
        _colorTriangle.setObjectName("colorTriangle");
        _colorTriangle.setMinimumSize(QSize(85,85));
        lbox.insertWidget(0, _colorTriangle, 0, 0);
        _colorTriangle.setMaximumHeight(100);

        connect(_colorTriangle, QtColorTriangle.colorChanged, newColorSlot(,false,false));
        connect(_opacitySlider, QAbstractSlider.valueChanged, newOpacitySlot);
        connect(_sizeSlider, QAbstractSlider.valueChanged, newSizeSlot);
        connect(_drawDock, QDockWidget.dockLocationChanged, locationChangedSlot);
        connect(_drawDock, QDockWidget.topLevelChanged, topLevelChangedSlot);

        connect(_colorButton, QPushButton.clicked, chooseColorSlot);
        connect(_colorDialog, QColorDialog.currentColorChanged, newColorSlot(,true,true));

        _undoAct = QAction(auxIcon("undo_64x64.png"), "Undo", m);
        _redoAct = QAction(auxIcon("redo_64x64.png"), "Redo", m);
        _clearAct = QAction(auxIcon("clear_64x64.png"), "Clear Frame", m);

        // We use QKeySequence::StandardKey instead of the string constructor for better MacOS support
        _undoAct.setShortcut(QKeySequence(QKeySequence.Undo));
        _redoAct.setShortcut(QKeySequence(QKeySequence.Redo));

        _undoAct.setShortcutContext(Qt.ApplicationShortcut);
        _redoAct.setShortcutContext(Qt.ApplicationShortcut);

        connect(_undoAct, QAction.triggered, undoSlot);
        connect(_redoAct, QAction.triggered, redoSlot);

        _undoButton.setDefaultAction(_undoAct);
        _redoButton.setDefaultAction(_redoAct);
        _clearButton.setDefaultAction(_clearAct);

        let clearMenu = QMenu("Clear Frame", _clearButton);
        let clearFrame = clearMenu.addAction("Clear Frame");
        let clearAllFrames = clearMenu.addAction("Clear All Frames on Timeline");

        _clearButton.setMenu(clearMenu);
        _clearButton.setPopupMode(QToolButton.InstantPopup);

        connect(clearFrame, QAction.triggered, clearSlot);
        connect(clearAllFrames, QAction.triggered, clearAllSlot);
        _clearButton.setStyleSheet("QToolButton::menu-indicator { subcontrol-position: bottom right; top: -2px; }");

        _drawDock.setWidget(_drawPane);
        _drawDock.ensurePolished();
        m.addDockWidget(_dockArea, _drawDock);
        _drawDock.setGeometry(QRect(m.x() + 60,
                                    m.y() + 60,
                                    _drawDock.width(),
                                    _drawDock.height()));

        _drawDock.setFloating(_topLevel);

        _drawDock.show();

        //
        //  Force update of UI elements
        //

        setColor(_currentDrawMode.color);
        setBrushSize(_currentDrawMode.size);
        _currentDrawMode.button.setChecked(true);

        //
        //  Call mode init
        //

        init(name,
             nil,
             [("pointer-1--drag", drag, "Add to current stroke"),
              ("pointer-1--push", push, "Start New Stroke"),
              ("pointer-1--release", release, "End Current Stroke"),
              ("pointer--move", move, ""),
              ("pointer--shift--move", editRadius, ""),
              ("pointer--enter", enter, ""),
              ("pointer--leave", leave, ""),
              ("before-session-deletion", shutdown, ""),
              ("graph-node-inputs-changed", nodeInputsChanged, "Update UI"),
              ("before-graph-view-change", beforeGraphViewChange, "Update UI"),
              ("after-graph-view-change", afterGraphViewChange, "Update UI"),
              ("frame-changed", updateFrameDependentStateEvent, ""),
              ("play-stop", updateFrameDependentStateEvent, ""),
              ("stylus-pen--push", penPush, "Pen Down"),
              ("stylus-pen--drag", drag, "Pen Drag"),
              ("stylus-pen--shift--move", editRadius, ""),
              ("stylus-pen--move", move, "Pen Move"),
              ("stylus-pen--release", release, "Pen Release"),
              ("stylus-eraser--push", eraserPush, "Pen Down"),
              ("stylus-eraser--drag", drag, "Pen Drag"),
              ("stylus-eraser--move", move, "Pen Move"),
              ("stylus-eraser--shift--move", editRadius, ""),
              ("stylus-eraser--release", release, "Pen Release"),
              ("key-down--d", toggleDebug, "Toggle debug display"),
              ("pointer--leave", pointerGone, "Pointer Gone"),
              ("pointer--enter", pointerNotGone, "Pointer Back"),
              ("graph-state-change", maybeAutoMark, "Possibly Auto-mark Frames"),
              ("key-down--meta-shift--right", nextEvent, "Next Annotated Frame"),
              ("key-down--meta-shift--left", prevEvent, "Previous Annotated Frame"),
              ("key-down--alt-shift--right", nextEvent, "Next Annotated Frame"),
              ("key-down--alt-shift--left", prevEvent, "Previous Annotated Frame"),
              ("internal-sync-presenter-changed", onPresenterChanged, "Live Review Presenter Changed")
              //("key-down--control--z", keyUndoEvent, "Undo"),
              //("key-down--control--Z", keyRedoEvent, "Redo"),
              //("preferences-show", prefsShow, "Configure Preferences"),
              //("preferences-hide", prefsHide, "Write Preferences"),
              ],
             Menu {
                 {"Annotation", Menu {
                     {"Actions on Current Frame", nil, nil, inactiveState},
                     {"   Undo", undoEvent, nil, undoState},
                     {"   Redo", redoEvent, nil, redoState},
                     {"   Clear Drawings", clearEvent, nil, undoState},
                     {"_", nil, nil, nil},
                     {"Actions on Timeline", nil, nil, inactiveState},
                     {"   Clear All Drawings", clearAllEvent, nil, undoState},
                     {"   Show Drawings", showDrawingsSlot, nil, isShowingDrawings},
                     {"   Next Annotated Frame", nextEvent, "alt shift right", nextPrevState},
                     {"   Previous Annotated Frame", prevEvent, "alt shift left", nextPrevState},
                     {"_", nil, nil, nil},
                     {"Configure", Menu {
                         {"Show Brush", showBrushSlot, nil, isShowingBrush},
                         {"Brush Size Relative to View", scaleBrushSlot, nil, isScalingBrush},
                         {"_", nil, nil, nil},
                         {"Draw On Source When Possible", toggleStoreOnSrc, nil, storeOnSrc},
                         {"Automatically Mark Annotated Frames", toggleAutoMark, nil, autoMark},
                         {"Unique Color For Each Tool", toggleLinkToolColors, nil, unlinkToolColors},
                         {"_", nil, nil, nil},
                         {"Live Drawing in Sync", liveDrawingsEvent, nil, liveDrawingsState},
                         {"Start Automatically During Sync", syncAutoStartEvent, nil, isSyncAutoStart},
                         {"_", nil, nil, nil},
                         {"Save Current Settings as Defaults", saveDefaults, nil, nil},
                         {"Always Save Settings as Defaults On Exit", autoSave, nil, isAutoSaveEnabled}
                     }}
             }}},
             "z"
             );

        defineEventTable("dropper",
                         [("pointer-1--push", dropperStartSample, "Sample Color"),
                          ("pointer-1--drag", dropperSample, "Sample Color"),
                          ("pointer-1--release", dropperSample, "Sample Color"),
                          ("pointer--shift--move", noop, ""),
                          ("stylus-pen--push", dropperStartSample, "Sample Color"),
                          ("stylus-pen--drag", dropperSample, "Sample Color"),
                          ("stylus-pen--shift--move", dropperSample, "Sample Color"),
                          ("stylus-pen--move", noop, ""),
                          ("stylus-pen--release", noop, ""),
                          ("stylus-eraser--push", noop, ""),
                          ("stylus-eraser--drag", noop, ""),
                          ("stylus-eraser--move", noop, ""),
                          ("stylus-eraser--shift--move", noop, ""),
                          ("stylus-eraser--release", noop, "")
                          ]);

        defineEventTable("textplacement",
                         [("pointer-1--push", startTextPlacement, "Start Placing Text"),
                          ("pointer-1--drag", noop, ""),
                          ("pointer-1--release", noop, ""),
                          ("pointer--shift--move", noop, ""),
                          ("stylus-pen--push", startTextPlacement, "Start Placing Text"),
                          ("stylus-pen--drag", noop, ""),
                          ("stylus-pen--shift--move", noop, ""),
                          ("stylus-pen--move", noop, ""),
                          ("stylus-pen--release", noop, ""),
                          ("stylus-eraser--push", noop, ""),
                          ("stylus-eraser--drag", noop, ""),
                          ("stylus-eraser--move", noop, ""),
                          ("stylus-eraser--shift--move", noop, ""),
                          ("stylus-eraser--release", noop, ""),
                          ("frame-changed", commitText(true,), ""),
                          ("before-session-write", commitText(true,), ""),
                          ("before-session-write-copy", commitText(true,), ""),
                          ("before-play-start", commitText(true,), ""),
                          ("before-session-read", commitText(true,), ""),
                          ("before-graph-view-changed", commitText(true,), ""),
                          ("before-session-deletion", shutdown, ""),
                          ("before-progressive-loading", commitText(true,), ""),
                          ("before-node-delete", commitText(true,), ""),
                          ("key-down--backspace", backwardDeleteChar, "Back Char"),
                          ("key-down--delete", backwardDeleteChar, "Back Char"),
                          ("key-down--control--a", killLine, ""),
                          ("key-down--meta--backspace", backwardsKillWord, ""),
                          ("key-down--alt--backspace", backwardsKillWord, ""),
                          ("key-down--shift--backspace", backwardsKillWord, ""),
                          ("key-down--control--backspace", backwardsKillWord, ""),
                          ("key-down--meta--a", killLine, ""),
                          ("key-down--alt--a", killLine, ""),
                          ("key-down--space", insertChar, ""),
                          ("key-down--enter", insertNL, ""),
                          ("key-down--control--enter", commitText(false,), ""),
                          ("key-down--meta--enter", commitText(false,), ""),
                          ("key-down--alt--enter", commitText(false,), ""),
                          ("key-down--escape", commitText(false,), "Stop Placing Text"),
                          ("key-down--keypad-enter", commitText(false,), "Stop Placing Text"),
                          ]);

        defineEventTableRegex("textplacement",
                              [("^key-down--.$", insertChar, ""),
                               ("^key-down--shift--.$", insertChar, ""),
                               ("^key-up--.$", ignoreKeyUp, ""),
                               ("^key-up--.$--.$", ignoreKeyUp, "")]
                              );

        //
        //  Initialize the sync setting (has to be done after calling init)
        //


        try
        {
            use SettingsValue;
            let StringArray extraModes = readSetting("Sync",
                                                     "extraModes",
                                                     StringArray(string[]{}));

            for_each (mode; extraModes) if (mode == _modeName) _syncAutoStart = true;
            let String name = readSetting("Annotate", "drawmode", String("Pen"));
        }
        catch (...)
        {
            ; // ignore
        }
    }

    method: deactivate (void;)
    {
        if (_hideDrawPane != 1)
        {
            if (_manageDock neq nil) _manageDock.hide();
            if (_drawDock neq nil) _drawDock.hide();
            _hideDrawPane = 0;
        }

        setCursor(CursorDefault);
        removeTags();
    }

    method: activate (void;)
    {
        updateCurrentNode();
        if (_manageDock neq nil) _manageDock.show();
        if (_drawDock neq nil) _drawDock.show();
        setCursor(_currentDrawMode.cursor);
        updateDrawModeUI();
        setTags();
    }

    method: render (void; Event event)
    {
        if (_currentDrawMode eq _dropperDrawMode ||
            !_showBrush ||
            _pointerGone)
        {
            return;
        }

        let domain = event.domain(),
            flip   = event.domainVerticalFlip();

        setupProjection(domain.x, domain.y, flip);

        let r = _pointerRadius * (if _scaleBrush then 1.0 / scale() else 1.0),
            c = _currentDrawMode.color,
            c0 = Color(c[0], c[1], c[2], 1.0),
            c1 = Color(0, 0, 0, 1);

        glEnable(GL_BLEND);

        if (_currentDrawMode eq _textDrawMode)
        {
            let b = (int(QTime.currentTime().msec() / 500) & 0x1) == 1;
            glColor(c0 * (if b then Color(1,1,1,1) else Color(1,1,1,.5)));

            glBegin(GL_LINE_LOOP);
            glVertex(_pointer.x - r * .2, _pointer.y - r);
            glVertex(_pointer.x + r * .2, _pointer.y - r);
            glVertex(_pointer.x + r * .2, _pointer.y + r);
            glVertex(_pointer.x - r * .2, _pointer.y + r);
            glEnd();
        }
        else
        {
            glEnable(GL_POINT_SMOOTH);
            glBegin(GL_POINTS);

            for (float a = 0.0; a <= math.pi * 2.0; a += math.pi / 10.0)
            {
                let x = sin(a) * r + _pointer.x,
                    y = cos(a) * r + _pointer.y;

                glColor(c1); glVertex(x, y);
                glColor(c0); glVertex(x, y);
            }

            glEnd();
        }
    }

}


\: createMode(Mode; )
{
    return AnnotateMinorMode("annotate_mode");
}

}
