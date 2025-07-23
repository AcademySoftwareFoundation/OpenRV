//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved. 
// 
// SPDX-License-Identifier: Apache-2.0 
//
//
//  Default English localized RV user interface
//

module: rvui {

use gl;
use glu;
require gltext;
require runtime;
require system;
require qt;
//require HUD;
use app_utils;
use io;
use commands;
use extra_commands;
use glyph;
use math;
use math_util;
use rvtypes;


global int preSnapRenderCount = 0;
global Vec2 snapTranslation;
global bool snapDoit = false;
global bool expandWidth = true;

global Configuration globalConfig =
    Configuration(nil,  // lastOpenDir
                  nil,  // lastLUTDir
                  Color(0.0, 0.0,  0.0, 0.75), // bg
                  Color(0.75, 0.75,  0.75, 1),    // fg
                  Color(0.4, 0.2,  0.0, 0.2),  // bgErr
                  Color(1.0, 0.75, 0.0, 1),    // fgErr
                  Color(0.5, 0.5, 0.5, 1.0),   // bgFeedback
                  Color(0.0, 0.0, 0.0, 1.0),   // fgFeedback
                  16.0,                        // text entry text size
                  12.0,                        // inspector text size
                  14.0,                        // info text size
                  0.5,                         // wipe delay sec
                  100.0,                       // wipe fade prox pixels
                  25.0,                        // wipe grab prox pixels
                  12.0,                        // wipe info text size
                  16.0,                        // moscope frame text size
                  20.0,                        // timeline frame text size
                  10.0,                        // timeline bounds text
                  Color(0.3, 0.3, 0.3, 1.0),   // timeline bounds
                  Color(0.52, 0.52, 0.52, 1.0),   // timeline range
                  Color(0.1, 0.7, 0.2, 1.0),   // timeline cache
                  Color(0.1, 0.3, 0.6, 1.0),   // timeline cache full
                  Color(0.01, 0.58, 0.85, 1.0),// timeline audio cache color
                  Color(0.55, 0.55, 0.55, 1.0),   // timeline in/out cap color
                  Color(0.65, 0.65, 1.0, 1.0),   // timeline mark color
                  Color(1.0, 0.25, 0.0, 1.0),  // timeline frame skip color
                  Color(0.0, 0.0,  0.0, 1.0),  // timeline frame skip text color
                  Color(0.0, 0.0,  0.0, 1.0),  // matte color
                  Color(0.42, 0.42,  0.42, 0.35),  // vcr bg
                  Color(0.3, 0.3,  0.3, 0.25),  // vcr fg
                  Color(0.0, 0.0, 0.0, .7),     // vcr timeline bg
                  Color(0.7, 0.7, 0.7, 0.5),    // vcr button bg
                  Color(0.7, 0.7, 0.7, 1.0),    // vcr button highlight
                  "viewPDF",                    // pdf viewer program from manuals
                  "viewHTML",                   // html viewer program from manuals
                  runtime.build_os(),
                  20,                           // box margin
                  nil,                          // menu creation func
                  nil, nil                      // render hooks
                  );

//----------------------------------------------------------------------
//
//  Utilities
//

\: makeStateFunc (MenuStateFunc; BoolFunc f)
{
    \: (int;) { if f() then CheckedMenuState else UncheckedMenuState; };
}

\: fillProperty (string name, float value, int channel = -1)
{
    try
    {
        let a = getFloatProperty(name);

        if (channel >= 0) a[channel] = value;
        else              for_index (i; a) a[i] = value;

        setFloatProperty(name, a);
    }
    catch (exception exc)
    {
        let sexc = string(exc);
        displayFeedback("Unable to fill property values: %s" % sexc);
    }
}

\: nudgeProperty (string name, float value, float delta, int channel = -1)
{
    try
    {
        let a = getFloatProperty(name);

        if (channel >= 0) a[channel] += delta;
        else for_index (i; a) a[i] += delta;

        setFloatProperty(name, a);
    }
    catch (exception exc)
    {
        let sexc = string(exc);
        displayFeedback("Unable to fill property values: %s" % sexc);
    }
}

//----------------------------------------------------------------------
//
//  Luminance LUT generator functions
//

\: toRGB (Vec3; Vec3 hsv)
{
    let s = hsv[1],
        h = hsv[0],
        v = hsv[2],
        r = 0.0,
        g = 0.0,
        b = 0.0;

    if (s == 0.0)
    {
        r = g = b = v;
    }
    else
    {
        if (h == 1.0) h = 0.0;
        float H = h * 6.0;
        int i   = int(math.floor(H));
        if (i > 5 || i < 0) i = 0;
        float f = H - i;
        float p = v * (1.0 - s);
        float q = v * (1.0 - (s * f));
        float t = v * (1.0 - (s * (1.0 - f)));

        if      (i == 0) { r = v; g = t; b = p; }
        else if (i == 1) { r = q; g = v; b = p; }
        else if (i == 2) { r = p; g = v; b = t; }
        else if (i == 3) { r = p; g = q; b = v; }
        else if (i == 4) { r = t; g = p; b = v; }
        else if (i == 5) { r = v; g = p; b = q; }
        else
        {
            print("ERROR: toRGB, i = %d\n" % i);
            assert(false);
        }
    }

    Vec3(r,g,b);
}

\: setLUT (void; 
           string target,
           float[] lut,
           string kind,
           string name,
           int xs, int ys=0, int zs=0)
{
    string node;

    let lum = kind == "Luminance";

    case (target)
    {
        "Color"     -> { node = if kind == "Luminance" then "#RVColor" else "#RVLinearize"; }
        "Display"   -> { node = "@RVDisplayColor"; }
    }

    let comp = if kind == "Luminance" then "luminanceLUT" else "lut",
        fmt  = (node, comp);

    try
    {
        setFloatProperty("%s.%s.lut" % fmt, lut, true);
        setStringProperty("%s.%s.name" % fmt, string[] {name});
        setIntProperty("%s.%s.active" % fmt, int[] {1});

        if (target == "Color")
        {
            setIntProperty("%s.%s.size" % fmt, int[] {xs});
        }
        else
        {
            setIntProperty("%s.%s.size" % fmt, int[] {xs, ys, zs});
        }
        
        displayFeedback("Using %s %s %s LUT" % (name, target, kind));
        redraw();
    }
    catch (exception exc)
    {
        let sexc = string(exc);
        displayFeedback("Unable to set LUT for %s: %s" % (node, sexc));
    }
}

\: HSVLUT (void;)
{
    float[] lut;
    lut.resize(256 * 3);

    for (int i=1; i < 256; i++)
    {
        float c = float(i) / 255.0;

        Vec3 rgb = toRGB(Vec3(c, 1.0, 1.0));
        lut[i * 3 + 0] = rgb.x;
        lut[i * 3 + 1] = rgb.y;
        lut[i * 3 + 2] = rgb.z;
    }

    setLUT("Color", lut, "Luminance", "HSV", lut.size()/3);
    lut;
}

\: randomLUT (void;)
{
    float[] lut;
    int n = math_util.random(25);
    lut.resize(n * 3);

    for (int i=1; i < n; i++)
    {
        float c = float(i) / float(n-1);

        lut[i * 3 + 0] = math_util.random(1.0);
        lut[i * 3 + 1] = math_util.random(1.0);
        lut[i * 3 + 2] = math_util.random(1.0);
    }

    setLUT("Color", lut, "Luminance", "Random", lut.size()/3);
}


\: contourLUT ((void;); int n)
{
    \: (void;)
    {
        float[] lut;
        lut.resize(n * 3);

        for (int i=0; i < n; i++)
        {
            Vec3 rgb;
            if ((i & 1) == 1) rgb = Vec3(1,1,1);
            else rgb = Vec3(0,0,0);

            float c = float(i) / float(n-1);

            lut[i * 3 + 0] = rgb.x;
            lut[i * 3 + 1] = rgb.y;
            lut[i * 3 + 2] = rgb.z;
        }

        setLUT("Color", lut, "Luminance", "Contour %d" % n, lut.size()/3);
    };
}

\: outputLUT (void;)
{
    let lut = getFloatProperty("@RVDisplayColor.lut:output.lut"),
        sizes = getIntProperty("@RVDisplayColor.lut:output.size");

    print("lut size = %d\ndimensions = %s\n" % (lut.size() / 3, sizes));

    for (int i=0; i < lut.size(); i+=3)
    {
        print("%10.4f %10.4f %10.4f\n" %
              (lut[i + 0], lut[i + 1], lut[i + 2]));
    }
}

\: identityLUT (void; Event ev, int size)
{
    float[] lut;
    lut.resize(size * size * size * 3);

    for (int z=0; z < size; z++)
    {
        for (int y=0; y < size; y++)
        {
            for (int x=0; x < size; x++)
            {
                int i = (z * size * size + y * size + x) * 3;
                lut[i+0] = x / float(size-1);
                lut[i+1] = y / float(size-1);
                lut[i+2] = z / float(size-1);
            }
        }
    }

    setLUT("Display", lut, "Cube", "Identity %d" % size, size, size, size);
};

//
//  Higher order functions.
//

\: alphaTypeState (MenuStateFunc; int alphaType)
{
    \: (int;)
    {
        try
        {
            return if getIntProperty("#RVLinearize.color.alphaType").front() == alphaType
                    then CheckedMenuState
                    else UncheckedMenuState;
        }
        catch (...)
        {
            return DisabledMenuState;
        }
    };
}

\: aspectState (MenuStateFunc; float a)
{
    \: (int;)
    {
        try
        {
            return if getFloatProperty("#RVLensWarp.warp.pixelAspectRatio").front() == a
                    then CheckedMenuState 
                    else UncheckedMenuState;
        }
        catch (...)
        {
            return DisabledMenuState;
        }
    };
}

\: lutState (MenuStateFunc; 
             string name, 
             string lutType,    // type == Display or Color
             string kind)       // kind == Luminance or not
{
    let node = if lutType == "Display" then "@RVDisplayColor" 
                        else if kind == "Luminance" then "#RVColor" else "#RVLinearize",
        comp = if kind == "Luminance" then "luminanceLUT" else "lut",
        fmt  = (node, comp);

    \: (int;)
    {
        try
        {
            let a = getIntProperty("%s.%s.active" % fmt).front(),
                n = getStringProperty("%s.%s.name" % fmt).front();

            return if (a == 1 && name == n) 
                    then CheckedMenuState 
                    else UncheckedMenuState;
        }
        catch (...)
        {
            return DisabledMenuState;
        }
    };
}

\: fileLUTState (MenuStateFunc; string name) { lutState(name, "Color", "File"); }
\: luminanceLUTState (MenuStateFunc; string name) { lutState(name, "Color", "Luminance"); }
\: displayLUTState (MenuStateFunc; string name) { lutState(name, "Display", "LUT"); }

\: dispGammaState (int;)
{
    float g = 1.0;

    try
    {
        g = getFloatProperty("@RVDisplayColor.color.gamma").front();
    }
    catch (...)
    {
        return DisabledMenuState;
    }

    return if (g != 1.0 && g != 2.2 && g != 2.4)
            then CheckedMenuState
            else UncheckedMenuState;
}

\: fileGammaState (int;)
{
    try
    {
        int a = getIntProperty("#RVLinearize.color.active").front();
        if (a == 0) return DisabledMenuState;

        float g = getFloatProperty("#RVLinearize.color.fileGamma").front();
        if (g != 1.0 && g != 2.2)
            then CheckedMenuState
            else UncheckedMenuState;
    }
    catch (...)
    {
        return DisabledMenuState;
    }
}

\: consoleState (int;)
{
    if isConsoleVisible() then CheckedMenuState else UncheckedMenuState;
}

\: setRangeOffsetValue (void; string v)
{
    if (v != "")
    {
        setIntProperty("#RVSource.group.rangeOffset", int[] { int(v) });
        displayFeedback("Range Offset: " + v);
    }
    redraw();
}


\: setRangeOffset (void; Event event)
{
    State state = data();
    state.prompt = "Range Offset: ";
    state.textFunc = setRangeOffsetValue;
    state.textEntry = true;
    state.textOkWhenEmpty = true;
    let oldV = getIntProperty("#RVSource.group.rangeOffset").front();
    state.text = string(oldV);
    pushEventTable("textentry");
    redraw();

    try
    {
        let k = event.key();
        if (k >= '0' && k <= '9') selfInsert(event);
    }
    catch (...)
    {
        ;// Just ignore non-key events don't selfInsert
    }
}

\: toggleIntProp (VoidFunc; string name)
{
    \: (void;)
    {
        let s = getIntProperty(name).front(),
            n = string.split(name, ".").back();

        setIntProperty(name, int[] {if s == 1 then 0 else 1});
        State state = data();
        displayFeedback(n);
        redraw();
    };
}

\: toggleFloat (void;)
{
    let name = "#RVFormat.color.allowFloatingPoint",
        s = getIntProperty(name).front(),
        n = string.split(name, ".").back();

    setIntProperty(name, int[] {if s == 1 then 0 else 1});
    State state = data();
    displayFeedback(n);
    reload();
}

\: setColorSpaceAttr (VoidFunc; string attr, string value)
{
    \: (void;)
    {
        for_each (node; sourcesAtFrame(frame()))
        {
            string attrProp = "%s.attributes.ColorSpace/%s" % (node,attr);
            bool exists = propertyExists(attrProp);
            if (value == "From Image")
            {
                if (exists) deleteProperty(attrProp);
            }
            else
            {
                if (!exists) newProperty(attrProp, StringType, 1);
                setStringProperty(attrProp, string[] {value}, true);
            }
        }
    };
}

\: setImageResolution (VoidFunc; float res)
{
    \: (void;)
    {
        setFloatProperty("#RVFormat.geometry.scale", float[] {res});
        reload();
    };
}

//
//  This function only applies if you're already at 1:1 scaling on an
//  image. It will force the camera translation so that there is no
//  sub-pixel translation. After it runs, toggling linear filter
//  on/off will show no difference in the image
//
//  NOTE: the way this is working now, you can't just call it
//  immediately after 1:1 -- you need to wait for the next render. 
//

\: snapToPixelCenter (void;)
{
   require math_linear;

   updatePixelInfo(nil);
   State state = data();

   if (state.pixelInfo neq nil && !state.pixelInfo.empty())
   {
       PixelImageInfo info = state.pixelInfo.front();

       for_each (ri; renderedImages())
       {
           if (ri.name == info.name)
           {
               let totalView = viewSize(),
                   m = margins(),
                   v0 = Vec2(totalView.x - m[0] - m[1], totalView.y - m[2] - m[3]),
                   v = v0 * 0.5,
                   VP = float[4,4] { v.x, 0,    0,  v.x,
                                     0,   v.y,  0,  v.y,
                                     0,   0,    1,  0,
                                     0,   0,    0,  1 },
                   PM = ri.projectionMatrix,
                   MV = ri.globalMatrix,
                   S = float[4,4] {  scale(), 0,        0, 0,
                                     0,       scale(),  0, 0,
                                     0,       0,        1, 0,
                                     0,       0,        0,  1 },
                   T = float[4,4] {  1,   0,   0, translation().x,
                                     0,   1,   0, translation().y,
                                     0,   0,   1,  0,
                                     0,   0,   0,  1 },
                   invST = inverse(S * T),
                   M = invST  * MV,
                   t  = Vec3(0, 0, 0),
                   L = VP * PM * S,
                   R = M * t,
                   tv = VP * PM * MV * t,
                   nx = int(floor(tv.x + 0.5)),
                   ny = int(floor(tv.y + 0.5)),
                   d = Vec3(nx, ny, 0),
                   invL = inverse(L),
                   newt = invL * d - R;

               //
               //  This will cause a crash eventually (don't know why).  The
               //  root seems to be setting the translation "mid-render", so
               //  we delay it and do it in the next pre-render event.
               //
               // setTranslation(Vec2(newt.x, newt.y));
               //
               snapDoit = true;
               snapTranslation = Vec2(newt.x, newt.y);
               redraw();
               break;
           }
       }
   }
}

\: computePixelRelativeScale (float; float scl, bool fitWidthOnly, bool silent=false)
{
    require math_linear;

    updatePixelInfo(nil);
    State state = data();

    if (state.pixelInfo neq nil && !state.pixelInfo.empty())
    {
        PixelImageInfo info = state.pixelInfo.front();

        for_each (ri; renderedImages())
        {
            if (ri.name == info.name)
            {
                let totalView = viewSize(),
                    m = margins(),
                    w = ri.width * (if (expandWidth) then ri.pixelAspect else 1.0),
                    G = ri.globalMatrix,
                    h = ri.height / (if (expandWidth) then 1.0 else ri.pixelAspect),
                    d = Vec2(totalView.x - m[0] - m[1], totalView.y - m[2] - m[3]);

                if (fitWidthOnly)
                {
                        let ia = w / h,
                            va = d.x / d.y;

                        return if (ia > va) then 1.0 else float (va/ia);
                }
                
                let r0 = Vec3(G[0,0], G[1,0], G[2,0]),
                    r1 = Vec3(G[0,1], G[1,1], G[2,1]),
                    r2 = Vec3(G[0,2], G[1,2], G[2,2]),
                    nr0 = normalize(r0),
                    nr1 = normalize(r1),
                    nr2 = normalize(r2),

                    R = float[3,3] { r0.x,  r0.y,  r0.z,
                                     r1.x,  r1.y,  r1.z,
                                     r2.x,  r2.y,  r2.z },

                    O = float[3,3] { nr0.x, nr0.y, nr0.z,
                                     nr1.x, nr1.y, nr1.z,
                                     nr2.x, nr2.y, nr2.z },

                    S = R * inverse(O);

                let s = S[1,1] / scale(), 
                    ns = h / (s * d.y) * scl;

                if (!silent)
                {
                    displayFeedback("Scale %g:%g" %
                                (if scl > 1 then scl else 1.0,
                                if scl > 1 then 1.0 else 1.0 / scl));
                }

                return ns;
            }
        }
    }
    return 0.0;
}

\: setPixelRelativeScale(void; float ns, float requestedScale=0.0)
{
    setScale(ns);

    if (requestedScale == 1.0) preSnapRenderCount = 2;

    redraw();
}

\: pixelRelativeScale (VoidFunc; float scl)
{
    \: (void; )
    {
        let ns = computePixelRelativeScale(scl, false);
        if (ns != 0.0) setPixelRelativeScale(ns, scl);
    };
}

\: frameWidth (void; Event event)
{
    let ns = computePixelRelativeScale(1.0, true);

    if (ns != 0.0) 
    {
        extra_commands.setTranslation (rvtypes.Point(0,0));
        setPixelRelativeScale(ns);
    }
}

\: imageResState (MenuStateFunc; float res)
{
    \: (int;)
    {
        try
        {
            return if getFloatProperty("#RVFormat.geometry.scale").front() == res
                then CheckedMenuState else UncheckedMenuState;
        }
        catch (...)
        {
            return DisabledMenuState;
        }
    };
}


\: sourcesExistState (int;)
{
    if sources().size() > 0 then NeutralMenuState else DisabledMenuState;
}

\: singleSourceState (int;)
{
    try
    {
        return if sourcesRendered().size() == 1 then
            NeutralMenuState else
            DisabledMenuState;
    }
    catch (...)
    {
        return DisabledMenuState;
    }
}

\: videoSourcesExistState (int;)
{
    for_each (s; sources())
    {
        if (s eq nil) continue;
        let video  = s._6;
        if (video) return NeutralMenuState;
    }

    DisabledMenuState;
}

\: videoSourcesExistAndExportOKState (int;)
{
    let vidOK = (videoSourcesExistState() == NeutralMenuState),
        noexp = (system.getenv("RV_NO_MOVIE_EXPORT", nil) neq nil);

    if (!noexp && vidOK) return NeutralMenuState;

    DisabledMenuState;
}

\: videoSourcesAndNodeExistState (MenuStateFunc; string nodeType)
{
    \: (int;)
    {
        let vidOK  = (videoSourcesExistState() == NeutralMenuState),
            exists = (nodeType == "" || closestNodesOfType(nodeType).size() > 0);

        if (exists && vidOK) return NeutralMenuState;

        DisabledMenuState;
    };
}

\: canExportOTIOState (int;)
{
    let vnode = viewNode();

    if (vnode neq nil) 
    {
        let typeName = nodeType(vnode);

        if (typeName == "RVStackGroup" ||
            typeName == "RVSequenceGroup" ||
            typeName == "RVSourceGroup")
        {
            return if isOtioEnabled()
                   then NeutralMenuState
                   else DisabledMenuState;
        }
    }
    DisabledMenuState;
}

\: exportOtio (void; Event event)
{
    State state = data();

    try
    {
        string f = saveFileDialog(false, "otio|OpenTimelineIO File");

        if (path.extension(f) != "otio")
        {
            sendInternalEvent("otio-export", f + ".otio", "rvui");
        }
        else
        {
            sendInternalEvent("otio-export", f, "rvui");
        }
    }
    catch (...)
    {
        displayFeedback("Cancelled");
    }

    redraw();
}

\: matchesColorSpaceAttr(MenuStateFunc; string attr, string value)
{
    \: (int;)
    {
        try
        {
            if (videoSourcesAndNodeExistState("RVFileSource")() == DisabledMenuState)
            {
                return DisabledMenuState;
            }
            string attrProp = "#RVFileSource.attributes.ColorSpace/%s" % attr;
            if (propertyExists(attrProp))
            {
                return if getStringProperty(attrProp).front() == value
                    then CheckedMenuState else UncheckedMenuState;
            }
            else if (value == "From Image")
            {
                return CheckedMenuState;
            }
            else
            {
                return UncheckedMenuState;
            }
        }
        catch (...)
        {
            return DisabledMenuState;
        }
    };
}

\: toggleIntPropState (MenuStateFunc; string name)
{
    \: (int;)
    {
        try
        {
            return if getIntProperty(name).front() == 0
                        then UncheckedMenuState
                        else CheckedMenuState;
        }
        catch (...)
        {
            return DisabledMenuState;
        }
    };
}


global let toggleFlip = toggleIntProp("#RVTransform2D.transform.flip"),
           toggleFlop = toggleIntProp("#RVTransform2D.transform.flop"),
           toggleRFlip = toggleIntProp("@RVDisplayStereo.rightTransform.flip"),
           toggleRFlop = toggleIntProp("@RVDisplayStereo.rightTransform.flop"),
           toggleSourceRFlip = toggleIntProp("#RVSourceStereo.rightTransform.flip"),
           toggleSourceRFlop = toggleIntProp("#RVSourceStereo.rightTransform.flop"),
           toggleChromaticities = toggleIntProp("#RVLinearize.color.ignoreChromaticities"),
           toggleOutOfRange = toggleIntProp("@RVDisplayColor.color.outOfRange"),
           toggleSwapEyes = toggleIntProp("@RVDisplayStereo.stereo.swap"),
           toggleSourceSwapEyes = toggleIntProp("#RVSourceStereo.stereo.swap"),
           toggleMute = toggleIntProp("#RVSoundTrack.audio.mute"),
           toggleFlipState = toggleIntPropState("#RVTransform2D.transform.flip"),
           toggleFlopState = toggleIntPropState("#RVTransform2D.transform.flop"),
           toggleRFlipState = toggleIntPropState("@RVDisplayStereo.rightTransform.flip"),
           toggleRFlopState = toggleIntPropState("@RVDisplayStereo.rightTransform.flop"),
           toggleSourceRFlipState = toggleIntPropState("#RVSourceStereo.rightTransform.flip"),
           toggleSourceRFlopState = toggleIntPropState("#RVSourceStereo.rightTransform.flop"),
           isDisplayICCActiveState = toggleIntPropState("@ICCDisplayTransform.node.active"),
           isDisplayLUTActiveState = toggleIntPropState("@RVDisplayColor.lut.active"),
           isCacheLUTActiveState = toggleIntPropState("#RVCacheLUT.lut.active"),
           isFileLUTActiveState = toggleIntPropState("#RVLinearize.lut.active"),
           isLookLUTActiveState = toggleIntPropState("#RVLookLUT.lut.active"),
           isFileCDLActiveState = toggleIntPropState("#RVLinearize.CDL.active"),
           isLookCDLActiveState = toggleIntPropState("#RVColor.CDL.active"),
           isFileICCActiveState = toggleIntPropState("#ICCLinearizeTransform.node.active"),
           isLuminanceLUTActiveState = toggleIntPropState("#RVColor.luminanceLUT.active"),
           isInvert = toggleIntPropState("#RVColor.color.invert"),
           isIgnoringChromaticies = toggleIntPropState("#RVLinearize.color.ignoreChromaticities"),
           isOutOfRange = toggleIntPropState("@RVDisplayColor.color.outOfRange"),
           isFloatAllowed = toggleIntPropState("#RVFormat.color.allowFloatingPoint"),
           isMuted = toggleIntPropState("#RVSoundTrack.audio.mute"),
           isSwapEyes = toggleIntPropState("@RVDisplayStereo.stereo.swap"),
           isSourceSwapEyes = toggleIntPropState("#RVSourceStereo.stereo.swap");


\: toggleNormalizeColor (void; Event ev)
{
    try
    {
        int v = if isNormalizingColor() == CheckedMenuState then 0 else 1;
        setIntProperty("#RVColor.color.normalize", int[] {v}, true);
        setIntProperty("#RVHistogram.node.active", int[] {v}, true);
        reload();
    }
    catch (exception exc)
    {
        let sexc = string(exc);
        displayFeedback("Unable to toggle normalize color: %s" % sexc);
    }
}

\: isNormalizingColor (int;)
{
    try
    {
        return if (getIntProperty("#RVColor.color.normalize").front() == 1 &&
                   getIntProperty("#RVHistogram.node.active").front() == 1)
                then CheckedMenuState
                else UncheckedMenuState;
            
    }
    catch (...)
    {
        return DisabledMenuState;
    }
}

\: isOtioEnabled (bool;)
{
    string result = sendInternalEvent("otio-import-enabled");
    return if sendInternalEvent("otio-import-enabled") == ""
           then false
           else true;
}       
 
\: checkForDisplayColor (MenuStateFunc;)
{
    \: (int;)
    {
        if (metaEvaluateClosestByType(frame(),"RVDisplayColor").size() > 0)
        {
            return UncheckedMenuState;
        }
        return DisabledMenuState;
    };
}

\: checkForDisplayNode (MenuStateFunc;)
{
    \: (int;)
    {
        if (metaEvaluateClosestByType(frame(),"OCIODisplay").size() > 0)
        {
            return UncheckedMenuState;
        }
        if (metaEvaluateClosestByType(frame(),"RVDisplayColor").size() > 0)
        {
            return UncheckedMenuState;
        }
        return DisabledMenuState;
    };
}

\: checkForOTIOFile (MenuStateFunc;)
{
    \: (int;)
    {
        return if isOtioEnabled() 
               then UncheckedMenuState
               else DisabledMenuState;
    };
}

\: channelState (MenuStateFunc; int ch)
{
    \: (int;)
    {
        try
        {
            string propertyName = "@OCIODisplay.color.channelFlood";
            if (!propertyExists(propertyName))
            {
                propertyName = "@RVDisplayColor.color.channelFlood";
            }

            return if getIntProperty(propertyName).front() == ch
                    then CheckedMenuState else UncheckedMenuState;

        }
        catch (...)
        {
            return DisabledMenuState;
        }
    };
}

\: channelOrderState (MenuStateFunc; string order)
{
    \: (int;)
    {
        try
        {
            string propertyName = "@OCIODisplay.color.channelOrder";
            if (!propertyExists(propertyName))
            {
                propertyName = "@RVDisplayColor.color.channelOrder";
            }

            return if getStringProperty(propertyName).front() == order
                    then CheckedMenuState else UncheckedMenuState;
        }
        catch (...)
        {
            return DisabledMenuState;
        }
    };
}

\: frameFunc (VoidFunc; (void;int) F)
{
    \: (void;) { F(frame()); };
}

\: incN ((void;); int i)
{
    \: (void;) {
        if (filterLiveReviewEvents()) {
            sendInternalEvent("live-review-blocked-event");
            return;
        }
        setInc(i);
    };
}

global (string, Glyph)[] showChannelGlyphs =
{
    ("Color Display", xformedGlyph(rgbGlyph, 0, 1.5)),
    ("Red Channel", xformedGlyph(coloredGlyph(circleGlyph, Color(1,0,0,1)), 0, 1.5)),
    ("Green Channel", xformedGlyph(coloredGlyph(circleGlyph, Color(0,1,0,1)), 0, 1.5)),
    ("Blue Channel", xformedGlyph(coloredGlyph(circleGlyph, Color(0,0,1,1)), 0, 1.5)),
    ("Alpha Channel",
     xformedGlyph(coloredGlyph(drawCircleFan(0, 0, 0.5, 0.0, 0.5, .3,),
                               Color(0.25, 0.25, 0.25, 1.0)) &
                  coloredGlyph(drawCircleFan(0, 0, 0.5, 0.5, 1.0, .3,),
                               Color(0.75, 0.75, 0.75, 1.0)),
                  0, 1.5)),
    ("Luminance Display", xformedGlyph(circleGlyph, 0, 1.5))
};

\: paramFeedbackData ((int, string, Glyph); )
{
    State state = data();

    let ch       = state.parameterChannel,
        paramTxt = string.split(state.parameter, ".").back(),
        chTxt   = if (ch == 0) then "red " else if (ch == 1) then "green " else if (ch == 2) then "blue " else "";

    Glyph g = nil;
    if (ch != -1)
    {
        let i = if (ch < 0) then 0 else ch + 1,
            (_, gf) = showChannelGlyphs[i];

        g = gf;
    }

    return (ch, chTxt + paramTxt, g);
}

\: startParameterMode (EventFunc;
                       string param, 
                       float scl, 
                       float reset,
                       float minValue       = -float.max,
                       (float;) granularity = nil,
                       float precision      = 0.1)
{
    \: (void; Event event)
    {
        if (filterLiveReviewEvents() && regex.match("color", param)) {
            sendInternalEvent("live-review-blocked-event");
            return;
        }
        if (!sources().empty())
        {
            State state = data();

            try
            {
                let info = propertyInfo(param),
                    //  Some assumptions here ...
                    isColor = info.type == FloatType && 
                    info.dimensions._0 == 3 &&
                    info.dimensions._1 == 0;

                pushEventTable("paramscrub");
            
                state.parameter            = param;
                state.parameterScale       = scl;
                state.parameterReset       = reset;
                state.parameterMinValue    = minValue;
                state.parameterGranularity = granularity;
                state.parameterPrecision   = precision;
                state.parameterLocked      = false;
                state.parameterNode        = string.split(param, ".").front();

                state.parameterChannel = if (isColor) then -2 else -1;

                let (ch, txt, gf) = paramFeedbackData();

                displayFeedback("Edit %s (? for hotkey help)" % txt, 100000, gf);
                redraw();
            }
            catch (...)
            {
                displayFeedback("Edit mode not available (%s)" % param);
                redraw();
            }
        }
    };
}

\: resetAllColorParameters (void;)
{
    if (filterLiveReviewEvents()) {
        sendInternalEvent("live-review-blocked-event");
        return;
    }
    for_each (p; (string, float)[] {
                  ("gamma",        DefaultGamma),
                  ("exposure",     DefaultExposure),
                  ("offset",       DefaultOffset),
                  ("saturation",   DefaultSaturation),
                  ("hue",          DefaultHue),
                  ("contrast",     DefaultContrast)})
    {
        let (name, value) = p;
        fillProperty("#RVColor.color.%s" % name, value);
    }

    redraw();
}

\: frameTimeGlobal (float; )
{
    return 1.0 / fps();
}

\: frameTimeLocal (float; )
{
    State state = data();
    float fps = fps();

    if (state.pixelInfo neq nil && !state.pixelInfo.empty())
    {
        let mediaName = state.pixelInfo.front().name;

        if (mediaName != "")
        {
            try
            {
                let sname = string.split(mediaName, ".").front(),
                    info = sourceMediaInfo(sname, nil);

                fps = info.fps;
            }
            catch (...) { ; }
        }
    }

    return 1.0 / fps;
}


global let gammaMode      = startParameterMode("#RVColor.color.gamma", 4.0, DefaultGamma, 0.0),
           exposureMode   = startParameterMode("#RVColor.color.exposure", 4.0, DefaultExposure),
           colorOffsetMode = startParameterMode("#RVColor.color.offset", 1.0, DefaultOffset),
           brightnessMode = startParameterMode("@RVDisplayColor.color.brightness", 4.0, DefaultExposure),
           saturationMode = startParameterMode("#RVColor.color.saturation", 2.0, DefaultSaturation, 0.0),
           hueMode        = startParameterMode("#RVColor.color.hue", math.pi, DefaultHue),
           contrastMode   = startParameterMode("#RVColor.color.contrast", 0.4, DefaultContrast),
           rotateMode     = startParameterMode("#RVTransform2D.transform.rotate", 25.0, 0.0),
           globalVolumeMode = startParameterMode("#RVSoundTrack.audio.volume", 1.0, 1.0, 0.0),
           globalBalanceMode = startParameterMode("#RVSoundTrack.audio.balance", 0.75, 0.0),
           globalAudioOffsetMode = startParameterMode("#RVSoundTrack.audio.offset", 0.25, 0.0),
           globalAudioOffsetFramesMode = startParameterMode("#RVSoundTrack.audio.offset", 0.5, 0.0, -float.max, frameTimeGlobal),
           sourceAudioOffsetMode = startParameterMode("#RVSource.group.audioOffset", 0.25, 0.0),
           sourceAudioOffsetFramesMode = startParameterMode("#RVSource.group.audioOffset", 0.5, 0.0, -float.max, frameTimeLocal),
           sourceVolumeMode  = startParameterMode("#RVSource.group.volume", 1.0, 1.0, 0.0),
           sourceBalanceMode = startParameterMode("#RVSource.group.balance", 0.75, 0.0),
           stereoOffsetMode = startParameterMode("@RVDisplayStereo.stereo.relativeOffset", 0.05, 0.0, -float.max, \:(float;) {0.01;}, 0.01),
           stereoROffsetMode = startParameterMode("@RVDisplayStereo.stereo.rightOffset", 0.05, 0.0, -float.max, \:(float;) {0.01;}, 0.01),
           sourceStereoOffsetMode = startParameterMode("#RVSourceStereo.stereo.relativeOffset", 0.05, 0.0, -float.max, \:(float;) {0.01;}, 0.01),
           sourceStereoROffsetMode = startParameterMode("#RVSourceStereo.stereo.rightOffset", 0.05, 0.0, -float.max, \:(float;) {0.01;}, 0.01);

\: startTextEntryMode (EventFunc; 
                       PromptFunc prompt,
                       TextCommitFunc func,
                       bool okWhenEmpty=false)
{
    \: (void; Event event)
    {
        State state = data();
        state.prompt = prompt();
        state.textFunc = func;
        state.textEntry = true;
        state.textOkWhenEmpty = okWhenEmpty;
        redraw();
        pushEventTable("textentry");
        killAllText(event);

        try
        {
            int k = event.key();
            if ((k >= '0' && k <= '9') || k == int('.')) selfInsert(event);
        }
        catch (...)
        {
            ;// Just ignore non-key events don't selfInsert
        }
    };
}



\: windowActivate (void; )
{
    State state = data();
    state.dragDropOccuring = false;
    setActiveState();
}

\: pointerEnterSession (void;)
{
    State state = data();
    state.pointerInSession = true;
    setActiveState();
}

\: pointerLeaveSession (void;)
{
    State state = data();
    state.pointerInSession = false;
    state.dragDropOccuring = false;
    setActiveState();
}

\: gotoFrame (void; string text)
{
    if (filterLiveReviewEvents()) {
        sendInternalEvent("live-review-blocked-event");
        return;
    }
    setFrame(int(text));
    redraw();
}

\: setRes (void; string text)
{
    setFloatProperty("#RVFormat.geometry.scale", float[] {float(text)});
    redraw();
}

\: paramPrompt (string;)
{
    let (ch, txt, gf) = paramFeedbackData();
    "Set %s: " % txt;
}

\: setParamValue (void; string text)
{
    State state = data();

    let nf = float(text),
        (ch, txt, gf) = paramFeedbackData(),
        locked = state.parameterLocked,
        textPrefix = if (locked) then "Set " else "Finished Editing ",
        feedback  = "";

    if (state.parameterGranularity neq nil)
    {
        feedback = textPrefix + "%s => %g" % (txt, nf);
        nf = nf * state.parameterGranularity();
    }
    else
    {
        feedback = textPrefix + "%s => %.02f" % (txt, nf);
    }
    fillProperty(state.parameter, nf, ch);

    if (regex.match("audio", state.parameter) && state.scrubAudio)
    {
        setAudioCacheMode(CacheBuffer);
        setAudioCacheMode(CacheGreedy);
    }

    if (locked) displayFeedback(feedback, 10000, gf);
    else
    {
        displayFeedback(feedback, 2.0, gf);
        popEventTable("paramscrub");
    }
    redraw();
}

\: orderPrompt (string;)
{
    string propertyName = "@OCIODisplay.color.channelOrder";
    if (!propertyExists(propertyName))
    {
        propertyName = "@RVDisplayColor.color.channelOrder";
    }

    "Enter Channel Order [%s]: " %
        getStringProperty(propertyName).back();
}

\: pixaPrompt (string;)
{
    float pa = 1.0;
    try
    {
        pa = getFloatProperty("#RVLensWarp.warp.pixelAspectRatio").front();
    }
    catch (...)
    {
        displayFeedback("Unable to access pixel aspect");
    }

    return "Pixel Aspect Ratio [%g]: " % pa;
}

\: fpsPrompt (string;)
{
    "FPS [%g]: " % fps();
}

\: sourceFpsPrompt (string;)
{
    "Source FPS [%g]: " % getFloatProperty("#RVSource.group.fps").front();
}

\: resPrompt (string;)
{
    "Image Playback Resolution [%g]: " %
        getFloatProperty("#RVFormat.geometry.scale").front();
}

\: dispGammaPrompt (string;)
{
    "Display Gamma Correction [%g]: " %
        getFloatProperty("@RVDisplayColor.color.gamma").front();
}

\: fileGammaPrompt (string;)
{
    float fgamma = 1.0;
    try
    {
        fgamma = getFloatProperty("#RVLinearize.color.fileGamma").front();
    }
    catch (...)
    {
        displayFeedback("Unable to access file gamma");
    }

    return "File Gamma Correction [%g]: " % fgamma;
}

\: chanMapPrompt (string;)
{
    string s;

    for_each (ch; getStringProperty("#RVChannelMap.format.channels"))
    {
        if (s == "") s = ch;
        else s = "%s, %s" % (s, ch);
    }

    "Remap Input Channels [%s]: " % s;
}

\: mattePrompt (string;)
{
    State state = data();
    "Matte Aspect Ratio [%g]: " % state.matteAspect;
}

\: matteOpacityPrompt (string;)
{
    State state = data();
    "Matte Opacity [%g]: " % state.matteOpacity;
}

\: setOrderValue (void; string order)
{
    string propertyName = "OCIODisplay.color.channelOrder";
    if (!propertyExists("@%s" % propertyName))
    {
        propertyName = "RVDisplayColor.color.channelOrder";
    }
    
    setStringProperty("#%s" % propertyName, string[]{order});
}

\: setPixaValue (void; string v)
{
    try
    {
        setFloatProperty("#RVLensWarp.warp.pixelAspectRatio", float[] {float(v)});
    }
    catch (exception exc)
    {
        let sexc = string(exc);
        displayFeedback("Unable to set pixel aspect: %s" % sexc);
    }
}

\: setFPSValue (void; string v)
{
    let f = float(v);

    if (f > 0)
    {
        setFPS(f);
    }
    else
    {
        displayFeedback("ERROR: FPS must be larger than 0",
                        2.0,
                        coloredGlyph(pauseGlyph, Color(1, 0, 0, 1.0)));
    }
}

\: setSourceFPSValue (void; string v)
{
    let f = float(v);

    if (f > 0)
    {
        setFloatProperty("#RVSource.group.fps", float[] {f} );
    }
    else
    {
        displayFeedback("ERROR: Source FPS must be larger than 0",
                        2.0,
                        coloredGlyph(pauseGlyph, Color(1, 0, 0, 1.0)));
    }
}

\: resetSessionMatteCenter(void;)
{
    setFloatProperty("#Session.matte.centerPoint", float[]{0.0, 0.0}, true);
    setFloatProperty("#Session.matte.heightVisible", float[]{-1.0}, true);
}

\: setMatteValue (void; string v)
{
    State state = data();
    state.matteAspect = float(v);
    state.showMatte   = true;
    setFloatProperty("#Session.matte.aspect", float[]{state.matteAspect}, true);
    resetSessionMatteCenter();
    setIntProperty("#Session.matte.show", int[]{1}, true);
    redraw();
}

\: setMatteOpacityValue (void; string v)
{
    State state = data();
    state.matteOpacity = float(v);
    state.showMatte    = true;
    writeSetting ("View", "matteOpacity", SettingsValue.Float(state.matteOpacity));
    setFloatProperty("#Session.matte.opacity", float[]{state.matteOpacity}, true);
    setIntProperty("#Session.matte.show", int[]{1}, true);
    redraw();
}

\: contentsEqual (bool; string[] a, string[] b)
{
    if (a.size() != b.size()) return false;
    for_index (i; a) if (a[i] != b[i]) return false;
    return true;
}

\: setChanMap (void; string v)
{
    let channels = string.split(v, ", "),
        pname = "#RVChannelMap.format.channels",
        current = getStringProperty(pname);

    if (!contentsEqual(current, channels))
    {
        setStringProperty(pname, channels, true);
        setIntProperty("#RVSource.request.readAllChannels", int[]{1}, true);
        reload();
    }
}

\: toggleAudioScrub (void; Event event)
{
    State state = data();
    state.scrubAudio = !state.scrubAudio;
    setAudioCacheMode(if state.scrubAudio then CacheGreedy else CacheBuffer);
    displayFeedback("Scrub %s" % (if state.scrubAudio then "ON" else "OFF"));
    redraw();
}

\: toggleScrubDisabled (void; Event event)
{
    State state = data();
    state.scrubDisabled = !state.scrubDisabled;
    writeSetting ("Controls", "disableScrubInView", SettingsValue.Bool(state.scrubDisabled));
    displayFeedback("Scrub in view %s" % (if state.scrubDisabled then "OFF" else "ON"));
    redraw();
}

\: toggleClickToPlayDisabled (void; Event event)
{
    State state = data();
    state.clickToPlayDisabled = !state.clickToPlayDisabled;
    writeSetting ("Controls", "disableClickToPlay", SettingsValue.Bool(state.clickToPlayDisabled));
    displayFeedback("Click to play %s" % (if state.clickToPlayDisabled then "OFF" else "ON"));
    redraw();
}

\: resetStereoOffsets (void; Event event)
{
    setFloatProperty("@RVDisplayStereo.stereo.relativeOffset", float[]{0.0}, true);
    setFloatProperty("@RVDisplayStereo.stereo.rightOffset", float[]{0.0}, true);

    for_each (s; nodesOfType("RVSourceStereo"))
    {
        setFloatProperty(s + ".stereo.relativeOffset", float[]{0.0}, true);
        setFloatProperty(s + ".stereo.rightOffset", float[]{0.0}, true);
    }
    redraw();
}

\: resetAudioOffsets (void; Event event)
{
    setFloatProperty("#RVSoundTrack.audio.offset", float[]{0.0}, true);

    for_each (s; nodesOfType("RVFileSource"))
    {
        setFloatProperty(s + ".group.audioOffset", float[]{0.0}, true);
    }

    State state = data();
    if (state.scrubAudio)
    {
        setAudioCacheMode(CacheBuffer);
        setAudioCacheMode(CacheGreedy);
    }
    redraw();
}

\: scrubAudioState (int;)
{
    State state = data();
    if state.scrubAudio == true then CheckedMenuState else UncheckedMenuState;
}

\: scrubDisabledState (int;)
{
    State state = data();
    if state.scrubDisabled == true then CheckedMenuState else UncheckedMenuState;
}

\: clickToPlayDisabledState (int;)
{
    State state = data();
    if state.clickToPlayDisabled == true then CheckedMenuState else UncheckedMenuState;
}

\: setDispGamma (void; string v)
{
    setFloatProperty("@RVDisplayColor.color.gamma", float[]{float(v)});
    setIntProperty("@RVDisplayColor.color.Rec709", int[] {0});
    setIntProperty("@RVDisplayColor.color.sRGB", int[] {0});
}

\: setFileGamma (void; string v)
{
    try
    {
        setFloatProperty("#RVLinearize.color.fileGamma", float[]{float(v)});
        setIntProperty("#RVLinearize.color.logtype", int[] {0});
        setIntProperty("#RVLinearize.color.sRGB2linear", int[] {0});
        setIntProperty("#RVLinearize.color.Rec709ToLinear", int[] {0});
    }
    catch (exception exc)
    {
        let sexc = string(exc);
        displayFeedback("Unable to set file gamma: %s" % sexc);
    }
}

global let enterFrame = startTextEntryMode(\: (string;) {"Go To Frame: ";}, gotoFrame),
           enterParam = startTextEntryMode(paramPrompt, setParamValue),
           enterOrder = startTextEntryMode(orderPrompt, setOrderValue),
           enterFPS = startTextEntryMode(fpsPrompt, setFPSValue),
           enterSourceFPS = startTextEntryMode(sourceFpsPrompt, setSourceFPSValue),
           enterPixAspect = startTextEntryMode(pixaPrompt, setPixaValue),
           enterMatte = startTextEntryMode(mattePrompt, setMatteValue),
           enterRes = startTextEntryMode(resPrompt, setRes),
           enterChanMap = startTextEntryMode(chanMapPrompt, setChanMap),
           enterDispGamma = startTextEntryMode(dispGammaPrompt, setDispGamma),
           enterFileGamma = startTextEntryMode(fileGammaPrompt, setFileGamma),
           enterMatteOpacity = startTextEntryMode(matteOpacityPrompt, setMatteOpacityValue);



//
//  Some utilities
//

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

//
//  Menu state update functions
//


\: inactiveState (int;) { DisabledMenuState; }

\: rangeState (int;)
{
    if isPlayable() then NeutralMenuState else DisabledMenuState;
}

\: forwardState (int;)
{
    if isPlayable()
         then (if inc() > 0 then CheckedMenuState else UncheckedMenuState)
         else DisabledMenuState;
}

\: backwardState (int;)
{
    if isPlayable()
        then (if inc() < 0 then CheckedMenuState else UncheckedMenuState)
        else DisabledMenuState;
}

\: hasMarksState (int;)
{
    if markedFrames().empty() then DisabledMenuState else NeutralMenuState;
}

\: markedState (int;)
{
    if (isPlayable())
    {
        let array = markedFrames(),
            fr    = frame();

        for_each (f; array) if (f == fr) return CheckedMenuState;
        return UncheckedMenuState;
    }
    else
    {
        return DisabledMenuState;
    }
}

\: sequenceState (int;)
{
    if sequenceBoundaries().size() > 1 then NeutralMenuState else DisabledMenuState;
}

\: playState (int;)
{
    if isPlaying() then CheckedMenuState else
        (if isPlayable() then UncheckedMenuState else DisabledMenuState);
}

\: pingPongState (int;)
{
    if isPlayable() then
      (if playMode() == PlayPingPong then CheckedMenuState else UncheckedMenuState)
      else DisabledMenuState;
}

\: playOnceState (int;)
{
    if isPlayable() then
      (if playMode() == PlayOnce then CheckedMenuState else UncheckedMenuState)
      else DisabledMenuState;
}

\: realtimeState (int;)
{
    if rangeState() == DisabledMenuState then DisabledMenuState
        else (if isRealtime() then UncheckedMenuState else CheckedMenuState);
}

\: cachingState (int;)
{
    if isPlayable() then (if isCaching() then CheckedMenuState else UncheckedMenuState)
                 else DisabledMenuState;
}

\: filterState (int;)
{
    if getFiltering() == GL_LINEAR then CheckedMenuState else UncheckedMenuState;
}


//
//  Actual commands
//

\: doubleClick (void; Event event)
{
    if (sources().size() == 0)
    //
    //  If we have nothing loaded yet, double-click is overloaded
    //  to mean "open media".
    //
    {
        addMovieOrImageSources (nil, false, false);
        return;
    }

    State state = data();
    qt.QTimer ct = state.clickTimer;
    if (!ct.active()) return;
    ct.stop();

    recordPixelInfo(event);
    scrubAudio(false);
    state.pushed = false;

    let input = inputAtPixel(event.pointer());
    if (input neq nil) setViewNode(input);
}

\: beginScrub (void; Event event)
{
    //
    //  Removing this for now since it prevents click-scrubbing on incomplete frame.
    //
    //if (isCurrentFrameIncomplete()) return;

    //
    //  On click-for focus os's we may get 'click-throughs' immediately immediately
    //  after an activation event, so if this time is too small, ignore the click
    //  (IE don't start playback.
    //
    if (event.activationTime() > 0.0 && event.activationTime() < 0.2) return;

    if (filterLiveReviewEvents()) {
        sendInternalEvent("live-review-blocked-event");
        return;
    }
    State state = data();
    state.scrubFrameOrigin = frame();
    state.playingBefore = (isPlaying());
    state.pushed = true;
}

\: releaseScrub (void; Event event)
{
    if (filterLiveReviewEvents()) {
        sendInternalEvent("live-review-blocked-event");
        return;
    }
    State state = data();
    if (!state.pushed) return;

    try
    {
        recordPixelInfo(event);
        qt.QTimer ct = state.clickTimer;
        ct.start();
        scrubAudio(false);
    }
    catch (...) {;}
    state.pushed = false;
}

\: dragScrub (void; bool enable, Event event)
{
    if (filterLiveReviewEvents()) {
        sendInternalEvent("live-review-blocked-event");
        return;
    }
    State state = data();
    if (!state.pushed) return;

    let p1 = event.pointer(),
        p0 = event.reference(),
        d  = event.domain(),
        relScale = numFrames()/d.x,
        absScale = 1.0/3.0, 
        nf = min(relScale,absScale) * (p1.x - p0.x) + state.scrubFrameOrigin;

    if (!state.scrubAudio) scrubAudio(false);

    //
    //  With the stylus/wacom it's hard to actually "click" ie
    //  stylus-up in the same place you stylus-down'ed, so we
    //  provide a little buffer here.  Note that "3.0" comes from
    //  the minumum scaling defined above.
    //
    if (abs(p1.x - p0.x) >= 3.0) 
    {
        stop();
        state.scrubbed = true;
    }
    if (nf > frameEnd()) nf = frameEnd();
    if (! state.scrubDisabled || enable) 
    {
        if (    state.scrubClamps &&
                state.scrubFrameOrigin <= outPoint() &&
                state.scrubFrameOrigin >= inPoint())
        {
            if (nf > outPoint()) nf = outPoint();
            if (nf < inPoint())  nf = inPoint();
        }
        if (state.scrubAudio && int(nf) != int(frame())) scrubAudio(true, 1.0 / fps(), 1);
        setFrame(nf);
    }
    redraw();
    state.perPixelInfoValid = false;
}

\: cycleMatte (void;)
{
    State state = data();
    let a = state.matteAspect;

    if (!state.showMatte)
    {
        state.showMatte = true;
        state.matteAspect = 1.33;
    }
    else if (a < 1.33) state.matteAspect = 1.33;
    else if (a < 1.66) state.matteAspect = 1.66;
    else if (a < 1.85) state.matteAspect = 1.85;
    else if (a < 2.35) state.matteAspect = 2.35;
    else if (a < 2.40) state.matteAspect = 2.40;
    else state.matteAspect = 1.33;
    
    setFloatProperty("#Session.matte.aspect", float[]{state.matteAspect}, true);
    resetSessionMatteCenter();
    setIntProperty("#Session.matte.show", int[]{if state.showMatte then 1 else 0}, true);

    displayFeedback("Matte Aspect Ratio %g" % state.matteAspect);

    redraw();
}

\: cycleMatteOpacity (void;)
{
    State state = data();
    let o = state.matteOpacity;

    if (!state.showMatte)
    {
        state.showMatte = true;
        state.matteOpacity = 0.33;
    }
    else if (o < 0.66) state.matteOpacity = 0.66;
    else if (o < 1.00) state.matteOpacity = 1.0;
    else
    {
        state.showMatte = false;
    }

    writeSetting ("View", "matteOpacity", SettingsValue.Float(state.matteOpacity));
    setFloatProperty("#Session.matte.opacity", float[]{state.matteOpacity}, true);
    setIntProperty("#Session.matte.show", int[]{if state.showMatte then 1 else 0}, true);

    if (state.showMatte) displayFeedback("Matte Opacity %g" % state.matteOpacity);
    else displayFeedback("Matte Off");

    redraw();
}

\: cacheModeFunc (EventFunc; int mode)
{
    \: (void; Event event)
    {
        setCacheMode(mode);
        redraw();
    };
}

\: toggleCacheModeFunc (EventFunc; int mode)
{
    \: (void; Event event)
    {
        setCacheMode(if cacheMode() == mode then CacheOff else mode);
        redraw();
    };
}

\: cacheStateFunc ((int;); int mode)
{
    \: (int;)
    {
        if cacheMode() == mode then CheckedMenuState else UncheckedMenuState;
    };
}

\: loadHUD (void;)
{
    runtime.load_module("HUD");
    let Fname = runtime.intern_name("HUD.initHUDObjects");
    (void;) F = runtime.lookup_function(Fname);
    F();
}

\: loadTimeline (Widget;)
{
    runtime.load_module("timeline");
    let Fname = runtime.intern_name("timeline.init");
    (Widget;) F = runtime.lookup_function(Fname);
    F();
}
        
\: toggleTimeline (void;)
{
    State state = data();
    if (state.timeline eq nil) state.timeline = loadTimeline();
    state.timeline.toggle();
}

\: toggleWipe (void;)
{
    State state = data();
    let vnode = viewNode();

    if (vnode eq nil ||
        sources().empty() ||
        (vnode neq nil && nodeType(vnode) != "RVStackGroup"))
    {
        return;
    }

    if (state.wipe eq nil)
    {
        runtime.load_module("wipes");
        let Fname = runtime.intern_name("wipes.constructor");
        (MinorMode;string) F = runtime.lookup_function(Fname);
        assert(F neq nil);
        state.wipe = F("wipe");
    }

    //
    //  If wipes are active we want to reset them before quitting
    //  and also make sure they don't come on again automagically,
    //  so call quitWipes rather than just toggling.
    //
    if (state.wipe.isActive()) 
    {
        let Fname = runtime.intern_name("wipes.quitWipes");
        (void;) F = runtime.lookup_function(Fname);
        assert(F neq nil);
        F();
    }
    else state.wipe.toggle();

    //  (old) Make sure the wipes stay below the timeline and image info
    //  
    //  (new) We don't have to do this anymore, because the wipes now only
    //  take events that occur (a) in their bbox, and (b) the wipes
    //  mode sort key is "zzz", which gives the timeline (and
    //  everything else) precedence.
    //
    //  if (state.timeline.isActive()) repeat (2) toggleTimeline();
    //  if (state.imageInfo.isActive()) repeat (2) toggleInfo();
}

\: toggleInfoStrip (void;)
{
    State state = data();
    if (state.infoStrip eq nil) loadHUD();
    state.infoStrip.toggle();
}

\: toggleVCRButtons (void;)
{
    print ("WARNING:  VCR controls are now part of the timeline, use the right-click\n" +
           "          menu on the timeline to turn on/off VCR controls.\n");
}

\: toggleColorInspector (void;)
{
    State state = data();
    let empty = sources().empty();

    if (!empty)
    {
        if (state.inspector eq nil)
        {
            runtime.load_module("inspector");
            let Fname = runtime.intern_name("inspector.constructor");
            (Widget;string) F = runtime.lookup_function(Fname);
            state.inspector = F("inspector");
        }
        
        state.inspector.toggle();
    }
}

\: toggleInfo (void;)
{
    State state = data();
    if (state.imageInfo eq nil) loadHUD();
    state.imageInfo.toggle();
    redraw();
}

\: toggleProcessInfo (void;)
{
    State state = data();
    if (state.processInfo eq nil) loadHUD();
    state.processInfo.toggle();
    redraw();
}

\: toggleSourceDetails (void;)
{
    State state = data();
    if (state.sourceDetails eq nil) loadHUD();
    state.sourceDetails.toggle();
    redraw();
}

\: togglePlayFunc (void;)
{
    if (filterLiveReviewEvents()) {
        sendInternalEvent("live-review-blocked-event");
        return;
    }
    togglePlay();
}

\: stopFunc (void;)
{
    if (filterLiveReviewEvents()) {
        sendInternalEvent("live-review-blocked-event");
        return;
    }
    stop();
}

\: togglePingPong (void;)
{
    setPlayMode(if playMode() == PlayPingPong then PlayLoop else PlayPingPong);
    displayFeedback("Ping Pong Mode: %s"
                    % (if playMode() == PlayPingPong then "ON" else "OFF"));
    redraw();
}

\: togglePlayOnce (void;)
{
    setPlayMode(if playMode() == PlayOnce then PlayLoop else PlayOnce);
    displayFeedback("Play Once Mode: %s"
                    % (if playMode() == PlayOnce then "ON" else "OFF"));
    redraw();
}

\: timelineShown (int;)
{
    State state = data();
    if state.timeline neq nil && state.timeline.isActive() 
                 then CheckedMenuState else UncheckedMenuState;
}

\: motionScopeShown (int;)
{
    State state = data();
    if (state.motionScope neq nil && state.motionScope.isActive()) 
    then CheckedMenuState
    else UncheckedMenuState;
}

\: colorInspectorShown (int;)
{
    State state = data();
    if (sources().empty()) return DisabledMenuState;
    if state.inspector neq nil && state.inspector.isActive() 
                 then CheckedMenuState else UncheckedMenuState;
}

\: infoStripShown (int;)
{
    State state = data();
    if (sources().empty()) return DisabledMenuState;
    if state.infoStrip neq nil && state.infoStrip.isActive() then CheckedMenuState else UncheckedMenuState;
}

\: processInfoShown (int;)
{
    State state = data();
    if state.processInfo neq nil && state.processInfo.isActive() 
                 then CheckedMenuState else UncheckedMenuState;
}

\: sourceDetailsShown (int;)
{
    State state = data();
    if (sources().empty()) return DisabledMenuState;
    if state.sourceDetails neq nil && state.sourceDetails.isActive() 
                 then CheckedMenuState else UncheckedMenuState;
}

\: networkAvailable (int;)
{
    if remoteNetworkStatus() == NetworkStatusOn 
        then UncheckedMenuState
        else DisabledMenuState;
}

\: wipeShown (int;)
{
    State state = data();
    let vnode = viewNode();

    if (vnode eq nil || 
        sources().empty() || 
        (vnode neq nil && nodeType(vnode) != "RVStackGroup"))
    {
        return DisabledMenuState;
    }

    if state.wipe neq nil && state.wipe.isActive() 
                 then CheckedMenuState else UncheckedMenuState;
}

\: menuBarShown (int;)
{
    if isMenuBarVisible() then CheckedMenuState else UncheckedMenuState;
}

\: botTBShown (int;)
{
    if isBottomViewToolbarVisible() then CheckedMenuState else UncheckedMenuState;
}

\: topTBShown (int;)
{
    if isTopViewToolbarVisible() then CheckedMenuState else UncheckedMenuState;
}

\: toggleTopViewToolbar (void; Event event)
{
    showTopViewToolbar(!isTopViewToolbarVisible());
    writeSetting("ViewToolBars", "top", SettingsValue.Bool(isTopViewToolbarVisible()));
}

\: toggleBottomViewToolbar (void; Event event)
{
    showBottomViewToolbar(!isBottomViewToolbarVisible());
    writeSetting("ViewToolBars", "bottom", SettingsValue.Bool(isBottomViewToolbarVisible()));
}

\: infoShown (int;)
{
    State state = data();
    if (sources().empty()) return DisabledMenuState;
    if state.imageInfo neq nil && state.imageInfo.isActive() 
                 then CheckedMenuState else UncheckedMenuState;
}

\: beginMoveOrZoom (void; Event event)
{
    State state = data();
    state.downPoint = translation();
    state.downScale = scale();
    recordPixelInfo(event);
}

\: dragZoom (void; Event event)
{
    State state = data();

    let dp = event.pointer() - event.reference(),
        d  = event.domain(),
        ds = state.downScale,
        m  = abs(dp.x / d.x) * ds * 2.0,
        s  = if dp.x < 0.0 then -0.75 else 15.0*m;

    float ns = math.max(ds + m * s, 0.01);

    let ms = margins(),
        center  = Vec2(float(viewSize().x - ms[0] - ms[1])/2.0 + float(ms[0]),
                       float(viewSize().y - ms[2] - ms[3])/2.0 + float(ms[3]));

    let normalizer = math.max(float(viewSize().y - ms[2] - ms[3]), 0.01),
        focus      = (center - event.reference())/(ds*normalizer),
        diff       = (focus/ds - focus/ns)*ds + state.downPoint;

    setScale(ns);
    setTranslation(diff);
    recordPixelInfo(event);
}

\: dragMoveLocked (void; bool lockToAxis, Event event)
{
    recordPixelInfo(event);
    State state = data();
    if (state.pixelInfo eq nil || state.pixelInfo.empty()) return;
    let pinfo = state.pixelInfo.front();

    let t = eventToCameraSpace(pinfo.name, event.pointer()) -
            eventToCameraSpace(pinfo.name, event.reference());

    Point f = state.downPoint + t / scale();
    if (lockToAxis)
    {
        if (abs(t.x) > abs(t.y))
        {
            f.y = state.downPoint.y;
        }
        else
        {
            f.x = state.downPoint.x;
        }
    }

    setTranslation(f);
}

\: dragMove (void; Event event)
{
    dragMoveLocked(false, event);
}

\: hasDispConversion (MenuStateFunc; string value)
{
    \: (int;)
    {
        let ret = DisabledMenuState;
        try
        {
            let s = getIntProperty("@RVDisplayColor.color.sRGB").front(),
                r = getIntProperty("@RVDisplayColor.color.Rec709").front(),
                g = getFloatProperty("@RVDisplayColor.color.gamma").front();

            if ((value == "sRGB"      && r == 0 && s == 1 && g == 1.0) ||
                (value == ""          && r == 0 && s == 0 && g == 1.0) ||
                (value == "Rec709"    && r == 1 && s == 0 && g == 1.0) ||
                (value == "Gamma 2.2" && r == 0 && s == 0 && g == 2.2) ||
                (value == "Gamma 2.4" && r == 0 && s == 0 && g == 2.4))
            {
                ret = CheckedMenuState;
            }
            else 
            {
                ret = UncheckedMenuState;
            }
        }
        catch (...)
        {
            ; // Going to return DisabledMenuState after this
        }
        return ret;
    };
}

\: hasLinConversion (MenuStateFunc; string value)
{
    \: (int;)
    {
        let ret = DisabledMenuState;
        try
        {
            int a = getIntProperty("#RVLinearize.color.active").front();
            if (a == 0) return DisabledMenuState;

            let l = getIntProperty("#RVLinearize.color.logtype").front(),
                s = getIntProperty("#RVLinearize.color.sRGB2linear").front(),
                r = getIntProperty("#RVLinearize.color.Rec709ToLinear").front(),
                g = getFloatProperty("#RVLinearize.color.fileGamma").front();

            if ((value == "Cineon Log" && r == 0 && l == 1 && s == 0 && g == 1.0) ||
                (value == "Viper Log"  && r == 0 && l == 2 && s == 0 && g == 1.0) ||
                (value == "ALEXA LogC" && r == 0 && l == 3 && s == 0 && g == 1.0) ||
                (value == "ALEXA LogC Film" && r == 0 && l == 4 && s == 0 && g == 1.0) ||
                (value == "Sony S-Log" && r == 0 && l == 5 && s == 0 && g == 1.0) ||
                (value == "Red Log"    && r == 0 && l == 6 && s == 0 && g == 1.0) ||
                (value == "Red Log Film"    && r == 0 && l == 7 && s == 0 && g == 1.0) ||
                (value == "sRGB"       && r == 0 && l == 0 && s == 1 && g == 1.0) ||
                (value == "Rec709"     && r == 1 && l == 0 && s == 0 && g == 1.0) ||
                (value == "Kodak Log"  && r == 0 && l == 1 && s == 0 && g == 1.0) || // For backwards compat; same as cineon
                (value == ""           && r == 0 && l == 0 && s == 0 && g == 1.0) ||
                (value == "Gamma 2.2"  && r == 0 && l == 0 && s == 0 && g == 2.2))
            {
                ret = CheckedMenuState;
            }
            else 
            {
                ret = UncheckedMenuState;
            }
        }
        catch (...)
        {
            ; // Going to return DisabledMenuState after this
        }
        return ret;
    };
}

\: setDispConvert (EventFunc; string type)
{
    \: (void; Event ev)
    {
        int s = 0;
        int r = 0;
        float g = 1.0;

        if      (type == "sRGB")      s = 1;
        else if (type == "Rec709")    r = 1;
        else if (type == "Gamma 2.2") g = 2.2;
        else if (type == "Gamma 2.4") g = 2.4;

        setIntProperty("@RVDisplayColor.color.sRGB", int[] {s});
        setIntProperty("@RVDisplayColor.color.Rec709", int[] {r});
        setFloatProperty("@RVDisplayColor.color.gamma", float[] {g});

        redraw();
    };
}

\: setLinConvert (EventFunc; string type)
{
    \: (void; Event ev)
    {
        if (filterLiveReviewEvents()) {
            sendInternalEvent("live-review-blocked-event");
            return;
        }
        int l = 0;
        int s = 0;
        int r = 0;
        float g = 1.0;

        if ((type == "Cineon Log") || (type == "Kodak Log"))  l = 1;
        else if (type == "Viper Log")  l = 2;
        else if (type == "ALEXA LogC") l = 3;
        else if (type == "ALEXA LogC Film") l = 4;
        else if (type == "SONY S-Log") l = 5;
        else if (type == "Red Log") l = 6;
        else if (type == "Red Log Film") l = 7;
        else if (type == "sRGB"     )  s = 1;
        else if (type == "Rec709"   )  r = 1;
        else if (type == "Gamma 2.2" ) g = 2.2;

        try
        {
            setIntProperty("#RVLinearize.color.logtype", int[] {l});
            setIntProperty("#RVLinearize.color.sRGB2linear", int[] {s});
            setIntProperty("#RVLinearize.color.Rec709ToLinear", int[] {r});
            setFloatProperty("#RVLinearize.color.fileGamma", float[] {g});

            if (type != "") displayFeedback("%s -> Linear" % type);
            else displayFeedback("No Conversion to Linear");
            redraw();
        }
        catch (exception exc)
        {
            let sexc = string(exc);
            displayFeedback("Unable to set linear conversion: %s" % sexc);
        }
    };
}

\: toggleLUT (void; string lutType, string lutKind) // Display or Color
{
    if (filterLiveReviewEvents()) {
        sendInternalEvent("live-review-blocked-event");
        return;
    }
    State state = data();

    string node;

    case (lutType)
    {
        "Display"   -> { node = "@RVDisplayColor"; }
        "Color"     -> { node = if lutKind == "Luminance" then "#RVColor" else "#RVLinearize"; }
        "Look"      -> { node = "#RVLookLUT"; }
        "Cache"     -> { node = "#RVCacheLUT"; }
    }

    try
    {
        let comp = if lutKind == "Luminance" then "luminanceLUT" else "lut",
            p    = getIntProperty("%s.%s.active" % (node, comp)).front();

        setIntProperty("%s.%s.active" % (node, comp),
                       int[] {if p == 0 then 1 else 0});

        if (p == 0)
        {
            let n = getStringProperty("%s.%s.name" % (node, comp)).front();
            displayFeedback(if n == "" 
                                then ("%s %s LUT is ON" % (lutType, lutKind)) 
                                else ("Using %s LUT: %s" % (lutType, n)));
        }
        else
        {
            displayFeedback("%s %s LUT is OFF" % (lutType, lutKind));
        }

        redraw();
    }
    catch (exception exc)
    {
        let sexc = string(exc);
        displayFeedback("Unable to toggle %s LUT: %s" % (lutType, sexc));
    }
}

\: toggleCacheLUT (void;) { toggleLUT("Cache", "File"); }
\: toggleFileLUT (void;) { toggleLUT("Color", "File"); }
\: toggleLookLUT (void;) { toggleLUT("Look", "File"); }
\: toggleLuminanceLUT (void;) { toggleLUT("Color", "Luminance"); }
\: toggleDisplayLUT (void;) { toggleLUT("Display", ""); }

\: toggleCDL (void; string cdlType, string cdlKind)
{
    if (filterLiveReviewEvents()) {
        sendInternalEvent("live-review-blocked-event");
        return;
    }
    State state = data();

    string node;

    case (cdlType)
    {
        "Color"     -> { node = "#RVLinearize"; }
        "Look"      -> { node = "#RVColor"; }
    }

    try
    {
        let comp = "CDL",
            p    = getIntProperty("%s.%s.active" % (node, comp)).front();

        setIntProperty("%s.%s.active" % (node, comp),
                       int[] {if p == 0 then 1 else 0});

        if (p == 0)
        {
            displayFeedback("%s %s CDL is ON" % (cdlType, cdlKind));
        }
        else
        {
            displayFeedback("%s %s CDL is OFF" % (cdlType, cdlKind));
        }

        redraw();
    }
    catch (exception exc)
    {
        let sexc = string(exc);
        displayFeedback("Unable to toggle %s CDL: %s" % (cdlType, sexc));
    }
}

\: toggleFileCDL (void;) { toggleCDL("Color", "File"); }
\: toggleLookCDL (void;) { toggleCDL("Look", "File"); }

\: toggleICC (void; string iccType, string iccKind)
{
    if (filterLiveReviewEvents()) {
        sendInternalEvent("live-review-blocked-event");
        return;
    }
    State state = data();

    string node;

    case (iccType)
    {
        "Display"   -> { node = "@ICCDisplayTransform"; }
        "Color"     -> { node = "#ICCLinearizeTransform"; }
    }

    try
    {
        let comp = "node",
            p    = getIntProperty("%s.%s.active" % (node, comp)).front();

        setIntProperty("%s.%s.active" % (node, comp),
                       int[] {if p == 0 then 1 else 0});

        if (p == 0)
        {
            displayFeedback("%s %s ICC is ON" % (iccType, iccKind));
        }
        else
        {
            displayFeedback("%s %s ICC is OFF" % (iccType, iccKind));
        }

        redraw();
    }
    catch (exception exc)
    {
        let sexc = string(exc);
        displayFeedback("Unable to toggle %s ICC: %s" % (iccType, sexc));
    }
}

\: toggleFileICC (void;) { toggleICC("Color", "File"); }
\: toggleDisplayICC (void;) { toggleICC("Display", ""); }

\: toggleInvert (void;)
{
    if (filterLiveReviewEvents()) {
        sendInternalEvent("live-review-blocked-event");
        return;
    }
    State state = data();

    try
    {
        let p = getIntProperty("#RVColor.color.invert").front();
        setIntProperty("#RVColor.color.invert", int[] {if p == 0 then 1 else 0});
        displayFeedback(if p == 0 then "Invert Display" else "No Invert Display");
        redraw();
    }
    catch (exception exc)
    {
        let sexc = string(exc);
        displayFeedback("Unable to toggle invert: %s" % sexc);
    }
}

\: togglePremult (void;)
{
    State state = data();

    let p = getIntProperty("@RVDisplayColor.color.premult").front();
    setIntProperty("@RVDisplayColor.color.premult", int[] {if p != 1 then 1 else 0});
    displayFeedback(if p == 0 then "Premult Display" else "No Premult Display");
    redraw();
}

\: toggleUnpremult (void;)
{
    State state = data();

    let p = getIntProperty("@RVDisplayColor.color.premult").front();
    setIntProperty("@RVDisplayColor.color.premult", int[] {if p != -1 then -1 else 0});
    displayFeedback(if p == 0 then "Unpremult Display" else "No Unpremult Display");
    redraw();
}


\: beginning (void;)
{
    if (filterLiveReviewEvents()) {
        sendInternalEvent("live-review-blocked-event");
        return;
    }
    if (isPlaying()) stop();
    setFrame(inPoint());
    redraw();
}

\: ending (void;)
{
    if (filterLiveReviewEvents()) {
        sendInternalEvent("live-review-blocked-event");
        return;
    }
    if (isPlaying()) stop();
    setFrame(outPoint());
    redraw();
}

\: toggleMark (void;)
{
    markFrame(frame(), !isMarked(frame()));
    redraw();
}

\: markSequence (void;)
{
    let smarks = sequenceBoundaries();
    for_each (m; smarks) markFrame(m, true);
    redraw();
}

\: markAnnotatedFrames (void;)
{
    for_each (f; findAnnotatedFrames()) markFrame(f, true);
    redraw();
}

\: clearAllMarks (void;)
{
    let marks = markedFrames();
    for_each (m; marks) markFrame(m, false);
    redraw();
}

\: clearMarksInRange (void;)
{
    let marks = markedFrames(),
        inp   = inPoint(),
        outp  = outPoint();

    for_each (m; marks)
    {
        if (m >= inp && m <= outp) markFrame(m, false);
    }

    redraw();
}

\: markedBoundariesAroundFrame ((int,int); int f, bool inclusive=true)
{
    let mframes     = markedFrames(),
        marks       = if mframes.empty() then sequenceBoundaries() else mframes,
        i           = lower_bounds(marks, f),
        fs          = frameStart(),
        fe          = frameEnd(),
        m0Ex        = if i < 0 then fs else (if f != marks[i] then marks[i] else (if i == 0 then fs else marks[i-1])),
        m0Inc       = if i < 0 then fs else marks[i],
        m0          = if inclusive then m0Inc else m0Ex,
        m1          = if i + 1 >= marks.size() then fe+1 else marks[i+1];

    (m0, m1);
}

\: setInOutMarkedRangeAtFrame (void; int f)
{
    let (start,end) = markedBoundariesAroundFrame(f),
        playing = (isPlaying() || isBuffering());

    if (playing) stop();

    setInPoint(start);
    setOutPoint(end - 1);
    setFrame(start);

    if (playing) play();

    redraw();
}

\: setInOutMarkedRange (void;)
{
    setInOutMarkedRangeAtFrame(frame());
}

\: nextView (void;)
{
    if (nextViewNode() neq nil) setViewNode(nextViewNode());
}

\: prevView (void;)
{
    if (previousViewNode() neq nil) setViewNode(previousViewNode());
}

\: nextMarkedFrame (void;)
{
    if (filterLiveReviewEvents()) {
        sendInternalEvent("live-review-blocked-event");
        return;
    }
    try
    {
        let (_,end) = markedBoundariesAroundFrame(frame());
        setFrame(end);
    }
    catch (...)
    {
        setFrame(frameEnd());
    }
    redraw();
}

\: previousMarkedFrame (void;)
{
    if (filterLiveReviewEvents()) {
        sendInternalEvent("live-review-blocked-event");
        return;
    }
    try
    {
        let (start,_) = markedBoundariesAroundFrame(frame(), false);
        setFrame(start);
    }
    catch (...)
    {
        setFrame(frameStart());
    }
    redraw();
}

\: nextMatchedFrame (void;)
{
    if (filterLiveReviewEvents()) {
        sendInternalEvent("live-review-blocked-event");
        return;
    }
    try
    {
        if (nodeType(viewNode()) != "RVSequenceGroup") return;

        let boundaries  = sequenceBoundaries(),
            currentLocal = sourceFrame(frame());

        int globalStart;
        int globalEnd;

        for (int b = 0; b < boundaries.size(); b++)
        {
            if (boundaries[b] > frame())
            {
                int i = b;
                if (b == boundaries.size() - 1) i = 0;
                globalStart = boundaries[i];
                globalEnd   = boundaries[i+1] - 1;
                break;
            }
        }
        setMatchedFrame(currentLocal, globalStart, globalEnd);
    }
    catch (...)
    {
        setFrame(frameEnd());
    }

    redraw();
}

\: previousMatchedFrame (void;)
{
    if (filterLiveReviewEvents()) {
        sendInternalEvent("live-review-blocked-event");
        return;
    }
    try
    {
        if (nodeType(viewNode()) != "RVSequenceGroup") return;

        let boundaries  = sequenceBoundaries(),
            currentLocal = sourceFrame(frame());

        int globalStart;
        int globalEnd;

        for (int b = (boundaries.size() - 1); b >= 0; b--)
        {
            if (boundaries[b] <= frame())
            {
                int i = b;
                if (b == 0) i = -1;
                globalEnd = boundaries[i] - 1;
                globalStart = boundaries[i-1];
                break;
            }
        }
        setMatchedFrame(currentLocal, globalStart, globalEnd);
    }
    catch (...)
    {
        setFrame(frameStart());
    }

    redraw();
}

\: setMatchedFrame (void; int currentLocal, int globalStart, int globalEnd)
{
    if (filterLiveReviewEvents()) {
        sendInternalEvent("live-review-blocked-event");
        return;
    }
    let localStart = sourceFrame(globalStart),
        localEnd   = sourceFrame(globalEnd);

    if (currentLocal >= localStart && currentLocal <= localEnd)
    {
        setFrame(globalStart + (currentLocal - localStart));
    }
    else if (currentLocal < localStart)
    {
        setFrame(globalStart);
    }
    else
    {
        setFrame(globalEnd);
    }
}

\: nextMarkedRange (void;)
{
    try
    {
        let (start,end) = markedBoundariesAroundFrame(frame());

        if (start != inPoint() || end != outPoint()+1)
        {
            setInPoint(start);
            setOutPoint(end-1);
        }
        else
        {
            let p = end-1;
            setInOutMarkedRangeAtFrame(if p >= (frameEnd()) 
                                       then frameStart() else p + 1);
        }
    }
    catch  (...)
    {
        setInPoint(frameStart());
        setOutPoint(frameEnd());
    }
}

\: previousMarkedRange (void;)
{
    try
    {
        let (start,end) = markedBoundariesAroundFrame(frame());

        if (start != inPoint() || end != outPoint()+1)
        {
            setInPoint(start);
            setOutPoint(end-1);
        }
        else
        {
            let p = start;
            setInOutMarkedRangeAtFrame(if p <= frameStart() 
                                       then frameEnd() else p - 1);
        }
    }
    catch (...)
    {
        setInPoint(frameStart());
        setOutPoint(frameEnd());
    }
}

\: expandMarkedRange (void;)
{
    try
    {
        let marks = markedFrames();
        int[] allMarks = {};
        allMarks.push_back(frameStart());
        for_each (mark; marks)
        {
            allMarks.push_back(mark);
        }
        allMarks.push_back(frameEnd());
        let boundaries = if marks.empty() then sequenceBoundaries() else allMarks,
            newIn = inPoint(),
            newOut = outPoint(),
            newOutFound = false;
        for_each (boundary; boundaries)
        {
            if (boundary < inPoint())
            {
                newIn = boundary;
            }
            if (boundary > (outPoint()+1) && !newOutFound)
            {
                newOut = boundary;
                newOutFound = true;
            }
        }
        if (newOut == frameEnd())
        {
            newOut = frameEnd() + 1;
        }
        setInPoint(newIn);
        setOutPoint(newOut-1);
    }
    catch  (...)
    {
        setInPoint(frameStart());
        setOutPoint(frameEnd());
    }

    if (isPlaying() && audioCacheMode() == CacheGreedy)
    {
        stop();
        play();
    }
}

\: contractMarkedRange (void;)
{
    try
    {
        let marks = markedFrames(),
            boundaries = if marks.empty() then sequenceBoundaries() else marks,
            newIn = inPoint(),
            newOut = outPoint(),
            newInFound = false;
        for_each (boundary; boundaries)
        {
            if (boundary > inPoint() && !newInFound)
            {
                newIn = boundary;
                newInFound = true;
            }
            if (boundary < (outPoint()+1))
            {
                newOut = boundary;
            }
        }
        if (newIn < newOut-1)
        {
            setInPoint(newIn);
            setOutPoint(newOut-1);
        }
    }
    catch  (...)
    {
        setInPoint(frameStart());
        setOutPoint(frameEnd());
    }

    if (isPlaying() && audioCacheMode() == CacheGreedy)
    {
        stop();
        play();
    }
}

\: showChannel (VoidFunc; int ch)
{
    \: (void;)
    {
        State state = data();

        string propertyName = "OCIODisplay.color.channelFlood";
        if (!propertyExists("@%s" % propertyName))
        {
            propertyName = "RVDisplayColor.color.channelFlood";
        }

        let v = getIntProperty("@%s" % propertyName).front();
        let nch = if v == ch then 0 else ch;
        setIntProperty("#%s" % propertyName, int[] {nch});
        let (name, glyphFunc) = showChannelGlyphs[nch];
        displayFeedback(name, 2, glyphFunc);
        redraw();
    };
}

\: channelOrder (VoidFunc; string order)
{
    \: (void;)
    {
        State state = data();

        string propertyName = "OCIODisplay.color.channelOrder";
        if (!propertyExists("@%s" % propertyName))
        {
            propertyName = "RVDisplayColor.color.channelOrder";
        }

        setStringProperty("#%s" % propertyName, string[] {order});
        displayFeedback("Channel Order => %s" % order);
        redraw();
    };
}

\: resetInOutPoints (void;)
{
    //
    // Check to see if we are resetting the in/out points
    // or storing them by comparing the values to media
    // start and end points. If they don't match then we
    // are resetting the in/out points, but if they do
    // then we are restoring the old values.
    //

    State state = data();
    
    let in    = inPoint(),
        out   = outPoint(),
        start = frameStart(),
        end   = frameEnd();

    if (in != start || out != end)
    {
        state.savedInOut.clear();
        state.savedInOut.push_back(in);
        state.savedInOut.push_back(out);
        setInPoint(start);
        setOutPoint(end);
    }
    else if (state.savedInOut.size() == 2)
    {
        setInPoint(state.savedInOut[0]);
        setOutPoint(state.savedInOut[1]);
    }
}

\: resetRange (void;)
{
    narrowToRange(frameStart(), frameEnd());
}

\: narrowToInOut (void;)
{
    narrowToRange(inPoint(), outPoint());
}

\: alphaTypeFunc (VoidFunc; int alphaType)
{
    \: (void;)
    {
        try
        {
            setIntProperty("#RVLinearize.color.alphaType", int[] {alphaType});
            redraw();
        }
        catch (exception exc)
        {
            let sexc = string(exc);
            displayFeedback("Unable to set alpha type: %s" % sexc);
        }
    };
}

\: pixelAspectFunc (VoidFunc; float a)
{
    \: (void;)
    {
        for_each (mi; metaEvaluate(frame())) 
        {
            if (mi.nodeType == "RVLensWarp" && 
                nodeType(nodeGroup(nodeGroup(mi.node))) == "RVSourceGroup")
            {
                setFloatProperty(mi.node + ".warp.pixelAspectRatio", float[] {a});
            }
        }

        redraw();
    };
}

\: setBitDepthFunc (VoidFunc; int depth)
{
    \: (void;)
    {
        setIntProperty("#RVFormat.color.maxBitDepth", int[] {depth});
        reload();
    };
}

\: setFPSFunc (VoidFunc; float rate)
{
    \: (void;) { setFPS(rate); redraw(); };
}

\: bitDepthState (MenuStateFunc; int depth)
{
    \: (int;)
    {
        try
        {
            let d = getIntProperty("#RVFormat.color.maxBitDepth").front();
            return if d == depth then CheckedMenuState else UncheckedMenuState;
        }
        catch (...)
        {
            return DisabledMenuState;
        }
    };
}

\: rotateState (MenuStateFunc; float deg, string prop="#RVTransform2D.transform.rotate")
{
    \: (int;)
    {
        try
        {
            let r = getFloatProperty(prop);
            return if r.front() == deg then CheckedMenuState else UncheckedMenuState;
        }
        catch (...)
        {
            return DisabledMenuState;
        }
    };
}

\: matteAspectState (MenuStateFunc; float maspect)
{
    \: (int;)
    {
        State state = data();
        float smaspect = state.matteAspect;
        if (!state.showMatte && maspect == 0.0) return CheckedMenuState;
        if (!state.showMatte) return UncheckedMenuState;
        if (maspect == -1.0  &&
            smaspect != 0.0  &&
            smaspect != 1.33 &&
            smaspect != 1.66 &&
            smaspect != 1.77 &&
            smaspect != 1.85 &&
            smaspect != 2.35 &&
            smaspect != 2.40) return CheckedMenuState;
        if smaspect == maspect then CheckedMenuState else UncheckedMenuState;
    };
}

\: matteOpacityState (MenuStateFunc; float mop)
{
    \: (int;)
    {
        State state = data();
        float smop = state.matteOpacity;
        if (mop == -1.0  &&
            smop != 0.33 &&
            smop != 0.66 &&
            smop != 1.0) return CheckedMenuState;
        if smop == mop then CheckedMenuState else UncheckedMenuState;
    };
}

\: setMatte (VoidFunc; float maspect, bool resetCenter=true)
{
    \: (void;)
    {
        State state = data();
        state.showMatte = maspect > 0.0;
        setIntProperty("#Session.matte.show", int[]{if state.showMatte then 1 else 0}, true);

        if (state.showMatte)
        {
            state.matteAspect = maspect;
            setFloatProperty("#Session.matte.aspect", float[]{state.matteAspect}, true);
            if (resetCenter)
            {
                resetSessionMatteCenter();
                displayFeedback("Matte Aspect Ratio %g" % maspect);
            }
        }
        else
        {
            displayFeedback("No Matte");
        }

        redraw();
    };
}

\: setMatteOpacity (VoidFunc; float mop)
{
    \: (void;)
    {
        State state = data();
        state.matteOpacity = mop;
        writeSetting ("View", "matteOpacity", SettingsValue.Float(state.matteOpacity));
        setFloatProperty("#Session.matte.opacity", float[]{state.matteOpacity}, true);
        redraw();
    };
}

\: setBGPattern (void; Event event, string bgmethod)
{
    setBGMethod(bgmethod);
    redraw();
}

\: testBGPattern ((int;); string bgmethod)
{
    \: (int;)
    {
        try
        {
            if (bgMethod() == bgmethod) return CheckedMenuState; 
            else                        return UncheckedMenuState;
        }
        catch (...)
        {
            return DisabledMenuState;
        }
    };
}

\: setDither (void; Event event, int bits)
{
    string propertyName = "@OCIODisplay.color.dither";
    if (!propertyExists(propertyName))
    {
        propertyName = "@RVDisplayColor.color.dither";
    }

    set(propertyName, bits);
    redraw();
}

\: testDither ((int;); int bits)
{
    \: (int;)
    {
        try
    {
            string propertyName = "@OCIODisplay.color.dither";
            if (!propertyExists(propertyName))
            {
                propertyName = "@RVDisplayColor.color.dither";
            }

            let p = getIntProperty(propertyName).front();
            return if p == bits then CheckedMenuState else UncheckedMenuState;
        }
        catch (...)
        {
            return DisabledMenuState;
        }
    };
}

\: rotateImage (VoidFunc; 
                float deg, 
                bool additive,
                string prop="#RVTransform2D.transform.rotate")
{
    \: (void;)
    {
        let r = getFloatProperty(prop)[0],
            d = if additive then r + deg else deg;
        setFloatProperty(prop, float[] {d});
        redraw();
    };
}

\: showFPS (void;)
{
    print("fps = %f\n" % fps());
}

\: ddNoop (void; int a, string b) {;}

\: ddCancelled (void; string b)
{
    throw "Drop Cancelled";
}

\: dragShow (void; Event event)
{
    State state = data();
    recordPixelInfo(event);
    redraw();
}

\: dragEnter (void; Event event)
{
    State state = data();
    state.dragDropOccuring = true;
    state.ddType = event.contentType();
    state.ddContent = event.contents();
    state.ddProgressiveDrop = false;
    state.ddDropFiles = string[]();
    state.ddDropFunc = nil;

    if (state.ddType == Event.FileObject)
    {
        try
        {
            string filepath = sendInternalEvent ("incoming-source-path", state.ddContent + ";;drag", "rvui");
            state.ddFileKind = fileKind(if (filepath neq nil && filepath != "") then filepath else state.ddContent);
        }
        catch (...)
        {
            state.ddFileKind = UnknownFileKind;
        }
    }

    dragShow(event);
}

\: dragLeave (void;)
{
    State state = data();
    displayFeedback("");
    state.dragDropOccuring = false;
    redraw();
}

\: dragRelease (void; Event event)
{
    State state = data();
    state.dragDropOccuring = false;
    recordPixelInfo(event);

    let kind = event.contentType(),
        F    = if state.ddDropFunc eq nil 
                    then ddNoop(0,)
                    else state.ddDropFunc(state.ddRegion,);

    \: loadFile (void; string file, (void;string) Fin = nil)
    {
        let root = string.split(file, "/").back(),
            ext  = string.split(root, ".").back(),
            f    = frameEnd(),
            F    = if Fin eq nil then (\: (void; string s) { addSources(string[] {s},"drop"); }) else Fin;

        try
        {
            F(file);
        }
        catch (string msg)
        {
            displayFeedback(msg);
        }
        catch (...)
        {
            alertPanel(true,
                       ErrorAlert,
                       "ERROR: While reading file:",
                       "%s\n" % file,
                       "Ok", nil, nil);
        }
    }

    if (kind == Event.FileObject)
    {
        // Transform each incoming path by the incoming-source-path so that it can be transformed before loading, similar to drag func.
        string filepath = sendInternalEvent ("incoming-source-path", event.contents() + ";;drop", "rvui");
        filepath = if (filepath neq nil && filepath != "") then filepath else event.contents();

        if (state.ddProgressiveDrop)
        {
            state.ddDropFiles.push_back(filepath);
            qt.QTimer dt = state.ddDropTimer;
            dt.start();
        }
        else loadFile(filepath, F);
    }
    else if (kind == Event.URLObject ||
             kind == Event.TextObject)  //  Chrome URL drops come accross as "Text"
    {
        try
        {
            let url      = event.contents(),
                mods     = event.modifiers(),
                re       = regex("([a-zA-Z]+):([^\r\n]+)"),
                parts    = re.smatch(url),
                protocol = parts[1],
                filepath = parts[2];

            if (state.ddDropFunc neq nil)
            //
            //  Some mode has claimed to handle this url, so let it handle it.
            //
            {
                state.ddDropFunc (state.ddRegion, url);
            }
            else if (protocol == "file")
            {
                loadFile(filepath, F);
            }
            else if (protocol == "http")
            {
                let temp = "/tmp/" + regex.replace("/", filepath, "_");
                //  XXX linux only, must be some Qtish way to do this ...
                system.system("curl -o %s %s" % (temp, url));
                loadFile(temp, F);
            }
            else if (protocol == "rvnode")
            {
                F(event.contents());
            }
            else
            {
                displayFeedback("Protocol %s is not implemented" % protocol);
            }
        }
        catch (exception exc)
        {
            alertPanel(true,
                       ErrorAlert,
                       "ERROR: While reading:",
                       "%s\n" % event.contents(),
                       "Ok", nil, nil);

            print("ERROR: \"%s\"\n" % event.contents());
        }
    }

    redraw();
}

\: getParamValue (float; )
{
    State state = data();
    let chan  = if (state.parameterChannel > 0) then state.parameterChannel else 0;
    return getFloatProperty(state.parameter)[chan];
}

\: beginParamScrub (void; Event event)
{
    State state = data();
    state.parameterValue = getParamValue();
}

\: setParamChannel (void; int chan, Event event)
{
    //
    // If the parameter was not a 3-channel float in the first place, don't
    // respond to requests to set the channel to be edited.
    //
    State state = data();
    if (state.parameterChannel == -1) return;

    if (state.parameterChannel >= 0 && chan < 0)
    {
        state.parameterChannel = -2;
    }
    else state.parameterChannel = chan;

    let (ch, txt, gf) = paramFeedbackData();

    displayFeedback("Edit %s (? for hotkey help)" % txt, 100000, gf);
    redraw();
}

\: selectParamTile (void; Event event)
{
    updatePixelInfo(nil);
    State state = data();

    if (state.parameterNode[0] != '#') return;

    let node = state.parameterNode,
        name = "";

    for_each (s; imagesAtPixel (state.pointerPosition, nil, true))
    {
        if (s.inside) 
        {
            let n = associatedNode (state.parameterNode, s.node);
            if (n != "")
            {
                node = n;
                name = "'%s' " % uiName(n);
                break;
            }
        }
    }
    let parts = string.split(state.parameter, "."),
        (ch, txt, gf) = paramFeedbackData();

    state.parameter = "%s.%s.%s" % (node, parts[1], parts[2]);
    displayFeedback("Edit %s%s (? for hotkey help)" % (name, txt), 100000, gf);
    redraw();
}

\: dragParamScrub (void; Event event)
{
    State state = data();
    let p  = getParamValue(),
        dp = event.pointer().x - event.reference().x,
        dx = event.domain().x,
        nf = dp / dx * state.parameterScale + state.parameterValue,
        (ch, txt, gf) = paramFeedbackData();

    if (nf < state.parameterMinValue) nf = state.parameterMinValue;

    if (state.parameterGranularity neq nil)
    {
        float prec = 1.0/state.parameterPrecision;
        nf = float(int(prec * nf / state.parameterGranularity() + 0.5)) / prec;

        displayFeedback("%s => %g" % (txt, nf), 10000, gf);
        nf *= state.parameterGranularity();
        float d = nf - p;
        nudgeProperty(state.parameter, nf, d, ch);
    }
    else
    {
        displayFeedback("%s => %.02f" % (txt, nf), 10000, gf);
        float d = nf - p;
        nudgeProperty(state.parameter, nf, d, ch);
    }

    if (regex.match("audio", state.parameter) && state.scrubAudio)
    {
        setAudioCacheMode(CacheBuffer);
        setAudioCacheMode(CacheGreedy);
    }
    redraw();
}

\: toggleParamLocked (void; Event event)
{
    let (ch, txt, gf) = paramFeedbackData();

    State state = data();
    state.parameterLocked = !state.parameterLocked;

    let lockText = if (state.parameterLocked) then "locked, ESC to exit" else "unlocked";

    displayFeedback("Edit %s (%s, ? for hotkey help)" % (txt, lockText), 100000, gf);
    redraw();
}

\: releaseParamForce (void; Event event)
{
    State state = data();
    state.parameterLocked = false;
    releaseParam(event);
}

\: releaseParam (void; Event event)
{
    //  Ignore shift keypress, since otherwise can't get '?' or 'R' or '+' or '-'
    if (event neq nil && event.name() == "key-down--shift--shift") return;

    State state = data();
    if (state.parameterLocked) return;

    let nf = getParamValue(),
        (ch, txt, gf) = paramFeedbackData();

    if (state.parameterGranularity neq nil)
    {
        float prec = 1.0/state.parameterPrecision;
        nf = float(int(prec * nf / state.parameterGranularity() + 0.5)) / prec;

        displayFeedback("Finished Editing %s => %g" % (txt, nf), 2.0, gf);
    }
    else
    {
        displayFeedback("Finished Editing %s => %.02f" % (txt, nf), 2.0, gf);
    }
    popEventTable("paramscrub");
    redraw();
}

\: incrementParam (void; Event event)
{
    State state = data();
    let p  = getParamValue(),
        nf = state.parameterScale * .05 + p,
        (ch, txt, gf) = paramFeedbackData();

    if (nf < state.parameterMinValue) nf = state.parameterMinValue;

    if (state.parameterGranularity neq nil)
    {
        float prec = 1.0/state.parameterPrecision;
        float rounder = if (p < 0.0) then 0.5 else 1.5;
        nf = float(int(prec * p/state.parameterGranularity() + rounder)) / prec;
        nf *= state.parameterGranularity();

        displayFeedback("%s => %g" % (txt, nf), 10000, gf);
        float d = nf - p;
        nudgeProperty(state.parameter, nf, d, ch);
    }
    else
    {
        displayFeedback("%s => %.02f" % (txt, nf), 10000, gf);
        float d = nf - p;
        nudgeProperty(state.parameter, nf, d, ch);
    }

    if (regex.match("audio", state.parameter) && state.scrubAudio)
    {
        setAudioCacheMode(CacheBuffer);
        setAudioCacheMode(CacheGreedy);
    }
    redraw();
}

\: decrementParam (void; Event event)
{
    State state = data();
    let p  = getParamValue(),
        nf = -state.parameterScale * .05 + p,
        (ch, txt, gf) = paramFeedbackData();

    if (nf < state.parameterMinValue) nf = state.parameterMinValue;

    if (state.parameterGranularity neq nil)
    {
        float prec = 1.0/state.parameterPrecision;
        float rounder = if (p > 0.0) then 0.5 else 1.5;
        nf = float(int(prec * p/state.parameterGranularity() - rounder)) / prec;
        nf *= state.parameterGranularity();

        displayFeedback("%s => %g" % (txt, nf), 10000, gf);
        float d = nf - p;
        nudgeProperty(state.parameter, nf, d, ch);
    }
    else
    {
        displayFeedback("%s => %.02f" % (txt, nf), 10000, gf);
        float d = nf - p;
        nudgeProperty(state.parameter, nf, d, ch);
    }

    if (regex.match("audio", state.parameter) && state.scrubAudio)
    {
        setAudioCacheMode(CacheBuffer);
        setAudioCacheMode(CacheGreedy);
    }
    redraw();
}

\: resetParam (void; Event event)
{
    State state = data();
    setParamValue (string(state.parameterReset));
}

\: helpParam (void; Event event)
{
    State state = data();
    let (ch, txt, gf) = paramFeedbackData(),
        chHelp = if (ch != -1) then ", r/g/b=channel" else "",
        vnodeType = nodeType(viewNode()),
        selectable = (vnodeType == "RVStackGroup" || vnodeType == "RVLayoutGroup"),
        selHelp = if (selectable) then ", s=select tile" else "";

    displayFeedback("Edit %s: +/-/scrub/wheel=modify%s, DEL=reset, RET=input, ESC=quit, l=lock%s" %
            (txt, chHelp, selHelp), 10000);
    redraw();
}

\: releaseNudge (void; Event event)
{
    displayFeedback("Nudge Keys Off");
    popEventTable("nudge");
    redraw();
}

\: releaseStereo (void; Event event)
{
    displayFeedback("Stereo Keys Off");
    popEventTable("stereo");
    redraw();
}

\: clearEverything (void;)
{
    let mode = cacheMode(),
        presMode = presentationMode();

    sendInternalEvent ("before-session-clear-everything", "", "rvui");

    setPresentationMode(false);
    clearSession();
    setPresentationMode(presMode);
    State state = data();
    state.feedbackText  = "";
    state.feedback      = 0;
    state.feedbackGlyph = nil;
    state.pixelInfo     = nil;
    state.scrubAudio    = false;

    if (state.inspector neq nil && state.inspector.isActive()) 
    {
        state.inspector.toggle();
    }

    setMargins (Vec4 (0,0,0,0));
    setCacheMode (mode);

    sendInternalEvent ("session-clear-everything", "", "rvui");
}


\: getMediaFilesFromBrowser (string[]; )
{
    State state = data();

    string[] files = string[]();

    try
    {
        let F     = state.defaultOpenDir,
            ddir  = state.config.lastOpenDir,
            dir   = if (ddir eq nil && (F neq nil)) then F() else (if (ddir eq nil) then nil else ddir);

        files = contractSequences(openMediaFileDialog(false,
                                                      ManyExistingFiles,
                                                      "*",
                                                      dir,
                                                      "Add Image/Movie/Sequence(s)"));

        state.config.lastOpenDir = path.dirname(files.front());
    }
    catch (...)
    {
        displayFeedback("Cancelled");
        files.clear();
    }

    return files;
}

\: addMovieOrImageSources (void; Event ev, bool mark, bool merge)
{
    State state = data();

    let files = getMediaFilesFromBrowser();
    if (files.empty()) return;

    try
    {
        addSources(files, "explicit", false, merge);
    }
    catch (object obj)
    {
        displayFeedback("ERROR: open failed: %s" % string(obj));
    }

    redraw();
}

\: addMovieOrImage (void; Event ev, (void;string) addFunc, bool mark)
{
    State state = data();

    let files = getMediaFilesFromBrowser();
    if (files.empty()) return;

    try
    {
        for_each (file; files)
        {
            let empty = sources().empty(),
                f = frameEnd();

            addFunc(file);

            if (mark)
            {
                if (empty)
                {
                    setFrame(frameStart());
                }
                else
                {
                    setFrame(f + 1);
                    //markFrame(f + 1, true);
                    //markFrame(frameStart(), true);
                }
            }
        }
    }
    catch (object obj)
    {
        displayFeedback("ERROR: open failed: %s" % string(obj));
    }

    redraw();
}

\: relocateMovieOrImage (void; Event ev)
{
    State state = data();

    try
    {
        let sources   = sourcesRendered(),
            groupNode = nodeGroup(sources.front().node),
            source    = nodesInGroupOfType(
                groupNode,"RVFileSource").front(),
            movs      = getStringProperty(source + ".media.movie");

        if (movs.size() > 1)
        {
            alertPanel(true,
                       ErrorAlert,
                       "ERROR:",
                       "Relocate only available for sources with one clip",
                       "Ok", nil, nil);
            return;
        }

        string dir = state.config.lastOpenDir;
        string ofile = movs.front();
        if (path.exists(path.dirname(ofile))) dir = path.dirname(ofile);
        let file = contractSequences(
            openMediaFileDialog(false, OneExistingFile, "*", dir,
            "Relocate Image/Movie/Sequence")).front();

        try
        {
            let mode = cacheMode();
            setCacheMode(CacheOff);
            print("INFO: relocating %s -> %s\n" % (ofile, file));
            relocateSource(ofile, file);
            redraw();
            setCacheMode(mode);
        }
        catch (...)
        {
            alertPanel(true,
                       ErrorAlert,
                       "ERROR:",
                       "While relocating  " + path.basename(file),
                       "Ok", nil, nil);
        }
    }
    catch (exception exc)
    {
        let sexc = string(exc);

        if (regex.match("operation cancelled", sexc))
        {
            displayFeedback("Cancelled");
        }
    }
    catch (...)
    {
        displayFeedback("Failed to relocate media!");
    }
}

\: convertPathToUIName(string; string filepath)
{
    return path.basename(path.without_extension(filepath));
}

\: replaceSourceMedia(void; Event event)
{
    State state = data();

    try
    {
        string dir = state.config.lastOpenDir;
        let newfile = contractSequences(
            openMediaFileDialog(false, OneExistingFile, "*", dir,
            "Replace Image/Movie/Sequence")).front();
        if (newfile neq nil)
        {
            let sources   = sourcesRendered(),
                groupNode = nodeGroup(sources.front().node),
                source    = nodesInGroupOfType(
                    groupNode,"RVFileSource").front();

            let name = uiName(groupNode),
                movs = getStringProperty(source + ".media.movie"),
                base = convertPathToUIName(movs.front());
            if (base == name)
            {
                setUIName(groupNode, convertPathToUIName(newfile));
            }

            setSourceMedia(source, string[] {newfile});
        }
    }
    catch (exception exc)
    {
        let sexc = string(exc);

        if (regex.match("operation cancelled", sexc))
        {
            displayFeedback("Cancelled");
        }
    }
    catch (...)
    {
        displayFeedback("Failed to replace media!");
    }
}

\: openMovieOrImage (void; Event ev)
{
    State state = data();

    try
    {
        let f    = frameEnd(),
            F    = state.defaultOpenDir,
            ddir = state.config.lastOpenDir,
            dir  = if (ddir eq nil && (F neq nil)) 
                     then F() 
                     else (if (ddir eq nil) 
                               then nil
                               else ddir),
            files = contractSequences(openMediaFileDialog(false,
                                                          ManyExistingFiles,
                                                          "*",
                                                          dir,
                                                          "Open Image/Movie/Sequence(s) in New Session"));

        newSession(files);
        state.config.lastOpenDir = path.dirname(files.front());
    }
    catch (exception exc)
    {
        displayFeedback("Cancelled");
    }

    redraw();
}

\: openCDLFile ((void;Event); string nodeName)
{
    \: (void; Event ev)
    {
        State state = data();
        string[] files;
        string file;

        try
        {
            let F    = state.defaultLUTDir,
                ddir = state.config.lastLUTDir,
                dir  = if (ddir eq nil && (F neq nil)) 
                            then F() 
                            else (if (ddir eq nil) 
                                  then nil
                                  else ddir);

            files = openFileDialog(false, false, false,
                                   [("cdl", "Color Decision List (cdl)"),
                                    ("cc", "Color Correction (cc)"),
                                    ("ccc", "Color Correction Collection (ccc)")],
                                   dir);
            file = files.front();
                                                                     
            readCDL(file, nodeName, true);
            displayFeedback("%s CDL" % path.basename(file));

            state.config.lastLUTDir = path.dirname(file);
        }
        catch (exception exc)
        {
            let sexc = string(exc);

            if (regex.match("operation cancelled", sexc))
            {
                displayFeedback("Cancelled");
            }
            else
            {
                alertPanel(true,
                           ErrorAlert,
                           "ERROR:",
                           "Cannot read CDL " + path.basename(file),
                           "Ok", nil, nil);
            }
        }
        catch (...)
        {
            alertPanel(true,
                       ErrorAlert,
                       "ERROR: Reading CDL file " + path.basename(file),
                       "Uncaught Exception",
                       "Ok", nil, nil);
        }

        redraw();
    };
}

\: openLUTFile ((void;Event); string nodeName)
{
    \: (void; Event ev)
    {
        State state = data();
        string[] files;
        string file;

        try
        {
            let F    = state.defaultLUTDir,
                ddir = state.config.lastLUTDir,
                dir  = if (ddir eq nil && (F neq nil)) 
                            then F() 
                            else (if (ddir eq nil) 
                                  then nil
                                  else ddir);

            files = openFileDialog(false, false, false,
                                   [("csp", "Cinespace 3D LUT (csp)"),
                                    ("3dl", "Autodesk Lustre 3DL (3dl)"),
                                    ("cube", "IRIDAS Cube LUT (cube)"),
                                    ("rvlut", "RV Channel LUT (rvlut)"),
                                    ("rvchlut", "RV Channel LUT (rvchlut)"),
                                    ("rv3dlut", "RV 3D LUT (rv3dlut)"),
                                    ("vf", "Nuke 3D LUT (vf)"),
                                    ("txt", "Shake LUT (txt)"),
                                    ("a3d", "Panavision LUT (a3d)"),
                                    ("mga", "Apple Color LUT (mga)")],
                                   dir);
            file = files.front();
                                                                     
            readLUT(file, nodeName, true);
            displayFeedback("%s LUT" % path.basename(file));

            state.config.lastLUTDir = path.dirname(file);
        }
        catch (exception exc)
        {
            let sexc = string(exc);

            if (regex.match("operation cancelled", sexc))
            {
                displayFeedback("Cancelled");
            }
            else
            {
                alertPanel(true,
                           ErrorAlert,
                           "ERROR:",
                           "Cannot read LUT " + path.basename(file),
                           "Ok", nil, nil);
            }
        }
        catch (...)
        {
            alertPanel(true,
                       ErrorAlert,
                       "ERROR: Reading LUT file " + path.basename(file),
                       "Uncaught Exception",
                       "Ok", nil, nil);
        }

        redraw();
    };
}


\: openOTIOFile ((void;Event);)
{
    \: (void; Event ev)
    {
        State state = data();
        string[] files;
        string file;

        try
        {
            let F    = state.defaultOpenDir,
                ddir = state.config.lastOpenDir,
                dir  = if (ddir eq nil && (F neq nil)) 
                            then F() 
                            else (if (ddir eq nil) 
                                  then nil
                                  else ddir);

            files = openFileDialog(false, false, false, [("otio", "OpenTimelineIO File (otio)")], dir);
            file = files.front();

            addSourceVerbose(string[]{file});
                                                                     
            state.config.lastOpenDir = path.dirname(file);
        }
        catch (exception exc)
        {
            let sexc = string(exc);

            if (regex.match("operation cancelled", sexc))
            {
                displayFeedback("Cancelled");
            }
            else
            {
                alertPanel(true,
                           ErrorAlert,
                           "ERROR:",
                           "Cannot read OTIO " + path.basename(file),
                           "Ok", nil, nil);
            }
        }
        catch (...)
        {
            alertPanel(true,
                       ErrorAlert,
                       "ERROR: Reading OTIO file " + path.basename(file),
                       "Uncaught Exception",
                       "Ok", nil, nil);
        }

        redraw();
    };
}


\: parseSimpleEDL (void; string filename)
{
    use io;

    let file  = ifstream(filename),
        all   = read_all(file),
        lines = string.split(all, "\n\r"),
        re    = regex("^[ \t]*\"(.+)\"[ \t]+" +   // path to movie or seq
                      "(-?[0-9]+)[ \t]+" +  // frame in
                      "(-?[0-9]+)"),  // frame out
        cre   = regex("^#.*$");

    file.close();

    string[] sourceNames;

    int[] sourceNums;
    int[] inpoints;
    int[] outpoints;
    int[] incs;
    int[] frames;
    int[] outframes;

    for_each (line; lines)
    {
        string movie;

        try
        {
            //  First check for a comment
            //
            if (cre.match(line)) continue;

            let tokens   = re.smatch(line);
            if (tokens eq nil) throw exception ("edl line '%s' has incorrect format" % line);

            let inpoint  = int(tokens[2]),
                outpoint = int(tokens[3]);

            movie = tokens[1];
            sourceNames.push_back(nodeGroup(addSourceVerbose(string[]{movie})));

            sourceNums.push_back(sourceNums.size());
            inpoints.push_back(inpoint);
            outpoints.push_back(outpoint);
            incs.push_back(1);
            if (frames.size() == 0) frames.push_back(inpoint);
            frames.push_back(outpoint - inpoint + 1 + frames.back());

            print("INFO: %s [%d, %d]\n" %
                  (path.basename(movie), inpoint, outpoint));
        }
        catch (exception exc)
        {
            int b = alertPanel(true,
                               ErrorAlert,
                               "ERROR: ",
                               "Problem in EDL %s: %s" %
                                    (path.basename(filename), string(exc)),
                               "Continue", "Stop reading EDL", nil);

            if (b == 1)
            {
                displayFeedback("Read Partial EDL");
                break;
            }
        }
        catch (...)
        {
            ;   // ignore lines we don't understand
        }
    }

    inpoints.push_back(0);
    outpoints.push_back(0);
    sourceNums.push_back(0);
    incs.push_back(0);

    let newSeq = newNode ("RVSequenceGroup", "SequenceGroup000000");
    extra_commands.setUIName(newSeq, path.basename(filename));

    setIntProperty(newSeq + "_sequence.edl.frame", frames, true);
    setIntProperty(newSeq + "_sequence.edl.in", inpoints, true);
    setIntProperty(newSeq + "_sequence.edl.out", outpoints, true);
    setIntProperty(newSeq + "_sequence.edl.source", sourceNums, true);

    setIntProperty(newSeq + "_sequence.mode.autoEDL", int[] {0}, true);

    setNodeInputs(newSeq, sourceNames);

    //setFrameStart(frames.front());
    //setFrameEnd(frames.back()-1);

    setViewNode(newSeq);

    resetInOutPoints();

    State state = data();
    if (state.timeline neq nil && !state.timeline.isActive()) toggleTimeline();
    if (state.imageInfo neq nil && state.imageInfo.isActive()) toggleInfo();
    if (state.sourceDetails neq nil && state.sourceDetails.isActive()) toggleSourceDetails();

    reloadInOut();
    redraw();
}


\: openSimpleEDL (void;)
{
    State state = data();

    try
    {
        let files = openFileDialog(false, false, false,
                                   [("*", "RV Simple EDL (any)")]);
        try
        {
            parseSimpleEDL(files.front());
        }
        catch (exception exc)
        {
            alertPanel(true,
                       ErrorAlert,
                       "ERROR: Reading EDL file",
                       "CAUGHT: %s\n" % exc,
                       "Ok", nil, nil);
        }
    }
    catch (...)
    {
        displayFeedback("Cancelled");
    }

    redraw();
}

\: tmpSessionCopyName(string; )
{
    let tmpDir = cacheDir(),
        sPath = sessionFileName(),
        sName = path.basename(sPath);

    if (path.extension(sName) == "rv")
    {
        let l = sName.size();
        sName = sName.substr(0,l-3);
    }

    let target = tmpDir + "/" + sName + "Copy" + ".rv",
        i = 2;
    while (path.exists(target))
    {
        target = tmpDir + "/" + sName + "Copy" + string(i) + ".rv";
        ++i;
    }
    return target;
}

\: cloneSession (void; Event ev)
{
    let target = tmpSessionCopyName();
    saveSession(target, true, true);
    newSession(string[] {target});
    //  XXX should delete target now ?  Not sure ...
}

\: cloneRV (void; Event ev)
{
    let target = tmpSessionCopyName(),
        rv     = system.getenv ("RV_APP_RV");

    saveSession(target, true, true);

    string[] arguments = { "%s" % (target) };
    // Quotes are not needed for program containing spaces with overloaded version of startDetached.
    qt.QProcess.startDetached (rv, arguments);
}

\: cloneSyncedRV (void; Event ev)
{
    //  Turn on network
    remoteNetwork(true);

    let myPort = myNetworkPort(),
        rv     = system.getenv ("RV_APP_RV");

    string[] arguments = { "-network", "-networkPort", "%s" % (myPort+1), "-networkConnect", "127.0.0.1", 
                           "%s" % (myPort), "-flags", "syncPullFirstfalse" };

    // Quotes are not needed for program containing spaces with overloaded version of startDetached.
    qt.QProcess.startDetached (rv, arguments);
}

\: save (void; Event ev)
{
    string n = sessionFileName();

    if (n == "Untitled" || path.extension(n) != "rv")
    {
        saveAs(ev);
    }
    else
    {
        saveSession(n);
    }
}

\: saveAs (void; Event ev)
{
    State state = data();

    try
    {
        string f = saveFileDialog(false, "rv|RV Session File");

        if (path.extension(f) != "rv")
        {
            saveSession(f + ".rv");
        }
        else
        {
            saveSession(f);
        }
    }
    catch (...)
    {
        displayFeedback("Cancelled");
    }

    redraw();
}

\: exportMarked (void; Event event)
{
    State state = data();

    if (state.externalProcess neq nil)
    {
        int choice = alertPanel(true, // associated panel (sheet on OSX)
                                WarningAlert,
                                "WARNING", "Another process is still running",
                                "OK", nil, nil);
        return;
    }

    try
    {
        runtime.load_module("export_utils");
        let Fname = runtime.intern_name("export_utils.exportMarkedFrames");
        (ExternalProcess;string,string) F = runtime.lookup_function(Fname);

        try
        {
            string f = saveFileDialog(false);
            let ext = path.extension(f);

            if (ext eq nil || ext == "")
            {
                alertPanel(true, // associated panel (sheet on OSX)
                           ErrorAlert,
                           "ERROR", 
                           "A file extension is required",
                           "OK", nil, nil);

                throw exception(); 
            }
            
            try
            {
                state.externalProcess = F(f, "default");
                toggleProcessInfo();
                redraw();
            }
            catch (...)
            {
                int choice = alertPanel(true, // associated panel (sheet on OSX)
                                        ErrorAlert,
                                        "ERROR", "Unable to call RVIO",
                                        "OK", nil, nil);
            }
        }
        catch (...)
        {
            displayFeedback("Cancelled");
        }
    }
    catch (string message)
    {
        displayFeedback("Cancelled%s" % (if message == "" then "" else " : " + message));
    }
    catch (...)
    {
        int choice = alertPanel(true, // associated panel (sheet on OSX)
                                ErrorAlert,
                                "ERROR", "Cannot locate export function",
                                "OK", nil, nil);
    }
}

\: exportAnnotatedFrames (void; Event event)
{
    let marks = markedFrames();
    clearAllMarks();
    markAnnotatedFrames();

    if (markedFrames().empty())
    {
        int choice = alertPanel(true, // associated panel (sheet on OSX)
                                ErrorAlert,
                                "No frames are annotated", "",
                                "OK", nil, nil);

        return;
    }

    exportMarked(event);
    clearAllMarks();
    for_each (f; marks) markFrame(f, true);
}

\: exportAs (void; Event ev, string requiredExt, string outputType)
{
    State state = data();

    if (state.externalProcess neq nil)
    {
        int choice = alertPanel(true, // associated panel (sheet on OSX)
                                WarningAlert,
                                "WARNING", "Another process is still running",
                                "OK", nil, nil);
        return;
    }

    try
    {
        runtime.load_module("export_utils");
        let Fname = runtime.intern_name("export_utils.exportMovieOverRange");
        (ExternalProcess;int,int,string,bool,string) F = runtime.lookup_function(Fname);

        try
        {
            string f = saveFileDialog(false);

            if (requiredExt neq nil)
            {
                let ext = path.extension(f);

                if (ext eq nil || ext == "")
                {
                    if (requiredExt != "*")
                    {
                        f += "." + requiredExt;
                    }
                    else
                    {
                        alertPanel(true, // associated panel (sheet on OSX)
                                   ErrorAlert,
                                   "ERROR", 
                                   "A file extension is required for %s" % outputType,
                                   "OK", nil, nil);

                        throw exception(); 
                    }
                }
            }
            
            try
            {
                state.externalProcess = F(inPoint(), outPoint(), f, false, "default");
                toggleProcessInfo();
                redraw();
            }
            catch (...)
            {
                int choice = alertPanel(true, // associated panel (sheet on OSX)
                                        ErrorAlert,
                                        "ERROR", "Unable to call RVIO for %s" % outputType,
                                        "OK", nil, nil);
            }
        }
        catch (...)
        {
            displayFeedback("Cancelled");
        }
    }
    catch (string message)
    {
        displayFeedback("Cancelled%s" % (if message == "" then "" else " : " + message));
    }
    catch (...)
    {
        int choice = alertPanel(true, // associated panel (sheet on OSX)
                                ErrorAlert,
                                "ERROR", "Cannot locate export function for %s" % outputType,
                                "OK", nil, nil);
    }
}

\: exportFrame (void; Event ev, (void;string) outputFunc)
{
    State state = data();

    try
    {
        string f = saveFileDialog(false);
        outputFunc(f);
    }
    catch (...)
    {
        displayFeedback("Cancelled");
    }
}

\: exportAttrs (void; Event ev)
{
    State state = data();

    try
    {
        string f   = saveFileDialog(false);
        ofstream o = ofstream(f);

        for_each (a; getCurrentAttributes())
        {
            print(o, "%s %s\n" % a);
        }

        o.close();
    }
    catch (...)
    {
        displayFeedback("Cancelled");
    }

    redraw();
}

\: exportNodeDefinition (void; bool all, Event ev)
{
    try
    {
        string f   = saveFileDialog(false);

        if (path.extension(f) != "gto") f += ".gto";

        if (all) writeAllNodeDefinitions(f);
        else     writeNodeDefinition(nodeType(viewNode()), f);

        displayFeedback("Export complete");
    }
    catch (exception exc)
    {
        print ("ERROR: %s\n" % string(exc));
    }
    catch (...)
    {
        displayFeedback("Cancelled");
    }
    redraw();
}

\: setToRenderer ((void;Event); string type)
{
    \: (void; Event ev)
    {
        setRendererType(type);
        redraw();
    };
}

\: setStereo ((void;Event); string type)
{
    \: (void; Event ev)
    {
        let mode = cacheMode(),
            oldType = getStringProperty ("@RVDisplayStereo.stereo.type").front(),
            kickCache = ((oldType == "off" && type != "off") || (oldType != "off" && type == "off"));
        
        if (kickCache) setCacheMode(CacheOff);

        setStringProperty("@RVDisplayStereo.stereo.type", string[]{type});
        setHardwareStereoMode(type == "hardware" && stereoSupported());

        if (kickCache) setCacheMode(mode);

        redraw();
    };
}

\: setCompositeMode ((void;Event); string type)
{
    \: (void; Event ev)
    {
        setStringProperty("#RVStack.composite.type", string[]{type});
        redraw();
    };
}

\: setSession ((void;Event); int type)
{
    \: (void; Event ev)
    {
        setSessionType(type);
        redraw();
    };
}

\: cycleInputs (void; bool fwd, string node=nil)
{
    if (node eq nil) node = viewNode();
    if (node eq nil) return;

    let inputs = nodeConnections(node, false)._0,
        sz = inputs.size();

    if (sz < 2) return;

    string[] newInputs;
    newInputs.resize(sz);

    if (fwd)
    {
        for (int i = 0; i < sz-1; ++i)
            newInputs[i] = inputs[i+1];
        newInputs[sz-1] = inputs[0];
    }
    else
    {
        for (int i = 0; i < sz-1; ++i)
            newInputs[i+1] = inputs[i];
        newInputs[0] = inputs[sz-1];
    }

    setNodeInputs (node, newInputs);
}

\: smartCycleInputs (void; bool fwd)
{
    let n = viewNode();

    if (n eq nil) return;

    let t = nodeType(n);
    //
    //  If view node is a Stack or Folder then cycle view node inputs,
    //  otherwise check to see if current input is a Stack or Folder, in which case 
    //  "drill down" and cycle that.  
    //
    if (t != "RVStackGroup" && t != "RVFolderGroup")
    {
        updatePixelInfo(nil);
        State state = data();
        if (state.currentInput neq nil)
        {
            t = nodeType(state.currentInput);

            if (t == "RVStackGroup" || t == "RVFolderGroup") 
            {
                cycleInputs(fwd, state.currentInput);
                return;
            }
        }
    }
    cycleInputs(fwd, n);
}

\: cycleStackForward (void; Event ev)
{
    smartCycleInputs (true);
}

\: cycleStackBackward (void; Event ev)
{
    smartCycleInputs (false);
}

\: isSessionMode ((int;); int type)
{
    \: (int;)
    {
        if getSessionType() == type then CheckedMenuState else UncheckedMenuState;
    };
}

\: isStackMode (int;)
{
    let typeName = nodeType(viewNode());
    if typeName == "RVStackGroup" || typeName == "RVLayoutGroup" 
               then UncheckedMenuState 
               else DisabledMenuState;
}

\: togglePresentationMode (void;)
{
    try
    {
        setPresentationMode(!presentationMode());
    }
    catch (exception exc)
    {
        alertPanel(true,
                   ErrorAlert,
                   "ERROR: while trying to enter presentation mode",
                   string(exc),
                   "Ok", nil, nil);

        setPresentationMode(false);
    }
    catch (...)
    {
        alertPanel(true,
                   ErrorAlert,
                   "ERROR: while trying to enter presentation mode",
                   "Uncaught exception",
                   "Ok", nil, nil);

        setPresentationMode(false);
    }

    redraw();
}

\: newSessionState (int;)
{
    if presentationMode() then DisabledMenuState else UncheckedMenuState;
}

\: presentationModeState (int;)
{
    if presentationMode() then CheckedMenuState 
            else if sessionNames().size() > 1 then DisabledMenuState 
                                              else UncheckedMenuState;
}

\: isSetToRenderer ((int;); string type)
{
    \: (int;)
    {
        if getRendererType() == type then CheckedMenuState else UncheckedMenuState;
    };
}

\: isStereo ((int;); string type)
{
    \: (int;)
    {
        let t = getStringProperty("@RVDisplayStereo.stereo.type").front();

        if (type == "any")
        {
            return if presentationMode() || t != "off" then UncheckedMenuState else DisabledMenuState;
        }
        else if (type == "hardware" && !stereoSupported())
        {
            return DisabledMenuState;
        }

        if t == type then CheckedMenuState else UncheckedMenuState;
    };
}

\: propToggledAndStereoOnState ((int;); string prop)
{
    \: (int; )
    {
        bool toggled = false;
        try 
        {
            toggled = (getIntProperty(prop).front() != 0);
        } 
        catch(...) { ; }

        bool stereoOn = false;
        try 
        {
            stereoOn = (presentationMode() || getStringProperty("@RVDisplayStereo.stereo.type").front() != "off");
        } 
        catch(...) { ; }

        int result;

        if (stereoOn) result = if (toggled) then CheckedMenuState else UncheckedMenuState;
        else          result = DisabledMenuState;

        result;
    };
}

\: isCompositeMode ((int;); string type)
{
    \: (int;)
    {
        let t = getStringProperty("#RVStack.composite.type").front();
        return if t == type then CheckedMenuState else UncheckedMenuState;
    };
}

\: lockWarning (void; Event ev, string msg)
{
    displayFeedback("WARNING: %s" % msg,
                    2.0,
                    coloredGlyph(pauseGlyph, Color(1, .5, 0, 1.0)));
    redraw();
}

\: nudge (EventFunc; float x, float y, float z)
{
    \: (void; Event ev)
    {
        let trans = "#RVDispTransform2D.transform.translate",
            scale = "#RVDispTransform2D.transform.scale",
            t     = getFloatProperty(trans),
            s     = getFloatProperty(scale);

        setFloatProperty(trans, float[] {t[0] + x, t[1] + y});
        setFloatProperty(scale, float[] {s[0] * z, s[1] * z});
        redraw();
    };
}


//----------------------------------------------------------------------
//
//  View Space Projection
//

\: setupProjection (void; int w, int h, bool vflip = false)
{
    glyph.setupProjection(w, h, vflip);
}

\: setupProjection (void; Event event)
{
    let domain = event.domain();
    glyph.setupProjection(domain.x, domain.y);
}


//----------------------------------------------------------------------


\: drawFeedback (void; Event event)
{
    State state = data();

    if (state.feedbackText eq nil) return;

    let devicePixelRatio = devicePixelRatio(),
        textsize = 20 * devicePixelRatio;
    gltext.size(textsize);

    let d  = event.domain(),
        w  = d.x,
        h  = d.y,
        bg = state.config.bgFeedback,
        fg = state.config.fgFeedback,
        gc = bg * .7,
        ct = state.feedback - elapsedTime(),
        p  = if ct > 1.0 then 1.0 else (if ct < 0.0 then 0.0 else ct),
        sb = gltext.boundsNL(state.feedbackText);    // size of frame string

    bg.w = p;   // animate opacity
    fg.w = p;
    gc.w = p;

    setupProjection(w, h, event.domainVerticalFlip());
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    let m = margins();
    drawTextWithCartouche(m[0]*devicePixelRatio + textsize,
                          h-textsize-sb[3] - m[2]*devicePixelRatio,
                          state.feedbackText,
                          textsize, fg, bg,
                          state.feedbackGlyph, gc);
    glDisable(GL_BLEND);

    if (ct <= 0 || !isTimerRunning())
    {
        state.feedback = 0.0;
        stopTimer();
    }
    //
    //  Redraw no matter what, because even if we're done rendering
    //  feedback, we want another render to "erase" any remaining
    //  feedback graphics.
    //
    redraw();
}

\: drawTextEntry (void; Event event)
{
    State state = data();
    let d  = event.domain(),
        s  = state.prompt;

    gltext.size(state.config.textEntryTextSize);

    let sb  = gltext.bounds(s),            // size of frame string
        mxb = gltext.bounds("||||"),
        tb  = gltext.bounds(state.text),
        w   = d.x,                         // window width
        h   = d.y,                         // window height
        hm0 = 10,                          // horizontal margin
        vm0 = 10,                          // vertical margin
        hm1 = 10,                          // horizontal margin
        vm1 = 5,                           // vertical margin
        tlw = w - hm1 - hm0;

    setupProjection(w,h);

    glPushAttrib(GL_POLYGON_BIT | GL_LINE_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    //
    //  Draw the background grey area for the time line
    //

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    let t = vm0 * 2.0 + mxb[3];
    glColor(state.config.bg);
    drawRect(GL_QUADS,
             Vec2(0, 0),
             Vec2(w, 0),
             Vec2(w, t),
             Vec2(0, t));

    glDisable(GL_BLEND);

    //
    //  Draw the prompt
    //

    glColor(state.config.fg);
    gltext.color(state.config.fg);
    gltext.writeAtNL(hm0, vm0, s);

    //
    //  Draw the text
    //

    gltext.writeAtNL(hm0 + sb[2] + 10, vm0, state.text);

    //
    //  Draw the cursor
    //

    let xc = hm0 + sb[2] + 10 + tb[2] + 2,
        yv = vm0 / 2.0;

    drawRect(GL_QUADS,
             Vec2(xc, yv),
             Vec2(xc + 2, yv),
             Vec2(xc + 2, yv + mxb[3] + vm0),
             Vec2(xc, yv + mxb[3] + vm0));

    //
    //  Clean up
    //

    glPopAttrib();
}

\: drawIncomplete (bool; Event event)
{
    runtime.gc.push_api(3);
    State state = data();

    let domain   = event.domain(),
        w        = domain.x,
        h        = domain.y,
        attrs    = getCurrentAttributes(),
        srcs     = sources(),
        noAttrs  = attrs == nil || attrs.empty(),
        textsize = 20 * devicePixelRatio(),
        colorscl = 0.75,
        fstatus  = currentFrameStatus();

    setupProjection(w, h, event.domainVerticalFlip());

    bool   partial       = false;
    bool   loading       = false;
    bool   audio         = false;
    bool   video         = false;
    string statusType    = "";
    string statusMessage = "";
    string statusNode    = "";
    string t             = "";

    if (!noAttrs)
    {
        for_each (a; attrs)
        {
            case (a._0)
            {
                "PartialImage"          -> { partial       = true; }
                "RequestedFrameLoading" -> { loading       = true; }
                "Type"                  -> { statusType    = a._1; }
                "Message"               -> { statusMessage = a._1; }
                "Node"                  -> { statusNode    = a._1; }
            }
        }
    }
    for_each (src; srcs)
    {
        if (src eq nil) continue;
        let (name, start, end, inc, fps, a, v) = src;
        if (a) audio = true;
        if (v) video = true;
        if (audio && !video) t = io.path.basename(name); 
    }

    if (partial && !loading)
    {
        displayFeedback("Incomplete Image", 1, drawXGlyph);
        runtime.gc.pop_api();
        return true;
    }

    gltext.size(textsize);


    if (fstatus == NoImageStatus)
    {
        t =  if t != ""
                    then t + " -- No Image"
                else if statusMessage != ""
                    then "%s -- %s" % (statusNode, statusMessage)
                else if statusNode != ""
                    then statusNode + " -- No Image"
                else "No Image";
    }
    else if (noAttrs)
    {
        if (!audio && !video)
        {
            t = state.emptySessionStr;
        }
        else
        {
            let ins = nodeConnections(viewNode(), false)._0;

            if (ins.empty())
            {
                t = "%s is Empty" % uiName(viewNode());
            }
            else
            {
                t = "%s -- No Image Attributes Found" % uiName(viewNode());
            }
        }
    }
    else if (loading)
    {
        t = "   Loading %d" % frame();
    }
    else
    {
        t = "Missing Source";
    }

    let b  = gltext.bounds(t),
        tw = b[2],
        th = b[3],
        x  = w / 2 - tw / 2,
        y  = h / 2 - th / 2,
        cy = y + th/2,
        ch = th * 0.7,
        r  = 1.0 - abs(sin((state.loadingCount % 200) / 200.0 * math.pi * 2.0));

    glLineWidth(1.0);
    glColor(state.config.bgFeedback * colorscl * colorscl);
    glBegin(GL_LINES);
    glVertex(w/8, h/2);
    glVertex(w - w/8, h/2);
    glVertex(w/2, h/8);
    glVertex(w/2, h - h/8);
    glEnd();

    drawTextWithCartouche(x, y, t, textsize,
                          state.config.fgFeedback,
                          state.config.bgFeedback * colorscl);

    glPushAttrib(GL_ENABLE_BIT);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(1.5);

    if (!noAttrs && t != "" && loading)
    {
        for (float q = ch; q >= 0.5; q *= 0.9)
        {
            let a = math.cbrt(1.0 - q/ch);
            glColor(lerp(Color(.2, 1, .2, 1), Color(.2, .2, .2, 1), r) * a);
            drawCircleFan(x, cy, q, 0.0, 1.0, .1, true);
        }
    }

    glPopAttrib();
    state.loadingCount++;
    runtime.gc.pop_api();
    return audio || video;
}

\: addToClosestSource (void; string file, string tag)
{
    let ss = sourcesAtFrame(frame());

    if (ss.size() > 0) addToSource (ss[0], file, tag);
}

\: dropNodeURL (void; int a, string s)
{
    State state = data();

    let thing    = state.ddContent,
        rvnodeRE = regex("rvnode://([^/]*)/([^/]*)/([^/]*)(.*)"),
        parts    = rvnodeRE.smatch(thing);

    setViewNode(parts[3]);
}

\: modeURLDropFunc (bool; string url, int w, int h, int x, int y)
{
    State state = data();
    bool foundOne = false;

    //
    //  Check to see if some mode want's to handle this URL.
    //
    for_each (m; state.minorModes) 
    {
        if (m.isActive())
        {
            let (dropFunc, dropMessage) = m.urlDropFunc (state.ddContent);

            if (dropFunc neq nil)
            {
                state.ddDropFunc = dropFunc;
                state.ddRegion = drawDropRegions(w, h, x, y, 20,
                        string[] {dropMessage});
                foundOne = true;
                break;
            }
        }
    }
    return foundOne;
}

\: drawDropSites (void; Event event)
{
    State state = data();

    let domain   = event.domain(),
        w        = domain.x,
        h        = domain.y,
        x        = state.pointerPosition.x,
        y        = state.pointerPosition.y,
        thing    = state.ddContent,
        rvnodeRE = regex("rvnode://([^/]*)/([^/]*)/([^/]*)(.*)");

    setupProjection(w, h);
    bool sourceExists = false;

    if (state.pixelInfo neq nil && !state.pixelInfo.empty())
    {
        let mediaName = state.pixelInfo.front().name;

        if (mediaName != "")
        {
            try
            {
                let sname = string.split(mediaName, ".").front(),
                    layers = sourceMedia(sname)._1;
                sourceExists = true;
            }
            catch (...)
            {
                ;
            }
        }
    }
    
    glPushAttrib(GL_ENABLE_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(1.5);

    state.ddProgressiveDrop = false;

    if (sourceExists)
    {
        if (state.ddType == Event.FileObject)
        {
            if (state.ddFileKind == ImageFileKind || 
                state.ddFileKind == MovieFileKind ||
                state.ddFileKind == DirectoryFileKind)
            {
                state.ddRegion = drawDropRegions(w, h, x, y, 20,
                                                 string[] {"Add as Layer", 
                                                         "Add Source to Session"});

                (void;string) F = if state.ddRegion == 0 
                                       then addToClosestSource(,"drop")
                                       else if state.ddRegion == 1
                                             then (\: (void; string s) { addSources(string[] {s} ,"drop"); })
                                             else ddCancelled;

                if (state.ddRegion == 1)
                {
                    state.ddProgressiveDrop = true;
                    state.ddDropFunc = \: (void; int a, string s) 
                    { 
                        let f     = frameEnd(),
                        empty = sources().size() == 0;

                        F(s);
                        
                        if (empty)
                        {
                            setFrame(frameStart());
                        }
                        else
                        {
                            setFrame(f + 1);
                            //markFrame(f + 1, true);
                            //markFrame(frameStart(), true);
                        }
                        redraw();
                    };
                }
                else
                {
                    state.ddDropFunc = \: (void; int a, string s) { F(s); };
                }

            }
            else if (state.ddFileKind == CDLFileKind)
            {
                state.ddRegion = drawDropRegions(w, h, x, y, 20,
                                                 string[] {"Set File CDL",
                                                           "Set Look CDL"});

                let N = if state.ddRegion == 0
                             then "#RVLinearize"
                             else if state.ddRegion == 1
                                   then "#RVColor"
                                   else "";

                state.ddDropFunc = \: (void; int a, string s) 
                { 
                    if (N == "")
                    {
                        ddCancelled(s);
                    }
                    else
                    {
                        try
                        {
                            readCDL(s, N, true);
                            displayFeedback("%s CDL" % path.basename(s));
                        }
                        catch (exception exc)
                        {
                            let sexc = string(exc);
                            displayFeedback("Unable to read CDL %s: %s" % (path.basename(s), sexc));
                        }
                    }
                };
            }
            else if (state.ddFileKind == LUTFileKind)
            {
                state.ddRegion = drawDropRegions(w, h, x, y, 20,
                                                 string[] {"Set Display LUT", 
                                                           "Set File LUT",
                                                           "Set Look LUT"});

                let N = if state.ddRegion == 0 
                           then "@RVDisplayColor"
                           else if state.ddRegion == 1
                                 then "#RVLinearize"
                                 else if state.ddRegion == 2
                                       then "#RVLookLUT"
                                       else "";

                state.ddDropFunc = \: (void; int a, string s) 
                { 
                    if (N == "")
                    {
                        ddCancelled(s);
                    }
                    else
                    {
                        try
                        {
                            readLUT(s, N, true);
                            displayFeedback("%s LUT" % path.basename(s));
                        }
                        catch (exception exc)
                        {
                            let sexc = string(exc);
                            displayFeedback("Unable to read LUT %s: %s" % (path.basename(s), sexc));
                        }
                    }
                };
            }
            else if (state.ddFileKind == RVFileKind)
            {
                state.ddProgressiveDrop = true;
                state.ddRegion = drawDropRegions(w, h, x, y, 20,
                                                 string[] {"Replace Session"});

                state.ddDropFunc = \: (void; int a, string s) { addSources(string[] {s}, "drop"); };
            }
            else
            {
                displayFeedback("Unknown File Type", 1, drawXGlyph);
            }
        }
        else
        if (state.ddType == Event.URLObject ||
            state.ddType == Event.TextObject) //  Chrome URL drops come across as "Text"
        {
            let parts = rvnodeRE.smatch(thing);

            if (parts eq nil || parts.size() < 4)
            {
                if (! modeURLDropFunc (state.ddContent, w, h, x, y)) displayFeedback("Unknown Object", 1, drawXGlyph);
            }
            else
            {
                state.ddDropFunc = dropNodeURL;
                state.ddRegion = drawDropRegions(w, h, x, y, 20,
                                                 string[] {"Drop to change view to %s" % uiName(parts[3])});
            }
        }
    }
    else
    {
        if (state.ddType == Event.FileObject)
        {
            if (state.ddFileKind == ImageFileKind ||
                state.ddFileKind == MovieFileKind ||
                state.ddFileKind == DirectoryFileKind)
            {
                state.ddProgressiveDrop = true;
                state.ddRegion = drawDropRegions(w, h, x, y, 20,
                                                 string[] {"Add Source to Session"});
                state.ddDropFunc = \: (void; int a, string s) 
                { 
                    let f     = frameEnd(),
                    empty = sources().size() == 0;

                    addSources(string[] {s}, "drop"); 

                    if (empty)
                    {
                        setFrame(frameStart());
                    }
                    else
                    {
                        setFrame(f + 1);
                        //markFrame(f + 1, true);
                        //markFrame(frameStart(), true);
                    }
                    redraw();
                };
            }
            else if (state.ddFileKind == RVFileKind)
            {
                state.ddProgressiveDrop = true;
                state.ddRegion = drawDropRegions(w, h, x, y, 20,
                                                 string[] {"Load Session"});

                state.ddDropFunc = \: (void; int a, string s) { addSources(string[] {s}, "drop"); };
            }
            else if (state.ddFileKind == LUTFileKind)
            {
                state.ddRegion = drawDropRegions(w, h, x, y, 20,
                                                 string[] {"Set Display LUT"});

                state.ddDropFunc = \: (void; int a, string s) 
                { 
                    readLUT(s, "@RVDisplayColor", true); 
                };
            }
            else
            {
                displayFeedback("Unknown File Type", 1, drawXGlyph);
            }
        }
        else
        if (state.ddType == Event.URLObject ||
            state.ddType == Event.TextObject)  //  Chrome URL drops come across as "Text"
        {
            if (! modeURLDropFunc (state.ddContent, w, h, x, y)) displayFeedback("Unknown Object", 1, drawXGlyph);
        }
        else
        {
            displayFeedback("Unknown Object", 1, drawXGlyph);
        }
    }

    glPopAttrib();
}

\: drawError (void; Event event)
{
    State state = data();

    let domain = event.domain(),
        w      = domain.x,
        h      = domain.y,
        attrs  = getCurrentAttributes(),
        noAttrs = attrs == nil,
        throb   = //1.0 - abs(sin((state.loadingCount % 200) / 200.0 * math.pi * 2.0));
        0;

    if (noAttrs)
    {
        displayFeedback("Error Frame %d" % frame(), 1, drawXGlyph);
    }
    else
    {
        for_each (a; attrs)
        {
            let (name, val) = a;

            if (name == "Message") 
            {
                displayFeedback(val, 1, drawXGlyph);
                break;
            }
        }
    }

    state.loadingCount++;
    redraw();
}

\: cacheProgressGlyph (void; bool outline)
{
    let (capacity, _, lusage, seconds, secondsNeeded, _, _) = cacheInfo(),
        oldU                              = float(lusage) / float(capacity),
        u                                 = seconds/secondsNeeded;
    
    if (u > .98 || !isBuffering())
    {
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glRotate(if inc() > 0 then 180.0 else 0.0, 0, 0, 1);
        triangleGlyph(outline);
        glPopMatrix();
    }
    else
    {
        glColor(Color(.25, .25, .25, 1));
        drawCircleFan(0, 0, 0.5, u, 1, .3, outline);
        glColor(Color(.6, .6, .6, 1));
        drawCircleFan(0, 0, 0.5, 0.0, u, .3, outline);
    }
}

\: audioCacheProgressGlyph (void; bool outline)
{
    let (_, u, _) = cacheUsage(),
        total     = (frameEnd() - frameStart()) / fps(),
        pcent     = u / total;
    
    glColor(Color(.25, .25, .25, 1));
    drawCircleFan(0, 0, 0.5, pcent, 1, .3, outline);
    glColor(Color(.6, .6, .6, 1));
    drawCircleFan(0, 0, 0.5, 0.0, pcent, .3, outline);
}

\: loadingProgressGlyph (void; bool outline)
{
    let pcent = float(loadCount()) / float(loadTotal());
    
    glColor(Color(.25, .25, .25, 1));
    drawCircleFan(0, 0, 0.5, pcent, 1, .3, outline);
    glColor(Color(.6, .6, .6, 1));
    drawCircleFan(0, 0, 0.5, 0.0, pcent, .3, outline);
}

\: missingImage (void; Event event)
{
    //
    //  NOTE: you can set up projection here and render 
    //

    let contents = event.contents(),
        parts = contents.split(";"),
        media = io.path.basename(sourceMedia(parts[1])._0);

    displayFeedback("MISSING: frame %s of %s" % (parts[0], media), 1, drawXGlyph);
}

\: layout (void; Event event)
{
    State state = data();
    if (state.timeline neq nil) state.timeline.layout(event);
    if (state.motionScope neq nil) state.motionScope.layout(event);
    if (state.imageInfo neq nil) state.imageInfo.layout(event);
    if (state.infoStrip neq nil) state.infoStrip.layout(event);
    if (state.processInfo neq nil) state.processInfo.layout(event);
    if (state.sourceDetails neq nil) state.sourceDetails.layout(event);
    if (state.inspector neq nil) state.inspector.layout(event);
    if (state.wipe neq nil) state.wipe.layout(event);
    if (state.sync neq nil) state.sync.layout(event);
    for_each (m; state.minorModes) if (m.isActive()) m.layout(event);
    for_each (w; state.widgets) if (w.isActive()) w.layout(event);
}

global bool firstRender = true;
global int totalBytes = 0;
global int totalRenderCount = 0;
global bool debugGC = false;

\: render (void; Event event)
{
    event.reject();

    let fstatus    = currentFrameStatus(),
        incomplete = isCurrentFrameIncomplete(),
        caching    = isCaching(),
        buffering  = isBuffering(),
        err        = isCurrentFrameError();

    State state = data();

    if (firstRender)
    {
        if (firstRender && state.pointerPosition == Point(0,0)) 
        //
        //  If the mouse has never entered the RV window, set pointerPosition
        //  to something that might be useful.
        //
        {
            state.pointerPosition = Point (event.domain().x/2, event.domain().y/2);
        }
        if (system.getenv("RV_DEBUG_GC", nil) neq nil) debugGC = true;

            firstRender = false;
        }

    if (debugGC) let a0 = runtime.gc.get_free_bytes();

    if (state.perPixelInfoValid)
    {
        if (state.perPixelFrame != frame() ||
            state.perPixelPosition != state.pointerPosition)
        {
            state.perPixelInfoValid = false;
        }
        else
        {
            runtime.gc.push_api(3);
            int viewHash = string.hash(viewNode());
            runtime.gc.pop_api();

            if (state.perPixelViewHash != viewHash) 
            {
                state.perPixelInfoValid = false;
            }
        } 
    }

    if (loadTotal() != 0)
    {
        displayFeedback("Loading %d of %d" % (loadCount()+1, loadTotal()), 
                        1.0,
                        loadingProgressGlyph);
        redraw();
    }

    if (state.dragDropOccuring) 
    {
        drawDropSites(event);
    }
    else if ((fstatus == NoImageStatus || incomplete) && !drawIncomplete(event)) 
    {
        if (state.feedback > 0) drawFeedback(event);
        if (state.textEntry && state.textOkWhenEmpty) drawTextEntry(event);
        for_each (m; state.minorModes) if (m.isActive() && m._drawOnEmpty) m.render(event);

        return; // empty session
    }
    else if (err) 
    {
        drawError(event);
    }

    if (preSnapRenderCount > 0 && !presentationMode())
    {
        if (preSnapRenderCount == 1) snapToPixelCenter();
        --preSnapRenderCount;
    }

    for_each (F; state.config.renderImageSpace) F(event);
    for_each (m; state.minorModes) if (m.isActive()) 
    { 
        if (debugGC) let b0 = runtime.gc.get_free_bytes();
        m.render(event);
        if (debugGC)
        {
            let b1 = runtime.gc.get_free_bytes(),
                d = b0 - b1;
            if (d > 0) print ("    %s: allocated %s bytes\n" % (m.name(), d));
        }
    }
    for_each (w; state.widgets) if (w.isActive()) w.render(event);
    for_each (F; state.config.renderViewSpace) F(event);

    //
    //  Draw standard widgets on top of everything
    //

    runtime.gc.disable();
    if (debugGC) let c0 = runtime.gc.get_free_bytes();
    runtime.gc.push_api(3);

    if (state.motionScope neq nil && state.motionScope.isActive()) state.motionScope.render(event);
    if (state.timeline neq nil && state.timeline.isActive()) state.timeline.render(event);
    if (state.imageInfo neq nil && state.imageInfo.isActive()) state.imageInfo.render(event);
    if (state.sourceDetails neq nil && state.sourceDetails.isActive()) state.sourceDetails.render(event);
    if (state.infoStrip     neq nil && state.infoStrip.isActive())     state.infoStrip.render(event);
    if (state.feedback > 0) drawFeedback(event);

    if (state.externalProcess neq nil && !state.externalProcess.isFinished())
    {
        if (state.processInfo neq nil && state.processInfo.isActive()) state.processInfo.render(event);
        redraw();
    }

    runtime.gc.pop_api();
    if (debugGC)
    {
        let c1 = runtime.gc.get_free_bytes(),
            dc = c0 - c1;
        if (dc > 0) print("    core modes allocated %d bytes\n" % dc);
    }
    runtime.gc.enable();

    if (state.inspector neq nil && state.inspector.isActive()) state.inspector.render(event);
    if (state.textEntry) drawTextEntry(event);
    if (state.wipe neq nil && state.wipe.isActive()) state.wipe.render(event);
    if (state.sync neq nil && state.sync.isActive()) state.sync.render(event);

    if (state.externalProcess neq nil && state.externalProcess.isFinished())
        {
            state.externalProcess = nil;
            if (state.processInfo neq nil && state.processInfo.isActive()) toggleProcessInfo();
        }

    if (audioCacheMode() == CacheGreedy)
    {
        let total         = (frameEnd() - frameStart()) / fps();
        let (_, asecs, _) = cacheUsage();
        let pcent         = asecs / total;
        let ct            = state.feedback - elapsedTime();

        if (pcent < 1.0 && ct <= 1.0)
        {
            displayFeedback("Audio BUFFERING", 1.0, audioCacheProgressGlyph);
        }
    }

    if (incomplete || caching) 
    {
        if (buffering && !isPlaying())
        {
            displayFeedback("BUFFERING", 1.0, cacheProgressGlyph);
        }

        redraw();
    }

    event.reject();

    if (debugGC)
    {
        let a1 = runtime.gc.get_free_bytes(),
            d = a0 - a1;
        if (d > 0) totalBytes += d;
        ++totalRenderCount;
        if (d > 0) print("frame %s: allocated %d bytes, ave %s\n" % (frame(), d, float(totalBytes)/float(totalRenderCount)));
    }
}

\: renderOutputDevice (void; Event event)
{
    event.reject();

    if (preSnapRenderCount > 0 && presentationMode())
    {
        if (preSnapRenderCount == 1) snapToPixelCenter();
        --preSnapRenderCount;
    }
}

\: preRender (void; Event event)
{
    event.reject();

    if (snapDoit)
    {
        setTranslation(snapTranslation);
        snapDoit = false;
    }
}

//
//  Add render hook functions
//

\: addRenderFunction (void; EventFunc F, RenderFunctionType rtype)
{
    State state = data();
    Configuration c = if state eq nil then globalConfig else state.config;

    case (rtype)
    {
        ImageSpaceRender -> { c.renderImageSpace = F : c.renderImageSpace; }
        ViewSpaceRender  -> { c.renderViewSpace = F : c.renderViewSpace; }
    }
}


//----------------------------------------------------------------------
//
//  Text entry handlers
//

\: setPrompt (void;)
{
    State state = data();
    state.prompt = "%s: " % state.prompt;
    displayFeedback("",0);
    redraw();
}

\: killAllText (void; Event event)
{
    State state = data();
    state.text = "";
    redraw();
}

\: backwardDeleteChar (void; Event event)
{
    State state = data();
    if (state.text.size() == 1) state.text = "";
    else state.text = state.text.substr(0, state.text.size() - 1);
    redraw();
}

\: selfInsert (void; Event event)
{
    State state = data();
    state.text += string(char(event.key()));
    redraw();
}

\: commitEntry (void; Event event)
{
    State state = data();
    popEventTable("textentry");
    try { if (state.text != "") state.textFunc(state.text); } catch (...) { ; }
    state.textEntry = false;
    redraw();
}

\: cancelEntry (void; Event event)
{
    State state = data();
    displayFeedback("Cancelled");
    state.textEntry = false;
    popEventTable("textentry");
    redraw();
}

\: ignoreEntry (void; Event event) {;}


//----------------------------------------------------------------------
//
//  Default pixel-block handling
//

\: defaultPixelBlockHandler (void; Event event)
{
    State state = data();

    insertCreatePixelBlock(event);

    let t = theTime();

    if (t - state.lastPixelBlockTime > 1.0) 
    {
        redraw();
        state.lastPixelBlockTime = t;
    }
} 

//----------------------------------------------------------------------

\: updateStateMatte (void; Event event)
{
    event.reject();
    
    let active  = getIntProperty("#Session.matte.show").front(),
        opacity = getFloatProperty("#Session.matte.opacity").front(),
        aspect  = getFloatProperty("#Session.matte.aspect").front();

    if (active == 1)
    {
        setMatte(aspect, false)();
        setMatteOpacity(opacity)();
    }
}

//----------------------------------------------------------------------

\: sessionDeletionHandler(void; Event event)
{
    event.reject();
    if (! checkIfCloseOK()) event.setReturnContent ("not OK");
}

\: checkIfCloseOK (bool; )
{
    //
    //  We should have something more sophisticated here that
    //  actually tests if there's something meaningful to save, and
    //  then provides the option to 'not ask again', but for now,
    //  we just close, so that people don't have to click 'OK' all
    //  the time if they don't know the magic hotkey.
    //  
    //  Update: state.unsavedChanges added, but nothing sets it in
    //  the default GUI.  Others can set it.
    //  
    //  Also now any mode can add a custom message via State.registerQuitMessage().
    //  

    State s = data();
    let message = s.getQuitMessage(),
        ret = true;

    if (s.unsavedChanges) message = "You have unsaved changes";

    if (message neq nil)
    {
        int choice = alertPanel(true, // associated panel (sheet on OSX)
                                WarningAlert,
                                message, nil,
                                "Don't Quit", "Quit", nil);
    
        if (choice == 1)
        {
            //
            //  Must clear state since we'll possibly end up back in
            //  this code during the quit process, and we don't want
            //  to query the user again.
            //
            s.clearQuitMessages();
            s.unsavedChanges = false;
        }
        else
        {
            ret = false;
            displayFeedback("Quit Cancelled");
            redraw();
        }
    }

    if (ret && presentationMode()) setPresentationMode(false);

    return ret;
}

\: showDiagnosticsWindow (void; Event ev)
{
    commands.showDiagnostics();
}

\: queryClose (void; Event ev)
{
    if (checkIfCloseOK()) close();
}

\: setCompModeAndView ((void;Event); string ctype, string view)
{
    \: (void; Event ev)
    {
        if (ctype neq nil) setStringProperty("defaultStack_stack.composite.type", string[]{ctype});

        bool playing = isPlaying();
        if (playing) stop();
        setViewNode(view);
        if (playing) play();

        //displayFeedback("Composite Mode: %s" % type);
        redraw();
    };
}

\: isStackAndCompMode ((int;); string type)
{
    \: (int;)
    {
        let t = getStringProperty("defaultStack_stack.composite.type").front(),
            isStack = (viewNode() == "defaultStack");

        return if (isStack && t == type) then CheckedMenuState else UncheckedMenuState;
    };
}

\: viewNodeState ((int;); string name)
{
    \: (int;)
    {
        let itIs = (viewNode() == name);

        return if (itIs) then CheckedMenuState else UncheckedMenuState;
    };
}

//----------------------------------------------------------------------
//
//  The menu layout
//

\: combine (Menu; Menu a, Menu b)
{
    if (a eq nil) return b;
    if (b eq nil) return a;

    Menu n;
    for_each (i; a) n.push_back(i);
    for_each (i; b) n.push_back(i);
    n;
}

\: combine (Menu; Menu a, Menu[] b)
{
    if (b eq nil) return a;
    Menu accum = a;
    for_each (m; b) accum = combine(accum, m);
    accum;
}

\: combine (Menu; Menu a, [MenuItem] b)
{
    if (b eq nil) return a;
    Menu accum = a;
    for_each (m; b) accum.push_back(m);
    accum;
}

\: toggleLockResizeScale (void; Event event)
{
    State state = data();
    state.lockResizeScale = !state.lockResizeScale;
    writeSetting ("View", "lockResizeScale", SettingsValue.Bool(state.lockResizeScale));
}

\: lockResizeScaleState (int; )
{
    State state = data();
    return if (state.lockResizeScale) then CheckedMenuState else UncheckedMenuState;
}

\: toggleExpandWidth (void; Event event)
{
    expandWidth = !expandWidth;
    writeSetting ("View", "expandWidth", SettingsValue.Bool(expandWidth));
}

\: isExpandedWidth (int; )
{
    return if (expandWidth) then CheckedMenuState else UncheckedMenuState;
}

\: buildViewMenu (Menu;)
{
    Menu a = {
        {"Stereo", Menu {
            {"Stereo Viewing Mode", nil, nil, inactiveState},
            {"   Off", setStereo("off"), nil, isStereo("off")},
            {"   Anaglyph", setStereo("anaglyph"), nil, isStereo("anaglyph")},
            {"   Luminance Anaglyph", setStereo("lumanaglyph"), nil, isStereo("lumanaglyph")},
            {"   Side-by-Side", setStereo("pair"), nil, isStereo("pair")},
            {"   Mirror Side-by-Side", setStereo("mirror"), nil, isStereo("mirror")},
            //{"   Horizontal Squeezed", setStereo("hsqueezed"), nil, isStereo("hsqueezed")},
            //{"   Vertical Squeezed", setStereo("vsqueezed"), nil, isStereo("vsqueezed")},
            {"   DLP Checker", setStereo("checker"), nil, isStereo("checker")},
            {"   Scanline", setStereo("scanline"), nil, isStereo("scanline")},
            {"   Left Eye Only", setStereo("left"), nil, isStereo("left")},
            {"   Right Eye Only", setStereo("right"), nil, isStereo("right")},
            {"   Shutter Glasses", setStereo("hardware"), nil, isStereo("hardware")},
            {"_", nil},
            {"Global Swap Eyes", ~toggleSwapEyes, nil, propToggledAndStereoOnState("@RVDisplayStereo.stereo.swap")},
            {"Global Flip Right Eye", ~toggleRFlip, nil, propToggledAndStereoOnState("@RVDisplayStereo.rightTransform.flip")},
            {"Global Flop Right Eye", ~toggleRFlop, nil, propToggledAndStereoOnState("@RVDisplayStereo.rightTransform.flop")},
            {"_", nil},
            {"Interactive Edit",      nil, nil, inactiveState },
            {"    Global Relative Eye Offset (%)", stereoOffsetMode, nil, nil},
            {"    Global Right Eye Only Offset (%)", stereoROffsetMode, nil, nil},
            {"_", nil},
            {"Reset All Stereo Offsets", resetStereoOffsets, nil, nil},
            }},
        //{"Renderer", Menu {
            //{"Composite", setToRenderer("Composite"), nil, isSetToRenderer("Composite")}
            //{"Direct", setToRenderer("Direct"), nil, isSetToRenderer("Direct")}
        //  }},
        {"_", nil},
        {"Presentation Mode", ~togglePresentationMode, "control p", presentationModeState},
        {"Presentation Settings", Menu()},
        {"_", nil},
        {"Frame",           ~frameImage,     "f"},
        {"Frame Width",     frameWidth,     "control f"},
        {"_", nil},
        {"Linear to Display Correction", nil, nil, inactiveState},
        {"   No Correction", setDispConvert(""), nil, hasDispConversion("")},
        {"   sRGB", setDispConvert("sRGB"), nil, hasDispConversion("sRGB")},
        {"   Rec709", setDispConvert("Rec709"), nil, hasDispConversion("Rec709")},
        {"   Display Gamma 2.2", setDispConvert("Gamma 2.2"), nil, hasDispConversion("Gamma 2.2")},
        {"   Display Gamma 2.4", setDispConvert("Gamma 2.4"), nil, hasDispConversion("Gamma 2.4")},
        {"   Display Gamma...", enterDispGamma, "v", dispGammaState},
        {"_", nil},
        {"Display LUT Active", ~toggleDisplayLUT, "D", isDisplayLUTActiveState},
        {"Display ICC Active", ~toggleDisplayICC, "", isDisplayICCActiveState},
        {"_", nil},
        {"Display Brightness (Interactive)", brightnessMode, "B", checkForDisplayColor()},
        {"_", nil},
        {"Matte", Menu {
            {"No Matte", ~setMatte(0),     nil, matteAspectState(0)},
            {"1.33",     ~setMatte(1.33),  nil, matteAspectState(1.33)},
            {"1.66",     ~setMatte(1.66),  nil, matteAspectState(1.66)},
            {"1.77",     ~setMatte(1.77),  nil, matteAspectState(1.77)},
            {"1.85",     ~setMatte(1.85),  nil, matteAspectState(1.85)},
            {"2.35",     ~setMatte(2.35),  nil, matteAspectState(2.35)},
            {"2.40",     ~setMatte(2.40),  nil, matteAspectState(2.40)},
            {"_", nil},
            {"Custom...",    enterMatte,   nil, matteAspectState(-1.0)}
        }},
        {"Matte Opacity", Menu {
            {"33%",     ~setMatteOpacity(0.33),  nil, matteOpacityState(0.33)},
            {"66%",     ~setMatteOpacity(0.66),  nil, matteOpacityState(0.66)},
            {"100%",    ~setMatteOpacity(1.0),   nil, matteOpacityState(1.0)},
            {"_", nil},
            {"Custom...", enterMatteOpacity,   nil, matteOpacityState(-1.0)}
        }},
        {"_", nil},
        {"Channel Display", Menu {
            {"Color (All Channels)", ~showChannel(0), "c", channelState(0)},
            {"Red",             ~showChannel(1), "r", channelState(1)},
            {"Green",           ~showChannel(2), "g", channelState(2)},
            {"Blue",            ~showChannel(3), "b", channelState(3)},
            {"Alpha",           ~showChannel(4), "a", channelState(4)},
            {"_", nil},
            {"Luminance",       ~showChannel(5), "l", channelState(5)}}},
        {"Channel Order", Menu {
            {"RGBA", ~channelOrder("RGBA"), nil, channelOrderState("RGBA")},
            {"_", nil},
            {"Custom...", enterOrder, nil, checkForDisplayNode()},
            {"_", nil},
            {"RBGA", ~channelOrder("RBGA"), nil, channelOrderState("RBGA")},
            {"GBRA", ~channelOrder("GBRA"), nil, channelOrderState("GBRA")},
            {"GRBA", ~channelOrder("GRBA"), nil, channelOrderState("GRBA")},
            {"BRGA", ~channelOrder("BRGA"), nil, channelOrderState("BRGA")},
            {"BGRA", ~channelOrder("BGRA"), nil, channelOrderState("BGRA")},
            {"_", nil},
            {"ABGR", ~channelOrder("ABGR"), nil, channelOrderState("ABGR")},
            {"ARGB", ~channelOrder("ARGB"), nil, channelOrderState("ARGB")},
            {"_", nil},
            {"R00A", ~channelOrder("R00A"), nil, channelOrderState("R00A")},
            {"0G0A", ~channelOrder("0G0A"), nil, channelOrderState("0G0A")},
            {"00BA", ~channelOrder("00BA"), nil, channelOrderState("00BA")}
        }},
        {"Background", Menu {
            {"Black", setBGPattern(,"black"), nil,            testBGPattern("black")},
            {"18% Grey", setBGPattern(,"grey18"), nil,        testBGPattern("grey18")},
            {"50% Grey", setBGPattern(,"grey50"), nil,        testBGPattern("grey50")},
            {"White", setBGPattern(,"white"), nil,            testBGPattern("white")},
            {"Checker", setBGPattern(,"checker"), nil,        testBGPattern("checker")},
            {"Cross Hatch", setBGPattern(,"crosshatch"), nil, testBGPattern("crosshatch")}
        }},
        {"_", nil},
        {"Dither", Menu {
            {"Off", setDither(,0), nil, testDither(0)},
            {"8 Bit", setDither(,8), nil, testDither(8)},
            {"10 Bit", setDither(,10), nil, testDither(10)}
        }},
        {"_", nil},
        {"Show Out Of Range Colors", ~toggleOutOfRange, nil, isOutOfRange},
        {"_", nil},
        {"Create/Edit Display Profiles...", ~editProfiles, nil, nil},
        {"_", nil},
        {"Linear Filter",   ~toggleFilter, "n", filterState},
        {"Lock Pixel Scale During Resize",  toggleLockResizeScale, nil, lockResizeScaleState},
        {"Preserve Image Height in Pixel Aspect Scaling",  toggleExpandWidth, nil, isExpandedWidth}
    };

    return a;
}

\: buildMainMenu (Menu; [MenuItem] userMenus = nil)
{

   Menu exportMenu = Menu {
            {"Quicktime Movie...", exportAs(, "mov", "Quicktime Export"), "control e", videoSourcesExistAndExportOKState},
            {"Image Sequence...", exportAs(, "*", "Image Sequence Export"), nil, videoSourcesExistAndExportOKState},
            {"Marked Frames...", exportMarked, nil, hasMarksState},
            {"Annotated Frames...", exportAnnotatedFrames, nil, videoSourcesExistState},
            {"Audio File...", exportAs(, "*", "Audio Export"), nil, sourcesExistState},
            {"Snapshot...", exportFrame(,exportCurrentFrame), nil, videoSourcesExistState},
            {"Current Source Frame...", exportFrame(,exportCurrentSourceFrame), nil, videoSourcesExistState},
            {"Image Attributes...", exportAttrs, nil, videoSourcesExistState},
            {"RVIO Ready Session...", exportAs(, "rv", "Session Export"), nil, videoSourcesExistState},
            {"OTIO File...", exportOtio, nil, canExportOTIOState}};

    if (true || shortAppName() == "rvx")
    {
         exportMenu.push_back (MenuItem {"Definition of Current View Node ...", exportNodeDefinition(false,), nil, videoSourcesExistState});
         exportMenu.push_back (MenuItem {"All Node Definitions ...", exportNodeDefinition(true,), nil, videoSourcesExistState});
    }

    Menu mainMenu_part1 = {
        {"File", Menu {
            {"New Session", \: (void; Event ev) { newSession(nil); }, "control n", newSessionState},
            {"Open...",  addMovieOrImageSources(,true,false), "control o", nil},
            {"Merge...",  addMovieOrImageSources(,true,true), nil, nil},
            {"Open into Layer...",  addMovieOrImage(,addToClosestSource(,"explicit"),false), nil, sourcesExistState},
            {"Open in New Session...",  openMovieOrImage, "control O", newSessionState},
            {"_", nil},
            {"Clone Session", cloneSession, nil, newSessionState},
            {"Clone RV", cloneRV, nil, nil},
            {"Clone Synced RV", cloneSyncedRV, nil, nil},
            {"_", nil},
            {"Relocate Movie or Image Sequence...",  relocateMovieOrImage, nil, singleSourceState},
            {"Replace Source Media...", replaceSourceMedia, nil, singleSourceState},
            {"_", nil},
            {"Save Session", save, "control s", nil},
            {"Save Session As...", saveAs, "control S", nil},
            {"_", nil},
            {"Import", Menu {
                {"Display LUT...",  openLUTFile("@RVDisplayColor"), nil, checkForDisplayColor()},
                {"Pre-Cache LUT...",  openLUTFile("#RVCacheLUT"), nil, videoSourcesAndNodeExistState("RVCacheLUT")},
                {"File LUT...",  openLUTFile("#RVLinearize"), nil, videoSourcesAndNodeExistState("RVLinearize")},
                {"File CDL...",  openCDLFile("#RVLinearize"), nil, videoSourcesAndNodeExistState("RVLinearize")},
                {"Look LUT...",  openLUTFile("#RVLookLUT"), nil, videoSourcesAndNodeExistState("RVLookLUT")},
                {"Look CDL...",  openCDLFile("#RVColor"), nil, videoSourcesAndNodeExistState("RVColor")},
                {"File OTIO...",  openOTIOFile(), nil, checkForOTIOFile()},
                {"Simple EDL...",  ~openSimpleEDL, nil, nil}}},
            {"Export", exportMenu},
            {"_", nil},
            {"Clear",           ~clearEverything, "control N", nil},
            {"Close Session",    queryClose, "control w", nil}
            }},
        {"Edit", Menu {
            //{"Undo",              nil,     nil,   inactiveState},
            //{"Redo",              nil,     nil,   inactiveState},
            //{"_", nil},
            //{"Cut",               nil,     nil,   inactiveState},
            //{"Copy",              nil,     nil,   inactiveState},
            //{"Paste",             nil,     nil,   inactiveState},
            //{"_", nil},
            {"Mark Frame",               ~toggleMark,    "m",   markedState},
            {"Mark Sequence Boundaries", ~markSequence,  nil,   sequenceState},
            {"Mark Annotated Frames", ~markAnnotatedFrames,  nil,   rangeState},
            {"Clear All Marks",          ~clearAllMarks, nil,   hasMarksState},
            {"Mark in Range", Menu {
                {"Clear Range",          ~clearMarksInRange,  nil,     hasMarksState},
                {"_", nil},
                {"Set Range In Point",    ~frameFunc(setInPoint),   "[", rangeState},
                {"Set Range Out Point",   ~frameFunc(setOutPoint),  "]", rangeState},
                {"Reset Range",            ~resetInOutPoints,  "\\",     rangeState},
                /*
                {"_", nil},
                {"Narrow to Range",            ~narrowToInOut,  nil,     nil},
                {"Widen to Full Range",        ~resetRange,  nil,     nil},
                */
                {"_", nil},
                {"Set Range From Marks/Boundaries",  ~setInOutMarkedRange, "|",      rangeState},
                {"Next Range From Marks/Boundaries", ~nextMarkedRange, "control right", rangeState},
                {"Prev Range From Marks/Boundaries", ~previousMarkedRange, "control left", rangeState},
                {"Expand Range From Marks/Boundaries", ~expandMarkedRange, "control up", rangeState},
                {"Contract Range From Marks/Boundaries", ~contractMarkedRange, "control down", rangeState}}},
            {"Set Range Offset", setRangeOffset, nil, sourcesExistState},
            }},
        {"Control", Menu {
            {"Play",            ~togglePlayFunc,     nil, playState},
            {"Stop",            ~stopFunc,           nil, rangeState},
            {"PingPong",        ~togglePingPong, nil, pingPongState},
            {"Play Once",       ~togglePlayOnce, nil, playOnceState},
            {"Step Forward",    ~stepForward1,   nil, rangeState},
            {"Step Backward",   ~stepBackward1,  nil, rangeState},
            {"Reverse",         ~toggleForwardsBackwards, nil, rangeState},
            {"_", nil},
            {"Go To Frame...",   enterFrame, "G",  rangeState},
            {"_", nil},
            {"Play All Frames", ~toggleRealtime, "A",           realtimeState},
                {"FPS", Menu {
                    {"24",       ~setFPSFunc(24.0),  "alt 2",   rangeState},
                    {"25",       ~setFPSFunc(25.0),  "alt 5",   rangeState},
                    {"23.98",    ~setFPSFunc(23.98),  nil,          rangeState},
                    {"30",       ~setFPSFunc(30.0),  "alt 3",   rangeState},
                    {"29.97",    ~setFPSFunc(29.97),  nil,          rangeState},
                    {"_", nil},
                    {"Custom...", enterFPS,           "F",          rangeState}}},
            {"_", nil},
            {"Play Forward",    ~incN(1),       ".",            forwardState},
            {"Play Backward",   ~incN(-1),      ",",            backwardState},
            {"_", nil},
            {"Jump To Beginning", ~beginning,           "home",         rangeState},
            {"Jump To Ending",    ~ending,              "end",          rangeState},
            {"Next Marked Frame", ~nextMarkedFrame,     "alt right", rangeState},
            {"Prev Marked Frame", ~previousMarkedFrame, "alt left", rangeState},
            {"Matching Frame Of Next Source", ~nextMatchedFrame,     ">", rangeState},
            {"Matching Frame Of Previous Source", ~previousMatchedFrame, "<", rangeState},
            /*  
             * Moved to prefs GUI
             *
            {"_", nil},
            {"Disable Scrubbing in View", toggleScrubDisabled, nil, scrubDisabledState},
            {"Disable Click in View to Play", toggleClickToPlayDisabled, nil, clickToPlayDisabledState},
            */
            }},
        {"Tools", Menu {
            {"Default Views", nil, nil, inactiveState},
            {"   Sequence", setCompModeAndView(nil,"defaultSequence"), nil, viewNodeState("defaultSequence")},
            {"   Replace", setCompModeAndView("replace","defaultStack"), nil, isStackAndCompMode("replace")},
            {"   Over", setCompModeAndView("over","defaultStack"), nil, isStackAndCompMode("over")},
            {"   Add", setCompModeAndView("add","defaultStack"), nil, isStackAndCompMode("add")},
            //{"   Dissolve", setCompModeAndView("difference","defaultStack"), nil, isStackAndCompMode("dissolve")}, 
            {"   Difference", setCompModeAndView("difference","defaultStack"), nil, isStackAndCompMode("difference")}, 
            {"   Difference (Inverted)", setCompModeAndView("-difference","defaultStack"), nil, isStackAndCompMode("-difference")},
            //{"   Picture in Picture", setCompModeAndView("pip","defaultStack"), nil, isStackAndCompMode("pip")},
            {"   Tile", setCompModeAndView(nil,"defaultLayout"), nil, viewNodeState("defaultLayout")},
            {"_", nil},
            {"Timeline",        ~toggleTimeline, "F2", timelineShown},
            {"Timeline Magnifier", ~toggleMotionScope, "F3", motionScopeShown},
            {"Image Info",      ~toggleInfo,     "F4", infoShown},
            {"Color Inspector", ~toggleColorInspector, "F5", colorInspectorShown},
            {"Wipes",           ~toggleWipe, "F6", wipeShown},
            {"Info Strip",      ~toggleInfoStrip, "F7", infoStripShown},
            {"Process Info",    ~toggleProcessInfo, "F8", processInfoShown},
            {"Source Details",  ~toggleSourceDetails, "F11", sourceDetailsShown},
            {"_", nil},
            {"Menu Bar",        ~toggleMenuBar,  "F1", menuBarShown},
            {"Top View Toolbar",    toggleTopViewToolbar,  nil, topTBShown},
            {"Bottom View Toolbar", toggleBottomViewToolbar,  nil, botTBShown},
            {"_", nil},
            {"Force Reload Current Frame", ~reload, "R", sourcesExistState},
            {"Force Reload Region", ~reloadInOut, "control R", sourcesExistState},
            {"Reload Changed Frames", ~loadCurrentSourcesChangedFrames, "control C", sourcesExistState},
            {"_", nil},
            {"Cache Mode",  nil,  nil, inactiveState},
            {"   Look-Ahead Cache", cacheModeFunc(CacheBuffer), "control l", cacheStateFunc(CacheBuffer)},
            {"   Region Cache",  cacheModeFunc(CacheGreedy), "C", cacheStateFunc(CacheGreedy)},
            {"   Cache Off", cacheModeFunc(CacheOff),  nil, cacheStateFunc(CacheOff)},
            {"   Release All Cached Images", ~releaseAllCachedImages,  nil, nil},
            {"_", nil},
            {"Diagnostics",    showDiagnosticsWindow, nil, nil},
            {"_", nil},
            }},
        {"Audio", Menu {
            {"Mute", ~toggleMute, nil, isMuted},
            {"Scrubbing", toggleAudioScrub, nil, scrubAudioState},
            {"_", nil},
            {"Global",  nil,  nil, inactiveState},
            {"   Volume",  globalVolumeMode,     "control v", nil},
            {"   Offset (seconds)",  globalAudioOffsetMode, nil, nil},
            {"   Offset (frames)",  globalAudioOffsetFramesMode, nil, nil},
            {"   Balance", globalBalanceMode,   nil, nil},
            {"Source",  nil,  nil, inactiveState},
            {"   Volume", sourceVolumeMode, nil, sourcesExistState},
            {"   Offset (seconds)", sourceAudioOffsetMode, nil, sourcesExistState},
            {"   Offset (frames)",  sourceAudioOffsetFramesMode, nil, sourcesExistState},
            {"   Balance", sourceBalanceMode,   nil, sourcesExistState},
            {"_", nil},
            {"   Reset All Offsets", resetAudioOffsets, nil, nil},
        }},
        {"Image", Menu {
                {"Color Resolution", Menu {
                    {"Allow Floating Point", ~toggleFloat, nil, isFloatAllowed},
                    {"_", nil},
                    {"Maximum Allowed", ~setBitDepthFunc(0), nil, bitDepthState(0)},
                    {"8", ~setBitDepthFunc(8), nil, bitDepthState(8)},
                    {"16", ~setBitDepthFunc(16), nil, bitDepthState(16)},
                    {"32", ~setBitDepthFunc(32), nil, bitDepthState(32)}}},
                {"Image Resolution", Menu {
                    {"1.0",  ~setImageResolution(1.0), nil, imageResState(1.0)},
                    {"0.5",  ~setImageResolution(0.5), nil, imageResState(0.5)},
                    {"0.25", ~setImageResolution(0.25), nil, imageResState(0.25)},
                    {"_", nil},
                    {"Custom...", enterRes, nil, videoSourcesAndNodeExistState("RVFormat")}}},
                {"Scale", Menu {
                    {"1:1",  ~pixelRelativeScale(1.0),  "1", videoSourcesExistState},
                    {"_", nil},
                    {"2:1",  ~pixelRelativeScale(2.0),  "2", videoSourcesExistState},
                    {"4:1",  ~pixelRelativeScale(4.0),  "4", videoSourcesExistState},
                    {"8:1",  ~pixelRelativeScale(8.0),  "8", videoSourcesExistState},
                    {"_", nil},
                    {"1:2",  ~pixelRelativeScale(1.0/2.0),  "control 2", videoSourcesExistState},
                    {"1:4",  ~pixelRelativeScale(1.0/4.0),  "control 4", videoSourcesExistState},
                    {"1:8",  ~pixelRelativeScale(1.0/8.0),  "control 8", videoSourcesExistState}}},
                {"Rotation", Menu {
                    {"No Rotation",         ~rotateImage(0.0, false),    nil, rotateState(0.0)},
                    {"90 Clockwise",        ~rotateImage(90.0, false),   nil, rotateState(90.0)},
                    {"90 Counter-Clockwise",~rotateImage(-90.0, false),  nil, rotateState(-90.0)},
                    {"180",                 ~rotateImage(180.0, true),  nil, rotateState(180.0)},
                    {"_", nil},
                    {"+90 Clockwise",        ~rotateImage(90.0, true),   nil, videoSourcesAndNodeExistState("RVTransform2D")},
                    {"+90 Counter-Clockwise",~rotateImage(-90.0, true),  nil, videoSourcesAndNodeExistState("RVTransform2D")},
                    {"_", nil},
                    {"Arbitrary (Rotate Mode)", rotateMode,   nil, videoSourcesAndNodeExistState("RVTransform2D")}}},
                {"Alpha Type", Menu {
                    {"From Image",  ~alphaTypeFunc(0),  nil, alphaTypeState(0)},
                    {"_", nil},
                    {"Unpremultiplied",  ~alphaTypeFunc(2),  nil, alphaTypeState(2)},
                    {"Premultiplied",  ~alphaTypeFunc(1),  nil, alphaTypeState(1)}}},
                {"Pixel Aspect Ratio", Menu {
                    {"From Image",  ~pixelAspectFunc(0.0),  nil, aspectState(0.0)},
                    {"_", nil},
                    {"Square",  ~pixelAspectFunc(1.0),  nil, aspectState(1.0)},
                    {"NTSC D1 DV  4:3",  ~pixelAspectFunc(0.9),  nil, aspectState(0.9)},
                    {"NTSC D1 DV  16:9", ~pixelAspectFunc(1.2), nil, aspectState(1.2)},
                    {"PAL  4:3",         ~pixelAspectFunc(1.06666),  nil, aspectState(1.0666)},
                    {"PAL  16:9",        ~pixelAspectFunc(1.422), nil, aspectState(1.422)},
                    {"Anamorphic  2:1",  ~pixelAspectFunc(2.0), nil, aspectState(2.0)},
                    {"_", nil},
                    {"Custom...", enterPixAspect, nil, videoSourcesAndNodeExistState("RVLensWarp")}}},
                {"Stereo", Menu {
                    {"Swap Eyes", ~toggleSourceSwapEyes, nil, propToggledAndStereoOnState("#RVSourceStereo.stereo.swap")},
                    {"Flip Right Eye", ~toggleSourceRFlip, nil, propToggledAndStereoOnState("#RVSourceStereo.rightTransform.flip")},
                    {"Flop Right Eye", ~toggleSourceRFlop, nil, propToggledAndStereoOnState("#RVSourceStereo.rightTransform.flop")},
                    {"_", nil},
                    {"Interactive Edit",      nil, nil, inactiveState },
                    {"    Relative Eye Offset (%)", sourceStereoOffsetMode, nil, nil},
                    {"    Right Eye Only Offset (%)", sourceStereoROffsetMode, nil, nil},
                    }},
            {"_", nil},
            {"Flip",            ~toggleFlip, "Y", toggleFlipState},
            {"Flop",            ~toggleFlop, "X", toggleFlopState},
            {"_", nil},
            {"Remap Source Image Channels...", enterChanMap, nil, videoSourcesAndNodeExistState("RVChannelMap")},
            {"_", nil},
            {"Set Source FPS...", enterSourceFPS, nil, sourcesExistState},
            {"_", nil},
            {"Cycle Stack Forward", cycleStackForward, ")", isStackMode},
            {"Cycle Stack Backward", cycleStackBackward, "(", isStackMode}
            }},
        {"Color", Menu {
                {"Luminance Look Up Table", Menu {
                    {"Active",   ~toggleLuminanceLUT, "T", isLuminanceLUTActiveState},
                    {"_", nil},
                    {"Luminance LUTS", nil, nil, inactiveState},
                    {"   HSV ",   ~HSVLUT,    nil, luminanceLUTState("HSV")},
                    {"   Contour 3",   ~contourLUT(3),    nil, luminanceLUTState("Contour 3")},
                    {"   Contour 4",   ~contourLUT(4),    nil, luminanceLUTState("Contour 4")},
                    {"   Contour 6",   ~contourLUT(6),    nil, luminanceLUTState("Contour 6")},
                    {"   Contour 10",   ~contourLUT(10),    nil, luminanceLUTState("Contour 10")},
                    {"   Contour 20",   ~contourLUT(20),    nil, luminanceLUTState("Contour 20")},
                    {"   Contour 100",   ~contourLUT(100),  nil, luminanceLUTState("Contour 100")},
                    {"   Random",   ~randomLUT,  "*", luminanceLUTState("Random")}}},
                    //
                    // XXX I am not sure what this is supposed to do but it only generates errors right now (jon)
                    //
                    //{"_", nil},
                    //{"output LUT to shell",   ~outputLUT,    nil, nil}}},
            {"_", nil},
            {"File Nonlinear to Linear Conversion", nil, nil, inactiveState},
            {"   No Conversion",    setLinConvert(""), nil, hasLinConversion("")},
            {"   Cineon/DPX Log",   setLinConvert("Cineon Log"), "L", hasLinConversion("Cineon Log")},
            {"   ALEXA LogC",       setLinConvert("ALEXA LogC"), nil, hasLinConversion("ALEXA LogC")},
            {"   ALEXA LogC Film",  setLinConvert("ALEXA LogC Film"), nil, hasLinConversion("ALEXA LogC Film")},
            //{"   SONY S-Log",       setLinConvert("SONY S-Log"), nil, hasLinConversion("SONY S-Log")},
            {"   Viper Log",        setLinConvert("Viper Log"), nil, hasLinConversion("Viper Log")},
            {"   Red Log",          setLinConvert("Red Log"), nil, hasLinConversion("Red Log")},
            {"   Red Log Film",     setLinConvert("Red Log Film"), nil, hasLinConversion("Red Log Film")},
            {"   sRGB",             setLinConvert("sRGB"), nil, hasLinConversion("sRGB")},
            {"   Rec709",           setLinConvert("Rec709"), nil, hasLinConversion("Rec709")},
            {"   File Gamma 2.2",        setLinConvert("Gamma 2.2"), nil, hasLinConversion("Gamma 2.2")},
            {"   File Gamma...",         enterFileGamma, nil, fileGammaState},
            {"_", nil},
            {"Pre-Cache LUT", ~toggleCacheLUT, nil, isCacheLUTActiveState},
            {"File LUT",      ~toggleFileLUT, nil, isFileLUTActiveState},
            {"File CDL",      ~toggleFileCDL, nil, isFileCDLActiveState},
            {"File ICC",      ~toggleFileICC, nil, isFileICCActiveState},
            {"Look LUT",      ~toggleLookLUT, nil, isLookLUTActiveState},
            {"Look CDL",      ~toggleLookCDL, nil, isLookCDLActiveState},
            {"_", nil},
            {"Invert", ~toggleInvert, "I", isInvert},
            //  RVHistogram node deprecated to remove from GUI for now
            //{"Normalize", toggleNormalizeColor, nil, isNormalizingColor},
            {"_", nil},
            {"Interactive Edit",      nil, nil, inactiveState },
            {"    Gamma",      gammaMode,      "y", videoSourcesAndNodeExistState("RVColor")},
            {"    Color Offset", colorOffsetMode,  nil,  videoSourcesAndNodeExistState("RVColor")},
            {"    Exposure",   exposureMode,   "e", videoSourcesAndNodeExistState("RVColor")},
            {"    Saturation", saturationMode, "S", videoSourcesAndNodeExistState("RVColor")},
            {"    Hue",        hueMode,        "h", videoSourcesAndNodeExistState("RVColor")},
            {"    Contrast",   contrastMode,   "k", videoSourcesAndNodeExistState("RVColor")},
            {"_", nil},
            {"Range", Menu {
                {"From Image", ~setColorSpaceAttr("Range","From Image"), nil, matchesColorSpaceAttr("Range","From Image")},
                {"_", nil},
                {"Video Range", ~setColorSpaceAttr("Range","Video Range"), nil, matchesColorSpaceAttr("Range","Video Range")},
                {"Full Range", ~setColorSpaceAttr("Range","Full Range"), nil, matchesColorSpaceAttr("Range","Full Range")}}},
            {"_", nil},
            {"Ignore File Primaries", ~toggleChromaticities, nil, isIgnoringChromaticies},
            {"Reset All Color", ~resetAllColorParameters, "shift Home", videoSourcesAndNodeExistState("RVColor")}
            }},
        {"View", buildViewMenu() }
        //{"Debug", buildDebugMenu() }
    };


    let F = globalConfig.menuBarCreationFunc,
        a = mainMenu_part1,
        b = combine(a, if F eq nil then nil else F()),
        c = combine(b, userMenus);

    return c;
}

//
//  For backwards compat
//

//global Menu mainMenu = buildMainMenu();


\: retainScaleAfterResize (void; int oldW, int oldH, int newW, int newH)
{
    if (oldW <= 0 || oldH <= 0 || newW <= 0 || newH <= 0) return;

    let oldVA = float(oldW)/float(oldH),
        newVA = float(newW)/float(newH),
        ca = contentAspect();

    if (ca == 0.0) return;

    if (oldVA >= ca)
    {
        if (newVA >= ca) setScale (scale() * (float(oldH)/float(newH)));
        else
        {
            //
            //  In this case the scale only changes to the point where
            //  the aspects are equal, so find that H.
            //
            float targetH = float(newW/ca);
            setScale (scale() * (float(oldH)/float(targetH)));
        }
    }
    if (oldVA < ca)
    {
        if (newVA < ca) setScale (scale() * (float(oldW)/float(newW)));
        else
        {
            //
            //  In this case the scale only changes to the point where
            //  the aspects are equal, so find that W.
            //
            float targetW = float(newH*ca);
            setScale (scale() * (float(oldW)/float(targetW)));
        }
    }
}

\: viewResized (Event event)
{
    event.reject();

    State state = data();
    if (state eq nil || !state.lockResizeScale || presentationMode()) return;
    
    //
    // Skip the first resize if the user requested it in the prefs
    //

    if (state.startupResize)
    {
        state.startupResize = false; // from now we should resize the view;
        return;
    }

    let sizes = string.split(event.contents(), "|"),
        m     = margins(),
        oldWH = string.split(sizes[0], " "),
        oldW  = int(oldWH[0]) - m[0] - m[1],
        oldH  = int(oldWH[1]) - m[2] - m[3],
        newWH = string.split(sizes[1], " "),
        newW  = int(newWH[0]) - m[0] - m[1],
        newH  = int(newWH[1]) - m[2] - m[3];

    retainScaleAfterResize (oldW, oldH, newW, newH);
}

\: marginsChanged (Event event)
{
    event.reject();

    State state = data();
    if (state eq nil || !state.lockResizeScale || presentationMode()) return;

    if (state.startupResize) return;

    let oldM = string.split(event.contents(), " "),
        m0    = int(oldM[0]),
        m1    = int(oldM[1]),
        m2    = int(oldM[2]),
        m3    = int(oldM[3]),
        vs    = viewSize(),
        m     = margins(),
        oldW  = vs[0] - m0 - m1,
        oldH  = vs[1] - m2 - m3,
        newW  = vs[0] - m[0] - m[1],
        newH  = vs[1] - m[2] - m[3];

    if (m0 != m[0] || m1 != m[1] || m2 != m[2] || m3 != m[3])
    {
        retainScaleAfterResize (oldW, oldH, newW, newH);
    }
}

\: stateInitialized (void; Event event)
{
    event.reject();

    //  
    //  Write network port to user-supplied filename
    //
    let portFile = commands.commandLineFlag("networkPortFile", nil);
    if (portFile neq nil) 
    {
        remoteNetwork(true);
        try 
        {
            io.ofstream o = io.ofstream (portFile);
            io.print (o, "%s\n" % commands.myNetworkPort());
            o.close();
        }
        catch (...)
        {
            print ("ERROR: failed to write networkPortFile '%s'.\n" % portFile);
        }
    }

    let SettingsValue.Bool b = readSetting ("Tools", "show_session_manager", SettingsValue.Bool(false));
    if (b)
    {
        require mode_manager;

        State s = data();
        mode_manager.ModeManagerMode mmm = s.modeManager;
        mmm.activateMode("session_manager", true);
    }

    let SettingsValue.Bool ew = readSetting ("View","expandWidth", SettingsValue.Bool(expandWidth));
    expandWidth = if (ew) then true else false;
}

\: moveCallback (void; Event event)
{
    try
    {
        recordPixelInfo(event);
        redraw();
        event.reject();
    }
    catch (...) {;}
}

\: stateOrInputsChanged (void; Event event)
{
    event.reject();
    try 
    {
        State s = data();
        s.perPixelInfoValid = false;
        redraw();
    } catch (...) {;}
}

\: updateStateFromPrefs (void; State s)
{
    let SettingsValue.Bool b5 = readSetting ("Controls", "disableScrubInView", SettingsValue.Bool(false));
    s.scrubDisabled = b5;

    let SettingsValue.Bool b6 = readSetting ("Controls", "disableClickToPlay", SettingsValue.Bool(false));
    s.clickToPlayDisabled = b6;
}

\: updateFromPrefs (void; Event event)
{
    event.reject();

    State s = data();

    updateStateFromPrefs (s);
}

//----------------------------------------------------------------------
//
//  Session Initialization functions
//

\: defineDefaultBindings (void;)
{
    //
    //  All of these can call event.contents() to get a string
    //  from the event which gives specific info about what 
    //  happened.
    //
    //  Try:
    //
    //      bind("new-source", sourceSetup);
    //
    //  for a version that sets up color conversions automatically based on
    //  colorspace, file type, and file attributes.  the
    //  sourceSetupNoColor() function and sourceSetup() are located in the
    //  file mu_source_setup.mu. Hacking that file or simply binding your
    //  own version of the function to new-source is highly recommended.
    //

    //THIS IS NOW DONE BY THE color_management PACKAGE
    //bind("new-source", sourceSetup(,false));

    //bind("source-modified", ...); // "sourceName;;media1;;media2;;..."
    //bind("media-relocated", ...); // "sourceName;;oldmedia;;newmedia"
    //bind("before-session-write", ...); "filename"
    //bind("after-session-write", ...); "filename"
    //bind("before-session-write-copy", ...); "filename"
    //bind("after-session-write-copy", ...); "filename"
    //bind("graph-layer-change", ...);  // "stack" or "sequence"
    //bind("view-size-changed", ...);  // render event

    //
    // E.g, For hiding/showing mouse. These indicate the user has become
    // passive or active
    //

    bind("user-active", setActiveState);
    bind("user-inactive", setInactiveState);

    //
    //  If you bind these to something else you are responsible for
    //  drawing/layout of everything
    //

    bind("render", render);
    bind("layout", layout);
    bind("render-output-device", renderOutputDevice);
    bind("pre-render", preRender);

    //
    //  Missing frames will produce this event
    //

    bind("missing-image", missingImage);

    //
    //  How pixel blocks are handled (sent from display drivers, etc)
    //

    bind("pixel-block", defaultPixelBlockHandler);

    //
    //  The main stuff
    //
    bind("graph-state-change", stateOrInputsChanged, "Graph State Changed");
    bind("graph-node-inputs-changed", stateOrInputsChanged, "Graph Node Inputs Changed");

    bind("pointer--move", moveCallback);
    bind("pointer--enter", pointerEnterSession);
    bind("pointer--activate", windowActivate);
    bind("pointer--leave", pointerLeaveSession);
    bind("key-down-- ", togglePlayFunc, "Toggle Play");
    bind("key-down--(", cycleStackBackward, "Cycle Image Stack Backwards");
    bind("key-down--)", cycleStackForward, "Cycle Image Stack Forwards");
    bind("key-down--*", randomLUT, "Apply Random Luminance LUT");
    bind("key-down--,", incN(-1), "Set Frame Increment to -1 (reverse)");
    bind("key-down--.", incN(1), "Set Frame Increment to 1 (forward)");
    bind("key-down--1", pixelRelativeScale(1.0), "Scale 1:1");
    bind("key-down--2", pixelRelativeScale(2.0), "Scale 2:1");
    bind("key-down--3", pixelRelativeScale(3.0), "Scale 3:1");
    bind("key-down--4", pixelRelativeScale(4.0), "Scale 4:1");
    bind("key-down--5", pixelRelativeScale(5.0), "Scale 5:1");
    bind("key-down--6", pixelRelativeScale(6.0), "Scale 6:1");
    bind("key-down--7", pixelRelativeScale(7.0), "Scale 7:1");
    bind("key-down--8", pixelRelativeScale(8.0), "Scale 8:1");
    bind("key-down--A", toggleRealtime, "Toggle Real-Time Playback");
    bind("key-down--C", toggleCacheModeFunc(CacheGreedy), "Toggle Region Caching");
    bind("key-down--D", toggleDisplayLUT, "Toggle Display LUT");
    bind("key-down--F", enterFPS, "Enter FPS Value From Keyboard");
    bind("key-down--G", enterFrame, "Set Frame Number Using Keyboard");
    bind("key-down--I", toggleInvert, "Toggle Color Invert");
    bind("key-down--L", setLinConvert("Cineon Log"), "Toggle Cineon Log to Linear Conversion");
    bind("key-down--M", cycleMatteOpacity, "Cycle Matte Opacity");
    bind("key-down--P", togglePingPong, "Toggle Ping/Pong Playback");
    bind("key-down--R", ~reload, "Force Reload of Current Source");
    bind("key-down--T", toggleLuminanceLUT, "Toggle Current Luminance LUT");
    bind("key-down--X", toggleFlop, "Flop Image");
    bind("key-down--Y", toggleFlip, "Flip Image");
    bind("key-down--[", frameFunc(setInPoint), "Set In Point");
    bind("key-down--\\", resetInOutPoints, "Reset In/Out Points");
    bind("key-down--]", frameFunc(setOutPoint), "Set Out Point");
    bind("key-down--a", showChannel(4), "Show Alpha Channel");
    bind("key-down--b", showChannel(3), "Show Blue Channel");
    bind("key-down--c", showChannel(0), "Normal Color Channel Display");
    bind("key-down--alt--l", ~rotateImage(-90, true), "Rotate Image 90deg Counter-Clockwise");
    bind("key-down--alt--r", ~rotateImage(90, true), "Rotate Image 90deg Clockwise");
    bind("key-down--alt--left", previousMarkedFrame, "Go to Previous Marked Frame");
    bind("key-down--alt--right", nextMarkedFrame, "Go to Next Marked Frame");
    bind("key-down--<", previousMatchedFrame, "Go to Matching Frame of Previous Source");
    bind("key-down-->", nextMatchedFrame, "Go to Matching Frame of Next Source");
    bind("key-down--shift--left", prevView, "Go to Previous View");
    bind("key-down--shift--right", nextView, "Go to Next View");
    bind("key-down--control--e", exportAs(, "mov", "Quicktime Export"), "Export Quicktime Movie");
    bind("key-down--control--q", queryClose, "Close Session");
    bind("key-down--control--N", clearEverything, "Clear Session");
    bind("key-down--control--S", saveAs, "Save Session As");
    bind("key-down--control--i", addMovieOrImageSources(,true,false), "Add Source");
    bind("key-down--control--s", save, "Save Session");
    bind("key-down--control--w", queryClose, "Close Session");
    bind("key-down--control--o", addMovieOrImageSources(,true,false), "Open File");
    bind("key-down--control--O", openMovieOrImage, "Open in New Session");
    bind("key-down--control--1", pixelRelativeScale(1.0/1.0), "Scale 1:1");
    bind("key-down--control--2", pixelRelativeScale(1.0/2.0), "Scale 1:2");
    bind("key-down--control--3", pixelRelativeScale(1.0/3.0), "Scale 1:3");
    bind("key-down--control--4", pixelRelativeScale(1.0/4.0), "Scale 1:4");
    bind("key-down--control--5", pixelRelativeScale(1.0/5.0), "Scale 1:5");
    bind("key-down--control--6", pixelRelativeScale(1.0/6.0), "Scale 1:6");
    bind("key-down--control--7", pixelRelativeScale(1.0/7.0), "Scale 1:7");
    bind("key-down--control--8", pixelRelativeScale(1.0/8.0), "Scale 1:8");
    bind("key-down--control--l", toggleCacheModeFunc(CacheBuffer), "Toggle Look-Ahead Caching");
    bind("key-down--control--m", cycleMatte, "Cycle Mattes");
    bind("key-down--control--left", previousMarkedRange, "Set In/Out to Previous Marked Range");
    bind("key-down--control--right", nextMarkedRange, "Set In/Out to Next Marked Range");
    bind("key-down--control--up", expandMarkedRange, "Expand In/Out to Neighboring Marked Ranges");
    bind("key-down--control--down", contractMarkedRange, "Contract In/Out from Neighboring Marked Ranges");
    bind("key-down--control--v", globalVolumeMode, "Edit Global Audio Volume");
    bind("key-down--control--R", ~reloadInOut, "Reload In/Out Region");
    bind("key-down--control--C", ~loadCurrentSourcesChangedFrames, "Load Changed Frames of Current Sources");
    bind("key-down--control--f", frameWidth, "Frame Image Width");
    bind("key-down--down", togglePlayFunc, "Toggle Play");
    bind("key-down--end", ending, "Go to End of In/Out Region");
    bind("key-down--f", frameImage, "Frame Image in View");
    bind("key-down--f1", toggleMenuBar, "Toggle Menu Bar Visibility");
    bind("key-down--f2", toggleTimeline, "Toggle Heads-Up Timeline");
    bind("key-down--f3", toggleMotionScope, "Toggle Timeline Magnifier");
    bind("key-down--f4", toggleInfo, "Toggle Heads-Up Image Info");
    bind("key-down--f5", toggleColorInspector, "Toggle Heads-Up Color Inspector");
    bind("key-down--f6", toggleWipe, "Toggle Wipes");
    bind("key-down--f7", toggleInfoStrip, "Toggle Heads-Up Info Strip");
    bind("key-down--f8", toggleProcessInfo, "Toggle Heads-Up External Process Progress");
    bind("key-down--f11", toggleSourceDetails, "Toggle Heads-Up Source Details");
    bind("key-down--g", showChannel(2), "Show Green Channel");
    bind("key-down--home", beginning, "Go to Beginning of In/Out Range");
    bind("key-down--shift--home", resetAllColorParameters, "Reset All Color");
    bind("key-down--i", toggleInfo, "Toggle Heads-Up Image Info");
    bind("key-down--l", showChannel(5), "Show Image Luminance");
    bind("key-down--left", stepBackward1, "Move Back One Frame");
    bind("key-down--m", toggleMark, "Toggle Mark At Frame");
    bind("key-down--n", toggleFilter, "Toggle Nearest Neighbor/Linear Filter");
    bind("key-down--p", togglePremult, "Toggle Premult Display");
    bind("key-down--page-down", previousMarkedRange, "Set In/Out to Previous Marked Range");
    bind("key-down--page-up", nextMarkedRange, "Set In/Out to Next Marked Range");
    bind("key-down--control--p", togglePresentationMode, "Toggle Presentation Mode");
    bind("key-down--r", showChannel(1), "Show Red Channel");
    bind("key-down--right", stepForward1, "Step Forward 1 Frame");
    bind("key-down--t", toggleTimeline, "Toggle Heads-Up Timeline");
    bind("key-down--tab", toggleTimeline, "Toggle Heads-Up Timeline");
    bind("key-down--up", toggleForwardsBackwards, "Toggle Forward/Backward Playback");
    bind("key-down--v", enterDispGamma, "Enter Display Gamma");
    bind("key-down--|", setInOutMarkedRange, "Set In/Out Range From Surrounding Marks");
    bind("key-down--~", toggleTimeline, "Toggle Timeline");
    bind("pointer--wheeldown", stepForward1, "Step Forward 1 Frame");
    bind("pointer--wheelup", stepBackward1, "Step Backward 1 Frame");
    bind("pointer-3--wheeldown", stepForward10, "Step Forward 10 Frames");
    bind("pointer-3--wheelup", stepBackward10, "Step Backward 10 Frames");
    bind("pointer-2-3--wheeldown", stepForward100, "Step Forward 100 Frames");
    bind("pointer-2-3--wheelup", stepBackward100, "Step Backward 100 Frames");
    bind("pointer--control--wheeldown", stepForward10, "Step Forward 10 Frames");
    bind("pointer--control--wheelup", stepBackward10, "Step Backward 10 Frames");
    bind("pointer--alt-control--wheeldown", stepForward100, "Step Forward 100 Frames");
    bind("pointer--alt-control--wheelup", stepBackward100, "Step Backward 100 Frames");
    bind("pointer-1--alt--drag", dragMoveLocked(false,), "Translate View");
    bind("pointer-1--alt-shift--drag", dragMoveLocked(true,), "Translate View");
    bind("pointer-1--alt--push", beginMoveOrZoom);
    bind("pointer-1--alt-shift--push", beginMoveOrZoom);
    bind("pointer-1--alt-control--drag", dragMoveLocked(false,), "Translate View");
    bind("pointer-1--alt-shift-control--drag", dragMoveLocked(true,), "Translate View");
    bind("pointer-1--alt-control--push", beginMoveOrZoom);
    bind("pointer-1--alt-shift-control--push", beginMoveOrZoom);
    bind("pointer-1--control--drag", dragZoom);
    bind("pointer-1--control--push", beginMoveOrZoom);
    bind("pointer-1--control-shift--push", beginMoveOrZoom);
    bind("pointer-1--drag", dragScrub(false,), "Scrub Frames");
    bind("pointer-1--push", beginScrub);
    bind("pointer-1--release", releaseScrub);
    bind("pointer-1--double",  doubleClick);
    bind("pointer-1-2--alt--drag", dragZoom, "Zoom View");
    bind("pointer-1-2--alt--push", beginMoveOrZoom);
    bind("pointer-1-2--alt-shift--push", beginMoveOrZoom);
    bind("pointer-2--alt--drag", dragMoveLocked(false,), "Translate View");
    bind("pointer-2--alt-shift--drag", dragMoveLocked(true,), "Translate View");
    bind("pointer-2--alt--push", beginMoveOrZoom);
    bind("pointer-2--alt-shift--push", beginMoveOrZoom);
    bind("pointer-2--control--drag", dragZoom, "Zoom View");
    bind("pointer-2--control--push", beginMoveOrZoom);
    bind("pointer-2--control-shift--push", beginMoveOrZoom);
    bind("pointer-2--drag", dragMoveLocked(false,), "Translate View");
    bind("pointer-2--shift--drag", dragMoveLocked(true,), "Translate View");
    bind("pointer-2--push", beginMoveOrZoom);
    bind("pointer-2--shift--push", beginMoveOrZoom);
    bind("pointer-3--push", popupMenu(,nil), "Popup Menu");

    bind("toggle-hud-info-widget", toggleInfo, "Toggle info widget via event");
    bind("toggle-hud-timeline-widget", toggleTimeline, "Toggle timeline widget via event");
    bind("toggle-hud-timeline-mag-widget", toggleMotionScope, "Toggle timeline magnifier widget via event");

    //
    //  back-door scrubbing, works even if scrubbing is "disabled"
    //
    bind("pointer-1--control-shift--push", beginScrub);
    bind("pointer-1--control-shift--drag", dragScrub(true,), "Scrub Frames");
    bind("pointer-1--control-shift--release", releaseScrub);
    bind("stylus-pen--control-shift--push", beginScrub);
    bind("stylus-pen--control-shift--drag", dragScrub(true,), "Scrub Frames");
    bind("stylus-pen--control-shift--release", releaseScrub);

    bind("pointer-1--shift--push", \: (void; Event event)
    {
        toggleColorInspector();
        event.reject();  // it will find the bindings in the inspector
    },
    "Color Inspector");

    bind("stylus-pen--shift--push", \: (void; Event event)
    {
        toggleColorInspector();
        event.reject();  // it will find the bindings in the inspector
    },
    "Color Inspector");

    bind("stylus-pen--move", moveCallback);
    bind("stylus-pen--enter", pointerEnterSession);
    bind("stylus-pen--leave", pointerLeaveSession);
    bind("stylus-pen--alt--drag", dragMoveLocked(false,), "Translate View");
    bind("stylus-pen--alt-shift--drag", dragMoveLocked(true,), "Translate View");
    bind("stylus-pen--alt--push", beginMoveOrZoom);
    bind("stylus-pen--alt-shift--push", beginMoveOrZoom);
    bind("stylus-pen--alt-control--drag", dragMoveLocked(false,), "Translate View");
    bind("stylus-pen--alt-shift-control--drag", dragMoveLocked(true,), "Translate View");
    bind("stylus-pen--alt-control--push", beginMoveOrZoom);
    bind("stylus-pen--alt-shift-control--push", beginMoveOrZoom);
    bind("stylus-pen--control--drag", dragZoom);
    bind("stylus-pen--control--push", beginMoveOrZoom);
    bind("stylus-pen--control-shift--push", beginMoveOrZoom);
    bind("stylus-pen--drag", dragScrub(false,), "Scrub Frames");
    bind("stylus-pen--push", beginScrub);
    bind("stylus-pen--release", releaseScrub);
    bind("stylus-eraser--push", popupMenu(,nil), "Popup Menu");

    bind("key-down--alt--alt", noop, "Intercept Menu Navigation");
    bind("key-up--alt--alt", noop, "Intercept Menu Navigation");

    bind("remote-connection-start", ~activateSync, "Auto start sync mode");

    bind("view-resized", viewResized, "RV View Resized");
    bind("margins-changed", marginsChanged, "RV View Margins Changed");
    bind("state-initialized", stateInitialized, "RVUI State Initialized");

    bind("key-down--keypad-enter", enterFrame, "Set Frame Number Using Keyboard");
    bindRegex("default", "global", "key-down--keypad-[0-9.].*", enterFrame);
    bindRegex("default", "global", ".*--caplock.*", lockWarning(,"cap-lock is ON"));
    bindRegex("default", "global", ".*--scrolllock.*", lockWarning(,"scroll-lock is ON"));

    bindRegex("default", "global", "dragdrop--.*enter", dragEnter);
    bindRegex("default", "global", "dragdrop--.*move", dragShow);
    bindRegex("default", "global", "dragdrop--.*leave", dragLeave);
    bindRegex("default", "global", "dragdrop--.*release", dragRelease);

    //
    //  Hack to "scrub" float parameters
    //

    bind("key-down--e", exposureMode, "Edit Current Source Relative Exposure");
    bind("key-down--S", saturationMode, "Edit Current Source Saturation");
    bind("key-down--h", hueMode, "Edit Current Source Hue");
    bind("key-down--y", gammaMode, "Edit Current Source Gamma");
    bind("key-down--k", contrastMode, "Edit Current Source Contrast");
    bind("key-down--B", brightnessMode, "Edit Display Brightness");
    bind("default", "paramscrub", "pointer-1--push", beginParamScrub);
    bind("default", "paramscrub", "pointer-1--drag", dragParamScrub, "Scrub Modify");
    bind("default", "paramscrub", "pointer-1--release", releaseParam);
    bind("default", "paramscrub", "stylus-pen--push", beginParamScrub);
    bind("default", "paramscrub", "stylus-pen--drag", dragParamScrub, "Scrub Modify");
    bind("default", "paramscrub", "stylus-pen--release", releaseParam);
    bind("default", "paramscrub", "key-down--?", helpParam, "Help on Edit");
    bind("default", "paramscrub", "key-down--R", resetParam, "Reset Edit to Default");
    bind("default", "paramscrub", "key-down--delete", resetParam, "Reset Edit to Default");
    bind("default", "paramscrub", "key-down--backspace", resetParam, "Reset Edit to Default");
    bind("default", "paramscrub", "key-down--s", selectParamTile, "Select Tile for Param Editing");
    bind("default", "paramscrub", "key-down--r", setParamChannel(0,), "Edit Red Channel");
    bind("default", "paramscrub", "key-down--g", setParamChannel(1,), "Edit Green Channel");
    bind("default", "paramscrub", "key-down--b", setParamChannel(2,), "Edit Blue Channel");
    bind("default", "paramscrub", "key-down--c", setParamChannel(-1,), "Edit All Channels");
    bind("default", "paramscrub", "key-down--l", toggleParamLocked, "Lock/Unlock Param Edit Mode");
    bind("default", "paramscrub", "key-down--=", incrementParam, "Edit Increment");
    bind("default", "paramscrub", "key-down---", decrementParam, "Edit Decrement");
    bind("default", "paramscrub", "key-down--+", incrementParam, "Edit Increment");
    bind("default", "paramscrub", "key-down--_", decrementParam, "Edit Decrement");
    bind("default", "paramscrub", "key-down--enter", enterParam, "Edit Enter Value Directly");
    bind("default", "paramscrub", "key-down--keypad-enter", enterParam, "Edit Enter Value Directly");
    bind("default", "paramscrub", "pointer--wheelup", incrementParam, "Edit Increment");
    bind("default", "paramscrub", "pointer--wheeldown", decrementParam, "Edit Decrement");
    bindRegex("default", "paramscrub", "key-down--(keypad-)?[0-9.].*", enterParam, "Edit Enter Value Directly and Self Insert");
    bindRegex("default", "paramscrub", "key-down-- ", releaseParamForce);
    bindRegex("default", "paramscrub", "key-down--escape", releaseParamForce);
    bindRegex("default", "paramscrub", "key-down--q", releaseParamForce);
    bindRegex("default", "paramscrub", "key-down.*", releaseParam);

    bind("default", "textentry", "key-down--enter", commitEntry);
    bind("default", "textentry", "key-down--keypad-enter", commitEntry);
    bind("default", "textentry", "key-down--backspace", backwardDeleteChar);
    bind("default", "textentry", "key-down--delete", backwardDeleteChar);
    bind("default", "textentry", "key-down--escape", cancelEntry);
    bind("default", "textentry", "key-down--control--a", killAllText);
    bind("default", "textentry", "key-down--meta--a", killAllText);
    bindRegex("default", "textentry", "^key-down--..+", ignoreEntry);
    bindRegex("default", "textentry", "^key-down.*", selfInsert);
    bindRegex("default", "textentry", "^pointer-.*--push", cancelEntry);

    bind("default", "stereo", "key-down--a", setStereo("anaglyph"), "Anaglyph Mode");
    bind("default", "stereo", "key-down--l", setStereo("lumanaglyph"), "Luminance Anaglyph Mode");
    bind("default", "stereo", "key-down--d", setStereo("checker"), "Checked Stereo Mode");
    bind("default", "stereo", "key-down--k", setStereo("scanline"), "Scanline Stereo Mode");
    bind("default", "stereo", "key-down--s", setStereo("pair"), "Side-by-Side Stereo Mode");
    bind("default", "stereo", "key-down--p", setStereo("pair"), "Side-by-Side Stereo Mode");
    bind("default", "stereo", "key-down--m", setStereo("mirror"), "Mirrored Side-by-Side Stereo Mode");
    bind("default", "stereo", "key-down--z", setStereo("hsqueezed"), "Horizontal Squeezed Stereo Mode");
    bind("default", "stereo", "key-down--v", setStereo("vsqueezed"), "Vertical Squeezed Stereo Mode");
    bind("default", "stereo", "key-down--h", setStereo("hardware"), "Hardware Stereo Mode");
    bind("default", "stereo", "key-down--,", setStereo("left"), "Left Eye Only Stereo Mode");
    bind("default", "stereo", "key-down--.", setStereo("right"), "Right Eye Only Stereo Mode");
    bind("default", "stereo", "key-down--<", setStereo("left"), "Left Eye Only Stereo Mode");
    bind("default", "stereo", "key-down-->", setStereo("right"), "Right Eye Only Stereo Mode");
    bind("default", "stereo", "key-down--O", setStereo("off"), "Stereo Off");
    bind("default", "stereo", "key-down--S", toggleSwapEyes, "Swap Eyes");
    bind("default", "stereo", "key-down--o", stereoOffsetMode, "Edit Stereo Offset");
    bind("default", "stereo", "key-down--r", stereoROffsetMode, "Edit Right-Eye-Only Stereo Offset");
    bind("default", "stereo", "key-down--c", sourceStereoOffsetMode, "Edit Source/Clip Stereo Offset");
    bind("default", "stereo", "key-down--R", sourceStereoROffsetMode, "Edit Source/Clip Right-Eye-Only Stereo Offset");
    bind("default", "stereo", "key-down--/", resetStereoOffsets, "Reset Stereo Offsets");
    bind("default", "stereo", "key-down--alt--s", releaseStereo, "Turn Off Stereo Keys");

    bind("key-down--alt--s", \: (void; Event ev)
    {
        displayFeedback("Stereo Keys (a, d, s, h, <, >, m, S, o, toggle off)...", 10e6);
        redraw();
        pushEventTable("stereo");
    }, "Turn On Stereo Keys");

    bind("default", "nudge", "key-down--right", nudge(-.01,0,1), "Nudge Right");
    bind("default", "nudge", "key-down--left", nudge(.01,0,1), "Nudge Left");
    bind("default", "nudge", "key-down--up", nudge(0,-.01,1), "Nudge Up");
    bind("default", "nudge", "key-down--down", nudge(0,.01,1), "Nudge Down");
    bind("default", "nudge", "key-down--control--right", nudge(-0.001,0,1), "Nudge Right Tiny");
    bind("default", "nudge", "key-down--control--left", nudge(0.001,0,1), "Nudge Left Tiny");
    bind("default", "nudge", "key-down--control--up", nudge(0,-.001,1), "Nudge Up Tiny");
    bind("default", "nudge", "key-down--control--down", nudge(0,.001,1), "Nudge Down Tiny");
    bind("default", "nudge", "key-down--shift--right", nudge(-.1,0,1), "Nudge Right Big");
    bind("default", "nudge", "key-down--shift--left", nudge(.1,0,1), "Nudge Left Big");
    bind("default", "nudge", "key-down--shift--up", nudge(0,-.1,1), "Nudge Up Big");
    bind("default", "nudge", "key-down--shift--down", nudge(0,.1,1), "Nudge Down Big");
    bind("default", "nudge", "key-down--z", nudge(0,0,1.1), "Nudge Zoom-In");
    bind("default", "nudge", "key-down--control--z", nudge(0,0,1.01), "Nudge Big Zoom-Out");
    bind("default", "nudge", "key-down--alt--n", releaseNudge, "Turn Off Nudge Keys");

    bind("key-down--alt--n", \: (void; Event ev)
    {
        displayFeedback("Nudge Keys (toggle)...", 10e6);
        redraw();
        pushEventTable("nudge");
    }, "Turn On Nudge Keys");

    //-- help

    // Since we cannot set the audio cache mode until there is some audio,
    // make sure to set to greedy (scrub on) once we have a source, if the
    // scrubbing state is true.
    bind("after-progressive-loading", \: (void; Event event)
    {
        event.reject();
        State s = data();
        if (s.scrubAudio && audioCacheMode() != CacheGreedy)
        {
            setAudioCacheMode(CacheGreedy);
        }
    });

    //
    //  Handle preferences written by Preferences dialog (C++ side), but
    //  implemented here, in Mu.
    //
    
    bind("after-preferences-write", updateFromPrefs, "Preferences Updated"); 

    bind("before-session-deletion", sessionDeletionHandler, "Session Deletion Requested"); 

    //
    //  Handle resetting the matte menus to any saved matte state in the session
    //

    bind("after-session-read", updateStateMatte, "Check Matte Settings in Session");

    //
    //  Scariest event. This will eventually need to have some access
    //  list or something 
    //

    bind("remote-eval", \: (void; Event event)
    {
        let t = event.contents();

        try
        {
            event.setReturnContent(runtime.eval(t, ["commands"]));
        }
        catch (exception exc)
        {
            let s = "ERROR: remote-eval: %s\nERROR: remote-eval: '%s'\n" % (exc, t);
            print(s);
            event.setReturnContent(s);
        }
    });
}

\: clickTimerTimeout (void; )
{
    State state = data();
    if (! state.clickToPlayDisabled) togglePlayIfNoScrub();
}

\: dropTimerTimeout (void; )
{
    State state = data();

    string[] myFiles;
    for (int i = 0; i < state.ddDropFiles.size(); ++i) myFiles.push_back(state.ddDropFiles[i]);
    state.ddDropFiles.clear();
    state.ddProgressiveDrop = false;

    addSources(myFiles, "drop");
}


\: initStateObject(State; State s)
{
    s.config           = globalConfig;
    s.feedback         = 0;
    s.showMatte        = false;
    s.matteOpacity     = 0.66;
    s.matteAspect      = 1.85;
    s.textEntry        = false;
    s.scrubbed         = false;
    s.scrubDisabled    = false;
    s.playingBefore    = false;
    s.emptySessionStr  = "Empty Session";
    s.loadingCount     = 0;
    s.pointerInSession = true;
    s.dragDropOccuring = false;
    s.timeline         = nil;
    s.motionScope      = nil;
    s.sync             = nil;
    s.imageInfo        = nil;
    s.infoStrip        = nil;
    s.processInfo      = nil;
    s.sourceDetails    = nil;
    s.widgets          = Widget[]();
    s.minorModes       = MinorMode[]();
    s.firstRender      = true;
    s.userActive       = true;
    s.unsavedChanges   = false;

    s.perPixelFrame    = int.max;
    s.perPixelInfoValid = false;
    s.perPixelViewHash  = 0;
    s.perPixelPosition = Vec2(10.0, 10.0);

    //
    //  These are now dynamically loaded when they're first used
    //  (see toggleWipe and toggleColorInspector)
    //

    use mode_manager;
    s.modeManager = ModeManagerMode();
    s.minorModes.push_back(s.modeManager);

    use window_mode;
    s.minorModes.push_back(WindowMinorMode());

    use presentation_mode;
    s.minorModes.push_back(PresentationControlMinorMode());

    for_each (m; s.minorModes) m.toggle();

    s.lockResizeScale = false;
    try
    {
        //
        //  Turn on timeline by default.
        //
        let SettingsValue.Bool b1 = readSetting ("Tools", "show_timeline", SettingsValue.Bool(true));

        if (b1)
        {
            s.timeline = loadTimeline();
            s.timeline.toggle();
        }

        let SettingsValue.Bool b2 = readSetting ("Controls", "stepWraps", SettingsValue.Bool(false));
        s.stepWraps = b2;

        //
        //   Remember if moscope is on.
        //
        let SettingsValue.Bool b3 = readSetting ("Tools", "show_motionScope", SettingsValue.Bool(false));
        if (b3) toggleMotionScopeFromState(s);

        let SettingsValue.Bool b4 = readSetting ("View", "lockResizeScale", SettingsValue.Bool(false));
        s.lockResizeScale = b4;

        let SettingsValue.Bool b5 = readSetting ("Controls", "scrubClamps", SettingsValue.Bool(false));
        s.scrubClamps = b5;

        // Since we cannot set the audio cache mode until there is some audio,
        // set the scrubbing state for now. We will check this once we have a
        // source.
        let SettingsValue.Bool b6 = readSetting ("Audio", "audioScrub", SettingsValue.Bool(false));
        s.scrubAudio = b6;

        let SettingsValue.Float f1 = readSetting ("View", "matteOpacity", SettingsValue.Float(0.33));
        s.matteOpacity = f1;
        setFloatProperty("#Session.matte.opacity", float[]{s.matteOpacity}, true);

        updateStateFromPrefs(s);
    }
    catch (...) { ; }

    qt.QTimer ct = qt.QTimer(mainWindowWidget());
    ct.setSingleShot(true);
    ct.setInterval(100);
    qt.connect (ct, qt.QTimer.timeout, clickTimerTimeout);
    s.clickTimer = ct;

    qt.QTimer dt = qt.QTimer(mainWindowWidget());
    dt.setSingleShot(true);
    dt.setInterval(200);
    qt.connect (dt, qt.QTimer.timeout, dropTimerTimeout);
    s.ddDropTimer = dt;

    s.ddDropFiles = string[]();
    s.ddProgressiveDrop = false;

    s.quitConfirmMessages = (string,string)[]();
    s.savedInOut = int[]();
    
    s.startupResize = commands.startupResize();

    return s;
}

\: newStateObject (State;) { initStateObject(State()); }

\: minorModeFromName (MinorMode; string nm)
{
    State state = data();

    for_each (m; state.minorModes) if (m._modeName == nm) return m;

    return nil;
}

}   // END MODULE rvui
