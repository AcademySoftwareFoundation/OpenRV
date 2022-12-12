//******************************************************************************
// Copyright (c) 2001-2006 Tweak Inc. All rights reserved.
// 
// SPDX-License-Identifier: Apache-2.0
// 
//******************************************************************************
//
//  This is a real working piece of code that was used in a movie
//  to generate crowds. The processShot() function (at the end 
//  of the file) was called either by the standalone exporter or by
//  the renderer to generate the crowds on the fly.
//
//  The behavior of the module is controled by a user parameter file
//  which indicates weights for various combonations of sprite attributes.
//  The parser for the file is included in the module. Its very brute
//  force (not recursive decent, etc).
//

require io;

//----------------------------------------------------------------------
//
//  Types
//

Vec2            := math.vec2f;
Vec3            := math.vec3f;
Point           := math.vec3f;
WeightOp        := (float;float,float);
WeightFilter    := (regex,float);
WeightFilters   := WeightFilter[];
ActionFilter    := (regex,float);
ActionFilters   := ActionFilter[];
NamedFilter     := (string,regex);
NamedFilters    := NamedFilter[];
StaticFilter    := ActionFilter;
StaticFilters   := StaticFilter[];
SpriteFiles     := (string,string)[];
Override        := (int,string);          // Force id -> sprite
Overrides       := Override[];      

class: ActionType
{
    string  name;
    int     start;
    int     end;
}

class: Sprite
{
    string      name;
    string      file;
    float       weight;
    float       angle;
    bool        lit;
    ActionType  action;
    float       staticWeight;
    int         actor;
}

class: Instance
{
    Sprite  sprite;
    int     frameStart;
}

class: Parameters
{
    int             startFrame;
    int             endFrame;
    string          cacheOutput;
    string          obj;
    Point           target;
    int             seed;
    bool            allowFlip;
    bool            alwaysFlip;
    string          outfile;
    string          particles;
    Vec2            cardSize;
    int             offsetStart;
    int             offsetEnd;
    Vec2            staticOffset;
    string          suffix;
    string          geomfile;
    string          geomobject;
    string          densityMapFile;
    string          litMapFile;
    float           margin;
    float           skipProbability;
    float           stagger;
    float           seperation;
    int             emitSeed;
    Vec3            cardOffset;
    string          selectMethod;
    WeightFilters   weightFilters;
    ActionFilters   actionFilters;
    StaticFilters   staticFilters;
    Overrides       overrides;
    Sprite[]        sprites;
}

class: KeywordPattern
{
    string  word;
    regex   pattern;
}

Instances        := Instance[];
ActionTypes      := ActionType[];
SpriteDB         := Sprite[];
KeywordPatterns  := KeywordPattern[];
Backend          := (void;Parameters);
SelectionFunc    := (Instances; Parameters, int);
SelectionMethod  := (string, SelectionFunc);
SelectionMethods := SelectionMethod[];

//----------------------------------------------------------------------
//
//  Some helpful operators
//

\: select_1st (float; float a, float b) { a; }
\: select_2nd (float; float a, float b) { b; }

\: irandom (int; int a, int b)
{
    math_util.random(float(a), float(b) + 0.9999999999);
}

\: copy (Sprite; Sprite s)
{
    Sprite n       = Sprite();
    n.name         = s.name;
    n.file         = s.file;
    n.angle        = s.angle;
    n.lit          = s.lit;
    n.weight       = s.weight;
    n.action       = s.action;
    n.staticWeight = s.staticWeight;
    n;
}

\: nospaces (string; string text)
{
    let m = regex.smatch("([ \t]*)$", text),
        r = text.substr(0, -m[1].size());
    r;
}

\: parsePoint (Point; string text)
{
    let m = regex.smatch("[ \t]*(-?[0-9.]+)" +
                         "[ \t]*(-?[0-9.]+)?" +
                         "[ \t]*(-?[0-9.]+)?", text),
        x = float(m[1]),
        y = if m[2] == "" then x else float(m[2]),
        z = if m[3] == "" then y else float(m[3]);

    Point(x,y,z);
}

\: padNL (string; string text)
{
    regex.replace("\n", text, " \n");
}

\: sortSprites (Sprite[]; Sprite[] array)
{
    \: comp (int; Sprite s1, Sprite s2) 
    { 
        let d = s2.weight - s1.weight;
        if d < 0 then -1 else (if d > 0 then 1 else 0);
    }

    \: qsort (void; Sprite[] a, int lo, int hi)
    {
        if (lo < hi) 
        {
            let l = lo,
                h = hi,
                p = a[hi];

            do 
            {
                while (l < h && comp(a[l],p) <= 0) l++;
                while (h > l && comp(a[h],p) >= 0) h--;

                if (l < h)
                {
                    let t = a[l];
                    a[l] = a[h];
                    a[h] = t;
                }

            } while (l < h);

            let t = a[l];
            a[l] = a[hi];
            a[hi] = t;

            qsort(a, lo, l-1);
            qsort(a, l+1, hi);
        }
    }

    qsort(array, 0, array.size() - 1);
    array;
}

module: crowd {

//----------------------------------------------------------------------
//
//  SpriteType definitions
//
// frame 1 is synch mark
// frame 2 is start of "sit" action
// frame 242 "boo" action
// frame 482 "clap" action (seated)
// frame 842 "sit" (clapping ends)
// frame 962 "stand and cheer"
// frame 1202 "sit" (cheering ends)
// frame 1442 end

global ActionTypes action_types =
{
 { "sit1",      2,    241 },
 { "boo",       242,  481 },
 { "clap",      482,  841 },
 { "clap2",     621,  841 },
 { "sit2",      842,  961 },
 { "stand",     962,  1201 },
 { "standClap", 1101, 1201 },
 { "sit3",      1202, 1442 },
 { "sit4",      1282, 1442 }
};

global SpriteDB sprite_db       = nil;

global NamedFilters named_filters = 
{
    ("lit",             regex("in_sprite_k.*")),
    ("ambient",         regex("in_sprite_a.*")),
    ("forwardFacing",   regex("in_sprite_....f.*")),
    ("cameraFacing",    regex("in_sprite_....c.*")),
    ("winterClothes",   regex("in_sprite_.....w.*")),
    ("summerClothes",   regex("in_sprite_.....s.*")),
    ("aClothes",        regex("in_sprite_......a.*")),
    ("bClothes",        regex("in_sprite_......b.*")),
    ("teamNeutral",     regex("in_sprite_.........n.*")),
    ("teamCowboys",     regex("in_sprite_.........c.*")),
    ("teamEagles",      regex("in_sprite_.........e.*"))
};

global SpriteFiles sprite_files =
{
    // in_sprite_a000fsa:
     ("in_sprite_a000fsa71dA01", ""),
     ("in_sprite_a000fsa72dA01", ""),
     ("in_sprite_a000fsa74dA01", ""),
     ("in_sprite_a000fsa75dA01", ""),
     ("in_sprite_a000fsa76dA01", ""),
     ("in_sprite_a000fsa77dA01", ""),
     ("in_sprite_a000fsa78dA01", ""),
     ("in_sprite_a000fsa81dA01", ""),
     ("in_sprite_a000fsa82dA01", ""),
     ("in_sprite_a000fsa84dA01", ""),
     ("in_sprite_a000fsa85dA01", ""),
     ("in_sprite_a000fsa86dA01", ""),
     ("in_sprite_a000fsa87dA01", ""),
     ("in_sprite_a000fsa88dA01", ""),
     ("in_sprite_a000fsa91dA01", ""),
     ("in_sprite_a000fsa92dA01", ""),
     ("in_sprite_a000fsa94dA01", ""),
     ("in_sprite_a000fsa95dA01", ""),
     ("in_sprite_a000fsa96dA01", ""),
     ("in_sprite_a000fsa97dA01", ""),
     ("in_sprite_a000fsa98dA01", ""),
     ("in_sprite_a000fsa01nB02", ""),
     ("in_sprite_a000fsa02nB02", ""),
     ("in_sprite_a000fsa04nB02", ""),
     ("in_sprite_a000fsa05nB02", ""),
     ("in_sprite_a000fsa06nB02", ""),
     ("in_sprite_a000fsa07nB02", ""),
     ("in_sprite_a000fsa08nB02", ""),
     ("in_sprite_a000fsa10nB02", ""),
     ("in_sprite_a000fsa12nB02", ""),
     ("in_sprite_a000fsa13nB02", ""),
     ("in_sprite_a000fsa14nB03", ""),
     ("in_sprite_a000fsa16nB02", ""),
     ("in_sprite_a000fsa17nB02", ""),
     ("in_sprite_a000fsa18nB03", ""),
     ("in_sprite_a000fsa19nB03", ""),
     ("in_sprite_a000fsa20nB03", ""),
     ("in_sprite_a000fsa21eB03", ""),
     ("in_sprite_a000fsa24nB03", ""),
     ("in_sprite_a000fsa25nB03", ""),
     ("in_sprite_a000fsa26nB03", ""),
     ("in_sprite_a000fsa27nB10", ""),
     ("in_sprite_a000fsa28nB10", ""),
     ("in_sprite_a000fsa29eB10", ""),
     ("in_sprite_a000fsa31nB10", ""),
     ("in_sprite_a000fsa39nB09", ""),
     ("in_sprite_a000fsa40nB09", ""),
     ("in_sprite_a000fsa41nB09", ""),
     ("in_sprite_a000fsa42nB09", ""),
     ("in_sprite_a000fsa44nB09", ""),
     ("in_sprite_a000fsa45nB10", ""),
     ("in_sprite_a000fsa46nB10", ""),
     ("in_sprite_a000fsb47nB11", ""),
     ("in_sprite_a000fsb48nB11", ""),
     ("in_sprite_a000fsb49nB11", ""),
     ("in_sprite_a000fsc01nB05", ""),
     ("in_sprite_a000fsc02nB04", ""),
     ("in_sprite_a000fsc03cB04", ""),
     ("in_sprite_a000fsc04nB04", ""),
     ("in_sprite_a000fsc05nB04", ""),
     ("in_sprite_a000fsc06nB04", ""),
     ("in_sprite_a000fsc08nB05", ""),
     ("in_sprite_a000fsc09nB05", ""),
     ("in_sprite_a000fsc10nB05", ""),
     ("in_sprite_a000fsc12nB05", ""),
     ("in_sprite_a000fsc13cB05", ""),
     ("in_sprite_a000fsc14cB05", ""),
     ("in_sprite_a000fsc16nB05", ""),
     ("in_sprite_a000fsc17nB05", ""),
     ("in_sprite_a000fsc18nB05", ""),
     ("in_sprite_a000fsc19nB05", ""),
     ("in_sprite_a000fsc20nB05", ""),
     ("in_sprite_a000fsc21eB05", ""),
     ("in_sprite_a000fsc22nB05", ""),
     ("in_sprite_a000fsc24nB05", ""),
     ("in_sprite_a000fsc25nB05", ""),
     ("in_sprite_a000fsc26nB05", ""),
     ("in_sprite_a000fsc50nB11", ""),

    // Additional blue a000 sprites:
//     ("in_sprite_a000fsa04nG02", ""), // Deleted 2006-03-20
//     ("in_sprite_a000fsa04nT02", ""), // excited woman is way too saturated to use
//     ("in_sprite_a000fsa05nT02", ""), // Deleted 2006-03-20
     ("in_sprite_a000fsa21eT03", ""),
     ("in_sprite_a000fsa29eG10", ""),
//     ("in_sprite_a000fsa29eT10", ""), // bald guy is way too saturated to use
//     ("in_sprite_a000fsa29eX10", ""), // Deleted 2006-03-20
//     ("in_sprite_a000fsa29eY10", ""), // Deleted 2006-03-20
//     ("in_sprite_a000fsa42nT09", ""), // Deleted 2006-03-20
     ("in_sprite_a000fsb48nT11", ""),
//     ("in_sprite_a000fsc01nT05", ""), // Deleted 2006-03-20
//     ("in_sprite_a000fsc04nT04", ""), // Deleted 2006-03-20
//     ("in_sprite_a000fsc05nT04", ""), // Deleted 2006-03-20
     ("in_sprite_a000fsc10nT05", ""),
//     ("in_sprite_a000fsc21eT05", ""), // Deleted 2006-03-20

    // in_sprite_k000fs:
     ("in_sprite_k000fsa32nB10", ""),
     ("in_sprite_k000fsa33nB10", ""),
     ("in_sprite_k000fsa34nB10", ""),
     ("in_sprite_k000fsa35nB10", ""),
     ("in_sprite_k000fsa36nB10", ""),
     ("in_sprite_k000fsa47nB07", ""),
     ("in_sprite_k000fsd02nB06", ""),
     ("in_sprite_k000fsd05nB06", ""),
     ("in_sprite_k000fsd06nB06", ""),
     ("in_sprite_k000fsd07nB06", ""),
     ("in_sprite_k000fsd08nB06", ""),
     ("in_sprite_k000fsd09nB06", ""),
     ("in_sprite_k000fsd10nB06", ""),
     ("in_sprite_k000fsd12nB06", ""),
     ("in_sprite_k000fsd14nB06", ""),
     ("in_sprite_k000fsd15nB06", ""),
     ("in_sprite_k000fsd17nB06", ""),
     ("in_sprite_k000fsd18nB06", ""),
     ("in_sprite_k000fsd19nB06", ""),
     ("in_sprite_k000fsd20nB06", ""),
//    ("in_sprite_k000fsd21nB06", ""), // this guy has too dark shirt and pants under a light jacket
     ("in_sprite_k000fsd22nB06", ""),
     ("in_sprite_k000fsd23nB06", ""),
     ("in_sprite_k000fsd24nB07", ""),
     ("in_sprite_k000fsd26nB07", ""),

    // Darker-blue sprites added 03/13/2006
     ("in_sprite_a000fsa21eQ03", ""),
     ("in_sprite_a000fsb48nQ11", ""),
     ("in_sprite_a000fsc05nQ04", ""),
     ("in_sprite_a000fsa05nQ02", ""),
     ("in_sprite_a000fsc04nQ04", ""),
     ("in_sprite_a000fsa42nQ09", ""),
     ("in_sprite_a000fsc10nQ05", ""),
     ("in_sprite_a000fsc21eQ05", ""),
     ("in_sprite_a000fsc01nQ05", ""),
     ("in_sprite_a000fsa29eQ10", ""),

 
    // in_sprite_a180fs:
     ("in_sprite_a180fsa54nA12", ""),
     ("in_sprite_a180fsa55nB12", ""),
     ("in_sprite_a180fsa56nB12", ""),
     ("in_sprite_a180fsa57nB12", ""),
     ("in_sprite_a180fsa58nB12", ""),
     ("in_sprite_a180fsa60nA12", ""),
     ("in_sprite_a180fsa61nA12", ""),
     ("in_sprite_a180fsa62nA12", ""),
//     ("in_sprite_a180fsa64nB12", "")    // <- HAS MISSING FRAMES

    // additional blue a180 sprites:
     ("in_sprite_a180fsa54nX12", ""),
     ("in_sprite_a180fsa54nY12", ""),
     ("in_sprite_a180fsa62nX12", ""),

    // in_sprite_a160fs:
     ("in_sprite_a160fsa27nB10", ""),
     ("in_sprite_a160fsa28nB10", ""),
     ("in_sprite_a160fsa30nB10", ""),
     ("in_sprite_a160fsa31nB10", ""),
     ("in_sprite_a160fsa38eB10", ""),
     ("in_sprite_a160fsa39nB09", ""),
     ("in_sprite_a160fsa40nB09", ""),
     ("in_sprite_a160fsa42nB09", ""),
     ("in_sprite_a160fsa44nB09", ""),
     ("in_sprite_a160fsa45nB10", ""),
     ("in_sprite_a160fsa46nB10", ""),
     ("in_sprite_a160fsb39nB10", ""),
     ("in_sprite_a160fsb41nB10", ""),
     ("in_sprite_a160fsb46nB11", ""),
     ("in_sprite_a160fsb47nB11", ""),
     ("in_sprite_a160fsb50nB11", ""),
     ("in_sprite_a160fsc38nB11", ""),
     ("in_sprite_a160fsc41nB11", ""),
     ("in_sprite_a160fsc43nB11", ""),
     ("in_sprite_a160fsc45nB11", ""),
     ("in_sprite_a160fsc47cB11", ""),
     ("in_sprite_a160fsc48nB11", ""),
     ("in_sprite_a160fsc49nB11", ""),
     ("in_sprite_a160fsc50nB11", ""),
     ("in_sprite_a160fsd38nB11", ""),
     ("in_sprite_a160fsd41nB11", ""),
     
     // in_sprite_a030cw -- added 2006-03-17
     ("in_sprite_a030cwa01nA01", ""),
     ("in_sprite_a030cwa02nA01", ""),
     ("in_sprite_a030cwa03nA01", ""),
     ("in_sprite_a030cwa04nA01", ""),
     ("in_sprite_a030cwa07nA01", ""),
     ("in_sprite_a030cwa08nA01", ""),
     ("in_sprite_a030cwa09nA01", ""),
     ("in_sprite_a030cwa10nA01", ""),
     ("in_sprite_a030cwa11eA01", ""),
     ("in_sprite_a030cwa12nA01", ""),
     ("in_sprite_a030cwa13nA01", ""),
     ("in_sprite_a030cwa14nA01", ""),
     ("in_sprite_a030cwa16nA01", ""),
     ("in_sprite_a030cwa17nA02", ""),
     ("in_sprite_a030cwa18nA01", ""),
     ("in_sprite_a030cwa19nA01", ""),
     ("in_sprite_a030cwa20nA02", ""),
     ("in_sprite_a030cwa21eA02", ""),
     ("in_sprite_a030cwa22nA02", ""),
     ("in_sprite_a030cwa23nA02", ""),
     ("in_sprite_a030cwa24eA02", ""),
     ("in_sprite_a030cwa25nA02", ""),
     ("in_sprite_a030cwa26nA02", ""),
     ("in_sprite_a030cwb31eA09", ""),
     ("in_sprite_a030cwb32nA09", ""),
     ("in_sprite_a030cwb33nA09", ""),
     ("in_sprite_a030cwb34nA09", ""),
     ("in_sprite_a030cwb35nA09", ""),
     ("in_sprite_a030cwb36nA09", ""),
     ("in_sprite_a030cwb37nA09", ""),
     ("in_sprite_a030cwb38nA09", ""),


     // in_sprite_k030cs -- added 2006-03-17
     ("in_sprite_k030csa33nA10", ""),
     ("in_sprite_k030csa34nA10", ""),
     ("in_sprite_k030csa35nA10", ""),
     ("in_sprite_k030csa36nA10", ""),
     ("in_sprite_k030csa37nA10", ""),
     ("in_sprite_k030csb01nA03", ""),
     ("in_sprite_k030csb18nA04", ""),
     ("in_sprite_k030csb19nA04", ""),
     ("in_sprite_k030csb20nA04", ""),
     ("in_sprite_k030csb21nA04", ""),
     ("in_sprite_k030csb22nA04", ""),
     ("in_sprite_k030csb24nA04", ""),
     ("in_sprite_k030csb26cA04", ""),
     ("in_sprite_k030csd01nA06", ""),
     ("in_sprite_k030csd02nA06", ""),
     ("in_sprite_k030csd04nA06", ""),
     ("in_sprite_k030csd05nA06", ""),
     ("in_sprite_k030csd06nA06", ""),
     ("in_sprite_k030csd07nA06", ""),
     ("in_sprite_k030csd08nA06", ""),
     ("in_sprite_k030csd09nA06", ""),
     ("in_sprite_k030csd10nA06", ""),
     ("in_sprite_k030csd11nA06", ""),
     ("in_sprite_k030csd12nA06", ""),
     ("in_sprite_k030csd13nA06", ""),
     ("in_sprite_k030csd14nA06", ""),
     ("in_sprite_k030csd15nA06", ""),
     ("in_sprite_k030csd16nA06", ""),
     ("in_sprite_k030csd17nA06", ""),
     ("in_sprite_k030csd18nA06", ""),
     ("in_sprite_k030csd19nA06", ""),
     ("in_sprite_k030csd20nA06", ""),
//     ("in_sprite_k030csd21nA06", ""), // this guy has too dark shirt and pantsunder a light jacket
     ("in_sprite_k030csd22nA06", ""),
     ("in_sprite_k030csd23nA06", ""),
     ("in_sprite_k030csd24nA07", ""),
     ("in_sprite_k030csd26nA06", ""),
     ("in_sprite_k030cse61nA07", ""),
     ("in_sprite_k030cse64nA07", ""),

    // Added 2006-03-20
     ("in_sprite_a030csd03nA05", ""),
     ("in_sprite_a030csa09nA02", ""),
     ("in_sprite_a030csa11nA03", ""),
     ("in_sprite_a030csa15nA03", ""),
     ("in_sprite_a030csa22nA03", ""),
     ("in_sprite_a030csa23nA03", ""),
     ("in_sprite_a030csa30nA10", ""),
     ("in_sprite_a030csa32nA10", ""),
     ("in_sprite_a030csa43nA09", ""),
     ("in_sprite_a030csa52nA03", ""),
     ("in_sprite_a030csb50nA11", ""),
     ("in_sprite_a030csc07nA04", ""),
     ("in_sprite_a030csc11eA05", ""),
     ("in_sprite_a030csc23eA05", ""),
     ("in_sprite_a030csc38nA11", ""),
     ("in_sprite_a030csc48nA11", ""),
     ("in_sprite_a030csc49nA11", ""),
     ("in_sprite_a030fsc15nA05", "")


};

//----------------------------------------------------------------------
//
//  Build an exhaustive list of "sprites". Sprites, here, are
//  defined as ranges with a single sprite "shoot". Since there
//  are multiple actions in a single shoot, each action becomes
//  a seperate "sprite".
//

\: buildSpriteDB (SpriteDB; SpriteFiles names, ActionTypes actions)
{
    SpriteDB sprites = SpriteDB();

    for_each (name; names)
    {
        let matches = regex.smatch("in_sprite_(.)([0-9]+)(.)(..)([0-9]+)(.)(.*)", 
                                   name._0),
            lit     = matches[1] == 'k',
            angle   = float(matches[2]),
            facing  = matches[3],
            clothes = matches[4],
            actor   = matches[5],
            team    = matches[6],
            tape    = matches[7];

        for_each (action; actions)
        {
            Sprite sprite = Sprite(name._0, name._1, 1.0, 
                                   angle, lit, action, 1.0,
                                   int(actor));
            sprites.push_back(sprite);
        } 
    }

    shuffleDB(sprites, 2);
    sprites;
}

//----------------------------------------------------------------------
//
//  The parameter file is a list of name value pairs with the exception
//  that certain names are allowed to have an additional argument. In
//  most cases the values are probabilities. However, this file also
//  handles compiling general parameters to the instancer function.
//
//  A lot of the logic in here is for providing useful error messages
//  when the file is not syntactically correct.
//

\: parseParameterFile (Parameters; string filename)
{
    if( ! io.path.exists( filename ) )
    {
        print("ERROR: File not found: \"%s\"\n" % (filename));
        throw exception();
    }
    let file     = io.ifstream(filename),
        all      = padNL(io.read_all(file)),
        lines    = string.split(all, "\n\r"),
        params   = Parameters(1, 100,       // start and end frames
                              "cache",      // cacheOutput
                              "people",     // obj
                              Point(0.0),   // target
                              0,            // seed
                              true,         // allowFlip
                              false,        // alwaysFlip
                              "",           // outfile
                              "",           // particles
                              Vec2(60,72),  // cardSize
                              0,            // offsetStart
                              0,            // offsetEnd
                              Vec2(0,0),    // staticOffset
                              "",           // suffix
                              "",           // geomfile
                              "",           // geomobject
                              "",           // densityMapFile
                              "",           // litMapFile
                              0.0, 0.0, 0.0, 0.0, // c4 params
                              0,            // emit seed
                              Vec3(0,0,0),  // cardoffset
                              "random",     // selection method
                              WeightFilters(), 
                              ActionFilters(),
                              StaticFilters(),
                              Overrides()
                              ),
        filters   = params.weightFilters,
        afilters  = params.actionFilters,
        sfilters  = params.staticFilters,
        overrides = params.overrides,
        rec       = regex("^[ \t]*#.*"), // comment line
        reb       = regex("^[ \t]*$"),
        re        = regex("^[ \t]*([a-zA-Z0-9_]+)" + // command
                          "[ \t]*([^ \t]+)?" + // arg
                          "[ \t]*=[ \t]*" + 
                          "([^#]*)(#.*)?"); // value
    file.close();

    for (int i=0; i < lines.size(); i++)
    {
        let line = lines[i];

        if (rec.match(line) || reb.match(line)) continue;

        \: error (void; string text)
        {
            print("ERROR: \"%s\", line %d: %s\n" % (filename, i+1, text));
            throw exception();
        }

        try
        {
            let tokens   = re.smatch(line),
                name     = tokens[1],
                arg      = tokens[2],
                value    = tokens[3];

            //print("line %d tokens = %s\n" % (i, string(tokens)));

            if (name == "angle")
            {
                if (arg eq nil)
                {
                    error("angle expected an angle in degrees: angle DEG = WEIGHT");
                }
                else
                {
                    filters.push_back((regex("in_sprite_.%03d.*" % int(arg)), 
                                       float(value)));
                }
                continue;
            }
            if (name == "actor")
            {
                if (arg eq nil)
                {
                    error("actor expected an actor number: actor NUM = WEIGHT");
                }
                else
                {
                    filters.push_back((regex("in_sprite_.......%02d.*" % int(arg)),
                                       float(value)));
                }
                continue;
            }
            if (name == "action")
            {
                if (arg eq nil)
                {
                    error("action expected an action name: action NAME = WEIGHT");
                }
                else
                {
                    afilters.push_back((regex("^%s$" % arg, regex.Extended), float(value)));
                }
                continue;
            }
            if (name == "static")
            {
                if (arg eq nil)
                {
                    error("static expected an action name: static NAME = WEIGHT");
                }
                else
                {
                    sfilters.push_back((regex("^%s$" % arg, regex.Extended), float(value)));
                }
                continue;
            }
            if (name == "file")
            {
                if (arg eq nil)
                {
                    error("file expected a sprite file name or regex: file NAME = WEIGHT");
                }
                else
                {
                    filters.push_back((regex(arg), float(value)));
                }
                continue;
            }
            if (name == "force")
            {
                if (arg eq nil)
                {
                    error("force expected a sprite name: force ID = SPRITE");
                }
                else
                {
                    overrides.push_back((int(arg), nospaces(value)));
                }
                continue;
            }
            if (arg eq nil)
            {
                if (name == "seed")   
                {
                    params.seed = int(value);
                    continue;
                }
                if (name == "outfile")
                {
                    params.outfile = nospaces(value);
                }
                if (name == "particles")
                {
                    params.particles = nospaces(value);
                }
                if (name == "select")
                {
                    params.selectMethod = nospaces(value);
                }
                else if (name == "suffix")
                {
                    params.suffix = nospaces(value);
                }
                else if (name == "geomfile")
                {
                    params.geomfile = nospaces(value);
                }
                else if (name == "geomobject")
                {
                    params.geomobject = nospaces(value);
                }
                else if (name == "densityMap")
                {
                    params.densityMapFile = nospaces(value);
                }
                else if (name == "litMap")
                {
                    params.litMapFile = nospaces(value);
                }
                else if (name == "seperation")
                {
                    params.seperation = float(value);
                }
                else if (name == "emitSeed")
                {
                    params.emitSeed = int(value);
                }
                else if (name == "margin")
                {
                    params.margin = float(value);
                }
                else if (name == "stagger")
                {
                    params.stagger = float(value);
                }
                else if (name == "skipProbability")
                {
                    params.skipProbability = float(value);
                }
                else if (name == "cardSize")
                {
                    let m = regex.smatch("([0-9.]+)[ \t]+([0-9.]+)", value);
                    params.cardSize = Vec2(float(m[1]), float(m[2]));
                }
                else if (name == "offset")
                {
                    let m = regex.smatch("([0-9-]+)[ \t]+([0-9-]+)", value);
                    params.offsetStart = int(m[1]);
                    params.offsetEnd = int(m[2]);
                }
                else if (name == "staticOffset")
                {
                    let m = regex.smatch("([0-9-]+)[ \t]+([0-9-]+)", value);
                    params.staticOffset = Vec2( int(m[1]), int(m[2]) );
                }
                else if (name == "flip")   
                {
                    let v = nospaces(value);
                    params.allowFlip = 
                        v == "yes" ||
                        v == "true" ||
                        v == "1";
                }
                else if (name == "alwaysFlip")   
                {
                    let v = nospaces(value);
                    params.alwaysFlip = 
                        v == "yes" ||
                        v == "true" ||
                        v == "1";
                }
                else if (name == "target") 
                {
                    params.target = parsePoint(value);
                }
                else if (name == "cardOffset") 
                {
                    params.cardOffset = parsePoint(value);
                }
                else if (name == "startFrame")
                {
                    params.startFrame = int(value);
                }
                else if (name == "endFrame")
                {
                    params.endFrame = int(value);
                }
                else if (name == "cacheOutput")
                {
                    params.cacheOutput = nospaces(value);
                }
                else
                {
                    for_each (filter; named_filters)
                    {
                        if (filter._0 == name)
                        {
                            filters.push_back((filter._1, float(value)));
                        }
                    }
                }
            }
            else
            {
                error("unexpected argument (%s) to %s" % (arg, name));
            }
        }
        catch (exception exc)
        {
            error("caught %s" % string(exc));
            throw;
        }
        catch (...)
        {
            error("parse error");
            throw;
        }
    }

    params;
}

//----------------------------------------------------------------------
//
//  Apply filters in the Parameters object.
//  Weight filters (which match names) and action filters which 
//  match action names. The original DB is munged in the process.
//

\: applyFilters (SpriteDB; Parameters params, SpriteDB db, WeightOp op)
{
    let ndb      = SpriteDB(),
        wfilters = params.weightFilters,
        afilters = params.actionFilters,
        sfilters = params.staticFilters,
        accum    = 0.0;

    //
    //  Apply weight filters (across name)
    //


    for_each (s; db)
    {
        for_each (filter; wfilters)
        {
            if (filter._0.match(s.name))
            {
                s.weight = op(s.weight, filter._1);
            }
        }

        for_each (filter; afilters)
        {
            if (filter._0.match(s.action.name))
            {
                s.weight = op(s.weight, filter._1);
            }
        }
        for_each (filter; sfilters)
        {
            if (filter._0.match(s.action.name))
            {
                s.staticWeight = op(s.staticWeight, filter._1);
            }
        }

        if (s.weight > 0.0)
        {
            accum += s.weight;
            ndb.push_back(s);
        }
    }

    if( ndb.size() == 0 )
    {
        print("\nNo sprites matched filter criteria!\n");
        throw exception();
    }

    for_each (s; ndb) s.weight /= accum;

    //sortSprites(ndb);
    ndb;
}

//----------------------------------------------------------------------
//
//  Shuffle sprite instances
//

\: shuffle (void; Instances instances)
{
    for (int i=0, n=instances.size(); i < n; i++)
    {
        let r = irandom(i, n-1),
            t = instances[r];
            
        instances[r] = instances[i];
        instances[i] = t;
    }
}

\: neighborShuffle (void; Instances instances)
{
    for (int i=0, n=instances.size()-1; i < n; i++)
    {
        let a = instances[i],
            b = instances[i+1];

        if (a.sprite.name == b.sprite.name)
        {
            let t = instances[-1];
            instances[-1] = instances[i];
            instances[i] = t;
        }
    }
}

\: shuffleDB (void; SpriteDB db, int num)
{
    repeat (num)
    {
        for (int i=0, n=db.size(); i < n; i++)
        {
            let r = irandom(i, n-1),
                t = db[r];
            
            db[r] = db[i];
            db[i] = t;
        }
    }
}

//----------------------------------------------------------------------
//
//  Choose random sprite
//


\: chooseSprite (Sprite; SpriteDB db)
{
    // Hack to get around unresolved func bug
    (float;float) F = math_util.random;

    let r = F(1.0),
        a = 0.0;

    Sprite rval = nil;

    for_each (s; db)
    {
        let w = s.weight;

        if (a + w > r) 
        { 
            rval = s; 
            break; 
        }

        a += w;
    }

    rval;
}

//----------------------------------------------------------------------
//
//  Generate a sprite instance
//

\: chooseOneRandom (Instance; Parameters params)
{
    let s = chooseSprite(params.sprites),
        f = irandom(params.offsetStart, params.offsetEnd) + s.action.start;

    Instance(s, math.max(f,1) );
}

// for backwards compat
generateInstance := chooseOneRandom;

//----------------------------------------------------------------------
//
//  Generate N instances using the linear shuffle method
//

\: chooseLinearShuffle (Instances; Parameters params, int n)
{
    chooseLinearShuffleWithSeed(params, n, nil);
}

\: chooseLinearShuffleWithSeed (Instances; Parameters params, int n, (void;int) seedFunc)
{
    Instances instances;
    math_util.seed(params.seed + params.emitSeed);

    class: SpriteIterator
    {
        SpriteDB    db;
        Sprite      sprite;
        float       end;   
        float       current;
    }

    \: inc (SpriteIterator; SpriteIterator i)
    {
        i.current = (i.current+1) % i.db.size();
        i.sprite = i.db[i.current];
        i.end += i.sprite.weight;
        i;
    }

    //
    //  Choose instances by linearly walking through
    //  sprite db.
    //

    print("INFO: selecting sprites by linear walk\n");

    let si    = SpriteIterator(params.sprites, params.sprites.front(), 0, 0),
        start = params.offsetStart,
        end   = params.offsetEnd;

    for (int i=0; i < n; i++)
    {
        float d = 1.0 / n * float(i);
        while (d > si.end) inc(si);

        if (!(seedFunc eq nil)) seedFunc(i);

        let s = si.sprite,
            f = math.max(irandom(start, end) + s.action.start, 1);

        instances.push_back(Instance(s, f));
    }

    shuffle(instances);
    neighborShuffle(instances);
    instances;
}

//----------------------------------------------------------------------
//
//  Choose using the random selection method
//


\: chooseRandomSelectionWithSeed (Instances; 
                                  Parameters params, 
                                  int n, 
                                  (void;int) seedFunc)
{
    Instances instances;
    math_util.seed(params.seed + params.emitSeed);

    for (int i=0; i < n; i++) 
    {
        seedFunc(i);
        instances.push_back(chooseOneRandom(params));
    }

    shuffle(instances);
    neighborShuffle(instances);
    instances;
}

\: chooseRandomSelection (Instances; Parameters params, int n)
{
    // BUG: why can't I overload chooseRandomSelection instead of
    // using a unique name (chooseRandomSelectionWithSeed). 
    chooseRandomSelectionWithSeed(params, n, \: (void;int i) {;});
}

//----------------------------------------------------------------------
//
//  Selection method database
//

global SelectionMethods methods = { ("random", chooseRandomSelection),
                                    ("linear", chooseLinearShuffle) };

\: smethod (SelectionFunc; string name)
{
    for_each (m; methods)
    {
        if (m._0 == name) return m._1;
    }

    print("WARNING: no selection methods called \"%s\", using random\n" % name);
    methods.front()._1;
}

//----------------------------------------------------------------------
//
//  
//

\: buildParameters (Parameters; SpriteDB sprites, string filename)
{
    let params       = parseParameterFile(filename),
        finalSprites = applyFilters(params, sprites, (*));

    params.sprites = finalSprites;
    params;
}

//----------------------------------------------------------------------
//
//  Dummy backend for processShot()
//

\: debugBackend (void; Parameters params) 
{
    print("\nPARAMETERS\n%s\n" % string(params));

    print("\nweight filters\n");
    for_each (f; params.weightFilters)
    {
        print("%s\n" % string(f));
    }
    
    print("\naction filters\n");
    for_each (f; params.actionFilters)
    {
        print("%s\n" % string(f));
    }
    
    print("\nfinal sprites\n");
    for_each (s; params.sprites)
    {
        print("%s\n" % string(s));
    }
}

\: nullBackend (void; Parameters params) { ; }


//
// call this curried with n = how many you want
//

\: chooseNSprites (Instances; Parameters params, int n, (void;int) F = nil)
{
    print("INFO: method = %s\n" % params.selectMethod);

    if (params.selectMethod == "linear")
    {
        if (F eq nil)
        {
            return chooseLinearShuffle(params, n);
        }
        else
        {
            return chooseLinearShuffleWithSeed(params, n, nil);
        }
    }
    else
    {
        if (F eq nil)
        {
            return chooseRandomSelection(params, n);
        }
        else
        {
            return chooseRandomSelectionWithSeed(params, n, F);
        }
    }
}

\: chooseNBackend (void; Parameters params, int n)
{
    for_each (s; chooseNSprites(params, n)) 
    {
        print("%s\n" % string(s));
    }
}

\: silentChooseNBackend (void; Parameters params, int n)
{
    chooseNSprites(params, n);
}

//----------------------------------------------------------------------
//
//  Main entry point
//

\: processShot (void; string paramfile, Backend backend)
{
    math_util.seed(132);

    let sprites = buildSpriteDB(sprite_files, action_types),
        params  = buildParameters(sprites, paramfile);

    backend(params);
}

} // module: crowd

//
//  Call it with the debugging backend that just chooses N
//  sprites and spits them out.
//

crowd.processShot("testdata/crowd_L13.parameters", 
                       crowd.silentChooseNBackend(,10000));
