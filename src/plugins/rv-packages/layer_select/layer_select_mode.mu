//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
module: layer_select_mode 
{
use rvtypes;
use commands;
use rvui;
use gl;
use glyph;
use app_utils;
use math;
use math_util;
use extra_commands;
use glu;
require io;

//----------------------------------------------------------------------
//
//  LayerSelect Widget
//

\: sourceName(string; string name)
{
    let s = name.split(".");
    return s[0];
}

class: LayerSelect : Widget
{

    int     _activeLayerIndex;
    int     _selectLayerIndex;
    bool    _default;
    float   _th;
    int     _nLayers;
    Vec2    _tbox;
    float   _nw;
    bool    _drawInMargin;

    method: selectLayer(void; Event event, int incr)
    {
        State state = data();

        let pinfo   = state.pixelInfo,
            iname   = if (pinfo neq nil && !pinfo.empty())
                then pinfo.front().name
                else nil;

        let layers  = sourceMedia(iname)._1,
            ls = layers.size();

        let idx = _selectLayerIndex - incr;

        if(idx >= 0 && idx < _nLayers) _selectLayerIndex = idx;

        redraw();

        _drawOnPresentation = true;

    }

    method: setSelectedLayer(void; Event event)
    {
        State state = data();

        let pinfo   = state.pixelInfo,
            iname   = if (pinfo neq nil && !pinfo.empty())
                then pinfo.front().name
                else nil,
            node    = if (pinfo neq nil && !pinfo.empty())
                then pinfo.front().node
                else nil;

        if (iname eq nil) return;

        let layers  = sourceMedia(iname)._1;

        try
        {
            setStringProperty("%s.request.imageComponent" % node,
                if _selectLayerIndex == 0
                    then string[]()
                    else string[]{ "layer", "", layers[_selectLayerIndex - 1]},
                true);
            _activeLayerIndex = _selectLayerIndex;
        }
        catch (exception exc)
        {
            print("\n");
            print(exc);
            print("\n");
        }

        redraw();

    }

    method: eventToIndex(int; Point p)
    {
        State state = data();
        let margin  = state.config.bevelMargin;


        return _nLayers - int(((p.y - _y + margin ) / _th )) + 1;
    }

    method: releaseSelect(void; Event event)
    {
        State state = data();
        let margin  = state.config.bevelMargin;

        let rx = event.relativePointer().x;
        if(margin < rx && rx < _tbox.x + margin)
        {
            let di = eventToIndex(_downPoint);

            if(di < _nLayers && di >= 0)
            {
                _selectLayerIndex = di;
                setSelectedLayer(event);
            }
        }

        release(this, event, nil);
    }

    method: handleMotion(void; Event event)
    {
        let gp = event.pointer();
                    
        if (!this.contains (gp))
        {   
            if(_default){

                _selectLayerIndex = 0;
            }
            else
            {
                _selectLayerIndex = _activeLayerIndex + 1;
            }
        }
        else
        {

            let di = eventToIndex(event.pointer());

            if(di < _nLayers && di >= 0)
            {
                _selectLayerIndex = di;
            }
        }

        State state = data();

        let domain = event.subDomain(),
            p      = event.relativePointer(),
            tl     = vec2f(0, domain.y),
            pc     = p - tl,
            d      = mag(pc),
            m      = state.config.bevelMargin,
            lc     = this._inCloseArea,
            near   = d < m;

        if (near != lc) redraw();
        this._inCloseArea = near;

        redraw();
    }

    method: optFloatingSelector (void; Event event)
    {
        _drawInMargin = !_drawInMargin;
        writeSetting ("LayerSelect", "widgetIsDocked", SettingsValue.Bool(_drawInMargin));

        if (_drawInMargin) drawInMargin (0);
        else 
        {
            drawInMargin (-1);
	    vec4f m = vec4f{-1.0, -1.0, -1.0, -1.0};
            m[0] = 0;
            setMargins (m, true);
        }
        redraw();
    }

    method: isFloatingSelector (int;) 
    { 
        if _drawInMargin then UncheckedMenuState else CheckedMenuState; 
    }

    method: popupOpts (void; Event event)
    {
        popupMenu (event, Menu {
            {"Layer Selector", nil, nil, \: (int;) { DisabledMenuState; }},
            {"_", nil},
            {"Floating Selector", optFloatingSelector, nil, isFloatingSelector},
        });
    }

    method: LayerSelect (LayerSelect; string name)
    {
        this.init(name, 
                  [ ("pointer-1--push", storeDownPoint(this,), ""),
                    ("pointer--move", handleMotion, ""), 
                    ("pointer-1--release", releaseSelect, ""),
                    ("pointer-1--drag", drag(this,), "Move Widget"),
                    ("stylus-pen--push", storeDownPoint(this,), ""),
                    ("stylus-pen--move", handleMotion, ""), 
                    ("stylus-pen--release", releaseSelect, ""),
                    ("stylus-pen--drag", drag(this,), "Move Widget"),
                    ("pointer--wheelup", selectLayer(,1), "Choose Previous Layer"),
                    ("pointer--wheeldown", selectLayer(,-1), "Choose Next Layer"),
                    ("pointer-3--push", popupOpts, "Popup Selector Options"),
                    ("pointer-2--push", setSelectedLayer, "Set Selected Layer") ],
                  false);

        _x = 40;
        _y = 60;
       _activeLayerIndex = _selectLayerIndex = 0;
       let SettingsValue.Bool b1 = readSetting ("LayerSelect", "widgetIsDocked", SettingsValue.Bool(true));
       _drawInMargin = b1;
       
        
        if (_drawInMargin) drawInMargin(0);
    }

    method: render (void; Event event)
    {
        State state = data();

        let sinfo = sourcesRendered(),
            iname = if (sinfo neq nil && !sinfo.empty())
                         then sinfo.front().name
                         else nil,
            node  = if (sinfo neq nil && !sinfo.empty())
                         then sinfo.front().node
                         else nil;

        let domain  = event.domain(),
            bg      = state.config.bg,
            fg      = state.config.fg,
            err     = isCurrentFrameError(),
            layers  = sourceMedia(iname)._1;

        _nLayers = layers.size() + 1; //add the default

        (string,string)[] attrs;
        int[]             ai; 
        string            activelayer;

        if(layers eq nil) return;

        try
        {
            let imgComp = getStringProperty("%s.request.imageComponent" % node);
            if (imgComp neq nil && imgComp.size() == 3 && imgComp[0] == "layer")
            {
                activelayer = imgComp[2];
            }
            else
            {
                activelayer = "";
            }
        }
        catch (exception exc)
        {
            print(exc);
        }

        for(int i = layers.size() - 1; i >= 0; i--)
        {
            attrs.push_back(("        ", layers[i]));
            if(activelayer == layers[i])
            {
                ai.push_back(i);
               _activeLayerIndex = i;
            }
        }

        if (activelayer == "")
        {
            ai.push_back( -1 );
           _default = true;
           _activeLayerIndex = 0;
       }
       else
       {
            _default = false;
        }


        attrs.push_back(("        ", "Default"));

        gltext.size(state.config.infoTextSize);
        setupProjection(domain.x, domain.y, event.domainVerticalFlip());

        let margin  = state.config.bevelMargin,
            x       = if (_drawInMargin) then 0 else _x + margin,
            vMargins= margins(),
            nvb1    = nameValuePairBounds(expandNameValuePairs(attrs), margin),
            vs      = viewSize(),
            yspace  = vs[1]-vMargins[3]-vMargins[2],
            midy    = vMargins[3] + yspace/2,
            adjy    = midy - (nvb1._0[1])/2,
            targetY = max(vMargins[3] + margin, adjy + margin),
            targetW = nvb1._0[0] + 1.25*margin; 

        if (_drawInMargin) 
        {
            _y = targetY - margin;
            let w = max (vMargins[0], targetW);
            glColor(Color(0,0,0,1));
            glBegin(GL_QUADS);
            glVertex(0, vs[1]-vMargins[2]);
            glVertex(w, vs[1]-vMargins[2]);
            glVertex(w, vMargins[3]);
            glVertex(0, vMargins[3]);
            glEnd();
        }

        let y       = _y + margin,
            nvb     = drawNameValuePairs(expandNameValuePairs(attrs), fg, bg, x, y, margin, 
                    0, 0, 0, 0, _drawInMargin),
            tbox    = nvb._0,
            emin    = vec2f(if (_drawInMargin) then 0 else _x, _y),
            emax    = emin + tbox + vec2f(margin + (if (_drawInMargin) then margin/4 else margin), 0.0);

        let fa = int(gltext.ascenderHeight()),
            fd = int(gltext.descenderDepth()),
            th = fa - fd,
            gx = x + margin/2;

        _th = th;
        _tbox = tbox;
        _nw = nvb._3;

        glEnable(GL_POINT_SMOOTH);
        glPointSize(6.0);
        glBegin(GL_POINTS);

        glColor(Color(.75,.75,.15,1));
        let gy = y + th * (layers.size() - _selectLayerIndex ) + fd + th/2 + 2.0;
        glVertex(gx, gy);

        glColor(Color(.15,.75,.75,1));

        for_index(a; ai)
        {

            let gy = y + th * (layers.size() - ai[a] - 1) + fd + th/2 + 2.0;
            glVertex(gx, gy);

        }

        glEnd();

        if (_inCloseArea)
        {
            drawCloseButton(x - margin/2,
                            tbox.y + y - margin - margin/4,
                            margin/2, bg, fg);
        }

        

        this.updateBounds(emin, emax);
    }
}


\: createMode (Mode;)
{
    return LayerSelect("LayerSelect");
}

}
