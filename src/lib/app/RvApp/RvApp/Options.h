//
//  Copyright (c) 2013 Tweak Software
//  All rights reserved.
//  
//  SPDX-License-Identifier: Apache-2.0
//  
//
#ifndef __RV__Options__h__
#define __RV__Options__h__
#include <vector>
#include <string>
#include <map>
#include <deque>
#include <limits>

namespace Rv {

//
//  This is for use with the libarg parser. The MACRO below is
//  provided as an arg_parse function argument. This should contain
//  logic and arguments common to all implementations. This struct
//  should probably not be used anywhere in RvApp (eventhough it lives
//  here). 
//

struct Options
{
    typedef std::vector<std::string> StringVector;
    typedef std::map<std::string,std::string> StringMap;
    typedef std::vector<std::string> Files;
    typedef std::deque<std::pair<std::string,std::string> > Params;

    struct SourceArgs
    {
        SourceArgs() : audioOffset(0), rangeOffset(0), volume(1), fps(0),
                       pixelAspect(0), stereoRelativeOffset(0), stereoRightOffset(0),
                       hascrop(false), hasuncrop(false), singleSource (false),
                       noMovieAudio(false)
            {
                cutIn  = (std::numeric_limits<int>::max)();
                cutOut = (std::numeric_limits<int>::max)();
                rangeStart = (std::numeric_limits<int>::max)();
            }

        StringVector files;
        float        audioOffset;
        int          rangeOffset;
        int          rangeStart;
        float        volume;
        float        fps;
        bool         hascrop;
        bool         singleSource;
        bool         noMovieAudio;
        float        crop[4];
        bool         hasuncrop;
        float        uncrop[4];
        float        pixelAspect;
        float        stereoRelativeOffset;
        float        stereoRightOffset;
        int          cutIn;
        int          cutOut;
        std::string  fcdl;
        std::string  lcdl;
        std::string  flut;
        std::string  pclut;
        std::string  llut;
        std::string  cmap;
        std::string  selectName;
        std::string  selectType;
        std::string  mediaRepName;
        std::string  mediaRepSource;
        Params       inparams;
    };

    typedef std::vector<SourceArgs> SourceArgsVector;

    struct SendExternalEvent
    {
        std::string name;
        std::string content;
    };

    typedef std::vector<SendExternalEvent> SendExternalEventVector;

    Options();

    //
    //  Use this, don't make your own Options object
    //

    static Options& sharedOptions();

    //
    //  Call this prior to calling arg_parse with argv
    //

    static void manglePerSourceArgs(char** argv, int argc);

    //
    //  Call this before you start your app up. It may act on some of the
    //  option state and exit (for example show formats). It will also
    //  convert the inputFiles into SourceArgsVector
    //

    bool initializeAfterParsing(Options* prefs = 0);

    //
    //  Set IO env variable arguments from the options
    //

    void exportIOEnvVars() const;

    //
    //  Find seperate frame ranges
    //

    void integrateFrameRangeStrings(std::vector<std::string>&);

    //
    //  Parse an input file list into SourceArgs. Files is a bit deceiving,
    //  it can actullay contain groupings "[" and "]" as well as per-source
    //  args premanged from manglePerSourceArgs() like +fps, etc. The
    //  parsed per-source args are in the structs along with the actual
    //  final file list with arguments stripped.
    //

    SourceArgsVector parseSourceArgs(const StringVector&);

    //
    //  Output as ASCII data. Each line is preceeded by "#" character
    //

    void output(std::ostream&);

    //
    //  The state
    //

    float        fps;
    float        defaultfps;
    int          useCache;
    int          useLCache;
    int          useNoCache;
    int          showFormats;
    int          fullscreen;
    int          usecli;
    int          nomb;
    char*        bgpattern;
    int          nukeSequence;
    int          noRanges;
    char*        initscript;
    float        scale;
    char*        windowType;
    char*        sessionType;
    char*        resampleMethod;
    char*        licarg;
    float        maxvram;
    float        totalcram;
    float        maxcram;
    float        maxlram;
    float        maxbwait;
    float        lookback;
    int          cacheOutsideRegion;
    int          noPBO;
    int          swapScanlines;
    int          prefetch;
    int          useAppleClientStorage;
    int          useThreadedUpload;
    int          readerThreads;
    int          workItemThreads;
    int          apple;
    int          allowYUV;
    int          sync;
    int          lqt_decoder;
    int          exrcpus;
    int          networkPort;
    int          dispRedBits;
    int          dispGreenBits;
    int          dispBlueBits;
    int          dispAlphaBits;
    char*        networkHost;
    std::string  networkHostBuf;
    char*        networkTag;
    std::string  networkTagBuf;
    int          networkPerm;
    char*        networkUser;
    int          exrRGBA;
    int          exrIOMethod;
    int          exrIOSize;
    int          exrMaxAsync;
    int          exrInherit;
    int          exrNoOneChannel;
    int          exrReadWindowIsDisplayWindow;
    int          exrReadWindow;
    int          exrPlanar3Chan;
    int          exrStripAlpha;
    int          exrConvertYRYBY;
    int          jpegRGBA;
    int          jpegIOMethod;
    int          jpegIOSize;
    int          jpegMaxAsync;
    int          nofloat;
    int          maxbits;
    int          audioOff;
    int          audioNoLock;
    int          audioNice;
    int          audioScrub;
    int          audioPreRoll;
    char*        audioModule;
    char*        audioDevice;
    float        audioRate;
    int          audioPrecision;
    int          audioLayout;
    float        audioMinCache;
    float        audioMaxCache;
    float        audioGlobalOffset;
    float        audioDeviceLatency;
    float        volume;
    char*        eval;
    char*        pyeval;
    float        gamma;
    float        brightness;
    int          showVersion;
    int          play;
    int          playMode;
    int          loopMode;
    int          showCMS;
    int          cinalt;
    int          cinchroma;
    int          dpxchroma;
    int          cinIOMethod;
    int          cinIOSize;
    int          cinMaxAsync;
    int          dpxIOMethod;
    int          dpxIOSize;
    int          dpxMaxAsync;
    int          tgaIOMethod;
    int          tgaIOSize;
    int          tgaMaxAsync;
    int          tiffIOMethod;
    int          tiffIOSize;
    int          tiffMaxAsync;
    int          aframesize;
    int          acachesize;
    char*        cms;
    char*        cmsDisplay;
    char*        cmsSimulation;
    char*        stereoMode;
    char*        compMode;
    char*        layoutMode;
    int          stereoSwapEyes;
    int          over;
    int          replace;
    int          topmost;
    int          layer;
    int          tile;
    int          diff;
    int          wipes;
    int          noSequence;
    int          inferSequence;
    int          autoRetime;
    StringVector inputFiles;
    char*        fileCDL;
    char*        lookCDL;
    char*        fileLUT;
    char*        preCacheLUT;
    char*        lookLUT;
    char*        dispLUT;
    char*        cmapString;
    char*        selectType;
    char*        selectName;
    char*        mediaRepName;
    char*        mediaRepSource;
    int          noPrefs;
    int          resetPrefs;
    int          useCrashReporter;
    int          encodeURL;
    int          bakeURL;
    int          sRGB;
    int          rec709;
    char*        cinPixel;
    char*        dpxPixel;
    char*        mistPixel;
    int          newGLSLlutInterp;
    int          vsync;
    char*        qtstyle;
    char*        qtcss;
    int          qtdesktop;
    int          xl;
    int          network;
    char*        connectHost;
    int          connectPort;
    int          networkOnStartup;
    int          stylusAsMouse;
    int          scrubEnable;
    int          clickToPlayEnable;
    int          urlsReuseSession;
    int          noPackages;
    int          delaySessionLoading;
    int          progressiveSourceLoading;
    StringVector debugKeyWords;
    StringMap    muFlags;
    char*        view;
    int          imageFilter;
    
    float        pixelAspect;
    int          rangeOffset;
    int          rangeStart;
    int          cutInPoint;
    int          cutOutPoint;
    int          noMovieAudio;
    float        stereoOffset;
    float        rightEyeStereoOffset;
    float        audioOffset;
    float        cropX0;
    float        cropY0;
    float        cropX1;
    float        cropY1;
    float        uncropW;
    float        uncropH;
    float        uncropX;
    float        uncropY;
    Params       inparams;

    int          x;
    int          y;
    int          width;
    int          height;
    int          screen;
    int          startupResize;
    int          noBorders;
    int          displayPriority;
    int          audioPriority;
    char*        schedulePolicy;

    char*        presentDevice;
    char*        presentFormat;
    char*        presentData;
    int          present;
    int          presentAudio;
    int          fontSize1;
    int          fontSize2;

    SendExternalEventVector  sendEvents;
};

int collectParameters(Options::Params&, const Options::Files&, int);
int parseInParams(int, char**);
int parseDebugKeyWords(int, char**);
int parseMuFlags(int, char**);
int parseSendEvents(int, char**);
const char* getDebugCategories();

} // Rv

//
//  This should be one of the arguments to arg_parse
//

#define RV_ARG_EXAMPLES                                                 \
         "", "Usage: RV movie and image sequence viewer",               \
         "", "",                                                        \
         "", "  One File:                   rv foo.jpg",                \
         "", "  This Directory:             rv .",                      \
         "", "  Other Directory:            rv /path/to/dir",           \
         "", "  Image Sequence w/Audio:     rv [ in.#.tif in.wav ]",    \
         "", "  Stereo w/Audio:             rv [ left.#.tif right.#.tif in.wav ]", \
         "", "  Stereo Movies:              rv [ left.mov right.mov ]", \
         "", "  Stereo Movie (from rvio):   rv stereo.mov",             \
         "", "  Cuts Sequenced:             rv cut1.mov cut2.#.exr cut3.mov", \
         "", "  Stereo Cuts Sequenced:      rv [ l1.mov r1.mov ] [ l2.mov r2.mov ]", \
         "", "  Forced Anamorphic:          rv [ -pa 2.0 fullaperture.#.dpx ]", \
         "", "  Compare:                    rv -wipe a.exr b.exr",      \
         "", "  Difference:                 rv -diff a.exr b.exr",      \
         "", "  Slap Comp Over:             rv -over a.exr b.exr",      \
         "", "  Tile Images:                rv -tile *.jpg",            \
         "", "  Cache + Play Movie:         rv -l -play foo.mov",       \
         "", "  Cache Images to Examine:    rv -c big.#.exr",           \
         "", "  Fullscreen on 2nd monitor:  rv -fullscreen -screen 1",  \
         "", "  Select Source View:         rv [ in.exr -select view right ] ", \
         "", "  Select Source Layer:        rv [ in.exr -select layer light1.diffuse ]       (single-view source)", \
         "", "  Select Source Layer:        rv [ in.exr -select layer left,light1.diffuse ]  (multi-view source)", \
         "", "  Select Source Channel:      rv [ in.exr -select channel R ]                  (single-view, single-layer source)", \
         "", "  Select Source Channel:      rv [ in.exr -select channel left,Diffuse,R ]     (multi-view, multi-layer source)" \

#define RV_ARG_SEQUENCE_HELP                                            \
         "", "Image Sequence Numbering",                                \
         "", "",                                                        \
         "", "  Frames 1 to 100 no padding:     image.1-100@.jpg",      \
         "", "  Frames 1 to 100 padding 4:      image.1-100#.jpg -or- image.1-100@@@@.jpg", \
         "", "  Frames 1 to 100 padding 5:      image.1-100@@@@@.jpg",  \
         "", "  Frames -100 to -200 padding 4:  image.-100--200#jpg",   \
         "", "  printf style padding 4:         image.%%04d.jpg",  \
         "", "  printf style w/range:           image.%%04d.jpg 1-100",  \
         "", "  printf no padding w/range:      image.%%d.jpg 1-100",    \
         "", "  Complicated no pad 1 to 100:    image_887f1-100@_982.tif", \
         "", "  Stereo pair (left,right):       image.#.%%V.tif", \
         "", "  Stereo pair (L,R):              image.#.%%v.tif", \
         "", "  All Frames, padding 4:          image.#.jpg", \
         "", "  All Frames in Sequence:         image.*.jpg", \
         "", "  All Frames in Directory:        /path/to/directory",    \
         "", "  All Frames in current dir:      ."

#define RV_ARG_SOURCE_OPTIONS(opt) \
        "", "Per-source arguments (inside [ and ] restricts to that source only)", \
         "", "",                                                        \
        "-pa %f", &opt.pixelAspect, "Per-source pixel aspect ratio",       \
        "-ro %d", &opt.rangeOffset, "Per-source range offset",             \
        "-rs %d", &opt.rangeStart, "Per-source range start",             \
        "-fps %f", &opt.fps, "Per-source or global fps", \
        "-ao %f", &opt.audioOffset, "Per-source audio offset in seconds", \
        "-so %f", &opt.stereoOffset, "Per-source stereo relative eye offset", \
        "-rso %f", &opt.rightEyeStereoOffset, "Per-source stereo right eye offset", \
        "-volume %f", &opt.volume, "Per-source or global audio volume (default=%.2g)", opt.volume, \
        "-fcdl %S", &opt.fileCDL, "Per-source file CDL",        \
        "-lcdl %S", &opt.lookCDL, "Per-source look CDL",        \
        "-flut %S", &opt.fileLUT, "Per-source file LUT",        \
        "-llut %S", &opt.lookLUT, "Per-source look LUT",        \
        "-pclut %S", &opt.preCacheLUT, "Per-source pre-cache software LUT",        \
        "-cmap %S", &opt.cmapString, "Per-source channel mapping (channel names, separated by ',')",        \
        "-select %S %S", &opt.selectType, &opt.selectName, "Per-source view/layer/channel selection", \
        "-mediaRepName %S", &opt.mediaRepName, "Per-source media representation name", \
        "-mediaRepSource %S", &opt.mediaRepSource, "The source for which to add the source media representation", \
        "-crop %d %d %d %d", &opt.cropX0, &opt.cropY0, &opt.cropX1, &opt.cropY1, "Per-source crop (xmin, ymin, xmax, ymax)", \
        "-uncrop %d %d %d %d", &opt.uncropW, &opt.uncropH, &opt.uncropX, &opt.uncropY, "Per-source uncrop (width, height, xoffset, yoffset)", \
        "-in %d", &opt.cutInPoint, "Per-source cut-in frame", \
        "-out %d", &opt.cutOutPoint, "Per-source cut-out frame", \
        "-noMovieAudio", ARG_FLAG(&opt.noMovieAudio), "Disable source movie's baked-in audio", \
        "-inparams", ARG_SUBR(&Rv::parseInParams), "Source specific input parameters"

#define RV_ARG_PARSE_OPTIONS(opt)                                       \
         "-c", ARG_FLAG(&opt.useCache), "Use region frame cache",       \
         "-l", ARG_FLAG(&opt.useLCache), "Use look-ahead cache",        \
         "-nc", ARG_FLAG(&opt.useNoCache), "Use no caching",        \
         "-s %f", &opt.scale, "Image scale reduction",                  \
         "-ns", ARG_FLAG(&opt.nukeSequence), "Nuke style sequence notation (deprecated and ignored -- no longer needed)", \
         "-noRanges", ARG_FLAG(&opt.noRanges), "No separate frame ranges (i.e. 1-10 will be considered a file)", \
         "-sessionType %S", &opt.sessionType, "Session type (sequence, stack) (deprecated, use -view)", \
         "-stereo %S", &opt.stereoMode, "Stereo mode (hardware, checker, scanline, anaglyph, lumanaglyph, left, right, pair, mirror, hsqueezed, vsqueezed)", \
         "-stereoSwap %d", &opt.stereoSwapEyes, "Swap left and right eyes stereo display (0 == no, 1 == yes, default=%d)", opts.stereoSwapEyes, \
         "-vsync %d", &opt.vsync, "Video Sync (1 = on, 0 = off, default = %d)", opts.vsync,  \
         "-comp %S", &opt.compMode, "Composite mode (over, add, difference, replace, topmost)", \
         "-layout %S", &opt.layoutMode, "Layout mode (packed, row, column, manual)", \
         "-over", ARG_FLAG(&opt.over), "Same as -comp over -view defaultStack", \
         "-diff", ARG_FLAG(&opt.diff), "Same as -comp difference -view defaultStack", \
         "-replace", ARG_FLAG(&opt.replace), "Same as -comp replace -view defaultStack", \
         "-topmost", ARG_FLAG(&opt.topmost), "Same as -comp topmost -view defaultStack", \
         "-layer", ARG_FLAG(&opt.layer), "Same as -comp topmost -view defaultStack, with strict frame ranges", \
         "-tile", ARG_FLAG(&opt.tile), "Same as -layout packed -view defaultLayout", \
         "-wipe", ARG_FLAG(&opt.wipes), "Same as -over with wipes enabled",   \
         "-view %S", &opt.view, "Start with a particular view", \
         "-noSequence", ARG_FLAG(&opt.noSequence), "Don't contract files into sequences", \
         "-inferSequence", ARG_FLAG(&opt.inferSequence), "Infer sequences from one file", \
         "-autoRetime %d", &opt.autoRetime, "Automatically retime conflicting media fps in sequences and stacks (1 = on, 0 = off, default = %d)", opts.autoRetime, \
         "-rthreads %d", &opt.readerThreads, "Number of reader threads (default=%d)", opts.readerThreads, \
         "-workItemThreads %d", &opt.workItemThreads, "Number of work item threads (default=%d)", opts.workItemThreads, \
         "-progressiveSourceLoading %d", &opt.progressiveSourceLoading, "Use asynchronous source loading, default=0 (off)", \
         "-fullscreen", ARG_FLAG(&opt.fullscreen), "Start in fullscreen mode", \
         "-present", ARG_FLAG(&opt.present), "Start in presentation mode (using presentation device)", \
         "-presentAudio %d", &opt.presentAudio, "Use presentation audio device in presentation mode (1 = on, 0 = off)", \
         "-presentDevice %S", &opt.presentDevice, "Presentation mode device", \
         "-presentVideoFormat %S", &opt.presentFormat, "Presentation mode override video format (device specific)", \
         "-presentDataFormat %S", &opt.presentData, "Presentation mode override data format (device specific)", \
         "-screen %d", &opt.screen, "Start on screen (0, 1, 2, ...)", \
         "-noBorders", ARG_FLAG(&opt.noBorders), "No window manager decorations", \
         "-geometry %d %d [%d %d]", &opt.x, &opt.y, &opt.width, &opt.height, "Start geometry X, Y, W, H", \
         "-fitMedia", ARG_FLAG(&opt.startupResize), "Fit the window to the first media shown", \
         "-init %S", &opt.initscript, "Override init script",           \
         "-nofloat", ARG_FLAG(&opt.nofloat), "Turn off floating point by default", \
         "-maxbits %d", &opt.maxbits, "Maximum default bit depth (default=%d)", opt.maxbits, \
         "-gamma %f", &opt.gamma, "Set display gamma (default=%g)", opt.gamma, \
         "-sRGB", ARG_FLAG(&opt.sRGB), "Display using linear -> sRGB conversion", \
         "-rec709", ARG_FLAG(&opt.rec709), "Display using linear -> Rec 709 conversion", \
         "-dlut %S", &opt.dispLUT, "Apply display LUT", \
         "-brightness %f", &opt.brightness, "Set display relative brightness in stops (default=%g)", opt.brightness, \
         "-resampleMethod %S", &opt.resampleMethod, "Resampling method (area, linear, cubic, nearest, default=%s)", opt.resampleMethod, \
         "-eval %S", &opt.eval, "Evaluate Mu expression at every session start", \
         "-pyeval %S", &opt.pyeval, "Evaluate Python expression at every session start", \
         "-nomb", ARG_FLAG(&opt.nomb), "Hide menu bar on start up",     \
         "-play", ARG_FLAG(&opt.play), "Play on startup",               \
         "-playMode %d", &opt.playMode, "Playback mode (0=Context dependent, 1=Play all frames, 2=Realtime, default=%d)", opt.playMode, \
         "-loopMode %d", &opt.loopMode, "Playback loop mode (0=Loop, 1=Play Once, 2=Ping-Pong, default=%d)", opt.loopMode, \
         "-cli", ARG_FLAG(&opt.usecli), "Mu command line interface",    \
         "-vram %f", &opt.maxvram, "VRAM usage limit in Mb, default = %f", opt.maxvram, \
         "-cram %f", &opt.maxcram, "Max region cache RAM usage in Gb, (%.2gGb available, default %.2gGb)", opt.totalcram, opt.maxcram, \
         "-lram %f", &opt.maxlram, "Max look-ahead cache RAM usage in Gb, (%.2gGb available, default %.2gGb)", opt.totalcram, opt.maxlram, \
         "-noPBO", ARG_FLAG(&opt.noPBO), "Prevent use of GL PBOs for pixel transfer", \
         "-prefetch", ARG_FLAG(&opt.prefetch), "Prefetch images for rendering", \
         "-useAppleClientStorage", ARG_FLAG(&opt.useAppleClientStorage), "Use APPLE_client_storage extension", \
         "-useThreadedUpload", ARG_FLAG(&opt.useThreadedUpload), "Use threading for texture uploading/downloading if possible", \
         "-bwait %f", &opt.maxbwait, "Max buffer wait time in cached seconds, default %.1f", opt.maxbwait, \
         "-lookback %f", &opt.lookback, "Percentage of the lookahead cache reserved for frames behind the playhead, default %g", opt.lookback, \
         "-yuv", ARG_FLAG(&opt.allowYUV), "Assume YUV hardware conversion", \
         "-noaudio", ARG_FLAG(&opt.audioOff), "Turn off audio",         \
         "-audiofs %d", &opt.aframesize, "Use fixed audio frame size (results are hardware dependant ... try 512)", \
         "-audioCachePacket %d", &opt.acachesize, "Audio cache packet size in samples (default=%d)", opt.acachesize, \
         "-audioMinCache %f", &opt.audioMinCache, "Audio cache min size in seconds (default=%f)", opt.audioMinCache,  \
         "-audioMaxCache %f", &opt.audioMaxCache, "Audio cache max size in seconds (default=%f)", opt.audioMaxCache,  \
         "-audioModule %S", &opt.audioModule, "Use specific audio module", \
         "-audioDevice %S", &opt.audioDevice, "Use specific audio device", \
         "-audioRate %f", &opt.audioRate, "Use specific output audio rate (default=ask hardware)", \
         "-audioPrecision %d", &opt.audioPrecision, "Use specific output audio precision (default=%d)", opt.audioPrecision,  \
         "-audioNice %d", &opt.audioNice, "Close audio device when not playing (may cause problems on some hardware) default=%d", opt.audioNice,  \
         "-audioNoLock %d", &opt.audioNoLock, "Do not use hardware audio/video syncronization (use software instead, default=%d)", opt.audioNoLock, \
         "-audioPreRoll %d", &opt.audioPreRoll, "Preroll audio on device open (Linux only; default=%d)", opt.audioPreRoll, \
         "-audioGlobalOffset %f", &opt.audioGlobalOffset, "Global audio offset in seconds", \
         "-audioDeviceLatency %f", &opt.audioDeviceLatency, "Audio device latency compensation in milliseconds", \
         "-bg %S", &opt.bgpattern, "Background pattern (default=black, white, grey18, grey50, checker, crosshatch)", \
         "-formats", ARG_FLAG(&opt.showFormats), "Show all supported image and movie formats", \
         "-apple", ARG_FLAG(&opt.apple), "Use Quicktime and NSImage libraries (on OS X)", \
         "-cinalt", ARG_FLAG(&opt.cinalt), "Use alternate Cineon/DPX readers", \
         "-exrcpus %d", &opt.exrcpus, "EXR thread count (default=%d)", opt.exrcpus, \
         "-exrRGBA", ARG_FLAG(&opt.exrRGBA), "EXR Always read as RGBA (default=false)", \
         "-exrInherit", ARG_FLAG(&opt.exrInherit), "EXR guess channel inheritance (default=false)", \
         "-exrNoOneChannel", ARG_FLAG(&opt.exrNoOneChannel), "EXR never use one channel planar images (default=false)", \
         "-exrIOMethod %d [%d]", &opt.exrIOMethod, &opt.exrIOSize, "EXR I/O Method (0=standard, 1=buffered, 2=unbuffered, 3=MemoryMap, 4=AsyncBuffered, 5=AsyncUnbuffered, default=%d) and optional chunk size (default=%d)", opts.exrIOMethod, opts.exrIOSize, \
         "-exrReadWindowIsDisplayWindow", ARG_FLAG(&opts.exrReadWindowIsDisplayWindow), "EXR read window is display window (default=false)", \
         "-exrReadWindow %d", &opt.exrReadWindow, "EXR Read Window Method (0=Data, 1=Display, 2=Union, 3=Data inside Display, default=%d)", opts.exrReadWindow, \
         "-jpegRGBA", ARG_FLAG(&opt.jpegRGBA), "Make JPEG four channel RGBA on read (default=no, use RGB or YUV)", \
         "-jpegIOMethod %d [%d]", &opt.jpegIOMethod, &opt.jpegIOSize, "JPEG I/O Method (0=standard, 1=buffered, 2=unbuffered, 3=MemoryMap, 4=AsyncBuffered, 5=AsyncUnbuffered, default=%d) and optional chunk size (default=%d)", opts.exrIOMethod, opts.exrIOSize, \
         "-cinpixel %S", &opt.cinPixel, "Cineon pixel storage (default=%s)", opt.cinPixel, \
         "-cinchroma", ARG_FLAG(&opt.cinchroma), "Use Cineon chromaticity values (for default reader only)", \
         "-cinIOMethod %d [%d]", &opt.cinIOMethod, &opt.cinIOSize, "Cineon I/O Method (0=standard, 1=buffered, 2=unbuffered, 3=MemoryMap, 4=AsyncBuffered, 5=AsyncUnbuffered, default=%d) and optional chunk size (default=%d)", opts.cinIOMethod, opts.cinIOSize, \
         "-dpxpixel %S", &opt.dpxPixel, "DPX pixel storage (default=%s)", opt.dpxPixel, \
         "-dpxchroma", ARG_FLAG(&opt.dpxchroma), "Use DPX chromaticity values (for default reader only)", \
         "-dpxIOMethod %d [%d]", &opt.dpxIOMethod, &opt.dpxIOSize, "DPX I/O Method (0=standard, 1=buffered, 2=unbuffered, 3=MemoryMap, 4=AsyncBuffered, 5=AsyncUnbuffered, default=%d) and optional chunk size (default=%d)", opts.dpxIOMethod, opts.dpxIOSize,  \
         "-tgaIOMethod %d [%d]", &opt.tgaIOMethod, &opt.tgaIOSize, "TARGA I/O Method (0=standard, 1=buffered, 2=unbuffered, 3=MemoryMap, 4=AsyncBuffered, 5=AsyncUnbuffered, default=%d) and optional chunk size (default=%d)", opts.tgaIOMethod, opts.tgaIOSize,  \
         "-tiffIOMethod %d [%d]", &opt.tiffIOMethod, &opt.tiffIOSize, "TIFF I/O Method (0=standard, 1=buffered, 2=unbuffered, 3=MemoryMap, 4=AsyncBuffered, 5=AsyncUnbuffered, default=%d) and optional chunk size (default=%d)", opts.tgaIOMethod, opts.tgaIOSize,  \
         "-lic %S", &opt.licarg, "Use specific license file", \
         "-noPrefs", ARG_FLAG(&opt.noPrefs), "Ignore preferences", \
         "-resetPrefs", ARG_FLAG(&opt.resetPrefs), "Reset preferences to default values", \
         "-qtcss %S", &opt.qtcss, "Use QT style sheet for UI", \
         "-qtstyle %S", &opt.qtstyle, "Use QT style", \
         "-qtdesktop %d", &opt.qtdesktop, "QT desktop aware, default=1 (on)", \
         "-xl", ARG_FLAG(&opt.xl), "Aggressively absorb screen space for large media", \
         "-mouse %d", &opt.stylusAsMouse, "Force tablet/stylus events to be treated as a mouse events, default=0 (off)", \
         "-network", ARG_FLAG(&opt.network), "Start networking", \
         "-networkPort %d", &opt.networkPort, "Port for networking", \
         "-networkHost %S", &opt.networkHost, "Alternate host/address for incoming connections", \
         "-networkTag %S", &opt.networkTag, "Tag to mark automatically saved port file", \
         "-networkConnect %S [%d]", &opt.connectHost, &opt.connectPort, "Start networking and connect to host at port", \
         "-networkUser %S", &opt.networkUser, "User for network", \
         "-networkPerm %d", &opt.networkPerm, "Default network connection permission (0=Ask, 1=Allow, 2=Deny, default=0)", \
         "-reuse %d", &opt.urlsReuseSession, "Try to re-use the current session for incoming URLs (1 = reuse session, 0 = new session, default = %d)", opt.urlsReuseSession, \
         "-nopackages", ARG_FLAG(&opt.noPackages), "Don't load any packages at startup (for debugging)", \
         "-encodeURL", ARG_FLAG(&opt.encodeURL), "Encode the command line as an rvlink URL, print, and exit", \
         "-bakeURL", ARG_FLAG(&opt.bakeURL), "Fully bake the command line as an rvlink URL, print, and exit", \
         "-sendEvent", ARG_SUBR(&Rv::parseSendEvents), "Send external events e.g. -sendEvent 'name' 'content'",               \
         "-flags", ARG_SUBR(&Rv::parseMuFlags), "Arbitrary flags (flag, or 'name=value') for use in Mu code",\
         "-debug", ARG_SUBR(&Rv::parseDebugKeyWords), Rv::getDebugCategories(), \
         "-version", ARG_FLAG(&opt.showVersion), "Show RV version number"

#endif // __RV__Options__h__
