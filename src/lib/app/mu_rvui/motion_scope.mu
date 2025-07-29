//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

module: motion_scope {

use rvtypes;
use glyph;
use app_utils;
use math;
use math_util;
use commands;
use extra_commands;
use gl;
use glu;
require io;
require timeline;

\: deb(void; string s) { if (false) print("moscope: " + s + "\n"); }

//----------------------------------------------------------------------
//
//  MotionScope
//

class: MotionScope : Widget
{
    TextBounds := float[4];
    FrameStringFunc := (string;int);
    AudioSmall := 25;
    AudioMedium := 55;
    AudioLarge := 100;
    int[] _frameMap;

    class: Settings
    {
        bool showVCRButtons;
        bool drawInMargin;
        bool drawAtTopOfView;
        bool showInOut;
        bool showFrameDirection;
        int frameDisplayFormat;
        int audioHeight;

        method: writeSettings (void; )
        {
            writeSetting ("MotionScope", "drawInMargin", SettingsValue.Bool(drawInMargin));
            writeSetting ("MotionScope", "drawAtTopOfView", SettingsValue.Bool(drawAtTopOfView));
            writeSetting ("MotionScope", "frameDisplayFormatNew", SettingsValue.Int(frameDisplayFormat));
            writeSetting ("MotionScope", "audioHeight", SettingsValue.Int(audioHeight));
        }

        method: readSettings (void; )
        {
            let SettingsValue.Bool b2 = readSetting ("MotionScope", "drawInMargin", SettingsValue.Bool(true));
            drawInMargin = b2;

            let SettingsValue.Bool b3 = readSetting ("MotionScope", "drawAtTopOfView", SettingsValue.Bool(true));
            drawAtTopOfView = b3;

            let SettingsValue.Int i1 = readSetting ("MotionScope", "frameDisplayFormatNew", SettingsValue.Int(0));
            frameDisplayFormat = i1;

            let SettingsValue.Int i2 = readSetting ("MotionScope", "audioHeight", SettingsValue.Int(AudioSmall));
            audioHeight = i2;
            if (    audioHeight != AudioSmall &&
                    audioHeight != AudioMedium &&
                    audioHeight != AudioLarge)
            {
                audioHeight = AudioSmall;
            }
        }

        method: Settings (Settings; )
        {
            try
            {
                readSettings();
            }
            catch (...)
            {
                ; // ignore
            }
        }
    }

    Settings        _settings;
    float           _nudgeY;
    float           _X0;
    float           _Y0;
    float           _X1;
    float           _H;
    float           _W;
    float           _tlw;
    float           _controlSize;
    float           _phantomFrameX;
    float           _phantomFrame;
    bool            _pointerInMotionScope;
    bool            _phantomFrameDisplay;
    bool            _drawInOut;
    bool            _drag;
    int             _delayedInFrame;
    int             _delayedOutFrame;
    FrameStringFunc _frameFunc;
    FrameStringFunc _totalFunc;
    bool            _displayMbps;
    float           _frameTextOffset;
    timeline.Timeline _timeline;
    int             _audioSmall;
    int             _audioMedium;
    int             _audioLarge;
    bool            _usingSourceFrames;

    class: Locations
    {
        Vec2       domain;
        int        width;
        int        height;
        int        timelineHeight;
        TextBounds textBounds;
        int        hMargin;
        int        vMargin;
        int        something;
        int        bMargin;
        int        tMargin;
        int        tOff;
        int        t;
        int        baseY;
    }

    method: fullWidth (int; Event event)
    {
        let d = event.domain();
        return d.x;
    }

    method: renderLocations (Locations; Event event)
    {
        let d       = event.domain(),
            isRenderEvent       = event.name() == "render",
            devicePixelRatio    = (if isRenderEvent then devicePixelRatio() else 1.0),
            tbounds             = gltext.bounds("||||"),
            audioH              = float(_settings.audioHeight) * devicePixelRatio,
            tOff                = (if (_timeline._active && _settings.drawAtTopOfView == _timeline._settings.drawAtTopOfView) then _timeline._h else 0) * devicePixelRatio,
            t                   = floor((5 + 8) * devicePixelRatio + tbounds[3] + 0.5 + max(audioH, 8 * devicePixelRatio));

        return Locations(d,
                         d.x,                   // window width
                         d.y,                   // window height
                         8*devicePixelRatio,    // timeline height
                         tbounds,               // text bounds
                         30*devicePixelRatio,   // horizontal margin
                         30*devicePixelRatio,   // vertical margin
                         0,
                         5*devicePixelRatio,    // bottom  margin
                         8*devicePixelRatio,    // top margin
                         tOff,
                         t,
                         if _settings.drawAtTopOfView then floor(d.y - t - tOff) else tOff);
    }

    method: frameAtPointer (int; Locations loc, Event event)
    {
        let {d, w, h, tlh, mxb, hm0, hm1, thm1, vm0, vm1, tOff, t, Y} = loc;

        let x     = event.pointer().x,
            x0    = hm0,
            x1    = w - hm1,
            fs    = inPoint(),
            fe    = outPoint(),
            xfrac = (x - x0) / (x1 - x0),
            frac  = min(float(fe-fs)/float(fe-fs+1), max(0.0, xfrac));

        return frac * (fe - fs + 1) + fs;
    }

    method: invalidateSourceFrameMapping (void; Event event=nil)
    {
        deb ("inval");
        _frameMap.clear();
        if (event neq nil) event.reject();
    }

    method: toggle (void; )
    {
        deb ("toggle");
        invalidateSourceFrameMapping();
        Widget.toggle(this);
        if (_active) updateAudioSize();
        else
        {
            setIntProperty("#RVSoundTrack.visual.width", int[] {0});
            setIntProperty("#RVSoundTrack.visual.height", int[] {0});
            setIntProperty("#RVSoundTrack.visual.frameStart", int[] {0});
            setIntProperty("#RVSoundTrack.visual.frameEnd", int[] {0});
        }
    }

    method: updateSourceFrameMapping (void; )
    {
        let in = inPoint(),
            out = outPoint();
        deb ("rebuilding frame map (%s to %s) ..." % (in, out));
        _frameMap.clear();
        for (int f = in; f <= out; ++f)
        {
            _frameMap.push_back(sourceFrame(f));
        }
        deb ("done");
    }

    method: updateSourceFrameMappingIfNeeded (void; )
    {
        if (_frameMap.size() == 0 && loadTotal() == 0 && _usingSourceFrames == true)
        {
            let in = inPoint(),
                out = outPoint(),
                pixelsPerFrame = float(_tlw/(out - in + 1));
            if (10*pixelsPerFrame >= 6)
            {
                updateSourceFrameMapping();
            }
        }
    }

    method: preRender (void; Event event)
    {
        let d = event.domain(),
          hm0 = 30,  // horizontal margin
          hm1 = 30;  // horizontal margin
         _tlw = d.x - hm1 - hm0;

        updateSourceFrameMappingIfNeeded();

        event.reject();
    }
    
    method: sourceFrameMapping (int; int frame)
    {
        //  When loading progressively, don't recompute frame
        //  mapping, cause it takes too long.

        if (loadTotal() > 0) return 1;

        updateSourceFrameMappingIfNeeded();

        if (_frameMap.size() == 0) return sourceFrame(frame);

        let sampFrame = frame - inPoint();
        if (sampFrame < 0) sampFrame = 0;
        if (sampFrame > _frameMap.size()-1) sampFrame = _frameMap[_frameMap.size()-1];
        //deb ("sampFrame %s sz %s" % (sampFrame, _frameMap.size()));
        return _frameMap[sampFrame];
    }

    method: handleLeave (void; Event event)
    {
        deb ("handleLeave");
        _pointerInMotionScope = false;
        redraw();
        event.reject();
    }

    method: processPointer (bool; Event event)
    {
        deb ("processPointer");
        let gp = event.pointer(),
            p  = event.relativePointer(),
            modified = false;

        if (contains (gp))
        {
            _timeline._pointerInTimeline = false;
            if (!_pointerInMotionScope)
            {
                _pointerInMotionScope = true;
                modified = true;
                redraw();
            }
            if (p.x != _phantomFrameX)
            {
                _phantomFrameX = p.x;
                _phantomFrame = frameAtPointer(renderLocations(event), event);
                modified = true;
                redraw();
            }
        }
        else if (_pointerInMotionScope)
        {
            _pointerInMotionScope = false;
            modified = true;
            redraw();
        }

        return modified;
    }

    method: handleMotion (void; Event event)
    {
        deb ("handleMotion");
        let gp = event.pointer();
        let p = event.relativePointer();

        processPointer (event);
        event.reject();
    }

    method: whichMargin (int; )
    {
        if (_settings.drawAtTopOfView) then 2 else 3;
    }


    \: scrubSetFrame (void; int frame, int x, int y)
    {
        deb ("scrubSetFrame");
        State state = data();
        stop();
        if (state.scrubAudio) scrubAudio(true, 1.0 / fps(), 1);
        else scrubAudio(state.scrubAudio, 0, 0);
        setFrame(frame);
    }
    
    method: setFrameOrNudge (void; int f, int x, int y)
    {
        deb ("setFrameOrNudge");
        let x0 = _X0,
            x1 = _X1,
            devicePixelRatio = devicePixelRatio();

        // Note: x and y are in device pixels (mouse pointers), so we may need to scale them
        x = x * devicePixelRatio;
        y = y * devicePixelRatio;

        if (x > x0 && x < x1) setFrame(f); 
        else
        {
            deb ("x %s y %s _Y0 %s _nudgeY %s" % (x, y, _Y0, _nudgeY));
            if (y > _Y0 + _nudgeY - 8*devicePixelRatio && y < _Y0 + _nudgeY + 8*devicePixelRatio)
            {
                let p = isPlaying();

                if      (x < 18*devicePixelRatio)      setInPoint(inPoint()-1);
                else if (x < 30*devicePixelRatio)      setInPoint(inPoint()+1);
                if      (x > _W - 18*devicePixelRatio) setOutPoint(outPoint()+1);
                else if (x > _W - 30*devicePixelRatio) setOutPoint(outPoint()-1);

                if (p != isPlaying()) togglePlay();
            }
        }
    }

    method: setInOutPoint (void; string which, int f, int x, int y)
    {
        deb ("setInOutPoint");
        if (which == "in") setInPoint(f);
        else setOutPoint(f);
    }

    method: setInOutPointDelayed (void; string which, int f, int x, int y)
    {
        deb ("setInOutPointDelayed");
        if (which == "in") 
        {
            _delayedInFrame = f;
            _delayedOutFrame = f;
        }
        else               
        {
            _delayedOutFrame = f;
            redraw();
        }
    }

    method: clickFunction (void; Event event, (void;int,int,int) F)
    {
        if (filterLiveReviewEvents()) {
            sendInternalEvent("live-review-blocked-event");
            return;
        }
        deb ("clickFunction");

        //   Disregard drag events if we didn't mouse-down in this widget.
        //
        if (!_drag && (event.name() == "pointer-1--drag" || event.name() == "stylus-pen--drag"))
        {
                event.reject();
                return;
        }
        State state = data();
        let loc = renderLocations(event);
        let {d, w, h, tlh, mxb, hm0, hm1, thm1, vm0, vm1, tOff, t, Y} = loc;

        let x0 = hm0,
            x1 = w - hm1,
            x  = event.relativePointer().x,
            y  = event.relativePointer().y,
            f  = frameAtPointer(loc, event);

        _drag = true;
        if (x > w) return;
        
        F(f, x, y);
        redraw();   // so audio waveform can be updated while scrubbing

        _pointerInMotionScope = false;
    }

    //
    //  put the symbols of the same name in timeline into MotionScope
    //  so we don't have duplicates.
    //

    globalTimeCode  := timeline.Timeline.globalTimeCode;
    sourceTimeCode  := timeline.Timeline.sourceTimeCode;
    seconds         := timeline.Timeline.seconds;
    footage         := timeline.Timeline.footage;

    method: getSourceFrame(string; int frame) 
    {                                   
        try
        {
            return "%d" % sourceFrameMapping(frame);
        }
        catch (...)
        {
            return " ";
        }
    }

    method: showSourceFrame(void;)
    {
        _frameFunc = getSourceFrame;
        _totalFunc = \: (string; int f) { "%d frames" % f; };
        _phantomFrameDisplay = true;
        _usingSourceFrames = true;
    }

    method: showTimeCode (void;)
    {
        _frameFunc = globalTimeCode;
        _totalFunc = \: (string; int f) { globalTimeCode(f) + " min"; };
        _phantomFrameDisplay = true;
    }

    method: showSourceTimeCode (void;)
    {
        _frameFunc = sourceTimeCode;
        _totalFunc = \: (string; int f) { globalTimeCode(f) + " min"; };
        _phantomFrameDisplay = true;
    }

    method: showSeconds (void;)
    {
        _frameFunc = seconds;
        _totalFunc = \: (string; int f) { seconds(f) + " sec"; };
        _phantomFrameDisplay = true;
    }

    method: showFootage (void;)
    {
        _frameFunc = footage;
        _totalFunc = \: (string; int f) { footage(f) + " ft"; };
        _phantomFrameDisplay = true;
        _usingSourceFrames = false;
    }

    method: showFrames (void;)
    {
        _frameFunc = \: (string; int f) { string(f); };
        _totalFunc = \: (string; int f) { "%d frames" % f; };
        _phantomFrameDisplay = true;
        _usingSourceFrames = false;
    }

    method: optShowVCRButtons (void; Event event)
    {
        _settings.showVCRButtons = !_settings.showVCRButtons;
        _settings.writeSettings();
    }

    method: optShowFrameDirection (void; Event event)
    {
        _settings.showFrameDirection = !_settings.showFrameDirection;
        _settings.writeSettings();
    }

    method: isShowingVCRButtons (int;) 
    { 
        if _settings.showVCRButtons then CheckedMenuState else UncheckedMenuState; 
    }

    method: isShowingFrameDirection (int;) 
    { 
        if _settings.showFrameDirection then CheckedMenuState else UncheckedMenuState; 
    }

    method: optDrawMotionScopeOverImagery (void; Event event)
    {
        _settings.drawInMargin = !_settings.drawInMargin;
        _settings.writeSettings();

        if (_settings.drawInMargin) drawInMargin (whichMargin());
        else 
        {
            drawInMargin (-1);
	    vec4f m = vec4f{-1.0, -1.0, -1.0, -1.0};
            m[whichMargin()] = 0;
            setMargins (m, true);
        }
        redraw();
    }

    method: isDrawingMotionScopeOverImagery (int;) 
    { 
        if _settings.drawInMargin then UncheckedMenuState else CheckedMenuState; 
    }

    method: optDrawMotionScopeAtTopOfView (void; Event event)
    {
        let oldMargin = whichMargin();

        _settings.drawAtTopOfView = !_settings.drawAtTopOfView;
        _settings.writeSettings();

        if (_settings.drawInMargin)
        {
            drawInMargin (whichMargin());

	    vec4f m = vec4f{-1.0, -1.0, -1.0, -1.0};
            m[oldMargin] = 0;
            setMargins (m, true);
        }
        redraw();
    }

    method: isDrawingMotionScopeAtTopOfView (int;)
    { 
        if _settings.drawAtTopOfView then CheckedMenuState else UncheckedMenuState; 
    }

    method: optShowInOutTime (void; Event event)
    {
        _settings.showInOut = !_settings.showInOut;
        _settings.writeSettings();
        redraw();
    }

    method: isShowingInOutTime (int;)
    {
        if _settings.showInOut then CheckedMenuState else UncheckedMenuState;
    }

    method: setFrameDisplayFormat (void; int format)
    {
        if (0 == format) showSourceFrame();
        if (1 == format) showTimeCode();
        if (2 == format) showFrames();
        if (3 == format) showFootage();
        if (4 == format) showSourceTimeCode();
        if (5 == format) showSeconds();

        _settings.frameDisplayFormat = format;
        redraw();
    }

    method: setFrameDisplay (void; int format, Event event)
    {
        setFrameDisplayFormat (format);
        _settings.writeSettings();
    }

    method: isDisplayFormat ((int;); int format)
    {
        \: (int;)
        {
            if (this._settings.frameDisplayFormat == format) 
                then CheckedMenuState 
                else UncheckedMenuState; 
        };
    }

    method: updateAudioSize (void; )
    {
        deb ("updateAudioSize");
        if (getIntProperty("#RVSoundTrack.visual.width").front() != _settings.audioHeight)
        {
            setAudioSize(_settings.audioHeight, nil);
        }
    }

    method: newSource (void; Event event)
    {
        updateAudioSize();
        event.reject();
    }

    method: setAudioSize (void; int height, Event event)
    {
        deb ("setAudioSize");
        _settings.audioHeight = height;
        _settings.writeSettings();
        let width = if (height != 0) then 2048 else 0;

        setIntProperty("#RVSoundTrack.visual.width", int[] {height});
        setIntProperty("#RVSoundTrack.visual.height", int[] {width});
        setIntProperty("#RVSoundTrack.visual.frameStart", int[] {inPoint()});
        setIntProperty("#RVSoundTrack.visual.frameEnd", int[] {outPoint()});

        if (_settings.drawInMargin)
        {
            drawInMargin (whichMargin());

	    vec4f m = vec4f{-1.0, -1.0, -1.0, -1.0};
            m[whichMargin()] = 0;
            setMargins (m, true);
        }
        redraw();
        deb ("setAudioSize %s complete" % height);
    }

    method: isAudioSize((int;); int height)
    {
        \: (int;)
        {
            if this._settings.audioHeight == height then CheckedMenuState else UncheckedMenuState;
        };
    }


    method: optStepWraps (void; Event event)
    {
        State s = data();
        s.toggleStepWraps();
    }

    method: isStepWrapping (int;)
    { 
        State s = data();
        if s.stepWraps then CheckedMenuState else UncheckedMenuState; 
    }

    method: phantomSetInPoint (void; Event event)
    {
        setInPoint (_phantomFrame);
    }

    method: phantomSetOutPoint (void; Event event)
    {
        setOutPoint (_phantomFrame);
    }

    method: currentSetInPoint (void; Event event)
    {
        setInPoint (frame());
    }

    method: currentSetOutPoint (void; Event event)
    {
        setOutPoint (frame());
    }

    method: clearInOut (void; Event event)
    {
        setInPoint(frameStart());
        setOutPoint(frameEnd());
    }

    method: doResetMbps (void; Event event)
    {
        resetMbps();
    }

    method: phantomMarkFrame (void; Event event)
    {
        markFrame (_phantomFrame, true);
    }

    method: currentMarkFrame (void; Event event)
    {
        markFrame (frame(), true);
    }

    method: doClearAllMarks (void; Event event)
    {
        let marks = markedFrames();
        for_each (m; marks) markFrame(m, false);
        redraw();
    }


    \: combine (Menu; Menu a, Menu b)
    {
        if (a eq nil) return b;
        if (b eq nil) return a;

        Menu n;
        for_each (i; a) n.push_back(i);
        for_each (i; b) n.push_back(i);
        n;
    }

    method: popupOpts (void; Event event)
    {
        if (filterLiveReviewEvents()) {
            sendInternalEvent("live-review-blocked-event");
            return;
        }

        if (isCurrentFrameIncomplete() ||!_pointerInMotionScope)
        {
            event.reject();
            return;
        }

        let fs = frameStart(),
            fe = frameEnd(),
            pointerWasInMotionScope = _pointerInMotionScope;

        _pointerInMotionScope = false;

        Menu lastHalf = Menu { 
            {"_", nil},
            {"Time/Frame Display", nil, nil, disabledItem},
            {"  Global Frame Numbers", setFrameDisplay(2,), nil, isDisplayFormat(2)},
            {"  Source Frame Numbers", setFrameDisplay(0,), nil, isDisplayFormat(0)},
            {"  Global Time Code Display", setFrameDisplay(1,), nil, isDisplayFormat(1)},
            {"  Source Time Code Display", setFrameDisplay(4,), nil, isDisplayFormat(4)},
            {"  Global Seconds", setFrameDisplay(5,), nil, isDisplayFormat(5)},
            {"  Footage Display", setFrameDisplay(3,), nil, isDisplayFormat(3)},
            {"_", nil},
            {"Audio", nil, nil, disabledItem},
            {"  No Audio Display", setAudioSize(0,), nil, isAudioSize(0)},
            {"  Small", setAudioSize(AudioSmall,), nil, isAudioSize(AudioSmall)},
            {"  Medium", setAudioSize(AudioMedium,), nil, isAudioSize(AudioMedium)},
            {"  Large", setAudioSize(AudioLarge,), nil, isAudioSize(AudioLarge)},
            {"_", nil},
            {"Configure", Menu {
                {"Draw Magnifier Over Imagery", optDrawMotionScopeOverImagery, nil, isDrawingMotionScopeOverImagery},
                {"Position Magnifier At Top", optDrawMotionScopeAtTopOfView, nil, isDrawingMotionScopeAtTopOfView},
                {"Step Wraps At In/Out", optStepWraps, nil, isStepWrapping},
                //{"Show Play Direction Indicator", optShowFrameDirection, nil, isShowingFrameDirection},
                }
            }};

        Menu mbpsMenu = Menu {{"Reset MBPS", doResetMbps}};

        if ( pointerWasInMotionScope && 
            _phantomFrame >= fs && 
            _phantomFrame <= fe)
        {
            let fname ="%s" % _frameFunc(_phantomFrame),
                title = "Magnifier at %s" % fname,
                media = inputNodeUserNameAtFrame(_phantomFrame);

            Menu firstHalfA = Menu {
                {title, nil, nil, disabledItem},
                {media, nil, nil, disabledItem},
                {"_", nil},
                {"Set In Frame to %s" % fname, phantomSetInPoint},
                {"Set Out Frame to %s" % fname, phantomSetOutPoint},
                {"Clear In/Out Frames", clearInOut},
                {"Mark Frame %s" % fname, phantomMarkFrame},
                {"Clear Marks", doClearAllMarks}};

            Menu all;
            if (_displayMbps) all = combine (firstHalfA, combine (mbpsMenu, lastHalf));
            else all = combine (firstHalfA, lastHalf);

            popupMenu (event, all);
        }
        else
        {
            Menu firstHalfB = Menu {
                {"Timeline Magnifier", nil, nil, \: (int;) { DisabledMenuState; }},
                {"_", nil},
                {"Set In Frame to Current", currentSetInPoint},
                {"Set Out Frame to Current", currentSetOutPoint},
                {"Clear In/Out Frames", clearInOut},
                {"Mark Current Frame", currentMarkFrame},
                {"Clear Marks", doClearAllMarks}};

            Menu all;
            if (_displayMbps) all = combine (firstHalfB, combine (mbpsMenu, lastHalf));
            else all = combine (firstHalfB, lastHalf);

            popupMenu (event, all);
        }
    }

    method: releaseDrag (void; Event event)
    {
        _drag = false;
    }

    method: release (void; Event event)
    {
        deb ("release");
        scrubAudio(false);
        _drag = false;
            
        let p = event.relativePointer(),
            d = event.subDomain(),
            W = fullWidth(event),
            x = (p.x - W) / (d.x - W);

        if (x < 0 || p.y > _controlSize) return;

        if (x > 0.4 && x < 0.6)
        {
            stop();
        }
        else if (x > 0.6 && x < 0.8)
        {
            if (inc() < 0)
            {
                toggleForwardsBackwards();
                if (!isPlaying()) togglePlay();
            }
            else
            {
                togglePlay();
            }
        }
        else if (x < 0.4 && x > 0.2)
        {
            if (inc() > 0)
            {
                toggleForwardsBackwards();
                if (!isPlaying()) togglePlay();
            }
            else
            {
                togglePlay();
            }
        }
        else if (x < 0.2)
        {
            stepBackward1();
        }
        else if (x > 0.2)
        {
            stepForward1();
        }
    }

    method: recenterScope (void; Event event)
    {
        deb ("rescenterScope");
        if (!_active) toggle();

        if (inPoint() == frameStart() && outPoint() == frameEnd())
        {
            setInPoint(frame()-10);
            setOutPoint(frame()+10);
        }
        else
        {
            let diff = outPoint() - inPoint();
            setInPoint(frame()-diff/2);
            setOutPoint(frame()+diff/2);
        }
        redraw();
        event.reject();
    }

    method: newInOutPoint (void; Event event)
    {
        deb ("newInOutPoint");
        if (inPoint() < outPoint())
        {
            try 
            {
                setIntProperty("#RVSoundTrack.visual.frameStart", int[] {inPoint()});
                setIntProperty("#RVSoundTrack.visual.frameEnd", int[] {outPoint()});
            }
            catch (...) {;}
        }
        invalidateSourceFrameMapping();
        event.reject();
    }

    method: MotionScope (MotionScope; string name, timeline.Timeline t)
    {
        deb ("MotionScope constructor name '%s'" % name);

        _frameMap = int[]();
        init(name,
             [ ("pointer-1--push", clickFunction(,setFrameOrNudge), "Set Frame On Magnifier"),
               ("pointer-1--drag", clickFunction(,scrubSetFrame), "Drag Frame On Magnifier"),
               ("pointer-1--shift--push", clickFunction(,setInOutPointDelayed("in",,,)), "Set In Point on Magnifier"),
               ("pointer-1--shift--drag", clickFunction(,setInOutPointDelayed("out",,,)), "Select In/Out Region on Magnifier"),
               ("pointer-3--push", popupOpts, "Popup Magnifier Options"),
               ("pointer--move", handleMotion, ""),
               ("pointer--leave", handleLeave, "Track pointer leave"),
               ("pointer-1--shift--release", releaseDrag, ""),
               ("pointer-1--release", release, ""),
               ("new-in-point", newInOutPoint, "In Point Changed"),
               ("new-out-point", newInOutPoint, "Out Point Changed"),
               ("stylus-pen--push", clickFunction(,setFrameOrNudge), "Set Frame On Timeline"),
               ("stylus-pen--drag", clickFunction(,scrubSetFrame), "Drag Frame On Timeline"),
               ("stylus-pen--move", handleMotion, ""),
               ("stylus-pen--release", release, ""),
               ("stylus-eraser--push", clickFunction(,setFrameOrNudge), "Set Frame On Timeline"),
               ("stylus-eraser--drag", clickFunction(,scrubSetFrame), "Drag Frame On Timeline"),
               ("stylus-eraser--move", handleMotion, ""),
               ("stylus-eraser--release", release, ""),
               ("new-source", newSource, "Update Audio on new-source"),
               ("after-graph-view-change", invalidateSourceFrameMapping, ""),
               ("range-changed", invalidateSourceFrameMapping, ""),
               ("pre-render", preRender, ""),
               ],
             false);

        //  XXX
        //  app_utils.bind("key-down--x", recenterScope);
        //  app_utils.bind("key-down--X", clearInOut);

        _timeline          = t;
        _phantomFrame      = 1.0;
        _phantomFrameX     = 0;
        _pointerInMotionScope = false;
        _drag              = false;
        _displayMbps       = (system.getenv("RV_DISPLAY_MBPS", "no") != "no");
        _controlSize       = 35;
        _usingSourceFrames = false;
        _tlw               = 100.0;

        _settings = Settings();
        setAudioSize(_settings.audioHeight, nil);
        setFrameDisplayFormat (_settings.frameDisplayFormat);

        //
        //  Draw inside the bottom margin.
        //

        if (_settings.drawInMargin) drawInMargin (whichMargin());

        _delayedInFrame = _delayedOutFrame = -1;

        updateBounds(vec2f(0,0), vec2f(0,0));
    }

    method: layout (void; Event event)
    {
        deb ("MotionScope layout()");
        State state = data();
        let devicePixelRatio = devicePixelRatio();
        gltext.size(state.config.msFrameTextSize * devicePixelRatio);

        let d   = event.domain(),
            f   = frame(),
            s   = _frameFunc(f),
            sb  = gltext.bounds(s),             // size of frame string
            mxb = gltext.bounds("||||"),
            vm0 =  if (_settings.drawAtTopOfView) then 5*devicePixelRatio else 5*devicePixelRatio,  // vertical margin
            vm1 = 8*devicePixelRatio,
            tlh = 8*devicePixelRatio,           // timeline height
            t   = floor (vm0 + vm1 + mxb[3] + 0.5 + max(_settings.audioHeight*devicePixelRatio,tlh)),
            tOff = (if (_timeline._active && _settings.drawAtTopOfView == _timeline._settings.drawAtTopOfView) then _timeline._h else 0)*devicePixelRatio,
            _Y0 = if (_settings.drawAtTopOfView) then d.y - t - tOff else tOff;

        deb ("layout: tOff %s _Y0 %s t %s d.y %s\n" % (tOff, _Y0, t, d.y));

        updateBounds(vec2f(0,_Y0)/devicePixelRatio, vec2f(d.x,_Y0+t)/devicePixelRatio);
        event.reject();
    }

    method: isUndiscoveredFrame(bool; int frameNum)
    {
        let metainfo = sourceMetaInfoAtFrame(frameNum);
        if (metainfo eq nil || metainfo.node == "")
            return true;

        return nodeRangeInfo(metainfo.node).isUndiscovered;
    }

    method: render (void; Event event)
    //
    //  NOTE: this fuction cannot allocate any memory that it expects to keep
    //  across calls.  To reduce memory leaks this function is called with an
    //  arena allocator and all memory it allocates is deleted on return.
    //
    //  If you must allocate memory to keep, bracket that allocation with 
    //  runtime.push_api(0), runtim.pop_api().
    //
    {
        //deb ("MotionScope render()");
        if (commands.audioTextureComplete() < 1.0)
        {
            redraw();
        }

        State state = data();
        let devicePixelRatio = devicePixelRatio();
        gltext.size(state.config.msFrameTextSize * devicePixelRatio);

        if (_settings.audioHeight != 0)
        {
            let ip = inPoint(),
                op = outPoint(),
                fs = getIntProperty("#RVSoundTrack.visual.frameStart").front(),
                fe = getIntProperty("#RVSoundTrack.visual.frameEnd").front();

            if (fs != ip || fe != op)
            {
                setIntProperty("#RVSoundTrack.visual.frameStart", int[] {inPoint()});
                setIntProperty("#RVSoundTrack.visual.frameEnd", int[] {outPoint()});
            }
        }

        let loc = renderLocations(event);
        let {d, w, h, tlh, mxb, hm0, hm1, thm1, vm0, vm1, tOff, t, Y} = loc;

        let f   = frame(),
            s   = _frameFunc(f),
            sb  = gltext.bounds(s),             // size of frame string
            audioH = float(_settings.audioHeight) * devicePixelRatio,
            _Y0 = Y,
            Ybot = _Y0 + vm0,
            Ytop = Ybot + max(audioH,tlh);

        _tlw = w - hm1 - hm0;

        let fi  = inPoint(),
            fo  = outPoint(),
            fs  = fi,
            fe  = fo,
            dio = fs != fi || fe != fo,
            r   = fe - fs + 1,                  // frame range
            fp  = float(f - fs) / float(r),
            ip  = float(fi - fs) / float(r),
            op  = float(fo - fs + 1) / float(r),
            ope = float(fo - fs + 1) / float(r),
            fg  = state.config.fg,
            bg  = state.config.bg,
            xframe = fp * _tlw + hm0,
            xframe1 = (fp + 1.0/float(r)) * _tlw + hm0,
            xframeMid = (xframe + xframe1) * 0.5,
            xdiff = xframe1 - xframe,
            frameIsMarked = false,
            phantomFrameIsMarked = false,
            hlColor = Color(.3, .3, .3, 1),
            config = state.config,
            playing = isPlaying();

        let vnode = viewNode(),
            vtype = nodeType(vnode),
            (vins, vouts) = nodeConnections(vnode, false);

        _X0 = hm0;
        _X1 = w - hm1;
        _H  = t;
        _W  = w;

        //
        //  This affects timeline event table
        //  events will be selected inside this bbox
        //

        if (event.name() == "render") 
        {
            updateBounds(vec2f(0,_Y0)/devicePixelRatio, vec2f(d.x,_Y0+t)/devicePixelRatio);
        }
        else
        //
        //  OK this is a mess.  The problem is that the widget class still
        //  thinks it knows w/h/x/y for the widget window, but in fact it
        //  doesn't since there may be two such windows, one on controller and
        //  one on pres device.  In addition the moscope is special since it
        //  has to know how to "stack" with the timeline if they are both
        //  displayed at top or bottom.
        //  
        //  Really the way the margins are "shared" should be re-done, but
        //  don't have time at the mo.
        //
        {
            let m = margins(),
                marginVal = if (whichMargin() == 2) then /*top*/ d.y-_Y0 else /*bottom*/ _Y0+t;
                
            if (_settings.drawInMargin && m[whichMargin()] < marginVal)
            {
            m[whichMargin()] = marginVal;
            setMargins (m);
            }
        }

        \: motionScopeQuad (void; Color color, int x1, int x2, int thinnerBy = 0)
        {
            glColor(color);
            glVertex(x1, Ybot + thinnerBy);
            glVertex(x2, Ybot + thinnerBy);
            glVertex(x2, Ytop - thinnerBy);
            glVertex(x1, Ytop - thinnerBy);
        }

        //
        //  Draw the background grey area for the time line
        //

        setupProjection(d.x, d.y, event.domainVerticalFlip());

        if (!_settings.drawInMargin)
        {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        glColor(bg);

        drawRect(GL_QUADS,
                 Vec2(0, _Y0),
                 Vec2(w, _Y0),
                 Vec2(w, _Y0+t),
                 Vec2(0, _Y0+t));

        //
        //  Draw Audio
        //


        if (audioH > 0)
        {
            let startX = hm0,
                endX   = w - hm1,
                rangeX = float(endX - startX),
                texID  = audioTextureID();

            //  deb ("texID %s\n" % texID);
            //  deb ("startX %s endX %s Ybot %s audioH %s\n" % (startX,endX,Ybot,audioH));

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            if (texID != 0)
            {
                glEnable(GL_TEXTURE_RECTANGLE_ARB);
                glBindTexture(GL_TEXTURE_RECTANGLE_ARB, texID);
                glMatrixMode(GL_TEXTURE);
                glLoadIdentity();
                glMatrixMode(GL_MODELVIEW);
                glTexEnv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

                glColor(fg);    // controls base color of audio display
                glBegin(GL_QUADS);
                // XXX
                if (false)
                {
                    glTexCoord(0, 0);   glVertex(startX, Ytop);         
                    glTexCoord(0, 2047.0);   glVertex(endX, Ytop);           
                    glTexCoord(float(100)-1.0, 2047.0);   glVertex(endX, Ytop+audioH);   
                    glTexCoord(float(100)-1.0, 0);   glVertex(startX, Ytop+audioH);
                }
                glTexCoord(0, 0);   glVertex(startX, Ybot);         
                glTexCoord(0, 2047.0);   glVertex(endX, Ybot);           
                glTexCoord(float(audioH)-1.0, 2047.0);
                glVertex(endX, Ybot + audioH);   
                glTexCoord(float(audioH)-1.0, 0);
                glVertex(startX, Ybot + audioH);
                glEnd();

                glDisable(GL_TEXTURE_RECTANGLE_ARB);
            }

            glColor(fg);
            glBegin(GL_LINES);
            glVertex(startX, Ybot + audioH*.5);
            glVertex(endX, Ybot + audioH*.5);
            glEnd();

            glDisable(GL_BLEND);
        }

        if (_drag && _delayedInFrame != _delayedOutFrame)
        {
            let inF  = min(_delayedInFrame, _delayedOutFrame),
                outF = max(_delayedInFrame, _delayedOutFrame),
                inX  = (float(inF  - fs) / float(r)) * _tlw + hm0,
                outX = (float(outF - fs) / float(r)) * _tlw + hm0;

            glBegin(GL_QUADS);
            motionScopeQuad (config.tlBoundsColor, inX, outX, 0);
            glEnd();
        }

        {
            //
            //  Draw the frame line with in/out points
            //

            let dlFact      = 0.12,
                dFact       = 1.0-dlFact,
                lFact       = 1.0+dlFact,
                boundsDark  = config.tlBoundsColor * Color(dFact, dFact, dFact, 1),
                boundsLight = config.tlBoundsColor * Color(lFact, lFact, lFact, 1),
                rc          = config.tlRangeColor * 1.1,
                rangeDark   = rc * Color(dFact, dFact, dFact, 1),
                rangeLight  = rc * Color(lFact, lFact, lFact, 1),
                ins         = sequenceBoundaries(),
                startX      = hm0,
                endX        = w - hm1,
                rangeX      = float(endX - startX);

            if (audioH == 0)
            {
                glBegin(GL_QUADS);
                motionScopeQuad (config.tlBoundsColor, startX, endX, 0);
                glEnd();
            }

            //don't draw alternatng colors for sources in moscope.

            if (false)
            {
            glBegin(GL_QUADS);

            if (ins.size() > 1)
            //
            //  Draw alternating colors in timeline, representing
            //  different sources.
            //
            {
                for_index (q; ins)
                {
                    let i            = q + 1,
                        color        = if (i % 2 == 1) then boundsDark else boundsLight,
                        fracStart    = if (i == 1) then 0.0 else
                                       float(ins[i-1] - frameStart())/r,
                        fracEnd      = if (i == ins.size()) then 1.0 else 
                                       float(ins[i] - frameStart())/r,
                        sectionStart = startX + int (fracStart * rangeX),
                        sectionEnd   = startX + int (fracEnd   * rangeX);

                    motionScopeQuad (color, sectionStart, sectionEnd, 1);
                }

                let withinInOut = false;

                for_index (q; ins)
                {
                    let i         = q + 1,
                        color     = if (i % 2 == 1) then rangeDark else rangeLight,
                        fracStart = if (i == 1) then 0.0 else
                                    float(ins[i-1] - frameStart())/r,
                        fracEnd   = if (i == ins.size()) then 1.0 else 
                                    float(ins[i] - frameStart())/r,
                        last      = false;

                    if (!withinInOut && fracEnd > ip)
                    {
                        withinInOut = true;
                        fracStart = ip;
                    }

                    if (withinInOut && fracEnd >= op)
                    {
                        fracEnd = op;
                        last = true;
                    }

                    if (last || withinInOut)
                    {
                        let sectionStart = startX + int (fracStart * rangeX),
                            sectionEnd   = startX + int (fracEnd   * rangeX);

                        motionScopeQuad (color, sectionStart, sectionEnd);

                        if (last) break;
                    }
                }
            }
            else
            {
                motionScopeQuad (config.tlBoundsColor, startX, endX, 1);
                motionScopeQuad (config.tlRangeColor, hm0 + ip * _tlw, hm0 + ope * _tlw);
            }
            glEnd();
            }

            //
            //  Turn on anti-aliasing + blending here and set the default
            //  line width
            //

            glEnable(GL_BLEND);
            //glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
            //glEnable(GL_POINT_SMOOTH);
            //glEnable(GL_LINE_SMOOTH);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glLineWidth(1.5 * devicePixelRatio);

            //
            //  In / Out points
            //

            //  Always show in/out in moscope

                gltext.color(fg);
                gltext.size(config.tlBoundsTextSize * devicePixelRatio);

                let sin  = _frameFunc(fi),
                    sout = _frameFunc(fo),
                    bin  = gltext.bounds(sin),
                    bout = gltext.bounds(sout),
                    win  = bin[2],
                    wout = bout[2];

                if (!isUndiscoveredFrame(fi))
                {
                    gltext.writeAt(hm0 - win - 4,
                                   Ybot,
                                   sin);
                }

                if (!isUndiscoveredFrame(fo))
                {
                    gltext.writeAt(hm0 + _tlw + 4,
                               Ybot,
                               sout);
                }

                gltext.size(config.msFrameTextSize * devicePixelRatio);


            //
            //  Draw the marked frames
            //

            let mfs = markedFrames();
            glColor(config.tlMarkedColor);

            glLineWidth(1.0 * devicePixelRatio);
            glBegin(GL_LINES);

            for_each (mf; mfs)
            {
                if (mf == f) frameIsMarked = true;
                if (mf == _phantomFrame) phantomFrameIsMarked = true;
                let xframe = ((mf - fs)/ float(r)) * _tlw + hm0;
                if (xframe >= hm0 && xframe <= hm0 + _tlw)
                {
                    glVertex(xframe, Ybot - 1);
                    glVertex(xframe, Ytop);
                }
            }

            glEnd();
            glLineWidth(1.5 * devicePixelRatio);

            glPointSize(3.2 * devicePixelRatio);
            glBegin(GL_POINTS);

            for_each (mf; mfs)
            {
                let xframe = ((mf - fs)/ float(r)) * _tlw + hm0;
                glVertex(xframe, Ybot - 1);
            }

            glEnd();

        }
        //
        //  Draw in/out nudge buttons
        //

        glColor(1.3*config.bgVCRButton);

        _nudgeY = if (audioH > 0) then ((2*vm0) + audioH)/2.0 else vm0 + 16;
        draw(triangleGlyph, hm0 - 17 * devicePixelRatio, _Y0 + _nudgeY, 0,   8 * devicePixelRatio, false);
        draw(triangleGlyph, hm0 - 6 * devicePixelRatio,  _Y0 + _nudgeY, 180, 8 * devicePixelRatio, false);

        draw(triangleGlyph, hm0 + _tlw + 6 * devicePixelRatio,  _Y0 + _nudgeY, 0,   8 * devicePixelRatio, false);
        draw(triangleGlyph, hm0 + _tlw + 17 * devicePixelRatio, _Y0 + _nudgeY, 180, 8 * devicePixelRatio, false);

        //
        //  Draw the phantom frame number and tick (colored if necessary)
        //
        let frameFunc = _frameFunc,
            pixelsPerFrame = _tlw/r,
            sin  = _frameFunc(fs),
            sout = _frameFunc(fe),
            bin  = gltext.bounds(sin),
            bout = gltext.bounds(sout),
            win  = bin[2],
            wout = bout[2],
            wlim = pixelsPerFrame - 2,
            drawNumbersOnOne = (win < wlim && wout < wlim),
            drawNumbersOnFive = (win < 5*wlim && wout < 5*wlim),
            drawNumbersOnTen = (win < 10*wlim && wout < 10*wlim);

        \: drawTickAndFrame (void; Color c, int x, int frame, int pixelsPerFrame, bool force = false)
        {
            if (xframe < hm0 || xframe >= hm0 + this._tlw) return;
            if (frame == f && !force) return;
            let sFrame = if (this._usingSourceFrames) then sourceFrameMapping(frame) else frame;

            let maxHeight = max(audioH,tlh),
                height = maxHeight;
            if ((sFrame % 10) == 0)
            {
                if (10*pixelsPerFrame < 6) return;
            }
            else 
            if ((sFrame % 5) == 0) 
            {
                if (5*pixelsPerFrame < 6) return;
                height = 0.8*maxHeight;
            }
            else                  
            {
                if (pixelsPerFrame < 6) return;
                height = 0.6*maxHeight;
            }

            let myFrame = floor(xframe+0.5);
            //  XXX
            if (false)
            {
                glColor(fg);
                glLineWidth(1.0 * devicePixelRatio);
                glBegin(GL_LINES);
                glVertex(xframe-1, Ybot - 1 + (maxHeight-height)/2);
                glVertex(xframe-1, Ybot + 1 + maxHeight - (maxHeight-height)/2);
                glEnd();
                glColor(bg);
                glLineWidth(1.0 * devicePixelRatio);
                glBegin(GL_LINES);
                glVertex(xframe, Ybot - 1 + (maxHeight-height)/2);
                glVertex(xframe, Ybot + 1 + maxHeight - (maxHeight-height)/2);
                glEnd();
            }
            //  XXX
            if (true)
            {
                glColor(.9,.6,.6,1);
                glLineWidth(1.0 * devicePixelRatio);
                glBegin(GL_LINES);
                glVertex(myFrame, Ybot - 1 + (maxHeight-height)/2);
                glVertex(myFrame, Ybot + 1 + maxHeight - (maxHeight-height)/2);
                glColor(0,0,0,1);
                glVertex(myFrame+1, Ybot - 1 + (maxHeight-height)/2);
                glVertex(myFrame+1, Ybot + 1 + maxHeight - (maxHeight-height)/2);
                glEnd();
            }
            else
            {
                glColor(c);
                glLineWidth(2.0 * devicePixelRatio);
                glBegin(GL_LINES);
                glVertex(xframe, Ybot - 1 + (maxHeight-height)/2);
                glVertex(xframe, Ybot + 1 + maxHeight - (maxHeight-height)/2);
                glEnd();
            }

            gltext.color(c);

            if (drawNumbersOnOne ||
                    (drawNumbersOnFive && (sFrame % 5 == 0)) ||
                    (drawNumbersOnTen  && (sFrame % 10 == 0)))
            {
                let pfp      = float(frame - fs) / float(r),
                    pxframe  = pfp * _tlw + hm0,
                    pxframe1 = (pfp + 1.0/float(r)) * _tlw + hm0,
                    ps       = frameFunc(frame),
                    psb      = gltext.bounds(ps),
                    psW      = psb[2] + psb[0],
                    xpf      = float(pxframe+pxframe1)/2.0 - psW / 2.0,
                    ypf      = Ybot + 3 + max(audioH,tlh);

                if (!isUndiscoveredFrame(frame))
                {
                    gltext.writeAtNL(xpf, ypf, ps);
                }
            }
        }

        let ptc     = 0.8*fg,
            ptcolor = Color(ptc[0],ptc[1],ptc[2],1),
            pmcolor = config.tlMarkedColor;

        if (10*pixelsPerFrame >= 6)
        {
            //
            //  Draw marked frames/frame numbers
            //
            int start = fs;
            for_each (mf; markedFrames())
            {
                //  draw unmarked frames
                for (int pf = start; pf < mf; ++pf)
                {
                let xframe = ((pf - fs)/ float(r)) * _tlw + hm0;
                    drawTickAndFrame (ptcolor, xframe, pf,
                    pixelsPerFrame);
                }

                //  draw marked frames

                let xframe = ((mf - fs)/ float(r)) * _tlw + hm0;
                drawTickAndFrame (pmcolor, xframe, mf, pixelsPerFrame);
                
                //  IF we entered the in/out region, start checking next mark from the last mark.
                if (mf + 1 > fs) start = mf + 1;
            }

            //  draw unmarked frames if there were no marked frames

            for (int pf = start; pf <= fe; ++pf)
            {
                let xframe = ((pf - fs)/ float(r)) * _tlw + hm0;
                drawTickAndFrame (ptcolor, xframe, pf, pixelsPerFrame);
            }
        }

        //
        //  Draw in/out caps
        //

        glColor(config.tlInOutCapsColor);
        glLineWidth(2 * devicePixelRatio);
        glBegin(GL_LINES);
        glVertex(hm0 + ip * _tlw, Ybot - 1);
        glVertex(hm0 + ip * _tlw, Ybot + 1 + max(audioH,tlh));
        glVertex(hm0 + op * _tlw, Ybot - 1);
        glVertex(hm0 + op * _tlw, Ybot + 1 + max(audioH,tlh));
        glEnd();
        //
        //  Draw phantom frame.
        //
        if (state.userActive &&
            _phantomFrameDisplay && _pointerInMotionScope &&
            _phantomFrame >= fs && _phantomFrame < (fe+1.0) && _phantomFrame != f)
        {
            if (phantomFrameIsMarked)
            {
                glColor(pmcolor);
                gltext.color(pmcolor);
            }
            else
            {
                glColor(fg);
                gltext.color(ptcolor);
            }

            let pfp        = float(_phantomFrame - fs) / float(r),
                pxframe    = pfp * _tlw + hm0,
                pxframe1   = (pfp + 1.0/float(r)) * _tlw + hm0,
                ps         = _frameFunc(_phantomFrame),
                psb        = gltext.bounds(ps),
                psW        = psb[2] + psb[0];

            glBegin(GL_LINES);
            glVertex(pxframe, Ybot - 1);
            glVertex(pxframe, Ybot + 1 + max(audioH,tlh));
            glEnd();

            let xpf  = pxframe - psW / 2.0,
                ypf  = Ybot + 3 + max(audioH,tlh),
                psw  = (psb[2] + psb[0]) / 2.0,
                psh  = -psb[1] + psb[3],
                psY  = Ybot,
                psYh = psY + 3 + max(audioH,tlh);

            if (xdiff > 2)
            {
                let fp = float(floor(_phantomFrame) - fs) / float(r),
                    x0 = fp * _tlw + hm0,
                    x1 = (fp + 1.0/float(r)) * _tlw + hm0;

                //glColor(hlColor * Color(.7,.7,.7,1.0));
                glColor(hlColor * Color(.8,.8,.8,1.0));
                glBlendFunc(GL_ONE, GL_ONE);
                glBegin(GL_QUADS);
                glVertex(x0, psY - 1);
                glVertex(x0, psYh - 2);
                glVertex(x1, psYh - 2);
                glVertex(x1, psY - 1);
                glEnd();
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            }

            if (!isUndiscoveredFrame(_phantomFrame))
            {
                glColor(bg);
                glBegin(GL_QUADS);
                glVertex(pxframe - psw, psYh);
                glVertex(pxframe + psw, psYh);
                glVertex(pxframe + psw, psYh + psh);
                glVertex(pxframe - psw, psYh + psh);
                glEnd();

                gltext.writeAtNL(xpf, ypf, ps);
            }
        }

        //
        //  frame number and tick
        //

        let fcol  = if frameIsMarked then config.tlMarkedColor else fg,
            sw    = (sb[2] + sb[0]) / 2.0,
            sh    = -sb[1] + sb[3],
            sY    = Ybot,
            sYh   = sY + 3 + max(audioH,tlh),
            ffilt = if playing then .1 else 1.0,
            fdiff = -sb[2] / 2.0 * ffilt + _frameTextOffset * (1.0 - ffilt),
            ftx   = xframeMid + fdiff,
            dframe= (xframe >= hm0 && xframe < hm0 + _tlw);

        _frameTextOffset = fdiff;

        if (dframe)
        {
            glColor(fcol);
            gltext.color(fcol);

            glLineWidth(2.0 * devicePixelRatio);
            glBegin(GL_LINES);
            glVertex(xframe, sY - 1);
            glVertex(xframe, sYh - 2);
            glEnd();

            if (_settings.showFrameDirection)
            {
                let fwd    = inc() > 0,
                    xg     = if fwd then xframeMid - fdiff + 8.0 else ftx - 6.0,
                    yg     = sYh + (-mxb[1] + mxb[3]) / 2.0,
                    angle  = if fwd then 180.0 else 0.0,
                    radius = 8.0;

                if (playing)
                {
                    glColor(config.bgVCRButton);
                    draw(triangleGlyph, xg, yg, angle, radius, false);
                }

                glColor(config.bgVCRButton * .8);
                draw(triangleGlyph, xg, yg, angle, radius, true);
            }

            glColor(bg);
            glBegin(GL_QUADS);
            glVertex(xframeMid - sw, sYh);
            glVertex(xframeMid + sw, sYh);
            glVertex(xframeMid + sw, sYh + sh);
            glVertex(xframeMid - sw, sYh + sh);
            glEnd();

            if (!isUndiscoveredFrame(f)) 
            {
                gltext.writeAtNL(ftx, sYh, s);
            }

            gltext.color(fg);

            if (xdiff > 2)
            {
                glColor(hlColor);
                glBlendFunc(GL_ONE, GL_ONE);
                glBegin(GL_QUADS);
                glVertex(xframe, sY - 1);
                glVertex(xframe, sYh - 2);
                glVertex(xframe1, sYh - 2);
                glVertex(xframe1, sY - 1);
                glEnd();
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                glLineWidth(1.0 * devicePixelRatio);
                glColor(fcol);
                glBegin(GL_LINES);
                glVertex(xframe1, sY - 1);
                glVertex(xframe1, sYh - 2);
                glEnd();
            }
        }

        glLineWidth(1.5 * devicePixelRatio);

        //
        //  Draw the in/out info if needed
        //

        if (dio && !isUndiscoveredFrame(f))
        {
            gltext.writeAtNL(10, Ytop + 3, "xxx" /*"%d" % (f - fi + 1)*/);
        }

        if (!_drag && _delayedInFrame != _delayedOutFrame)
        {
            setInPoint  (min(_delayedInFrame, _delayedOutFrame));
            setOutPoint (max(_delayedInFrame, _delayedOutFrame));
            _delayedInFrame = _delayedOutFrame = -1;
        }

        glDisable(GL_BLEND);

    }
}   //  Class MotionScope

\: startUp (void; State state)
{
    if (state.motionScope eq nil)
    {
        let ms = MotionScope("motionScope", state.timeline); 
        state.motionScope = ms;
        if (!ms._active) ms.toggle();
    }
    else
    {
        if (!state.motionScope._active) state.motionScope.toggle();
    }
}

} // motion_scope
