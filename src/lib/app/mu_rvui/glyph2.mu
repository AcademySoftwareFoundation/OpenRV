//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

//
//  Glyphs and other drawing code using QPainter instead of OpenGL
//

module: glyph2 {
use qt;
use math;
use math_util;
use io;
use rvtypes;
use commands;

global regex MatchRN = "\r\n";
global regex MatchNN = "\n\n";


global float g_viewHeight = 1;
global float g_viewWidth = 1;
global bool g_flipY = false;

//
//  Helper to convert Color to QColor
//

\: toQColor (qt.QColor; Color c)
{
    qt.QColor(int(c.x * 255), int(c.y * 255), int(c.z * 255), int(c.w * 255));
}

//
//  Higher order glyph functions
//

\: g2XformedGlyph (Glyph; Glyph g, float angle, float scale=1.0)
{
    \: (void; bool outline)
    {
        let painter = commands.mainViewPainter();
        if (painter eq nil) return;
        
        painter.save();
        painter.rotate(angle);
        painter.scale(scale, scale);
        g(outline);
        painter.restore();
    };
}

\: g2XformedGlyph (Glyph; Glyph g, Point tran, float angle, float scale=1.0)
{
    \: (void; bool outline)
    {
        let painter = commands.mainViewPainter();
        if (painter eq nil) return;
        
        painter.save();
        painter.rotate(angle);
        painter.scale(scale, scale);
        painter.translate(tran.x, tran.y);
        g(outline);
        painter.restore();
    };
}

\: g2ColoredGlyph (Glyph; Glyph g, Color c)
{
    \: (void; bool outline)
    {
        let painter = commands.mainViewPainter();
        if (painter eq nil) return;
        
        let qc = toQColor(c);
        painter.setPen(qt.QPen(qc));
        painter.setBrush(qt.QBrush(qc));
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


\: vflip (Vec2; Vec2 p)
{
    Vec2(p.x, g_viewHeight - p.y);
}

\: vflip (float; float p)
{
    Vec2(p.x, g_viewHeight - p.y);
}


\: g2SetupProjection (void; float w, float h)
{
    g_viewHeight = h;
    g_viewWidth = w;
    g_flipY = true;
}

\: transformY (float; float y)
{
    if (g_flipY) return g_viewHeight - y;
    else return y;
}

\: g2DrawRect (void; Vec2 a, Vec2 b, Vec2 c, Vec2 d, bool filled=true)
{
    let painter = commands.mainViewPainter();
    if (painter eq nil) return;
    
    // Draw as lines connecting the points
    if (filled)
    {
        // For filled, draw as polygon using lines (simple approximation)
        // A better implementation would use fillPolygon if available
        painter.drawLine(a.x, a.y, b.x, b.y);
        painter.drawLine(b.x, b.y, c.x, c.y);
        painter.drawLine(c.x, c.y, d.x, d.y);
        painter.drawLine(d.x, d.y, a.x, a.y);
    }
    else
    {
        painter.drawLine(a.x, a.y, b.x, b.y);
        painter.drawLine(b.x, b.y, c.x, c.y);
        painter.drawLine(c.x, c.y, d.x, d.y);
        painter.drawLine(d.x, d.y, a.x, a.y);
    }
}

\: g2DrawLineBox (void; Vec2 a, Vec2 b, Vec2 c, Vec2 d)
{
    g2DrawRect(a, b, c, d, false);
}

\: g2DrawColoredLineBox (void; float red, float green, float blue, Vec2 a, Vec2 b, Vec2 c, Vec2 d)
{
    let painter = commands.mainViewPainter();
    if (painter eq nil) return;
    
    let qc = qt.QColor(int(red * 255), int(green * 255), int(blue * 255));
    painter.setPen(qt.QPen(qc));
    g2DrawRect(a, b, c, d, false);
}

\: g2DrawCircleFan (void; float x, float y, float w,
                  float start, float end,
                  float ainc, bool outline = false)
{
    let painter = commands.mainViewPainter();
    if (painter eq nil) return;
    
    // Draw circle/arc using drawEllipse
    if (start == 0.0 && end == 1.0)
    {
        // Full circle
        painter.drawEllipse(qt.QPointF(x, y), w, w);
    }
    else
    {
        // Partial arc - approximate with lines
        let a0 = start * pi * 2.0,
            a1 = end * pi * 2.0;
        
        if (!outline)
        {
            // Draw filled pie: lines from center to arc
            let prevX = sin(a0) * w + x;
            let prevY = cos(a0) * w + y;
            
            for (float a = a0 + ainc; a < a1; a += ainc)
            {
                let nx = sin(a) * w + x;
                let ny = cos(a) * w + y;
                painter.drawLine(x, y, prevX, prevY);
                painter.drawLine(prevX, prevY, nx, ny);
                prevX = nx;
                prevY = ny;
            }
            // Last segment
            let nx = sin(a1) * w + x;
            let ny = cos(a1) * w + y;
            painter.drawLine(x, y, prevX, prevY);
            painter.drawLine(prevX, prevY, nx, ny);
            painter.drawLine(x, y, nx, ny);
        }
        else
        {
            // Draw arc outline
            let prevX = sin(a0) * w + x;
            let prevY = cos(a0) * w + y;
            
            for (float a = a0 + ainc; a < a1; a += ainc)
            {
                let nx = sin(a) * w + x;
                let ny = cos(a) * w + y;
                painter.drawLine(prevX, prevY, nx, ny);
                prevX = nx;
                prevY = ny;
            }
            // Last segment
            let nx = sin(a1) * w + x;
            let ny = cos(a1) * w + y;
            painter.drawLine(prevX, prevY, nx, ny);
        }
    }
}

\: g2TriangleGlyph (void; bool outline)
{
    let painter = commands.mainViewPainter();
    if (painter eq nil) return;
    
    painter.drawLine(-0.5, 0.0, 0.5, -0.5);
    painter.drawLine(0.5, -0.5, 0.5, 0.5);
    painter.drawLine(0.5, 0.5, -0.5, 0.0);
}

\: g2CircleGlyph (void; bool outline)
{
    g2DrawCircleFan(0, 0, 0.5, 0.0, 1.0, .3, outline);
}

\: g2SquareGlyph (void; bool outline)
{
    let painter = commands.mainViewPainter();
    if (painter eq nil) return;
    
    if (outline)
    {
        painter.drawLine(-0.5, -0.5, 0.5, -0.5);
        painter.drawLine(0.5, -0.5, 0.5, 0.5);
        painter.drawLine(0.5, 0.5, -0.5, 0.5);
        painter.drawLine(-0.5, 0.5, -0.5, -0.5);
    }
    else
    {
        painter.fillRect(-0.5, -0.5, 1.0, 1.0, painter.brush());
    }
}

\: g2PauseGlyph (void; bool outline)
{
    let painter = commands.mainViewPainter();
    if (painter eq nil) return;
    
    if (outline)
    {
        painter.drawRect(-0.5, -0.5, 0.4, 1.0);
        painter.drawRect(0.1, -0.5, 0.4, 1.0);
    }
    else
    {
        painter.fillRect(-0.5, -0.5, 0.4, 1.0, painter.brush());
        painter.fillRect(0.1, -0.5, 0.4, 1.0, painter.brush());
    }
}

\: g2AdvanceGlyph (void; bool outline)
{
    let painter = commands.mainViewPainter();
    if (painter eq nil) return;
    
    // Triangle part
    painter.drawLine(-0.5, 0.0, 0.2, -0.5);
    painter.drawLine(0.2, -0.5, 0.2, 0.5);
    painter.drawLine(0.2, 0.5, -0.5, 0.0);
    
    // Rectangle part
    if (outline)
        painter.drawRect(0.3, -0.5, 0.2, 1.0);
    else
        painter.fillRect(0.3, -0.5, 0.2, 1.0, painter.brush());
}

\: g2RgbGlyph (void; bool outline)
{
    let painter = commands.mainViewPainter();
    if (painter eq nil) return;
    
    painter.save();
    painter.setPen(qt.QPen(qt.QColor(255, 0, 0)));
    painter.setBrush(qt.QBrush(qt.QColor(255, 0, 0)));
    g2DrawCircleFan(0, 0, 0.5, 0.0, 0.33, .3, outline);
    
    painter.setPen(qt.QPen(qt.QColor(0, 255, 0)));
    painter.setBrush(qt.QBrush(qt.QColor(0, 255, 0)));
    g2DrawCircleFan(0, 0, 0.5, 0.33, 0.66, .3, outline);
    
    painter.setPen(qt.QPen(qt.QColor(0, 0, 255)));
    painter.setBrush(qt.QBrush(qt.QColor(0, 0, 255)));
    g2DrawCircleFan(0, 0, 0.5, 0.66, 1.0, .3, outline);
    painter.restore();
}

\: g2DrawXGlyph (void; bool outline)
{
    g2DrawCloseButton(0, 0, 0.75, Color(.3,.2,0,1), Color(1,.6,0,1));
}

\: g2DrawStopGlyph (void; bool outline)
{
    let painter = commands.mainViewPainter();
    if (painter eq nil) return;
    
    painter.save();
    painter.setRenderHint(qt.QPainter.Antialiasing, true);
    
    // Draw filled black circle
    painter.setPen(qt.QPen(qt.QColor(0, 0, 0)));
    painter.setBrush(qt.QBrush(qt.QColor(0, 0, 0)));
    painter.drawEllipse(qt.QPointF(0.0, 0.0), 0.75, 0.75);
    
    // Draw horizontal line (stop symbol)
    let pen = qt.QPen(qt.QColor(255, 255, 255, 179)); // 0.7 * 255
    pen.setWidthF(3.0);
    painter.setPen(pen);
    painter.drawLine(-0.5, 0.0, 0.5, 0.0);
    
    painter.restore();
}

\: g2InfoGlyph (void; bool outline)
{
    let painter = commands.mainViewPainter();
    if (painter eq nil) return;
    
    painter.save();
    painter.setRenderHint(qt.QPainter.Antialiasing, true);
    
    // Draw filled black circle (background)
    painter.setPen(qt.QPen(qt.QColor(0, 0, 0)));
    painter.setBrush(qt.QBrush(qt.QColor(0, 0, 0)));
    painter.drawEllipse(qt.QPointF(0.0, 0.0), 0.5, 0.5);
    
    // Draw outline if requested
    if (outline)
    {
        let pen = qt.QPen(qt.QColor(128, 128, 128));
        pen.setWidthF(2.5);
        painter.setPen(pen);
        painter.setBrush(qt.QBrush());
        painter.drawEllipse(qt.QPointF(0.0, 0.0), 0.5, 0.5);
    }
    
    // Draw "i" character in light gray/white
    painter.setPen(qt.QPen(qt.QColor(217, 217, 217))); // 0.85 * 255
    painter.setBrush(qt.QBrush(qt.QColor(217, 217, 217)));
    
    // Dot on top of "i"
    painter.drawEllipse(qt.QPointF(0.0, 0.18), 0.06, 0.06);
    
    // Vertical line (stem of "i")
    let pen = qt.QPen(qt.QColor(217, 217, 217));
    pen.setWidthF(2.5);
    painter.setPen(pen);
    painter.drawLine(0.0, 0.02, 0.0, -0.25);
    
    painter.restore();
}

\: g2CircleLerpGlyph (void; float start, bool outline)
{
    let painter = commands.mainViewPainter();
    if (painter eq nil) return;
    
    painter.save();
    painter.setRenderHint(qt.QPainter.Antialiasing, true);
    
    let ch = 1.0;
    for (float q = ch; q >= start; q *= 0.9)
    {
        let a = math.cbrt(1.0 - q/ch);
        let qc = qt.QColor(int(0.2 * a * 255), int(1.0 * a * 255), int(1.0 * a * 255));
        painter.setPen(qt.QPen(qc));
        painter.setBrush(qt.QBrush());
        painter.drawEllipse(qt.QPointF(0.0, 0.0), q, q);
    }
    painter.restore();
}

\: g2TformCircle (void; bool outline)
{
    let painter = commands.mainViewPainter();
    if (painter eq nil) return;
    
    painter.save();
    painter.scale(0.2333, 0.2333);
    g2CircleGlyph(outline);
    painter.restore();
}

\: g2TformTriangle (void; float angle, bool outline)
{
    let painter = commands.mainViewPainter();
    if (painter eq nil) return;
    
    painter.save();
    painter.rotate(angle);
    painter.scale(0.25, 0.25);
    painter.translate(-1.3, 0.0);
    g2TriangleGlyph(outline);
    painter.restore();
}

\: g2TranslateIconGlyph (void; bool outline)
{
    g2TformCircle(outline);
    
    for (float a = 0.0; a <= 360.0; a += 90.0)
    {
        g2TformTriangle(a, outline);
    }
}

\: g2TranslateXIconGlyph (void; bool outline)
{
    g2TformCircle(outline);
    
    for (float a = 90.0; a <= 360.0; a += 180.0)
    {
        g2TformTriangle(a, outline);
    }
}

\: g2TranslateYIconGlyph (void; bool outline)
{
    g2TformCircle(outline);
    
    for (float a = 0.0; a <= 360.0; a += 180.0)
    {
        g2TformTriangle(a, outline);
    }
}

\: g2Lower_bounds (int; int[] array, int n)
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

\: g2DrawTextWithCartouche (BBox; float x, float y, string text,
                          float textsize, Color textcolor,
                          Color bgcolor,
                          Glyph g = nil,
                          Color gcolor = Color(.2,.2,.2,1),
                          float[] textSizes = nil)
{
    let painter = commands.mainViewPainter();
    if (painter eq nil) return BBox(0, 0, 0, 0);
    
    painter.save();
    painter.setRenderHint(qt.QPainter.Antialiasing, true);
    painter.setRenderHint(qt.QPainter.TextAntialiasing, true);
    
    // Auto-detect multiline text
    let isMultiline = string.split(text, "\n").size() > 1;
    
    // Set font for bounds calculation
    let font = qt.QFont("Arial", int(textsize));
    painter.setFont(font);
    
    // Add glyph spacing to text
    if (g neq nil)
    {
        if (isMultiline)
        {
            let lines = string.split(text, "\n");
            string spacedText = "";
            for_index (i; lines)
            {
                if (i > 0) spacedText += "\n";
                spacedText += "   " + lines[i];
            }
            text = spacedText;
        }
        else
        {
            text = "   " + text;
        }
    }
    
    // Calculate rough text bounds (simplified)
    let w = textsize * text.size() * 0.6;
    let h = textsize * 1.2;
    let mx = textsize * 0.1;
    
    let x0 = x - mx,
        x1 = x + w + mx,
        y0 = y - textsize - mx,
        y1 = y + mx,
        rad = (y1 - y0) * 0.5,
        ymid = (y1 + y0) * 0.5;
    
    // Draw background rounded rectangle (simplified)
    painter.setPen(qt.QPen());
    painter.setBrush(qt.QBrush(toQColor(bgcolor)));
    painter.drawRoundedRect(x0, y0, x1 - x0, y1 - y0, rad, rad);
    
    // Draw outline
    let outlineQC = toQColor(bgcolor * 0.8);
    painter.setPen(qt.QPen(outlineQC));
    painter.drawRoundedRect(x0, y0, x1 - x0, y1 - y0, rad, rad);
    
    // Draw glyph if provided
    if (g neq nil)
    {
        let glyphOffset = if isMultiline then (mx * 6.0) else 0.0;
        let glyphX = x - glyphOffset;
        let glyphRad = if isMultiline then rad * 0.6 else rad;
        
        painter.setPen(qt.QPen(toQColor(gcolor)));
        painter.setBrush(qt.QBrush(toQColor(gcolor)));
        
        painter.save();
        painter.translate(glyphX, ymid);
        painter.scale(glyphRad, glyphRad);
        g(false);
        painter.restore();
        
        painter.save();
        painter.translate(glyphX, ymid);
        painter.scale(glyphRad, glyphRad);
        painter.setPen(qt.QPen(toQColor(gcolor * 0.8)));
        painter.setBrush(qt.QBrush());
        g(true);
        painter.restore();
    }
    
    // Draw text
    painter.setPen(qt.QPen(toQColor(textcolor)));
    painter.drawText(x, y, text);
    
    painter.restore();
    
    return BBox(x0 - rad, y0, x1 + rad, y1);
}

\: g2SetupProjection (void; float w, float h, bool vflip = false)
{
    // QPainter uses screen coordinates, no projection setup needed
    // This is a no-op for API compatibility
    ;
}

\: g2SetupProjectionFromEvent (void; Event event)
{
    // QPainter uses screen coordinates, no projection setup needed
    // This is a no-op for API compatibility
    ;
}

\: g2FitTextInBox (int; string text, int w, int h)
{
    // Simplified approximation
    if (text eq nil || text.size() == 0) 
    {
        throw exception("ERROR: g2FitTextInBox: empty string.");
    }
    
    // Simple estimate based on character count
    let charsPerLine = w / 10;
    let lines = (text.size() / charsPerLine) + 1;
    let textSize = h / (lines * 1.5);
    
    return int(textSize);
}

\: g2FitNameValuePairsInBox (int; 
                           StringPair[] pairs, 
                           int margin, int w, int h)
{
    if (pairs.size() == 0) 
    {
        throw exception("ERROR: g2FitNameValuePairsInBox: empty pairs.");
    }
    
    // Simple estimate
    let lines = pairs.size();
    let textSize = h / (lines * 1.5);
    
    return int(textSize);
}

\: g2DrawRoundedBox (void;
                   int x0, int y0,
                   int x1, int y1,
                   int m,
                   Color c,
                   Color oc)
{
    let painter = commands.mainViewPainter();
    if (painter eq nil) return;
    
    painter.save();
    painter.setRenderHint(qt.QPainter.Antialiasing, true);
    
    // Draw filled rounded rect
    painter.setPen(qt.QPen());
    painter.setBrush(qt.QBrush(toQColor(c)));
    painter.drawRoundedRect(float(x0), float(y0), float(x1 - x0), float(y1 - y0), float(m), float(m));
    
    // Draw outline
    let pen = qt.QPen(toQColor(oc));
    pen.setWidthF(3.0);
    painter.setPen(pen);
    painter.setBrush(qt.QBrush());
    painter.drawRoundedRect(float(x0), float(y0), float(x1 - x0), float(y1 - y0), float(m), float(m));
    
    painter.restore();
}

\: g2DrawDropRegions (int; 
                    int w,
                    int h,
                    int x,
                    int y,
                    int margin,
                    string[] descriptors)
{
    let painter = commands.mainViewPainter();
    if (painter eq nil) return -1;
    
    let m  = margins(),
        devicePixelRatio=devicePixelRatio(),
        bsize = (h - m[2] - m[3]) / descriptors.size(),
        inregion = -1;

    x *= devicePixelRatio;
    y *= devicePixelRatio;
    margin *= devicePixelRatio;

    painter.save();
    painter.setRenderHint(qt.QPainter.Antialiasing, true);
    painter.setRenderHint(qt.QPainter.TextAntialiasing, true);
    
    let textsize = 20.0 * devicePixelRatio;
    let font = qt.QFont("Arial", int(textsize));
    painter.setFont(font);

    for_index (i; descriptors)
    {
        let y0 = bsize * i + margin + m[3],
            x0 = m[0] + margin,
            y1 = bsize * (i+1) - margin + m[3],
            x1 = w - margin - m[1],
            t  = descriptors[i],
            tw = textsize * t.size() * 0.6, // Rough estimate
            active = y >= y0 && y <= y1,
            fg = if active then Color(1,1,1,1) else Color(.5, .5, .5, 1),
            bg = Color(0, 0, 0, .85);

        g2DrawRoundedBox(x0, y0, x1, y1, 10, bg, fg);

        if (active) inregion = i;

        painter.setPen(qt.QPen(toQColor(fg)));
        painter.drawText((x1 - x0 - tw) * .5 + x0, 
                        math_util.lerp(y0, y1, 0.5), t);
    }
    
    painter.restore();
    
    return inregion;
}

\: g2WrapValue(string; string value, int wrap)
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

\: g2NameValuePairBounds (NameValueBounds; (string,string)[] pairs, int margin)
{
    let painter = commands.mainViewPainter();
    
    float nw = 0.0;
    float vw = 0.0;
    float h = 0.0;
    float th = 20.0;  // Default line height
    
    if (painter neq nil)
    {
        // Estimate based on character count and font size
        // Approximate character width as 60% of font size
        let font = painter.font();
        let fontSize = float(font.pointSize());
        if (fontSize <= 0) fontSize = 12.0;  // Default if not set
        
        let charWidth = fontSize * 0.6;
        th = fontSize * 1.5;  // Line height
        
        for_each (pair; pairs)
        {
            let (name, value) = pair;
            let nameWidth = float(name.size()) * charWidth;
            let valueWidth = float(value.size()) * charWidth;
            
            nw = math.max(nw, nameWidth);
            vw = math.max(vw, valueWidth);
            h += th;
        }
    }
    else
    {
        // Fallback to simple approximation
        for_each (pair; pairs)
        {
            let (name, value) = pair;
            nw = math.max(nw, float(name.size()) * 8.0);
            vw = math.max(vw, float(value.size()) * 8.0);
            h += th;
        }
    }
    
    // Create bounds arrays (simplified - just width/height)
    let nbounds = float[4][]();
    let vbounds = float[4][]();
    
    for_each (pair; pairs)
    {
        let (name, value) = pair;
        let nameWidth = float(name.size()) * 8.0;
        let valueWidth = float(value.size()) * 8.0;
        nbounds.push_back(float[4] {0.0, 0.0, nameWidth, th});
        vbounds.push_back(float[4] {0.0, 0.0, valueWidth, th});
    }
    
    let x0 = 0.0;
    let x1 = nw + vw;
    let y0 = -float(margin);
    let y1 = float(margin) + h;

    return (Vec2(x1-x0, y1-y0), nbounds, vbounds, int(nw));
}

\: g2ExpandNameValuePairs (StringPair[]; StringPair[] pairs, int wrap=0)
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
        if (wrap > 0 && value.size() > wrap) value = g2WrapValue(value, wrap);
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

    return newPairs;
}

\: g2DrawNameValuePairs (NameValueBounds;
                       StringPair[] pairs,
                       Color fg, Color bg,
                       int x, int y, int margin,
                       int maxw=0, int maxh=0,
                       int minw=0, int minh=0,
                       bool nobox=false)
{
    let painter = commands.mainViewPainter();
    if (painter eq nil) return (Vec2(0,0), float[4][](), float[4][](), 0);
    painter.save();
    painter.setRenderHint(qt.QPainter.Antialiasing, true);
    painter.setRenderHint(qt.QPainter.TextAntialiasing, true);
    
    let m = margin;
    let (tbox, nbounds, vbounds, nw) = g2NameValuePairBounds(pairs, m);
    
    let th = 20.0;

    // Input y is bottom-origin (OpenGL style), convert to screen coords
    float x0 = x,
          y0 = y - m,
          x1 = tbox.x + x0,
          y1 = tbox.y + y0;

    let xs = x1 - x0,
        ys = y1 - y0;

    if (minw > 0 && xs < minw) x1 = x0 + minw;
    if (minh > 0 && ys < minh) y1 = y0 + minh;
    if (maxw > 0 && xs > maxw ) x1 = x0 + maxw;
    if (maxh > 0 && ys > maxh ) y1 = y0 + maxh;

    tbox.x = x1 - x0;
    tbox.y = y1 - y0;

    // Transform box coordinates from bottom-origin to top-origin
    let ty0 = transformY(y0);
    let ty1 = transformY(y1);
    
    // Ensure ty0 < ty1 for QPainter (top-left origin)
    if (ty0 > ty1)
    {
        let temp = ty0;
        ty0 = ty1;
        ty1 = temp;
    }

    if (!nobox) g2DrawRoundedBox(int(x0), int(ty0), int(x1), int(ty1), m, bg, fg * Color(.5,.5,.5,.5));

    // Draw separator line
    let sepColor = fg * Color(1,1,1,.25);
    painter.setPen(qt.QPen(toQColor(sepColor)));
    painter.drawLine(x + nw + m/4, ty0 + m/2, x + nw + m/4, ty1 - m/2);

    // Draw text lines from bottom going up (OpenGL coords), transform each
    float currentY = y;
    for_index (i; pairs)
    {
        let (n, v)  = pairs[i];
        let textY = transformY(currentY);

        // Draw name (left-aligned for now - need QFontMetrics for right-align)
        let nameColor = fg - Color(0,0,0,.25);
        painter.setPen(qt.QPen(toQColor(nameColor)));
        painter.drawText(float(x) + float(m)/2.0, textY, n);
        
        // Draw value (left-aligned at start of value column)
        painter.setPen(qt.QPen(toQColor(fg)));
        painter.drawText(float(x) + float(nw) + float(m)/2.0, textY, v);
        
        currentY += th;  // Move up in OpenGL coords
    }

    painter.restore();
    
    return (tbox, nbounds, vbounds, nw);
}

\: g2DrawCloseButton (void; float x, float y, float radius,
                    Color bg, Color fg)
{
    let painter = commands.mainViewPainter();
    if (painter eq nil) return;
    
    painter.save();
    painter.setRenderHint(qt.QPainter.Antialiasing, true);
    
    let r2 = radius / 2;
    
    // Draw filled circle
    painter.setPen(qt.QPen());
    painter.setBrush(qt.QBrush(toQColor(bg)));
    painter.drawEllipse(qt.QPointF(x, y), radius, radius);
    
    // Draw outline
    let pen = qt.QPen(toQColor(fg));
    pen.setWidthF(2.0);
    painter.setPen(pen);
    painter.setBrush(qt.QBrush());
    painter.drawEllipse(qt.QPointF(x, y), radius, radius);
    
    // Draw X
    painter.drawLine(x - r2, y - r2, x + r2, y + r2);
    painter.drawLine(x - r2, y + r2, x + r2, y - r2);
    
    painter.restore();
}

\: g2Draw (void; Glyph g, float x, float y, float angle, float size, bool outline)
{
    let painter = commands.mainViewPainter();
    if (painter eq nil) return;
    
    painter.save();
    painter.translate(x, y);
    painter.rotate(angle);
    painter.scale(size, size);
    g(outline);
    painter.restore();
}

}   // END MODULE glyph2
