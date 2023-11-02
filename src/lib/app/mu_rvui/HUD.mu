//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Heads up Display Widgets
//
module: HUD {
use rvtypes;
use glyph;
use app_utils;
use math_linear;
use math;
use math_util;
use commands;
use extra_commands;
use gl;
use glu;
require io;
require runtime;

//----------------------------------------------------------------------
//
//  ImageInfo
//

class: ImageInfo : Widget
{
    bool _wrap;

    method: optWrap (void; Event event)
    {
        _wrap = !_wrap;
        writeSetting("ImageInfo", "wrap", SettingsValue.Bool(_wrap));
    }

    method: isWrapping (int; )
    { 
        if (_wrap) then CheckedMenuState else UncheckedMenuState; 
    }

    method: popupOpts (void; Event event)
    {
        popupMenu(event, Menu {
                {"Image Info", nil, nil, \: (int;) { DisabledMenuState; }},
                {"_", nil},
                {"Wrap Long Fields", optWrap, nil, isWrapping},
            });
    }

    method: ImageInfo (ImageInfo; string name)
    {
        init(name, 
             [ ("pointer-1--push", storeDownPoint(this,), "Move Image Info"),
               ("pointer-1--drag", drag(this,), "Move Image Info"),
               ("pointer-1--release", release(this,, \: (void;) { Widget.toggle(this); }), ""),
               ("pointer-3--push", popupOpts, "Image Info Options"),
               ("pointer--move", move(this,), ""),
               ("stylus-pen--push", storeDownPoint(this,), "Move Image Info"),
               ("stylus-pen--drag", drag(this,), "Move Image Info"),
               ("stylus-pen--release", release(this,, \: (void;) { Widget.toggle(this); }), ""),
               ("stylus-eraser--push", popupOpts, "Image Info Options"),
               ("stylus-pen--move", move(this,), "") ],
             false);

        _x = 40;
        _y = 60;
        try
        {
            let SettingsValue.Bool w = readSetting("ImageInfo", "wrap", SettingsValue.Bool(true));
            _wrap = w;
        }
        catch (...)
        {
            _wrap = true;
        }
    }

    method: activate (void; )
    {
        State state = data();
        state.perPixelInfoValid = false;

        Widget.activate(this);
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
        runtime.gc.push_api(0);
        updatePixelInfo(nil);
        runtime.gc.pop_api();

        State state = data();

        let pinfo = state.pixelInfo;
        string iname = nil;
        if (pinfo neq nil)
        {
            for_each (info; pinfo)
            {
                if (nodeType(nodeGroup(info.node)) == "RVSourceGroup")
                {
                    iname = info.name;
                    break;
                }
            }
            if (iname eq nil && !pinfo.empty()) iname = pinfo.front().name;
        }
        if (iname eq nil) return;

        let attrs   = sourceAttributes(iname);
            if (attrs eq nil)  return;

        let chans   = sourceDisplayChannelNames(iname),
            domain  = event.domain(),
            bg      = state.config.bg,
            fg      = state.config.fg,
            err     = isCurrentFrameError(),
            sName   = "";

        if (attrs.back()._0 == "RVSource") 
        {
            let nodeName = string.split(string.split(attrs.back()._1, "/").front(), ".").front();

            // should this stuff be moved to source info?

            let retimeNodes = nodesInEvalPath (frame(), "RVRetime", nodeName),
                retimedFPS = if (retimeNodes.empty()) then 0.0 else getFloatProperty(retimeNodes[0] + ".output.fps").back();

            //
            // Search for audio-only sources in all layers/eyes
            //

            bool hasAudioAttrs = false;
            (string,string)[] src_attrs = {};
            try
            {
                let movies = getStringProperty(nodeName + ".media.movie");

                for_each (m; movies)
                {
                    let info = sourceMediaInfo (nodeName, m);
                    if (info.channels == 0 && info.audioChannels != 0)
                    {
                        hasAudioAttrs = true;
                        for_each (a; sourceAttributes(iname, m))
                        {
                            //
                            // For audio-only sources skip the pixel aspect and
                            // hasAudio attrs. We will use any pixel aspect
                            // found if there is video and we will update the
                            // audio state as well.
                            //

                            if (a._0 != "Audio")
                            {
                                src_attrs.push_back((a._0, a._1));
                            }
                        }
                    }
                }
            }
            catch (...) { ; } // image source may not have media

            //
            // If we have a audio-only attrs, then reorder the list of attrs
            // here to put audio info at the bottom. Also append the retimed
            // FPS to the FPS attr if one exists.
            //

            if (retimedFPS != 0.0 || hasAudioAttrs)
            {
                for_each (a; attrs)
                {
                    if (a._0 == "FPS" && retimedFPS != 0.0) a._1 = "%s (retimed to %g)" % (a._1, retimedFPS);
                    if (a._0 == "Audio" && hasAudioAttrs) a._1 = "Yes";
                    src_attrs.push_back(a);
                }
                attrs = src_attrs;
            }

            sName = uiName(nodeName);
            attrs.push_back(("Source", sName));
        }

        if (attrs eq nil) return;

        let (w, h, bits, ch, hasfloat, nplanes) = sourceImageStructure(iname);

        //  Additional info

        if (chans eq nil)
        {
            attrs = (string,string)[] {("-", "Session is Empty")};
        }
        else
        {
            string dchans;
            for_index (i; chans)
            {
                let chname = chans[i];
                dchans += "%s%s" % (if i != 0 then ", " else "", chname);
            }

            if (w != 0 && !err)
            {
                if (state.currentInput neq nil) 
                {
                    let inName = uiName(state.currentInput);
                    if (inName != sName) attrs.push_back(("Input", inName));
                }

                attrs.push_back(("", ""));
                attrs.push_back(("DisplayChannels", dchans));

                RenderedSourceInfo rinfo = nil;
                for_each (rsrc; sourcesRendered())
                {
                    if (rsrc.name == iname) { rinfo = rsrc; break; }
                }

                if (rinfo neq nil)
                {
                    attrs.push_back(("DisplayResolution", "%d x %d, %d ch, %d bit%s%s"
                                     % (rinfo.width,
                                        rinfo.height,
                                        if rinfo.planar then 1 else rinfo.numChannels,
                                        rinfo.bitDepth,
                                        if rinfo.floatingPoint then " floating point" else "",
                                        if rinfo.planar then ", %d planes" % rinfo.numChannels else "")));
                }

                attrs.push_back(("Resolution", "%d x %d, %d ch, %d bit%s%s"
                                 % (w, h, ch, bits,
                                    if hasfloat then " floating point" else "",
                                    if nplanes > 1 then ", %d planes" % nplanes else "")));
            }
        }

        if (err)
        {
            fg = state.config.fgErr;
            bg = state.config.bgErr;
        }

        gltext.size(state.config.infoTextSize);
        setupProjection(domain.x, domain.y, event.domainVerticalFlip());

        let margin  = state.config.bevelMargin,
            x       = _x + margin,
            y       = _y + margin,
            wrap    = if (_wrap) then 80 else 0,
            tbox    = drawNameValuePairs(expandNameValuePairs(attrs, wrap),
                                         fg, bg, x, y, margin)._0,
            emin    = vec2f(_x, _y),
            emax    = emin + tbox + vec2f(margin*2.0, 0.0);

        if (_inCloseArea)
        {
            drawCloseButton(x - margin/2,
                            tbox.y + y - margin - margin/4,
                            margin/2, bg, fg);
        }

        updateBounds(emin, emax);
    }
}

//----------------------------------------------------------------------
//
//  InfoStrip
//

class: InfoStrip : Widget
{
    bool _showFilename;
    bool _scaleWithResolution;

    method: showFilename (EventFunc; bool useFilename)
    {
        \: (void; Event event)
        {
            this._showFilename = useFilename;
            writeSetting("InfoStrip", "showFilename", SettingsValue.Bool(_showFilename));
        };
    }

    method: isFilename (MenuStateFunc; bool checkFilename)
    {
        \: (int;)
        {
            if ((checkFilename && this._showFilename) ||
                (!checkFilename && !this._showFilename))
                then CheckedMenuState else UncheckedMenuState;
        };
    }

    method: toggleScaleWithResolution (void; Event event)
    {
        _scaleWithResolution = !_scaleWithResolution;
        writeSetting("InfoStrip", "scaleWithResolution", SettingsValue.Bool(_scaleWithResolution));
    }

    method: isScaling (int; )
    { 
        if (_scaleWithResolution) then CheckedMenuState else UncheckedMenuState; 
    }

    method: popupOpts (void; Event event)
    {
        popupMenu(event, Menu {
                {"Info Strip", nil, nil, \: (int;) { DisabledMenuState; }},
                {"_", nil},
                {"Show Filename", showFilename(true), nil, isFilename(true)},
                {"Show UI Name", showFilename(false), nil, isFilename(false)},
                {"Scale with Resolution", toggleScaleWithResolution, nil, isScaling},
            });
    }

    method: InfoStrip (InfoStrip; string name)
    {
        init(name, 
             [ ("pointer-1--push", storeDownPoint(this,), "Drag Info Strip"),
               ("pointer-1--drag", drag(this,), "Drag Info Strip"),
               ("pointer-1--release", release(this,, \: (void;) { Widget.toggle(this); }), ""),
               ("pointer-3--push", popupOpts, "Info Strip Options"),
               ("pointer--move", move(this,), ""),
               ("stylus-pen--push", storeDownPoint(this,), "Drag Info Strip"),
               ("stylus-pen--drag", drag(this,), "Drag Info Strip"),
               ("stylus-pen--release", release(this,, \: (void;) { Widget.toggle(this); }), ""),
               ("stylus-eraser--push", popupOpts, "Info Strip Options"),
               ("stylus-pen--move", move(this,), "") ],
             false);
        _y = 50;
        try
        {
            let SettingsValue.Bool isFilenameSelected = readSetting("InfoStrip", "showFilename", SettingsValue.Bool(true));
            let SettingsValue.Bool isScalingSelected = readSetting("InfoStrip", "scaleWithResolution", SettingsValue.Bool(true));
            _showFilename = isFilenameSelected;
            _scaleWithResolution = isScalingSelected;
        }
        catch (...)
        {
            _showFilename = true;
            _scaleWithResolution = true;
        }
    }

    method: activate (void; )
    {
        State state = data();
        state.perPixelInfoValid = false;

        Widget.activate(this);
    }

    method: render (void; Event event)
    {
        runtime.gc.push_api(0);
        updatePixelInfo(nil);
        runtime.gc.pop_api();

        State state = data();
        use io;

        let domain = event.domain(),
            w      = domain.x,
            h      = domain.y,
            pinfo  = state.pixelInfo,
            attrs  = sourceAttributes(if (pinfo neq nil && !pinfo.empty()) 
                                          then pinfo.front().name
                                          else nil);

        string iname = if (pinfo neq nil && !pinfo.empty()) 
                          then pinfo.front().name
                          else nil;
        if (iname eq nil) return;


        //  Don't allow the widget to be dragged off the window or
        //  into the top/bottom margin.
        //
        _y = max(min(_y, h - 30 - margins()[2]), margins()[3]);

        if (attrs eq nil)  return;

        string filename = "unknown      ";

        if (_showFilename)
        {
            for_each (a; attrs)
            {
                let (name, value) = a;

                if (name == "File")
                {
                    filename = path.basename(value) + "      ";
                    break;
                }
            }
        }
        else
        {
            filename = uiName(pinfo.front().node) + "      ";
        }

        state.filestripX1 = 0;

        int textSize = 20;
        if (_scaleWithResolution)
        {
            int baseResolution = 1280;
            int fontSize = 20;
            int minTextSize = 12;
            textSize = max((w / baseResolution) * fontSize, minTextSize);
        }

        gltext.size(textSize);
        setupProjection(w, h, event.domainVerticalFlip());

        let b      = gltext.bounds(filename),
            margin = state.config.bevelMargin,
            m2     = margin,
            md     = margin / 2,
            tw     = b[2],
            th     = b[3],
            x      = w - tw,
            y      = _y + md;

        state.filestripX1 = y + th + 5;
        glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        let bg = state.config.bgFeedback,
            fg = state.config.fgFeedback;

        drawTextWithCartouche(x, y, filename, textSize,
                                fg, bg,
                                circleGlyph, bg * .7);

        updateBounds(vec2f(x - margin, y - md),
                        vec2f(x + tw, y + th + md));

        if (_inCloseArea)
        {
            drawCloseButton(x - md, th + y, md, fg, bg);
        }

        glPopAttrib();
    }
}

//----------------------------------------------------------------------
//
//  ProcessInfo
//

class: ProcessInfo : Widget
{
    method: ProcessInfo (ProcessInfo; string name)
    {
        init(name, 
             [ ("pointer-1--push", storeDownPoint(this,), ""),
               ("pointer-1--drag", drag(this,), ""),
               ("pointer-1--release", release(this,, \: (void;) { Widget.toggle(this); }), ""),
               ("pointer--move", move(this,), ""),
               ("stylus-pen--push", storeDownPoint(this,), ""),
               ("stylus-pen--drag", drag(this,), ""),
               ("stylus-pen--release", release(this,, \: (void;) { Widget.toggle(this); }), ""),
               ("stylus-pen--move", move(this,), "") ],
             false);

        //
        //  Make sure we don't suck up any events until we have a rendered
        //  widget to receive them.
        //
        setEventTableBBox(_modeName, "global", vec2f(0,0), vec2f(0,0));

        _x = 40;
        _y = 60;
    }

    \: killProcess (void; ExternalProcess p, Event ev)
    {
        let details = if (p._cancelDetails neq nil) then p._cancelDetails else ("Do you want to quit \"%s\"?" % p._name);
        int choice = alertPanel(true, // associated panel (sheet on OSX)
                                WarningAlert,
                                "Cancel?", details,
                                "Don't Quit", "Quit", nil);

        if (choice == 1) p.cancel();
        redraw();
    }

    method: render (void; Event event)
    {
        State state = data();

        let domain  = event.domain(),
            bg      = state.config.bg,
            fg      = state.config.fg,
            p       = state.externalProcess;
        
        float pcent = 0;
        string message = "";
        
        try
        {
            pcent = p._progress;
            message = p._lastMessage;
            if (message eq nil) message = "";
        }
        catch (...)
        {
            state.externalProcess = nil;
        }

        if (state.externalProcess eq nil) 
        {
            updateBounds (vec2f(0,0), vec2f(0,0));
            return;
        }

        gltext.size(state.config.infoTextSize);
        setupProjection(domain.x, domain.y, event.domainVerticalFlip());

        (string,string)[] pairs;

        pairs.push_back((" ", "%s" % message));
        pairs.push_back((p._name, " "));

        let margin  = state.config.bevelMargin,
            x       = _x + margin,
            y       = _y + margin,
            bgbar   = (fg + bg) / 4.0,
            fgbar   = (fg + bg) / 2.0,
            barsize = gltext.bounds("          ")[2] * 8,
            wwidth  = barsize + gltext.bounds(p._name + " 000.00%%")[2] + margin;

        let (tbox, nbounds, vbounds, nw) = drawNameValuePairs(expandNameValuePairs(pairs),
                                                              fg, bg, x, y, margin, 
                                                              0, 0, 
                                                              wwidth, 0);

        let md   = margin / 2,
            fa   = int(gltext.ascenderHeight()),
            fd   = int(gltext.descenderDepth()),
            th   = fa - fd,
            px   = x + nw + md,
            py   = y + th * 1 + fd + th/2,
            emin = vec2f(_x, _y),
            emax = emin + tbox + vec2f(margin*2.0, 0.0);

        let bwidth = barsize * pcent * .01;

        glColor(bgbar);

        drawRect(GL_QUADS, 
                 Vec2(px, py),
                 Vec2(px + barsize, py),
                 Vec2(px + barsize, py + th),
                 Vec2(px, py + th) );

        glColor(fgbar);

        drawRect(GL_QUADS,
                 Vec2(px + 1, py + 1),
                 Vec2(px + 1 + bwidth, py + 1),
                 Vec2(px + 1 + bwidth, py + th - 1),
                 Vec2(px + 1, py + th - 1));

        let pcx = px + barsize,
            pcy = py,
            rad = th / 2,
            d   = mag(_downPoint - vec2f(pcx + 4 + rad, pcy + rad));

        _buttons = Button[] {{pcx + 4, pcy, rad * 2, rad * 2, killProcess(p,)}};
        gltext.writeAt(pcx + 3.0 * rad, pcy, "%-3.1f%%" % pcent);

        drawCloseButton(pcx + rad + 4, pcy + rad, rad, 
                        if (d <= rad) then Color(1,0,0,1) else fgbar, 
                        bg);

        if (_inCloseArea)
        {
            drawCloseButton(x - margin/2,
                            tbox.y + y - margin - margin/4,
                            margin/2, bg, fg);
        }

        updateBounds(emin, emax);
    }
}

//----------------------------------------------------------------------
//
//  SourceDetails
//

class: SourceDetails : Widget
{
    method: SourceDetails (SourceDetails; string name)
    {
        init(name, 
             [ ("pointer-1--push", storeDownPoint(this,), "Move Image Settings"),
               ("pointer-1--drag", drag(this,), "Move Image Settings"),
               ("pointer-1--release", release(this,, \: (void;) { Widget.toggle(this); }), ""),
               ("pointer--move", move(this,), ""),
               ("stylus-pen--push", storeDownPoint(this,), "Move Image Settings"),
               ("stylus-pen--drag", drag(this,), "Move Image Settings"),
               ("stylus-pen--release", release(this,, \: (void;) { Widget.toggle(this); }), ""),
               ("stylus-pen--move", move(this,), "") ],
             false);

        _x = 40;
        _y = 60;
    }

    method: activate (void; )
    {
        State state = data();
        state.perPixelInfoValid = false;

        Widget.activate(this);
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
        runtime.gc.push_api(0);
        updatePixelInfo(nil);
        runtime.gc.pop_api();

        State state = data();

        let attrs = (string,string)[]();
        string groupNode = nil;
        let pinfo = state.pixelInfo;
        if (pinfo neq nil && !pinfo.empty()) groupNode = nodeGroup(pinfo.front().node);

        if (groupNode eq nil) 
        {
            let nodes = metaEvaluateClosestByType(frame(), "RVSourceGroup");
            if (nodes.empty()) return;
            groupNode = nodes[0].node;
        }
        if (groupNode eq nil) return;

        \: either (string; string a, string b) { if a eq nil then b else a; }

        \: nodeTypeInGroupList (string; string[] nodes, string typeName)
        {
            for_each (n; nodes) if (nodeType(n) == typeName) return n;
            return nil;
        }

        \: stringProp (string; string node, string prop)
        {
            getStringProperty("%s.%s" % (node, prop)).front();
        }

        \: floatProp (float; string node, string prop)
        {
            getFloatProperty("%s.%s" % (node, prop)).front();
        }

        \: vec3fProp (math.vec3f; string node, string prop)
        {
            let value = math.vec3f(0,0,0);
            try
            {
                let array = getFloatProperty("%s.%s" % (node, prop));
                value = math.vec3f(array[0], array[1], array[2]);
            }
            catch (...) { ; }
            value;
        }

        \: intProp (int; string node, string prop)
        {
            getIntProperty("%s.%s" % (node, prop)).front();
        }

        \: maybeAddVec3fAttr (void; string node, string prop, string label, 
                              float x, float y, float z,
                              string fmt = "%g %g %g")
        {
            try
            {
                let value = vec3fProp(node, prop);
                if (value != math.vec3f(x,y,z)) 
                {
                    attrs.push_back((label, fmt % (value.x, value.y, value.z)));
                }
            }
            catch (...)
            {
                ;
            }
        }

        \: maybeAddFloatAttr (void; string node, string prop, string label, 
                              float defaultValue, string fmt = "%g")
        {
            try
            {
                let value = floatProp(node, prop);
                if (value != defaultValue) attrs.push_back((label, fmt % value));
            }
            catch (...)
            {
                ;
            }
        }

        \: maybeAddIntAttr (void; string node, string prop, string label, 
                            int defaultValue, string fmt = "%d")
        {
            try
            {
                let value = intProp(node, prop);
                if (value != defaultValue) attrs.push_back((label, fmt % value));
            }
            catch (...)
            {
                ;
            }
        }

        \: maybeAddLUTAttr (void; string node, string title)
        {
            try
            {
                int active = intProp(node, "lut.active");
                if (active == 1)
                {
                    string lutPath = stringProp(node, "lut.file");
                    if (lutPath == "") lutPath = "<programmatic>";
                    attrs.push_back((title, lutPath));
                }
            }
            catch (...)
            {
                ;
            }
        }

        \: maybeAddCDLAttrs (void; string node, string title)
        {
            try
            {
                let cdlActive     = getIntProperty(node + ".CDL.active").front(),
                    cdlSlope      = getFloatProperty(node + ".CDL.slope"),
                    cdlOffset     = getFloatProperty(node + ".CDL.offset"),
                    cdlPower      = getFloatProperty(node + ".CDL.power"),
                    cdlSaturation = getFloatProperty(node + ".CDL.saturation").front();

                if (cdlActive != 1) return;

                if (cdlSlope[0] != 1 || cdlSlope[1] != 1 || cdlSlope[2] != 1 ||
                    cdlOffset[0] != 0 || cdlOffset[1] != 0 || cdlOffset[2] != 0 ||
                    cdlPower[0] != 1 || cdlPower[1] != 1 || cdlPower[2] != 1 ||
                    cdlSaturation != 1)
                {
                    attrs.push_back(("%s Slope" % title,"%g, %g, %g" % (cdlSlope[0],cdlSlope[1],cdlSlope[2])));
                    attrs.push_back(("%s Offset" % title,"%g, %g, %g" % (cdlOffset[0],cdlOffset[1],cdlOffset[2])));
                    attrs.push_back(("%s Power" % title,"%g, %g, %g" % (cdlPower[0],cdlPower[1],cdlPower[2])));
                    attrs.push_back(("%s Saturation" % title, "%g" % cdlSaturation));
                }
            }
            catch (...)
            {
                ;
            }
        }

        \: maybeAddICCAttrs (void; string node, string title)
        {
            try
            {
                int active = intProp(node, "node.active");
                if (active == 1)
                {
                    if (propertyExists("%s.inProfile.version" % node))
                    {
                        string url = stringProp(node, "inProfile.url");
                        string desc = stringProp(node, "inProfile.description");
                        attrs.push_back(("%s URL" % title, url));
                        attrs.push_back(("%s Description" % title, desc));
                    }
                    if (propertyExists("%s.outProfile.version" % node))
                    {
                        string url = stringProp(node, "outProfile.url");
                        string desc = stringProp(node, "outProfile.description");
                        attrs.push_back(("%s URL" % title, url));
                        attrs.push_back(("%s Description" % title, desc));
                    }
                }
            }
            catch (...)
            {
                ;
            }
        }

        let groupMembers          = nodesInGroup(groupNode),
            linearizePipelineNode = nodeTypeInGroupList(groupMembers, "RVLinearizePipelineGroup"),
            colorPipelineNode     = nodeTypeInGroupList(groupMembers, "RVColorPipelineGroup"),
            lookPipelineNode      = nodeTypeInGroupList(groupMembers, "RVLookPipelineGroup"),
            channelMapNode        = nodeTypeInGroupList(groupMembers, "RVChannelMap"),
            formatNode            = nodeTypeInGroupList(groupMembers, "RVFormat"),
            transformNode         = nodeTypeInGroupList(groupMembers, "RVTransform2D"),
            stereoNode            = nodeTypeInGroupList(groupMembers, "RVSourceStereo"),
            preCacheNode          = nodeTypeInGroupList(groupMembers, "RVCacheLUT"),
            sourceNode            = either(nodeTypeInGroupList(groupMembers, "RVFileSource"),
                                           nodeTypeInGroupList(groupMembers, "RVImageSource")),
            usingCacheOCIO        = false,
            usingFileOCIO         = false,
            usingLookOCIO         = false;

            string linearizeNode  = nil;
            string warpNode       = nil;
            string fileICCNode    = nil;
            string fileOCIONode   = nil;
            string colorNode      = nil;
            string lookNode       = nil;
            string lookOCIONode   = nil;

        try
        {
            let linearizeMembers  = nodesInGroup(linearizePipelineNode);
                linearizeNode     = nodeTypeInGroupList(linearizeMembers, "RVLinearize");
                fileICCNode       = nodeTypeInGroupList(linearizeMembers, "ICCLinearizeTransform");
                warpNode          = nodeTypeInGroupList(linearizeMembers, "RVLensWarp");
                fileOCIONode      = nodeTypeInGroupList(linearizeMembers, "OCIOFile");
                usingFileOCIO     = fileOCIONode neq nil;
        }
        catch (...)
        {
            ;
        }

        try
        {
            let colorMembers      = nodesInGroup(colorPipelineNode);
                colorNode         = nodeTypeInGroupList(colorMembers, "RVColor");
        }
        catch (...)
        {
            ;
        }

        try
        {
            let lookMembers       = nodesInGroup(lookPipelineNode);
                lookNode          = nodeTypeInGroupList(lookMembers, "RVLookLUT");
                lookOCIONode      = nodeTypeInGroupList(lookMembers, "OCIOLook");
                usingLookOCIO     = lookOCIONode neq nil;
        }
        catch (...)
        {
            ;
        }

        //
        // Required "On"
        //

        attrs.push_back(("Source Name", uiName(groupNode)));

        if (formatNode neq nil)
        {
            maybeAddFloatAttr(formatNode, "geometry.scale", "Image Resolution", 1.0);
            //float imageRes = floatProp(formatNode, "geometry.scale");
            //if (imageRes != 1.0) attrs.push_back(("Image Resolution", "%g" % imageRes));

            string colorRes = "%d" % intProp(formatNode, "color.maxBitDepth");
            colorRes = if (colorRes == "0") then "Maximum Allowed" else colorRes + " bits";
            if (colorRes != "Maximum Allowed") attrs.push_back(("Color Resolution", colorRes));
        }

        let dscale = floatProp("#RVDispTransform2D", "transform.scale");

        if (pinfo neq nil && !pinfo.empty())
        {
            for_each (ri; renderedImages())
            {
                if (ri.name == pinfo.front().name)
                {
                    let totalView = viewSize(),
                        m = margins(),
                        w = ri.width * ri.pixelAspect,
                        h = ri.height,
                        sx = ri.modelMatrix[0,0],
                        d = Vec2(totalView.x - m[0] - m[1], totalView.y - m[2] - m[3]),
                        ia = w / h,
                        va = d.x / d.y,
                        wide = va > ia,
                        nsOverScl = if wide then h / d.y else w / d.x,
                        oneOverScl = (if wide then 1.0 / sx else 1.0) * nsOverScl;
                    attrs.push_back(("Scale", "%g" % (float(dscale / oneOverScl))));
                    break;
                }
            }
        }

        if (usingFileOCIO)
        {
            let cs = stringProp(fileOCIONode, "ocio.inColorSpace");
            attrs.push_back(("OpenColorIO File ColorSpace", cs));
        }
        else if (linearizeNode neq nil)
        {
            let l = intProp(linearizeNode, "color.logtype"),
                s = intProp(linearizeNode, "color.sRGB2linear"),
                r = intProp(linearizeNode, "color.Rec709ToLinear"),
                g = floatProp(linearizeNode, "color.fileGamma");

            string linXform = "No Conversion";
            if (r == 0 && l == 1 && s == 0 && g == 1.0) linXform = "Cineon Log";
            if (r == 0 && l == 2 && s == 0 && g == 1.0) linXform = "Viper Log";
            if (r == 0 && l == 3 && s == 0 && g == 1.0) linXform = "ALEXA LogC";
            if (r == 0 && l == 4 && s == 0 && g == 1.0) linXform = "ALEXA LogC Film";
            if (r == 0 && l == 5 && s == 0 && g == 1.0) linXform = "Sony S-Log";
            if (r == 0 && l == 6 && s == 0 && g == 1.0) linXform = "Red Log";
            if (r == 0 && l == 7 && s == 0 && g == 1.0) linXform = "Red Log Film";
            if (r == 0 && l == 0 && s == 1 && g == 1.0) linXform = "sRGB";
            if (r == 1 && l == 0 && s == 0 && g == 1.0) linXform = "Rec709";
            if (r == 0 && l == 0 && s == 0 && g != 1.0) linXform = "Gamma %g" % g;
            attrs.push_back(("Linearization Transform", linXform));

            maybeAddCDLAttrs(linearizeNode, "File CDL");
        }

        //
        // "On" if different than default
        //

        if (channelMapNode neq nil)
        {
            let chanMap = getStringProperty(channelMapNode + ".format.channels");
            if (chanMap.size() > 0)
            {
                attrs.push_back(("Source Image Channel Map Order", string.join(chanMap,",")));
            }
        }

        if (sourceNode neq nil)
        {
            let rangeInfo  = nodeRangeInfo(sourceNode),
                frameRange = rangeInfo.start + "-" + rangeInfo.end;

            attrs.push_back(("Frame Range", frameRange));
            attrs.push_back(("Source Frame Rate", "%g" % rangeInfo.fps));

            if (rangeInfo.cutIn > rangeInfo.start || rangeInfo.cutOut < rangeInfo.end)
            {
                attrs.push_back(("Source CutIn/CutOut", rangeInfo.cutIn + "-" + rangeInfo.cutOut));
            }

            maybeAddIntAttr(sourceNode, "group.rangeOffset", "Range Offset", 0);
            maybeAddFloatAttr(sourceNode, "group.audioOffset", "Audio Offset", 0);
            maybeAddFloatAttr(sourceNode, "group.volume", "Audio Volume", 1);
        }

        if (colorNode neq nil)
        {
            maybeAddVec3fAttr(colorNode, "color.gamma", "Gamma", 1, 1, 1);
            maybeAddVec3fAttr(colorNode, "color.offset", "Color Offset", 0, 0, 0);
            maybeAddVec3fAttr(colorNode, "color.exposure", "Relative Exposure", 0, 0, 0);
            maybeAddVec3fAttr(colorNode, "color.contrast", "Contrast", 0, 0, 0);
            maybeAddFloatAttr(colorNode, "color.saturation", "Saturation", 1);
            maybeAddFloatAttr(colorNode, "color.hue", "Hue", 0);
            maybeAddIntAttr(colorNode, "color.normalize", "Normalized Color", 0);
            maybeAddIntAttr(colorNode, "color.invert", "Inverted Color", 0);
            maybeAddIntAttr(colorNode, "luminanceLUT.active", "Luminance LUT", 0, "ON");

            maybeAddCDLAttrs(colorNode, "Look CDL");
        }

        if (transformNode neq nil)
        {
            string leftOrientation = "";
            if (intProp(transformNode, "transform.flip") != 0)
            {
                leftOrientation = "Flipped";
            }
            if (intProp(transformNode, "transform.flop") != 0)
            {
                if (leftOrientation != "") leftOrientation = leftOrientation + " & ";
                leftOrientation = leftOrientation + "Flopped";
            }
            if (leftOrientation != "")
            {
                attrs.push_back(("Orientation",leftOrientation));
            }
        
            maybeAddFloatAttr(transformNode, "transform.rotate", "Rotation", 0, "%g Degrees");
            maybeAddFloatAttr(transformNode, "transform.scale", "Image Scale", 1);
        }

        if (warpNode neq nil)
        {
            maybeAddFloatAttr(warpNode, "warp.pixelAspectRatio", "Pixel Aspect Ratio", 0);
        }
        
        if (fileICCNode neq nil)
        {
            maybeAddICCAttrs(fileICCNode, "File ICC");
        }
        
        if (usingFileOCIO)
        {
            ; // not sure yet
        }
        else if (linearizeNode neq nil)
        {
            let alphaType = intProp(linearizeNode, "color.alphaType");
            if (alphaType != 0)
            {
                string iaType = if (alphaType == 1) then "Premultiplied" else "Unpremultiplied";
                attrs.push_back(("Alpha Type", iaType));
            }
            maybeAddLUTAttr(linearizeNode, "File LUT");
        }

        if (usingLookOCIO)
        {
            let cs      = stringProp(lookOCIONode, "ocio_look.look"),
                forward = intProp(lookOCIONode, "ocio_look.direction") == 1;

            attrs.push_back(("OpenColorIO Look", "%s%s" % (cs, if forward then "" else " (Reversed)")));
        }
        else if (lookNode neq nil)
        {
            maybeAddLUTAttr(lookNode, "Look LUT");
        }

        if (usingCacheOCIO)
        {
            ; // not sure yet
        }
        else if (preCacheNode neq nil)
        {
            maybeAddLUTAttr(preCacheNode, "Cache LUT");
        }

        if (formatNode neq nil)
        {
            int cropOn = intProp(formatNode, "crop.active");
            if (cropOn == 1)
            {
                int xmin = intProp(formatNode, "crop.xmin");
                int xmax = intProp(formatNode, "crop.xmax");
                int ymin = intProp(formatNode, "crop.ymin");
                int ymax = intProp(formatNode, "crop.ymax");
                attrs.push_back(("Crop", "(%d, %d) (%d, %d)" % (xmin,ymin,xmax,ymax)));
            }

            int uncropOn = intProp(formatNode, "uncrop.active");
            if (uncropOn == 1)
            {
                int w = intProp(formatNode, "uncrop.width");
                int h = intProp(formatNode, "uncrop.height");
                int x = intProp(formatNode, "uncrop.x");
                int y = intProp(formatNode, "uncrop.y");
                attrs.push_back(("UnCrop", "%dx%d, offset (%d, %d)" % (w,h,x,y)));
            }
        }

        //
        // "On" if stereo
        //

        if (stereoNode neq nil)
        {
            maybeAddIntAttr(stereoNode, "stereo.swap", "Eyes Swapped", 0, "");

            string rightOrientation = "";
            if (intProp(stereoNode, "rightTransform.flip") != 0)
            {
                rightOrientation = "Flipped";
            }
            if (intProp(stereoNode, "rightTransform.flop") != 0)
            {
                if (rightOrientation != "") rightOrientation = rightOrientation + " & ";
                rightOrientation = rightOrientation + "Flopped";
            }
            if (rightOrientation != "")
            {
                attrs.push_back(("Stereo Right Eye Orientation",rightOrientation));
            }

            float relativeOffset = floatProp(stereoNode, "stereo.relativeOffset");
            if (relativeOffset != 0)
            {
                attrs.push_back(("Stereo Relative Eye Offset", "%g" % relativeOffset));
            }

            float rightOffset = floatProp(stereoNode, "stereo.rightOffset");
            if (rightOffset != 0)
            {
                attrs.push_back(("Stereo Right Eye Offset", "%g" % rightOffset));
            }
        }

        //
        // Always last
        //

        if (sourceNode neq nil)
        {
            let movies = getStringProperty(sourceNode + ".media.movie");
            for_each (m; movies)
            {
                string chunk = if m[0] == '/' then "/" else "";
                string[] chunks;
                string[] parts = m.split("/");
                for_index (i; parts)
                {
                    let p = parts[i];

                    if (i == 0) chunk = chunk + p;
                    else
                    {
                        if ((chunk + p).size() < 70)
                        {
                            chunk = chunk + "/" + p;
                        }
                        else
                        {
                            chunks.push_back(chunk + "/");
                            chunk = p;
                        }
                    }
                }
                if (chunk != "") chunks.push_back(chunk);
                bool newPath = true;
                for_each (chunk; chunks)
                {
                    string title = "";
                    if (newPath)
                    {
                        title = "File Path";
                        newPath = false;
                    }
                    attrs.push_back((title,chunk));
                }
            }
        }

        (string,string)[] details;
        for_index (i; attrs)
        {
            let value = attrs[attrs.size() - i - 1];
            details.push_back(value);
        }

        let domain  = event.domain(),
            bg      = state.config.bg,
            fg      = state.config.fg,
            err     = isCurrentFrameError();

        if (err)
        {
            fg = state.config.fgErr;
            bg = state.config.bgErr;
        }

        gltext.size(state.config.infoTextSize);
        setupProjection(domain.x, domain.y, event.domainVerticalFlip());

        let margin  = state.config.bevelMargin,
            x       = _x + margin,
            y       = _y + margin,
            tbox    = drawNameValuePairs(expandNameValuePairs(details),
                                         fg, bg, x, y, margin)._0,
            emin    = vec2f(_x, _y),
            emax    = emin + tbox + vec2f(margin*2.0, 0.0);

        if (_inCloseArea)
        {
            drawCloseButton(x - margin/2,
                            tbox.y + y - margin - margin/4,
                            margin/2, bg, fg);
        }

        updateBounds(emin, emax);
    }
}

\: initHUDObjects (void;)
{
    State s = data();
    s.imageInfo   = HUD.ImageInfo("imageinfo");
    s.infoStrip   = HUD.InfoStrip("infoStrip");
    s.processInfo = HUD.ProcessInfo("processinfo");
    s.sourceDetails = HUD.SourceDetails("sourcedetails");
}

} // HUD
