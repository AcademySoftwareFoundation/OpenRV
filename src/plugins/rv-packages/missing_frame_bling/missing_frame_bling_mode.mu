//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
module: missing_frame_bling_mode {
use rvtypes;
use commands;
use glyph;
use extra_commands;
use gl;
require gltext;

class: MissingFrameBlingMode : MinorMode
{ 
    NEPair := (string, EventFunc);

    bool        _dispMsg;
    string      _type;
    EventFunc   _render;
    NEPair[]    _typeMap;

    method: renderX (void; Event event)
    {
        let contents  = event.contents(),
            parts     = contents.split(";"),
            source    = parts[1],
            mediaPath = sourceMedia(source)._0,
            media     = io.path.basename(mediaPath),
            geom      = sourceGeometry(source),
            domain    = event.domain(),
            w         = domain.x,
            h         = domain.y;

        setupProjection(w, h, event.domainVerticalFlip());

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_LINE_SMOOTH);
        glColor(Color(1,0,0,1));

        glBegin(GL_LINES);
        glVertex(geom[0]);
        glVertex(geom[2]);
        glVertex(geom[1]);
        glVertex(geom[3]);
        glEnd();


        glDisable(GL_BLEND);
    }

    method: renderShow (void; Event event)
    {
        let contents  = event.contents(),
            parts     = contents.split(";"),
            source    = parts[1],
            mediaPath = sourceMedia(source)._0,
            media     = io.path.basename(mediaPath),
            geom      = sourceGeometry(source),
            domain    = event.domain(),
            w         = domain.x,
            h         = domain.y,
            iw        = geom[2].x - geom[0].x,
            ih        = geom[2].y - geom[0].y,
            ix        = geom[0].x,
            iy        = geom[0].y;

        setupProjection(w, h, event.domainVerticalFlip());

        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor(Color(.5,.5,.5,.1));

        gltext.size(25);

        let text = "Missing % 4s" % parts[0],
            b    = gltext.bounds("Missing 0000"),
            tw   = b[2],
            th   = b[3],
            x    = iw / 2 - tw / 2 + ix,
            y    = ih / 2 - th / 2 + iy;

        gltext.color(Color(0, 0, 0, 1));
        gltext.writeAtNL(x-1, y-1, text);

        gltext.color(Color(.8, .8, .8, 1));
        gltext.writeAtNL(x, y, text);

        glDisable(GL_BLEND);
        glPopAttrib();
   }

    method: renderBlack (void; Event event)
    {
        let contents  = event.contents(),
            parts     = contents.split(";"),
            geom      = sourceGeometry(parts[1]),
            domain    = event.domain(),
            w         = domain.x,
            h         = domain.y;

        setupProjection(w, h, event.domainVerticalFlip());

        glDisable(GL_BLEND);
        glColor(Color(0, 0, 0, 1));

        glBegin(GL_QUADS);
        glVertex(geom[0]);
        glVertex(geom[1]);
        glVertex(geom[2]);
        glVertex(geom[3]);
        glEnd();
    }

    method: renderNothing (void; Event event)
    {
        ; // nothing
    }

    method: renderMissing (void; Event event)
    {
        //
        // Presently rvui uses the missing-image event to display a feedback 
        // message about the frames that are missing. If we want to stop this
        // behavior, then we do not reject the event for rvui to see.
        //

        _render(event);
        if (_dispMsg)
        {
            event.reject();
        }
    }

    method: checkFunc ((int;); string name)
    {
        \: (int;)
        {
            if this._type == name then CheckedMenuState else UncheckedMenuState;
        };
    }
    
    method: setFunc (void; string name, Event event)
    {
        for_each (t; _typeMap) 
        {
            if (t._0 == name) 
            {
                _render = t._1;
                _type = name;
                writeSetting("MissingBling", "type", SettingsValue.String(name));
            }
        }
    }

    method: toggleMsg(void; Event event)
    {
        _dispMsg = !_dispMsg;
        writeSetting("MissingBling", "displayMsg", SettingsValue.Bool(_dispMsg));
    }

    method: dispMsg(int;)
    {
        return if _dispMsg then CheckedMenuState else UncheckedMenuState;
    }

    method: MissingFrameBlingMode(MissingFrameBlingMode; string name)
    {
        init(name,
             nil,
             [("missing-image", renderMissing, "Render indication of missing frames")],
             Menu {
                 {"View", Menu {
                         {"Missing Frames", Menu {
                                 {"Hold", setFunc("hold",), nil, checkFunc("hold")},
                                 {"Red X", setFunc("x",), nil, checkFunc("x")},
                                 {"Show Frame Number", setFunc("show",), nil, checkFunc("show")},
                                 {"Black", setFunc("black",), nil, checkFunc("black")},
                                 {"_", nil},
                                 {"Display Feedback Message", toggleMsg, nil, dispMsg}
                             }
                         }
                     }
                 }
             });

        let SettingsValue.String s
            = readSetting("MissingBling", "type", SettingsValue.String("show")),
            SettingsValue.Bool d
            = readSetting("MissingBling", "displayMsg", SettingsValue.Bool(true));

        _dispMsg = d;
        _type    = s;
        _typeMap = NEPair[] { ("hold", renderNothing),
                              ("x", renderX), 
                              ("show", renderShow),
                              ("black", renderBlack)  };

        for_each (t; _typeMap) if (t._0 == s) _render = t._1;
        if (_render eq nil) _render = renderShow;
    }
}

\: createMode (Mode;)
{
    return MissingFrameBlingMode("missing-frame-bling");
}
}
