//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Deprecated commands reimplemented for compatibility sake
//  

module: commands
{
    use commands;
    RenderedSourceInfo := RenderedImageInfo;
    PixelSourceInfo := PixelImageInfo;

    global regex sourceMatchRE = "source";
    global regex switchMatchRE = "switch";

    \: sourceGeometry ((vector float[2])[]; string name)
    {
        imageGeometry(name);
    }

    \: sourceMediaInfoList ([SourceMediaInfo]; string nodeName)
    {
        let media = getStringProperty(nodeName + ".media.movie");
        [SourceMediaInfo] list = nil;
        for_each (p; media) list = sourceMediaInfo(nodeName, p) : list;
        return list;
    }

    \: sourceNameWithoutFrame (string; string name)
    {
        let s = name.split("/"),
            last = s.size() - 1;
        string r;
        
        for_index (i; s) 
        {
            if (i != last) 
            {
                if (i != 0) r += "/";
                r += s[i];
            }
        }
        
        return r;
    }

    \: sourcesRendered (RenderedSourceInfo[];)
    {
        RenderedImageInfo[] array;

        for_each (i; renderedImages())
        {
            if (sourceMatchRE.match(i.name) && !switchMatchRE.match(i.name)) array.push_back(i);
        }

        array;
    }

    \: sourceAtPixel (PixelSourceInfo[]; vector float[2] p)
    {
        imagesAtPixel(p, nil, true);
    }

    \: openFileDialog (string[]; 
                       bool associated,
                       bool multiple,
                       bool directory,
                       [(string, string)] filters,
                       string defaultPath = nil)
    {
        use io;
        osstream str;
        print(str, "%s|%s" % head(filters));
        
        for_each (filter; tail(filters))
        {
            print(str, "|%s|%s" % filter);
        }
        
        return openFileDialog(associated, multiple, directory, string(str), defaultPath);
    }
}

module: extra_commands {
use commands;
use rvtypes;
use glyph;
use gl;
use glu;
use io;
use mode_manager;
require system;

\: displayFeedback (void;
                    string text,
                    float duration = 2.0,
                    Glyph g = nil)
{
    State state         = data();
    state.feedbackText  = text;
    state.feedback      = duration;
    state.feedbackGlyph = g;
    startTimer();
    redraw();
}

\: displayFeedback2 (void;
                     string text,
                     float duration)
{
    displayFeedback(text, duration);
}

\: isSessionEmpty (bool;)
{
    sources().empty();
}

\: isNarrowed (bool;)
{
    frameStart() != narrowedFrameStart() || frameEnd() != narrowedFrameEnd();
}

\: setActiveState (void;)
{
    State state = data();

    if (!state.userActive)
    {
        state.userActive = true;
        setCursor(CursorDefault);
    }
}

\: setInactiveState (void;)
{
    State state = data();

    if (isPlaying() && state.pointerInSession) 
    {
        if (state.userActive)
        {
            state.userActive = false;
            setCursor(CursorNone);
        }
    }
}

\: recordPixelInfo (void; Event event)
{
    //
    //  We are allocating a bunch of structures here that we need to keep, so
    //  make sure that the current allocator API is not the arena allocator.
    //
    exception excOrig = nil;
    runtime.gc.push_api(0);
    try
    {
        State state = data();
        Point p = if (event neq nil) then event.pointer() else state.pointerPosition;
        setActiveState();
        let pi = imagesAtPixel(p, nil, true);

        if (pi neq nil && state.pixelInfo neq nil)
        {
            if (pi.size() != state.pixelInfo.size())
            {
                redraw();
            }
            else
            {
                for_index (i; pi)
                {
                    if (pi[i].name != state.pixelInfo[i].name)
                    {
                        redraw();
                        break;
                    }
                }
            }
        }
        else if (pi neq nil || state.pixelInfo neq nil)
        {
            redraw();
        }

        state.pixelInfo = pi;
        state.currentInput = inputAtPixel(p, false);
        state.pointerPosition = p;

        if (pi neq nil && !pi.empty())
        {
            let sourceNameNoFrame = sourceNameWithoutFrame(pi[0].name),
                sourceNameParts = sourceNameNoFrame.split("/"),
                sourceName = if (sourceNameParts.size() != 0) then sourceNameParts[0] else "";

            for_each (info; pi)
            {
                if (nodeType(nodeGroup(info.node)) == "RVSourceGroup")
                {
                    sourceNameNoFrame = sourceNameWithoutFrame(info.name);
                    sourceNameParts = sourceNameNoFrame.split("/");
                    if (sourceNameParts.size() != 0)
                    {
                        sourceName = sourceNameParts.front();
                        break;
                    }
                }
            }

            if (sourceName != "")
            {
                let normalizedIP = eventToImageSpace(sourceName, p, true);
                state.pointerPositionNormalized = normalizedIP;
            }
        }
        state.perPixelFrame = frame();
        state.perPixelPosition = state.pointerPosition;
        state.perPixelViewHash = string.hash(viewNode());
        state.perPixelInfoValid = true;
    }
    catch(exception exc) {excOrig = exc;}
    catch(...) {;} 

    runtime.gc.pop_api();

    if (excOrig neq nil) throw(excOrig);
}

\: updatePixelInfo(void; Event event)
{
    State state = data();
    if (! state.perPixelInfoValid) recordPixelInfo(event);
}

\: isPlayable (bool;) { frameEnd() != frameStart(); }
\: isPlayingForwards (bool;) { isPlaying() && inc() > 0; }
\: isPlayingBackwards (bool;) { isPlaying() && inc() < 0; }

\: setScale (void; float s)
{
    setFloatProperty("#RVDispTransform2D.transform.scale", float[] {s,s});
    redraw();
}

\: scale (float;)
{
    getFloatProperty("#RVDispTransform2D.transform.scale").front();
}

\: setTranslation (void; Vec2 t)
{
    setFloatProperty("#RVDispTransform2D.transform.translate",
                     float[] {t.x, t.y});
    redraw();
}

\: translation (Vec2;)
{
    let t = getFloatProperty("#RVDispTransform2D.transform.translate");
    Vec2(t[0], t[1]);
}

\: toggleFullScreen (void;)
{
    fullScreenMode(!isFullScreen());
}

\: togglePlayIfNoScrub (void;)
{
    State state = data();
   
    if (!state.scrubbed) togglePlay();
    state.scrubbed = false;
    state.playingBefore = false;
}

\: togglePlay (void; )
{
    togglePlayVerbose (true);
}

\: togglePlayVerbose (void; bool verbose)
{
    if (isPlayable())
    {
        State state = data();
        let rate = fps();

        scrubAudio(false);

        if (isPlaying() || isBuffering())
        {
            if (verbose) displayFeedback("STOP", 2, squareGlyph);
            stop();
        }
        else if (rate <= 0)
        {
            displayFeedback("WARNING: Can't play FPS = %g" % rate,
                            2.0,
                            coloredGlyph(pauseGlyph, Color(1, .5, 0, 1.0)));
        }
        else
        {
            string t;
            if (isRealtime()) t = "PLAY";
            else t = "PLAY (ALL FRAMES)";
            play();
            let g = if isPlayingForwards() then xformedGlyph(triangleGlyph, 180) else triangleGlyph;
            if (verbose) displayFeedback(t, 2, g);
        }
    }
}

\: toggleForwardsBackwards (void;)
{
    if (filterLiveReviewEvents()) {
        sendInternalEvent("live-review-blocked-event");
        return;
    }
    setInc(-inc());
    redraw();
}

\: toggleRealtime (void;)
{
    if (filterLiveReviewEvents()) {
        sendInternalEvent("live-review-blocked-event");
        return;
    }
    State state = data();

    if (isRealtime())
    {
        setRealtime(false);
        displayFeedback("PLAY ALL FRAMES");
    }
    else
    {
        setRealtime(true);
        displayFeedback("REALTIME");
    }
}

\: currentImageAspect (float;)
{
    let g = getCurrentImageSize();
    g.x / g.y;
}

\: sourceImageStructure ((int, int, int, int, bool, int); string n, string m = nil)
{
    //
    //  This is back-compatibility function: it now returns the uncrop
    //  w and h instead of possibly croped w, h
    //

    let info = sourceMediaInfo(n, m);

    if (info neq nil)
    {
        return (info.uncropWidth,
                info.uncropHeight,
                info.bitsPerChannel,
                info.channels,
                info.isFloat,
                info.planes);
    }
    else
    {
        return nil;
    }
}

\: frameImage (void;)
{
    setTranslation(Point(0,0));
    setScale(1.0);
}

\: reloadInOut (void;)
{
    reload(inPoint(), outPoint());
}

\: centerResizeFit (void;)
{
    frameImage();
    //resizeFit();
    center();
}

\: toggleFilter (void;)
{
    State state = data();
    setFiltering(if getFiltering() == GL_NEAREST then GL_LINEAR else GL_NEAREST);

    if (getFiltering() == GL_NEAREST)
    {
        displayFeedback("Nearest Neighbor Filter");
    }
    else
    {
        displayFeedback("Linear Filter");
    }

    redraw();
}


\: numFrames (int;) { frameEnd() - frameStart() + 1; }

\: stepForward (void; int n)
{
    if (filterLiveReviewEvents()) {
        sendInternalEvent("live-review-blocked-event");
        return;
    }
    let f = frame(),
        newFrame = frame() + n,
        inInOut = (inPoint() <= f && outPoint() >= f),
        upper =  if (inInOut) then outPoint() else frameEnd(),
        lower = if (inInOut) then inPoint() else frameStart();

    State s = data();
    if (s.scrubAudio) scrubAudio(true, 1.0 / fps(), 1);

    if ((upper == frameEnd() || s.stepWraps) && newFrame > upper) 
    {
        newFrame = lower + (newFrame - upper) - 1;
    }

    stop();
    setFrame(newFrame);
    redraw();
}

\: stepBackward (void; int n)
{
    if (filterLiveReviewEvents()) {
        sendInternalEvent("live-review-blocked-event");
        return;
    }
    let f = frame(),
        newFrame = frame() - n,
        inInOut = (inPoint() <= f && outPoint() >= f),
        upper =  if (inInOut) then outPoint() else frameEnd(),
        lower = if (inInOut) then inPoint() else frameStart();

    State s = data();

    if ((lower == frameStart() || s.stepWraps) && newFrame < lower) 
    {
        newFrame = upper - (lower - newFrame) + 1;
    }

    stop();
    setFrame(newFrame);
    if (s.scrubAudio) scrubAudio(true, 1.0 / fps(), 1);
    redraw();
}

\: stepForward1 (void;) { stepForward(1); }
\: stepBackward1 (void;) { stepBackward(1); }
\: stepForward10 (void;) { stepForward(10); }
\: stepBackward10 (void;) { stepBackward(10); }
\: stepForward100 (void;) { stepForward(100); }
\: stepBackward100 (void;) { stepBackward(100); }

\: cacheUsage ((float,float,int[]);)
{
    let (capacity, usage, lusage, seconds, secondsNeeded, audioSeconds, ranges) = cacheInfo();

//    return (if cacheMode() == CacheBuffer 
//               then float(lusage) / float(capacity)
//               else float(usage) / float(capacity),
//            audioSeconds);

    return (float(usage) / float(capacity), audioSeconds, ranges);
}

\: toggleMotionScopeFromState (void; State state)
{
    if (state.motionScope eq nil)
    {
        //
        //  moscope calls functions on timeline, so if we don't have a
        //  timeline, build it here.
        //
        if (state.timeline eq nil) 
        {
            runtime.load_module("timeline");
            let Fname = runtime.intern_name("timeline.init");
            (Widget;) T = runtime.lookup_function(Fname);
            state.timeline = T();
        }
        runtime.load_module("motion_scope");
        let name = runtime.intern_name("motion_scope.startUp");
        (void; State) M = runtime.lookup_function(name);
        M(state);
    }
    else
    {
        state.motionScope.toggle();
    }
}

\: toggleMotionScope (void; )
{
    State state = data();

    toggleMotionScopeFromState (state);
}

\: minorModeIsLoaded (bool; string nm)
{
    State state = data();

    for_each (m; state.minorModes) if (m._modeName == nm) return true;

    return false;
}

\: toggleSync (void; )
{
    State state = data();

    if (state.sync eq nil)
    {
        // Is the RV Sync module loaded ?
        let rvSyncIsLoaded = minorModeIsLoaded("sync-mode");
        if (rvSyncIsLoaded)
        {
            runtime.load_module("sync");
            let name = runtime.intern_name("sync.startUp");
            (void;) F = runtime.lookup_function(name);
            F();
        }
        else
        {
            print("WARNING: RV Sync module is not currently loaded.\n" + 
                  "Please make sure the RV Sync Package is enabled in the RV Preferences/Packages\n");
        }
    }
    else
    {
        state.sync.toggle();
    }
}

\: activateSync (void; )
{
    State state = data();

    if (state.sync eq nil || !state.sync._active)
    {
        //  Check to see that we have at least one connection to an
        //  RV process
        let startSync = false;
        for_each (app; remoteApplications()) if (app == "rv") startSync = true;

        if (startSync) toggleSync();
    }
}

\: activatePackageModeEntry (Mode; string modeModuleName)
{
    State state = data();
    ModeManagerMode m = state.minorModes.front();
    return m.activateMode(modeModuleName, true);
}

\: deactivatePackageModeEntry (Mode; string modeModuleName)
{
    State state = data();
    ModeManagerMode m = state.minorModes.front();
    return m.activateMode(modeModuleName, false);
}

//  per-type settings commands.
/*
      case SettingsValueType::FloatType: return QVariant::Double;
      case SettingsValueType::IntType: return QVariant::Int;
      case SettingsValueType::StringType: return QVariant::String;
      case SettingsValueType::BoolType: return QVariant::Bool;
      case SettingsValueType::FloatArrayType: return QVariant::List;// same as in
      case SettingsValueType::IntArrayType: return QVariant::List;  // same as float
      case SettingsValueType::StringArrayType: return QVariant::StringList;
*/


\: cprop (void; string name, int propType)
{
    if (!propertyExists(name)) newProperty(name, propType, 1);
}

\: set (void; string name, float[] value) 
{ 
    cprop(name, FloatType); 
    setFloatProperty(name, value, true); 
}

\: set (void; string name, int[] value) 
{ 
    cprop(name, IntType); 
    setIntProperty(name, value, true); 
}

\: set (void; string name, string[] value) 
{ 
    cprop(name, StringType); 
    setStringProperty(name, value, true); 
}

\: set (void; string name, float value) 
{ 
    cprop(name, FloatType); 
    setFloatProperty(name, float[]{value}, true); 
}

\: set (void; string name, int value) 
{ 
    cprop(name, IntType); 
    setIntProperty(name, int[]{value}, true); 
}

\: set (void; string name, string value) 
{ 
    cprop(name, StringType); 
    setStringProperty(name, string[]{value}, true); 
}

\: appendToProp (void; string name, string value)
{
    cprop(name, StringType);
    let val = getStringProperty(name);
    val.push_back(value);
    set(name, val);
}

\: removeFromProp (void; string name, string value)
{
    cprop(name, StringType);
    let val = getStringProperty(name);
    string[] newVal;
    for_each (x; val) if (x != value) newVal.push_back(x);
    set(name, newVal);
}

\: existsInProp (bool; string name, string value)
{
    let val = getStringProperty(name);
    for_each (x; val) if (x == value) return true;
    return false;
}

\: sourceMetaInfoAtFrame (MetaEvalInfo; int atframe, string root = nil)
{
    for_each (info; metaEvaluate(atframe, root))
    {
        if (info.nodeType == "RVFileSource" || info.nodeType == "RVImageSource")
        {
            return info;
        }
    }

    return nil;
}

\: sourceFrame (int; int atframe, string root = nil)
{
    let info = sourceMetaInfoAtFrame(atframe, root);
    return if info eq nil then atframe else info.frame;
}

documentation: """
Find all nodes of type typeName associated with the given
node. associatedNodes walks down the DAG starting at nodeName (often a
leaf) and makes an array of all nodes of type typeName.
""";

\: associatedNodes (string[]; string typeName, string nodeName)
{
    \: recursiveAssociatedNode (void; string typeName, string nodeName, string[] array)
    {
        if (typeName == nodeType(nodeName)) array.push_back(nodeName);
        let outnodes = nodeConnections(nodeName, true)._1;
        //print("outnodes = %s\n" % outnodes);
        for_each (n; outnodes) recursiveAssociatedNode(typeName, n, array);
    }

    let t = nodeType(nodeName);
    string[] array;
    recursiveAssociatedNode(if typeName[0] == '#' 
                                then typeName.substr(1,0) 
                                else typeName,
                            nodeName,
                            array);

    return array;
}


documentation: """
Find the node of type typeName which is associated with the given node
nearest the given node. Normally the given node will be a source node
and the type will something like RVColor. associatedNode walks down
the DAG starting at nodeName until it finds a node that matches the
input type. It will search all roots.

This replaces all the similarily named functions in packages and
elsewhere called associatedNode which was merely fabricating the node
name based on the input name.
""";

\: associatedNode (string; string typeName, string nodeName)
{
    let array = associatedNodes(typeName, nodeName);
    return if array.empty() then "" else array.front();
}

documentation: """
Find the nodes of type typeName which are in the evaluation path
between the root and the given node. 

If typeName is nil (default) then all the nodes are returned.

If nodeName is nil (default) then all leaf paths will be searched.
""";

\: nodesInEvalPath (string[]; int frame, string typeName = "", string nodeName = nil)
{
    string[] nodes;
    bool rvsource = typeName == "RVSource";

    for_each (info; metaEvaluate(frame, viewNode(), nodeName))
    {
        let {name, nodeType, eframe} = info;
        if (typeName == "" || 
            nodeType == typeName ||
            (rvsource && 
             (nodeType == "RVFileSource" || 
              nodeType == "RVImageSource"))) 
        {
            nodes.push_back(name);
        }
    }

    nodes;
}

\: nodesUnderPointer (string[][]; Event event, string typeName = "")
{
    string[][] paths;

    State state = data();
    let domain  = event.domain(),
        p       = state.pointerPosition;

    for_each (i; state.pixelInfo)
    {
        let name = i.name.split(".").front();
        paths.push_back(nodesInEvalPath(frame(), typeName, name));
    }

    paths;
}

\: topLevelGroup (string; string innode)
{
    let group = nodeGroup(innode);

    if (group neq nil)
    {
        while (nodeGroup(group) neq nil) group = nodeGroup(group);
    }

    return group;
}

documentation: """
Return list of node names in the given group, with type equal to the given type.
""";

\: nodesInGroupOfType (string[]; string groupNode, string typeName)
{
    string[] nodes;

    for_each (n; nodesInGroup(groupNode))
    {
        let t = nodeType(n);

        if (typeName == "#RVSource" || typeName == "RVSource")
        {
            if (t == "RVFileSource" || t == "RVImageSource") nodes.push_back(n);
        }
        else if (t == typeName) nodes.push_back(n);
    }

    return nodes;
}

\: uiName (string; string innode)
{
    let group = topLevelGroup(innode),
        node  = if group eq nil then innode else group,
        name  = "%s.ui.name" % node;

    return if propertyExists(name) then getStringProperty(name).front() else node;
}


\: isViewNode (bool; string name)
{
    for_each (n; viewNodes()) if (n == name) return true;
    return false;
}


documentation: """
Set the user interface name of a node. May modify the name to make it unqiue.
""";

\: setUIName (void; string node, string val)
{
    string newName = val;
    bool dupcheck = true;

    while (dupcheck)
    {
        dupcheck = false;

        for_each (n; viewNodes())
        {
            if (uiName(n) == newName && n != node)
            {
                let re = regex("(.*)([0-9]+)$");
                
                if (re.match(newName))
                {
                    let m = re.smatch(newName);
                    newName = "%s%d" % (m[1], int(m[2]) + 1);
                }
                else
                {
                    newName += "2";
                }

                dupcheck = true;
                break;
            }
        }
    };

    let name = "%s.ui.name" % node;
    if (!propertyExists(name)) newProperty(name, StringType, 1);
    set(name, newName);
}

documentation:  "Cycle inputs on a node";

\: cycleNodeInputs (void; string node, bool forward)
{
    let inputs = nodeConnections(node, false)._0,
        s = inputs.size();

    if (!inputs.empty())
    {
        string[] newInputs;
        newInputs.resize(s);

        for_index (i; inputs)
        {
            let inc = if forward then 1 else s - 1;
            newInputs[(i + inc) % s] = inputs[i];
        }

        setNodeInputs(node, newInputs);
    }
}

documentation: """
Reorder inputs of a node so that index is the first input. The ordering is stable.
""";

\: popInputToTop (void; string node, int index)
{
    let (inputs, _) = nodeConnections(node, false),
        s = inputs.size();

    string[] newInputs;
    newInputs.resize(s);

    for_index (i; inputs)
    {
        newInputs[i] = if i == 0 
                          then inputs[index]
                          else if i <= index 
                               then inputs[i-1]
                               else inputs[i];
    }

    setNodeInputs(node, newInputs);
}

\: popInputToTop (void; string node, string input)
{
    let (inputs, _) = nodeConnections(node, false);

    for_index (i; inputs)
    {
        if (inputs[i] == input) 
        {
            popInputToTop(node, i);
        }
    }
}

documentation: """
Locate the input in the eval path at frame starting at node and return its ui name.
""";

\: inputNodeUserNameAtFrame (string; int frame, string node)
{
    let (ins, _) = nodeConnections(node, false),
        nodes = nodesInEvalPath(frame);

    osstream buffer;
    int count = 0;
    
    for_each (n; nodes) 
    {
        for_each (i; ins) 
        {
            if (i == n) 
            {
                if (count < 3)
                {
                    if (count > 0) print(buffer, " / ");
                    print(buffer, uiName(i));
                }

                count++;
            }
        }
    }

    if (count <= 3) 
    {
        return string(buffer);
    }
    else if (count > 3)
    {
        return "%d things" % count;
    }
    else
    {
        // fall back
        let sourceNames = sourcesAtFrame(frame),
            media       = if sourceNames.empty() 
                             then "No Media" 
                             else sourceMedia(sourceNames.front())._0;
    
        return media.split("/").back();
    }
}

\: inputNodeUserNameAtFrame (string; int frame)
{
    inputNodeUserNameAtFrame(frame, viewNode());
}

\: sequenceBoundaries (int[]; string node = nil)
{
    if (node eq nil) node = viewNode();
    mapPropertyToGlobalFrames("edl.frame", 1, node);
}

documentation: """
Returns an array of all annotated frames relative to the node passed to the function.
If there is no node, the view node is used instead. The array is not sorted and some
frames may appear more than once.
""";

\: findAnnotatedFrames (int[]; string node = nil)
{
    string[] tempProps;
    let seqb = sequenceBoundaries();
    let testFrames = if seqb.empty() then int[](frameStart()) else seqb;
    if (node eq nil) node = viewNode();

    for_each (f; testFrames)
    {
        for_each (info; metaEvaluate(f, node))
        {
            let {name, nodeType, eframe} = info;

            if (nodeType == "RVPaint")
            {
                let tprop = "%s.find.frames" % name;

                if (!propertyExists(tprop))
                {
                    int[] nodeFrames;

                    for_each (p; properties(name))
                    {
                        let parts = p.split("."),
                            pname = parts[-1],
                            pcomp = parts[-2],
                            cparts = pcomp.split(":");

                        if (cparts[0] == "frame" && 
                            pname == "order" && 
                            cparts.size() == 2 &&
                            propertyInfo(p).size > 0)
                        {
                            nodeFrames.push_back(int(cparts.back()));
                        }
                    }
                
                    set(tprop, nodeFrames);
                    tempProps.push_back(tprop);
                }
            }
        }
    }

    let frames = mapPropertyToGlobalFrames("find.frames", 1);
    for_each (p; tempProps) if (propertyExists(p)) deleteProperty(p);
    return frames;
}

\: _print (void; string s) { print(s); }

\: associatedVideoDevice (string; string displayGroup)
{
    if (nodeType(displayGroup) != "RVDisplayGroup") return "";
    return getStringProperty("%s.device.name" % displayGroup).front();
}

\: setDisplayProfilesFromSettings (string[];)
{
    string[] nodes;

    for_each (dnode; nodesOfType("RVDisplayGroup"))
    {
        let nameProp = "%s.device.name" % dnode;

        if (propertyExists(nameProp))
        {
            use SettingsValue;

            let device   = getStringProperty(nameProp)[0],
                module   = getStringProperty("%s.device.moduleName" % dnode)[0];

            for_each (t; [VideoAndDataFormatID, DeviceNameID, ModuleNameID])
            {
                let str      = videoDeviceIDString(module, device, t),
                    String s = readSetting(str, "DisplayProfile", String(""));

                if (s != "")
                {
                    try
                    {
                        let pipe = nodesInGroupOfType(dnode, "RVDisplayPipelineGroup").front();
                        readProfile(s, pipe, true, "display");
                        nodes.push_back(dnode);
                        print("INFO: display profile %s assigned to %s\n" % (s, device));
                        break;
                    }
                    catch (...)
                    {
                        print("WARNING: display profile %s not found for %s/%s\n" % (s, module, device));
                    }
                }
            }
        }
    }
    redraw();

    return nodes;
}

\: loadCurrentSourcesChangedFrames(void; )
{
    let metaInfo = metaEvaluateClosestByType (frame(), "RVFileSource"),
        sourceNames = string[]();

    for_each (mi; metaInfo) sourceNames.push_back(mi.node);

    if (! sourceNames.empty()) loadChangedFrames(sourceNames);
}

}

//
// Since contentAspect makes use of methods in both commands and extra_commands
// we have to add it last.
//

module: commands
{
    use commands;
    use extra_commands;
    use rvtypes;

    \: contentAspect (float;)
    {
        updatePixelInfo(nil);
        State state = data();
        if (state.pixelInfo neq nil && !state.pixelInfo.empty())
        {
            PixelImageInfo info = state.pixelInfo.front();

            for_each (ri; renderedImages())
            {
                if (ri.name == info.name)
                {
                    return ri.pixelAspect * float(ri.width) / float(ri.height);
                }
            }
        }
        return 0.0;
    }
}
