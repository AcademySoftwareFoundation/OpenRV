//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

module: rvtypes {
use commands;
use math;
require io;
require system;

//
//  Shared type definitions for the rv modules should go here
//

VoidFunc            := (void;);
EventFunc           := (void; Event);
Vec2                := vector float[2];
Vec3                := vector float[3];
Vec4                := vector float[4];
Point               := Vec2;
Color               := Vec4;
BBox                := Vec4;
NameValueBounds     := (Vec2, float[4][], float[4][], int);
DropFunc            := (void; int, string);
Glyph               := (void; bool);
StringPair          := (string, string);
MenuStateFunc       := (int;);
BoolFunc            := (bool;);
TextCommitFunc      := (void; string);
PromptFunc          := (string;);
Menu                := MenuItem[];
MenusFunc           := (Menu[];);
DefaultDirFunc      := (string;);
ConfirmFunc         := (void; string);
Labels              := string[];
BBoxes              := BBox[];
BindingList         := [(string, EventFunc, string)];
DefaultFeedbackTextSize := 20.0;
DefaultGamma        := 1.0;
DefaultHue          := 0.0;
DefaultSaturation   := 1.0;
DefaultContrast     := 0.0;
DefaultExposure     := 0.0;
DefaultOffset       := 0.0;
PixelSources        := PixelImageInfo[];

union: RenderFunctionType { ImageSpaceRender | ViewSpaceRender }

documentation: """
Used to keep track of external processes that report progress. RV can
display the process in a widget. This is a base class. To create a
working ExternalProcess you choose from either ExternalQProcess (Qt)
or ExternalIOProcess (which is an iostream). The QProcess version is
in external_qprocess.mu.

If in previous versions you inherited from ExternalProcess, you should
now inherit from ExternalQProcess (for RV) or ExternalIOProcess (for
rvio).
"""

class: ExternalProcess
{
    string      _name;
    string      _lastMessage;
    VoidFunc    _cleanup;
    float       _progress;
    regex       _progressRE;
    regex       _messageRE;
    int         _exitcode;
    string      _cancelDetails;

    union: Type { ReadOnly | ReadWrite };

    method: finish (void; int exitcode = -1) {;}
    method: isFinished (bool;) { return false; }
    method: cancel (void;) {;}
    method: processIO (void;) {;} 

    method: init (string name,
                  string path,
                  Type t,
                  VoidFunc cleanup_func,
                  regex progressRE = ".*\(([0-9]+(\.[0-9]*)?)% done\).*",
                  regex messageRE  = "INFO:[ \t]+([^(]+)(\([0-9]+\.?[0-9]*% +done\))?")
    {
        _name          = name;
        _progress      = 0;
        _cleanup       = cleanup_func;
        _progressRE    = progressRE;
        _messageRE     = messageRE;
        _exitcode      = 0;
        _cancelDetails = nil;
    }
}

documentation: """
These are overall look and behavior preferences They may be obtained
from a preferences file or set on the fly.  Each State object has a
reference to one of these which, by default, is the global
configuration object. If you replace the State reference to a unique
Configuration object, that Session will have unique configuration
values (not shared by the others).
"""

class: Configuration
{
    string lastOpenDir;
    string lastLUTDir;

    Color  bg;
    Color  fg;

    Color  bgErr;
    Color  fgErr;

    Color  bgFeedback;
    Color  fgFeedback;

    float  textEntryTextSize;

    float  inspectorTextSize;

    float  infoTextSize;

    float  wipeFade;          // seconds
    float  wipeFadeProximity; // pixels
    float  wipeGrabProximity; // pixels
    float  wipeInfoTextSize;

    float  msFrameTextSize;

    float  tlFrameTextSize;
    float  tlBoundsTextSize;
    Color  tlBoundsColor;
    Color  tlRangeColor;
    Color  tlCacheColor;
    Color  tlCacheFullColor;
    Color  tlAudioCacheColor;
    Color  tlInOutCapsColor;
    Color  tlMarkedColor;
    Color  tlSkipColor;
    Color  tlSkipTextColor;

    Color  matteColor;

    Color  bgVCR;
    Color  fgVCR;
    Color  bgTlVCR;
    Color  bgVCRButton;
    Color  hlVCRButton;

    string pdfReader;
    string htmlReader;
    string os;

    int bevelMargin;

    MenusFunc menuBarCreationFunc;  // for local menu customization

    [EventFunc] renderImageSpace;
    [EventFunc] renderViewSpace;
}

documentation: """
A mode is a feature unit. It comes in two varieties: major and
minor. The mode can be declared in its own file and provide key
bindings, a menu, multiple event tables if needed, a render function,
and is identifiable by name and icon. When active the minor mode can
render and its menu is visible. When inactive, the minor mode is
completely gone from RV.
"""

class: Mode 
{
    bool        _active;
    string      _modeName;
    Menu        _menu;
    string      _supportFilesPath;
    bool        _drawOnEmpty;
    bool        _drawOnPresentation;
    
    documentation: "Called right after activation";
    method: activate (void;) {;}

    documentation: "Called right before deactivation";
    method: deactivate (void;) {;}

    method: isActive (bool;) { return _active; }

    method: name (string;) { return _modeName; }

    method: toggle (void;)
    {
        _active = !_active;

        if (_active)
        {
            activateMode(_modeName);
            this.activate();
        }
        else
        {
            this.deactivate();
            deactivateMode(_modeName);
        }

        redraw();

        sendInternalEvent ("mode-toggled", "%s|%s" % (_modeName, _active), "Mode");
    }

    documentation: "Layout any margins or precompute anything necessary for rendering"
    method: layout (void; Event event) { ; }
    
    documentation: "The render function is called on each active minor mode."
    method: render (void; Event event) { ; }

    method: supportPath (string; string moduleName, string packageName)
    {
        use io;
        for_each (m; runtime.module_locations())
        {
            if (m._0 == moduleName)
            {
                return path.join(path.join(path.dirname(path.dirname(m._1)), "SupportFiles"), 
                                 path.without_extension(packageName));
            }
        }

        return nil;
    }

    method: supportPath (string; string moduleName)
    {
        return supportPath(moduleName, moduleName);
    }

    documentation: """
    Returns a path to a writable directory where temporary, and
    configuration files specific to a package are/should be located. The
    directory will be created if it does not yet exist."""

    method: configPath (string; string packageName)
    {
        use io;

        let envvar = system.getenv("TWK_APP_SUPPORT_PATH"),
            userDir = string.split(envvar, io.path.concat_separator()).front(),
            dir = path.join(path.join(userDir, "ConfigFiles"), packageName);

        try
        {
            if (!io.path.exists(dir)) system.mkdir(dir, 0x1ff);
        }
        catch (...)
        {
            print("ERROR: mode config path failed to create directory %s\n" % dir);
        }

        return dir;
    }
}

documentation: """
MinorModes are modes which are non-exclusive: there can be many minor
modes active at the same time. 
"""

class: MinorMode : Mode
{
    method: init (void; 
                  string name, 
                  BindingList globalBindings,
                  BindingList overrideBindings,
                  Menu menu = nil,
                  string sortKey = nil,
                  int ordering = 0)
    {
        _modeName           = name;
        _drawOnEmpty        = false;
        _drawOnPresentation = false;

        defineMinorMode(name, sortKey, ordering);

        for_each (b; globalBindings)
        {
            let (event, func, docs) = b;
            bind(_modeName, "global", event, func, docs);
        }

        for_each (b; overrideBindings)
        {
            let (event, func, docs) = b;
            bind(_modeName, "global", event, func, docs);
        }

        setMenu(menu);
    }

    method: init (void; 
                  string name,
                  BindingList blist,
                  bool dummy)
    {
        MinorMode.init(this, name, nil, blist, nil, nil, 0);
    }

    method: setMenu (void; Menu menu)
    {
        _menu = menu;
        defineModeMenu(_modeName, _menu, false);
    }

    method: setMenuStrict (void; Menu menu)
    {
        _menu = menu;
        defineModeMenu(_modeName, _menu, true);
    }

    method: defineEventTable (void; 
                              string tableName,
                              BindingList bindings)
    {
        for_each (b; bindings)
        {
            let (event, func, docs) = b;
            bind(_modeName, tableName, event, func, docs);
        }
    }

    method: defineEventTableRegex (void; 
                                   string tableName,
                                   BindingList bindings)
    {
        for_each (b; bindings)
        {
            let (event, func, docs) = b;
            bindRegex(_modeName, tableName, event, func, docs);
        }
    }

    //
    //  Virtual method for modes to "accept" urls for processing. To accept a URL
    //  the mode returns a non-nil drop function.  The returned string is drawn
    //  in the drop site.  First non-nil return wins.
    //

    method: urlDropFunc (((void; int, string), string); string url)
    {
        (void; int, string) f = nil;
        string s = nil;
        return (f, s);
    }
}

documentation: """
The Widget class is the base class for HUD widgets.  Each Widget must
provide a set of bindings, a render function (to draw itself) and an
*accurate* bounds which will determine where events are sent.
"""

class: Widget : MinorMode
{

  documentation: """
    The Widget.Button class represents a button for event handling purposes.
    The button uses the event map of the widget and is handled
    by the standard move/drag events.
    """

    class: Button
    {
        float _x;
        float _y;
        float _w;
        float _h;
        (void; Event) _callback;
        bool  _near;
        
        \: inside (bool; Button this, int x, int y)
        {
            x >= _x && x <= (_x + _w) && y >= _y && y <= (_y + _w);
        }
    }

    bool          _multiple;
    Configuration _config;
    float         _x;
    float         _y;
    float         _w;
    float         _h;
    Point         _downPoint;
    bool          _dragging;
    bool          _inCloseArea;
    bool          _containsPointer;
    Button[]      _buttons;
    int           _whichMargin;

    method: layout (void; Event event) { ; }
 
    method: render (void; Event event);

    method: init (void; 
                  string n, 
                  BindingList g,
                  BindingList o,
                  Menu m = nil,
                  string s = nil,
                  int or = 0)
    {
        _inCloseArea = false;
        _containsPointer = false;
        MinorMode.init(this, n, g, o, m, s, or);
    }

    method: init (void; 
                  string name,
                  BindingList blist,
                  bool allowMultipleInstances=true)
    {
        MinorMode.init(this, name, nil, blist, nil, nil, 0);

        _multiple    = allowMultipleInstances;
        _inCloseArea = false;
        _whichMargin = -1;
    }

    method: toggle (void;)
    {
        _active = !_active;

        if (_active)
        {
            writeSetting ("Tools", "show_" + _modeName, SettingsValue.Bool(_active));
            activateMode(_modeName);
        }
        else
        {
            writeSetting ("Tools", "show_" + _modeName, SettingsValue.Bool(_active));
            deactivateMode(_modeName);

            updateMargins (false);
        }

        redraw();

        sendInternalEvent ("mode-toggled", "%s|%s" % (_modeName, _active), "Mode");
    }

    method: updateMargins (void; bool activated)
    {
        //
        //  Below api push/pop is required because setMargins() causes
        //  margins-changed event to be sent which then triggers arbitrary mu
        //  code, which could of course allocate memory.
        //
        exception excOrig = nil;
        runtime.gc.push_api(0);
        try
        {
            if (activated)
            {
                if (_whichMargin != -1)
                {
		    //
		    //  updateMargins with activated==true should only be
		    //  called inside a render() call, since it's only then
		    //  that the "event device" is set.
		    //
                    let m = margins(),
                        marginValue = requiredMarginValue();

                    if (m[_whichMargin] < marginValue)
                    {
                        m[_whichMargin] = marginValue;
                        setMargins (m);
                    }
                }
            }
            else
            {
                if (_whichMargin != -1)
                {
		    //
		    //  Change only the margin this mode is rendering in, but change it
		    //  on _all_ devices.
		    //
                    vec4f m = vec4f{-1.0, -1.0, -1.0, -1.0};
                    m[_whichMargin] = 0;
                    setMargins (m);
                    setMargins (m, true);
                }
            }
        }
        catch(exception exc) { excOrig = exc;}
        catch(...) {;} 

        runtime.gc.pop_api();

        if (excOrig neq nil) throw(excOrig);
    }

    method: updateBounds (void; Vec2 minp, Vec2 maxp)
    {
        _x = minp.x;
        _y = minp.y;
        _w = maxp.x - minp.x;
        _h = maxp.y - minp.y;
        
        if (isModeActive(_modeName) && !_dragging)
        {
            setEventTableBBox(_modeName, "global", minp, maxp);

            updateMargins (true);
        }
    }

    method: contains (bool; Vec2 p)
    {
        if (p.x >= _x && p.x <= _x + _w && p.y >= _y && p.y <= _y + _h)
            return true;

        return false;
    }

    method: requiredMarginValue (float; )
    {
        let vs = viewSize(),
            devicePixelRatio = devicePixelRatio();

        if (_whichMargin == -1)
        //
        //  No margin required
        //
        {
            return 0.0;
        }
        else if (_whichMargin == 0)
        // 
        //  Left margin
        //
        {
            return (_x + _w)*devicePixelRatio;
        }
        else if (_whichMargin == 1)
        // 
        //  Right margin
        //
        {
            return vs.x - _x*devicePixelRatio;
        }
        else if (_whichMargin == 2)
        // 
        //  Top margin
        //
        {
            return vs.y - _y*devicePixelRatio;
        }
        else if (_whichMargin == 3)
        // 
        //  Bottom margin
        //
        {
            return (_y + _h)*devicePixelRatio;
        }

        return 0.0;
    }

    method: drawInMargin (void; int whichMargin)
    {
        _whichMargin = whichMargin;
    }
}


documentation: """
Holds the state for the UI. Return one of these from the session()
function. There will be one for each RV session launched. One per
RV window.
"""

class: State
{
    Configuration   config; // initialized to globalConfig

    int             scrubFrameOrigin;
    bool            scrubbed;
    bool            pushed;
    bool            scrubDisabled;
    bool            clickToPlayDisabled;
    bool            playingBefore;
    Point           downPoint;
    float           downScale;
    bool            scrubAudio;
    string          emptySessionStr;

    bool            firstRender;

    bool            pointerInSession;
    PixelSources    pixelInfo;
    Point           pointerPosition;
    Point           pointerPositionNormalized;
    string          currentInput;

    int             perPixelFrame;
    Point           perPixelPosition;
    int             perPixelViewHash;
    bool            perPixelInfoValid;

    bool            showInfoStrip;

    bool            stepWraps; //  single frame step wraps at in/out points.
    bool            scrubClamps; //  scrub stops at in/out points.
    bool            lockResizeScale; // lock pixel relative scale during window resize;

    float           feedback;
    string          feedbackText;
    Glyph           feedbackGlyph;
    float[]         feedbackTextSizes;  // Optional per-line text sizes
    bool            feedbackQueueEnabled;   // Enable/disable message queue feature
    bool            feedbackQueueTruncate;  // Truncate messages duration to 1s when queue has items
    (string, float, Glyph, float[])[] feedbackQueue; // Queue of pending feedback messages

    DefaultDirFunc  defaultOpenDir;
    DefaultDirFunc  defaultSaveDir;
    DefaultDirFunc  defaultLUTDir;
    DefaultDirFunc  default3DLUTDir;

    int             loadingCount;
    int             bufferingCount;

    string          parameter;
    float           parameterValue;
    float           parameterScale;
    float           parameterReset;
    float           parameterMinValue;
    (float;)        parameterGranularity;
    float           parameterPrecision;
    int             parameterChannel;
    bool            parameterLocked;
    string          parameterNode;

    bool            showMatte;
    float           matteAspect;
    float           matteOpacity;

    bool            textEntry;
    TextCommitFunc  textFunc;
    string          prompt;
    string          text;
    bool            textOkWhenEmpty;

    float           filestripX1;

    bool            dragDropOccuring;
    int             ddType;
    string          ddContent;
    int             ddRegion;
    DropFunc        ddDropFunc;
    int             ddFileKind;
    bool            ddProgressiveDrop;
    string[]        ddDropFiles;
    object          ddDropTimer;

    float           lastPixelBlockTime;

    bool            userActive;
    bool            unsavedChanges;
    bool            startupResize;
    
    ExternalProcess externalProcess;

    Widget          timeline;
    Widget          motionScope;
    Widget          imageInfo;
    Widget          inspector;
    Widget          infoStrip;
    Widget          processInfo;
    Widget          sourceDetails;
    MinorMode       modeManager;
    MinorMode       sessionManager;
    MinorMode       wipe;
    MinorMode       sync;
    Widget[]        widgets;
    MinorMode[]     minorModes;

    object          clickTimer;

    (string,string)[] quitConfirmMessages;
    int[]           savedInOut;

    method: toggleStepWraps (void; )
    {
        stepWraps = !stepWraps;
        writeSetting ("Controls", "stepWraps", SettingsValue.Bool(stepWraps));
    }

    method: toggleScrubClamps (void; )
    {
        scrubClamps = !scrubClamps;
        writeSetting ("Controls", "scrubClamps", SettingsValue.Bool(scrubClamps));
    }

    documentation: """Remove any unresolved quit messages."""
    method: clearQuitMessages(void;)
    {
        quitConfirmMessages.clear();
    }

    documentation: """Retrieve the most recent unresolved quit message."""
    method: getQuitMessage(string;)
    {
        if (quitConfirmMessages.size() > 0) return quitConfirmMessages.back()._1;
        else return nil;
    }

    documentation: """RV will first check to see if there are any unresolved quit
    messages before shutting down. To add your own use this method. This method
    takes "registrar": any unique string identifier you wish to refer to the unresolved
    quit blocker, and "message": a string message describing why RV should not quit."""
    method: registerQuitMessage(void; string registrar, string message)
    {
        quitConfirmMessages.push_back((registrar,message));
    }

    documentation: """Once your script has resolved its quit blocking state
    use this method to remove the specific message of your script. This method
    takes "registrar": the string identifier that refers to your unresolved quit
    blocker."""
    method: unregisterQuitMessage(void; string registrar)
    {
        (string,string)[] newQuitConfirmMessages = (string,string)[]();
        for_each (qm;quitConfirmMessages)
        {
            if (qm._0 != registrar) newQuitConfirmMessages.push_back(qm);
        }
        quitConfirmMessages = newQuitConfirmMessages;
    }
}

//
//  Common Widget Event handlers. 
//

\: storeDownPoint (void; Widget widget, Event event)
{
    widget._downPoint = event.pointer();
}

\: drag (void; Widget widget, Event event)
{
    if (widget._whichMargin != -1)
        return;

    widget._dragging = true;

    let pp = event.pointer(),
        dp = widget._downPoint,
        ip = pp - dp;

    if (!widget._inCloseArea)
    {
        widget._x += ip.x;
        widget._y += ip.y;
    }

    widget._downPoint = pp;

    redraw();
}

\: move (void; Widget widget, Event event)
{
    widget._dragging = false;
    State state = data();
    
    let domain = event.subDomain(),
        p      = event.relativePointer(),
        tl     = vec2f(0, domain.y),
        pc     = p - tl,
        d      = mag(pc),
        m      = state.config.bevelMargin,
        lc     = widget._inCloseArea,
        near   = d < m;

    if (near != lc) redraw();
    widget._inCloseArea = near;

    //
    //  If we've left the widget, don't draw the close button.
    //
    widget._containsPointer = widget.contains(event.pointer());
    if (!widget._containsPointer) widget._inCloseArea = false;

    event.reject();
}

\: release (void; Widget widget, Event event, (void;) closeFunc)
{
    widget._dragging = false;
    State state = data();
    let bp = event.pointer();

    if (widget._buttons neq nil)
    {
        for_each (b; widget._buttons)
        {
            if (b._callback neq nil && b.inside(bp.x, bp.y))
            {
                b._callback(event);
                return;
            }
        }
    }
    
    let domain = event.subDomain(),
        p      = event.relativePointer(),
        tl     = vec2f(0, domain.y),
        pc     = p - tl,
        d      = mag(pc),
        m      = state.config.bevelMargin,
        cen    = vec2f(m * 0.5, -m * 0.5) + tl,
        near   = d < m;
    
    if (near)
    {
        if (closeFunc neq nil) closeFunc();
        else if (widget._active) widget.toggle();
        widget._inCloseArea = false;
    }

    storeDownPoint(widget, event);
}


}
