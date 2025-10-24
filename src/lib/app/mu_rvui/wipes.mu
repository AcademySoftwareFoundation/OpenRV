//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  The widget that controls the wipes
//

module: wipes {
use rvtypes;
use glyph;
use app_utils;
use math;
use math_util;
use commands;
use extra_commands;
use gl;
use glu;
use qt;
require io;

class: Wipe : MinorMode
{

    class: EditNodePair 
    { 
        string tformNode;
        string inputNode; 
    }

    ManipPoint := (PixelSourceInfo,Point,EditNodePair);

    bool            _editing;
    bool            _cameraChange;
    bool            _centerEdit;
    bool            _cornerEdit;
    bool            _drawManip;
    bool            _didDrag;
    bool            _showInfo;
    int             _nearEdge;
    int             _nearCorner;
    string          _sourceName;
    Vec2            _sourcePoint;
    Vec2            _dir;
    Vec2            _grabPoint;
    Vec2            _grabPointNorm;
    ManipPoint      _manipPoint;
    Point[]         _corners;
    Point           _downPoint;
    EditNodePair[]  _editNodes;
    EditNodePair    _currentEditNode;

    method: clear (void;)
    {
        _manipPoint      = nil;
        _currentEditNode = nil;
        _cameraChange    = false;
        _centerEdit      = false;
        _cornerEdit      = false;
        _didDrag         = false;
        _nearEdge        = -1;
        _nearCorner      = -1;
        _sourceName      = "";
        _manipPoint      = nil;
        _corners         = nil;
        _editNodes       = nil;
    }

    \: closestPointOnLine (Point; Point p, Point a, Point b)
    {
        let dir  = normalize(b - a),
            u    = dot(p - a, dir);

        u * dir + a;
    }

    \: tagValue (string; (string,string)[] tags, string name)
    {
        for_each (t; tags)
        {
            let (n, v) = t;
            if (n == name) return v;
        }

        return nil;
    }

    method: isOccluded (bool; string manipTFormNode, Point p)
    {
        //
        //  Check for occlusion of the _manipPoint. If so start over
        //

        PixelSourceInfo[] infos = imagesAtPixel(p, "wipe");

        for_each (i; infos)
        {
            EditNodePair enode = editNode(tagValue(i.tags, "wipe"));
            if (manipTFormNode == enode.tformNode) break;

            if (i.inside && manipTFormNode != enode.tformNode)
            {
                return true;
            }
        }

        return false;
    }

    method: editNode (EditNodePair; string name)
    {
        for_each (enode; _editNodes)
            if (enode.tformNode == name) return enode;
        return nil;
    }

    method: move (void; Event event)
    {
        event.reject();

        if (sourcesRendered().empty()) return;

        _cameraChange = false;
        _currentEditNode = nil;

        recordPixelInfo(event);

        State state = data();
        let domain  = event.domain(),
            p       = state.pointerPosition,
            devicePixelRatio = devicePixelRatio();

        _cornerEdit = false;
        _drawManip = false;

        PixelSourceInfo info = nil;
        PixelSourceInfo[] infos = imagesAtPixel(event.pointer(), "wipe");
        (PixelSourceInfo,Point)[] centers;
        string tagValue = nil;

        for_each (i; infos)
        {
            let enode = editNode(tagValue(i.tags, "wipe")),
                tformNode = enode.tformNode;

            if (info eq nil) 
            {
                if (isOccluded(tformNode, Point(i.x, i.y))) continue;
                info = i;
            }
            else if (i.edge < info.edge)
            {
                if (isOccluded(tformNode, Point(i.x, i.y))) continue;
                info = i;
            }

            let param   = "%s.stencil.visibleBox" % tformNode,
                vals    = getFloatProperty(param),
                corners = imageGeometryByTag(i.tags[0]._0, i.tags[0]._1);

            for_index (c; corners) corners[c] /= devicePixelRatio;

            if (i.inside)
            {
                if (vals[0] > 0.0 || vals[1] < 1.0 ||
                    vals[2] > 0.0 || vals[3] < 1.0 ||
                    _manipPoint eq nil)
                {
                    Point gc;
                    for_each (c; corners) gc += c;
                    gc /= float(corners.size());

                    if (!isOccluded(tformNode, gc) &&
                        mag(gc - p) < state.config.wipeGrabProximity)
                    {
                        _drawManip = true;
                        _manipPoint = (i, gc, enode);
                    }
                }
            }

            for_index (c; corners)
            {
                if (!isOccluded(tformNode, corners[c]) &&
                    mag(corners[c] - p) < state.config.wipeGrabProximity)
                {
                    _drawManip  = true;
                    _manipPoint = (i, corners[c], enode);
                    _cornerEdit = true;
                    _nearCorner = c;
                }
            }

            _currentEditNode = enode;
        }

        if (_drawManip) 
        {
            info             = _manipPoint._0;
            _sourceName      = sourceNameWithoutFrame(info.name);
            _sourcePoint     = state.pointerPosition;
            _nearEdge        = 0;
            _grabPoint       = _manipPoint._1*devicePixelRatio;
            _grabPointNorm   = eventToImageSpace(_sourceName, _grabPoint, true);
            _centerEdit      = !_cornerEdit;
            _currentEditNode = _manipPoint._2;
        }
        else if (info neq nil)
        {
            _sourceName    = sourceNameWithoutFrame(info.name);
            _grabPoint     = Point(info.x*devicePixelRatio, info.y*devicePixelRatio);
	    _grabPointNorm = eventToImageSpace(_sourceName, _grabPoint, true);
            _manipPoint    = (info, _grabPoint, editNode(tagValue(info.tags, "wipe")));
            _sourcePoint   = state.pointerPosition;
            _nearEdge      = -1;
            _centerEdit    = false;

            let corners = imageGeometryByTag(info.tags[0]._0, info.tags[0]._1);

            if (corners.empty())
            {
                print("EMPTY\n");
            }

            for_index (i; corners)
            {
                let a    = corners[i],
                    b    = corners[(i + 1) % corners.size()],
                    dist = mag(closestPointOnLine(_grabPoint, a, b) - _grabPoint);
                        
                if (dist < .5) { _nearEdge = i; break; }
            }
                    
            // this happens when the view changes for some reason
            //assert(_nearEdge != -1);
            if (_nearEdge == -1) print("wipes: _nearEdge == -1\n");

        }

        redraw();
    }

    method: push (void; Event event)
    {
        _downPoint = event.pointer();
        _editing = true;
        _didDrag = false;
        startTimer();
        redraw();
    }

    method: drag (void; Event event)
    {
        if (_corners eq nil || _manipPoint eq nil) 
        {
            print("returning early\n");
            return;
        }

        State state = data();
        _didDrag = true;
        recordPixelInfo(event);

        if (_nearEdge != -1) 
        {
            let tformNode = _manipPoint._2.tformNode,
                param = "%s.stencil.visibleBox" % tformNode,
                vals  = getFloatProperty(param),
                xs    = vals[1] - vals[0],
                ys    = vals[3] - vals[2],
                pp    = event.pointer(),
                dp    = _downPoint,
                ip    = pp - dp,
                devicePixelRatio = devicePixelRatio(),
                a     = _corners[0]/devicePixelRatio,
                b     = _corners[1]/devicePixelRatio,
                c     = _corners[2]/devicePixelRatio,
                d     = _corners[3]/devicePixelRatio,
                dx    = dot(ip, normalize(b - a)) * xs / mag(b - a),
                dy    = dot(ip, normalize(d - a)) * ys / mag(d - a),
                v0    = vals[0] + dx,
                v1    = vals[1] + dx,
                v2    = vals[2] + dy,
                v3    = vals[3] + dy;

            if (_centerEdit)
            {
                if (v0 >= 0.0 && v1 <= 1.0)
                {
                    vals[0] = v0;
                    vals[1] = v1;
                }

                if (v2 >= 0.0 && v3 <= 1.0)
                {
                    vals[2] = v2;
                    vals[3] = v3;
                }
            }
            else if (_cornerEdit)
            {
                case (_nearCorner)
                {
                    0 -> { vals[0] = v0; vals[2] = v2; }
                    1 -> { vals[1] = v1; vals[2] = v2; }
                    2 -> { vals[1] = v1; vals[3] = v3; }
                    3 -> { vals[0] = v0; vals[3] = v3; }
                }
            }
            else
            {
                case (_nearEdge)
                {
                    0 -> { vals[2] = v2; }
                    1 -> { vals[1] = v1; }
                    2 -> { vals[3] = v3; }
                    3 -> { vals[0] = v0; }
                }
            }

            if (vals[0] > vals[1]) vals[1] = vals[0] + .001;
            if (vals[2] > vals[3]) vals[3] = vals[2] + .001;
            if (vals[0] < 0) vals[0] = 0;
            if (vals[0] > 1) vals[0] = 0.99;

            if (vals[1] > 1) vals[1] = 1;
            if (vals[1] < 0) vals[1] = 0.01;

            if (vals[2] < 0) vals[2] = 0;
            if (vals[2] > 1) vals[2] = 0.99;

            if (vals[3] > 1) vals[3] = 1;
            if (vals[3] < 0) vals[3] = 0.01;

            setFloatProperty(param, vals);
            recordPixelInfo(event);
            _downPoint = pp;
        }

        redraw();
    }

    method: release (void; Event event)
    {
        _editing = false;
        _currentEditNode = nil;
        recordPixelInfo(event);
        move(event);
        redraw();

        if (!_didDrag && _drawManip)
        {
            _showInfo = !_showInfo;
        }
    }

    method: toggleLayerInfo (void; Event event)
    {
        _showInfo = !_showInfo;
        writeSetting ("Wipes", "showInfo", SettingsValue.Bool(_showInfo));
        redraw();
    }

    method: resetAll (void; Event event)
    {
        let nodes = nodesInEvalPath(frame(), "RVTransform2D");

        for_each (node; nodes)
        {
	    let param = "%s.stencil.visibleBox" % node;

            setFloatProperty(param, float[] {0, 1, 0, 1});
        }

        redraw();
    }

    method: quitWipes (void; Event event)
    {
        toggle();
        resetAll(nil);
        set(viewNode() + ".ui.wipes", 0);
    }

    method: quitWipesNoReset (void; Event event)
    {
        toggle();
        set(viewNode() + ".ui.wipes", 0);
    }

    method: removeTags (void;)
    {
        for_each (x; _editNodes)
        {
            let node = x.tformNode,
                pmanip = node + ".tag.wipe",
                pstate = node + ".tag.wipe_name";

            for_each (p; [pmanip, pstate])
            {
                if (propertyExists(p)) deleteProperty(p);
            }
        }
    }

    method: findEditingNodes (void; bool setStates=true)
    {
        clear();

        let vnode       = viewNode(),
            infos       = metaEvaluateClosestByType(frame(), "RVTransform2D"),
            (ins, outs) = nodeConnections(viewNode(), false);

        _editNodes = EditNodePair[]();

        // happens when shutting down or deletion
        if (infos.size() != ins.size()) return;

        for_index (i; infos)
        {
            let info  = infos[i],
                pname = info.node + ".tag.wipe",
                sname = info.node + ".tag.wipe_name";

            _editNodes.push_back(EditNodePair(info.node, ins[i]));

            if (setStates || !propertyExists(pname))
            {
                set(pname, info.node);
                set(sname, ins[i]);
            }
        }
    }

    method: nodeInputsChanged (void; Event event) 
    { 
        event.reject();
        let node = event.contents();

        // Don't set the node states in this case 
        if (viewNode() neq nil && node == viewNode()) findEditingNodes(false);
    }


    method: activate(void;) 
    { 
        try
        {
            findEditingNodes(); 
            set(viewNode() + ".ui.wipes", 1);
        }
        catch (...) {;}
    }

    method: deactivate (void;) { setCursor(Qt.ArrowCursor); removeTags(); }

    method: toggle (void; )
    {
        //  print ("********* wipe toggle active %s\n" % _active);
        MinorMode.toggle(this);
    }

    method: beforeGraphViewChange (void; Event event) 
    { 
        event.reject();
        removeTags(); 
        set(viewNode() + ".ui.wipes", 1);
    }

    method: afterGraphViewChange (void; Event event) 
    { 
        event.reject();
        if (nodeType(viewNode()) != "RVStackGroup")
        {
            toggle();
        }
        else
        {
            findEditingNodes(); 
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
        //  If display changes we need to turn off rendering 
        //


        if (nodeType(node) == "RVDispTransform2D" || nodeType(node) == "RVDisplayStereo")
        {
            _cameraChange = true;
        }

        event.reject();
    }

    method: Wipe (Wipe; string name)
    {
        clear();

        //
        //  Make a little thunk that captures this and returns the
        //  menu state. Right now, this is the easiest way to do this:
        //  the partial application syntax won't let you partially
        //  apply *all* of the arguments.
        //

        let layerState = \: (int;) 
            {
                if this._showInfo then CheckedMenuState else UncheckedMenuState;
            };

        init(name, 
             nil, // no global 
             [("pointer--move", move, "Search for Nearest Wipe"), 
              // bound by menuItem("Show Source List") // ("key-down--L", toggleLayerInfo, "Toggle Wipe Layer Info"),
              // bound by menuItem("Reset All Wipes") // ("key-down--R", resetAll, "Reset All Wipes"),
              ("pointer-1--push", push, "Move Wipe or Popup Wipe Layers"),
              ("pointer-1--drag", drag, "Move Wipe"),
              ("pointer-1--release", release, ""),
              ("graph-node-inputs-changed", nodeInputsChanged, "Update session UI"),
              ("after-graph-view-change", afterGraphViewChange, "Update UI"),
              ("before-graph-view-change", beforeGraphViewChange, "Update UI"),
              ("graph-state-change", propertyChanged,  "Maybe update session UI"),
              ("stylus-pen--move", move, "Search for Nearest Wipe"), 
              ("stylus-pen--push", push, "Move Wipe or Popup Wipe Layers"),
              ("stylus-pen--drag", drag, "Move Wipe"),
              ("stylus-pen--release", release, "")],
             newMenu(MenuItem[] {
                 subMenu("Wipes", MenuItem[] {
                         menuItem("Show Source List", "key-down--L", "info_category", toggleLayerInfo, layerState),
                         menuItem("Reset All Wipes", "key-down--R", "wipes_category", resetAll, enabledItem),
                         menuSeparator(),
                         menuItem("Quit Wipes (without resetting)", "", "system_category", quitWipesNoReset, enabledItem),
                         menuItem("Quit Wipes", "", "system_category", quitWipes, enabledItem)
                 })
             })
             ,
             //
             //  Wipes events must be processed last, since they
             //  cover the screen.
             //
             "zzz" );
        
        let SettingsValue.Bool b1 = readSetting ("Wipes", "showInfo", SettingsValue.Bool(false));
        _showInfo = b1;
    }
    
    method: render (void; Event event)
    {
        if (_manipPoint eq nil) 
        {
            return;
        }

        State state = data();

        if (_cameraChange || !state.pointerInSession) return;

        let domain  = event.domain(),
            devicePixelRatio = devicePixelRatio(),
            bg      = state.config.bg,
            fg      = state.config.fg,
            p       = state.pointerPosition*devicePixelRatio,
	    ms      = margins();

	if (p.x < ms[0] || p.x > domain.x - ms[1] || p.y > domain.y - ms[2] || p.y < ms[3]) return;

        // NOTE: don't need to test flip here because its handled by renderer
        setupProjection(domain.x, domain.y);
        RenderedImageInfo info = nil;

        try
        {
            let wipeTag = _manipPoint._0.tags[0];

            _corners = imageGeometryByTag(wipeTag._0, wipeTag._1);

            if (_corners eq nil) return;

	    let infos = imagesAtPixel(_sourcePoint),
	        name = sourceNameWithoutFrame(_manipPoint._0.name);

	    let ep = imageToEventSpace (name, _grabPointNorm, true);

            let d      = mag(_grabPoint - p),
                rotang = degrees(atan2(_corners[1].y - _corners[0].y, 
                                       _corners[1].x - _corners[0].x));

            glEnable(GL_BLEND);
            glEnable(GL_LINE_SMOOTH);
            glEnable(GL_POINT_SMOOTH);
            glLineWidth(2.0);

            if (_editing)
            {
                let t = elapsedTime(),
                    t0 = state.config.wipeFade;

                if (t > t0) glColor(Color(0));
                else glColor(fg * Color(1,1,1, t0 - t));
            }
            else
            {
                let prox = if _centerEdit then state.config.wipeGrabProximity 
                                          else state.config.wipeFadeProximity,
                   alpha = 1.0 - d / prox;

                glColor(fg * Color(1,1,1, if alpha < 0 then 0.0 else alpha));
            }
            
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glBegin(GL_LINE_LOOP);
            for_each (c; _corners) glVertex(c);
            glEnd();

            if (!_editing)
            {
                if (_drawManip)
                {
                    glPushMatrix();
                    glTranslate(ep.x, ep.y, 0.0);
                    glScale(25.0*devicePixelRatio, 25.0*devicePixelRatio, 25.0*devicePixelRatio);
                    glRotate(rotang, 0, 0, 1);
                    glColor(bg * Color(1,1,1,.5));
                    circleGlyph(false);
                    circleGlyph(true);
                    glPopMatrix();

                    glPushMatrix();
                    glTranslate(ep.x, ep.y, 0.0);
                    glScale(25.0*devicePixelRatio, 25.0*devicePixelRatio, 25.0*devicePixelRatio);
                    glRotate(rotang, 0, 0, 1);
                    glColor(fg);
                    translateIconGlyph(false);
                    glColor(fg * 0.5);
                    glLineWidth(1.0);
                    translateIconGlyph(true);
                    glPopMatrix();
                }
                else
                { 
                    glPushMatrix();
                    glTranslate(ep.x, ep.y, 0.0);
                    glScale(25.0*devicePixelRatio, 25.0*devicePixelRatio, 25.0*devicePixelRatio);
                    glRotate(rotang, 0, 0, 1);
                    glColor(bg * Color(1,1,1,.25));
                    circleGlyph(false);
                    circleGlyph(true);
                    glPopMatrix();

                    glPushMatrix();
                    glTranslate(ep.x, ep.y, 0.0);
                    glScale(25.0*devicePixelRatio, 25.0*devicePixelRatio, 25.0*devicePixelRatio);
                    glRotate(rotang, 0, 0, 1);
                    glColor(fg);
                    if ((_nearEdge & 1) == 0) translateXIconGlyph(false);
                        else translateYIconGlyph(false);
                    glColor(fg * 0.5);
                    glLineWidth(1.0);
                    if ((_nearEdge & 1) == 0) translateXIconGlyph(false);
                        else translateYIconGlyph(false);
                    glPopMatrix();
                }
                
                glColor(fg*0.5);
                glPointSize(12.0*devicePixelRatio);
                glBegin(GL_POINTS); glVertex(ep); glEnd();
                glColor(fg);
                glPointSize(8.0*devicePixelRatio);
                glBegin(GL_POINTS); glVertex(ep); glEnd();

                if (_showInfo)
                {
                    (string,string)[] pairs;
                    gltext.size(state.config.wipeInfoTextSize*devicePixelRatio);

                    \: reverse ((string,string)[]; (string,string)[] s)
                    {
                        (string,string)[] n;
                        for (int i=s.size()-1; i >= 0; i--) n.push_back(s[i]);
                        n;
                    }

                    int eye = 0, grab = 0;

                    let index = 0;

                    for_each (r; renderedImages())
                    {
                        let val = tagValue(r.tags, "wipe_name");

                        if (val neq nil)
                        {
                            pairs.push_back(("      ", uiName(val)));
                            if (val == _manipPoint._2.inputNode) grab = index;
                            else ++index;
                        }
                    }

                    grab = pairs.size() - 1 - grab;

                    let margin  = state.config.bevelMargin * .5,
                        viewMargins = margins();

                    let size = nameValuePairBounds(reverse(pairs), margin)._0;
                    let boxW = size.x + margin,
                        boxH = size.y;

                    let x       = p.x,
                        y       = p.y,
                        xOver   = x + 25.0 + boxW + viewMargins[1] - domain.x,
                        yOver   = y + 25.0 + boxH + viewMargins[2] - domain.y;

                    if (xOver > 0.0) x += max (25.0-xOver, -25.0-boxW);
                    else x += 25.0;
                    if (yOver > 0.0) y += max (25.0-yOver, -25.0-boxH);
                    else y += 25.0;

                    let (tbox, nbounds, vbounds, nw) = 
                        drawNameValuePairs(reverse(pairs), fg, bg,
                                           x, y,
                                           margin);

                    let fa = int(gltext.ascenderHeight()),
                        fd = int(gltext.descenderDepth()),
                        th = fa - fd,
                        gx = x + margin/2 + 1,
                        gy = y + th * grab + fd + th/2 + 0.0,
                        ex = x + margin/2 + 12.0,
                        ey = y + th * (state.pixelInfo.size() - 1) + fd + th/2 + 2.0;

                    glEnable(GL_POINT_SMOOTH);
                    glPointSize(6.0*devicePixelRatio);
                    glBegin(GL_POINTS);
                    glColor(Color(.75,.75,.15,1));
                    glVertex(gx, gy);
                    /*
                    Remove blue dot for now.
                    glColor(Color(.15,.75,.75,1));
                    glVertex(ex, ey);
                    */
                    glEnd();
                }
            }
            else
            {
                redraw();
            }
            
            glDisable(GL_BLEND);
        }
        catch (exception exc)
        {
            //if (isPlaying()) { print("%s\n" % exc); }
            _editing = false;
            print("ERROR: wipes caught %s\n" % exc);
            string[] trace = exc.backtrace();
            
            for (int i=0; i < trace.size(); i++)
            {
                if (i < 100) print(" ");
                if (i < 10) print(" ");
                print(i + ": " + trace[i] + "\n");
            }
        }
    }
}

\: constructor (MinorMode; string name)
{
    return Wipe(name);
}

\: quitWipes (void; )
{
    State state = data();
    Wipe w = state.wipe;

    if (w neq nil) w.quitWipes(nil);
}

}
