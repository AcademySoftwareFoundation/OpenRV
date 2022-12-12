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

Draws a water mark centered on the image. The script takes two
arguments: the string to render as a watermark and the opacity of the
watermark. The font size is determined by the bounding box of the
characters in the input string. So large strings are scaled down to
fit into the image.

----
  rvio in.#.jpg -o out.mov -overlay watermark "FilmCo Eyes Only" .25
----

The above example will render a water mark which says "FilmCo Eyes Only" 
centered across the input imagery. 
""";

module: watermark
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
        let _ : text : op : _ = argv;
        let a = float(w) / float(h);

        setupProjection(w, h);

        int textSize = 10;
        int width = 0;

        do
        {
            textSize += 5;
            gltext.size(textSize);
            let b = gltext.bounds(text);
            width = b[0] + b[2];
        }
        while (width < w - 100);

        gltext.size(textSize - 10);
        let b = gltext.bounds(text);
        width = b[0] + b[2];
        let height = b[1] + b[3];

        glEnable(GL_LINE_SMOOTH);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        gltext.color(Color(1,1,1,float(op)));
        gltext.writeAt((w - width)/2, (h - height) / 2, text);

        //for_each (s; sourcesRendered())
        //{
            //for_each (a; sourceAttributes(s.name))
            //{
                //print("%+ 20s = %s\n" % a);
            //}
        //}
    }
}
