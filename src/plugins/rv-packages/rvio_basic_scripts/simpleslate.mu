//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
use glyph;
use gl;
use gltext;
use gltexture;
use image;
use rvtypes;
use commands;
use io;

documentation: """
This is an rvio leader script. A leader script has a main() function
which is called to render a complete frame for output.

The simpleslate script creates a slate. 

Most companies have some type of environment set up which describes 
the current shot, show, etc. So normally, you'd want the slate function 
to get values from there or a database somehow

In this case, we're going to pass all the information in the argv
list.  the arguments will take the form:

----
    name=value
----

So for example: "Artist=John Doe" "Show=Blockbuster" "Shot=BestShot-01"
will render each piece of data on its own line.  For multiple values
(like multiple Artists) you can add more =values:

----
    "Artists=John Doe=Jane Doe"
----

This code calls some RV ui functions to format the results like the
image info HUD widget.

You can optoinally pass a path to a tif file to use as the background
of the slate if you do not wish to use the default color bars. This has
been tested with 32-bit float tifs.

Example usage with default background:

----
rvio in.#.jpg -o out.mov -leader simpleslate "FilmCo" \
              "Artist=Jane Q. Artiste" "Shot=S01" "Show=BlockBuster" \
              "Comments=You said it was too blue so I made it red"
----

Example usage with custom background:

----
rvio in.#.jpg -o out.mov -leader simpleslate "FilmCo" bg.tif \
              "Artist=Jane Q. Artiste" "Shot=S01" "Show=BlockBuster" \
              "Comments=You said it was too blue so I made it red"
----

"""

module: simpleslate
{
    global byte[32,32] halftone_bitmap = {
        0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55, 
        0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55, 
        0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55, 
        0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55, 
        0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55, 
        0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55, 
        0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55, 
        0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55, 
        0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55, 
        0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55, 
        0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55, 
        0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55, 
        0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55, 
        0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55, 
        0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55, 
        0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55 };

    global int bgID = -1;

    \: init (void; int w, int h, 
         int tx, int ty,
         int tw, int th,
         bool stereo,
         bool rightEye,
         int frame,
         [string] argv)
    {
        //
        // Convert the argv list to an options array
        //

        string[] options = string[]();
        let _ : args = argv;
        while (args neq nil)
        {
            let input : rest = args;
            options.push_back(input);
            args = rest;
        }

        //
        // Look for bg path and create a texture
        //

        if (options.size() < 2) return;
        string tif = options[1];
        if (!path.exists(options[1])) return;
        let logo = image(tif),
            texid = createScalable2DTexture(logo);
        bgID = texid;
    }

    \: drawClapboardStripes (void; int polytype, int n, int sw, int h)
    {
        for (int i = -1; i < n; i++)
        {
            float sc = ((i+1) % 2) * .3 + .1;
            glColor(Color(sc, sc, sc, 1));
            
            let hn = h / n,
                h0 = hn * i,
                h1 = hn * (i+1),
                h2 = hn * (i+2);

            glBegin(polytype);
            glVertex(0, h0);
            glVertex(sw, h1);
            glVertex(sw, h2);
            glVertex(0, h1);
            glEnd();
        }
    }

    \: drawDefaultBG (void; int n, int sw, int w, int h)
    {
        glEnable(GL_BLEND);
        glEnable(GL_LINE_SMOOTH);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        drawClapboardStripes(GL_POLYGON, n, sw, h);
        drawClapboardStripes(GL_LINE_LOOP, n, sw, h);

        //
        //  Draw some ramps and colors for gamma purposes
        //

        glColor(Color(0,0,0,1));

        glBegin(GL_POLYGON);
        glVertex(w - 20, 0);
        glVertex(w, 0);
        glColor(Color(1,1,1,1));
        glVertex(w, h);
        glVertex(w - 20, h);
        glEnd();

        glDisable(GL_BLEND);

        glColor(Color(0,0,0,1));
        glBegin(GL_POLYGON);
        glVertex(w - 30, 0); glVertex(w - 20, 0);
        glVertex(w - 20, h); glVertex(w - 30, h);
        glEnd();

        glEnable(GL_POLYGON_STIPPLE);
        glPolygonStipple(halftone_bitmap);

        glColor(Color(1,1,1,1));
        glBegin(GL_POLYGON);
        glVertex(w - 30, 0); glVertex(w - 20, 0);
        glVertex(w - 20, h); glVertex(w - 30, h);
        glEnd();

        glDisable(GL_POLYGON_STIPPLE);

        gltext.size(10);
        gltext.color(.5, .5, .5, 1);

        for_each (gamma; [2.2, 1.8, 1.0])
        {
            let gh = math.pow(.5, 1.0 / gamma);
            gltext.writeAt(w-60, h * gh - 3, "%1.1f" % gamma);

            glColor(Color(.5,.5,.5,1));
            glBegin(GL_LINES);
            glVertex(w-45, h * gh); 
            glVertex(w-30, h * gh);
            glEnd();
        }
        
        Color[] colors = 
        {
            {0, 0, 0, 1},
            {1, 1, 1, 1},
            {1, 1, 0, 1},
            {1, 0, 1, 1},
            {0, 1, 1, 1},
            {0, 0, 1, 1},
            {0, 1, 0, 1},
            {1, 0 ,0, 1},
        };

        for_index (i; colors)
        {
            glColor(colors[i]);
            glBegin(GL_POLYGON);
            glVertex(w - 40 - i * 20 - 40, h - 20);
            glVertex(w - 20 - i * 20 - 40, h - 20);
            glVertex(w - 20 - i * 20 - 40, h);
            glVertex(w - 40 - i * 20 - 40, h);
            glEnd();
        }
    }

    \: main (void; int w, int h, 
             int tx, int ty,
             int tw, int th,
             bool stereo,
             bool rightEye,
             int frame,
             [string] argv)
    {
        gltext.init(); // default font
        // note: you could do gltext.init("/path/to/font.ttf") for 
        // a custom font.

	glViewport(0, 0, w, h);

        \: reverse ((string,string)[] s)
        {
            (string,string)[] n;
            for (int i=s.size()-1; i >= 0; i--) n.push_back(s[i]);
            n;
        }

        //
        //  This uses a list pattern match to get the arguments.
        //
        //  ignore the fist list element (its always "simpleslate")
        //  this is done using the _ pattern which discards that place.
        //
        //  the second is the text to draw sideways.
        //
        //  if the third is a path to a file on disk skip it, because it is
        //  the background.
        //
        //  the rest are name=val lines. 
        //

        string sidetext = nil;
        [string] lines = nil;
        let _ : theRest = argv,
            m = w / 24;

        if (theRest neq nil) 
        {
            let s : l = theRest;
            sidetext = s;
            lines = l;
        }

        StringPair[] pairs;

        bool first = true;
        for_each (arg; lines)
        {
            let nv = string.split(arg, "=");

            if (nv.size() > 1)
            {
                for_index (i; nv)
                {
                    if (i == 1) pairs.push_back((nv[0], nv[i]));
                    else if (i > 1) pairs.push_back(("", nv[i]));
                }
            }
            else
            {
                let unpaired = nv.front();
                if (first && path.exists(unpaired)) continue;
                else pairs.push_back(("", unpaired));
            }
            first = false;
        }

        setupProjection(w, h);
        glClearColor(.18, .18, .18, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        //
        //  Draw the background 
        //

        let n = 10, sw = 50;
        if (bgID != -1) drawTexture(bgID, 0, 0, w, h, 1.0, true);
        else drawDefaultBG(n, sw, w, h);

        //
        //  Draw the rotated text
        //

        let bh = 0;
        if (sidetext neq nil)
        {
            let sidesize = math.min(fitTextInBox(sidetext, h * .8, 80), 80);

            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            glRotate(90, 0, 0, 1);
            gltext.size(sidesize);
            gltext.color(.5, .5, .6, 1);

            let b = gltext.bounds(sidetext),
                bw = b[0] + b[2];

            bh = b[1] + b[3];

            gltext.writeAt((h - bw) / 2,        // in rotated space
                        -(bh + 20) - sw, 
                        sidetext);
            glPopMatrix();
        }

        if (pairs.size() != 0)
        {
            let leftmargin = math.abs(-(bh + 20) - sw);

            //
            //  Text
            //

            int tsize = math.min(100, fitNameValuePairsInBox(pairs, 
                                                            m, 
                                                            w - leftmargin - 60 - m, 
                                                            h * .9));
            gltext.size(tsize);
            let drawPairs = reverse(pairs);
            let (tbox, _, _, _) = nameValuePairBounds(drawPairs, m);
            
            //
            //  Center it
            //

            let x = (w - leftmargin - tbox.x - m - 60) / 2 + leftmargin,
                y = (h - tbox.y) / 2;

            drawNameValuePairs(drawPairs, 
                            Color(.75,.75,.75,1),
                            Color(0,0,0,1),
                            x, y, m,
                            0, 0 ,0, 0, true);
        }
    }
}
