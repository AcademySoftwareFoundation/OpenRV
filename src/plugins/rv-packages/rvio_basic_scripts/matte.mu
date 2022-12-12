//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
use glyph;
use gl;
use gltext;
use rvtypes;
use commands;

documentation: """
This is an rvio overlay script. Overlay scripts are Mu modules which
contain a function called main() with specific arguments which rvio
will supply. This script is not used by rv.

Draws a centered matte on the output imagery. The script takes two
arguments: the aspect ratio of the input imagery (in case its column
boxed) and an opacity.

----
  rvio in.#.jpg -o out.mov -overlay matte 2.35 0.8
----

The above example will render a slightly opaque 2.35 matte over the
imagery.
""";

module: matte
{
    documentation: "See module documentation.";

    \: main (void; int w, int h, 
             int tx, int ty,
             int tw, int th,
             bool stereo,
             bool rightEye,
             int frame,
             [string] argv)
    {
        let _ : inaspect : op : _ = argv,
            aspect                = float(inaspect),
            a                     = float(w) / float(h),
            y                     = (1.0 - a / aspect) / 2.0;

        gltext.size(18.0);

        if (a < aspect)
        {
            setupProjection(w, h);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glColor(Color(0,0,0,float(op)));

            glBegin(GL_QUADS);
            glVertex(0, 0);
            glVertex(w, 0);
            glVertex(w, y * h);
            glVertex(0, y * h);

            glVertex(0, (1.0-y) * h);
            glVertex(w, (1.0-y) * h);
            glVertex(w, h);
            glVertex(0, h);
            glEnd();

            let b = gltext.bounds(inaspect),
                sh = b[1] + b[3],
                margin = 6;

            gltext.color(Color(.2,.2,.2,float(op)));
            gltext.writeAt(margin, y * h - sh - margin, inaspect);
        }
        else if (a > aspect)
        {
	          let inset = (w-(h*float(aspect)))/2.0;

            setupProjection(w, h);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glColor(Color(0,0,0,float(op)));

            glBegin(GL_QUADS);
            // Left
            glVertex(0, 0);
            glVertex(inset, 0);
            glVertex(inset, h);
            glVertex(0, h);

            // Right
            glVertex(w-inset, 0);
            glVertex(w, 0);
            glVertex(w, h);
            glVertex(w-inset, h);

            glEnd();
            let b = gltext.bounds(inaspect),
                sh = b[1] + b[3],
                margin = 6;

            gltext.color(Color(.2,.2,.2,float(op)));
            gltext.writeAt(margin - y, h - sh - margin, inaspect);
        }
    }
}
