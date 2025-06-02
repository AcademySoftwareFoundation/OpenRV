//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

//
//  Glyphs and other drawing code not related to RV specifically
//

module: glyph {
use gl;
use glu;
use math;
use math_util;
use io;
use rvtypes;
use commands;
require gltext;

global regex MatchRN = "\r\n";
global regex MatchNN = "\n\n";

//
//  Higher order glyph functions
//

\: xformedGlyph (Glyph; Glyph g, float angle, float scale=1.0)
{
    \: (void; bool outline)
    {
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glRotate(angle, 0, 0, 1);
        glScale(scale, scale, scale);
        g(outline);
        glPopMatrix();
    };
}

\: xformedGlyph (Glyph; Glyph g, Point tran, float angle, float scale=1.0)
{
    \: (void; bool outline)
    {
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glRotate(angle, 0, 0, 1);
        glScale(scale, scale, scale);
        glTranslate(tran.x, tran.y, 0.0);
        g(outline);
        glPopMatrix();
    };
}

\: coloredGlyph (Glyph; Glyph g, Color c)
{
    \: (void; bool outline)
    {
        glColor(c);
        g(outline);
    };
}

operator: & (Glyph; Glyph a, Glyph b)
{
    \: (void; bool outline)
    {
        a(outline);
        b(outline);
    };
}

//
//  Primitive glyph functions
//

\: glSetupProjection (void; int w, int h)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, w, 0, h);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

\: drawRect (void; int glenum, Vec2 a, Vec2 b, Vec2 c, Vec2 d)
{
    glBegin(glenum);
    glVertex(a);
    glVertex(b);
    glVertex(c);
    glVertex(d);
    glEnd();
}

\: drawLineBox (void; Vec2 a, Vec2 b, Vec2 c, Vec2 d)
{
    drawRect(GL_LINE_LOOP, a, b, c, d);
}

\: drawColoredLineBox (void; float red, float green, float blue, Vec2 a, Vec2 b, Vec2 c, Vec2 d)
{
    glColor(red, green, blue, 1.0);
    drawRect(GL_LINE_LOOP, a, b, c, d);
}

\: drawCircleFan (void; float x, float y, float w,
                  float start, float end,
                  float ainc, bool outline = false)
{
    let a0 = start * pi * 2.0,
        a1 = end * pi * 2.0;

    glBegin(if outline then GL_LINE_STRIP else GL_TRIANGLE_FAN);
    if (!outline) glVertex(x, y);

    for (float a = a0; a < a1; a+= ainc)
    {
        glVertex(sin(a) * w + x, cos(a) * w + y);
    }

    glVertex(sin(a1) * w + x, cos(a1) * w + y);
    glEnd();
}


\: triangleGlyph (void; bool outline)
{
    glBegin(if outline then GL_LINE_LOOP else GL_TRIANGLES);
    glVertex(-0.5, 0);
    glVertex(0.5, -0.5);
    glVertex(0.5, 0.5);
    glEnd();
}

\: circleGlyph (void; bool outline)
{
    drawCircleFan(0, 0, 0.5, 0.0, 1.0, .3, outline);
}

\: squareGlyph (void; bool outline)
{
    glBegin(if outline then GL_LINE_LOOP else GL_QUADS);
    glVertex(-0.5, -0.5);
    glVertex(0.5, -0.5);
    glVertex(0.5, 0.5);
    glVertex(-0.5, 0.5);
    glEnd();
}

\: pauseGlyph (void; bool outline)
{
    glPushAttrib(GL_POLYGON_BIT);
    if (outline) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBegin(GL_QUADS);
    glVertex(-0.5, -0.5);
    glVertex(-0.1, -0.5);
    glVertex(-0.1, 0.5);
    glVertex(-0.5, 0.5);

    glVertex(0.1, -0.5);
    glVertex(0.5, -0.5);
    glVertex(0.5, 0.5);
    glVertex(0.1, 0.5);
    glEnd();
    glPopAttrib();
}

\: advanceGlyph (void; bool outline)
{
    glBegin(if outline then GL_LINE_LOOP else GL_TRIANGLES);
    glVertex(-0.5, 0);
    glVertex(0.2, -0.5);
    glVertex(0.2, 0.5);
    glEnd();

    glBegin(if outline then GL_LINE_LOOP else GL_QUADS);
    glVertex(0.3, -0.5);
    glVertex(0.5, -0.5);
    glVertex(0.5, 0.5);
    glVertex(0.3, 0.5);
    glEnd();
}

\: rgbGlyph (void; bool outline)
{
    glColor(1,0,0,1); drawCircleFan(0, 0, 0.5, 0.0, 0.33, .3, outline);
    glColor(0,1,0,1); drawCircleFan(0, 0, 0.5, 0.33, 0.66, .3, outline);
    glColor(0,0,1,1); drawCircleFan(0, 0, 0.5, 0.66, 1.0, .3, outline);
}

\: drawXGlyph (void; bool outline)
{
    // Yeah, this is a little weird
    drawCloseButton(0, 0, 0.75, Color(.3,.2,0,1), Color(1,.6,0,1));
}

\: drawStopGlyph (void; bool outline)
{
    glPushAttrib(GL_ENABLE_BIT);
    glEnable(GL_BLEND);
    glEnable(GL_LINE_SMOOTH);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(3.0);
    glColor(Color(0,0,0,1));
    drawCircleFan(0, 0, 0.75, 0, 1, .3);

    glColor(Color(1,1,1,0.7));
    glBegin(GL_LINES);
    glVertex(-0.5, 0);
    glVertex(0.5, 0);
    glEnd();
    glPopAttrib();
}

\: circleLerpGlyph (void; float start, bool outline)
{
    let ch = 1;

    for (float q = ch; q >= start; q *= 0.9)
    {
        let a = math.cbrt(1.0 - q/ch);
        glColor(Color(.2, 1, 1, 1) * a);
        drawCircleFan(0, 0, q, 0.0, 1.0, .1, true);
    }
}

\: tformCircle (void; bool outline)
{
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glScale(0.2333, 0.2333, 0.2333);
    circleGlyph(outline);
    glPopMatrix();
}

\: tformTriangle (void; float angle, bool outline)
{
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glRotate(angle, 0, 0, 1);
    glScale(0.25, 0.25, 0.25);
    glTranslate(-1.3, 0.0, 0.0);
    triangleGlyph(outline);
    glPopMatrix();
}

\: translateIconGlyph (void; bool outline)
{
    tformCircle(outline);
    
    for (float a = 0.0; a <= 360.0; a += 90.0)
    {
        tformTriangle(a, outline);
    }
}

\: translateXIconGlyph (void; bool outline)
{
    tformCircle(outline);
    
    for (float a = 90.0; a <= 360.0; a += 180.0)
    {
        tformTriangle(a, outline);
    }
}

\: translateYIconGlyph (void; bool outline)
{
    tformCircle(outline);
    
    for (float a = 0.0; a <= 360.0; a += 180.0)
    {
        tformTriangle(a, outline);
    }
}


\: lower_bounds (int; int[] array, int n)
{
    \: f (int; int[] array, int n, int i, int i0, int i1)
    {
        if (array[i] <= n)
        {
            if (i+1 == array.size() || array[i+1] > n)
            {
                return i;
            }
            else
            {
                return f(array, n, (i + i1) / 2, i, i1);
            }
        }

        if i == 0 then -1 else f(array, n, (i + i0) / 2, i0, i);
    }

    f(array, n, array.size() / 2, 0, array.size());
}



\: drawTextWithCartouche (BBox; float x, float y, string text,
                          float textsize, Color textcolor,
                          Color bgcolor,
                          Glyph g = nil,
                          Color gcolor = Color(.2,.2,.2,1))
{
    gltext.size(textsize);
    if (g neq nil) text = "   " + text;

    let b = gltext.bounds(text),
        w = b[2] + b[0],
        a = gltext.ascenderHeight(),
        d = gltext.descenderDepth(),
        mx = (a - d) * .05,
        x0 = x - mx,
        x1 = x + w + mx,
        y0 = y + d - mx,
        y1 = y + a + mx,
        rad = (y1 - y0) * 0.5,
        ymid = (y1 + y0) * 0.5;

    //print("%s, %s, %s\n" % (a, d, b));

    glColor(bgcolor);
    glBegin(GL_POLYGON);
    glVertex(x0, y0);
    glVertex(x1, y0);
    glVertex(x1, y1);
    glVertex(x0, y1);
    glEnd();

    drawCircleFan(x0, ymid, rad, 0.5, 1.0, .3);
    drawCircleFan(x1, ymid, rad, 0.0, 0.5, .3);

    glColor(bgcolor * 0.8);
    glPushAttrib(GL_ENABLE_BIT);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(1.0);
    drawCircleFan(x0, ymid, rad, 0.5, 1.0, .3, true);
    drawCircleFan(x1, ymid, rad, 0.0, 0.5, .3, true);
    glBegin(GL_LINES);
    glVertex(x0, y0);
    glVertex(x1, y0);
    glVertex(x1, y1);
    glVertex(x0, y1);
    glEnd();

    if (g neq nil)
    {
        glColor(gcolor);
        draw(g, x, ymid, 0, rad, false);
        glEnable(GL_LINE_SMOOTH);
        glColor(gcolor * 0.8);
        draw(g, x, ymid, 0, rad, true);
    }


    glPopAttrib();

    gltext.color(textcolor);
    gltext.writeAt(x, y, text);

    return BBox(x0 - rad, y0, x1 + rad, y1);
}

\: setupProjection (void; float w, float h, bool vflip = false)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (vflip) gluOrtho2D(0.0, w-1, h-1, 0.0);
    else gluOrtho2D(0.0, w-1, 0.0, h-1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

\: setupProjectionFromEvent (void; Event event)
{
    let d = event.domain();

    setupProjection(d.x, d.y, event.domainVerticalFlip());
}

\: fitTextInBox (int; string text, int w, int h)
{
    int textSize = 64, lastUpperBound = 2048, lastLowerBound = 4;
    int bw, bh;

    if (text eq nil || text.size() == 0) 
    {
        throw exception("ERROR: fitTextInBox: empty string.");
    }
    
    int count = 0;
    do
    {
        gltext.size(textSize);
        let b  = gltext.bounds(text);
        bw = b[0] + b[2];
        bh = b[1] + b[3];
        if (bw > w || bh > h) 
        {
            lastUpperBound = textSize;
            textSize = (textSize + lastLowerBound)/2;
        }
        else
        {
            lastLowerBound = textSize;
            textSize = (textSize + lastUpperBound)/2;
        }
        count += 1;
    }
    while (bw > w || bh > h || (lastUpperBound - lastLowerBound) > 1);

    return textSize - 2;
}


\: fitNameValuePairsInBox (int; 
                           StringPair[] pairs, 
                           int margin, int w, int h)
{
    int textSize = 64, lastUpperBound = 2048, lastLowerBound = 4;
    vector float[2] tbox;

    if (pairs.size() == 0) 
    {
        throw exception("ERROR: fitNameValuePairsInBox: empty pairs.");
    }
    
    int count = 0;
    do
    {
        gltext.size(textSize);
        tbox = nameValuePairBounds(pairs, margin)._0;
        if (tbox.x > w || tbox.y > h) 
        {
            lastUpperBound = textSize;
            textSize = (textSize + lastLowerBound)/2;
        }
        else
        {
            lastLowerBound = textSize;
            textSize = (textSize + lastUpperBound)/2;
        }
        count += 1;
    }
    while (tbox.x > w || tbox.y > h || (lastUpperBound - lastLowerBound) > 1);

    return textSize - 2;
}


\: drawRoundedBox (void;
                   int x0, int y0,
                   int x1, int y1,
                   int m,
                   Color c,
                   Color oc)
{
    glColor(c);
    glBegin(GL_QUADS);
    glVertex(x0, y0);
    glVertex(x1, y0);
    glVertex(x1, y1);
    glVertex(x0, y1);

    glVertex(x0 - m, y0 + m);
    glVertex(x0, y0 + m);
    glVertex(x0, y1 - m);
    glVertex(x0 - m, y1 - m);

    glVertex(x1, y0 + m);
    glVertex(x1 + m, y0 + m);
    glVertex(x1 + m, y1 - m);
    glVertex(x1, y1 - m);
    glEnd();

    drawCircleFan(x0, y0 + m, m, 0.5, 0.75, .3);
    drawCircleFan(x1, y0 + m, m, 0.25, 0.5, .3);
    drawCircleFan(x1, y1 - m, m, 0.0, 0.25, .3);
    drawCircleFan(x0, y1 - m, m, 0.75, 1.0, .3);

    glPushAttrib(GL_ENABLE_BIT);

    glColor(oc);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POLYGON_SMOOTH);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glHint (GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);
    glLineWidth(3.0);

    drawCircleFan(x0, y0 + m, m, 0.5, 0.75, .3, true);
    drawCircleFan(x1, y0 + m, m, 0.25, 0.5, .3, true);
    drawCircleFan(x1, y1 - m, m, 0.0, 0.25, .3, true);
    drawCircleFan(x0, y1 - m, m, 0.75, 1.0, .3, true);

    glBegin(GL_LINES);
    glVertex(x0, y0);
    glVertex(x1, y0);
    glVertex(x1, y1);
    glVertex(x0, y1);
    glVertex(x0 - m, y0 + m);
    glVertex(x0 - m, y1 - m);
    glVertex(x1 + m, y0 + m);
    glVertex(x1 + m, y1 - m);
    glEnd();
    glLineWidth(1.0);
    glPopAttrib();
}

\: drawDropRegions (int; 
                    int w,
                    int h,
                    int x,
                    int y,
                    int margin,
                    string[] descriptors)
{
    let m  = margins(),
        devicePixelRatio=devicePixelRatio(),
        bsize = (h - m[2] - m[3]) / descriptors.size(),
        inregion = -1;

    for_index (i; descriptors)
    {
        gltext.size(20*devicePixelRatio);

        let y0 = bsize * i + margin + m[3],
            x0 = m[0] + margin,
            y1 = bsize * (i+1) - margin + m[3],
            x1 = w - margin - m[1],
            t  = descriptors[i],
            b  = gltext.bounds(t),
            tw = b[2] + b[0],
            active = y >= y0 && y <= y1,
            fg = if active then Color(1,1,1,1) else Color(.5, .5, .5, 1),
            bg = Color(0, 0, 0, .85);

        drawRoundedBox(x0, y0, x1, y1, 10, bg, fg);

        if (active) inregion = i;

        gltext.color(fg);
        gltext.writeAt((x1 - x0 - tw) * .5 + x0, 
                       math_util.lerp(y0, y1, 0.5), 
                       t);
    }

    inregion;
}

\: wrapValue(string; string value, int wrap)
{
    if (wrap <= 0) return value;

    int count = 0;
    string final;
    for_each (p; string.split(value, " "))
    {
        if (count > wrap)
        {
            final += "\n";
            count = 0;
        }
        final += p + " ";
        count += p.size() + 1;
    }

    return final;
}

\: nameValuePairBounds (NameValueBounds; (string,string)[] pairs, int margin)
{
    let nw      = 0,
        vw      = 0,
        h       = 0,
        a       = gltext.ascenderHeight(),
        d       = gltext.descenderDepth(),
        th      = a - d,
        x0      = -d,
        x1      = -d,
        y0      = -margin,
        y1      = margin,
        nbounds = float[4][](),
        vbounds = float[4][]();

    for_each (a; pairs)
    {
        let (name, value) = a,
            bn            = gltext.bounds(name),
            bv            = gltext.bounds(value);

        nbounds.push_back(bn);
        vbounds.push_back(bv);

        nw = max(nw, bn[2] + bn[0]);
        vw = max(vw, bv[2] + bv[1]);
        h += th;
    }

    x1 += nw + vw;
    y1 += h;

    (Vec2(x1-x0, y1-y0), nbounds, vbounds, nw);
}


\: expandNameValuePairs (StringPair[]; StringPair[] pairs, int wrap=0)
{
    StringPair[] newPairs;

    \: reverse (string[]; string[] s)
    {
        string[] n;
        for (int i=s.size()-1; i >= 0; i--) n.push_back(s[i]);
        n;
    }

    for_each (p; pairs)
    {
        let (name, value) = p;
        if (wrap > 0 && value.size() > wrap) value = wrapValue(value, wrap);
	//  Don't collapse multiple newlines
	value = MatchRN.replace(value, "\n");
	value = MatchNN.replace(value, "\n \n");
        let lines = string.split(value, "\n\r");

        if (lines.size() > 1)
        {
            for_each (line; reverse(lines.rest()))
            {
                if (line != "") newPairs.push_back(StringPair("", line));
            }

            if (lines[0] != "") newPairs.push_back(StringPair(name, lines[0]));
        }
        else
        {
            newPairs.push_back(p);
        }
    }

    newPairs;
}

\: drawNameValuePairs (NameValueBounds;
                       StringPair[] pairs,
                       Color fg, Color bg,
                       int x, int y, int margin,
                       int maxw=0, int maxh=0,
                       int minw=0, int minh=0,
                       bool nobox=false)
{
    m := margin;    // alias

    let (tbox, nbounds, vbounds, nw) = nameValuePairBounds(pairs, m);

    let vw      = 0,
        h       = 0,
        a       = gltext.ascenderHeight(),
        d       = gltext.descenderDepth(),
        th      = a - d;

    float
        x0      = x - d,
        y0      = y - m,
        x1      = tbox.x + x0,
        y1      = tbox.y + y0;

    let xs = x1 - x0,
        ys = y1 - y0;

    if (minw > 0 && xs < minw) x1 = x0 + minw;
    if (minh > 0 && ys < minh) y1 = y0 + minh;
    if (maxw > 0 && xs > maxw ) x1 = x0 + maxw;
    if (maxh > 0 && ys > maxh ) y1 = y0 + maxh;

    tbox.x = x1 - x0;   // adjust 
    tbox.y = y1 - y0;

    glPushAttrib(GL_ENABLE_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (!nobox) drawRoundedBox(x0, y0, x1, y1, m, bg, fg * Color(.5,.5,.5,.5));

    glColor(fg * Color(1,1,1,.25));
    glBegin(GL_LINES);
    glVertex(x + nw + m/4, y0 + m/2);
    glVertex(x + nw + m/4, y1 - m/2);
    glEnd();

    for_index (i; pairs)
    {
        let (n, v)  = pairs[i],
            bn      = nbounds[i],
            bv      = vbounds[i],
            tw      = bn[2] + bn[0];

        gltext.color(fg - Color(0,0,0,.25));
        gltext.writeAt(x + (nw - tw), y, n);
        gltext.color(fg);
        gltext.writeAt(x + nw + m/2, y, v);
        y += th;
        //if (i == s - 3) y+= m/2;
    }

    glDisable(GL_BLEND);
    glPopAttrib();

    (tbox, nbounds, vbounds, nw);
}


\: drawCloseButton (void; float x, float y, float radius,
                    Color bg, Color fg)
{
    let r2 = radius / 2;

    glPushAttrib(GL_ENABLE_BIT);
    glEnable(GL_BLEND);
    glEnable(GL_LINE_SMOOTH);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(2.0);
    glColor(bg);
    drawCircleFan(x, y, radius, 0, 1, .3);
    glColor(fg);
    drawCircleFan(x, y, radius, 0, 1, .3, true);

    glBegin(GL_LINES);
    glVertex(x - r2, y - r2);
    glVertex(x + r2, y + r2);
    glVertex(x - r2, y + r2);
    glVertex(x + r2, y - r2);
    glEnd();
    glPopAttrib();
}

\: draw (Glyph g, float x, float y, float angle, float size, bool outline)
{
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glTranslate(x, y, 0);
    glRotate(angle, 0, 0, 1);
    glScale(size, size, size);
    g(outline);
    glPopMatrix();
}

}   // END MODULE glyph
