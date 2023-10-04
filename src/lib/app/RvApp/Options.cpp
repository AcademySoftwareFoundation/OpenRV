//
//  Copyright (c) 2013 Tweak Software
//  All rights reserved.
//  
//  SPDX-License-Identifier: Apache-2.0
//  
//
#include <RvApp/Options.h>
#include <RvApp/RvSession.h>
#include <RvApp/FormatIPNode.h>
#include <IPBaseNodes/FileSourceIPNode.h>
#include <IPBaseNodes/LayoutGroupIPNode.h>
#include <IPBaseNodes/SequenceGroupIPNode.h>
#include <IPBaseNodes/StackGroupIPNode.h>
#include <IPBaseNodes/StackIPNode.h>
#include <IPCore/Application.h>
#include <IPCore/AudioRenderer.h>
#include <IPCore/DisplayStereoIPNode.h>
#include <IPCore/Exception.h>
#include <IPCore/FBCache.h>
#include <IPCore/ImageRenderer.h>
#include <IPCore/LUTIPNode.h>
#include <IPCore/NodeManager.h>
#include <IPCore/ShaderProgram.h>
#include <IPCore/SoundTrackIPNode.h>
#include <ImfHeader.h>
#include <MuTwkApp/MuInterface.h>
#include <TwkApp/Application.h>
#include <TwkApp/Bundle.h>
#include <TwkMovie/MovieIO.h>
#include <TwkMovie/ResamplingMovie.h>
#include <TwkUtil/EnvVar.h>
#include <TwkUtil/FrameUtils.h>
#include <TwkUtil/Macros.h>
#include <TwkUtil/Notifier.h>
#include <TwkUtil/PathConform.h>
#include <TwkUtil/TwkRegEx.h>
#include <TwkUtil/SystemInfo.h>
#include <TwkUtil/Timer.h>
#include <TwkAudio/Audio.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stl_ext/string_algo.h>

extern void TwkMovie_GenericIO_setDebug(bool);
extern void TwkFB_GenericIO_setDebug(bool);

namespace
{
  const int DEFAULT_AUDIO_CACHE_SIZE_IN_SECONDS_PER_GB_OF_USABLE_MEMORY = 4;
}

namespace Rv {
using namespace TwkUtil;
using namespace std;
using namespace IPCore;
using namespace boost;

static Options* globalOptions=0;

static ENVVAR_INT( evProgressiveSourceLoading, "RV_PROGRESSIVE_SOURCE_LOADING", 0 );

// If you add a new debugging switch, please update getDebugCategories for the help menu too.
void debugSwitches(const std::string& name)
{
    if (name == "events") TwkApp::Document::debugEvents();
    else if (name == "threads") stl_ext::thread_group::debug_all(true);
    else if (name == "gpu") ImageRenderer::reportGL(true);
    else if (name == "audio") { AudioRenderer::setDebug(true); TwkMovie::ResamplingMovie::setDebug(true); }
    else if (name == "audioverbose") { AudioRenderer::setDebugVerbose(true); AudioRenderer::setDebug(true); TwkMovie::ResamplingMovie::setDebug(true); }
    else if (name == "dumpaudio") { AudioRenderer::setDumpAudio(true); TwkMovie::ResamplingMovie::setDumpAudio(true); }
    else if (name == "shaders") { Shader::setDebugging(Shader::AllDebugInfo); }
    else if (name == "shadercode") { Shader::setDebugging(Shader::ShaderCodeDebugInfo); }
    else if (name == "profile") debugProfile = true;
    else if (name == "playback") debugPlayback = true;
    else if (name == "playbackverbose") debugPlaybackVerbose = debugPlayback =true;
    else if (name == "cache") TwkFB::Cache::debug() = true;
    else if (name == "mu") TwkApp::setDebugging(true);
    else if (name == "muc") TwkApp::setDebugMUC(true);
    else if (name == "compile") TwkApp::setCompileOnDemand(true);
    else if (name == "dtree") IPGraph::setDebugTreeOutput(true);
    else if (name == "passes") ImageRenderer::setPassDebug(true);
    else if (name == "imagefbo") ImageRenderer::setOutputIntermediateDebug(true);
    else if (name == "nogpucache") ImageRenderer::setNoGPUCache(true);
    else if (name == "imagefbolog") ImageRenderer::setIntermediateLogging(true);
    else if (name == "nodes") NodeManager::setDebug(true);
    else if (name == "plugins") 
    {
        TwkMovie_GenericIO_setDebug(true);
        TwkFB_GenericIO_setDebug(true);
    }
    else
    {
        Notifier::MessageId mid = Notifier::registerMessage(name);
        Notifier::debugMessage(mid, true);
        
        cout << "INFO: debugging message \""
             << name 
             << "\"" << endl;
    }
}

const char* 
getDebugCategories()
{
   return "Debuging categories: "
    "events, "
    "threads, "
    "gpu, "
    "audio, "
    "audioverbose, "
    "dumpaudio, "
    "shaders, "
    "shadercode, "
    "profile, "
    "playback, "
    "playbackverbose, "
    "cache, "
    "mu, "
    "muc, "
    "compile, "
    "dtree, "
    "passes, "
    "imagefbo, "
    "nogpucache, "
    "imagefbolog, "
    "nodes, "
    "plugins";
}

int
collectParams(Options::Params& p, const Options::Files& inputFiles, int index)
{
    int count = -1;
    for (int i = index; i < inputFiles.size(); i++, count++)
    {
        string v = inputFiles[i];
        if (v.substr(0,1) == "+" || v.substr(0,1) == "]") return count;

        string::size_type pos = v.find("=");
        if (pos == string::npos) return count;
        p.push_back(pair<string,string>(v.substr(0, pos), v.substr(pos+1, string::npos)));
    }
    return count;
}

int
parseInParams(int argc, char *argv[])
{
    Options& opts = Options::sharedOptions();

    Options::Files infiles;
    for (int i=0; i<argc; i++) infiles.push_back(argv[i]);

    collectParams(opts.inparams, infiles, 0);
    return 0;
}

int
parseDebugKeyWords(int argc, char *argv[])
{
    Options& opts = Options::sharedOptions();

    for (int i=0; i<argc; i++)
    {
        opts.debugKeyWords.push_back(argv[i]);
    }

    return 0;
}

int
parseMuFlags(int argc, char *argv[])
{
    Options& opts = Options::sharedOptions();

    for (int i=0; i<argc; i++)
    {
        vector<string> parts;
        stl_ext::tokenize (parts, argv[i], "=");

        if (1 == parts.size()) //  simple flag
        {
            opts.muFlags[parts[0]] = "true";
        }
        else
        if (2 == parts.size()) //  name=value flag
        {
            opts.muFlags[parts[0]] = parts[1];
        }
        else cerr << "ERROR: mal-formed flag: " << argv[i] << endl;
    }

    return 0;
}

int
parseSendEvents(int argc, char *argv[])
{
    Options& opts = Options::sharedOptions();

    for (int i = 0; i < argc; i += 2)
    {
        Options::SendExternalEvent e;
        e.name = argv[i];

        if (i+1 < argc) e.content = argv[i+1];

        opts.sendEvents.push_back(e);
        // cerr << "parseSendEvents name=" << e.name << " content=" << e.content << endl;
    }

    return 0;
}


Options& 
Options::sharedOptions()
{
    if (!globalOptions) globalOptions = new Options();
    return *globalOptions;
}

Options::Options()
{
    static const int numLogicalCPUs =
            static_cast<int>( TwkUtil::SystemInfo::numCPUs() );
    static const size_t usableMemory = TwkUtil::SystemInfo::usableMemory();

    displayPriority      = -1;
    audioPriority        = -1;
    schedulePolicy       = 0;
    fps                  = 0.0;
    defaultfps           = 24.0; // no command line for this one
    useCache             = 0;
    useLCache            = 1; // Use 'Look-Ahead Cache' by default
    useNoCache           = 0;
    showFormats          = 0;
    fullscreen           = 0;
    noBorders            = 0;
    screen               = -1;
    x                    = -1;
    y                    = -1;
    width                = -1;
    height               = -1;
    startupResize        = 0;
    usecli               = 0;
    nomb                 = 0;
    bgpattern            = 0;
    initscript           = 0;
    scale                = 1.0;
    sessionType          = (char*)"";
    resampleMethod       = (char*)"area";
    licarg               = 0;
    maxvram              = 64.0;      // should be determined at start up
    totalcram            = 0.2 * (double(usableMemory)/1024.0/1024.0/1024.0); 
    maxcram              = 1.0;       // 1 GB 
    maxlram              = 1.0;       // 1 GB 
    maxbwait             = 5.0;       // seconds
    lookback             = 25.0;      // percent
    readerThreads        = ( numLogicalCPUs > 4 ) ? std::min( numLogicalCPUs / 4, 4 ) : 1;
    workItemThreads      = ( numLogicalCPUs > 4 ) ? std::min( numLogicalCPUs / 4, 4 ) : 1;
    cacheOutsideRegion   = 0;
    apple                = 0;
    allowYUV             = 0;
    sync                 = 0;
    lqt_decoder          = 0;
    exrcpus              = 0; 
    dispRedBits          = 8;
    dispGreenBits        = 8;
    dispBlueBits         = 8;
    dispAlphaBits        = 8;
    networkPort          = 0; 
    networkHost          = NULL; 
    networkTag           = NULL; 
    networkUser          = NULL;
    networkPerm          = -1; 
    exrRGBA              = 0;
    exrInherit           = 1;
    exrNoOneChannel      = 0;
    exrReadWindowIsDisplayWindow = 0;
    exrReadWindow        = 3;
    exrPlanar3Chan       = 0; /* Packed buffers are more optimal. Based on performance tuning */
    exrStripAlpha        = 0;
    exrConvertYRYBY      = 0;
    jpegRGBA             = 0;
    nofloat              = 0;
    noPBO                = 0;
    swapScanlines        = 0;
    prefetch             = 1;
    useThreadedUpload    = 0;
    maxbits              = 32;
    eval                 = 0;
    pyeval               = 0;
    gamma                = 1.0;
    brightness           = 0.0;
    showVersion          = 0;
    play                 = 0;
    playMode             = 0;
    loopMode             = 0;
    showCMS              = 0;
    cinalt               = 0;
    cinchroma            = 0;
    dpxchroma            = 0;
    cms                  = 0;
    cmsDisplay           = 0;
    cmsSimulation        = 0;
    stereoMode           = 0;
    stereoSwapEyes       = 0;
    compMode             = 0;
    layoutMode           = 0;
    over                 = 0;
    layer                = 0;
    replace              = 0;
    topmost              = 0;
    tile                 = 0;
    wipes                = 0;
    diff                 = 0;
    noSequence           = 0;
    inferSequence        = 0;
    autoRetime           = 1;
    aframesize           = 0;
    acachesize           = 2048;
    nukeSequence         = 0;
    noRanges             = 0;
#ifdef PLATFORM_LINUX
    audioNice            = 0;
    audioNoLock          = 1;
    audioPreRoll         = 1;
#else
    audioNice            = 0;
    audioNoLock          = 0;
    audioPreRoll         = 0;
#endif
    audioScrub           = 0;
    audioModule          = 0;
    audioDevice          = 0;
    audioRate            = TWEAK_AUDIO_DEFAULT_SAMPLE_RATE;
    audioPrecision       = 16;
    audioLayout          = (int) TwkAudio::Stereo_2;
    audioMaxCache        = ROUND( usableMemory *
                            DEFAULT_AUDIO_CACHE_SIZE_IN_SECONDS_PER_GB_OF_USABLE_MEMORY /
                            ( 1024ULL * 1024ULL * 1024ULL ) );
    audioMinCache        = 0.9 * audioMaxCache;
    audioGlobalOffset    = 0;
    audioDeviceLatency   = 0;
    volume               = 1.0;
    fileCDL              = NULL;
    lookCDL              = NULL;
    fileLUT              = NULL;
    lookLUT              = NULL;
    dispLUT              = NULL;
    preCacheLUT          = NULL;
    cmapString           = NULL;
    selectType           = NULL;
    selectName           = NULL;
    mediaRepName         = NULL;
    mediaRepSource       = NULL;
    noPrefs              = 0;
    resetPrefs           = 0;
    useCrashReporter     = 0;
    encodeURL            = 0;
    bakeURL              = 0;
    sRGB                 = 0;
    rec709               = 0;
    cinPixel             = (char*)"RGB8_PLANAR";
    dpxPixel             = (char*)"RGB8_PLANAR";
    mistPixel            = (char*)"RGB8_PLANAR";

    newGLSLlutInterp = 0;
#if defined(PLATFORM_LINUX)
    vsync          = 0;
#else
    vsync          = 1;
#endif
    xl             = 0;

#ifdef PLATFORM_LINUX
    stylusAsMouse  = 1;
#else
    stylusAsMouse  = 0;
#endif

    scrubEnable  = 1;
    clickToPlayEnable = 1;

    qtstyle        = 0;
    qtcss          = 0;
    qtdesktop      = 1;

    view           = (char*)"";

#ifdef WIN32
    exrIOMethod  = 3; /* Memory Map. Based on performance tuning */
    cinIOMethod  = 1;
    dpxIOMethod  = 1;
    tgaIOMethod  = 1;
    tiffIOMethod = 1;
    jpegIOMethod = 1;
#else
    exrIOMethod  = 3; /* Memory Map. Based on performance tuning */
    cinIOMethod  = 2;
    dpxIOMethod  = 2;
    tgaIOMethod  = 2;
    tiffIOMethod = 2;
    jpegIOMethod = 0;
#endif

    exrIOSize    = 61440;
    dpxIOSize    = 61440;
    cinIOSize    = 61440;
    jpegIOSize   = 61440;
    tgaIOSize    = 61440;
    tiffIOSize   = 61440;

    exrMaxAsync  = 16;
    dpxMaxAsync  = 16;
    tgaMaxAsync  = 16;
    tiffMaxAsync = 16;
    cinMaxAsync  = 16;
    jpegMaxAsync = 16;

    pixelAspect  = 0.0;
    stereoOffset = 0.0;
    rightEyeStereoOffset = 0.0;
    audioOffset  = 0.0;
    audioOff     = 0;
    rangeOffset  = 0;
    rangeStart   = numeric_limits<int>::max();
    cutInPoint   = numeric_limits<int>::max();
    cutOutPoint  = numeric_limits<int>::max();
    noMovieAudio = 0;
    cropX0       = 0;
    cropX1       = 0;
    cropY0       = 0;
    cropY1       = 0;
    uncropW      = 0;
    uncropH      = 0;
    uncropX      = 0;
    uncropY      = 0;

    urlsReuseSession = 1;
    noPackages   = 0;
    delaySessionLoading = 1;
    progressiveSourceLoading = evProgressiveSourceLoading.getValue();

    network      = 0;
    connectHost  = NULL;
    connectPort  = 0;
    networkOnStartup = 0;

    windowType = 0;

    present = false;
    presentAudio = -1;
    presentDevice = (char*)"";
    presentFormat = (char*)"";
    presentData = (char*)"";

#ifdef PLATFORM_DARWIN
    fontSize1 = 13;
    fontSize2 = 10;
    useAppleClientStorage = 0;
#else
    fontSize1 = 10;
    fontSize2 = 8;
    useAppleClientStorage = 0;
#endif

    imageFilter = GL_LINEAR;
}

Options::SourceArgsVector
Options::parseSourceArgs(const Files& inputFiles)
{
    SourceArgsVector sources;
    bool inSource = false;
    bool needFreshSource = true;
    RegEx sequenceSpec("[#@%]");

    for (int i=0; i < inputFiles.size(); i++)
    {
        const string& arg = inputFiles[i];

        if (arg == "[")
        {
            if (inSource)
            {
                TWK_THROW_STREAM(ReadFailedExc, "Cannot nest source groups");
            }

            inSource = true;
            sources.resize(sources.size() + 1);
            sources.back().singleSource = true;

            SourceArgs& a = sources.back();

            //
            //  Copy over default per-source values
            //

            a.audioOffset          = audioOffset;
            a.rangeOffset          = rangeOffset;
            a.rangeStart           = rangeStart;
            a.noMovieAudio         = noMovieAudio;
            a.stereoRelativeOffset = stereoOffset;
            a.stereoRightOffset    = rightEyeStereoOffset;
            a.crop[0]              = cropX0;
            a.crop[1]              = cropY0;
            a.crop[2]              = cropX1;
            a.crop[3]              = cropY1;
            a.uncrop[0]            = uncropW;
            a.uncrop[1]            = uncropH;
            a.uncrop[2]            = uncropX;
            a.uncrop[3]            = uncropY;
            a.fcdl                 = fileCDL ? fileCDL : "";
            a.lcdl                 = lookCDL ? lookCDL : "";
            a.flut                 = fileLUT ? fileLUT : "";
            a.llut                 = lookLUT ? lookLUT : "";
            a.pclut                = preCacheLUT ? preCacheLUT : "";
            a.cmap                 = cmapString ? cmapString : "";
            a.selectType           = selectType ? selectType : "";
            a.selectName           = selectName ? selectName : "";
            a.mediaRepName         = mediaRepName ? mediaRepName : "";
            a.mediaRepSource       = mediaRepSource ? mediaRepSource : "";
            a.cutIn                = cutInPoint;
            a.cutOut               = cutOutPoint;
            a.inparams             = inparams;
        }
        else if (arg == "]")
        {
            inSource = false;
            needFreshSource = true;
        }
        else if (inSource && arg.size() && arg[0] == '+' && (i+1 < inputFiles.size()))
        {
            SourceArgs& a = sources.back();

            //
            //  First check for per-source args that take no arguement
            //
            if (arg == "+noMovieAudio" || arg == "+nma") a.noMovieAudio = true;
            //
            //  Then check for per-source args that take at least
            //  one arguement
            //
            else if (i+1 < inputFiles.size())
            {
                i++;
                const char* v = inputFiles[i].c_str();

                if (arg == "+fps") a.fps = atof(v);
                else if (arg == "+volume") a.volume = atof(v);
                else if (arg == "+audioOffset" || arg == "+ao") a.audioOffset = atof(v);
                else if (arg == "+rangeOffset" || arg == "+ro") a.rangeOffset = atoi(v);
                else if (arg == "+rangeStart"  || arg == "+rs") a.rangeStart  = atoi(v);
                else if (arg == "+pixelAspect" || arg == "+pa") a.pixelAspect = atof(v);
                else if (arg == "+stereoOffset" || arg == "+so") a.stereoRelativeOffset = atof(v);
                else if (arg == "+rightEyeStereoOffset" || arg == "+rso") a.stereoRightOffset = atof(v);
                else if (arg == "+fcdl") a.fcdl = v;
                else if (arg == "+lcdl") a.lcdl = v;
                else if (arg == "+flut") a.flut = v;
                else if (arg == "+pclut") a.pclut = v;
                else if (arg == "+llut") a.llut = v;
                else if (arg == "+cmap") a.cmap = v;
                else if (arg == "+select") { a.selectType = v; a.selectName = inputFiles[i+1].c_str(); i++; }
                else if (arg == "+mrn" || arg == "+mediaRepName") a.mediaRepName = v;
                else if (arg == "+mrs" || arg == "+mediaRepSource") a.mediaRepSource = v;
                else if (arg == "+in") a.cutIn = atoi(v);
                else if (arg == "+out") a.cutOut = atoi(v);
                else if (arg == "+inparams") i += Rv::collectParams(a.inparams, inputFiles, i);
                else if (i+4 < inputFiles.size())
                {
                    if (arg == "+crop")
                    {
                        a.crop[0] = atoi(v);
                        a.crop[1] = atoi(inputFiles[i+1].c_str());
                        a.crop[2] = atoi(inputFiles[i+2].c_str());
                        a.crop[3] = atoi(inputFiles[i+3].c_str());
                        a.hascrop = true;
                        i+=3;
                    }
                    else if (arg == "+uncrop")
                    {
                        a.uncrop[0] = atoi(v);
                        a.uncrop[1] = atoi(inputFiles[i+1].c_str());
                        a.uncrop[2] = atoi(inputFiles[i+2].c_str());
                        a.uncrop[3] = atoi(inputFiles[i+3].c_str());

                        a.hasuncrop = true;
                        i+=3;
                    }
                }
                else
                {
                    TWK_THROW_STREAM(ReadFailedExc, "Unknown flag '" + arg + "' in input");
                }
            }
        }
        else if (!inSource && (needFreshSource || TwkUtil::Match(sequenceSpec, arg)))
        {
            sources.resize(sources.size() + 1);
            sources.back().files.push_back(arg);
            needFreshSource = false;
        }
        else
        {
            sources.back().files.push_back(arg);
        }
    }

    if (inSource)
    {
        TWK_THROW_STREAM(ReadFailedExc, "Unmatched '[' in input");
    }

    //
    //  Convert frame range style to shake style
    //

    for (size_t i = 0; i < sources.size(); i++)
    {
        integrateFrameRangeStrings(sources[i].files);
    }

    return sources;
}

void
Options::integrateFrameRangeStrings(vector<string>& inputFiles)
{
    vector<string> newInputFiles;

    for (size_t i = 0; i < inputFiles.size(); i++)
    {
        if (isFrameRange(inputFiles[i]) && i != 0)
        {
            if (!isFrameRange(inputFiles[i-1]))
            {
                if (!newInputFiles.empty()) newInputFiles.pop_back();
                newInputFiles.push_back(integrateFrameRange(inputFiles[i-1], inputFiles[i]));
            }
            else
            {
                cout << "WARNING: ignoring \"" << inputFiles[i]
                     << "\" (input " << i 
                     << ") because it looks like a second frame range"
                     << endl;
            }
        }
        else
        {
            newInputFiles.push_back(inputFiles[i]);
        }
    }

    inputFiles = newInputFiles;
}

static string 
strFromChar (const char* cp) 
{
    return (cp) ? string(cp) : string();
}

static void
addIfMissing (ostringstream& stream, const string& userString, const char* name, const int value)
{
    if (userString.find(name) == string::npos) stream << " " << name << " " << value;
}

static void
addIfMissing (ostringstream& stream, const string& userString, const char* name, const char* value)
{
    if (userString.find(name) == string::npos) stream << " " << name << " " << value;
}

static void
addIfMissing (ostringstream& stream, const string& userString, const char* name, const float value)
{
    if (userString.find(name) == string::npos) stream << " " << name << " " << value;
}

void 
Options::exportIOEnvVars() const
{
    //
    //  NOTE: MovieR3D can take MOVIER3D_ARGS env variable, but since
    //  there currently are no command line options for RED we don't
    //  need to do anything here. Just let the variable go through
    //  unchanged.
    //
    //  What's the point of all this env var fiddling? The env var
    //  holds arguments to not-yet-existing reader/writer objects
    //  (plugins). When they're created they parse the env var args to
    //  get their defaults. After that (e.g. prefs) can set attributes
    //  on the live objects directly. This also allows us to provide
    //  additional arguments to the readers/writers without having to
    //  modify the support code directly.
    //

    ostringstream exr;
    ostringstream cin;
    ostringstream dpx;
    ostringstream jpeg;
    ostringstream tif;
    ostringstream tga;
    ostringstream mistika;
    ostringstream ffmpeg;

    TwkApp::Bundle* bundle = TwkApp::Bundle::mainBundle();

    string rvexr  = strFromChar (getenv("RV_IOEXR_ARGS"));
    string rvcin  = strFromChar (getenv("RV_IOCIN_ARGS"));
    string rvdpx  = strFromChar (getenv("RV_IODPX_ARGS"));
    string rvjpeg = strFromChar (getenv("RV_IOJPEG_ARGS"));
    string rvtif  = strFromChar (getenv("RV_IOTIFF_ARGS"));
    string rvtga  = strFromChar (getenv("RV_IOTARGA_ARGS"));
    string rvmist = strFromChar (getenv("RV_MOVIEMISTIKA_ARGS"));
    string rvffm  = strFromChar (getenv("RV_MOVIEFFMPEG_ARGS"));

    //  EXR plugin options

    exr << rvexr;

    addIfMissing (exr, rvexr, "--rgbaOnly",       exrRGBA);
    addIfMissing (exr, rvexr, "--inherit",        exrInherit);
    addIfMissing (exr, rvexr, "--noOneChannel",   exrNoOneChannel);
    addIfMissing (exr, rvexr, "--readWindowIsDisplayWindow", exrReadWindowIsDisplayWindow);
    addIfMissing (exr, rvexr, "--readWindow",     exrReadWindow);
    addIfMissing (exr, rvexr, "--planar3channel", exrPlanar3Chan);
    addIfMissing (exr, rvexr, "--stripAlpha",     exrStripAlpha);
    addIfMissing (exr, rvexr, "--convertYRYBY",   exrConvertYRYBY);
    addIfMissing (exr, rvexr, "--ioSize",         exrIOSize);
    addIfMissing (exr, rvexr, "--ioMethod",       exrIOMethod);
    addIfMissing (exr, rvexr, "--ioMaxAsync",     exrMaxAsync);

    bundle->setEnvVar("IOEXR_ARGS", exr.str().c_str(), true);

    //  CIN plugin options

    cin << rvcin; 

    addIfMissing (cin, rvcin, "--format",            cinPixel);
    addIfMissing (cin, rvcin, "--useChromaticities", cinchroma);
    addIfMissing (cin, rvcin, "--ioSize",            cinIOSize);
    addIfMissing (cin, rvcin, "--ioMethod",          cinIOMethod);
    addIfMissing (cin, rvcin, "--ioMaxAsync",        cinMaxAsync);

    bundle->setEnvVar("IOCIN_ARGS", cin.str().c_str(), true);

    //  DPX plugin options

    dpx << rvdpx;

    addIfMissing (dpx, rvdpx, "--format",            dpxPixel);
    addIfMissing (dpx, rvdpx, "--useChromaticities", dpxchroma);
    addIfMissing (dpx, rvdpx, "--ioSize",            dpxIOSize);
    addIfMissing (dpx, rvdpx, "--ioMethod",          dpxIOMethod);
    addIfMissing (dpx, rvdpx, "--ioMaxAsync",        dpxMaxAsync);

    bundle->setEnvVar("IODPX_ARGS", dpx.str().c_str(), true);

    //  JPEG plugin options

    jpeg << rvjpeg;

    addIfMissing (jpeg, rvjpeg, "--ioSize",     jpegIOSize);
    addIfMissing (jpeg, rvjpeg, "--ioMethod",   jpegIOMethod);
    addIfMissing (jpeg, rvjpeg, "--ioMaxAsync", jpegMaxAsync);

    bundle->setEnvVar("IOJPEG_ARGS", jpeg.str().c_str(), true);

    //  TIFF plugin options

    tif << rvtif;

    addIfMissing (tif, rvtif, "--ioSize",     tiffIOSize);
    addIfMissing (tif, rvtif, "--ioMethod",   tiffIOMethod);

    bundle->setEnvVar("IOTIFF_ARGS", tif.str().c_str(), true);

    //  TARGA plugin options

    tga << rvtga;
    addIfMissing (tga, rvtga, "--ioSize",     tgaIOSize);
    addIfMissing (tga, rvtga, "--ioMethod",   tgaIOMethod);
    addIfMissing (tga, rvtga, "--ioMaxAsync", tgaMaxAsync);

    bundle->setEnvVar("IOTARGA_ARGS", tga.str().c_str(), true);

    //  MISTIKA plugin options

    mistika << rvmist;

    addIfMissing (mistika, rvmist, "--format", mistPixel);

    bundle->setEnvVar("MOVIEMISTIKA_ARGS", mistika.str().c_str(), true); 

    //  FFMPEG movie plugin options

    ffmpeg << rvffm;

    addIfMissing (ffmpeg, rvffm, "--defaultFPS", defaultfps);
    bundle->setEnvVar("MOVIEFFMPEG_ARGS", ffmpeg.str().c_str(), true);
}

bool
Options::initializeAfterParsing(Options *prefs)
{
    //
    //  Initialize IPCore prefs from Options
    //

    IPCore::Application::OptionMap& optionMap = IPCore::Application::optionMap();
    
    optionMap["playMode"]        = int(playMode);
    optionMap["loopMode"]        = int(loopMode);
    optionMap["acachesize"]      = size_t(acachesize);
    optionMap["audioMinCache"]   = double(audioMinCache);
    optionMap["audioMaxCache"]   = double(audioMaxCache);
    optionMap["displayPriority"] = size_t(displayPriority);
    optionMap["schedulePolicy"]  = string(schedulePolicy ? schedulePolicy : "");
    optionMap["workItemThreads"] = int(workItemThreads);
    optionMap["progressiveSourceLoading"] = bool(progressiveSourceLoading!=0);

    //
    //  Initialize IPCore prefs from Env Vars.  These may be for testing or
    //  rare use cases, might "graduate" to real preferences in the future.
    //

    if (const char* c = getenv("TWK_MAX_TEXTURE_SIZE")) 
    {
        IPCore::Application::setOptionValue<int>("maxTextureSizeOverride", atoi(c));
    }

    if (getenv("TWK_DISABLE_CACHE_STATS") != NULL)
    {
        optionMap["disableCacheStats"] = true;
    }


    //
    //  Rectify conflicting prefs/args
    //

    if (prefs) 
    {
        if (useNoCache)
        {
            useCache = 0;
            useLCache = 0;
        }
        else
        if (useLCache && useCache)
        {
            if (!prefs->useLCache) useCache = 0;
            else                   useLCache = 0;
        }
        defaultfps = prefs->defaultfps;
    }

    //
    //  gamma wins over srgb/rec709, and rec709 wins over srgb.
    //

    if (gamma != 1.0)
    {
        rec709 = 0;
        sRGB   = 0;
    }
    else if (rec709 != 0) 
    {
        sRGB = 0;
    }

    //
    //  If they want to change the UI they need to supply both qtstyle
    //  and qtcss
    //

    if (qtcss && !qtstyle) qtstyle = (char*)"RV";

    //
    //  Defaults from command line first
    //

    for (size_t i = 0; i < debugKeyWords.size(); i++)
    {
        debugSwitches(debugKeyWords[i]);
    }

    if (bgpattern)
    {
        ImageRenderer::BGPattern p = ImageRenderer::Solid0;

             if (!strcmp(bgpattern, "black"))      p = ImageRenderer::Solid0;
        else if (!strcmp(bgpattern, "grey18"))     p = ImageRenderer::Solid18;
        else if (!strcmp(bgpattern, "grey50"))     p = ImageRenderer::Solid50;
        else if (!strcmp(bgpattern, "white"))      p = ImageRenderer::Solid100;
        else if (!strcmp(bgpattern, "checker"))    p = ImageRenderer::Checker;
        else if (!strcmp(bgpattern, "crosshatch")) p = ImageRenderer::CrossHatch;

        ImageRenderer::setDefaultBGPattern(p);
    }

    ImageRenderer::defaultAllowPBOs(!noPBO);
    ImageRenderer::setUseAppleClientStorage(useAppleClientStorage);
    DisplayStereoIPNode::setSwapScanlines(swapScanlines);
    SystemInfo::setMaxVRAM(size_t(double(maxvram) * 1024.0 * 1024.0));
    if (eval) RvSession::setInitEval(eval);
    if (pyeval) RvSession::setPyInitEval(pyeval);
    if (maxbits != 32) FormatIPNode::defaultBitDepth = maxbits;
    if (nofloat) FormatIPNode::defaultAllowFP = false;
    SoundTrackIPNode::defaultVolume = volume;
    FormatIPNode::defaultResampleMethod = resampleMethod;
    FileSourceIPNode::defaultOverrideFPS = fps;
    FileSourceIPNode::defaultFPS = defaultfps;
    SystemInfo::setUseableMemory(size_t(double(maxcram) * 1024.0 * 1024.0 * 1024.0));
    Session::setUsePreEval(prefetch);
    ImageRenderer::setUseThreadedUpload(useThreadedUpload);
    Session::setMaxBufferedWaitTime(maxbwait);
    Session::setCacheLookBehindFraction(lookback);
    Session::setMaxGreedyCacheSize(size_t(double(maxcram) * 1024.0 * 1024.0 * 1024.0));
    Session::setMaxBufferCacheSize(size_t(double(maxlram) * 1024.0 * 1024.0 * 1024.0));
    LUTIPNode::newGLSLlutInterp = newGLSLlutInterp;
    LayoutGroupIPNode::setDefaultAutoRetime(autoRetime);
    StackGroupIPNode::setDefaultAutoRetime(autoRetime);
    SequenceGroupIPNode::setDefaultAutoRetime(autoRetime);
    IPCore::FBCache::setCacheOutsideRegion(cacheOutsideRegion != 0);

    if (audioOff) AudioRenderer::setAudioNever(true);

    AudioRenderer::audioModule = audioModule ? audioModule : "";

    AudioRenderer::RendererParameters audioParams;
    audioParams.holdOpen        = !audioNice;
    audioParams.framesPerBuffer = aframesize;
    audioParams.device          = audioDevice ? audioDevice : "";
    audioParams.hardwareLock    = !audioNoLock;
    audioParams.rate            = audioRate;
    audioParams.preRoll         = audioPreRoll;

    // Convert from msecs to secs and reverse the sign.
    audioParams.latency         = -audioDeviceLatency / 1000.0; 

    switch (audioPrecision)
    {
      case 32: audioParams.format = TwkAudio::Float32Format; break;
      case -32: audioParams.format = TwkAudio::Int32Format; break;
      case 24: audioParams.format = TwkAudio::Int24Format; break;
      default:
      case 16: audioParams.format = TwkAudio::Int16Format; break;
      case 8: audioParams.format = TwkAudio::Int8Format; break;
    }

    audioParams.layout = TwkAudio::Layout(audioLayout);

    AudioRenderer::setDefaultParameters(audioParams);
    AudioRenderer::initialize();

    if (usecli) TwkApp::cli();

    //
    //  Expand "macro" arguments
    //

    if (over + tile + diff + wipes + layer + topmost + replace > 1)
    {
        cerr << "ERROR: you can only have one of -wipe, -over, -diff, -replace, -layer, -topmost or -tile" << endl;
        exit(-1);
    }

    if (over)
    {
        view = (char*)"defaultStack";
        compMode = (char*)"over";
    }

    if (tile)
    {
        view = (char*)"defaultLayout";
        layoutMode = (char*)"packed";
    }

    if (diff)
    {
        view = (char*)"defaultStack";
        compMode = (char*)"difference";
    }

    if (replace)
    {
        view = (char*)"defaultStack";
        compMode = (char*)"replace";
    }

    if (topmost)
    {
        view = (char*)"defaultStack";
        compMode = (char*)"topmost";
    }

    if (layer)
    {
        view = (char*)"defaultStack";
        compMode = (char*)"layer";
    }

    if (sessionType)
    {
        if (!strcmp(sessionType, "sequence")) view = (char*)"defaultSequence";
        else if (!strcmp(sessionType, "stack")) view = (char*)"defaultStack";
    }

    if (wipes)
    {
        //  Don't set view here because the viewNode may already be
        //  a stack, in which case we just want to turn on wipes.
        //  This'll happen in RvApplication.
    }

    if (stereoMode)
    {
        DisplayStereoIPNode::setDefaultType(stereoMode);
    }

    if (compMode)
    {
        StackIPNode::setDefaultCompType(compMode);
    }

    if (layoutMode)
    {
        LayoutGroupIPNode::setDefaultMode(layoutMode);
    }

    if (showFormats)
    {
        TwkMovie::GenericIO::outputFormats();
        return false;
    }

    exportIOEnvVars();

    return true;
}

void
Options::manglePerSourceArgs(char** argv, int argc)
{
    bool in = false;

    //
    //  if the argument starts with a '-' and is between a '[' and ']'
    //  argument and it is not a negative number, replace the '-' with
    //  a '+'. This is trying to convert flags of the form: -foo to
    //  +foo so that they can be parsed by the Session
    //

    for (int i = 0; i < argc; ++i)
    {
        char *s = argv[i];
        if (!strcmp(s, "[")) in = true;
        else if (!strcmp(s, "]")) in = false;
        else if (in && s[0] == '-' && atof(s) >= 0) s[0] = '+';
    }
}


void
Options::output(ostream& out)
{
    static const char* intfields[] = 
        { "acachesize", "aframesize", "allowYUV", "apple", "audioNice", "audioLayout",
          "audioNoLock", "audioScrub", "audioPreRoll", "audioOff", "audioPrecision", "autoRetime", "bakeURL", "cinIOMethod",
          "cinIOSize", "cinMaxAsync", "cinalt", "cinchroma", "connectPort", "cutInPoint",
          "cutOutPoint", "diff", "dispAlphaBits", "dispBlueBits", "dispGreenBits", "dispRedBits",
          "dpxIOMethod", "dpxIOSize", "dpxMaxAsync", "dpxchroma", "encodeURL", "exrIOMethod",
          "exrIOSize", "exrInherit", "exrMaxAsync", "exrNoOneChannel", "exrReadWindow",          
          "exrPlanar3Chan", "exrStripAlpha", "exrRGBA", "exrcpus",
          "newGLSLlutInterp", "fullscreen", "present", "presentAudio", "inferSequence", 
          "jpegIOMethod", "jpegIOSize", "jpegMaxAsync",
          "jpegRGBA", "lqt_decoder", "maxbits", "network", "networkOnStartup", "networkPerm",
          "networkPort", "noMovieAudio", "noPBO", "swapScanlines", "noPackages", "noPrefs", "noRanges", "noSequence",
          "nofloat", "nomb", "nukeSequence", "over", "play", "playMode", "prefetch", 
          "useThreadedUpload", "useAppleClientStorage", "progressiveSourceLoading",
          "qtdesktop", "rangeOffset", "rangeStart", "readerThreads", "workItemThreads", "rec709", "resetPrefs", "sRGB",
          "showCMS", "showFormats", "showVersion", "stylusAsMouse", "sync", "tgaIOMethod", "tgaIOSize",
          "tgaMaxAsync", "tiffIOMethod", "tiffIOSize", "tiffMaxAsync", "tile", "urlsReuseSession",
          "useCache", "useLCache", "usecli", "vsync", "wipes", "xl", 
          "noBorders", "screen", "x", "y", "width", "height", "startupResize", "displayPriority", "audioPriority", 
          "stereoSwap",
          NULL };

    static const char* floatfields[] = 
        { "audioMaxCache", "audioMinCache", "audioOffset", "audioRate", "brightness",
          "cropX0", "cropX1", "cropY0", "cropY1", "defaultfps", "fps", "gamma",
          "lookback", "maxbwait", "maxcram", "maxlram", "maxvram", "pixelAspect", "scale",
          "stereoOffset", "rightEyeStereoOffset", "totalcram", "uncropH", "uncropW", "uncropX",
          "uncropY", "volume", "audioGlobalOffset", "audioDeviceLatency",
      NULL
    };

    size_t intoffsets[] =
    {
        static_cast<size_t>((char*)(&this->acachesize) - (char*)this),
        static_cast<size_t>((char*)(&this->aframesize) - (char*)this),
        static_cast<size_t>((char*)(&this->allowYUV) - (char*)this),
        static_cast<size_t>((char*)(&this->apple) - (char*)this),
        static_cast<size_t>((char*)(&this->audioNice) - (char*)this),
        static_cast<size_t>((char*)(&this->audioLayout) - (char*)this),
        static_cast<size_t>((char*)(&this->audioNoLock) - (char*)this),
        static_cast<size_t>((char*)(&this->audioScrub) - (char*)this),
        static_cast<size_t>((char*)(&this->audioPreRoll) - (char*)this),
        static_cast<size_t>((char*)(&this->audioOff) - (char*)this),
        static_cast<size_t>((char*)(&this->audioPrecision) - (char*)this),
        static_cast<size_t>((char*)(&this->autoRetime) - (char*)this),
        static_cast<size_t>((char*)(&this->bakeURL) - (char*)this),
        static_cast<size_t>((char*)(&this->cinIOMethod) - (char*)this),
        static_cast<size_t>((char*)(&this->cinIOSize) - (char*)this),
        static_cast<size_t>((char*)(&this->cinMaxAsync) - (char*)this),
        static_cast<size_t>((char*)(&this->cinalt) - (char*)this),
        static_cast<size_t>((char*)(&this->cinchroma) - (char*)this),
        static_cast<size_t>((char*)(&this->connectPort) - (char*)this),
        static_cast<size_t>((char*)(&this->cutInPoint) - (char*)this),
        static_cast<size_t>((char*)(&this->cutOutPoint) - (char*)this),
        static_cast<size_t>((char*)(&this->diff) - (char*)this),
        static_cast<size_t>((char*)(&this->dispAlphaBits) - (char*)this),
        static_cast<size_t>((char*)(&this->dispBlueBits) - (char*)this),
        static_cast<size_t>((char*)(&this->dispGreenBits) - (char*)this),
        static_cast<size_t>((char*)(&this->dispRedBits) - (char*)this),
        static_cast<size_t>((char*)(&this->dpxIOMethod) - (char*)this),
        static_cast<size_t>((char*)(&this->dpxIOSize) - (char*)this),
        static_cast<size_t>((char*)(&this->dpxMaxAsync) - (char*)this),
        static_cast<size_t>((char*)(&this->dpxchroma) - (char*)this),
        static_cast<size_t>((char*)(&this->encodeURL) - (char*)this),
        static_cast<size_t>((char*)(&this->exrIOMethod) - (char*)this),
        static_cast<size_t>((char*)(&this->exrIOSize) - (char*)this),
        static_cast<size_t>((char*)(&this->exrInherit) - (char*)this),
        static_cast<size_t>((char*)(&this->exrMaxAsync) - (char*)this),
        static_cast<size_t>((char*)(&this->exrNoOneChannel) - (char*)this),
        static_cast<size_t>((char*)(&this->exrReadWindowIsDisplayWindow) - (char*)this),
        static_cast<size_t>((char*)(&this->exrReadWindow) - (char*)this),
        static_cast<size_t>((char*)(&this->exrPlanar3Chan) - (char*)this),
        static_cast<size_t>((char*)(&this->exrStripAlpha) - (char*)this),
        static_cast<size_t>((char*)(&this->exrRGBA) - (char*)this),
        static_cast<size_t>((char*)(&this->exrcpus) - (char*)this),
        static_cast<size_t>((char*)(&this->newGLSLlutInterp) - (char*)this),
        static_cast<size_t>((char*)(&this->fullscreen) - (char*)this),
        static_cast<size_t>((char*)(&this->present) - (char*)this),
        static_cast<size_t>((char*)(&this->presentAudio) - (char*)this),
        static_cast<size_t>((char*)(&this->inferSequence) - (char*)this),
        static_cast<size_t>((char*)(&this->jpegIOMethod) - (char*)this),
        static_cast<size_t>((char*)(&this->jpegIOSize) - (char*)this),
        static_cast<size_t>((char*)(&this->jpegMaxAsync) - (char*)this),
        static_cast<size_t>((char*)(&this->jpegRGBA) - (char*)this),
        static_cast<size_t>((char*)(&this->lqt_decoder) - (char*)this),
        static_cast<size_t>((char*)(&this->maxbits) - (char*)this),
        static_cast<size_t>((char*)(&this->network) - (char*)this),
        static_cast<size_t>((char*)(&this->networkOnStartup) - (char*)this),
        static_cast<size_t>((char*)(&this->networkPerm) - (char*)this),
        static_cast<size_t>((char*)(&this->networkPort) - (char*)this),
        static_cast<size_t>((char*)(&this->noMovieAudio) - (char*)this),
        static_cast<size_t>((char*)(&this->noPBO) - (char*)this),
        static_cast<size_t>((char*)(&this->swapScanlines) - (char*)this),
        static_cast<size_t>((char*)(&this->noPackages) - (char*)this),
        static_cast<size_t>((char*)(&this->noPrefs) - (char*)this),
        static_cast<size_t>((char*)(&this->noRanges) - (char*)this),
        static_cast<size_t>((char*)(&this->noSequence) - (char*)this),
        static_cast<size_t>((char*)(&this->nofloat) - (char*)this),
        static_cast<size_t>((char*)(&this->nomb) - (char*)this),
        static_cast<size_t>((char*)(&this->nukeSequence) - (char*)this),
        static_cast<size_t>((char*)(&this->over) - (char*)this),
        static_cast<size_t>((char*)(&this->play) - (char*)this),
        static_cast<size_t>((char*)(&this->playMode) - (char*)this),
        static_cast<size_t>((char*)(&this->loopMode) - (char*)this),
        static_cast<size_t>((char*)(&this->prefetch) - (char*)this),
        static_cast<size_t>((char*)(&this->useThreadedUpload) - (char*)this),
        static_cast<size_t>((char*)(&this->useAppleClientStorage) - (char*)this),
        static_cast<size_t>((char*)(&this->progressiveSourceLoading) - (char*)this),
        static_cast<size_t>((char*)(&this->qtdesktop) - (char*)this),
        static_cast<size_t>((char*)(&this->rangeOffset) - (char*)this),
        static_cast<size_t>((char*)(&this->rangeStart) - (char*)this),
        static_cast<size_t>((char*)(&this->readerThreads) - (char*)this),
        static_cast<size_t>((char*)(&this->workItemThreads) - (char*)this),
        static_cast<size_t>((char*)(&this->rec709) - (char*)this),
        static_cast<size_t>((char*)(&this->resetPrefs) - (char*)this),
        static_cast<size_t>((char*)(&this->sRGB) - (char*)this),
        static_cast<size_t>((char*)(&this->showCMS) - (char*)this),
        static_cast<size_t>((char*)(&this->showFormats) - (char*)this),
        static_cast<size_t>((char*)(&this->showVersion) - (char*)this),
        static_cast<size_t>((char*)(&this->stylusAsMouse) - (char*)this),
        static_cast<size_t>((char*)(&this->sync) - (char*)this),
        static_cast<size_t>((char*)(&this->tgaIOMethod) - (char*)this),
        static_cast<size_t>((char*)(&this->tgaIOSize) - (char*)this),
        static_cast<size_t>((char*)(&this->tgaMaxAsync) - (char*)this),
        static_cast<size_t>((char*)(&this->tiffIOMethod) - (char*)this),
        static_cast<size_t>((char*)(&this->tiffIOSize) - (char*)this),
        static_cast<size_t>((char*)(&this->tiffMaxAsync) - (char*)this),
        static_cast<size_t>((char*)(&this->tile) - (char*)this),
        static_cast<size_t>((char*)(&this->urlsReuseSession) - (char*)this),
        static_cast<size_t>((char*)(&this->useCache) - (char*)this),
        static_cast<size_t>((char*)(&this->useLCache) - (char*)this),
        static_cast<size_t>((char*)(&this->usecli) - (char*)this),
        static_cast<size_t>((char*)(&this->vsync) - (char*)this),
        static_cast<size_t>((char*)(&this->wipes) - (char*)this),
        static_cast<size_t>((char*)(&this->xl) - (char*)this),
        static_cast<size_t>((char*)(&this->noBorders) - (char*)this),
        static_cast<size_t>((char*)(&this->screen) - (char*)this),
        static_cast<size_t>((char*)(&this->x) - (char*)this),
        static_cast<size_t>((char*)(&this->y) - (char*)this),
        static_cast<size_t>((char*)(&this->width) - (char*)this),
        static_cast<size_t>((char*)(&this->height) - (char*)this),
        static_cast<size_t>((char*)(&this->startupResize) - (char*)this),
        static_cast<size_t>((char*)(&this->displayPriority) - (char*)this),
        static_cast<size_t>((char*)(&this->audioPriority) - (char*)this),
        static_cast<size_t>((char*)(&this->stereoSwapEyes) - (char*)this),
        0
    };

    size_t floatoffsets[] = {
        static_cast<size_t>((char*)(&this->audioMaxCache) - (char*)this),
        static_cast<size_t>((char*)(&this->audioMinCache) - (char*)this),
        static_cast<size_t>((char*)(&this->audioOffset) - (char*)this),
        static_cast<size_t>((char*)(&this->audioRate) - (char*)this),
        static_cast<size_t>((char*)(&this->brightness) - (char*)this),
        static_cast<size_t>((char*)(&this->cropX0) - (char*)this),
        static_cast<size_t>((char*)(&this->cropX1) - (char*)this),
        static_cast<size_t>((char*)(&this->cropY0) - (char*)this),
        static_cast<size_t>((char*)(&this->cropY1) - (char*)this),
        static_cast<size_t>((char*)(&this->defaultfps) - (char*)this),
        static_cast<size_t>((char*)(&this->fps) - (char*)this),
        static_cast<size_t>((char*)(&this->gamma) - (char*)this),
        static_cast<size_t>((char*)(&this->lookback) - (char*)this),
        static_cast<size_t>((char*)(&this->maxbwait) - (char*)this),
        static_cast<size_t>((char*)(&this->maxcram) - (char*)this),
        static_cast<size_t>((char*)(&this->maxlram) - (char*)this),
        static_cast<size_t>((char*)(&this->maxvram) - (char*)this),
        static_cast<size_t>((char*)(&this->pixelAspect) - (char*)this),
        static_cast<size_t>((char*)(&this->scale) - (char*)this),
        static_cast<size_t>((char*)(&this->stereoOffset) - (char*)this),
        static_cast<size_t>((char*)(&this->rightEyeStereoOffset) - (char*)this),
        static_cast<size_t>((char*)(&this->totalcram) - (char*)this),
        static_cast<size_t>((char*)(&this->uncropH) - (char*)this),
        static_cast<size_t>((char*)(&this->uncropW) - (char*)this),
        static_cast<size_t>((char*)(&this->uncropX) - (char*)this),
        static_cast<size_t>((char*)(&this->uncropY) - (char*)this),
        static_cast<size_t>((char*)(&this->volume) - (char*)this),
        static_cast<size_t>((char*)(&this->audioGlobalOffset) - (char*)this),
        static_cast<size_t>((char*)(&this->audioDeviceLatency) - (char*)this),
        0
    };

    {
        size_t* o = intoffsets;
        for (const char** f = intfields; *f; f++, o++)
        {
            int* p = (int*) ((char*)this + *o);
            out << "# " << *f << " = " << *p << endl;
        }
    }

    {
        size_t* o = floatoffsets;
        for (const char** f = floatfields; *f; f++, o++)
        {
            float* p = (float*) ((char*)this + *o);
            out << "# " << *f << " = " << *p << endl;
        }
    }

#define CHAROUT(x) "# " << #x << " = " << (x ? x : "") << endl

    out << CHAROUT(audioDevice)
        << CHAROUT(audioModule)
        << CHAROUT(bgpattern)
        << CHAROUT(cinPixel)
        << CHAROUT(cmapString)
        << CHAROUT(selectType)
        << CHAROUT(selectName)
        << CHAROUT(mediaRepName)
        << CHAROUT(mediaRepSource)
        << CHAROUT(cms)
        << CHAROUT(cmsDisplay)
        << CHAROUT(cmsSimulation)
        << CHAROUT(compMode)
        << CHAROUT(connectHost)
        << CHAROUT(dispLUT)
        << CHAROUT(dpxPixel)
        << CHAROUT(eval)
        << CHAROUT(fileCDL)
        << CHAROUT(fileLUT)
        << CHAROUT(initscript)
        << CHAROUT(layoutMode)
        << CHAROUT(licarg)
        << CHAROUT(lookCDL)
        << CHAROUT(lookLUT)
        << CHAROUT(mistPixel)
        << CHAROUT(networkHost)
        << CHAROUT(networkTag)
        << CHAROUT(preCacheLUT)
        << CHAROUT(presentData)
        << CHAROUT(presentDevice)
        << CHAROUT(presentFormat)
        << CHAROUT(qtcss)
        << CHAROUT(qtstyle)
        << CHAROUT(resampleMethod)
        << CHAROUT(schedulePolicy)
        << CHAROUT(sessionType)
        << CHAROUT(stereoMode)
        << CHAROUT(view)
        << CHAROUT(windowType);

    for (int i = 0; i < sendEvents.size(); ++i)
    {
        out << CHAROUT(sendEvents[i].name.c_str())
            << CHAROUT(sendEvents[i].content.c_str());
    }
}

} // 
