//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//

documentation: """
This module programmatically creates a channel or 3D LUT by sampling a
function of type (Vec3; Vec3) i.e. RGB -> RGB. This function is known as a
TransformFunc. A few such functions are provided to various hacky things
with color.
"""

module: lutgen {

use rvtypes;
use commands;
use extra_commands;

documentation: """
TransformFunc is a function which maps color to a new color in 0,1
float space. The output and input do not need to be constrained to
[0,1] in the case of HDR LUT.
""";

TransformFunc := (Vec3; Vec3);


documentation: "Invert the channels pivot at 0.5. This is useful for testing.";

\: invertFunc (Vec3; Vec3 incolor) { incolor * -1.0 + Vec3(1.0, 1.0, 1.0); }

documentation: "Identity. This is useful for testing.";

\: identFunc (Vec3; Vec3 incolor) { incolor; }

documentation: "Lin -> Log black + white points TransformFunc";

\: applyLinearToKodakLogFunc (Vec3; 
                              Vec3 incolor,
                              double cinblack,
                              double cinwbdiff) 
{
    Vec3( math.log10((cinblack / cinwbdiff + incolor[0]) * cinwbdiff) / double(3.41),
          math.log10((cinblack / cinwbdiff + incolor[1]) * cinwbdiff) / double(3.41),
          math.log10((cinblack / cinwbdiff + incolor[2]) * cinwbdiff) / double(3.41) );
}

documentation: "Log -> Lin black + white points TransformFunc";

\: applyKodakLogToLinearFunc (Vec3; 
                              Vec3 incolor,
                              double cinblack,
                              double cinwbdiff) 
{
    Vec3 ( (math.pow(double(10.0), incolor[0] * double(3.41)) - cinblack) / cinwbdiff,
           (math.pow(double(10.0), incolor[1] * double(3.41)) - cinblack) / cinwbdiff,
           (math.pow(double(10.0), incolor[2] * double(3.41)) - cinblack) / cinwbdiff );
}

documentation: """
Called by newChannelLUT() and newPreLUT() to do the bulk of the work.
""";

\: newChannelOrPreLUT (void; 
                       string lutNodeTypeOrName,
                       string name,
                       bool asPreLUT,
                       TransformFunc F,
                       float low = 0.0,
                       float high = 1.0,
                       int resolution = 2048)
{
    let LUTProp    = "%s.lut.lut" % lutNodeTypeOrName,
        preLUTProp = "%s.lut.prelut" % lutNodeTypeOrName,
        matrixProp = "%s.lut.inMatrix" % lutNodeTypeOrName,
        typeProp   = "%s.lut.type" % lutNodeTypeOrName,
        nameProp   = "%s.lut.name" % lutNodeTypeOrName,
        fileProp   = "%s.lut.file" % lutNodeTypeOrName,
        sizeProp   = "%s.lut.size" % lutNodeTypeOrName,
        activeProp = "%s.lut.active" % lutNodeTypeOrName,
        range      = high - low,
        invrange   = 1.0 / range,
        maxu       = float(resolution - 1),
        step       = range / maxu,
        lut        = float[]();

    for (int i = 0; i < resolution; i++)
    {
        let u = float(i) / maxu * range + low,
            r = F(Vec3(u, 0, 0))[0],
            g = F(Vec3(0, u, 0))[1],
            b = F(Vec3(0, 0, u))[2];

        lut.push_back(r);
        lut.push_back(g);
        lut.push_back(b);
    }

    set(matrixProp, float[] {invrange, 0, 0, low,
                             0, invrange, 0, low,
                             0, 0, invrange, low,
                             0, 0, 0, 1});

    if (!asPreLUT)
    {
        set(LUTProp, lut);
        set(preLUTProp, float[] {});
        set(typeProp, "Channel");
        set(nameProp, name);
        set(fileProp, string[] {});
        set(sizeProp, resolution);
    }
    else
    {
        set(preLUTProp, lut);
    }

    set(activeProp, 1);
    redraw();
}



documentation: """
Create and install a new Channel LUT from the input TransformFunc. In
order to do so, a bounding box for the input data must be provided
along with a resolution for the LUT. 

This function will install the LUT on the node of the given type:
#RVLookLUT, #RVLinearize (file LUT), #RVPreCacheLUT, or #RVDisplayColor
(display LUT).

The resolution parameter indicates is indicates the sampling. For channel
LUTs try 512, 1024, 2048. Some cards can handle 4096.
""";

\: newChannelLUT(void;
                 string lutNodeTypeOrName,
                 string name,
                 TransformFunc F,
                 float low = 0.0,
                 float high = 1.0,
                 int resolution = 2048)
{
    newChannelOrPreLUT(lutNodeTypeOrName, name, false, F, low, high, resolution);
}



documentation: """
Create and install a new Pre-LUT on an existing LUT from the input
TransformFunc. In order to do so, a bounding box for the input data
must be provided along with a resolution for the LUT. The resulting
input range becomes the input range of the LUT.

This function will install the LUT on the node of the given type:
#RVLookLUT, #RVLinearize (file LUT), #RVPreCacheLUT, or #RVDisplayColor
(display LUT).

The resolution parameter indicates is indicates the sampling. For channel
LUTs try 512, 1024, 2048. Some cards can handle 4096.
""";

\: newPreLUT(void;
             string lutNodeTypeOrName,
             string name,
             TransformFunc F,
             float low = 0.0,
             float high = 1.0,
             int resolution = 2048)
{
    newChannelOrPreLUT(lutNodeTypeOrName, name, true, F, low, high, resolution);
}

documentation: """
Create and install a new 3D LUT from the input TransformFunc. In order to
do so, a bounding box for the input data must be provided along with a
resolution for the LUT. 

This function will install the LUT on the node of the given type:
#RVLookLUT, #RVLinearize (file LUT), #RVPreCacheLUT, or #RVDisplayColor
(display LUT).

The resolution parameter indicates is indicates the sampling. For 3D LUTs
try 16, 32, 64. Some cards can handle more. This value should be a power of
two for best results.
""";

\: new3DLUT (void; 
             string lutNodeTypeOrName,
             string name,
             TransformFunc F,
             Vec3 low = Vec3(0),
             Vec3 high = Vec3(1),
             int resolution = 32)
{
    let LUTProp    = "%s.lut.lut" % lutNodeTypeOrName,
        preLUTProp = "%s.lut.prelut" % lutNodeTypeOrName,
        matrixProp = "%s.lut.inMatrix" % lutNodeTypeOrName,
        typeProp   = "%s.lut.type" % lutNodeTypeOrName,
        nameProp   = "%s.lut.name" % lutNodeTypeOrName,
        fileProp   = "%s.lut.file" % lutNodeTypeOrName,
        sizeProp   = "%s.lut.size" % lutNodeTypeOrName,
        activeProp = "%s.lut.active" % lutNodeTypeOrName,
        range      = high - low,
        invrange   = Vec3(1) / range,
        maxu       = float(resolution - 1),
        step       = range / maxu,
        lut        = float[]();

    for (int z = 0; z < resolution; z++)
    {
        let zu = (float(z) / maxu) * range.z + low.z;

        for (int y = 0; y < resolution; y++)
        {
            let yu = (float(y) / maxu) * range.y + low.y;

            for (int x = 0; x < resolution; x++)
            {
                let xu = (float(x) / maxu) * range.x + low.x,
                     c = F(Vec3(xu, yu, zu));

                lut.push_back(c.x);
                lut.push_back(c.y);
                lut.push_back(c.z);
            }
        }
    }

    set(matrixProp, float[] {invrange.x, 0, 0, low.x,
                             0, invrange.y, 0, low.y,
                             0, 0, invrange.z, low.z,
                             0, 0, 0, 1});
    set(LUTProp, lut);
    set(preLUTProp, float[] {});
    set(typeProp, "3D");
    set(nameProp, name);
    set(fileProp, string[] {});
    set(sizeProp, int[] {resolution, resolution, resolution});
    set(activeProp, 1);

    redraw();
}

documentation: """
Install a 2k channel LUT which transforms from linear [0, cinmax] to Kodak
log [0, 1] where the output values represent normalized code values. 

E.g, converting EXR data into log space for use with known Log -> Film LUTs
later in the pipe. In this case you could apply this as a file LUT using
#RVLinearize as the lutNodeTypeOrName and use film LUT as a display LUT or look
LUT. 

Another use is to apply this as a pre-cache LUT on float/half input and
convert to 8 bit int for caching. Then apply the log->lin color shader to
expand the 8 bit log data back to its full range. This can double the
amount of data storable in the cache.

NOTE: if the pixels are in log space, color corrections will not behave as
one might expect.

You can specify the white and black codes as 10 bit values.
""";

\: linearToKodakLog (void; 
                     string lutNodeTypeOrName,
                     int whiteCode = 685,
                     int blackCode = 95)
{
    let cinblack = math.pow(double(10.0), blackCode * double(0.002) / 0.6),
        cinwhite = math.pow(double(10.0), whiteCode * double(0.002) / 0.6),
        cinwbdiff = cinwhite - cinblack,
        maxLinear = applyKodakLogToLinearFunc(Vec3(1.0), cinblack, cinwbdiff)[0];

    newChannelLUT(lutNodeTypeOrName,
                  "Linear -> Kodak Log (white=%d, black=%d)" % (whiteCode, blackCode),
                  applyLinearToKodakLogFunc(,cinblack,cinwbdiff),
                  0, maxLinear, 2048);
}

documentation: """
Install a 2k channel Pre-LUT which transforms from linear [0, cinmax] to Kodak
log [0, 1] where the output values represent normalized code values. 

E.g, converting EXR data into log space for use with known Log -> Film LUTs
later in the pipe. In this case you could apply this as a file LUT using
#RVLinearize as the lutNodeTypeOrName and use film LUT as a display LUT or look
LUT. 

You can specify the white and black codes as 10 bit values.
""";

\: linearToKodakLogPreLUT (void; 
                           string lutNodeTypeOrName,
                           int whiteCode = 685,
                           int blackCode = 95)
{
    let cinblack = math.pow(double(10.0), blackCode * double(0.002) / 0.6),
        cinwhite = math.pow(double(10.0), whiteCode * double(0.002) / 0.6),
        cinwbdiff = cinwhite - cinblack,
        maxLinear = applyKodakLogToLinearFunc(Vec3(1.0), cinblack, cinwbdiff)[0];

    newPreLUT(lutNodeTypeOrName,
              "Linear -> Kodak Log (white=%d, black=%d)" % (whiteCode, blackCode),
              applyLinearToKodakLogFunc(,cinblack,cinwbdiff),
              0, maxLinear, 2048);
}

documentation: """
Install a 2k channel LUT which transforms from Kodak log [0, 1.0] to 
linear [0, ~13.5]. Similar in spirit to the built in log->lin shader.
You can specify the white and black codes as 10 bit values.
""";

\: kodakLogToLinear (void; 
                     string lutNodeTypeOrName,
                     int whiteCode = 685,
                     int blackCode = 95)
{
    let cinblack = math.pow(double(10.0), blackCode * double(0.002) / 0.6),
        cinwhite = math.pow(double(10.0), whiteCode * double(0.002) / 0.6),
        cinwbdiff = cinwhite - cinblack,
        cinbwdiff = cinblack - cinwhite;

    newChannelLUT(lutNodeTypeOrName,
                  "Kodak Log -> Linear (white=%d, black=%d)" % (whiteCode, blackCode),
                  applyKodakLogToLinearFunc(,cinblack,cinwbdiff),
                  0, 1, 2048);
}

documentation: """
Creates an identity channel LUT at then replaces its input matrix with
the given one. This makes it possible to put an exact color matrix
alone without needing LUT interpolation.
"""

\: newMatrixAtLUT (void;
                   string lutNodeTypeOrName,
                   float[] M)
{
    let matrixProp = "%s.lut.inMatrix" % lutNodeTypeOrName;

    new3DLUT(lutNodeTypeOrName,
             "Identity with a custom color matrix",
             identFunc,
             Vec3(0), Vec3(1), 4);

    set(matrixProp, M);
    redraw();
}

documentation: """
Install a matrix which converts from 'RGB601'->RGB709. Occasionally
there is a codec bug (ffmpeg DNxHD) or user error which results in an
image that is incorrectly interpreted as YUV709 when its really YUV601
video with under blacks and over whites.

This matrix will expand out the data including the under and over
values. 
""";

\: convert_RGB601_RGB709 (void; string lutNodeTypeOrName)
{
    float[] M = // this is "RGB601" to RGB709
        { 1.14616407634,  0.015256546817, 2.962938484e-3,  -0.076188825019 ,
          7.771111269e-3,  1.1536496483,  2.9628020709e-3, -0.070697145168 ,
          7.771219199e-3, 0.015256546813,  1.14135579562,  -0.07701471787  ,
          0.,             0.,             0.,              1.        };

    newMatrixAtLUT(lutNodeTypeOrName, M);
}
                     

}
