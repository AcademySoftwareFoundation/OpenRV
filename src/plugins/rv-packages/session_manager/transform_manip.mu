//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  2D Transform manip
//

module: transform_manip {
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
use math_linear;
require io;

class: TransformManip : MinorMode
{
    ManipPoint := (PixelImageInfo,Point);
    Matrix     := float[4,4];

    class: EditNodePair 
    { 
        string tformNode;
        string inputNode; 
    }

    union: Control
    {
          NoControl
        | FreeTranslation 
        | TopLeftCorner
        | TopRightCorner
        | BotLeftCorner
        | BotRightCorner
    }

    use Control;

    EditNodePair[]   _editNodes;
    EditNodePair     _currentEditNode;
    Control          _control;
    Vec2             _gc;
    Vec2             _corner;
    Point            _downPoint;
    bool             _editing;
    bool             _didDrag;

    \: closestPointOnLine (Point; Point p, Point a, Point b)
    {
        let dir  = normalize(b - a),
            u    = dot(p - a, dir);

        u * dir + a;
    }

    \: computeGC (Vec2; Point[] corners)
    {
        Point gc;
        for_each (c; corners) gc += c;
        gc /= float(corners.size());
        gc;
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

    method: editNode (EditNodePair; string name)
    {
        for_each (enode; _editNodes)
            if (enode.tformNode == name) return enode;
        return nil;
    }

    method: activeImageIndex (int;)
    {
        for_each (i; renderedImages())
        {
            let v = tagValue(i.tags, "tmanip_state");
            if (v neq nil && v != "") return i.index;
        }

        return -1;
    }

    method: setManipState (void; EditNodePair p, string value)
    {
        if (p neq nil) 
        {
            if (nodeExists(p.tformNode)) set(p.tformNode + ".tag.tmanip_state", value);
        }
    }

    method: control ((Control, Vec2, Vec2); int index, Event event)
    {
        let corners = imageGeometryByIndex(index),
            p = event.pointer(),
            gc = computeGC(corners);

        for_each (c; corners)
        {
            let v = p - c;

            if (abs(v.x) < 25 && abs(v.y) < 25)
            {
                if (c.x < gc.x)
                {
                    return (if c.y > gc.y then TopLeftCorner else BotLeftCorner, gc, c);
                }
                else
                {
                    return (if c.y > gc.y then TopRightCorner else BotRightCorner, gc, c);
                }
            }
        }

        return (FreeTranslation, gc, gc);
    }

    method: move (void; Event event)
    {
        let last = _currentEditNode;
        _currentEditNode = nil;
        _control = NoControl;
        setCursor(Qt.ArrowCursor);

        for_each (p; imagesAtPixel(event.pointer()))
        {
            if (p.inside)
            {
                let v = tagValue(p.tags, "tmanip");

                if (v neq nil)
                {
                    _currentEditNode = editNode(v);
                    setManipState(_currentEditNode, "hover");
                    let (con, gc, corner) = control(p.index, event);
                    _control = con;
                    _gc      = gc;
                    _corner  = corner;

                    case (_control)
                    {
                        TopRightCorner  -> { setCursor(Qt.SizeBDiagCursor); }
                        BotLeftCorner   -> { setCursor(Qt.SizeBDiagCursor); }
                        TopLeftCorner   -> { setCursor(Qt.SizeFDiagCursor); }
                        BotRightCorner  -> { setCursor(Qt.SizeFDiagCursor); }
                        FreeTranslation -> { setCursor(Qt.OpenHandCursor); }
                        _               -> { setCursor(Qt.WhatsThisCursor); }
                    }
                    break;
                }
            }
        }

        if (last neq _currentEditNode) 
        {
            if (last neq nil) setManipState(last, "");
            redraw();
        }

        event.reject();
    }

    method: push (void; Event event)
    {
        if (_currentEditNode neq nil)
        {
            setCursor(Qt.ClosedHandCursor);
            //  popInputToTop(viewNode(), _currentEditNode.inputNode);
            setManipState(_currentEditNode, "editing");

            if (activeImageIndex() == -1) return;

            _downPoint = event.pointer();
            _didDrag   = false;
            _editing   = true;
            redraw();
        }
    }

    method: drag (void; Event event)
    {
        if (_currentEditNode neq nil)
        {
            let index = activeImageIndex();
            setCursor(Qt.ClosedHandCursor);

            if (index == -1) return;

            let tformNode = _currentEditNode.tformNode,
                inputNode = _currentEditNode.inputNode,
                transProp = "%s.transform.translate" % tformNode,
                scaleProp = "%s.transform.scale" % tformNode,
                trans     = getFloatProperty(transProp),
                scale     = getFloatProperty(scaleProp),
                corners   = imageGeometryByIndex(index), 
                a         = corners[0],
                b         = corners[1],
                c         = corners[2],
                d         = corners[3],
                pp        = event.pointer(),
                dp        = _downPoint,
                ip        = pp - dp,
                ba        = mag(b-a),
                da        = mag(d-a),
                aspect    = ba / da,
                dx        = ip.x / ba * scale[0] * aspect,
                dy        = ip.y / da * scale[1],
                diagDir   = normalize(_corner - _gc),
                diagDist  = dot(pp - _gc, diagDir),
                downDist  = dot(_downPoint - _gc, diagDir),
                diff      = diagDist - downDist,
                scl       = (diagDist - diff / 2.0) / downDist,
                sv        = diff * diagDir,
                sdx       = sv.x / ba * scale[0] * aspect,
                sdy       = sv.y / da * scale[1];
                //scl       = diagDist / downDist;


            case (_control)
            {
                FreeTranslation ->
                {
                    set(transProp, float[] {trans[0] + dx, trans[1] + dy});
                }

                _ ->
                {
                    set(transProp, float[] {trans[0] + sdx / 2.0, trans[1] + sdy / 2.0});
                    let newscale = max (scale[0] * scl, 0.01);
                    set(scaleProp, float[] {newscale, scale[1] * newscale/scale[0]});
                }
            }
            
            _downPoint = pp;
            _didDrag = true;
            redraw();

            //  popInputToTop(viewNode(), _currentEditNode.inputNode);
        }
    }

    method: release (void; Event event)
    {
        if (_editing) 
        {
            setManipState(_currentEditNode, "hover");
            setCursor(Qt.OpenHandCursor);
        }
        else
        {
            setCursor(Qt.ArrowCursor);
        }

        _didDrag = false;
        _editing = false;
    }

    method: resetAll (void; Event event)
    {
        for_each (enode; _editNodes)
        {
            let tformNode = enode.tformNode,
                transProp = "%s.transform.translate" % tformNode,
                scaleProp = "%s.transform.scale" % tformNode,
                rotProp   = "%s.transform.rotate" % tformNode;

            set(transProp, float[] {0,0});
            set(scaleProp, float[] {1,1});
            set(rotProp, float[] {0});
        }

        redraw();
    }

    \: nodeAspect (float; string node)
    {
        let geom = nodeImageGeometry(viewNode(), frame()),
            pa   = geom.pixelAspect,
            xps  = if pa > 1.0 then pa else 1.0,
            yps  = if pa < 1.0 then pa else 1.0;

        return (geom.width * xps) / (geom.height / yps);
    }

    method: fitAll (void; Event event)
    {
        let aspect = nodeAspect(viewNode());

        for_each (enode; _editNodes)
        {
            let tformNode = enode.tformNode,
                transProp = "%s.transform.translate" % tformNode,
                scaleProp = "%s.transform.scale" % tformNode,
                rotProp   = "%s.transform.rotate" % tformNode;

            let inaspect = nodeAspect(tformNode),
                s = aspect / inaspect;
            
            set(transProp, float[] {0,0});
            set(scaleProp, float[] {s,s});
            set(rotProp, float[] {0});
        }

        redraw();
    }

    method: removeTags (void;)
    {
        for_each (x; _editNodes)
        {
            let node = x.tformNode,
                pmanip = node + ".tag.tmanip",
                pstate = node + ".tag.tmanip_state";

            for_each (p; [pmanip, pstate])
            {
                if (propertyExists(p)) deleteProperty(p);
            }
        }
    }

    method: findEditingNodes (void; bool setStates=true)
    {
        let vnode       = viewNode(),
            infos       = metaEvaluateClosestByType(frame(), "RVTransform2D"),
            (ins, outs) = nodeConnections(viewNode(), false);

        _editNodes = EditNodePair[]();

        // happens when shutting down or deletion
        if (infos.size() != ins.size()) return;

        for_index (i; infos)
        {
            let info = infos[i],
                pname = info.node + ".tag.tmanip",
                sname = info.node + ".tag.tmanip_state";

            _editNodes.push_back(EditNodePair(info.node, ins[i]));

            if (setStates || !propertyExists(pname))
            {
                set(pname, info.node);
                set(sname, "");
            }
        }
    }

    method: nodeInputsChanged (void; Event event) 
    { 
        let node = event.contents(),
            vnode = viewNode();

        // Don't set the node states in this case 
        if (vnode neq nil && node == vnode) findEditingNodes(false);
    }

    method: afterGraphViewChange (void; Event event) { findEditingNodes(); event.reject(); }
    method: beforeGraphViewChange (void; Event event) { removeTags(); event.reject(); }
    method: activate(void;) { findEditingNodes(); }
    method: deactivate (void;) { setCursor(Qt.ArrowCursor); removeTags(); }

    method: TransformManip (TransformManip; string name)
    {
        init(name, 
             nil, // no global 
             [("pointer--move", move, "Search for Image"), 
              ("pointer-1--push", push, "Grab Tile"),
              ("pointer-1--drag", drag, "Move/Scale Tile"),
              ("pointer-1--release", release, ""),
              ("graph-node-inputs-changed", nodeInputsChanged, "Update session UI"),
              ("after-graph-view-change", afterGraphViewChange, "Update UI"),
              ("before-graph-view-change", beforeGraphViewChange, "Update UI"),
              ("stylus-pen--move", move, "Search for Nearest Edge"), 
              ("stylus-pen--push", push, "Move"),
              ("stylus-pen--drag", drag, "Move"),
              ("stylus-pen--release", release, "")],
             Menu {
                 {"Layout", Menu {
                         {"_", nil, nil, nil},
                         {"Fit All Images", fitAll, nil, nil},
                         {"Reset All Manips", resetAll, nil, nil}
                     }}}
             ,
             //
             //  manip events must be processed nearly last, since
             //  they cover the screen.
             //
             "zza" );
        
        _editing  = false;
        _didDrag  = false;
    }

    method: render (void; Event event)
    {
        if (_currentEditNode eq nil) return;

        State state = data();
        let domain  = event.domain(),
            bg      = state.config.bg,
            fg      = state.config.fg,
            index   = activeImageIndex();

        if (index == -1) return;

        setupProjection(domain.x, domain.y, event.domainVerticalFlip());

        try
        {
            let corners = imageGeometryByIndex(index),
                gc = computeGC(corners);

            _gc = gc;
            glEnable(GL_BLEND);
            glEnable(GL_LINE_SMOOTH);
            glEnable(GL_POINT_SMOOTH);
            glLineWidth(2.0);
                
            glBlendFunc(GL_SRC_ALPHA,
                        GL_ONE_MINUS_SRC_ALPHA);
            

            \: drawCorners (void; Vec2[] corners, float mult, float width)
            {
                for_index (i; corners) 
                {
                    let i0   = (if i == 0 then 3 else i - 1),
                        i1   = (i+1) % 4,
                        c    = corners[i],
                        c0   = corners[i0],
                        c1   = corners[i1],
                        m0   = mag(c0 - c),
                        m1   = mag(c1 - c),
                        dir0 = (c0 - c) / m0,
                        dir1 = (c1 - c) / m1,
                        nmult = if m1 / 2.0 < mult || m0 / 2.0 < mult
                                            then 0.0 else mult;
                    
                    glBegin(GL_LINES);
                    glVertex(c + dir0 * nmult); 
                    glVertex(c - normalize(dir0) * width);
                    glVertex(c + dir1 * nmult); 
                    glVertex(c - normalize(dir1) * width);
                    glEnd();
                }
            }

            glColor(Color(1,1,1,.5));
            glBegin(GL_LINE_LOOP);
            for_each (c; corners) glVertex(c);
            glEnd();

            glColor(Color(0,0,0,.5));
            glLineWidth(8.0);
            drawCorners(corners, 25, 0.0);
            glLineWidth(6.0);
            glColor(Color(1,1,1,.5));
            drawCorners(corners, 25, 0.0);

            glLineWidth(1.5);

            glPushMatrix();
            glTranslate(gc.x, gc.y, 0.0);
            glScale(25.0, 25.0, 25.0);
            //glRotate(rotang, 0, 0, 1);
            glColor(bg * Color(1,1,1,.5));
            circleGlyph(false);
            circleGlyph(true);
            glPopMatrix();

            glPushMatrix();
            glTranslate(gc.x, gc.y, 0.0);
            glScale(25.0, 25.0, 25.0);
            //glRotate(rotang + (if _cornerEdit then 45.0 else 0.0), 0, 0, 1);
            glColor(fg);
            translateIconGlyph(false);
            glColor(fg * 0.5);
            glLineWidth(1.0);
            translateIconGlyph(true);
            glPopMatrix();

            glDisable(GL_BLEND);
        }
        catch (...)
        {
            ; // ignore it
        }
    }
}

\: createMode (Mode;)
{
    return TransformManip("transform_manip");
}

}
