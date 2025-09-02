//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
module: timeline {
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

\: deb(void; string s) { if (false) print(s); }

//----------------------------------------------------------------------
//
//  Timeline
//

class: Timeline : Widget
{
    FrameStringFunc := (string;int);
    TextBounds := float[4];

    class: Settings
    {
        bool showVCRButtons;
        bool drawInMargin;
        bool drawAtTopOfView;
        bool showInOut;
        bool showInputName;
        bool showFrameDirection;
        int frameDisplayFormat;

        method: writeSettings (void; )
        {
            writeSetting ("Timeline", "showVCRButtons2", SettingsValue.Bool(showVCRButtons));
            writeSetting ("Timeline", "drawInMargin", SettingsValue.Bool(drawInMargin));
            writeSetting ("Timeline", "drawAtTopOfView", SettingsValue.Bool(drawAtTopOfView));
            writeSetting ("Timeline", "frameDisplayFormatNew", SettingsValue.Int(frameDisplayFormat));
            writeSetting ("Timeline", "showInOut", SettingsValue.Bool(showInOut));
            writeSetting ("Timeline", "showInputName", SettingsValue.Bool(showInputName));
            writeSetting ("Timeline", "showFrameDirection", SettingsValue.Bool(showFrameDirection));
        }

        method: readSettings (void; )
        {
            let SettingsValue.Bool b1 = readSetting ("Timeline", "showVCRButtons2", SettingsValue.Bool(false));
            showVCRButtons = b1;

            let SettingsValue.Bool b2 = readSetting ("Timeline", "drawInMargin", SettingsValue.Bool(true));
            drawInMargin = b2;

            let SettingsValue.Bool b3 = readSetting ("Timeline", "drawAtTopOfView", SettingsValue.Bool(false));
            drawAtTopOfView = b3;

            let SettingsValue.Bool b4 = readSetting ("Timeline", "showInOut", SettingsValue.Bool(false));
            showInOut = b4;

            let SettingsValue.Bool b5 = readSetting ("Timeline", "showInputName", SettingsValue.Bool(false));
            showInputName = b5;

            let SettingsValue.Int i1 = readSetting ("Timeline", "frameDisplayFormatNew", SettingsValue.Int(0));
            frameDisplayFormat = i1;

            let SettingsValue.Bool b6 = readSetting ("Timeline", "showFrameDirection", SettingsValue.Bool(false));
            showFrameDirection = b6;
        }

        method: Settings (Settings; )
        {
            try
            {
                readSettings();
            }
            catch (...)
            {
                ; // ignore read failure
            }
        }
    }

    Settings        _settings;
    float           _X0;
    float           _Y0;
    float           _X1;
    float           _H;
    float           _W;
    float           _vm0;
    float           _tlh;
    float           _tlw;
    float           _Ybot;
    float           _Ytop;
    float           _ipCapX;
    float           _opCapX;
    float           _controlSize;
    float           _phantomFrameX;
    float           _phantomFrame;
    bool            _pointerInTimeline;
    bool            _pointerInInCap;
    bool            _pointerInOutCap;
    bool            _pointerInInOutRegion;
    bool            _clickedInInOutRegion;
    bool            _phantomFrameDisplay;
    bool            _drawInOut;
    bool            _drag;
    int             _dragStartFrame;
    FrameStringFunc _frameFunc;
    FrameStringFunc _totalFunc;
    bool            _displayMbps;
    bool            _displayAudioCache;
    float           _frameTextOffset;
    float           _inOutRegionOffsetLeft;
    float           _inOutRegionOffsetRight;
    string          _mainNode;
    int[]           _sequenceBoundaries;
    bool            _sequenceBoundariesDirty;

    class: Locations
    {
        Vec2       domain;
        int        width;
        int        height;
        bool       showControls;
        int        timelineHeight;
        TextBounds textBounds;
        int        hMargin;
        int        vMargin;
        int        something;
        int        bMargin;
        int        tMargin;
        int        t;
        int        baseY;
    }

    method: fullWidth (int; Event event)
    {
        let d = event.domain(),
            drawControls = _settings.showVCRButtons && (_controlSize * 14 < d.x);
        return d.x - (if drawControls then _controlSize * 6 else 0);
    }

    method: renderLocations (Locations; Event event)
    {
        let d                = event.domain(),
            isRenderEvent    = event.name() == "render",
            devicePixelRatio = (if isRenderEvent then devicePixelRatio() else 1.0),
            drawControls     = _settings.showVCRButtons && (_controlSize * 14 * devicePixelRatio < d.x),
            tbounds          = gltext.bounds("||||"),
            t                = floor((5 + 8 + 5)*devicePixelRatio + tbounds[3] + 0.5); //floor(_vm0 + _tlh + vm1 + mxb[3] + 0.5),

        return Locations(d,
                         d.x - (if drawControls then _controlSize * devicePixelRatio * 6 else 0), // window width
                         d.y,                   // window height
                         drawControls,
                         8*devicePixelRatio,    // timeline height
                         tbounds,
                         100*devicePixelRatio,  // horizontal margin
                         100*devicePixelRatio,  // vertical margin
                         0,
                         5*devicePixelRatio,    // bottom  margin
                         5*devicePixelRatio,    // top margin
                         t,
                         if _settings.drawAtTopOfView then floor(d.y - t + 0.5) else 0);
    }

    method: frameAtPointer (int; Locations loc, Event event)
    {
        let {d, w, h, drawControls, tlh, mxb, hm0, hm1, thm1, vm0, vm1, t, Y} = loc;

        let x  = event.pointer().x,
            X0 = hm0,
            X1 = w - hm1,
            fs = frameStart(),
            fe = frameEnd();

        return (x - X0) / (X1 - X0) * (fe - fs + 1) + fs;
    }

    method: handleLeave (void; Event event)
    {
        deb ("handleLeave\n");
        _pointerInTimeline = false;
        redraw();
        event.reject();
    }

    method: processPointer (bool; Event event)
    {
        let gp = event.pointer(),
            p  = event.relativePointer(),
            modified = false,
            devicePixelRatio = devicePixelRatio();

        if (contains (gp))
        {
            if (!_pointerInTimeline)
            {
                _pointerInTimeline = true;
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

            // The remaining comparisons are done in device pixels so adjust gp accordingly
            gp *= devicePixelRatio;
            
            if (    gp.x >= _ipCapX-4 && gp.x <= _ipCapX+4 &&
                    gp.y >= _Ybot-2   && gp.y <= _Ytop+2)
            {
                deb ("in inCap\n");
                _pointerInInCap = true;
            }
            else
            {
                deb ("outside inCap\n");
                _pointerInInCap = false;
            }

            if (    gp.x >= _opCapX-4 && gp.x <= _opCapX+4 &&
                    gp.y >= _Ybot-2   && gp.y <= _Ytop+2)
            {
                deb ("in outCap\n");
                _pointerInOutCap = true;
            }
            else
            {
                deb ("outside outCap\n");
                _pointerInOutCap = false;
            }

            if (    gp.x >= _ipCapX+6 && gp.x <= _opCapX-6 &&
                    gp.y >= _Ybot     && gp.y <= _Ytop)
            {
                deb ("in in/out region\n");
                _pointerInInOutRegion = true;
            }
            else
            {
                deb ("outside in/out region\n");
                _pointerInInOutRegion = false;
            }
        }
        else
        {
            deb ("outside timeline\n");
            if (_pointerInTimeline)
            {
                modified = true;
                redraw();
            }
            _pointerInTimeline = false;
            _pointerInInCap = false;
            _pointerInOutCap = false;
            _pointerInInOutRegion = false;
        }

        return modified;
    }

    method: handleMotion (void; Event event)
    {
        let gp = event.pointer();
        let p = event.relativePointer();

        processPointer (event);
        event.reject();
    }

    method: whichMargin (int; )
    {
        if (_settings.drawAtTopOfView) then 2 else 3;
    }


    method: scrubSetFrameAndInOut (void; int frame)
    {
        deb ("scrubSetFrameAndInOut frame %s inInCap %s inOutCap %s\n" %
                (frame, _pointerInInCap, _pointerInOutCap));

        State state = data();
        if (_pointerInInCap)
        {
            setInPoint(min(frame,outPoint()-1));
        }
        else
        if (_pointerInOutCap)
        {
            setOutPoint(max(frame,inPoint()+1));
        }
        else
        if (_clickedInInOutRegion)
        {
            let targetIn  = frame - _inOutRegionOffsetLeft,
                finalIn   = max(frameStart(), min(targetIn, frameEnd()-1)),
                targetOut = frame - _inOutRegionOffsetRight,
                finalOut  = min(frameEnd(), max(targetOut, frameStart()+1));

            if (finalIn < outPoint())
            {
                setInPoint(finalIn);
                setOutPoint(finalOut);
            }
            else
            {
                setOutPoint(finalOut);
                setInPoint(finalIn);
            }
        }
        else
        {
            stop();
            if (    state.scrubClamps &&
                    _dragStartFrame >= inPoint() &&
                    _dragStartFrame <= outPoint())
            {
                if (frame > outPoint()) frame = outPoint();
                if (frame < inPoint())  frame = inPoint();
            }
            if (state.scrubAudio) scrubAudio(true, 1.0 / fps(), 1);
            else scrubAudio(state.scrubAudio, 0, 0);
        }
        if (!isPlaying()) setFrame (max(frameStart(), min(frame, frameEnd())));
    }

    \: scrubSetFrame (void; int frame)
    {
        State state = data();
        stop();
        if (state.scrubAudio) scrubAudio(true, 1.0 / fps(), 1);
        else scrubAudio(state.scrubAudio, 0, 0);
        setFrame(frame);
    }

    method: setFrameAndInOut (void; int f)
    {

        deb ("setFrameAndInOut f %s _drag %s\n" % (f,_drag));
        if (_pointerInInCap)
        {
            deb ("    pointerInInCap\n");
            if (!isPlaying()) setFrame(inPoint());
            //setInPoint(f);
        }
        else
        if (_pointerInOutCap)
        {
            deb ("    pointerInOutCap\n");
            if (!isPlaying()) setFrame(outPoint());
        }
        else
        if (_pointerInInOutRegion &&
                (inPoint() != frameStart() || outPoint() != frameEnd()))
        {
            deb ("    pointerInOutRegion\n");
            _clickedInInOutRegion = true;
            _inOutRegionOffsetLeft =  (f - inPoint());
            _inOutRegionOffsetRight =  (f - outPoint());
            if (!isPlaying()) setFrame(f);
        }
        else
        {
            deb ("    else\n");
            _clickedInInOutRegion = false;
            stop();
            setFrame(f);
        }
    }

    method: clickFunction (void; Event event, (void;int) F)
    {
	//   Disregard drag events if we didn't mouse-down in this widget.
	//
	if (!_drag && (event.name() == "pointer-1--drag" || event.name() == "stylus-pen--drag"))
	{
            event.reject();
            return;
	}
        if (filterLiveReviewEvents()) {
            sendInternalEvent("live-review-blocked-event");
            return;
        }
        State state = data();
        let loc = renderLocations(event);
        let {d, w, h, drawControls, tlh, mxb, hm0, hm1, thm1, vm0, vm1, t, Y} = loc;

        let x0 = hm0,
            x1 = w - hm1,
            x  = event.pointer().x,
            fs = frameStart(),
            fe = frameEnd(),
            f  = frameAtPointer(loc, event);

        _phantomFrameX = event.relativePointer().x;
        _phantomFrame = f;

        if (_drag == false) _dragStartFrame = int(f);
        _drag = true;
        if (x > w) return;

        F(f);
        redraw(); // to make sure audio waveform gets updated
    }

    \: globalTimeCode (string; int frame)
    {
        if (fps() == 0.0) return "";

        let f    = frame - 1,
            ifps = int(fps() + 0.5),
            sec  = int(f / ifps),
            min  = sec / 60,
            hrs  = min / 60,
            frm  = int(f % ifps);

        return if hrs == 0
            then "%02d:%02d:%02d" % (min, sec % 60, frm)
            else "%02d:%02d:%02d:%02d" % (hrs, min % 60, sec % 60, frm);
    }

    \: seconds (string; int frame)
    {
        if (fps() == 0.0) return "";

        let f   = frame - 1,
            sec = float(f) / fps();

        "%.2f" % sec;
    }

    \: sourceTimeCode (string; int frame)
    {
        try
        {
            let infos = metaEvaluate(frame);

            string name = "";
            int sourceFrame = frame;
            float fps = 0.0;
            //string fileTC = "";

            if (!infos.empty())
            {
                for (int i = infos.size() - 1; i >= 0; i--)
                {
                    let info = infos[i];

                    if (info.nodeType == "RVFileSource")
                    {
                        name = info.node;
                        sourceFrame = info.frame;
                        break;
                    }
                }
                if (name == "") return "__:__:__";

                float sfps = getFloatProperty("%s.group.fps" % name).front();
                fps = sfps;

                if (fps == 0.0)
                {
                    let attrs = sourceAttributes(name + ".0");
                    for_each (a; attrs)
                    {
                        if (a._0 == "Timecode") return a._1;
                        if (a._0 == "fps" || a._0 == "FPS") fps = float(a._1);
                        //if (a._0 == "DPX-TV/TimeCode") fileTC = a._1;
                    }
                }
            }
            else
            {
                sourceFrame = frame;
            }

            if (fps == 0.0) fps = fps();
            if (fps == 0.0) return "__:__:__";

            let f    = sourceFrame,
                ifps = int(fps + 0.5),
                sec  = int(f / ifps),
                min  = sec / 60,
                hrs  = min / 60,
                frm  = int(f % ifps);

            //if (fileTC != "") _phantomFrameDisplay = false;
            //if fileTC == "" then "%02d:%02d:%02d" % (min, sec % 60, frm) else fileTC;
            return if hrs == 0
                then "%02d:%02d:%02d" % (min, sec % 60, frm)
                else "%02d:%02d:%02d:%02d" % (hrs, min % 60, sec % 60, frm);
        }
        catch (exception exc)
        {
            print("CAUGHT %s\n" % exc);
        }
        catch (...)
        {
            ; // nothing
        }

        return "__:__:__";
    }

    \: footage (string; int frame)
    {
        let ft = int(frame / 16),
            frm = int(frame % 16);

        "%02d-%02d" % (ft, frm);
    }

    method: getSourceFrame(string; int frame)
    {
        try
        {
            let isLayout = (nodeType(viewNode()) == "RVLayoutGroup"),
		//  rig this to always show the source frame of _mainNode, since it's almost always the same
		//  for all inputs
                sFrame   = if (false && isLayout) then sourceFrame(frame, _mainNode) else sourceFrame(frame);

            return "%d" % sFrame;
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
        redraw();
    }

    method: showTimeCode (void;)
    {
        _frameFunc = globalTimeCode;
        _totalFunc = \: (string; int f) { globalTimeCode(f) + " min"; };
        _phantomFrameDisplay = true;
        redraw();
    }

    method: showSourceTimeCode (void;)
    {
        _frameFunc = sourceTimeCode;
        _totalFunc = \: (string; int f) { globalTimeCode(f) + " min"; };
        _phantomFrameDisplay = true;
        redraw();
    }

    method: showSeconds (void;)
    {
        _frameFunc = seconds;
        _totalFunc = \: (string; int f) { seconds(f) + " sec"; };
        _phantomFrameDisplay = true;
        redraw();
    }

    method: showFootage (void;)
    {
        _frameFunc = footage;
        _totalFunc = \: (string; int f) { footage(f) + " ft"; };
        _phantomFrameDisplay = true;
        redraw();
    }

    method: showFrames (void;)
    {
        _frameFunc = \: (string; int f) { string(f); };
        _totalFunc = \: (string; int f) { "%d frames" % f; };
        _phantomFrameDisplay = true;
        redraw();
    }

    method: optShowVCRButtons (void; Event event)
    {
        _settings.showVCRButtons = !_settings.showVCRButtons;
        _settings.writeSettings();
        redraw();
    }

    method: optShowFrameDirection (void; Event event)
    {
        _settings.showFrameDirection = !_settings.showFrameDirection;
        _settings.writeSettings();
        redraw();
    }

    method: isShowingVCRButtons (int;)
    {
        if _settings.showVCRButtons then CheckedMenuState else UncheckedMenuState;
    }

    method: isShowingFrameDirection (int;)
    {
        if _settings.showFrameDirection then CheckedMenuState else UncheckedMenuState;
    }

    method: optDrawTimelineOverImagery (void; Event event)
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

    method: isDrawingTimelineOverImagery (int;)
    {
        if _settings.drawInMargin then UncheckedMenuState else CheckedMenuState;
    }

    method: optDrawTimelineAtTopOfView (void; Event event)
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

    method: isDrawingTimelineAtTopOfView (int;)
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
        case (format)
        {
            0 -> { showSourceFrame(); }
            1 -> { showTimeCode(); }
            2 -> { showFrames(); }
            3 -> { showFootage(); }
            4 -> { showSourceTimeCode(); }
            5 -> { showSeconds(); }
        }

        _settings.frameDisplayFormat = format;
        redraw();
    }

    method: setShowInputName (void; bool b)
    {
        _settings.showInputName = b;
        _settings.writeSettings();
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

    method: optScrubClamps (void; Event event)
    {
        State s = data();
        s.toggleScrubClamps();
    }

    method: optStepWraps (void; Event event)
    {
        State s = data();
        s.toggleStepWraps();
    }

    method: optInputName (void; Event event)
    {
        setShowInputName(!_settings.showInputName);
        redraw();
    }

    method: isShowingInputName (int;)
    {
        if _settings.showInputName then CheckedMenuState else UncheckedMenuState;
    }

    method: isStepWrapping (int;)
    {
        State s = data();
        if s.stepWraps then CheckedMenuState else UncheckedMenuState;
    }

    method: isScrubClamping (int;)
    {
        State s = data();
        if s.scrubClamps then CheckedMenuState else UncheckedMenuState;
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

    method: hasInOutSubRange (int;)
    {
        if (inPoint() != frameStart() || outPoint() != frameEnd())
                      then UncheckedMenuState
                      else DisabledMenuState;
    }

    method: inputAtFrame (string; int frame)
    {
        inputNodeUserNameAtFrame(frame, _mainNode);
    }

    method: sourceAtFrame (string; int frame)
    {
        let info = sourceMetaInfoAtFrame(frame, _mainNode);
        if info eq nil then "" else uiName(info.node);
    }

    method: inputAndSourceAtFrame (string; int frame)
    {
        "%s / %s" % (inputAtFrame(frame), sourceAtFrame(frame));
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

        if (isCurrentFrameIncomplete() || !_pointerInTimeline)
        {
            event.reject();
            return;
        }

        let fs = frameStart(),
            fe = frameEnd(),
            pointerWasInTimeline = _pointerInTimeline;

        _pointerInTimeline = false;

        Menu lastHalf = Menu {
            {"_", nil},
            {"Time/Frame Display", nil, nil, disabledItem},
            {"   Global Frame Numbers", setFrameDisplay(2,), nil, isDisplayFormat(2)},
            {"   Source Frame Numbers", setFrameDisplay(0,), nil, isDisplayFormat(0)},
            {"   Global Time Code Display", setFrameDisplay(1,), nil, isDisplayFormat(1)},
            {"   Source Time Code Display", setFrameDisplay(4,), nil, isDisplayFormat(4)},
            {"   Global Seconds", setFrameDisplay(5,), nil, isDisplayFormat(5)},
            {"   Footage Display", setFrameDisplay(3,), nil, isDisplayFormat(3)},
            {"_", nil},
            {"Configure", Menu {
                {"Show Play Controls", optShowVCRButtons, nil, isShowingVCRButtons},
                {"Draw Timeline Over Imagery", optDrawTimelineOverImagery, nil, isDrawingTimelineOverImagery},
                {"Position Timeline At Top", optDrawTimelineAtTopOfView, nil, isDrawingTimelineAtTopOfView},
                {"Show In/Out Frame Numbers", optShowInOutTime, nil, isShowingInOutTime},
                {"Step Wraps At In/Out", optStepWraps, nil, isStepWrapping},
                {"Scrub Stops At In/Out", optScrubClamps, nil, isScrubClamping},
                {"Show Source/Input at Frame", optInputName, nil, isShowingInputName},
                {"Show Play Direction Indicator", optShowFrameDirection, nil, isShowingFrameDirection},
                }
            }};

        Menu mbpsMenu = Menu {{"Reset MBPS", doResetMbps}};

        if ( pointerWasInTimeline &&
            _phantomFrame >= fs &&
            _phantomFrame <= fe)
        {
            let fname ="%s" % _frameFunc(_phantomFrame),
                title = "Timeline at %s" % fname,
                media = sourceAtFrame(_phantomFrame);

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
                {"Timeline", nil, nil, \: (int;) { DisabledMenuState; }},
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
        deb ("releaseDrag _drag %s\n" % _drag);
        _drag = false;
    }

    method: release (void; Event event)
    {
        scrubAudio(false);
        deb ("release _drag %s\n" % _drag);
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

    method: updateMainNode (void;)
    {
        let vnode = viewNode();

        if (vnode neq nil)
        {
            let vtype = nodeType(vnode),
                (vins, vouts) = nodeConnections(vnode, false);

            _mainNode = if vins.size() > 1 && vtype != "RVSequenceGroup"
                           then vins.front()
                           else vnode;
        }
	_sequenceBoundariesDirty = true;
    }

    method: updateMainNodeEvent (void; Event event)
    {
        updateMainNode();
        event.reject();
    }

    method: updateSequenceBoundaries(void; )
    {
        if (_sequenceBoundariesDirty)
	{
	    _sequenceBoundaries = sequenceBoundaries(_mainNode);
	    _sequenceBoundariesDirty = false;
	}
    }

    method: dirtyBoundaries (void; Event event)
    {
        event.reject();
	_sequenceBoundariesDirty = true;
    }

    method: preRender (void; Event event)
    {
        event.reject();
	if (loadTotal() == 0) updateSequenceBoundaries();
    }

    \: markedBoundariesAroundFrame ((int,int); int f, bool inclusive=true)
    {
        let mframes     = markedFrames(),
            marks       = if mframes.empty() then sequenceBoundaries() else mframes,
            i           = lower_bounds(marks, f),
            fs          = frameStart(),
            fe          = frameEnd(),
            m0Ex        = if i < 0 then fs else (if f != marks[i] then marks[i] else (if i == 0 then fs else marks[i-1])),
            m0Inc       = if i < 0 then fs else marks[i],
            m0          = if inclusive then m0Inc else m0Ex,
            m1          = if i + 1 >= marks.size() then fe+1 else marks[i+1];

        (m0, m1);
    }

    method: doubleClick (void; Event event)
    {
        if (filterLiveReviewEvents()) {
            sendInternalEvent("live-review-blocked-event");
            return;
        }
        if (! _pointerInTimeline)
        {
            event.reject();
            return;
        }

        let loc = renderLocations(event),
            frame = int(frameAtPointer(loc, event));

        if ( frame < frameStart() || frame > frameEnd() )
        {
            deb ("FRAME OUT OF BOUNDS -- frameStart: %s frameEnd: %s frame: %s \n" %
                (frameStart(), frameEnd(), frame));

            // Ignore out of bounds click inside the timeline
            return;
        }

        let (start,end) = markedBoundariesAroundFrame(frame);

        setInPoint(start);
        setOutPoint(end - 1);

        setFrame(frame);

        redraw();
    }

    method: Timeline (Timeline; string name)
    {
        init(name,
             [ ("pointer-1--push", clickFunction(,setFrameAndInOut), "Set Frame On Timeline"),
               ("pointer-1--drag", clickFunction(,scrubSetFrameAndInOut), "Drag Frame On Timeline"),
               ("pointer-1--shift--push", clickFunction(,setInPoint), "Set In Point on Timeline"),
               ("pointer-1--shift--drag", clickFunction(,setOutPoint), "Select In/Out Region on Timeline"),
               ("pointer-3--push", popupOpts, "Popup Timeline Options"),
               ("pointer-1--meta--push", popupOpts, "Popup Timeline Options"),
               ("pointer--move", handleMotion, ""),
               ("pointer--leave", handleLeave, "Track pointer leave"),
               ("pointer-1--shift--release", releaseDrag, ""),
               ("pointer-1--release", release, ""),
               ("pointer-1--double",  doubleClick, ""),
               ("after-graph-view-change", updateMainNodeEvent, ""),
               ("graph-node-inputs-changed", updateMainNodeEvent, ""),
               ("stylus-pen--push", clickFunction(,setFrameAndInOut), "Set Frame On Timeline"),
               ("stylus-pen--drag", clickFunction(,scrubSetFrameAndInOut), "Drag Frame On Timeline"),
               ("stylus-pen--move", handleMotion, ""),
               ("stylus-pen--release", release, ""),
               ("stylus-eraser--push", clickFunction(,setFrameAndInOut), "Set Frame On Timeline"),
               ("stylus-eraser--drag", clickFunction(,scrubSetFrameAndInOut), "Drag Frame On Timeline"),
               ("stylus-eraser--move", handleMotion, ""),
               ("stylus-eraser--release", release, ""),
               ("range-changed", dirtyBoundaries, ""),
               ("pre-render", preRender, ""),
               ],
             false);

        _phantomFrame      = 1.0;
        _phantomFrameX     = 0;
        _pointerInTimeline = false;
        _drag              = false;
        _displayMbps       = (system.getenv("RV_DISPLAY_MBPS", "no") != "no");
        _displayAudioCache = (system.getenv("RV_DISPLAY_AUDIO_CACHE", "no") != "no");
        _controlSize       = 35;
        _tlw               = 100;

        _settings = Settings();

        _sequenceBoundariesDirty = true;
        _sequenceBoundaries = int[]();

        setFrameDisplayFormat (_settings.frameDisplayFormat);

        //
        //  Draw inside the bottom margin.
        //

        if (_settings.drawInMargin) drawInMargin (whichMargin());

        updateMainNode();

        updateBounds(vec2f(0,0), vec2f(0,0));
    }

    method: layout (void; Event event)
    {
        if (isCurrentFrameIncomplete()) updateBounds(vec2f(0,0), vec2f(0,0));

        State state = data();
        let devicePixelRatio = devicePixelRatio();
        gltext.size(state.config.tlFrameTextSize * devicePixelRatio);

        _vm0 =  if (_settings.drawAtTopOfView) then 5*devicePixelRatio else 5*devicePixelRatio; // vertical margin
        _tlh = 8*devicePixelRatio; // timeline height

        let d   = event.domain(),
            f   = frame(),
            s   = _frameFunc(f),
            sb  = gltext.bounds(s),             // size of frame string
            mxb = gltext.bounds("||||"),
            vm1 = 5*devicePixelRatio,
            t   = floor (_vm0 + _tlh + vm1 + mxb[3] + 0.5);

        _Y0  = if (_settings.drawAtTopOfView) then d.y - t else 0;

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
    //  NOTE: this function cannot allocate any memory that it expects to keep
    //  across calls.  To reduce memory leaks this function is called with an
    //  arena allocator and all memory it allocates is deleted on return.
    //
    //  If you must allocate memory to keep, bracket that allocation with
    //  runtime.push_api(0), runtim.pop_api().
    //
    {
        if (!_pointerInTimeline)
        {
            _pointerInInCap = false;
            _pointerInOutCap = false;
            _pointerInInOutRegion = false;
        }
        State state = data();
        let devicePixelRatio = devicePixelRatio();
        gltext.size(state.config.tlFrameTextSize * devicePixelRatio);

        let loc = renderLocations(event);
        let {d, w, h, drawControls, tlh, mxb, hm0, hm1, thm1, vm0, vm1, t, Y} = loc;

        let f    = frame(),
            fs   = frameStart(),
            fe   = frameEnd(),
            fi   = inPoint(),
            fo   = outPoint(),
            dio  = fs != fi || fe != fo,
            s    = _frameFunc(f),
            sb   = gltext.bounds(s); // size of frame string

        _vm0  = vm0;
        _tlh  = tlh;
        _Y0   = Y;
        _Ybot = _Y0 + _vm0;
        _Ytop = _Ybot + _tlh;
        _tlw  = w - hm1 - hm0;
        _X0   = hm0;
        _X1   = w - hm1;
        _H    = t;
        _W    = w;

        let r                    = fe - fs + 1, // frame range
            fp                   = float(f - fs) / float(r),
            ip                   = float(fi - fs) / float(r),
            op                   = float(fo - fs + 1) / float(r),
            ope                  = float(fo - fs + 1) / float(r),
            fg                   = state.config.fg,
            bg                   = state.config.bg,
            xframe               = fp * _tlw + hm0,
            xframe1              = (fp + 1.0/float(r)) * _tlw + hm0,
            xframeMid            = (xframe + xframe1) * 0.5,
            xdiff                = xframe1 - xframe,
            frameIsMarked        = false,
            phantomFrameIsMarked = false,
            hlColor              = Color(.2, .2, .2, 1),
            config               = state.config,
            playing              = isPlaying(),
            vnode                = viewNode(),
            vtype                = nodeType(vnode);

        let (vsecs, asecs, cachedRanges) = cacheUsage(),
            (audioSeconds, cachedAudioRanges) = audioCacheInfo(),
            (vins, vouts) = nodeConnections(vnode, false);

        //
        //  HACK!
        //

        \: drawInfoTab (void; int frame, float xframe, string text)
        {
            let devicePixelRatio = devicePixelRatio();
            gltext.size(config.tlFrameTextSize * devicePixelRatio * .75);

            let mediaName   = text,
                mediaBounds = gltext.bounds(mediaName),
                mediaW      = mediaBounds[2] + mediaBounds[0],
                mediaH      = -mediaBounds[1] + mediaBounds[3],
                fp          = float(frame - fs) / float(r),
                //xframe      = fp * _tlw + hm0,
                mediaX      = xframe - mediaW/2,
                mediaY      = this._Y0 + t + 10;

            glColor(Color(0,0,0,.8));

            drawRect(GL_QUADS,
                     Vec2(mediaX, mediaY - 10),
                     Vec2(mediaX + mediaW, mediaY - 10),
                     Vec2(mediaX + mediaW, mediaY + mediaH),
                     Vec2(mediaX, mediaY + mediaH));

            drawCircleFan(mediaX, mediaY - 10, mediaH + 10, .75, 1.0, .3);
            drawCircleFan(mediaX + mediaW, mediaY - 10, mediaH + 10, 0.0, 0.25, .3);
            drawCircleFan(mediaX, mediaY - 10, mediaH + 10, .75, 1.0, .3, true);
            drawCircleFan(mediaX + mediaW, mediaY - 10, mediaH + 10, 0.0, 0.25, .3, true);

            glBegin(GL_LINES);
            glVertex(mediaX + mediaW, mediaY + mediaH);
            glVertex(mediaX, mediaY + mediaH);
            glEnd();

            gltext.color(fg);
            gltext.writeAt(mediaX, mediaY - 4, mediaName);

            //glDisable(GL_BLEND);
            gltext.size(config.tlFrameTextSize * devicePixelRatio);
        }


        \: timelineQuad (void; Color color, int x1, int x2, int thinnerBy = 0)
        {
            glColor(color);
            glVertex(x1, this._Ybot + thinnerBy);
            glVertex(x2, this._Ybot + thinnerBy);
            glVertex(x2, this._Ytop - thinnerBy);
            glVertex(x1, this._Ytop - thinnerBy);
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
        //  This affects timeline event table
        //  events will be selected inside this bbox
        //
        //  NOTE: only allow this on the control render not the
        //  presentation render (when event.name() == "render-output-device")
        //

        if (event.name() == "render") updateBounds(vec2f(0,_Y0)/devicePixelRatio, vec2f(d.x,_Y0+t)/devicePixelRatio);
	else
	//
	//  OK this is a mess.  The problem is that the widget class still
	//  thinks it knows w/h/x/y for the widget window, but in fact it
	//  doesn't since there may be two such windows, one on controller and
	//  one on pres device.  In addition the timeline and moscope are special
        //  since they have to know how to "stack" with each other if they are both
	//  displayed at top or bottom.
	//
	//  Really the way the margins are "shared" should be re-done, but
	//  don't have time at the mo.
        //
        //  Copied from motion_scope.mu -- Jon
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
                startX      = hm0,
                endX        = w - hm1,
                rangeX      = float(endX - startX);

            glBegin(GL_QUADS);

            if (_pointerInInOutRegion &&
                (fi != fs || fo != fe))
            {
                timelineQuad (fg, _ipCapX, _opCapX, -1);
            }


            if (_sequenceBoundaries.size() > 1)
            //
            //  Draw alternating colors in timeline, representing
            //  different sources.
            //
            {
                for_index (q; _sequenceBoundaries)
                {
                    let i            = q + 1,
                        color        = if (i % 2 == 1) then boundsDark else boundsLight,
                        fracStart    = if (i == 1) then 0.0 else
                                       float(_sequenceBoundaries[i-1] - fs)/r,
                        fracEnd      = if (i == _sequenceBoundaries.size()) then 1.0 else
                                       float(_sequenceBoundaries[i] - fs)/r,
                        sectionStart = startX + int (fracStart * rangeX),
                        sectionEnd   = startX + int (fracEnd   * rangeX);

                    timelineQuad (color, sectionStart, sectionEnd, 1);
                }

                let withinInOut = false;

                for_index (q; _sequenceBoundaries)
                {
                    let i         = q + 1,
                        color     = if (i % 2 == 1) then rangeDark else rangeLight,
                        fracStart = if (i == 1) then 0.0 else
                                    float(_sequenceBoundaries[i-1] - fs)/r,
                        fracEnd   = if (i == _sequenceBoundaries.size()) then 1.0 else
                                    float(_sequenceBoundaries[i] - fs)/r,
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

                        timelineQuad (color, sectionStart, sectionEnd);

                        if (last) break;
                    }
                }
            }
            else
            {
                timelineQuad (config.tlBoundsColor, startX, endX, 1);
                timelineQuad (config.tlRangeColor, hm0 + ip * _tlw, hm0 + ope * _tlw);
            }
            glEnd();

            //
            //  Turn on anti-aliasing + blending here and set the default
            //  line width
            //

            glEnable(GL_BLEND);
            glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
            glEnable(GL_POINT_SMOOTH);
            glEnable(GL_LINE_SMOOTH);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glLineWidth(1.5*devicePixelRatio);

            //
            //  In / Out points
            //

            if (_settings.showInOut)
            {
                gltext.color(fg);
                gltext.size(config.tlBoundsTextSize * devicePixelRatio);

                let sin  = _frameFunc(fi),
                    sout = _frameFunc(fo),
                    bin  = gltext.bounds(sin),
                    bout = gltext.bounds(sout),
                    win  = bin[2],
                    wout = bout[2];

                gltext.writeAt(hm0 + ip * _tlw - win/2,
                               _Ytop + 3,
                               sin);

                gltext.writeAt(hm0 + ope * _tlw - wout/2,
                               _Ytop + 3,
                               sout);

                gltext.size(config.tlFrameTextSize * devicePixelRatio);
            }

            //
            //  draw the cached frames.
            //

            if (cacheMode() != CacheOff)
            {
                glLineWidth(3.0*devicePixelRatio);
                glColor(lerp(config.tlCacheColor, config.tlCacheFullColor, vsecs));
                glBegin(GL_LINES);

                \: xAtFrame (float; int f)
                {
                    ((max(fs,min(f,fe+1)) - fs)/ float(r)) * this._tlw + hm0;
                }

                for (int i=0; i < cachedRanges.size(); i+=2)
                {
                    let f0 = cachedRanges[i],
                        f1 = cachedRanges[i+1]+1;

                    glVertex(xAtFrame(f0), _Ybot + 4*devicePixelRatio);
                    glVertex(xAtFrame(f1), _Ybot + 4*devicePixelRatio);
                }

                glEnd();
                glLineWidth(1.5*devicePixelRatio);
            }

            //
            //  draw the cached audio samples.
            //

            if (_displayAudioCache && (audioCacheMode() != CacheOff))
            {
                glLineWidth(3.0*devicePixelRatio);
                glColor(config.tlAudioCacheColor);
                glBegin(GL_LINES);

                \: xAtFrame (float; int f)
                {
                    ((max(fs,min(f,fe+1)) - fs)/ float(r)) * this._tlw + hm0;
                }

                for (int i=0; i < cachedAudioRanges.size(); i+=2)
                {
                    let f0 = cachedAudioRanges[i],
                        f1 = cachedAudioRanges[i+1]+1;

                    glVertex(xAtFrame(f0), _Ybot + 6*devicePixelRatio);
                    glVertex(xAtFrame(f1), _Ybot + 6*devicePixelRatio);
                }

                glEnd();
                glLineWidth(1.5*devicePixelRatio);
            }

            //
            //  Draw in/out caps
            //

            _ipCapX = hm0 + ip * _tlw;
            _opCapX = hm0 + op * _tlw;

            glColor(config.tlInOutCapsColor);
            glLineWidth(2*devicePixelRatio);
            glBegin(GL_LINES);
            glVertex(_ipCapX, _Ybot - 1);
            glVertex(_ipCapX, _Ytop + 1);
            glVertex(_opCapX, _Ybot - 1);
            glVertex(_opCapX, _Ytop + 1);
            glEnd();

            glColor(fg);
            glLineWidth(5*devicePixelRatio);

            if ((_pointerInInOutRegion &&
                (fi != fs || fo != fe)) ||
                _pointerInInCap)
            {
                glBegin(GL_LINES);
                glVertex(_ipCapX, _Ybot - 2);
                glVertex(_ipCapX, _Ytop + 2);
                glEnd();
                draw(triangleGlyph, _ipCapX-5, (_Ybot+_Ytop)/2.0, 0.0, 9.0, false);
                glEnable(gl.GL_LINE_SMOOTH);
                glLineWidth(1.5*devicePixelRatio);
                draw(triangleGlyph, _ipCapX-6, (_Ybot+_Ytop)/2.0, 0.0, 9.0, true);
                glLineWidth(5*devicePixelRatio);

            }
            if ((_pointerInInOutRegion &&
                (fi != fs || fo != fe)) ||
                _pointerInOutCap)
            {
                glBegin(GL_LINES);
                glVertex(_opCapX, _Ybot - 2);
                glVertex(_opCapX, _Ytop + 2);
                glEnd();
                draw(triangleGlyph, _opCapX+5, (_Ybot+_Ytop)/2.0, 180.0, 9.0, false);
                glEnable(gl.GL_LINE_SMOOTH);
                glLineWidth(1.5*devicePixelRatio);
                draw(triangleGlyph, _opCapX+6, (_Ybot+_Ytop)/2.0, 180.0, 9.0, true);
                glLineWidth(5*devicePixelRatio);
            }

            //
            //  Draw the marked frames
            //

            let mfs = markedFrames();
            glColor(config.tlMarkedColor);

            glLineWidth(1.0*devicePixelRatio);
            glBegin(GL_LINES);

            for_each (mf; mfs)
            {
                if (mf == f) frameIsMarked = true;
                if (mf == _phantomFrame) phantomFrameIsMarked = true;
                let xframe = ((mf - fs)/ float(r)) * _tlw + hm0;
                glVertex(xframe, _Ybot - 1);
                glVertex(xframe, _Ytop);
            }

            glEnd();
            glLineWidth(1.5*devicePixelRatio);

            glPointSize(3.2);
            glBegin(GL_POINTS);

            for_each (mf; mfs)
            {
                let xframe = ((mf - fs)/ float(r)) * _tlw + hm0;
                glVertex(xframe, _Ybot - 1);
            }

            glEnd();
        }

        //
        //  Draw the phantom frame number and tick (colored if necessary)
        //

        if (state.userActive && !_drag &&
            _phantomFrameDisplay && _pointerInTimeline &&
            _phantomFrame >= fs && _phantomFrame < (fe+1.0) && _phantomFrame != f)
        {
            let ptcolor = 0.75*fg,
                pmcolor = config.tlMarkedColor;

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

            let pfp        = double(double(_phantomFrame) - double(fs)) / double(r),
                pxframe    = pfp * _tlw + hm0,
                pxframe1   = (pfp + 1.0/float(r)) * _tlw + hm0,
                ps         = _frameFunc(_phantomFrame),
                psb        = gltext.bounds(ps),
                psW        = psb[2] + psb[0];

            glBegin(GL_LINES);
            glVertex(pxframe, _Ybot - 1);
            glVertex(pxframe, _Ytop + 1);
            glEnd();

            let xpf  = pxframe - psW / 2.0,
                ypf  = _Ytop + 3,
                psw  = (psb[2] + psb[0]) / 2.0,
                psh  = -psb[1] + psb[3],
                psY  = _Ybot,
                psYh = psY + _tlh + 3;

            if (xdiff > 2)
            {
                let fp = float(floor(_phantomFrame) - fs) / float(r),
                    x0 = fp * _tlw + hm0,
                    x1 = (fp + 1.0/float(r)) * _tlw + hm0;

                glColor(hlColor * Color(.5,.5,.5,1.0));
                glBlendFunc(GL_ONE, GL_ONE);

                drawRect(GL_QUADS,
                         Vec2(x0, psY - 1),
                         Vec2(x0, psYh - 2),
                         Vec2(x1, psYh - 2),
                         Vec2(x1, psY - 1));

                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            }


            glColor(bg);
            drawRect(GL_QUADS,
                     Vec2(pxframe - psw, psYh),
                     Vec2(pxframe + psw, psYh),
                     Vec2(pxframe + psw, psYh + psh),
                     Vec2(pxframe - psw, psYh + psh));

            if ( ! isUndiscoveredFrame(_phantomFrame))
            {
                // draw the frame number under the cursor
                gltext.writeAtNL(xpf, ypf, ps);
            }

            if (!_drag && _settings.showInputName)
            {
                drawInfoTab(_phantomFrame,
                            pxframe,
                            sourceAtFrame(_phantomFrame));
            }
        }

        //
        //  frame number and tick
        //
        let fcol  = if frameIsMarked then config.tlMarkedColor else fg,
            sw    = (sb[2] + sb[0]) / 2.0,
            sh    = -sb[1] + sb[3],
            sY    = _Ybot,
            sYh   = sY + _tlh + 3 * devicePixelRatio,
            ffilt = if playing then .1 else 1.0,
            fdiff = -sb[2] / 2.0 * ffilt + _frameTextOffset * (1.0 - ffilt),
            ftx   = xframeMid + fdiff;

        _frameTextOffset = fdiff;


        //
        // draw current frame tick
        //
        glColor(fcol);
        gltext.color(fcol);

        glLineWidth(2.0*devicePixelRatio);
        glBegin(GL_LINES);
        glVertex(xframe, sY - 1);
        glVertex(xframe, sYh - 2);
        glEnd();


        if (_settings.showFrameDirection)
        {
            let fwd    = inc() > 0,
                xg     = if fwd then xframeMid - fdiff + 8.0*devicePixelRatio else ftx - 6.0,
                yg     = sYh + (-mxb[1] + mxb[3]) / 2.0,
                angle  = if fwd then 180.0 else 0.0,
                radius = 8.0 * devicePixelRatio;

            if (playing)
            {
                glColor(config.bgVCRButton);
                draw(triangleGlyph, xg, yg, angle, radius, false);
            }

            glColor(config.bgVCRButton * .8);
            draw(triangleGlyph, xg, yg, angle, radius, true);
        }

        glColor(bg);
        drawRect(GL_QUADS,
                 Vec2(xframeMid - sw, sYh),
                 Vec2(xframeMid + sw, sYh),
                 Vec2(xframeMid + sw, sYh + sh),
                 Vec2(xframeMid - sw, sYh + sh));

        if ( ! isUndiscoveredFrame(f))
        {
            // draw current frame number
            gltext.writeAtNL(ftx, sYh, s);
        }

        gltext.color(fg);

        if (xdiff > 2)
        {
            glColor(hlColor);
            glBlendFunc(GL_ONE, GL_ONE);
            drawRect(GL_QUADS,
                     Vec2(xframe, sY - 1),
                     Vec2(xframe, sYh - 2),
                     Vec2(xframe1, sYh - 2),
                     Vec2(xframe1, sY - 1));

            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glLineWidth(1.0*devicePixelRatio);
            glColor(fcol);
            glBegin(GL_LINES);
            glVertex(xframe1, sY - 1);
            glVertex(xframe1, sYh - 2);
            glEnd();
        }

        glLineWidth(1.5*devicePixelRatio);

        if (_drag && _settings.showInputName)
        {
            drawInfoTab(f,
                        xframeMid,
                        sourceAtFrame(f));
        }

        //
        //  Draw the in/out info if needed
        //

        if (dio)
        {
            gltext.writeAtNL(10, _Ytop + 3, "%d" % (f - fi + 1));
        }

        gltext.size(config.tlBoundsTextSize * devicePixelRatio);
        gltext.writeAtNL(10, _Ybot, _totalFunc(fo-fi+1));

        //
        //  Draw the real fps
        //

        {

            let rfps = " %.1f" % (floor(10.0 * realFPS() + 0.5)/10.0),
                tfps = "%.2f" % fps(),
                mmr  = "000.00",
                rl   = gltext.bounds(rfps)[2],
                mmb  = gltext.bounds(mmr),
                maxw = mmb[2],
                maxh = mmb[3] * 1.5,
                fx   = w - hm1 + thm1 + 10,
                sk   = skipped();

            gltext.writeAt(fx, _Ybot, tfps);
            gltext.writeAt(fx + maxw, _Ybot, rfps);
            gltext.writeAt(fx + maxw * 2, _Ybot, "fps");

            if (sk != 0 && sk != -1)
            {
                let skt = "%d  " % sk,
                    skb = gltext.bounds(skt);

                glColor(config.tlSkipColor);
                glPointSize(skb[3] * 1.4);
                glBegin(GL_POINTS);
                glVertex(w - skb[2] + skb[2] * .35, _Ybot + mxb[3] * 0.25);
                glEnd();

                gltext.color(config.tlSkipTextColor);
                gltext.writeAt(w - skb[2], _Ybot, skt);
            }
        }

        //
        //  Draw the mbps
        //

        if (_displayMbps)
        {
            let mbps = " %.0f" % (floor(mbps() + 0.5)),
                mmr  = "000.00",
                rl   = gltext.bounds(mbps)[2],
                mmb  = gltext.bounds(mmr),
                maxw = mmb[2],
                maxh = mmb[3] * 2.0,
                fx   = w - hm1 + 10,
                yoff = maxh;


            gltext.writeAt(fx + maxw, yoff + _Ybot, mbps);
            gltext.writeAt(fx + maxw * 2, yoff + _Ybot, "mbps");
        }

        //
        //   VCRButtons drawing
        //

        if (drawControls)
        {
            let vcr_h  = _controlSize * devicePixelRatio,
                vcr_w  = _controlSize * devicePixelRatio * 6,
                vcr_x0 = d.x - vcr_w,
                vcr_x1 = vcr_x0 + vcr_w,
                vcr_y0 = 0,
                vcr_y1 = _Ytop;

            setupProjection(d.x, d.y, event.domainVerticalFlip());

            //glPushAttrib(GL_POLYGON_BIT | GL_LINE_BIT);
            //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

            if (!_settings.drawInMargin)
            {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glColor(bg);
            }
            else (glColor(0,0,0,1));

            //
            //  Draw the background grey area
            //

            drawRect(GL_QUADS,
                     Vec2(vcr_x0, _Y0 + vcr_y0),
                     Vec2(vcr_x1, _Y0 + vcr_y0),
                     Vec2(vcr_x1, _Y0 + t),
                     Vec2(vcr_x0, _Y0 + t));

            let vcr_bg = state.config.bgVCR,
                vcr_fg = state.config.fgVCR;

            /*
            drawRoundedBox (vcr_x0 + 20, _Y0 + vcr_y0 + _vm0 - 1,
                            //vcr_x1 - 20, vcr_y1 - vm1 + 1, 12, vcr_bg, vcr_fg);
                            vcr_x1 - 20, _Y0 + t - vm1 + 2, 12, vcr_bg, vcr_fg);
            */

            \: glyphElement (void; int i, Glyph g, float rot, bool outline)
            {
                let h2  = vcr_h / 2,
                    x   = vcr_x0 + i * vcr_h + vcr_h,
                    y   = this._Y0 + vcr_y0 + h2 - 1;

                draw(g, x, y, rot, h2, outline);
            }

            let bc = state.config.bgVCRButton,
                hc = state.config.hlVCRButton;

            glColor(bc);
            glyphElement(0, advanceGlyph, 0, false);
            if (isPlayingBackwards()) glColor(hc);
            glyphElement(1, triangleGlyph, 0, false);
            glColor(bc);
            if (!isPlaying()) glColor(hc);
            glyphElement(2, xformedGlyph(squareGlyph, 0, 0.8), 0, false);
            glColor(bc);
            if (isPlayingForwards()) glColor(hc);
            glyphElement(3, triangleGlyph, 180, false);
            glColor(bc);
            glyphElement(4, advanceGlyph, 180, false);

            glColor(.15,.15,.15,1);
            glLineWidth(1.0*devicePixelRatio);
            glyphElement(0, advanceGlyph, 0, true);
            glyphElement(1, triangleGlyph, 0, true);
            glyphElement(2, xformedGlyph(squareGlyph, 0, 0.8), 0, true);
            glyphElement(3, triangleGlyph, 180, true);
            glyphElement(4, advanceGlyph, 180, true);
        }

        glDisable(GL_BLEND);
    }
}

\: init (Widget;) { Timeline("timeline"); }

} // timeline
