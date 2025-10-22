//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

module: presentation_mode {

use rvtypes;
use commands;
use extra_commands;
use app_utils;
use gl;
use glyph;

global int totalPresentationBytes = 0.0;
global int totalPresentationRenderCount = 0;

class: PresentationControlMinorMode : MinorMode
{ 
    class: FormatText
    {
        bool valid;
        string text;
        float fontSize;
        float width;
        float height;
    }

    bool       _showTimeline;
    bool       _showTimelineMagnifier;
    bool       _showImageInfo;
    bool       _showSourceDetails;
    bool       _showInspector;
    bool       _showWipes;
    bool       _showInfoStrip;
    bool       _showPointer;
    bool       _showFormat;
    bool       _showSync;
    bool       _showFeedback;
    FormatText _deviceText;
    FormatText _formatText;
    FormatText _dataFormatText;
    FormatText _audioFormatText;
    float      _totalHeight;
    Point      _pointerPosLast;
    float      _pointerRenderTime;
    EventFunc  _drawFeedback;
    bool       _mirroredDisplay;
    bool       _mirroredStateValid;
    float      _computeScale;
    float      _setScale;
    (void;float,float) _setPixelRelativeScale;
    (float;float,bool,bool) _computePixelRelativeScale;

    \: inactiveState (int;) { DisabledMenuState; }

    method: writePrefs (void; )
    {
        writeSetting ("PresentationControl", "showTimeline",          SettingsValue.Bool(_showTimeline));
        writeSetting ("PresentationControl", "showTimelineMagnifier", SettingsValue.Bool(_showTimelineMagnifier));
        writeSetting ("PresentationControl", "showImageInfo",         SettingsValue.Bool(_showImageInfo));
        writeSetting ("PresentationControl", "showSourceDetails",     SettingsValue.Bool(_showSourceDetails));
        writeSetting ("PresentationControl", "showInspector",         SettingsValue.Bool(_showInspector));
        writeSetting ("PresentationControl", "showWipes",             SettingsValue.Bool(_showWipes));
        writeSetting ("PresentationControl", "showInfoStrip",         SettingsValue.Bool(_showInfoStrip));
        writeSetting ("PresentationControl", "showPointer",           SettingsValue.Bool(_showPointer));
        writeSetting ("PresentationControl", "showSync",              SettingsValue.Bool(_showSync));
        writeSetting ("PresentationControl", "showFeedback",          SettingsValue.Bool(_showFeedback));
    }

    method: readPrefs (void; )
    {
        let SettingsValue.Bool b2 = readSetting ("PresentationControl", "showTimeline", SettingsValue.Bool(false));
        _showTimeline = b2;

        let SettingsValue.Bool b3 = readSetting ("PresentationControl", "showTimelineMagnifier", SettingsValue.Bool(false));
        _showTimelineMagnifier = b3;

        let SettingsValue.Bool b4 = readSetting ("PresentationControl", "showImageInfo", SettingsValue.Bool(false));
        _showImageInfo = b4;

        let SettingsValue.Bool b5 = readSetting ("PresentationControl", "showInspector", SettingsValue.Bool(false));
        _showInspector = b5;

        let SettingsValue.Bool b55 = readSetting ("PresentationControl", "showWipes", SettingsValue.Bool(false));
        _showWipes = b55;

        let SettingsValue.Bool b6 = readSetting ("PresentationControl", "showInfoStrip", SettingsValue.Bool(false));
        _showInfoStrip = b6;

        let SettingsValue.Bool b7 = readSetting ("PresentationControl", "showPointer", SettingsValue.Bool(false));
        _showPointer = b7;

        let SettingsValue.Bool b8 = readSetting ("PresentationControl", "showSync", SettingsValue.Bool(true));
        _showSync = b8;

        let SettingsValue.Bool b9 = readSetting ("PresentationControl", "showFeedback", SettingsValue.Bool(false));
        _showFeedback = b9;

        let SettingsValue.Bool b10 = readSetting ("PresentationControl", "showSourceDetails", SettingsValue.Bool(false));
        _showSourceDetails = b10;

    }

    method: zeroMargins(void; )
    {
	Vec4 m = Vec4 {0.0, 0.0, 0.0, 0.0};
	setMargins (m, true);
    }

    method: timelineState (int;)      { return if _showTimeline then CheckedMenuState else UncheckedMenuState; }
    method: timelineMagState (int;)   { return if _showTimelineMagnifier then CheckedMenuState else UncheckedMenuState; }
    method: imageInfoState (int;)     { return if _showImageInfo then CheckedMenuState else UncheckedMenuState; }
    method: sourceDetailsState (int;) { return if _showSourceDetails then CheckedMenuState else UncheckedMenuState; }
    method: inspectorState (int;)     { return if _showInspector then CheckedMenuState else UncheckedMenuState; }
    method: wipesState (int;)         { return if _showWipes then CheckedMenuState else UncheckedMenuState; }
    method: infoStripState (int;)     { return if _showInfoStrip then CheckedMenuState else UncheckedMenuState; }
    method: pointerState (int;)       { return if _showPointer then CheckedMenuState else UncheckedMenuState; }
    method: formatState (int;)        { return if _showFormat then CheckedMenuState else UncheckedMenuState; }
    method: feedbackState (int;)      { return if _showFeedback then CheckedMenuState else UncheckedMenuState; }
    method: syncState (int;)          { return if _showSync then CheckedMenuState else UncheckedMenuState; }

    method: toggleTimeline (void; Event v)      { _showTimeline = !_showTimeline; writePrefs(); zeroMargins(); redraw(); }
    method: toggleTimelineMag (void; Event v)   { _showTimelineMagnifier = !_showTimelineMagnifier; writePrefs(); zeroMargins(); redraw(); }
    method: toggleImageInfo (void; Event v)     { _showImageInfo = !_showImageInfo; writePrefs(); redraw(); }
    method: toggleSourceDetails (void; Event v) { _showSourceDetails = !_showSourceDetails; writePrefs(); redraw(); }
    method: toggleInfoStrip (void; Event v)     { _showInfoStrip = !_showInfoStrip; writePrefs(); redraw(); }
    method: toggleInspector (void; Event v)     { _showInspector = !_showInspector; writePrefs(); redraw(); }
    method: toggleWipes (void; Event v)         { _showWipes = !_showWipes; writePrefs(); redraw(); }
    method: togglePointer (void; Event v)       { _showPointer = !_showPointer; writePrefs(); redraw(); }
    method: toggleFormat (void; Event v)        { _showFormat = !_showFormat; redraw(); }
    method: toggleFeedback (void; Event v)      { _showFeedback = !_showFeedback; writePrefs(); redraw(); }
    method: toggleSync (void; Event v)          { _showSync = !_showSync; writePrefs(); redraw(); }

    method: move (void; Event event)
    {
        event.reject();
	State state = data();
	state.perPixelInfoValid = false;
    }

    method: _drawCross (void; Point p, float t)
    {
        let x0 = Vec2(2, 0),
            x1 = Vec2(10, 0),
            y0 = Vec2(0, 2),
            y1 = Vec2(0, 10);

       float f = t/2.0;

        glLineWidth(3.0);

        glColor(0.5-t,0.5-t,0.5-t,1);

        glBegin(GL_LINES);
        glVertex(p + x0); glVertex(p + x1);
        glVertex(p - x0); glVertex(p - x1);
        glVertex(p + y0); glVertex(p + y1);
        glVertex(p - y0); glVertex(p - y1);
        glEnd();

        glLineWidth(1.0);
        glColor(0.5+t,0.5+t,0.5+t,1);

        glBegin(GL_LINES);
        glVertex(p + x0); glVertex(p + x1);
        glVertex(p - x0); glVertex(p - x1);
        glVertex(p + y0); glVertex(p + y1);
        glVertex(p - y0); glVertex(p - y1);
        glEnd();
    }

    method: maybeAdjustMargins (bool; Widget mode, bool showing, bool renderedLast)
    {
        if (mode eq nil) return false;

        let renderedThis = (showing && mode._active);

        if (renderedThis != renderedLast) mode.updateMargins (renderedThis);

        return renderedThis;
    }

    method: _drawPointer (void; Event event, State state)
    {
        if (!_showPointer) return;

        //  If this is "real" render, bail
        if (event.name() == "render") return;

	runtime.gc.push_api(3);
        try
        {

            if (state eq nil) 
            {
                state = data();
                setupProjection(event.domain().x, event.domain().y, event.domainVerticalFlip());
            }

            if (state.pixelInfo neq nil && !state.pixelInfo.empty() && state.pixelInfo.front().name != "")
            {
                let pinfo = state.pixelInfo.front(),
                    ip    = state.pointerPositionNormalized,
                    name  = sourceNameWithoutFrame(pinfo.name).split("/").front();

                if (ip != _pointerPosLast) 
                {
                    _pointerPosLast = ip;
                    _pointerRenderTime = theTime();
                }
                if (theTime() - _pointerRenderTime < 2.0)
                {
                    try 
                    { 
                        let infos = imagesAtPixel(state.pointerPosition),
                            ep    = imageToEventSpace (pinfo.name, ip, true),
                            vs    = viewSize(),
                            xlo   = if (ep.x < 0) then 0 else ep.x,
                            ylo   = if (ep.y < 0) then 0 else ep.y,
                            x     = if (xlo  > vs.x-1) then vs.x-1 else xlo,
                            y     = if (ylo  > vs.y-1) then vs.y-1 else ylo,
                            flip  = event.domainVerticalFlip();

                        _drawCross (Point(x, if flip then vs.y - y - 1 else y), 1.0);
                        redraw();
                    }
                    catch(...) { ; }
                }
            }
        }
        catch(...) { ; }

	runtime.gc.pop_api();
    }

    method: render (void; Event event)
    {
        event.reject();

	if (! presentationMode() || !_showPointer) return;

        runtime.gc.push_api(0);
        updatePixelInfo(nil);
        runtime.gc.pop_api();

        _drawPointer (event, nil);
    }

    method: renderOutputDevice (void; Event event)
    {
        event.reject();

	//  let a0 = runtime.gc.get_free_bytes();

        //
        //  Scale 1:1
        //

	if (_computeScale == -1.0) _setScale = _computePixelRelativeScale(1.0, true, false);
	else
	if (_computeScale !=  0.0) _setScale = _computePixelRelativeScale(_computeScale, false, false);

        //
        //  Don't draw on empty session
        //
        if (isCurrentFrameIncomplete()) return;

        State state = data();

	if (state.perPixelInfoValid)
	{
	    if (state.perPixelFrame != frame() ||
		state.perPixelPosition != state.pointerPosition)
	    {
		state.perPixelInfoValid = false;
	    }
	    else
	    {
		runtime.gc.push_api(3);
		int viewHash = string.hash(viewNode());
		runtime.gc.pop_api();

		if (state.perPixelViewHash != viewHash) 
		{
		    state.perPixelInfoValid = false;
		}
	    } 
	}

        //
        //  Allow modes to render themselves on the presentation device with
        //  render method, if they've indicated they want to.
        //
        for_each (m; state.minorModes) if (m._active && m._drawOnPresentation) m.render(event);
        for_each (w; state.widgets) if (w._active && w._drawOnPresentation) w.render(event);

        runtime.gc.disable();
        runtime.gc.push_api(3);

	try
	{

        if (_showTimelineMagnifier && state.motionScope neq nil && state.motionScope._active) state.motionScope.render(event);
        if (_showTimeline && state.timeline neq nil && state.timeline._active) state.timeline.render(event);
        if (_showImageInfo && state.imageInfo neq nil && state.imageInfo._active) state.imageInfo.render(event);
        if (_showSourceDetails && state.sourceDetails neq nil && state.sourceDetails._active) state.sourceDetails.render(event);
        if (_showInfoStrip && state.infoStrip neq nil && state.infoStrip._active) state.infoStrip.render(event);
        if (_showFeedback && state.feedback > 0) _drawFeedback(event);

        let domain = event.domain();
        setupProjection(domain.x, domain.y, event.domainVerticalFlip());

        _drawPointer (event, state);

        if (_showFormat)
        {
            let vstate = videoState(),
                h = vstate.height / 4,
                w = vstate.width * 0.6,
                m = (vstate.width - w) / 2;

            \: init (void; FormatText t, string text, int w, int h)
            {
                t.text     = text;
                t.fontSize = fitTextInBox(text, w, h);

                gltext.size(t.fontSize);

                let b  = gltext.bounds(text);
                t.width    = b[0] + b[2];
                t.height   = -b[1] + b[3];
                t.valid    = true;
            }

            if (!_deviceText.valid)
            {
                init(_deviceText, "%s/%s" % (vstate.module, vstate.device), w, h);
                init(_formatText, vstate.videoFormat, w, h);
                init(_dataFormatText, vstate.dataFormat, w, h);
                init(_audioFormatText, vstate.audioFormat, w, h);

                _totalHeight = _deviceText.height + _formatText.height + 
                    _dataFormatText.height + _audioFormatText.height;
            }

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            let hm = (vstate.height - _totalHeight) / 2,
                sep = (vstate.height / 4.0) * 0.1;

            int y = vstate.height - hm;

            drawRoundedBox(m, y - _totalHeight - sep * 4, 
                           vstate.width - m, y,
                           20,
                           Color(0,0,0,.75), Color(0,0,0,1));

            for_each (t; [_deviceText, _formatText, _dataFormatText, _audioFormatText])
            {
                y -= t.height;

                gltext.size(t.fontSize);

                gltext.color(Color(0,0,0,.1)); gltext.writeAt(m - 2, y - 2, t.text);
                gltext.color(Color(0,0,0,.1)); gltext.writeAt(m + 2, y + 2, t.text);
                gltext.color(Color(0,0,0,.1)); gltext.writeAt(m - 2, y + 2, t.text);
                gltext.color(Color(0,0,0,.25)); gltext.writeAt(m + 4, y - 4, t.text);
                gltext.color(Color(.75,.75,.75,1)); gltext.writeAt(m, y, t.text);

                y -= sep;
            }

            glDisable(GL_BLEND);
        }

	}
	catch (object obj)
	{
	    print("ERROR: presentation render failed: %s\n" % string(obj));
	}
        runtime.gc.pop_api();
        runtime.gc.enable();

	//
	//  These guys are not "safe" to have inside the arena api push/pop
	//
        if (_showInspector && state.inspector neq nil && state.inspector._active) state.inspector.render(event);
        if (_showSync && state.sync neq nil && state.sync._active) state.sync.render(event);
        if (_showWipes && state.wipe neq nil && state.wipe._active) state.wipe.render(event);

	/*
	let a1 = runtime.gc.get_free_bytes(),
	    d = a0 - a1;
	if (d > 0) totalPresentationBytes += d;
	++totalPresentationRenderCount;
	if (d > 0) print("frame %s: presentation render allocated %d bytes, ave %s\n" % (frame(), d, float(totalPresentationBytes)/float(totalPresentationRenderCount)));
	*/
    }

    method: deviceChanged (void; Event event)
    {
        event.reject();
        _deviceText.valid = false;
        _mirroredStateValid = false;
    }

    method: setPixelScale(void; float scale)
    {
        if (presentationMode())
        {
	    _computeScale = scale;
            redraw();
        }
    }

    method: scaleOneToTwo (void; Event event)
    {
        if (presentationMode())
        {
	    _computeScale = 2.0;
            redraw();
        }
    }

    method: scaleOneToOne (void; Event event)
    {
        if (presentationMode())
        {
	    _computeScale = 1.0;
            redraw();
        }
    }

    method: scaleFrameWidth (void; Event event)
    {
        if (presentationMode())
        {
	    extra_commands.setTranslation (rvtypes.Point(0,0));
	    _computeScale = -1.0;
            redraw();
        }
    }

    method: presentationModeState (int;)
    {
        if presentationMode() then UncheckedMenuState else DisabledMenuState;
    }

    method: preRender(void; Event event)
    {
        event.reject();

        if (_setScale != 0.0) 
	{
            let requestedScale = if (_computeScale > 0.0) then _computeScale else 0.0;
	    _setPixelRelativeScale(_setScale, requestedScale);
	    _computeScale = 0.0;
	    _setScale = 0.0;
	}
    }

    method: PresentationControlMinorMode(PresentationControlMinorMode;)
    {
        //
        // dynamically import these funcs from rvui. we can't depend
        // on rvui directly in this module because its imported by
        // rvui
        //

        _drawFeedback = runtime.lookup_function(runtime.intern_name("rvui.drawFeedback"));
        _computePixelRelativeScale = runtime.lookup_function(runtime.intern_name("rvui.computePixelRelativeScale"));
        _setPixelRelativeScale = runtime.lookup_function(runtime.intern_name("rvui.setPixelRelativeScale"));

        _deviceText      = FormatText(); _deviceText.valid = false;
        _formatText      = FormatText(); _formatText.valid = false;
        _dataFormatText  = FormatText(); _dataFormatText.valid = false;
        _audioFormatText = FormatText(); _audioFormatText.valid = false;

	_setScale = 0.0;
	_computeScale = 0.0;

        _mirroredDisplay = false;
        _mirroredStateValid = false;

        readPrefs();

        init("presentation_control",
             [("pointer--move", move, "Track pointer"),
              ("stylus-pen--move", move, "Track stylus pointer"),
              ("render-output-device", renderOutputDevice, "Render"),
              ("output-video-device-changed", deviceChanged, ""),
              ("pre-render", preRender, ""),
              ("key-down--!", scaleOneToOne, "Set image scale to 1:1 on presentation device"),
              ("key-down--alt--f", scaleFrameWidth, "Set image scale fit image width on presentation device")
               ],
             nil,
            newMenu(MenuItem[] {
                subMenu("Image", MenuItem[] {
                    subMenu("Scale", MenuItem[] {
                        menuSeparator(),
                        menuText("On Presentation Device"),
                        menuItem("   1:1", "key-down--!", "presentation_category", scaleOneToOne, presentationModeState),
                        menuItem("   1:2", "", "presentation_category", scaleOneToTwo, presentationModeState),
                        menuItem("   Frame Width", "key-down--alt--f", "presentation_category", scaleFrameWidth, presentationModeState)
                    })
                }),
                subMenu("View", MenuItem[] {
                    subMenu("Presentation Settings", MenuItem[] {
                        menuText("Show"),
                        menuItem("   Pointer", "", "presentation_category", togglePointer, pointerState),
			 //  XXX
			 //  This errors in pres mode in rv4, crashes rvsdi, turn off for now
                        menuItem("   Video Format", "", "presentation_category", toggleFormat, formatState),
                        menuSeparator(),
                        menuText("Mirror"),
                        menuItem("   Timeline", "", "presentation_category", toggleTimeline, timelineState),
                        menuItem("   Timeline Magnitifer", "", "presentation_category", toggleTimelineMag, timelineMagState),
                        menuItem("   Image Info", "", "presentation_category", toggleImageInfo, imageInfoState),
                        menuItem("   Source Details", "", "presentation_category", toggleSourceDetails, sourceDetailsState),
                        menuItem("   Color Inspector", "", "presentation_category", toggleInspector, inspectorState),
                        menuItem("   Wipes", "", "presentation_category", toggleWipes, wipesState),
                        menuItem("   Info Strip", "", "presentation_category", toggleInfoStrip, infoStripState),
                        menuItem("   Feedback Messages", "", "presentation_category", toggleFeedback, feedbackState),
                        menuItem("   Remote Sync Pointers", "", "presentation_category", toggleSync, syncState)
                    })
                })
            })
        );
    }
}

\: theMode (PresentationControlMinorMode; )
{
    State state = data();
    PresentationControlMinorMode pcm = nil;

    for_each (m; state.minorModes) 
    {
        if (m._modeName == "presentation_control") pcm = m;
    }

    return pcm;
}

\: setPresentationModePixelScale (void; float scale)
{
    PresentationControlMinorMode pcm = theMode();
    if (pcm neq nil) pcm.setPixelScale(scale);
}

\: createMode (Mode;)
{
    return PresentationControlMinorMode();
}

}
