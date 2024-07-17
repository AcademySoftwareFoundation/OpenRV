//******************************************************************************
// Copyright (c) 2001-2005 Tweak Inc. All rights reserved.
// 
// SPDX-License-Identifier: Apache-2.0
// 
//******************************************************************************
#include "../../utf8Main.h"

#if defined(_MSC_VER) && _MSC_VER < 1900
#include <winsock2.h>
#include <pthread.h>
#include <implement.h>
#endif

#include <RvCommon/RvConsoleApplication.h>

#ifdef PLATFORM_DARWIN
#include <DarwinBundle/DarwinBundle.h>
#else
#include <QTBundle/QTBundle.h>
#endif

#include <UICommands.h>
#include <IOproxy/IOproxy.h>
#include <MovieProxy/MovieProxy.h>
#include <ImfThreading.h>
#include <MuTwkApp/MuInterface.h>
#include <PyTwkApp/PyInterface.h>
#include <RvApp/CommandsModule.h>
#include <IPCore/Application.h>
#include <RvApp/Options.h>
#include <RvApp/RvNodeDefinitions.h>
#include <IPCore/ImageRenderer.h>
#include <IPCore/LUTIPNode.h>
#include <IPCore/IPGraph.h>
#include <IPBaseNodes/SourceIPNode.h>
#include <IPCore/GroupIPNode.h>
#include <IPCore/OutputGroupIPNode.h>
#ifdef RVIO_HW
#include <MovieRV_FBO/MovieRV_FBO.h>
#else
#include <MovieRV/MovieRV.h>
#endif
#include <MovieFB/MovieFB.h>
#include <MovieMuDraw/MovieMuDraw.h>
#include <MovieProcedural/MovieProcedural.h>
#include <TwkAudio/Audio.h>
#include <TwkMovie/ReformattingMovie.h>
#include <TwkMovie/MovieNullIO.h>
#include <TwkMovie/Movie.h>
#include <TwkMovie/LeaderFooterMovie.h>
#include <TwkMovie/ThreadedMovie.h>
#include <TwkCMS/ColorManagementSystem.h>
#include <TwkMath/Mat44.h>
#include <TwkMath/Iostream.h>
#include <TwkDeploy/Deploy.h>
#include <TwkExc/TwkExcException.h>
#include <TwkFB/IO.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkFB/Operations.h>
#include <TwkFB/TwkFBThreadPool.h>
#include <TwkMovie/MovieIO.h>
#include <TwkUtil/FrameUtils.h>
#include <TwkUtil/PathConform.h>
#include <TwkUtil/SystemInfo.h>
#include <TwkUtil/Daemon.h>
#include <TwkUtil/File.h>
#include <TwkUtil/ThreadName.h>
#include <TwkQtBase/QtUtil.h>
#include <IPCore/ImageRenderer.h>
#include <RvApp/RvSession.h>
#include <RvApp/FormatIPNode.h>
#include <arg.h>
#include <iostream>
#include <sched.h>
#include <stdlib.h>
#include <stl_ext/thread_group.h>
#include <stl_ext/string_algo.h>
#include <string.h>
#include <vector>
#include <ImfHeader.h>

#include <TwkMediaLibrary/Library.h>

#ifdef PLATFORM_WINDOWS
#include <gl/glew.h>
#include <QtGui/QtGui>
#include <QtWidgets/QApplication>
#endif

#include <QtCore/QtCore>

#include <gc/gc.h>

#ifndef PLATFORM_WINDOWS
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>
#endif

#ifdef PLATFORM_WINDOWS
#undef uint16_t
#endif

#ifdef RVIO_HW
#include <TwkGLFFBO/FBOVideoDevice.h>
#else
#include <TwkGLFMesa/OSMesaVideoDevice.h>
#endif

// RVIO third party optional customization
#if defined(RVIO_THIRD_PARTY_CUSTOMIZATION)
    extern void rvioThirdPartyCustomization(TwkApp::Bundle& bundle, char* licarg);
#endif

typedef TwkContainer::StringProperty StringProperty;

extern void TwkMovie_GenericIO_setDebug(bool);
extern void TwkFB_GenericIO_setDebug(bool);

using namespace std;
using namespace TwkFB;
using namespace TwkMovie;
using namespace TwkUtil;
using namespace TwkMath;
using namespace TwkAudio;

struct Illuminant
{
    const char* name;
    float x;
    float y;
};

Illuminant standardIlluminants[] = {
    {"A",   0.44757f, 0.40745f },
    {"B",   0.34842f, 0.35161f},
    {"C",   0.31006f, 0.31616f},
    {"D50",  0.34567f, 0.35850f},
    {"D55",  0.33242f, 0.34743f},
    {"D65",  0.31271f, 0.32902f},
    {"D65REC709",  0.3127f, 0.3290f},
    {"D75",  0.29902f, 0.31485f},
    {"E",   1.0f/3.0f, 1.0f/3.0f},
    {"F1",  0.31310f, 0.33727f},
    {"F2",  0.37208f, 0.37529f},
    {"F3",  0.40910f, 0.39430f},
    {"F4",  0.44018f, 0.40329f},
    {"F5",  0.31379f, 0.34531f},
    {"F6",  0.37790f, 0.38835f},
    {"F7",  0.31292f, 0.32933f},
    {"F8",  0.34588f, 0.35875f},
    {"F9",  0.37417f, 0.37281f},
    {"F10",  0.34609f, 0.35986f},
    {"F11",  0.38052f, 0.37713f},
    {"F12",  0.43695f, 0.40441f},
    {NULL, 0, 0}
};

int parseInFiles(int argc, char *argv[])
{
    //  cerr << "Application::parseInFiles " << argc << " files" << endl;
    Rv::Options& opts = Rv::Options::sharedOptions();

    for (int i=0; i<argc; i++)
    {
        string newS = IPCore::Application::mapFromVar(argv[i]);

        //  cerr << "    " << newS << endl;
        opts.inputFiles.push_back(newS);
    }
    return 0;
}

bool findillum(const char* name, float &x, float &y)
{
    for (Illuminant* i = standardIlluminants; i->name; i++)
    {
        if (!strcmp(i->name, name))
        {
            x = i->x;
            y = i->y;
            return true;
        }
    }

    return false;
}

char* filename       = 0;
int   showFormats    = 0;
char* outputFile     = 0;
char* licarg         = 0;
int   lqt_decoder    = 0;
int   showVersion    = 0;
float filegamma      = 1.0;
float ingamma        = 1.0;
float outgamma       = 1.0;
float exposure       = 0.0;
float scale          = 1.0;
int   iomethod       = -1;
int   iosize         = 61440;
int   resizex        = 0;
int   resizey        = 0;
int   loglin         = 0;
int   loglinc        = 0;
int   redloglin      = 0;
int   redlogfilmlin  = 0;
int   linlog         = 0;
int   verbose        = 0;
int   reallyverbose  = 0;
int   outbits        = 0;
int   processFloat   = 0;
char* outtype        = 0;
int   ysamples       = 0;
int   rysamples      = 0;
int   bysamples      = 0;
int   usamples       = 0;
int   vsamples       = 0;
int   asamples       = 0;
int   xsize          = 0;
int   ysize          = 0;
int   flipImage      = 0;
int   flopImage      = 0;
int   strictlicense  = 0;
char* compressor     = (char*)"";
char* codec          = (char*)"";
char* audioCodec     = (char*)"";
float quality        = 0.9f;
char* paRatio        = (char*)"1:1";
float pixelAspect    = 1.0;
int   paNumerator    = 1;
int   paDenominator  = 1;
char* timerange      = 0;
int   stretch        = 0;
char* comment        = (char*)"";
char* copyright      = (char*)"";
char* initscript     = 0;
int   err2out        = 0;
int   inpremult      = 0;
int   inunpremult    = 0;
int   outpremult     = 0;
int   outunpremult   = 0;
float audioRate      = 0;
int   audioChannels  = 0;
char* outStereo      = (char*)"";
float outfps         = 0;
int   nosession      = 0;
int   outhalf        = 0;
int   out8           = 0;
int   leaderFrames   = 1;
int   outrgb         = 0;
char* dlut           = 0;
int   insrgb         = 0;
int   in709          = 0;
int   outsrgb        = 0;
int   out709         = 0;
int   outaces        = 0;
int   outlogc        = 0;
int   outlogcEI      = 0;
int   outredlog      = 0;
int   outredlogfilm  = 0;
float white0         = -999;
float white1         = -999;
char* illumName      = (char*)"";
int   outadaptive    = 0;
int   tio            = 0;
int   threads        = 1;
int   wthreads       = -1;
int   noprerender    = 0;
char* resampleMethod = (char*)"area";
char* view           = 0;

static void control_c_handler(int sig)
{
    cout << "INFO: stopped by user" << endl;
    exit(1);
}


vector<string>          inputFiles;
vector<string>          inchmap;
vector<string>          outchmap;
vector<vector<string> > leaderArgs;
vector<vector<string> > tailArgs;
vector<vector<string> > overlayArgs;
vector<string>          writerArgs;
float                   timerwait = 1.0 / 192;
deque<TwkUtil::StringPair> outparams;


#ifndef WIN32
extern "C" {
int GC_pthread_create(pthread_t *new_thread,
                      const pthread_attr_t *attr,
                      void *(*start_routine)(void *), void *arg);
#ifdef PLATFORM_DARWIN
int GC_pthread_sigmask(int how, const sigset_t *set, sigset_t *oset);
#endif
int GC_pthread_join(pthread_t thread, void **retval);
int GC_pthread_detach(pthread_t thread);
}
#endif

void
parseParam (string s)
{
    string::size_type pos = s.find("=");

    if (pos != string::npos)
    {
        outparams.push_back(TwkUtil::StringPair(s.substr(0, pos),
                                                s.substr(pos+1, string::npos)));
    }
}

int
parseOutParams(int argc, char *argv[])
{
    for (int i=0; i<argc; i++)
    {
        parseParam (argv[i]);
    }

    return 0;
}

int
parseInChannels(int argc, char* argv[])
{
    for (int i=0; i<argc; i++) inchmap.push_back(argv[i]);
    return 0;
}

int
parseOutChannels(int argc, char* argv[])
{
    for (int i=0; i<argc; i++) outchmap.push_back(argv[i]);
    return 0;
}

int
parseLeader(int argc, char* argv[])
{
    vector<string> args;
    for (int i=0; i<argc; i++) args.push_back(argv[i]);
    leaderArgs.push_back(args);
    return 0;
}

int
parseOverlay(int argc, char* argv[])
{
    vector<string> args;
    for (int i=0; i<argc; i++) args.push_back(argv[i]);
    overlayArgs.push_back(args);
    return 0;
}

int parseOutStereo (int argc, char* argv[])
{
    if (argc == 0) outStereo = (char *) "separate";
    else           outStereo = argv[0];
    return 0;
}

void
threadedMovieInit()
{
#ifdef WIN32
    struct GC_stack_base sb;
    int sb_result;
    sb_result = GC_get_stack_base(&sb);
    GC_register_my_thread(&sb);
#else
    GC_INIT();
#endif
}

TwkMovie::Movie*
makeLeaderMovie(MovieWriter::WriteRequest& writeRequest,
                TwkMovie::Movie* omov,
                Mu::MuLangContext* context,
                Mu::Process* process)
{
    TwkMovie::Movie* lmov = 0;

    //
    //  Optional Leader movie
    //

    if (!leaderArgs.empty())
    {
        ostringstream str;
        str << "solid,red=0,green=0,blue=0,alpha=1,depth=8f"
            << ",width=" << omov->info().width
            << ",height=" << omov->info().height
            << ",fps=" << omov->info().fps;

        int ff = writeRequest.frames.empty() ? omov->info().start : writeRequest.frames.front();
        int fs = ff - leaderFrames;
        str << ",start=" << fs << ",end=" << (fs+leaderFrames-1);

        if (omov->hasAudio())
        {
            str << ",audio=sine,amp=0,rate="
                << omov->info().audioSampleRate;
        }

        str << ".movieproc";

        //
        //  In HW rvio we create a MovieRV_FBO instead of just a MovieProcedural
        //

        MovieRV* mp = new MovieRV();
        Rv::RvSession* session = new Rv::RvSession();
        session->setBatchMode(true);
        session->read(str.str().c_str(), IPCore::Session::ReadRequest());
        session->setViewNode(session->sources()[0]->group()->name());

        int xs = omov->info().width;
        int ys = omov->info().height;

        TwkMovie::MovieInfo info;

        info.width           = xs;
        info.height          = ys;
        info.uncropWidth     = xs;
        info.uncropHeight    = ys;
        info.numChannels     = 4;
        info.audioSampleRate = omov->hasAudio() ? omov->info().audioSampleRate : 0;
        info.audioChannels   = omov->info().audioChannels;

        mp->open(session, info, info.audioChannels, audioRate);
        lmov = mp;

        for (size_t q = 0; q < leaderArgs.size(); q++)
        {
            MovieMuDraw* m = new MovieMuDraw(lmov, context, process);
            m->setFunction(leaderArgs[q].front(), leaderArgs[q]);
            lmov = m;
        }
    }

    return lmov;
}

TwkMovie::Movie*
makeOverlayMovie(TwkMovie::Movie* in,
                 Mu::MuLangContext* context,
                 Mu::Process* process)
{
    if (!overlayArgs.empty())
    {
        for (size_t q = 0; q < overlayArgs.size(); q++)
        {
            MovieMuDraw* m = new MovieMuDraw(in, context, process);
            m->setFunction(overlayArgs[q].front(), overlayArgs[q]);
            in = m;
        }
    }

    return in;
}

MovieRV*
makeInputMovie()
{
    Rv::Options& opts = Rv::Options::sharedOptions();

    opts.readerThreads = threads;

    int xs = xsize;
    int ys = ysize;

    MovieRV* rvmov = new MovieRV();
    Rv::RvSession* session = new Rv::RvSession();
    session->setBatchMode(true);

    if (inputFiles.size() == 1 &&
        extension(inputFiles.front()) == "rv")
    {
        session->read(inputFiles.front().c_str(), IPCore::Session::ReadRequest());
    }
    else
    {
        session->readUnorganizedFileList(inputFiles);
        //
        //  If only one input source, and it's not a session file,
        //  output the source directly instead of the default
        //  seq, so that output frame numbers match, unless the
        //  user specified a view.
        //
        if (session->sources().size() == 1 && !view)
        {
            session->setViewNode(session->sources()[0]->group()->name());
            cerr << "INFO: setting view to single Source" << endl;
        }
    }

    if (*outStereo && strcmp(outStereo, "separate"))
    {
        vector<StringProperty*> props;
        session->findPropertyOfType<StringProperty>(props, "#RVDisplayStereo.stereo.type");
        props.front()->front() = outStereo;
    }

    if (resizey || resizex)
    {
        Rv::resizeAllInputs(session->graph(), resizex, resizey);
    }

    TwkMath::Vec2i size = session->maxSize();

    if (xs == 0 || ys == 0)
    {
        xs = resizex;
        ys = resizey;

        float aspect = float(size[0]) / float(size[1]);
        if (!ys) ys = int(xs / aspect + 0.49f);
        else if (!xs) xs = int(ys * aspect + 0.49f);
    }

    if (xs == 0 || ys == 0)
    {
        xs = size[0];
        ys = size[1];
    }

    xs = int(float(xs) * scale + 0.49f);
    ys = int(float(ys) * scale + 0.49f);

    TwkMovie::MovieInfo info;
    info.width           = xs;
    info.height          = ys;
    info.uncropWidth     = xs;
    info.uncropHeight    = ys;
    info.numChannels     = 4;
    info.audioSampleRate = audioRate;
    info.audioChannels   = layoutChannels(channelLayouts(audioChannels).front());


    if (view && !session->setViewNode(view))
    {
        cout << "ERROR: view not found \"" << view << "\"" << endl;
        exit(-1);
    }
    //
    //  We show the user no cache stats, so don't bother to compute them.
    //
    session->graph().cache().setCacheStatsDisabled(true);

    session->setSessionStateFromNode(session->graph().viewNode());
    rvmov->open(session, info, info.audioChannels, audioRate);

    if (loglin) Rv::setLogLinOnAll(session->graph(), true, 1);
    if (loglinc) Rv::setLogLinOnAll(session->graph(), true, 3);
    if (redloglin) Rv::setLogLinOnAll(session->graph(), true, 6);
    if (redlogfilmlin) Rv::setLogLinOnAll(session->graph(), true, 7);
    if (insrgb) Rv::setSRGBLinOnAll(session->graph(), true);
    if (in709) Rv::setRec709LinOnAll(session->graph(), true);

    if (ingamma != 1.0) Rv::setGammaOnAll(session->graph(), ingamma);
    if (filegamma != 1.0) Rv::setFileGammaOnAll(session->graph(), filegamma);

    if (exposure != 0.0)
    {
        Rv::setFileExposureOnAll(session->graph(), exposure);
        exposure = 0;
    }

    if (flipImage || flopImage)
    {
        Rv::setFlipFlopOnAll(session->graph(), flipImage, true, flopImage, true);
        flipImage = 0;
        flopImage = 0;
    }

    if (!inchmap.empty()) Rv::setChannelMapOnAll(session->graph(), inchmap);
    Rv::fitAllInputs(session->graph(), xs, ys);

    if (dlut)
    {
        try
        {
            session->readLUT(dlut, "#RVDisplayColor", true);
        }
        catch (std::exception& exc)
        {
            cerr << "ERROR: " << exc.what() << endl;
            exit(-1);
        }
    }

    if (opts.fileLUT && strcmp(opts.fileLUT, ""))
    {
        try
        {
            session->readLUTOnAll(opts.fileLUT, "RVLinearize", true);
        }
        catch (std::exception& exc)
        {
            cerr << "ERROR: " << exc.what() << endl;
            exit(-1);
        }
    }

    if (opts.lookLUT && strcmp(opts.lookLUT, ""))
    {
        try
        {
            session->readLUTOnAll(opts.lookLUT, "RVLookLUT", true);
        }
        catch (std::exception& exc)
        {
            cerr << "ERROR: " << exc.what() << endl;
            exit(-1);
        }
    }

    return rvmov;
}

TwkMovie::Movie*
makeFormattedMovie(MovieWriter::WriteRequest& writeRequest,
                   TwkMovie::Movie* omov,
                   TwkMovie::Movie* lmov)
{
    //
    //  Reformatter for the content
    //

    ReformattingMovie* rlmov = lmov ? new ReformattingMovie(lmov) : 0;
    ReformattingMovie* rmov = new ReformattingMovie(omov);

    if (*illumName)
    {
        if (!findillum(illumName, white0, white1))
        {
            cerr << "ERROR: " << illumName
                 << " is not a standard illuminant"
                 << endl;
            exit(-1);
        }
    }

    rmov->setOutputGamma(outgamma);
    rmov->setUseFloatingPoint(processFloat);
    rmov->setVerbose(reallyverbose);
    rmov->setOutputLogSpace(linlog);
    rmov->setOutputSRGB(outsrgb);
    rmov->setOutputRec709(out709);
    rmov->setOutput709toACES(outaces);
    rmov->setOutputLogC(outlogc);
    rmov->setOutputLogCEI(outlogcEI);
    rmov->setOutputRedLog(outredlog);
    rmov->setOutputRedLogFilm(outredlogfilm);
    if (white0 != -999 && white1 != -999) rmov->setOutputWhite(white0, white1);
    rmov->setFlip(flipImage);
    rmov->setFlop(flopImage);
    if (outpremult) rmov->setOutputPremultiply();
    if (outunpremult) rmov->setOutputUnpremultiply();
    if (!outchmap.empty()) rmov->setChannelMap(outchmap);

    if (rlmov)
    {
        rlmov->setVerbose(reallyverbose);
        rlmov->setOutputLogSpace(linlog);
        rlmov->setOutputRedLog(outredlog);
        rlmov->setOutputRedLogFilm(outredlogfilm);
        if (!outchmap.empty()) rlmov->setChannelMap(outchmap);
        rlmov->setFlip(flipImage);
        rlmov->setFlop(flopImage);
        if (outpremult) rlmov->setOutputPremultiply();
        if (outunpremult) rlmov->setOutputUnpremultiply();
    }

    if (ysamples)
    {
        if (rysamples)
        {
            rmov->convertToYRYBY(ysamples, rysamples, bysamples, asamples);

            if (rlmov)
            {
                rlmov->convertToYRYBY(ysamples, rysamples, bysamples, asamples);
            }
        }
        else if (usamples)
        {
            rmov->convertToYUV(ysamples, usamples, vsamples);

            if (rlmov)
            {
                rlmov->convertToYUV(ysamples, usamples, vsamples);
            }
        }
    }

    if (outtype && outbits)
    {
        bool fp = !strcmp(outtype, "float") ||
            !strcmp(outtype, "half") ||
            !strcmp(outtype, "double");

        if (outbits <= 8)
        {
            rmov->setOutputFormat(FrameBuffer::UCHAR);
            if (rlmov) rlmov->setOutputFormat(FrameBuffer::UCHAR);
        }
        else if (outbits <= 16)
        {
            rmov->setOutputFormat(fp ? FrameBuffer::HALF : FrameBuffer::USHORT);
            if (rlmov) rlmov->setOutputFormat(fp ? FrameBuffer::HALF : FrameBuffer::USHORT);
        }
        else
        {
            rmov->setOutputFormat(fp ? FrameBuffer::FLOAT : FrameBuffer::USHORT);
            if (rlmov) rlmov->setOutputFormat(fp ? FrameBuffer::FLOAT : FrameBuffer::USHORT);
        }
    }

    omov = rmov;
    lmov = rlmov;

    //
    //  Sequence leader and content if leader exists
    //

    if (lmov)
    {
        int len = lmov->info().end - lmov->info().start + 1;
        int fs = lmov->info().start;
        int f0 = writeRequest.frames.empty() ? omov->info().start : writeRequest.frames.front();
        int f1 = writeRequest.frames.empty() ? omov->info().end : writeRequest.frames.back();
        LeaderFooterMovie* smov = new LeaderFooterMovie(omov, f0, f1,
                                                        lmov,
                                                        0);
        omov = smov;

        FrameList lframes;
        for (int f = lmov->info().start; f <= lmov->info().end; f++)
        {
            lframes.push_back(f);
        }

        if (!writeRequest.frames.empty())
        {
            writeRequest.frames.insert(writeRequest.frames.begin(),
                                       lframes.begin(),
                                       lframes.end());
        }
    }

    return omov;
}

TwkMovie::Movie*
makeMovieTree(MovieWriter::WriteRequest& writeRequest,
              Mu::MuLangContext* context,
              Mu::Process* process)
{
    MovieRV* reader = makeInputMovie();

    //
    //  Unless they were specified up front by a -t directive,
    //  makeMovieTree must collect some frames. If tio is true then we need to
    //  get those frames trimmed to the session in/out otherwise we can use the
    //  full range of the reader.
    //

    if (writeRequest.frames.empty())
    {
        int fs = reader->info().start;
        int fe = reader->info().end;
        int inc = reader->info().inc;
        if (tio)
        {
            fs = reader->session()->inPoint();
            fe = reader->session()->outPoint() - 1;
            inc = reader->session()->inc();
        }
        writeRequest.timeRangeOverride = true;

        //
        // Enforce forward incraments and fill the frames list smallest to
        // largest.
        //

        for (int i=min(fs,fe); i <= max(fs,fe); i+=abs(inc))
        {
            writeRequest.frames.push_back(i);
        }
    }

    //
    //  Add any overlays, etc
    //

    TwkMovie::Movie* omov   = makeOverlayMovie(reader, context, process);
    TwkMovie::Movie* lmov   = makeLeaderMovie(writeRequest, omov, context, process);
    TwkMovie::Movie* outmov = makeFormattedMovie(writeRequest, omov, lmov);
    //TwkMovie::Movie* outmov = makeFormattedMovie(writeRequest, reader, 0);

    return outmov;
}

string
str(float f)
{
    ostringstream str;
    str << f;
    return str.str();
}

string
str(int i)
{
    ostringstream str;
    str << i;
    return str.str();
}

void
addParam(const string& name, const string& val)
{
    outparams.push_front(StringPair(name, val));
}

void
addDefaultParams()
{
    if (outfps != 0.0) addParam("output/fps", str(outfps));

    if (outgamma != 1.0)
    {
        addParam("output/gamma", str(outgamma));
        addParam("output/transfer", TwkFB::ColorSpace::Gamma());
    }

    if (outsrgb) addParam("output/transfer", TwkFB::ColorSpace::sRGB());
    if (out709) addParam("output/transfer", TwkFB::ColorSpace::Rec709());
    if (outlogc)
    {
        if (outlogcEI == 0) addParam("output/transfer", TwkFB::ColorSpace::ArriLogC());
        else addParam("output/transfer", TwkFB::ColorSpace::ArriLogC() + " EI=" + str(outlogcEI));
    }

    if (outaces)
    {
        addParam("output/ACES", "");
        addParam("output/transfer", TwkFB::ColorSpace::Linear());
        addParam("output/primaries", TwkFB::ColorSpace::ACES());
        // NOTE: these go rX,rY,gX,gY,bX,bY,wX,wY
        addParam("output/chromaticities", "0.73470,0.26530,0.0,1.0,0.00010,-0.07700,0.32168,0.33767");
        addParam("output/neutral", "0.32168,0.33767");
    }

    if (linlog)
    {
        // should add black/white codes too someday
        addParam("output/transfer", TwkFB::ColorSpace::CineonLog());
    }

    if (outredlog)
    {
        addParam("output/transfer", TwkFB::ColorSpace::RedLog());
    }

    if (outredlogfilm)
    {
        addParam("output/transfer", TwkFB::ColorSpace::RedLogFilm());
    }

    if (outpremult) addParam("output/alpha", "PREMULT");
    else if (outunpremult) addParam("output/alpha", "UNPREMULT");

    if (paNumerator)
    {
        addParam("output/pa/numerator", str(paNumerator));
        addParam("output/pa/denominator", str(paDenominator));
    }
    else
    {
        addParam("output/pa", paRatio);
    }
}

bool
parsePA()
{
    string pa = paRatio;

    if (pa != "1:1")
    {
        //
        //  first check for correct syntax of pa
        //

        int colons = 0;
        int dots = 0;

        for (size_t i = 0; i < pa.size(); i++)
        {
            char c = pa[i];
            if (c != ':' && c != '.' && (c < '0' || c > '9')) return false;
            if (c == '.') dots++;
            if (c == ':') colons++;
        }

        if (dots != 0 && colons != 0) return false;

        //
        //  pull it apart if needed
        //

        if (colons)
        {
            string::size_type i = pa.find(':');
            paNumerator = atoi(pa.substr(0, i).c_str());
            paDenominator = atoi(pa.substr(i+1, pa.size() - i).c_str());
        }
        else
        {
            pixelAspect = atof(pa.c_str());
            paNumerator = 0;
            paDenominator = 0;
        }
    }

    return true;
}

void
writeSession(string outfile)
{
    if (!(overlayArgs.empty() && leaderArgs.empty()))
    {
        cerr << "WARNING: \"-overlay\" and \"-leader\" " <<
            "ignored when outputting session" << endl;
    }

    MovieRV* reader = makeInputMovie();

    TwkApp::Document::WriteRequest request;
    request.setOption("tag", string(""));
    request.setOption("compressed", false);
    request.setOption("writeAsCopy", false);
    request.setOption("partial", false);
    request.setOption("sparse", false);

    cout << "INFO: writing session: " << outfile << endl;

    reader->session()->write(outfile.c_str(), request);
}

void setEnvVar(const string& var, const string& val)
{
#ifdef WIN32
    ostringstream str;
    str << var << "=" << val;
    putenv(str.str().c_str());
#else
    setenv(var.c_str(), val.c_str(), 1);
#endif
}

//----------------------------------------------------------------------

int
utf8Main(int argc, char *argv[])
{
    setEnvVar("LANG", "C");
    setEnvVar("LC_ALL", "C");
    TwkFB::ThreadPool::initialize();

    //
    //  XXX dummyDev is leaking here.  Best would be to pass it to the App so
    //  that it could delete it after startup, since it's no longer needed at
    //  that point.
    //

#ifdef RVIO_HW
    TwkGLF::FBOVideoDevice* dummyDev = new TwkGLF::FBOVideoDevice(0, 10, 10, false);
#else
    TwkGLF::OSMesaVideoDevice* dummyDev = new TwkGLF::OSMesaVideoDevice(0, 10, 10, true);
    FrameBuffer* dummyFB = new FrameBuffer(10, 10, 4, FrameBuffer::FLOAT);
    dummyDev->makeCurrent(dummyFB);
#endif
    IPCore::ImageRenderer::queryGL();
    const char* glVersion =(const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
    IPCore::Shader::Function::useShadingLanguageVersion(glVersion);

    #ifndef PLATFORM_WINDOWS
        //
        //  Check the per-process limit on open file descriptors and
        //  reset the soft limit to the hard limit.
        //
        struct rlimit rlim;
        getrlimit (RLIMIT_NOFILE, &rlim);
        rlim.rlim_cur = rlim.rlim_max;
        setrlimit (RLIMIT_NOFILE, &rlim);
    #endif

#ifdef PTW32_STATIC_LIB
    pthread_win32_process_attach_np();
    pthread_win32_process_attach_np();
#endif


#ifdef PLATFORM_DARWIN
    QCoreApplication qapp(argc, argv);
    TwkApp::DarwinBundle bundle("RV", MAJOR_VERSION, MINOR_VERSION, REVISION_NUMBER);
#endif

#ifdef PLATFORM_LINUX
    QCoreApplication qapp(argc, argv);
    TwkApp::QTBundle bundle("rv", MAJOR_VERSION, MINOR_VERSION, REVISION_NUMBER);
#endif

#ifdef PLATFORM_WINDOWS
    QApplication qapp(argc, argv);
    TwkApp::QTBundle bundle("rv", MAJOR_VERSION, MINOR_VERSION, REVISION_NUMBER);
#endif

    Rv::Options& opts = Rv::Options::sharedOptions();

    opts.exrcpus = SystemInfo::numCPUs();
    Rv::Options::manglePerSourceArgs(argv, argc);
    IPCore::ImageRenderer::defaultAllowPBOs(false);
    Imf::staticInitialize();
    IPCore::Application::cacheEnvVars();

    TwkFB::GenericIO::init(); // Initialize TwkFB::GenericIO plugins statics
    TwkMovie::GenericIO::init(); // Initialize TwkMovie::GenericIO plugins statics

    IPCore::AudioRenderer::setNoAudio(true);

#ifdef PLATFORM_WINDOWS
    glewInit(NULL);
#endif

    //
    //  Call the deploy functions
    //

    TWK_DEPLOY_APP_OBJECT dobj(MAJOR_VERSION,
                               MINOR_VERSION,
                               REVISION_NUMBER,
                               argc, argv,
                               RELEASE_DESCRIPTION,
                               "HEAD=" GIT_HEAD);

    //
    //  Set some opts defaults for rvio that differ from RV
    //

    int sleepTime = 0;

    opts.exrInherit        = 0;
    opts.exrNoOneChannel   = 0;
    opts.exrReadWindowIsDisplayWindow = 0;
    opts.exrReadWindow     = 1; // DisplayWindow
    opts.dpxPixel          = (char*)"A2_BGR10";
    opts.cinPixel          = (char*)"A2_BGR10";

#ifdef PLATFORM_WINDOWS
    opts.exrIOMethod       = 2;
    opts.dpxIOMethod       = 2;
    opts.cinIOMethod       = 2;
    opts.tgaIOMethod       = 2;
    opts.jpegIOMethod      = 2;
    opts.tiffIOMethod      = 2;
#else
    opts.exrIOMethod       = 2;
    opts.dpxIOMethod       = 2;
    opts.cinIOMethod       = 2;
    opts.tgaIOMethod       = 2;
    opts.jpegIOMethod      = 2;
    opts.tiffIOMethod      = 2;
#endif

    //
    // We handle the '--help' flag by changing it to '-help' for
    // argparse to work.
    //
    for (int i = 0; i < argc; ++i)
    {
        if (!strcmp("--help", argv[i]))
        {
            strcpy(argv[i], "-help");
            break;
        }
    }

    //
    //  Parse cmd line args
    //

    if (arg_parse
        (argc, argv,
         "", "\nUsage: RVIO (hardware version) movie and image sequence conversion and creation",
         "", "",
         "", "  Make Movie:           rvio in.#.tif -o out.mov",
         "", "  Convert Image:        rvio in.tif -o out.jpg",
         "", "  Convert Image Seq.:   rvio in.#.tif -o out.#.jpg",
         "", "  Movie With Audio:     rvio [ in.#.tif in.wav ] -o out.mov",
         "", "  Movie With LUT:       rvio [ -llut log2film.csp in.#.dpx ] -o out.mov",
         "", "  Rip Movie Range #1:   rvio in.mov -t 1000-1200 -o out.mov",
         "", "  Rip Movie Range #2:   rvio in.mov -t 1000-1200 -o out.#.jpg",
         "", "  Rip Movie Audio:      rvio in.mov -o out.wav",
         "", "  Conform Image:        rvio in.tif -outres 512 512 -o out.tif",
         "", "  Resize Image:         rvio in.#.tif -scale 0.25 -o out.#.jpg",
         "", "  Resize/Stretch:       rvio in.#.tif -resize 640 480 -o out.#.jpg",
         "", "  Resize Keep Aspect:   rvio in.#.tif -resize 1920 0 -o out.#.jpg",
         "", "  Resize Keep Aspt #2:  rvio in.#.tif -resize 0 1080 -o out.#.jpg",
         "", "  Sequence:             rvio cut1.#.tif cut2.mov cut3.1-100#.dpx -o out.mov",
         "", "  Per-Source Arg:       rvio [ -pa 2.0 -fps 30 cut1.#.dpx ] cut2.mov -o out.mov",
         "", "  Stereo Movie File:    rvio [ left.mov right.mov ] -outstereo separate -o out.mov",
         "", "  Stereo Anaglyph:      rvio [ left.mov right.mov ] -outstereo anaglyph -o out.mov",
         "", "  Log Cin/DPX to Movie: rvio -inlog -outsrgb in.#.cin -o out.mov",
         "", "  Output Log Cin/DPX:   rvio -outlog in.#.exr -o out.#.dpx",
         "", "  OpenEXR 16 Bit Out:   rvio in.#.dpx -outhalf -o out.#.exr",
         "", "  OpenEXR to 8 Bit:     rvio in.#.exr -out8 -o out.#.tif",
         "", "  OpenEXR B44 4:2:0:    rvio in.#.exr -outhalf -yryby 1 2 2 -codec B44 -o out.#.exr",
         "", "  OpenEXR B44A 4:2:0:   rvio in.#.exr -outhalf -yrybya 1 2 2 1 -codec B44A -o out.#.exr",
         "", "  OpenEXR DWAA 4:2:0:   rvio in.#.exr -outhalf -yryby 1 2 2 -quality 45 -codec DWAA -o out.#.exr",
         "", "  OpenEXR DWAB 4:2:0:   rvio in.#.exr -outhalf -yrybya 1 2 2 1 -quality 45 -codec DWAB -o out.#.exr",
         "", "  ACES from PD DPX:     rvio in.#.dpx -inlog -outhalf -outaces out.#.aces",
         "", "  ACES from JPEG:       rvio in.#.jpg -insrgb -outhalf -outaces out.#.aces",
         "", "  Chng White to D75:    rvio in.#.exr -outillum D75 -outhalf -o out.#.exr",
         "", "  Chng White to D75 #2: rvio in.#.exr -outwhite 0.29902 0.31485 -outhalf -o out.#.exr",
         "", "  TIFF 32 Bit Float:    rvio in.#.tif -outformat 32 float -o out.#.tif",
         "", "  Anamorphic Unsqueeze: rvio [ -pa 2.0 in_2k_full_ap.#.dpx ] -outres 2048 1556/2 -o out_2k.mov",
         "", "  Camera JPEG to EXR:   rvio -insrgb IMG1234.jpg -o out.exr",
         "", "  Letterbox HD in 1.33: rvio [ -uncrop 1920 1444 0 182 in1080.#.dpx ] -outres 640 480 -o out.mov",
         "", "  Crop 2.35 of Full Ap: rvio [ -crop 0 342 2047 1213 inFullAp.#.dpx ] -o out.mov",
         "", "  Multiple CPUs:        rvio -v -rthreads 3 in.#.dpx -o out.mov",
         "", "  Test Throughput:      rvio -v in.#.dpx -o out.null",
         "", "",
         "", "Advanced EXR/ACES Header Attributes Usage:",
         "", "  Multiple -outparam values can be used.",
         "", "  Type names: f, i, s, sv        -- float, int, string, string vector [N values]",
         "", "              v2i, v2f, v3i, v3f -- 2D and 3D int and float vectors [2 or 3 values required]",
         "", "              b2i, b2f           -- 2D box float and int [4 values required]",
         "", "              c                  -- chromaticities [8 values required]",
         "", "  Passthrough syntax:   -outparams passthrough=REGEX",
         "", "  Attr creation syntax: -outparams NAME:TYPE=VALUE0[,VALUE1,...]\"",
         "", "  EXIF attrs:           rvio exif.jpg -insrgb -o out.exr -outparams \"passthrough=.*EXIF.*\"",
         "", "  Create float attr:    rvio in.exr -o out.exr -outparams pi:f=3.14",
         "", "  Create v2i attr:      rvio in.exr -o out.exr -outparams myV2iAttr:v2i=1,2",
         "", "  Create string attr:   rvio in.exr -o out.exr -outparams \"myAttr:s=HELLO WORLD\"",
         "", "  Chromaticies (XYZ):   rvio XYZ.tiff -o out.exr -outparams chromaticities:c=1,0,0,1,0,0,.333333,.3333333",
         "", "  No Color Adaptation:  rvio in.exr -o out.aces -outaces -outillum D65REC709",
         "", "",
         "", "Example Leader/Overlay Usage:",
         "", "          simpleslate: side-text Field1=Value1 Field2=Value2 ...",
         "", "          watermark: text opacity",
         "", "          frameburn: opacity grey font-point-size",
         "", "          bug: file.tif opacity height",
         "", "          matte: aspect-ratio opacity",
         "", "",
         "", "  Movie w/Slate:        rvio in.#.jpg -o out.mov -leader simpleslate \"FilmCo\" \\",
         "", "                             \"Artist=Jane Q. Artiste\" \"Shot=S01\" \"Show=BlockBuster\" \\",
         "", "                             \"Comments=You said it was too blue so I made it red\"",
         "", "  Movie w/Watermark:    rvio in.#.jpg -o out.mov -overlay watermark \"FilmCo Eyes Only\" .25",
         "", "  Movie w/Frame Burn:   rvio in.#.jpg -o out.mov -overlay frameburn .4 1.0 30.0",
         "", "  Movie w/Bug:          rvio in.#.jpg -o out.mov -overlay bug logo.tif 0.4 128 15 100",
         "", "  Movie w/Matte:        rvio in.#.jpg -o out.mov -overlay matte 2.35 0.8",
         "", "  Multiple:             rvio ... -leader ... -overlay ... -overlay ...",
         "", "",
         RV_ARG_SEQUENCE_HELP,
         "", "",
         RV_ARG_SOURCE_OPTIONS(opts),
         "", "",
         "", "Global arguments",
         "", "",
         "", ARG_SUBR(parseInFiles), "Input sequence patterns, images, movies, or directories ",
         "-o %S", &outputFile, "Output sequence or image",
         "-t %S", &timerange, "Output time range (default=input time range)",
         "-tio", ARG_FLAG(&tio), "Output time range from view's in/out points",
         "-v", ARG_FLAG(&verbose), "Verbose messages",
         "-vv", ARG_FLAG(&reallyverbose), "Really Verbose messages",
         "-q", ARG_FLAG(&processFloat), "Best quality color conversions (not necessary, slower)",
         "-ns", ARG_FLAG(&opts.nukeSequence), "Nuke-style sequences (deprecated and ignored -- no longer needed)",
         "-noRanges", ARG_FLAG(&opts.noRanges), "No separate frame ranges (i.e. 1-10 will be considered a file)", \
         "-rthreads %d", &threads, "Number of reader/render threads (default=1)",
         "-wthreads %d", &wthreads, "Number of writer threads (limited support for this)",
         "-view %S", &view, "View to render (default=defaultSequence or current view in RV file)",
         "-noSequence", ARG_FLAG(&opts.noSequence), "Don't contract files into sequences", \
         "-formats", ARG_FLAG(&showFormats), "Show all supported image and movie formats",
         "-leader", ARG_SUBR(parseLeader), "Insert leader/slate (can use multiple time)",
         "-leaderframes %d", &leaderFrames, "Number of leader frames (default=1)",
         "-overlay", ARG_SUBR(parseOverlay), "Visual overlay(s) (can use multiple times)",
         "-inlog", ARG_FLAG(&loglin), "Convert input to linear space via Cineon Log->Lin",
         "-inredlog", ARG_FLAG(&redloglin), "Convert input to linear space via Red Log->Lin",
         "-inredlogfilm", ARG_FLAG(&redlogfilmlin), "Convert input to linear space via Red Log Film->Lin",
         "-insrgb", ARG_FLAG(&insrgb), "Convert input to linear space from sRGB space",
         "-in709", ARG_FLAG(&in709), "Convert input to linear space from Rec-709 space",
         "-ingamma %f", &ingamma, "Convert input using gamma correction",
         "-filegamma %f", &filegamma, "Convert input using gamma correction to linear space",
         "-inchannelmap", ARG_SUBR(parseInChannels), "map input channels",
         "-inpremult", ARG_FLAG(&inpremult), "premultiply alpha and color",
         "-inunpremult", ARG_FLAG(&inunpremult), "un-premultiply alpha and color",
         "-exposure %f", &exposure, "Apply relative exposure change (in stops)",
         "-scale %f", &scale, "Scale input image geometry",
         "-resize %d [%d]", &resizex, &resizey, "Resize input image geometry to exact size on input",
//       "-resampleMethod %S", &resampleMethod, "Resampling method (area, linear, cubic, nearest, default=%s)", resampleMethod,
         "-dlut %S", &dlut, "Apply display LUT",
         "-flip", ARG_FLAG(&flipImage), "Flip image (flip vertical) (keep orientation flags the same)",
         "-flop", ARG_FLAG(&flopImage), "Flop image (flip horizontal) (keep orientation flags the same)",
         "-yryby %d %d %d", &ysamples, &rysamples, &bysamples, "Y RY BY sub-sampled planar output",
         "-yrybya %d %d %d %d", &ysamples, &rysamples, &bysamples, &asamples, "Y RY BY A sub-sampled planar output",
         "-yuv %d %d %d", &ysamples, &usamples, &vsamples, "Y U V sub-sampled planar output",
         "-outparams", ARG_SUBR(&parseOutParams), "Codec specific output parameters",               \
         "-outchannelmap", ARG_SUBR(parseOutChannels), "map output channels",
         "-outrgb", ARG_FLAG(&outrgb), "same as -outchannelmap R G B",
         "-outpremult", ARG_FLAG(&outpremult), "premultiply alpha and color",
         "-outunpremult", ARG_FLAG(&outunpremult), "un-premultiply alpha and color",
         "-outlog", ARG_FLAG(&linlog), "Convert output to log space via Cineon Lin->Log",
         "-outsrgb", ARG_FLAG(&outsrgb), "Convert output to sRGB ColorSpace",
         "-out709", ARG_FLAG(&out709), "Convert output to Rec-709 ColorSpace",
         "-outredlog", ARG_FLAG(&outredlog), "Convert output to Red Log ColorSpace",
         "-outredlogfilm", ARG_FLAG(&outredlogfilm), "Convert output to Red Log Film ColorSpace",
         "-outgamma %f", &outgamma, "Apply gamma to output",
         "-outstereo", ARG_SUBR(&parseOutStereo), "Output stereo (checker, scanline, anaglyph, left, right, pair, mirror, hsqueezed, vsqueezed, default=separate)",
         "-outformat %d %S", &outbits, &outtype, "Output bits and format (e.g. 16 float -or- 8 int)",
         "-outhalf", ARG_FLAG(&outhalf), "Same as -outformat 16 float",
         "-out8", ARG_FLAG(&out8), "Same as -outformat 8 int",
         "-outres %d %d", &xsize, &ysize, "Output resolution",
         "-outfps %f", &outfps, "Output FPS",
         "-outaces", ARG_FLAG(&outaces), "Output ACES gamut (converts pixels to ACES)",
         "-outwhite %f %f", &white0, &white1, "Output white CIE 1931 chromaticity x, y",
         "-outillum %S", &illumName, "Output standard illuminant name (A-C, D50, D55, D65, D65REC709, D75 E, F[1-12])",
         "-codec %S", &codec, "Output codec (varies with file format)",
         "-audiocodec %S", &audioCodec, "Output audio codec (varies with file format)",
         "-audiorate %f", &audioRate, "Output audio sample rate (default 48000)",
         "-audiochannels %d", &audioChannels, "Output audio channels (default 2)",
         "-quality %f", &quality, "Output codec quality 0.0 -> 1.0 (100000 for DWAA/DWAB) (varies w/format and codec default=%g)", quality,
         "-outpa %S", &paRatio, "Output pixel aspect ratio (e.g. 1.33 or 16:9, etc. metadata only) default=%s", paRatio,
         "-comment %S", &comment, "Ouput comment (movie files, default=\"%s\")", comment,
         "-copyright %S", &copyright, "Ouput copyright (movie files, default=\"%s\")", copyright,
         "-lic %S", &licarg, "Use specific license file (this param is simply ignored for Open RV as it is not required)",
         "-debug", ARG_SUBR(&Rv::parseDebugKeyWords), "Debug category",               \
         "-version", ARG_FLAG(&showVersion), "Show RVIO version number",
         "-iomethod %d [%d]", &iomethod, &iosize, "I/O Method (overrides all) (0=standard, 1=buffered, 2=unbuffered, 3=MemoryMap, 4=AsyncBuffered, 5=AsyncUnbuffered, default=%d) and optional chunk size (default=%d)", iomethod, iosize,  \
         "-exrcpus %d", &opts.exrcpus, "EXR thread count (default=%d)", opts.exrcpus, \
         "-exrRGBA", ARG_FLAG(&opts.exrRGBA), "EXR Always read as RGBA (default=false)", \
         "-exrInherit", ARG_FLAG(&opts.exrInherit), "EXR guess channel inheritance (default=false)", \
         "-exrNoOneChannel", ARG_FLAG(&opts.exrNoOneChannel), "EXR never use one channel planar images (default=false)", \
         "-exrIOMethod %d [%d]", &opts.exrIOMethod, &opts.exrIOSize, "EXR I/O Method (0=standard, 1=buffered, 2=unbuffered, 3=MemoryMap, 4=AsyncBuffered, 5=AsyncUnbuffered, default=%d) and optional chunk size (default=%d)", opts.exrIOMethod, opts.exrIOSize, \
         "-exrReadWindowIsDisplayWindow", ARG_FLAG(&opts.exrReadWindowIsDisplayWindow), "EXR read window is display window (default=false)", \
         "-exrReadWindow %d", &opts.exrReadWindow, "EXR Read Window Method (0=Data, 1=Display, 2=Union, 3=Data inside Display, default=%d)", opts.exrReadWindow, \
         "-jpegRGBA", ARG_FLAG(&opts.jpegRGBA), "Make JPEG four channel RGBA on read (default=no, use RGB or YUV)", \
         "-jpegIOMethod %d [%d]", &opts.jpegIOMethod, &opts.jpegIOSize, "JPEG I/O Method (0=standard, 1=buffered, 2=unbuffered, 3=MemoryMap, 4=AsyncBuffered, 5=AsyncUnbuffered, default=%d) and optional chunk size (default=%d)", opts.exrIOMethod, opts.exrIOSize, \
         "-cinpixel %S", &opts.cinPixel, "Cineon pixel storage (default=%s)", opts.cinPixel, \
         "-cinchroma", ARG_FLAG(&opts.cinchroma), "Use Cineon chromaticity values (for default reader only)", \
         "-cinIOMethod %d [%d]", &opts.cinIOMethod, &opts.cinIOSize, "Cineon I/O Method (0=standard, 1=buffered, 2=unbuffered, 3=MemoryMap, 4=AsyncBuffered, 5=AsyncUnbuffered, default=%d) and optional chunk size (default=%d)", opts.cinIOMethod, opts.cinIOSize, \
         "-dpxpixel %S", &opts.dpxPixel, "DPX pixel storage (default=%s)", opts.dpxPixel, \
         "-dpxchroma", ARG_FLAG(&opts.dpxchroma), "Use DPX chromaticity values (for default reader only)", \
         "-dpxIOMethod %d [%d]", &opts.dpxIOMethod, &opts.dpxIOSize, "DPX I/O Method (0=standard, 1=buffered, 2=unbuffered, 3=MemoryMap, 4=AsyncBuffered, 5=AsyncUnbuffered, default=%d) and optional chunk size (default=%d)", opts.dpxIOMethod, opts.dpxIOSize,  \
         "-tgaIOMethod %d [%d]", &opts.tgaIOMethod, &opts.tgaIOSize, "TARGA I/O Method (0=standard, 1=buffered, 2=unbuffered, 3=MemoryMap, 4=AsyncBuffered, 5=AsyncUnbuffered, default=%d) and optional chunk size (default=%d)", opts.tgaIOMethod, opts.tgaIOSize,  \
         "-tiffIOMethod %d [%d]", &opts.tiffIOMethod, &opts.tiffIOSize, "TIFF I/O Method (0=standard, 1=buffered, 2=unbuffered, 3=MemoryMap, 4=AsyncBuffered, 5=AsyncUnbuffered, default=%d) and optional chunk size (default=%d)", opts.tgaIOMethod, opts.tgaIOSize,  \
         "-init %S", &initscript, "Override init script",
         "-err-to-out", ARG_FLAG(&err2out), "Output errors to standard output (instead of standard error)",
         "-strictlicense", ARG_FLAG(&strictlicense), "Exit rather than consume an RV license if no rvio licenses are available (this param is simply ignored for Open RV as it is not required)",
         //"-noprerender", ARG_FLAG(&noprerender), "Turn off prerendering optimization",
         "-flags", ARG_SUBR(&Rv::parseMuFlags), "Arbitrary flags (flag, or 'name=value') for Mu",
#if defined(PLATFORM_WINDOWS) && !defined(NDEBUG)
         "-sleep %d", &sleepTime, "Sleep (in seconds) before starting to allow attaching debugger",
#endif
         NULL) < 0)
    {
       exit(-1);
    }

    TwkMovie::MovieWriter::setReallyVerbose(reallyverbose);

    //
    //  If the usere hasn't specified a number of writer threads, use the number
    //  of reader threads.
    //

    if (wthreads == -1) wthreads = threads;

    opts.delaySessionLoading = 0;

    // Note that some additional work would be required to enable progressive
    // source loading in RVIO. Some synchronization is required to know when
    // all the sources have been actually loaded.
    // With the aim of minimizing risks for the RV 7.8.0, this specific
    // optimization task has been intentionally postponed.
    opts.progressiveSourceLoading = 0;

    //
    //  Set up ENV vars for IO plugins based on opts
    //

    if (iomethod != -1)
    {
        //
        //  iomethod overrides all specific iomethods for backwards compat
        //

        opts.exrIOMethod  = iomethod;
        opts.dpxIOMethod  = iomethod;
        opts.cinIOMethod  = iomethod;
        opts.tgaIOMethod  = iomethod;
        opts.jpegIOMethod = iomethod;
        opts.tiffIOMethod = iomethod;
        opts.exrIOSize    = iosize;
        opts.dpxIOSize    = iosize;
        opts.cinIOSize    = iosize;
        opts.tgaIOSize    = iosize;
        opts.jpegIOSize   = iosize;
        opts.tiffIOSize   = iosize;
    }

    opts.exportIOEnvVars();

    //
    //  Sleep is used for debugging on windows -- need time to manually attach
    //

#if defined(PLATFORM_WINDOWS) && !defined(NDEBUG)
    if (sleepTime > 0)
    {
        cout << "INFO: sleeping " << sleepTime << " seconds" << endl;
        Sleep(sleepTime * 1000);
        cout << "INFO: continuing after sleep" << endl;
    }
#endif

    if (!parsePA())
    {
        cout << "ERROR: bad pixel aspect ratio syntax" << endl
             << "       you can use a scalar float: e.g. 1.33" << endl
             << "       or a ratio of integers: e.g. 16:9" << endl;

        return -1;
    }

    //
    //  Add default params first so that later user params will
    //  override them.  E.g. if you do -out709 and then for dpx
    //  transfer=USER it will put USER in the header instead of
    //  REC709.
    //

    addDefaultParams();

    if (const char *envParams = getenv("RVIO_OUTPARAMS"))
    {
        string s(envParams);
        vector<string> params;
        stl_ext::tokenize(params, s, " ");
        for (int i = 0; i < params.size(); ++i) parseParam(params[i]);
    }

    if (!opts.initializeAfterParsing(0))
    {
        cerr << "ERROR: failed to initialize options" << endl;
        exit(-1);
    }

    IPCore::FormatIPNode::defaultResampleMethod = resampleMethod;
    inputFiles = opts.inputFiles;

    //
    //  Just stop avoidresampling
    //

    nosession = 0;

    if (err2out) cerr.rdbuf(cout.rdbuf());

    if (showVersion)
    {
        cout << MAJOR_VERSION << "."
             << MINOR_VERSION << "."
             << REVISION_NUMBER << endl;

        exit(0);
    }

    if (outhalf) { outbits = 16; outtype = (char*)"float"; }
    if (out8)    { outbits = 8;  outtype = (char*)"int"; }
    if (reallyverbose) verbose = 1;

    if (outrgb)
    {
        outchmap.clear();
        outchmap.push_back("R");
        outchmap.push_back("G");
        outchmap.push_back("B");
    }

    if ((rysamples || bysamples) && (usamples || vsamples))
    {
        cerr << "ERROR: can output Y U V -or- Y RY BY but not both" << endl;
        exit(-1);
    }

    //
    //  Banners
    //

    TWK_DEPLOY_SHOW_PROGRAM_BANNER(cout);
    TWK_DEPLOY_SHOW_COPYRIGHT_BANNER(cout);
    TWK_DEPLOY_SHOW_LOCAL_BANNER(cout);

    //
    //  Get CPU info and tell the EXR library to use them all
    //


    if (opts.exrcpus > 1) Imf::setGlobalThreadCount(opts.exrcpus);

    string initPath = bundle.rcfile("rviorc", "mu", "RVIO_INIT");
    bundle.addPathToEnvVar("OIIO_LIBRARY_PATH", bundle.appPluginPath("OIIO"));
    if (initscript) initPath = initscript;

    // RVIO third party optional customization
#if defined(RVIO_THIRD_PARTY_CUSTOMIZATION)
    rvioThirdPartyCustomization(bundle, licarg);
#endif

    try
    {
        TwkFB::loadProxyPlugins("TWK_FB_PLUGIN_PATH");
        TwkMovie::loadProxyPlugins("TWK_MOVIE_PLUGIN_PATH");
    }
    catch (...)
    {
        cerr << "WARNING: a problem occured while loading image plugins." << endl;
        cerr << "         some plugins may not have been loaded." << endl;
    }

    TwkMovie::GenericIO::addPlugin(new MovieRVIO());
    TwkMovie::GenericIO::addPlugin(new MovieFBIO());
    TwkMovie::GenericIO::addPlugin(new MovieProceduralIO());
    TwkMovie::GenericIO::addPlugin(new MovieNullIO());

    //
    //  Compile the list of sequencable file extensions
    //

    TwkFB::GenericIO::compileExtensionSet(predicateFileExtensions());

    Rv::RvConsoleApplication app;

    //
    //  Initialize everything
    //

    try
    {
        TwkApp::initMu(0);
        TwkApp::initPython();
        RVIO::initUICommands(TwkApp::muContext());
        Rv::initCommands(TwkApp::muContext());
    }
    catch (const exception &e)
    {
        cerr << "ERROR: during initialization: " << e.what() << endl;
        exit( -1 );
    }

#ifndef PLATFORM_WINDOWS
    if (signal(SIGINT, control_c_handler) == SIG_ERR)
    {
        cout << "ERROR: failed to install SIGINT signal handler" << endl;
    }
#endif

    if (showFormats)
    {
        TwkMovie::GenericIO::outputFormats();
        exit(0);
    }

    if (inputFiles.empty())
    {
        cerr << "ERROR: no input files specified" << endl;
        exit(-1);
    }

    if (!outputFile)
    {
        cerr << "ERROR: no output file/sequence specified" << endl;
        exit(-1);
    }

    string outfile = pathConform(outputFile);

    //
    //  Set up Mu context and process. These will be shared by all of
    //  the drawing functions. (Allows for more hacking if you can set
    //  state and use it later).
    //

    Mu::MuLangContext* context = TwkApp::muContext();
    Mu::Process*       process = TwkApp::muProcess();

    TwkUtil::setThreadName("RVIO Main");

    //
    //  Main
    //

    try
    {
        // If this is just a session dump then handle that first
        if (extension(outfile) == "rv")
        {
            writeSession(outfile);
            return 0;
        }

        FrameBufferIO::ReadRequest readRequest;
        MovieWriter::WriteRequest writeRequest;

        writeRequest.threads        = wthreads;
        writeRequest.fps            = outfps;
        writeRequest.compression    = codec;
        writeRequest.codec          = codec;
        writeRequest.audioCodec     = audioCodec;
        writeRequest.quality        = quality;
        writeRequest.pixelAspect    = pixelAspect;
        writeRequest.verbose        = verbose;
        writeRequest.audioChannels  = audioChannels;
        writeRequest.audioRate      = audioRate;
        writeRequest.stereo         = !strcmp(outStereo, "separate");
        writeRequest.comments       = comment;
        writeRequest.copyright      = copyright;
        writeRequest.args           = writerArgs;

        copy(outparams.begin(),
             outparams.end(),
             back_inserter(writeRequest.parameters));

        if (timerange)
        {
            writeRequest.timeRangeOverride = true;
            writeRequest.frames = frameRange(timerange);

            if (writeRequest.frames.empty())
            {
                cerr << "ERROR: timerange \""
                     << timerange
                     << "\" expands to nothing"
                     << endl;

                exit(-1);
            }
        }

        if (rysamples || usamples)
        {
            writeRequest.preferCommonFormat = false;
            writeRequest.keepPlanar = true;
        }

        vector<TwkMovie::Movie*> inputMovies;
        MovieWriter::Frames outFrames;

        {
            Mu::Process* p = TwkApp::muProcess();
            inputMovies.push_back(makeMovieTree(writeRequest, context, p));
            outFrames = writeRequest.frames;
        }

        TwkMovie::Movie* outmov = 0;

#if 1
#ifdef WIN32
        GC_INIT();
        GC_allow_register_threads();
        outmov = new ThreadedMovie(inputMovies, outFrames, 8, 0, threadedMovieInit);
#else
        ThreadedMovie::ThreadAPI threadAPI;
        threadAPI.create = GC_pthread_create;
        threadAPI.join   = GC_pthread_join;
        threadAPI.detach = GC_pthread_detach;
        outmov = new ThreadedMovie(inputMovies, outFrames, 8, &threadAPI, threadedMovieInit);
#endif
#endif

        //assert(inputMovies.size() == 1);
        //outmov = inputMovies.front();

        //
        //   On windows, we need to do this *after* the threaded movie
        //   has been created. Not sure why.
        //

        TwkApp::initWithFile(TwkApp::muContext(),
                             TwkApp::muProcess(),
                             TwkApp::muModuleList(),
                             initPath.c_str());

        //
        //  Since the sessions we're created earlier (before we
        //  called initWithFile), we call postInitialize on each
        //  thread's session here.
        //
        //  Note that these sesssions may have "extra" sources in
        //  them representing the leader/overlay movies.
        //
        for (int i = 0; i < IPCore::App()->documents().size(); ++i)
        {
            if (Rv::RvSession *s = dynamic_cast<Rv::RvSession*> (IPCore::App()->documents()[i]))
            {
                s->postInitialize();
                //
                //  We show the user no cache stats, so don't bother to compute them.
                //
                s->graph().cache().setCacheStatsDisabled(true);
            }
        }

        MovieWriter* writer = TwkMovie::GenericIO::movieWriter(pathConform(outfile));

        if (!writer)
        {
            cerr << "ERROR: cannot find a way to write " << outfile << endl;
            exit(-1);
        }

        //
        //  Write it.
        //
        //  NOTE: some formats will automatically do color conversion
        //  to move the pixels into the right colorspace. Since we
        //  already transformed the image into linear non-planar none
        //  of that should be happening.
        //

        if (verbose)
        {
            cout << "INFO: writing " << outfile << endl;

            if (!writeRequest.preferCommonFormat && reallyverbose)
            {
                cout << "INFO: request non-common format" << endl;
            }

            if (writeRequest.compression != "")
            {
                cout << "INFO: output compressor " << writeRequest.compression << endl;
            }

            if (writeRequest.codec != "")
            {
                cout << "INFO: output codec " << writeRequest.codec << endl;
            }

            if (reallyverbose)
            {
                cout << "INFO: output quality " << writeRequest.quality << endl;
            }

            if (writeRequest.timeRangeOverride && timerange)
            {
                cout << "INFO: override time range " << timerange
                     << ", (" << writeRequest.frames.size()
                     << " frames)"
                     << endl;
            }

            if (writeRequest.keepPlanar && reallyverbose)
            {
                cout << "INFO: request planar output if possible" << endl;
            }
        }

        //
        //  Tell the writer to do its business
        //

        if (!writer->write(outmov, outfile, writeRequest)) exit (-2);

        // clean up any frame buffers we allocated writing the movie
        //
        MovieRV::uninit();
    }
    catch (TwkExc::Exception& exc)
    {
        cerr << exc << endl;
        exit(-1);
    }
    catch (exception& exc)
    {
        cerr << "ERROR: " << exc.what() << endl;
        exit(-1);
    }
    catch (...)
    {
        cerr << "ERROR: uncaught exception" << endl;
        exit(-1);
    }

#ifdef PTW32_STATIC_LIB
    pthread_win32_thread_detach_np();
    pthread_win32_process_detach_np();
#endif

    TwkMovie::GenericIO::shutdown(); // Shutdown TwkMovie::GenericIO plugins
    TwkFB::GenericIO::shutdown(); // Shutdown TwkFB::GenericIO plugins

    TwkFB::ThreadPool::shutdown();
    return 0;
}

