//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
module: inspector {
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

class: Inspector : Widget
{
    bool            _colorSampling;
    Point           _colorPointImage;
    Point           _colorPointEvent;
    Point           _colorPointPixel;
    PixelSourceInfo _colorSourceInfo;
    Color           _currentColor;
    Color           _fbColor;
    Color           _averageColor;
    int             _colorSamples;
    int             _colorFrame;
    float           _colorScale;        // 1.0 == float, 255 == byte, etc
    bool            _showFBColor;       // use color in fb or source?
    bool            _xFlipped;
    bool            _yFlipped;
    bool            _inputsChanged;
    bool            _graphStateChanged;
    bool            _glReadPending;
    bool            _updateAverageColor;
    bool            _skipNextSampleAtPoint;
    bool            _nearTriangle;
    int             _widW;

    PointerStylePlain := 0;
    PointerStyleBox   := 1;
    PointerStyleCross := 2;

    int             _pointerStyle;

    CloseButtonAlways := 0;
    CloseButtonNearby := 1;

    int             _closeButtonStyle;

    class: SourcePixel
    {
        float xOrient;
        float yOrient;
        float pasp;
        float displayWidth;
        float displayHeight;
        float sq_px;
        float sq_py;
        int normX;
        int normY;
        vector float[4] v;
    }

    \: getSourcePixel (SourcePixel; PixelImageInfo pinfo, string name, int frame )
    {
        SourcePixel sp = SourcePixel();
        let imageGeom = nodeImageGeometry(name, frame);

        // We protect the reading of the aspect ratio for the graph which 
        // does not have a lens warp node.
        let lensWarpPixelRatio = 0.0;
        try 
        {
            lensWarpPixelRatio = getFloatProperty("#RVLensWarp.warp.pixelAspectRatio").front();
        }
        catch(...)
        {
            ;
        }

        sp.xOrient = imageGeom.orientation[0,0];
        sp.yOrient = imageGeom.orientation[1,1];
        sp.pasp = imageGeom.pixelAspect;
        sp.displayWidth =   if (sp.pasp > 1) then imageGeom.width / sp.pasp else imageGeom.width;
        sp.displayHeight = imageGeom.height;
        sp.sq_px = pinfo.px / sp.pasp;
        sp.sq_py = pinfo.py;
        if ( lensWarpPixelRatio != 0 ){
            sp.displayWidth = if(lensWarpPixelRatio >= 1) then imageGeom.width / lensWarpPixelRatio else sp.displayWidth / lensWarpPixelRatio;
            sp.sq_px = if(lensWarpPixelRatio >= 1) then pinfo.px / lensWarpPixelRatio else sp.sq_px / lensWarpPixelRatio;
        }
        sp.normX =  if (sp.xOrient == - 1) then int(sp.displayWidth - sp.sq_px)
                    else int(sp.sq_px);
        sp.normY =  if (sp.yOrient == - 1) then int(sp.displayHeight - sp.sq_py)
                    else int(sp.sq_py);
        sp.v = sourcePixelValue(name, sp.normX, sp.normY);
        return sp;
    }

    \: initializeNoPoint (void; Inspector this)
    {
        State state = data();

        let pinfo  = state.pixelInfo.front(),
            sName  = sourceNameWithoutFrame(pinfo.name),
            ip     = state.pointerPosition,
            sp = getSourcePixel(pinfo, sName, frame());

        if (!_active) this.toggle();

        if (_showFBColor)
        {
            _glReadPending = true;
            _averageColor    = Color(0, 0, 0);
            _colorSamples    = 0;
        }
        else
        {
            _averageColor    = sp.v;
            _colorSamples    = 1;
        }

        _colorSampling   = true;
        _currentColor    = sp.v;
        _colorPointPixel = Point(pinfo.px, pinfo.py);
        _colorPointEvent = ip;
        _colorPointImage = eventToImageSpace(sName, ip);
        _colorSourceInfo = pinfo;
        _colorFrame      = frame();
        redraw();
    }

    \: beginSampling (void; Inspector this, Event event)
    {
        State state = data();
        recordPixelInfo(event);
        if (state.pixelInfo eq nil || state.pixelInfo.empty()) return;

        let pinfo  = state.pixelInfo.front(),
            sName  = sourceNameWithoutFrame(pinfo.name),
            ip     = state.pointerPosition,
            sp = getSourcePixel(pinfo, sName, frame());

        if (!_active) this.toggle();

        if (_showFBColor)
        {
            _glReadPending = true;
            _averageColor    = Color(0, 0, 0);
            _colorSamples    = 0;
        }
        else
        {
            _averageColor    = sp.v;
            _colorSamples    = 1;
        }

        _colorSampling   = true;
        _currentColor    = sp.v;
        _colorPointPixel = Point(pinfo.px, pinfo.py);
        _colorPointEvent = ip;
        _colorPointImage = eventToImageSpace(sName, ip);
        _colorSourceInfo = pinfo;
        _colorFrame      = frame();

        _skipNextSampleAtPoint = true;

        redraw();
    }

    \: sampleAtPoint (void; Inspector this, Event event)
    {
        // Whenever we shift-click to reset the _averageColor value, it calls 'pointer-1--shift--push' then
        // 'pointer-1--shift--drag'. This behaviour makes it so when we reset, we'd get 2 samples inside the
        // _averageColor var, which is not what we want, because we're resetting that value! 
        // (it should be either 0 or 1, for the value under the current position). 
        // This if statement corrects that behaviour, by making sure that we skip the first time we enter
        // sampleAtPoint() because beginSampling() was called at the same time.

        if (_skipNextSampleAtPoint)
        {
            _skipNextSampleAtPoint = false;
            return;
        }

        State state = data();
        recordPixelInfo(event);
        if (state.pixelInfo eq nil || state.pixelInfo.empty()) return;

        let pinfo  = state.pixelInfo.front(),
            sName  = sourceNameWithoutFrame(pinfo.name),
            ip     = state.pointerPosition,
            sp = getSourcePixel(pinfo, sName, frame());

        if (_showFBColor)
        {
            _glReadPending = true;
        }
        else
        {
            _averageColor    += sp.v;
            _colorSamples    += 1;
        }

        _currentColor     = sp.v;
        _colorPointEvent  = ip;
        _colorPointPixel  = Point(pinfo.px, pinfo.py);
        _colorPointImage  = eventToImageSpace(sName, ip);
        _colorSourceInfo  = pinfo;
        _colorFrame       = frame();

        redraw();
    }


    \: storeDownPoint (void; Inspector this, Event event)
    {
        State state = data();
        recordPixelInfo(event);
        if (state.pixelInfo eq nil || state.pixelInfo.empty()) return;

        let pinfo  = state.pixelInfo.front(),
            sName  = sourceNameWithoutFrame(pinfo.name),
            ip     = state.pointerPosition;

        _downPoint = imageToEventSpace(sName, _colorPointImage) - ip;
    }

    \: drag (void; Inspector this, Event event)
    {
        this._dragging = true;

        State state = data();
        recordPixelInfo(event);
        if (state.pixelInfo eq nil || state.pixelInfo.empty()) return;

        if (!_inCloseArea)
        {
            let ip     = state.pointerPosition,
                dp     = _downPoint,
                offpt  = ip + dp, 
                pinfo  = imagesAtPixel(offpt, nil, true).front(),
                sName  = sourceNameWithoutFrame(pinfo.name),
                sp = getSourcePixel(pinfo, sName, frame());

            if (_showFBColor)
            {
                _glReadPending = true;
            }

            _colorSampling   = true;
            _currentColor    = sp.v;
            _colorPointPixel = Point(pinfo.px, pinfo.py);
            _colorPointEvent = offpt;
            _colorPointImage = eventToImageSpace(sName, _colorPointEvent);
            _colorSourceInfo = pinfo;
            _colorFrame      = frame();
            _updateAverageColor = false;
        }

        redraw();
    }

    \: writeSettings (void; Inspector i)
    {
        writeSetting("Inspector", "colorScale", SettingsValue.Float(i._colorScale));
        writeSetting("Inspector", "showFBColor", SettingsValue.Bool(i._showFBColor));
        writeSetting("Inspector", "pointerStyle", SettingsValue.Int(i._pointerStyle));
        writeSetting("Inspector", "closeButtonStyle", SettingsValue.Int(i._closeButtonStyle));
    }

    \: optFinalColor (void; Inspector i, bool val, Event event)
    {
        i._showFBColor = val;
        i._graphStateChanged = true;
        i._averageColor = Color(0, 0, 0);
        i._colorSamples = 0;
        writeSettings(i);
    }

    \: isShowingFinalColor (int; Inspector i, bool val) 
    { 
        if i._showFBColor == val then CheckedMenuState else UncheckedMenuState; 
    }

    \: optNumericScale (void; Inspector i, int bits, Event event)
    {
        i._colorScale = math.pow(2, float(bits)) - 1;
        writeSettings(i);
    }

    \: isNumericScale (int; Inspector i, int bits)
    {
        if i._colorScale == math.pow(2, float(bits)) - 1
                    then CheckedMenuState
                    else UncheckedMenuState;
    }

    \: optPointerStyle (void; Inspector i, int style, Event event)
    {
        i._pointerStyle = style;
        writeSettings(i);
    }

    \: isPointerStyle (int; Inspector i, int style)
    {
        if i._pointerStyle == style
                    then CheckedMenuState
                    else UncheckedMenuState;
    }

    \: optCloseButtonStyle (void; Inspector i, int style, Event event)
    {
        i._closeButtonStyle = style;
        writeSettings(i);
    }

    \: isCloseButtonStyle (int; Inspector i, int style)
    {
        if i._closeButtonStyle == style
                    then CheckedMenuState
                    else UncheckedMenuState;
    }

    \: activeMenuState (int; ) { UncheckedMenuState; }
    
    \: popupOpts (void; Inspector this, Event event)
    {

        popupMenu(event, Menu {
                {"Inspector", nil, nil, \: (int;) { DisabledMenuState; }},
                {"_", nil},
                {"Final Rendered Color",
                        optFinalColor(this, true,), nil,
                        \: (int;) { isShowingFinalColor(this, true); }},
                {"Source Color",
                        optFinalColor(this, false,), nil,
                        \: (int;) { isShowingFinalColor(this, false); }},
                {"_", nil},
                {"Normalized [0.0, 1.0]",
                        optNumericScale(this, 1,), nil,
                        \: (int;) { isNumericScale(this, 1); }},
                {"8 Bit [0, 255]",
                        optNumericScale(this, 8,), nil,
                        \: (int;) { isNumericScale(this, 8); }},
                {"10 Bit [0, 1023]",
                        optNumericScale(this, 10,), nil,
                        \: (int;) { isNumericScale(this, 10); }},
                {"12 Bit [0, 4095]",
                        optNumericScale(this, 12,), nil,
                        \: (int;) { isNumericScale(this, 12); }},
                {"16 Bit [0, 65535]",
                        optNumericScale(this, 16,), nil,
                        \: (int;) { isNumericScale(this, 16); }},
                {"_", nil},
                {"Plain Pointer",
                        optPointerStyle(this, PointerStylePlain,), nil,
                        \: (int;) { isPointerStyle(this, PointerStylePlain); }},
                {"Cross Pointer",
                        optPointerStyle(this, PointerStyleCross,), nil,
                        \: (int;) { isPointerStyle(this, PointerStyleCross); }},
                {"Box Pointer",
                        optPointerStyle(this, PointerStyleBox,), nil,
                        \: (int;) { isPointerStyle(this, PointerStyleBox); }},
                {"_", nil},
                {"Close Button Always",
                        optCloseButtonStyle(this, CloseButtonAlways,), nil,
                        \: (int;) { isCloseButtonStyle(this, CloseButtonAlways); }},
                {"Close Button When Nearby",
                        optCloseButtonStyle(this, CloseButtonNearby,), nil,
                        \: (int;) { isCloseButtonStyle(this, CloseButtonNearby); }},
                {"_", nil},
                {"Close Inspector",
                        \: (void; Event event) { this.toggle(); }, nil,
                        \: (int;) { UncheckedMenuState; }},
            });
    }

    method: inputsChanged (void; Event event)
    {
        event.reject();
        _inputsChanged = true;
        redraw();
    }

    method: graphStateChanged (void; Event event)
    {
        event.reject();
        _graphStateChanged = true;
    }

    method: nearMenuTriangle (bool; Event event)
    {
        State state = data();

        let domain = event.subDomain(),
            p      = event.relativePointer(),
            m      = state.config.bevelMargin,
            tri    = vec2f(_widW/devicePixelRatio() - 32, domain.y-0.7*m),
            pc     = p - tri,
            d      = mag(pc),
            near   = d < 9;

        return near;
    }

    method: inspectorMove (void; Event event)
    {
        move (this, event);

        _nearTriangle = nearMenuTriangle(event);
    }

    method: inspectorRelease (void; Event event)
    {
        if (nearMenuTriangle(event))
        {
            popupOpts(this, event);
        }
        else release(this,event, \: (void;) { Widget.toggle(this); });
    }

    method: Inspector (Inspector; string name)
    {
        this.init(name,
                  [ ("pointer-1--push", storeDownPoint(this,), "Move Inspector"),
                    ("pointer-1--drag", drag(this,), "Move Inspector"),
                    ("pointer-1--release", inspectorRelease, ""),
                    ("pointer-3--push", popupOpts(this,), "Popup Inspector Options"),
                    ("pointer--move", inspectorMove, ""),
                    ("graph-node-inputs-changed", inputsChanged, ""),
                    ("graph-state-change", graphStateChanged, ""),
                    ("stylus-pen--push", storeDownPoint(this,), "Move Inspector"),
                    ("stylus-pen--drag", drag(this,), "Move Inspector"),
                    ("stylus-pen--release", release(this,, \: (void;) { Widget.toggle(this); }), ""),
                    ("stylus-eraser--push", popupOpts(this,), "Popup Inspector Options"),
                    ("stylus-pen--move", move(this,), "") ],
                  false);

        //
        //  These are in the global event table
        //

        bind("pointer-1--shift--drag", sampleAtPoint(this,), "Move Inspector");
        bind("pointer-1--shift--push", beginSampling(this,), "Show Inspector");
        bind("stylus-pen--shift--drag", sampleAtPoint(this,), "Move Inspector");
        bind("stylus-pen--shift--push", beginSampling(this,), "Show Inspector");

        try
        {
            let SettingsValue.Bool  fbc = readSetting("Inspector", "showFBColor",
                                                      SettingsValue.Bool(false)),
                SettingsValue.Float s   = readSetting("Inspector", "colorScale",
                                                      SettingsValue.Float(math.pow(2, 1) - 1)),
                SettingsValue.Int   ps  = readSetting("Inspector", "pointerStyle",
                                                      SettingsValue.Int(PointerStylePlain)),
                SettingsValue.Int   cbs = readSetting("Inspector", "closeButtonStyle",
                                                      SettingsValue.Int(CloseButtonNearby));

            _showFBColor      = fbc;
            _colorScale       = s;
            _pointerStyle     = ps;
            _closeButtonStyle = cbs;
        }
        catch (...)
        {
            ; // nothing
        }

        _xFlipped = false;
        _yFlipped = false;
        _inputsChanged = false;
        _graphStateChanged = false;
        _glReadPending = false;
        _updateAverageColor = true;
        _skipNextSampleAtPoint = false;
        _nearTriangle = false;
    }

    method: toggle (void; )
    {
        if (!_active) _colorSourceInfo = nil;
        Widget.toggle(this);
    }

    method: _drawCross (void; Point p, float t)
    {
        let devicePixelRatio = devicePixelRatio(),
            x0 = Vec2(2*devicePixelRatio, 0),
            x1 = Vec2(7*devicePixelRatio, 0),
            y0 = Vec2(0, 2*devicePixelRatio),
            y1 = Vec2(0, 7*devicePixelRatio);

        glLineWidth(3.0*devicePixelRatio);

        glColor(0.5-t,0.5-t,0.5-t,1);

        glBegin(GL_LINES);
        glVertex(p + x0); glVertex(p + x1);
        glVertex(p - x0); glVertex(p - x1);
        glVertex(p + y0); glVertex(p + y1);
        glVertex(p - y0); glVertex(p - y1);
        glEnd();

        glLineWidth(1.0*devicePixelRatio);
        glColor(0.5+t,0.5+t,0.5+t,1);

        glBegin(GL_LINES);
        glVertex(p + x0); glVertex(p + x1);
        glVertex(p - x0); glVertex(p - x1);
        glVertex(p + y0); glVertex(p + y1);
        glVertex(p - y0); glVertex(p - y1);
        glEnd();
    }

    method: render (void; Event event)
    {
        State state = data();
        if (state.pixelInfo eq nil || state.pixelInfo.empty()) return;

        if (_colorSourceInfo eq nil)
        {
            initializeNoPoint(this);
        }

        let isRenderEvent = event.name() == "render",
            devicePixelRatio = (if isRenderEvent then devicePixelRatio() else 1.0),
            pinfo = imagesAtPixel(_colorPointEvent, nil, true).front(),
            sName = sourceNameWithoutFrame(pinfo.name),
            sp = getSourcePixel(pinfo, sName, frame());

        let (w, h, bits, ch, hasfloat, nplanes) = sourceImageStructure(sName);

        let domain = event.domain(),
            bg     = state.config.bg,
            fg     = state.config.fg,
            margin = state.config.bevelMargin*devicePixelRatio,
            m2     = margin,
            md     = margin / 2,
            p      = imageToEventSpace(sName, _colorPointImage)*devicePixelRatio,
            c      = if _showFBColor then _fbColor else _currentColor;

        if (event.domainVerticalFlip()) p.y = domain.y - 1 - p.y;

        gltext.size(state.config.inspectorTextSize * devicePixelRatio);

        if (_colorSampling && (frame() != _colorFrame || _inputsChanged || _graphStateChanged || _glReadPending))
        {
            if (_showFBColor)
            {
                float[] pixels;
                glReadPixels(_colorPointEvent.x*devicePixelRatio, _colorPointEvent.y*devicePixelRatio, 1, 1, GL_RGBA, pixels);
                _fbColor = Color(pixels[0], pixels[1], pixels[2], pixels[3]);
                c = _fbColor;
            }
            else
            {
                _currentColor  = sp.v;
                c = _currentColor;
            }

            // This should be true most of the time, only changing when we are dragging [drag()]
            // (not to confuse with shift-dragging [sampleAtPoint()], which does update the _averageColor var)
            if(_updateAverageColor)
            {
                _averageColor += c;
                _colorSamples += 1;
            }

            _colorFrame    = frame();
            _inputsChanged = false;
            _graphStateChanged = false;
            _glReadPending = false;
            _updateAverageColor = true;
        }

        let a       = _averageColor / float(_colorSamples),
            ia      = float(w) / float(h),
            showAvg = _colorSamples > 1,
            yp      = int(_colorPointImage.y * (float(h) - 0.5)),
            xp      = int(_colorPointImage.x / ia * (float(w) - 0.5)),
            cs      = c * _colorScale;

        if (string(cs.x) == "nan") cs = Color(0,0,0,1);
        if (string(cs.y) == "nan") cs = Color(0,0,0,1);
        if (string(cs.z) == "nan") cs = Color(0,0,0,1);
        if (string(cs.w) == "nan") cs = Color(0,0,0,1);

        let fmt     = if _colorScale == 1.0 then "%f" else "%- 11.0f",
            R       = fmt % cs.x,
            G       = fmt % cs.y,
            B       = fmt % cs.z,
            A       = fmt % cs.w,
            label   = if _showFBColor then "Final Color" else "Source Color";

        (string,string)[] text = (string,string)[]{};

        let imageX = int(_colorPointPixel.x/sp.pasp),
            imageY = int(_colorPointPixel.y);

        //
        //  If we can figure out what the "normal" (meaning 0,0 at lower left)
        //  coords are for this pixel show them in addition to the native image
        //  file coords.
        //

        let extraLine = 0;
        try
        {
                text.push_back(("Norm. Pixel", "%d, %d" % (sp.normX, sp.normY)));
                extraLine = 1;
        }
        catch (...) {;}

        text.push_back(("Native Pixel", "%d, %d" % (imageX, imageY)));
        text.push_back(("", ""));
        text.push_back(("Alpha", A));
        text.push_back(("Blue",  B));
        text.push_back(("Green", G));
        text.push_back(("Red",   R));
        text.push_back(("", ""));
        text.push_back((label, ""));

        if (showAvg)
        {
            let as = a * _colorScale;
            text.push_back(("", ""));
            text.push_back(("Alpha", (fmt % as.w)));
            text.push_back(("Blue",  (fmt % as.z)));
            text.push_back(("Green", (fmt % as.y)));
            text.push_back(("Red",   (fmt % as.x)));
            text.push_back(("", ""));
            text.push_back(("Average", ""));
        }

        setupProjection(domain.x, domain.y, event.domainVerticalFlip());

        let vs     = viewSize(),
            ms     = margins(),
            xmin   = ms[0],
            xmax   = vs[0] - ms[1],
            ymin   = ms[3],
            ymax   = vs[1] - ms[2],
            bounds = nameValuePairBounds(text, margin);

        _widW      = bounds._0[0];
        let widH   = bounds._0[1],
            offset = m2 * 2,
            flipX  = ((p.x + offset + _widW > xmax) || (_xFlipped && (p.x > offset + _widW))),
            flipY  = ((p.y + widH > ymax) || (_yFlipped && (p.y > widH + offset))),
            x      = if (flipX) then p.x - offset - _widW else p.x + offset,
            y      = if (flipY) then p.y - widH else p.y + offset;

        _xFlipped = flipX;
        _yFlipped = flipY;

        /*
        print ("%s %s in %s - %s, %s - %s, flip %s %s, wid %s %s\n" % (
                p.x, p.y, xmin, xmax, ymin, ymax, flipX, flipY, _widW, widH));
        */

        let (tbox, nbounds, vbounds, nw)
            = drawNameValuePairs(text, fg, bg, x, y, margin);

        let fa   = int(gltext.ascenderHeight()),
            fd   = int(gltext.descenderDepth()),
            th   = fa - fd,
            cx   = x + nw + md,
            cy   = y + th * (6 + extraLine) + fd + th/2,
            ax   = cx,
            ay   = cy + th * 7,
            emin = vec2f(x, y) - vec2f(margin, margin),
            emax = emin + tbox + vec2f(margin*2.0, 0.0);

        this.updateBounds(emin/devicePixelRatio, emax/devicePixelRatio);

        glColor(bg);
        glPushAttrib(GL_ENABLE_BIT);
        glEnable(GL_BLEND);
        glEnable(GL_LINE_SMOOTH);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        let startX = if (flipX) then p.x - 1 else p.x + 1,
            startY = if (flipY) then p.y - 1 else p.y + 1,
            endX   = if (flipX) then p.x - 0.6 * offset else p.x + 0.7 * offset,
            endY   = if (flipY) then p.y - 0.6 * offset else p.y + 0.7 * offset;

        glBegin(GL_LINES);
        glVertex(startX, startY);
        glVertex(endX, endY);
        glEnd();

        let hStartX = if (flipX) then startX + 1 else startX - 1,
            hEndX   = if (flipX) then endX + 1 else endX - 1;

        glColor(fg * 0.7);
        glBegin(GL_LINES);
        glVertex(hStartX, startY);
        glVertex(hEndX, endY);
        glEnd();

        if (_pointerStyle == PointerStyleBox)
        {
            glColor(bg);
            let offset = 2*devicePixelRatio;
            glLineWidth(2.0*devicePixelRatio);
            drawRect(GL_LINE_LOOP,
                    p+Vec2( offset,  offset),
                    p+Vec2( offset, -offset),
                    p+Vec2(-offset, -offset),
                    p+Vec2(-offset,  offset));
            offset = 4*devicePixelRatio;
            glColor(0.8*fg);
            drawRect(GL_LINE_LOOP,
                    p+Vec2( offset,  offset),
                    p+Vec2( offset, -offset),
                    p+Vec2(-offset, -offset),
                    p+Vec2(-offset,  offset));
            glLineWidth(1.0*devicePixelRatio);
        }
        else
        if (_pointerStyle == PointerStyleCross)
        {
            _drawCross(p, 0.3);
        }

        glDisable(GL_BLEND);


        glColor(c);

        drawRect(GL_QUADS,
                 Vec2(cx, cy),
                 Vec2(cx + th * 3, cy),
                 Vec2(cx + th * 3, cy + th * 2),
                 Vec2(cx, cy + th * 2));

        if (showAvg)
        {
            glColor(a);

            drawRect(GL_QUADS,
                     Vec2(ax, ay),
                     Vec2(ax + th * 3, ay),
                     Vec2(ax + th * 3, ay + th * 2),
                     Vec2(ax, ay + th * 2));
        }

        //  Menu triangle

        glColor((if _nearTriangle then 0.9 else 0.7)*fg);
        draw (triangleGlyph, x + _widW - 52*devicePixelRatio, tbox.y + y - 1.7*margin , 90.0, 8.0*devicePixelRatio, false);

        if (_inCloseArea || (_containsPointer && _closeButtonStyle == CloseButtonAlways))
        {
            let color = fg * (if _inCloseArea then 0.9 else 0.7);
            drawCloseButton(x - md,
                            tbox.y + y - margin - md/2,
                            margin/2, bg, color);
        }

        glDisable(GL_LINE_SMOOTH);
        glPopAttrib();
    }
}

\: constructor (Widget; string name)
{
    return Inspector(name);
}

}
