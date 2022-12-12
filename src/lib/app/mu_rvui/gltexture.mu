//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
use gl;
use glu;

module: gltexture
{
    use image;

    //
    //  The image module is overly simple right now. It can only load tif 
    //  files and only represents the pixels as a block of RGBA floats.
    //  By using these functions we can hopefully hide this fact a bit.
    //

    \: createScalable2DTexture (int; image im)
    {
        let tid = glGenTextures(1)[0];
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, tid);

        //
        //  Only newer nvidia cards will succcessfully use linear
        //  interp with 32 bit float textures. Be warned.
        //

        glTexParameter(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameter(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameter(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameter(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // byte[] pixels;
        // pixels.resize(im.data.size() * 4);

        // for_index (i; im.data)
        // {
        //     let p = im.data[i];
        //     pixels[i * 4 + 0] = byte(p.x * 255.0);
        //     pixels[i * 4 + 1] = byte(p.y * 255.0);
        //     pixels[i * 4 + 2] = byte(p.z * 255.0);
        //     pixels[i * 4 + 3] = byte(p.w * 255.0);
        // }

        gluBuild2DMipmaps(GL_TEXTURE_2D, 
                          GL_RGBA, 
                          im.width, im.height, 
                          GL_RGBA, 
                          im.data);
                          //pixels);
        return tid;
    }

    \: drawTexture (void; int texid,
                    float x, float y,
                    float w, float h,
                    float opacity,
                    bool flip=false,
                    bool flop=false)
    {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texid);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        glColor(1, 1, 1, opacity);
        glBegin(GL_QUADS);
        glTexCoord(if flop then 1.0 else 0.0, if flip then 1.0 else 0.0); glVertex(x, y, 0);
        glTexCoord(if flop then 1.0 else 0.0, if flip then 0.0 else 1.0); glVertex(x, y + h, 0);
        glTexCoord(if flop then 0.0 else 1.0, if flip then 0.0 else 1.0); glVertex(x + w, y + h, 0);
        glTexCoord(if flop then 0.0 else 1.0, if flip then 1.0 else 0.0); glVertex(x + w, y, 0);
        glEnd();
    }
}
