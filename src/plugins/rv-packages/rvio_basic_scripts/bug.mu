//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
use glyph;
use gl;
use glu;
use gltext;
use gltexture;
use rvtypes;
use commands;
use image;
use math;

documentation: """
This is an rvio overlay script. Overlay scripts are Mu modules which
contain a function called main() with specific arguments which rvio
will supply. This script is not used by rv.

This overlay script also contains an init() function which is called
only once before the first time main() is called. In this case, this
function is used to load a texture (.tiff file) only once.

This module draws a logo/bug image in the output imagery. The options to this module are as follows:

required:
logo_file_path - path to the square power-of-two tiff to be used as the bug/logo

optional:
logo_opacity - how transparent the logo is rendered (default opacity is 0.3)
logo_height - height to render the logo at into each frame (default is the height of the image NOTE: will be resized to nearest power of 2)
logo_x_location - horizontal position of the logo based on native coordinate system of source media (default 10 pixels in)
logo_y_location - veritical position of the logo based on native coordinate system of source media (default 10 pxels in)

----
  rvio in.#.jpg -o out.mov -overlay bug logo.tif 0.4 100 15 100
----

The above example will render a 40% opaque bug image 128 pixels tall 15 pixels into the frame and 100 pixels down the frame.
""";

module: bug
{
    //
    //  init() is called only once before the first call to main. 
    //  It gets the same arguments that main does, but you can do
    //  initialization here without worrying about when/how its called
    //
    //  In this case we'll load the image and make a texture
    //  out of it so we don't have to do that every frame.
    //

    global int texid;
    global float aspect;
    global float op;
    global int size;
    global int xloc;
    global int yloc;

    documentation: "See module documentation.";

    \: init (void; int w, int h, 
             int tx, int ty,
             int tw, int th,
             bool stereo,
             bool rightEye,
             int frame,
             [string] argv)
    {

        \: log2 (float; float x) { return log(x) / log(2); }

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
        // Check for the logo, opacity, size, x position, y position
        //

        if (options.size() < 1) print("ERROR: bug requires a path to the logo tiff file.\n");
        string tif = options[0];
        let logo = image(tif), filename = logo.name;

        if (options.size() > 1) op = float(options[1]);
        else                    op = 0.3;

        if (options.size() > 2) size = int(options[2]);
        else                    size = logo.height;

        if (options.size() > 3) xloc = int(options[3]);
        else                    xloc = 10;

        if (options.size() > 4) yloc = int(options[4]);
        else                    yloc = 10;

        aspect = float(logo.width) / float(logo.height);

        let msize = max(float(size), float(size) * aspect),
            n = int(log2(msize));

        //
        //  Resize the image to a power of 2 texture
        //

        if (logo.width != logo.height || 
            pow(2, n) != logo.width || 
            aspect != 1.0)
        {
            let x = log2(float(msize)),
                d = x - n,
                p = int(if d > 0.0 then pow(2, n+1) else pow(2, n));

            print("INFO: Resizing %s to %d x %d texture\n" % (filename, p, p));
            logo = resize(logo, p, p);
        }

        //
        //  Un premultiply the image data for GL
        //

        for (int y = 0; y < logo.height; y++)
        {
            for (int x = 0; x < logo.width; x++)
            {
                int index = y * logo.height + x;
                let p = logo.data[index];
                logo.data[index] = Color(p.x / p.w, p.y / p.w, p.z / p.w, p.w);
            }
        }

        texid = createScalable2DTexture(logo);
    }

    //
    //  main is called per-frame
    //

    documentation: "See module documentation.";

    \: main (void; int w, int h, 
             int tx, int ty,
             int tw, int th,
             bool stereo,
             bool rightEye,
             int frame,
             [string] argv)
    {
        setupProjection(w, h);

        let iw      = size,
            ih      = size,
            iy      = h - ih - yloc,
            ix      = xloc;

        drawTexture(texid, ix, iy, iw * aspect, ih, op, true);
    }
}
