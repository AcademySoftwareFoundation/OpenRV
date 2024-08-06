//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//  
//  SPDX-License-Identifier: Apache-2.0
//  
//
#include <RvApp/CommandsModule.h>
#include <RvApp/Options.h>
#include <RvApp/RvNodeDefinitions.h>
#include <RvApp/RvSession.h>
#include <RvApp/FormatIPNode.h>
#include <RvApp/FileSpaceLinearizeIPNode.h>
#include <IPBaseNodes/CacheLUTIPNode.h>
#include <IPBaseNodes/ChannelMapIPNode.h>
#include <IPBaseNodes/ColorIPNode.h>
#include <IPBaseNodes/FileSourceIPNode.h>
#include <IPBaseNodes/LensWarpIPNode.h>
#include <IPBaseNodes/ImageSourceIPNode.h>
#include <IPBaseNodes/SequenceGroupIPNode.h>
#include <IPBaseNodes/SequenceIPNode.h>
#include <IPBaseNodes/SourceGroupIPNode.h>
#include <IPBaseNodes/SourceIPNode.h>
#include <IPBaseNodes/SourceStereoIPNode.h>
#include <IPBaseNodes/StackGroupIPNode.h>
#include <IPBaseNodes/StackIPNode.h>
#include <IPBaseNodes/SwitchIPNode.h>
#include <IPCore/AdaptorIPNode.h>
#include <IPCore/Application.h>
#include <IPCore/CacheIPNode.h>
#include <IPCore/ImageRenderer.h>
#include <IPCore/DefaultMode.h>
#include <IPCore/DispTransform2DIPNode.h>
#include <IPCore/DisplayGroupIPNode.h>
#include <IPCore/DisplayIPNode.h>
#include <IPCore/DisplayStereoIPNode.h>
#include <IPCore/Exception.h>
#include <IPCore/FBCache.h>
#include <IPCore/IPInstanceNode.h>
#include <IPCore/NodeDefinition.h>
#include <IPCore/NodeManager.h>
#include <IPCore/PerFrameAudioRenderer.h>
#include <IPCore/PipelineGroupIPNode.h>
#include <IPCore/SessionIPNode.h>
#include <IPCore/SoundTrackIPNode.h>
#include <IPCore/Transform2DIPNode.h>
#include <CDL/cdl_utils.h>
#include <Mu/Context.h>
#include <Mu/GarbageCollector.h>
#include <Mu/MuProcess.h>
#include <MuLang/MuLangContext.h>
#include <MuTwkApp/MuInterface.h>
#include <PyTwkApp/PyInterface.h>
#include <TwkApp/Event.h>
#include <TwkContainer/GTOReader.h>
#include <TwkContainer/GTOWriter.h>
#include <TwkContainer/PropertyContainer.h>
#include <TwkGLText/TwkGLText.h>
#include <TwkDeploy/Deploy.h>
#include <TwkMath/Function.h>
#include <TwkMath/Iostream.h>
#include <TwkMath/Math.h>
#include <TwkMovie/Movie.h>
#include <TwkUtil/File.h>
#include <TwkUtil/FrameUtils.h>
#include <TwkUtil/PathConform.h>
#include <TwkUtil/TwkRegEx.h>
#include <TwkUtil/sgcHop.h>
#include <TwkUtil/Clock.h>
#include <TwkUtil/EnvVar.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>
#include <numeric>
#include <sstream>
#include <stl_ext/stl_ext_algo.h>
#include <stl_ext/string_algo.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#ifndef PLATFORM_WINDOWS
#include <stdio.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>
#else
#include <windows.h>
#endif

#include <gc/gc.h>


template <typename T>
inline T mod( const T &a, const T &by )
{
    T rem = a % by;
    return rem < T(0) ? rem + by : rem;
}

// NOT A FRAME
#define NAF (std::numeric_limits<int>::min())

//  float worstFpsError = 0.0;

namespace Rv {

#if 0
#define DB_GENERAL      0x01
#define DB_EDGES        0x02
#define DB_ALL          0xff

//  #define DB_LEVEL        (DB_ALL & (~ DB_EDGES))
#define DB_LEVEL        DB_ALL

#define DB(x)           if (DB_GENERAL & DB_LEVEL) cerr << "Session: " << x << endl
#define DBL(level, x)   if (level & DB_LEVEL)      cerr << "Session: " << x << endl
#else
#define DB(x)
#define DBL(level, x)
#endif

using namespace TwkMovie;
using namespace TwkMath;
using namespace TwkApp;
using namespace TwkUtil;
using namespace TwkContainer;
using namespace Mu;
using namespace std;
using namespace IPCore;
using namespace boost;

std::string RvSession::m_initEval;
std::string RvSession::m_pyInitEval;

static RegEx oldContainerNameRE("^([a-zA-Z0-9]+[^0-9])([0-9][0-9][0-9])$");
static bool debugGC = false;
static std::string defaultSequenceSequenceIPNodeName("defaultSequence_sequence");

static ENVVAR_BOOL( evUseSequenceEventsTracking, "RV_USE_SEQUENCE_EVENTS_TRACKING", true );

namespace {
static bool firstTime = true;

bool
checkForDirFiles(string filename, bool noSeq, SequenceNameList* files)
{
#ifdef PLATFORM_WINDOWS
    if (filename.size() && filename[filename.size() - 1] == '/')
    {
        filename.erase(filename.size() - 1, 1);
    }
#endif

#ifdef _MSC_VER
    struct _stat64 sb;
#else
    struct stat sb;
#endif
    memset(&sb, 0, sizeof(sb));
    int err = TwkUtil::stat(filename.c_str(), &sb);

    bool isDir = false;
    if (!err && sb.st_mode & S_IFDIR)
    //
    //  Pattern is a directory, so unpack possibly into multiple
    //  sources (or multiple layers) here.
    //
    {
        isDir = true;
        SequenceNameList seqs = sequencesInDirectory(filename,
                noSeq ? FalseSequencePredicate : GlobalExtensionPredicate);

        for (int i=0; i < seqs.size(); i++)
        {
            if (seqs[i][0] == '.') continue;
            string fullname = filename + "/" + seqs[i];
            files->push_back(fullname);
        }
    }
    else files->push_back(filename);

    return isDir;
}

// Returns true if the specified source arguments has initialized (non default)
// cut points. Otherwise returns false.
bool hasCutPoints(const Options::SourceArgs& sargs)
{
    // Note that the default cutIn is initialized with either plus or minus
    // numeric_limits<int>::max() depending on where it is initialized
    // cutOut on the other hand is always initialized with the plus version.
    return sargs.cutIn!=-numeric_limits<int>::max() &&
           sargs.cutIn!=numeric_limits<int>::max() &&
           sargs.cutOut!=numeric_limits<int>::max();
}

} // empty namespace

RvSession::RvSession()
    : Session(new RvGraph(IPCore::Application::instance()->nodeManager())),
      m_loadingError(false),
      m_data(0),
      m_pydata(0),
      m_callEnv(new TwkApp::CallEnv(this)),
      m_loadState(0),
      m_gtoSourceCount(0),
      m_gtoSourceTotal(0),
      m_userHasSetViewSize(false),
      m_conductorSource(nullptr)
{
    Options& opts = Options::sharedOptions();
    m_realtime = (opts.playMode == 2);
    m_playMode = static_cast<PlayMode>(opts.loopMode);

    if (firstTime)
    {
        //
        //  Initialize nodes here -- at this point we know that GL has
        //  been initialized which is currently required for node
        //  initialization.
        //

        IPCore::Application::setOptionValueFromEnvironment("nodePath", "TWK_NODE_PLUGIN_PATH");
        IPCore::Application::setOptionValueFromEnvironment("nodeDefinitionOverride", "TWK_NODE_DEFINITION_OVERRIDE");
        addRvNodeDefinitions(IPCore::Application::instance()->nodeManager());
        IPCore::Application::instance()->loadOptionNodeDefinitions();
        firstTime = false;
    }

    m_mediaLoadingSetEmptyConnection = graph().mediaLoadingSetEmptySignal().connect(boost::bind(&RvSession::onGraphMediaSetEmpty, this));

    // register the signal to detecting removing of node in the graphe
    m_nodeWillRemoveConnection = graph().nodeWillRemoveSignal().connect(boost::bind(&RvSession::onGraphNodeWillRemove, this, std::placeholders::_1));

    //
    // register the signal to detect fastAddSourceEnabled in the graphe
    auto rvGraph = dynamic_cast<RvGraph*>(&graph());
    if (rvGraph)
        m_fastAddSourceChangedConnection = rvGraph->fastAddSourceChangedSignal().connect(boost::bind(&RvSession::onGraphFastAddSourceChanged, this, std::placeholders::_1, std::placeholders::_2));

    graph().clear();

    if (getenv("RV_DEBUG_GC")) debugGC = true;

    applyRvSessionSpecificOptions();
}

RvSession::~RvSession()
{
    m_fastAddSourceChangedConnection.disconnect();
    m_mediaLoadingSetEmptyConnection.disconnect();
    m_nodeWillRemoveConnection.disconnect();

    if (m_data) m_data->releaseExternal();
    m_data = 0;

    //
    //  Force a GC so we can hopefully remove any unwanted objects
    //

    Mu::GarbageCollector::collect();

    //
    //  Same with python
    //

    disposeOfPythonObject(m_pydata);

    //
    //  let the scripting stuff leak. There are timing issues with
    //  deletion. Hopefully at this point its all gone anyway.
    //

    ((TwkApp::CallEnv*)m_callEnv)->invalidate();
    //delete m_callEnv;
}

void
RvSession::makeActive()
{
    Session::m_currentSession = this;
    muProcess()->setCallEnv(m_callEnv);
    Session::makeActive();
}


void
RvSession::clear()
{
    Session::clear();
    m_loadingErrorString = "";
    m_loadingError = false;
    m_conductorSource = nullptr;
    m_sequenceIPNode = nullptr;

    if (!m_beingDeleted)
    {
        applyRvSessionSpecificOptions();
    }
}

void
RvSession::render()
{
    Session::render();

    try
    {
        continueLoading();
    }
    catch (const std::exception& exc)
    {
        cerr << "ERROR: " << exc.what() << endl;
    }
    catch (...)
    {
        cerr << "ERROR: Unknown Exception" << endl;
    }
}

void
RvSession::applyRvSessionSpecificOptions()
{
    Options& opts = Options::sharedOptions();

    if (opts.noSequence) m_noSequences = true;
    if (opts.scale != 1.0) setScaleOnAll(opts.scale);
    setFPS(opts.defaultfps);
    if (opts.fullscreen) fullScreenMode(true);
    setRendererType("Composite");

    if (opts.readerThreads > 0 &&
        opts.readerThreads <= 32)
    {
        graph().setNumEvalThreads(opts.readerThreads);
    }

    if (!strcmp(opts.sessionType, "sequence"))
    {
        setSessionType(SequenceSession);
    }
    else if (!strcmp(opts.sessionType, "stack"))
    {
        setSessionType(StackSession);
    }

    setGlobalAudioOffset(opts.audioGlobalOffset);
    setGlobalSwapEyes(opts.stereoSwapEyes != 0);

    m_muFlags = opts.muFlags;
}


std::string
RvSession::lookupMuFlag(std::string key)
{
    StringMap::iterator i = m_muFlags.find(key);
    if (i == m_muFlags.end()) return "";
    else return i->second;
}



class LoadState
{
  public:
    LoadState(
        RvSession* s,
        Options::SourceArgsVector& sav,
        bool noSeq,
        bool dpo,
        bool mg,
        string tg);

    bool isLoading()        { return (!m_sav.empty() && m_savIndex < m_sav.size()); }
    int loadCount()         { return isLoading() ? m_loadCount : 0; }
    int loadTotal()         { return m_totalCount; }
    void    storeCacheMode(RvSession::CachingMode m)
                            { m_cacheMode = m; }
    RvSession::CachingMode
            cacheMode()     { return m_cacheMode; }
    bool doProcessOpts()    { return m_doProcessOpts; }
    bool merge()            { return m_merge; }
    string  tag()           { return m_tag; }
    IPCore::ReadFailedExc&  readExc()       { return m_rexc; }
    int numSourcesRead()   { return (m_session->sources().size() - m_nPreexistingSources); }
    int lastLoadedIndex()   { return m_lastLoadedSourceIndex; }

    bool timeToLoad();
    void nextPattern (string& pattern, Options::SourceArgs& sargs, bool& addLayer);
    bool nextPatternIsLayer (Options::SourceArgs& sargs)
                            { return (m_perSourceIndex != 0 && sargs.singleSource); }

  private:
    RvSession*                m_session;
    Options::SourceArgsVector m_sav;
    int                       m_savIndex;
    int                       m_tryCount;
    vector<SequenceNameList>  m_perSourcePatterns;
    int                       m_perSourceIndex;
    int                       m_loadCount;
    int                       m_totalCount;
    RvSession::CachingMode    m_cacheMode;
    bool                      m_doProcessOpts;
    bool                      m_merge;
    string                    m_tag;
    IPCore::ReadFailedExc     m_rexc;
    int                       m_nPreexistingSources;
    int                       m_lastLoadedSourceIndex;
};

LoadState::LoadState (
        RvSession* s,
        Options::SourceArgsVector &sav,
        bool noSeq,
        bool dpo,
        bool mg,
        string tg) :
    m_session(s),
    m_sav(sav),
    m_savIndex(0),
    m_perSourceIndex(0),
    m_lastLoadedSourceIndex(0),
    m_tryCount(0),
    m_loadCount(0),
    m_totalCount(0),
    m_cacheMode(RvSession::NeverCache),
    m_doProcessOpts(dpo),
    m_merge(mg),
    m_tag(tg),
    m_rexc("")
{
    m_nPreexistingSources = m_session->sources().size();

    for (int q = 0; q < m_sav.size(); q++)
    {
        const Options::SourceArgs& sargs = m_sav[q];
        const Options::Files& files = sargs.files;
        SequenceNameList inputPatterns, inputPatternsOrig = sequencesInFileList(files,
                 noSeq ? FalseSequencePredicate : GlobalExtensionPredicate);

        //
        //  Need to check for directories
        //

        for (int p = 0; p < inputPatternsOrig.size(); ++p)
        {
            string filename = inputPatternsOrig[p];
            bool isDir = checkForDirFiles(filename, noSeq, &inputPatterns);

            //
            //  If the directory contained no media, use informative but
            //  non-existent name and itself and let it explode later.
            //
            if (isDir && inputPatterns.size() == 0)
            {
                string fullname = filename + "/" + "emptyDirectory";
                inputPatterns.push_back(fullname);
            }
        }

        m_perSourcePatterns.push_back(inputPatterns);
        m_totalCount += inputPatterns.size();
    }
}

bool
LoadState::timeToLoad()
{
    if (!isLoading()) return false;

    bool ret = (m_loadCount == 0) ? (m_tryCount == 3) : (m_tryCount % 2 == 1);
    ++m_tryCount;

    return ret;
}

void
LoadState::nextPattern (string& pattern, Options::SourceArgs& sargs, bool& addLayer)
{
    sargs = m_sav[m_savIndex];
    addLayer = m_perSourceIndex != 0 && sargs.singleSource;
    m_lastLoadedSourceIndex = m_perSourceIndex;

    SequenceNameList& patterns = m_perSourcePatterns[m_savIndex];
    pattern = patterns[m_perSourceIndex++];

    if (m_perSourceIndex >= patterns.size())
    {
        ++m_savIndex;
        m_perSourceIndex = 0;
    }

    ++m_loadCount;
}

int
RvSession::loadCount()
{
    return m_readingGTO ? m_gtoSourceCount : (m_loadState ? m_loadState->loadCount() : 0);
}

int
RvSession::loadTotal()
{
    return m_readingGTO ? m_gtoSourceTotal : (m_loadState ? m_loadState->loadTotal() : 0);
}

void
RvSession::continueLoading()
{
    if (!m_loadState || !m_loadState->timeToLoad() || m_readingGTO) return;

    HOP_ZONE( HOP_ZONE_COLOR_10 );
    HOP_PROF_FUNC();

    //
    //  If we're caching at the moment, turn it off, but remember what mode
    //  it is so we can turn it back on later.
    //
    if (m_cacheMode != NeverCache)
    {
        m_loadState->storeCacheMode(m_cacheMode);
        setCaching(NeverCache);
    }
    //
    //  On the other hand, if the session is empty, remember the preferred
    //  caching mode and turn _that_ on later.
    //
    if (sources().empty())
    {
        Options& opts = Options::sharedOptions();

        if (opts.useCache) m_loadState->storeCacheMode(RvSession::GreedyCache);
        else if (opts.useLCache) m_loadState->storeCacheMode(RvSession::BufferCache);
    }

    string pattern;
    Options::SourceArgs sargs;
    bool addLayer = false;

    m_loadState->nextPattern (pattern, sargs, addLayer);
    const char *file = pattern.c_str();

    int oldFrame = currentFrame();

    SourceIPNode* node = 0;
    try
    {
        readSource(file, sargs, true, addLayer, m_loadState->tag(), m_loadState->merge());

        cout << "INFO: "
                << ((addLayer) ? "+ " : "")
                << file << endl;
    }
    catch (TwkExc::Exception& exc)
    {
        m_loadState->readExc() << exc.str()
                << " (" << m_loadState->lastLoadedIndex() << ") "
                << endl;
    }
    catch (std::exception& exc)
    {
        m_loadState->readExc() << exc.what()
                << " while reading "
                << file
                << " (" << m_loadState->lastLoadedIndex() << ") "
                << endl;
    }
    catch (...)
    {
        m_loadState->readExc() << "Unknown Exception while reading " << file
                << " (" << m_loadState->lastLoadedIndex() << ") "
                << endl;
    }

    setViewNode(viewNodeName());
    askForRedraw();

    if (m_loadState->isLoading())
    {
        setFrameInternal(oldFrame);
    }
    else
    {
        if (m_loadState->loadTotal() > 1)
        {
            rvgraph().addSourceEnd();
        }

        IPCore::ReadFailedExc rexc = m_loadState->readExc();
        int numRead = m_loadState->numSourcesRead();
        bool doProcessOpts = m_loadState->doProcessOpts();
        CachingMode mode = m_loadState->cacheMode();

        delete m_loadState;
        m_loadState = 0;

        if (mode != NeverCache) setCaching(mode);

        if (doProcessOpts) processOptionsAfterSourcesLoaded(rexc.str() != "");

        // Note: It was decided to trigger the after-progressive-proxy-loading
        // event even when progressive source loading is disabled
        userGenericEvent("after-progressive-proxy-loading", "");

        // We delay the after-progressive-loading event after all the sources are loaded
        if (!m_graph->isMediaLoading())
            userGenericEvent("after-progressive-loading", "");

        if (rexc.str() != "")
        {
            if (numRead == 0)
            {
                m_loadingError = true;
                m_loadingErrorString = rexc.str();
                TWK_THROW_STREAM(AllReadFailedExc, rexc.str());
            }
            else throw rexc;
        }
    }
}

void
RvSession::readUnorganizedFileList(const StringVector& infiles,
                                 bool doProcessOpts,
                                 bool merge,
                                 const string& tag)
{
    int rvFileCount = 0;
    for (int i = 0; i < infiles.size(); ++i) if (string(extension(infiles[i])) == "rv") ++rvFileCount;

    if (rvFileCount > 1) merge = true;

    //  PROGLOAD  this func should take tag for when called from mu
    Options::SourceArgsVector insources = Options::sharedOptions().parseSourceArgs(infiles);

    int nexisting = sources().size();

    //
    //  NOTE: the exception is created but will not be used unless an
    //  error occurs. Each round of reading results in another line of
    //  exception message being appended to the exception stream
    //

    IPCore::ReadFailedExc rexc("");
    m_loadingError = false;
    int group = 0;
    int item = 0;

    if (Options::sharedOptions().delaySessionLoading)
    {
        if (!insources.empty())
        {
            if (m_loadState) cerr << "ERROR: already have load state!\n" << endl;
            m_loadState = new LoadState(this, insources, m_noSequences, doProcessOpts, merge, tag);

            if (m_loadState->loadTotal() > 1)
            {
                rvgraph().addSourceBegin();
            }

            userGenericEvent("before-progressive-loading", "");

            // Note: It was decided to trigger the before-progressive-proxy-loading
            // event even when progressive source loading is disabled
            userGenericEvent("before-progressive-proxy-loading", "");
        }
        else if (doProcessOpts)
        {
            processOptionsAfterSourcesLoaded();
            setViewNode(viewNodeName(), true);
        }
    }
    else
    {
        for (int q=0; q < insources.size(); q++)
        {
            Options::SourceArgs& sargs = insources[q];
            const Options::Files& files = sargs.files;
            SequenceNameList inputPatterns = sequencesInFileList(files,
                    m_noSequences ? FalseSequencePredicate : GlobalExtensionPredicate);

            for (int i=0; i < inputPatterns.size(); i++)
            {
                const char *file = inputPatterns[i].c_str();

                try
                {
                    bool addLayer = i != 0 && sargs.singleSource;

                    readSource(file, sargs, true, addLayer, "", merge);

                    cout << "INFO: "
                        << ((addLayer) ? "+ " : "")
                        << inputPatterns[i] << endl;
                }
                catch (TwkExc::Exception& exc)
                {
                    rexc << exc.str() << " (" << i << ") " << endl;
                }
                catch (std::exception& exc)
                {
                    rexc << exc.what()
                        << " while reading "
                        << file
                        << " (" << i << ") "
                        << endl;
                }
                catch (...)
                {
                    rexc << "Unknown Exception while reading " << file
                        << " (" << i << ") "
                        << endl;
                }
            }
        }

        if (rexc.str() != "")
        {
            int nread = sources().size() - nexisting;

            if (nread == 0)
            {
                m_loadingError = true;
                m_loadingErrorString = rexc.str();
                TWK_THROW_STREAM(AllReadFailedExc, rexc.str());
            }
            else
            {
                throw rexc;
            }
        }

        if (doProcessOpts) processOptionsAfterSourcesLoaded();

        setViewNode(viewNodeName(), true);
    }
}

void
RvSession::processOptionsAfterSourcesLoaded(bool hadLoadError)
{
    Options& opts = Options::sharedOptions();

    //
    //  Some session parameters need to be set after the read
    //

    if (opts.scale != 1.0) setScaleOnAll(opts.scale);
    opts.scale = 1.0; // use it only once

    if (opts.dispLUT)
    {
        try
        {
            readLUT(opts.dispLUT, "#RVDisplayColor", true);
        }
        catch (std::exception& exc)
        {
            cerr << "ERROR: " << exc.what() << endl;
        }
    }

    if (opts.fileLUT)
    {
        try
        {
            readLUTOnAll(opts.fileLUT, "RVLinearize", true);
        }
        catch (std::exception& exc)
        {
            cerr << "ERROR: " << exc.what() << endl;
        }
    }

    if (opts.lookLUT)
    {
        try
        {
            readLUTOnAll(opts.lookLUT, "RVLookLUT", true);
        }
        catch (std::exception& exc)
        {
            cerr << "ERROR: " << exc.what() << endl;
        }
    }

    if (opts.view && *opts.view)
    {
        if (!setViewNode(opts.view))
        {
            cerr << "ERROR: view not found \"" << opts.view << "\"" << endl;
        }
    }

    if (opts.wipes)
    {
        StackGroupIPNode* stackNode = dynamic_cast<StackGroupIPNode *> (graph().viewNode());
        if (!stackNode)
        {
            setViewNode ("defaultStack");
            stackNode = (StackGroupIPNode *) graph().viewNode();
        }
        TwkContainer::IntProperty* ip =
            stackNode->createProperty<TwkContainer::IntProperty>("ui.wipes");
        ip->resize(1);
        ip->front() = 1;
        stackNode->propertyChanged(ip);

        StackIPNode* compNode = stackNode->stackNode();
        TwkContainer::StringProperty* sp =
            stackNode->createProperty<TwkContainer::StringProperty>("composite.type");
        sp->resize(1);
        sp->front() = "over";
        compNode->propertyChanged(sp);
    }

    //
    //  These can cause problems if they are allowed to start *before*
    //  we know the GL capabilities. The queryGL() function above may
    //  indicate that certain pixel formats are (not) preferred. If
    //  the caching threads get there first, they can make bad
    //  assumptions about the image format
    //

    if (opts.useCache) setCaching(RvSession::GreedyCache);
    else if (opts.useLCache) setCaching(RvSession::BufferCache);

    //setFrame(1);

    if (opts.play && !hadLoadError)
    {
        if (m_cacheMode == BufferCache) { play(); m_bufferWait = true; }
        else play();
    }

    string mu = initEval();
    //  cerr << "newRvSessionFromFiles mu '" << mu << "'" << endl;

    if (mu != "")
    {
        //  Empty stored mu code now, otherwise we can recurse if the code
        //  creates a session.

        setInitEval("");

        string output = TwkApp::muEval(muContext(),
                                       muProcess(),
                                       muModuleList(),
                                       mu.c_str(),
                                       "Command line eval");

        if ("string => \"noprint\"" != output)
        {
            cout << "INFO: eval returned: " << output << endl;
        }
    }

    // Send external events and their corresponding content.
    for (int i = 0; i < opts.sendEvents.size(); ++i)
    {
        const Options::SendExternalEvent &e = opts.sendEvents[i];
        userGenericEvent("external-" + e.name, e.content);
    }
}

void
RvSession::applySingleSourceArgs (Options::SourceArgs& sargs, SourceIPNode* node)
{
    if (!node) return;

    if (sargs.cutIn != numeric_limits<int>::max())
    {
        if (IntProperty *p = node->property<IntProperty>("cut.in"))
        {
            p->front() = sargs.cutIn;
            node->propertyChanged(p);
        }
    }

    if (sargs.cutOut != numeric_limits<int>::max())
    {
        if (IntProperty *p = node->property<IntProperty>("cut.out"))
        {
            p->front() = sargs.cutOut;
            node->propertyChanged(p);
        }
    }

    if (sargs.fps != 0)
    {
        if (FloatProperty *p = node->property<FloatProperty>("group.fps"))
        {
            p->front() = sargs.fps;
            node->propertyChanged(p);
        }

        //
        //  If we only have one source loaded, the default views have adopted
        //  the "wrong" fps (ie the one from before we set it with this
        //  option), so adjust them.  BUT, not if the user specified a session
        //  FPS on the command line, since the defaultViews should not adopt an
        //  FPS anyway.
        //

        Options& opts = Options::sharedOptions();

        if (sources().size() == 1 && opts.fps == 0.0)
        {
            setFPS(sargs.fps);
            const IPGraph::NodeMap& vnodes = graph().viewableNodes();

            for (IPGraph::NodeMap::const_iterator i = vnodes.begin(); i != vnodes.end(); ++i)
            {
                if (graph().isDefaultView((*i).first)) copySessionStateToNode((*i).second);
            }
        }
    }

    if (sargs.volume != -1)
    {
        if (FloatProperty *p = node->property<FloatProperty>("group.volume"))
        {
            p->front() = sargs.volume;
            node->propertyChanged(p);
        }
    }

    if (sargs.audioOffset != 0)
    {
        if (FloatProperty *p = node->property<FloatProperty>("group.audioOffset"))
        {
            p->front() = sargs.audioOffset;
            node->propertyChanged(p);
        }
    }

    if (sargs.rangeOffset != 0)
    {
        if (IntProperty *p = node->property<IntProperty>("group.rangeOffset"))
        {
            p->front() = sargs.rangeOffset;
            node->propertyChanged(p);
        }
    }

    if (sargs.rangeStart != numeric_limits<int>::max())
    {
        IntProperty* p = node->createProperty<IntProperty>("group.rangeStart");
        p->resize(1);
        node->newPropertyCreated(p);
        p->front() = sargs.rangeStart;
        node->propertyChanged(p);
    }

    if (sargs.noMovieAudio)
    {
        if (IntProperty *p = node->property<IntProperty>("group.noMovieAudio"))
        {
            p->front() = 1;
            node->propertyChanged(p);
        }
    }

    if (sargs.stereoRelativeOffset != 0.0)
    {
        if (SourceStereoIPNode* snode =
            dynamic_cast<SourceStereoIPNode*>(graph().findNodeAssociatedWith(node, "RVSourceStereo")))
        {
            if (FloatProperty *p = snode->property<FloatProperty>("stereo.relativeOffset"))
            {
                p->front() = sargs.stereoRelativeOffset;
                snode->propertyChanged(p);
            }
        }
    }

    if (sargs.stereoRightOffset != 0.0)
    {
        if (SourceStereoIPNode* snode =
            dynamic_cast<SourceStereoIPNode*>(graph().findNodeAssociatedWith(node, "RVSourceStereo")))
        {
            if (FloatProperty *p = snode->property<FloatProperty>("stereo.rightOffset"))
            {
                p->front() = sargs.stereoRightOffset;
                snode->propertyChanged(p);
            }
        }
    }

    if (sargs.pixelAspect != 0.0)
    {
        if (LensWarpIPNode* lnode =
            dynamic_cast<LensWarpIPNode*>(graph().findNodeAssociatedWith(node, "RVLensWarp")))
        {
            if (FloatProperty* p = lnode->property<FloatProperty>("warp.pixelAspectRatio"))
            {
                p->front() = sargs.pixelAspect;
                lnode->propertyChanged(p);
            }
        }
    }

    //
    //  XXX All the below changes do not generate events, and so cannot be
    //  remote synced at the moment
    //

    if (sargs.selectType != "" && sargs.selectName != "")
    {
        StringVector parts;
        algorithm::split(parts, sargs.selectName, is_any_of(string(",")));

        if (sargs.selectType == "view")
        {
            StringVector v(2);
            v[0] = "view";
            v[1] = parts[0];
            node->setProperty<StringProperty>("request.imageComponent", v);
        }
        else
        if (sargs.selectType == "layer")
        {
            StringVector v(3);
            v[0] = "layer";
            v[1] = (parts.size() > 1) ? parts[0] : "";
            v[2] = (parts.size() > 1) ? parts[1] : sargs.selectName;
            node->setProperty<StringProperty>("request.imageComponent", v);
        }
        else
        if (sargs.selectType == "channel")
        {
            StringVector v(4);
            v[0] = "channel";
            v[1] = (parts.size() > 2) ? parts[0] : "";
            v[2] = (parts.size() > 2) ? parts[1] : "";
            v[3] = (parts.size() > 2) ? parts[2] : sargs.selectName;
            node->setProperty<StringProperty>("request.imageComponent", v);
        }
    }

    if (sargs.cmap != "")
    {
        if (ChannelMapIPNode* cmapnode =
            dynamic_cast<ChannelMapIPNode*>(graph().findNodeAssociatedWith(node, "RVChannelMap")))
        {
            StringVector chans;
            stl_ext::tokenize(chans, sargs.cmap, ",");
            cmapnode->setChannelMap(chans);
            if ( (chans.size() > 1) || ((sargs.selectType == "") && (sargs.selectName == "")) )
            {
                node->setProperty<IntProperty>("request.readAllChannels", 1);
            }
        }
    }

    if (sargs.fcdl != "")
    {
        if (PipelineGroupIPNode* pipeGroup =
            dynamic_cast<PipelineGroupIPNode*>(node->group()->memberByType("RVLinearizePipelineGroup")))
        {
            if (FileSpaceLinearizeIPNode* cdlnode =
                dynamic_cast<FileSpaceLinearizeIPNode*>(pipeGroup->memberByType("RVLinearize")))
            {
                readCDL(sargs.fcdl, cdlnode->name(), true);
            }
        }
    }

    if (sargs.lcdl != "")
    {
        if (PipelineGroupIPNode* pipeGroup =
            dynamic_cast<PipelineGroupIPNode*>(node->group()->memberByType("RVColorPipelineGroup")))
        {
            if (ColorIPNode* cdlnode =
                dynamic_cast<ColorIPNode*>(pipeGroup->memberByType("RVColor")))
            {
                readCDL(sargs.lcdl, cdlnode->name(), true);
            }
        }
    }

    if (sargs.flut != "")
    {
        if (PipelineGroupIPNode* pipeGroup =
            dynamic_cast<PipelineGroupIPNode*>(node->group()->memberByType("RVLinearizePipelineGroup")))
        {
            if (LUTIPNode* lutnode =
                dynamic_cast<LUTIPNode*>(pipeGroup->memberByType("RVLinearize")))
            {
                readLUT(sargs.flut, lutnode->name(), true);
            }
        }
    }

    if (sargs.pclut != "")
    {
        if (LUTIPNode* lutnode =
            dynamic_cast<LUTIPNode*>(graph().findNodeAssociatedWith(node, "RVCacheLUT")))
        {
            readLUT(sargs.pclut, lutnode->name(), true);
        }
    }

    if (sargs.llut != "")
    {
        if (PipelineGroupIPNode* pipeGroup =
            dynamic_cast<PipelineGroupIPNode*>(node->group()->memberByType("RVLookPipelineGroup")))
        {
            if (LUTIPNode* lutnode =
                dynamic_cast<LUTIPNode*>(pipeGroup->memberByType("RVLookLUT")))
            {
                readLUT(sargs.llut, lutnode->name(), true);
            }
        }
    }

    if (sargs.hascrop || sargs.hasuncrop)
    {
        if (FormatIPNode* fnode  =
            dynamic_cast<FormatIPNode*>(graph().findNodeAssociatedWith(node, "RVFormat")))
        {
            if (sargs.hascrop) fnode->setCrop(int(sargs.crop[0]),
                                                int(sargs.crop[1]),
                                                int(sargs.crop[2]),
                                                int(sargs.crop[3]));

            if (sargs.hasuncrop) fnode->setUncrop(int(sargs.uncrop[0]),
                                                    int(sargs.uncrop[1]),
                                                    int(sargs.uncrop[2]),
                                                    int(sargs.uncrop[3]));
        }
    }
}

void
RvSession::read(const string& filename, const ReadRequest& request)
{
    const bool   addContents = request.optionValue<bool>("append", false);
    const string tag         = request.optionValue<string>("tag", "");

    Options::SourceArgs sargs;
    readSource(filename, sargs, addContents, false, tag);
}

void
RvSession::readSourceHelper(StringVector files,
                            Options::SourceArgs& sargs,
                            bool addContents,
                            bool addToExistingSource)
{
    if (!addContents) clear();

    {
    RvGraph::FastAddSourceGuard fastAddSourceGuard(rvgraph());

    if (addToExistingSource) addToSourceHelper("", files, sargs);
    else for (int i=0; i < files.size(); i++) addSource(files[i], sargs);
    }

    // We need to update the frame range here because the FastAddSourceGuard set
    // the inputs of the SequenceIPNode after the RvSession::newMediaLoaded is
    // called for the last source.
    updateRange();
}

void
RvSession::readSource(const std::string& infilename,
                      Options::SourceArgs& sargs,
                      bool addContents,
                      bool addToExistingSource,
                      const string& tag,
                      bool merge)
{
    HOP_PROF_DYN_NAME(
        std::string(std::string("RvSession::readSource : ") +
                    infilename).c_str());

    string filename = pathConform(IPCore::Application::mapFromVar(infilename));
    string eventContents = filename + ";;" + tag;
    string newfilename = userGenericEvent("incoming-source-path", eventContents);

    if (newfilename != "")
    {
        filename = newfilename;
    }

    SequenceNameList files;
    if (checkForDirFiles(filename, m_noSequences, &files))
    {
        readSourceHelper(files, sargs, addContents, addToExistingSource);
    }
    else
    {
        //
        //  If its a single file, add it. If its an RV file then read
        //  it and reconstruct the session from that.
        //

        string ext = extension(filename);

        if (ext == "rv")
        {
            readGTO(filename.c_str(), merge, sargs);
        }
        else if (ext == "rvedl")
        {
            readEDL(filename.c_str());
        }
        else if (isStereoSequence(filename))
        {
            StringVector movies;
            checkForStereoPaths(filename, movies);

            if (movies.size() == 2)
            {
                if (!addContents) clear();

                if (addToExistingSource) addToSourceHelper("", movies, sargs);
                else addSource(movies, sargs);
            }
            else TWK_THROW_STREAM(IPCore::ReadFailedExc, "Cannot access stereo media " << filename);
        }
        else
        {
            StringVector movies;
            movies.push_back(filename);
            readSourceHelper(movies, sargs, addContents, addToExistingSource);
        }
    }
}


void
RvSession::addToSource(const string& srcNode,
                       const string& infilename,
                       const string& tag)
{
    string filename = pathConform(IPCore::Application::mapFromVar(infilename));
    string eventContents = filename + ";;" + tag;
    string newfilename = userGenericEvent("incoming-source-path", eventContents);

    if (newfilename != "")
    {
        filename = newfilename;
    }

    //
    //  If it's a directory, and there's one sequence in the
    //  directory, try to add that.
    //

    SequenceNameList files;
    if (checkForDirFiles(filename, false, &files))
    {
        if (files.size() == 1)
        {
            filename = files[0];
        }
        else if (files.size() == 0)
        {
            TWK_THROW_STREAM(IPCore::ReadFailedExc, "Direcory contains no media");
        }
        else
        {
            TWK_THROW_STREAM(IPCore::ReadFailedExc, "Direcory contains too many sequences");
        }
    }

    StringVector movies;
    checkForStereoPaths(filename, movies);
    Options::SourceArgs sargs;
    if (movies.size()) addToSourceHelper(srcNode, movies, sargs);
}

SourceIPNode*
RvSession::addToSourceHelper(const string& srcNode, StringVector movies,
    Options::SourceArgs& sargs)
{
    SourceIPNode* node = 0;
    if (movies.empty()) return 0;

    if (srcNode == "")
    {
        const RvGraph::Sources& sources = rvgraph().imageSources();
        if (sources.empty()) return 0;
        //
        //  Want to add to the source we just added, so that's the
        //  last one in this list.
        //
        node = sources[sources.size() - 1];
    }
    else node = sourceNode(srcNode);

    if (!node) return 0;

    //
    //  Store input parameters
    //

    if (FileSourceIPNode* fsipn = dynamic_cast<FileSourceIPNode*>(node))
    {
        fsipn->storeInputParameters(sargs.inparams);
    }

    //
    //  Set the media on the source
    //

    StringProperty* movie = node->property<StringProperty>("media.movie");
    for (int i = 0; i < movies.size(); ++i) movie->push_back(movies[i]);
    node->propertyChanged(movie);

    //
    //  Apply any source args
    //

    applySingleSourceArgs(sargs, node);

    //
    //  Send the user an event
    //

    ostringstream str;
    str << node->name() << ";;RVSource" << ";;" << movies[0];
    userGenericEvent("source-modified", str.str());

    return node;
}

void
RvSession::buildFileList(const StringVector& files, string tag, StringVector& collected)
{
    for (int i = 0; i < files.size(); i++)
    {
        string filename = pathConform(IPCore::Application::mapFromVar(files[i]));
        string eventContents = filename + ";;" + tag;
        string newfilename = userGenericEvent("incoming-source-path", eventContents);

        if (newfilename != "") filename = newfilename;

        checkForStereoPaths(filename, collected);
    }
}

SourceIPNode*
RvSession::addSourceWithTag(const StringVector& files, string tag, string nodeName, string mediaRepName)
{
    Options::SourceArgs sargs;
    if (!files.empty())
    {
        StringVector sourceArgs;
        sourceArgs.push_back("[");
        for (int i=0; i<files.size(); i++)
        {
            sourceArgs.push_back(files[i]);
        }
        sourceArgs.push_back("]");
        Options::SourceArgsVector srcArgsVector = Options::sharedOptions().parseSourceArgs(sourceArgs);
        sargs = srcArgsVector[0];
    }

    StringVector movies;
    buildFileList(sargs.files, tag, movies);
    if (movies.empty()) TWK_THROW_EXC_STREAM("ERROR: No files for reading" << endl);

    auto result = addSource(movies, sargs, nodeName, "RVFileSource", mediaRepName);

    // We need to update the frame range here because the FastAddSourceGuard set
    // the inputs of the SequenceIPNode after the RvSession::newMediaLoaded is
    // called for the last source.
    updateRange();

    return result;
}

SourceIPNode*
RvSession::addSource(string filename, Options::SourceArgs& sargs, string nodeName)
{
    StringVector movies(1);
    movies.front() = filename;
    return addSource(movies, sargs, nodeName);
}

SourceIPNode*
RvSession::addSource(
    const StringVector& movies,
    Options::SourceArgs& sargs,
    string nodeName,
    string nodeType,
    string mediaRepName,
    string mediaRepSource)
{
    //
    // Determine the source media rep name and source to use if any
    //

    if (mediaRepName=="" && sargs.mediaRepName!="")
    {
        mediaRepName = sargs.mediaRepName;
    }
    if (mediaRepSource=="" && sargs.mediaRepSource!="")
    {
        mediaRepSource = sargs.mediaRepSource;
    }
    SourceIPNode* mediaRepLinkedSrcNode = nullptr;
    if (mediaRepSource!="")
    {
        if (mediaRepSource == "last")
        {
            const RvGraph::Sources& sources = rvgraph().imageSources();
            if (sources.empty()) return 0;
            //
            //  Want to add to the source we just added, so that's the
            //  last one in this list.
            //
            mediaRepLinkedSrcNode = sources[sources.size() - 1];
        }
        else
        {
            mediaRepLinkedSrcNode = sourceNode(mediaRepSource);
        }
        if (!mediaRepLinkedSrcNode) TWK_THROW_EXC_STREAM("ERROR: Could not find specified source node" << endl);

        // Make sure the media rep name does not already exists
        if (mediaRepLinkedSrcNode)
        {
            StringVector mediaReps;
            sourceMediaReps(mediaRepLinkedSrcNode->name(), mediaReps);
            if (any_of(mediaReps.begin(), mediaReps.end(), [&](const string& elem) {return elem == mediaRepName;}))
            {
                TWK_THROW_EXC_STREAM("ERROR: Source media representation name already exists: " << mediaRepName << endl);
            }
        }
    }

    SourceIPNode* source = rvgraph().addSource(nodeType, nodeName, mediaRepName, mediaRepLinkedSrcNode);

    //
    //  Set the media on the source
    //

    source->mediaChangedSignal().connect(boost::bind(&RvSession::newMediaLoaded, this, source));
    StringProperty* movie = source->property<StringProperty>("media.movie");
    for (int i = 0; i < movies.size(); ++i) movie->push_back (movies[i]);

    //
    //  Store input parameters
    //
    //  Note that the FileSourceIPNode::storeInputParameters() must be called
    //  before the 'source->propertyChanged(movie)' below otherwise those input
    //  parameters might be missed entirely by FileSourceIPNode::openMovieTask()
    //  (which is executed in a worker thread) or reset cookies which might lead
    //  to file access errors such as :
    //  'Could not locate "https://.../mp4". Relocate source to fix'
    //

    if (FileSourceIPNode* fsipn = dynamic_cast<FileSourceIPNode*>(source))
    {
        fsipn->storeInputParameters(sargs.inparams);
    }

    // Set the proxy parameters based on the per-source command-line flags if
    // any were specified. These are not essential per say but really improves
    // the user experience in progressive source loading mode.
    // For example, the following SG fields are provided via these per-source
    // command-line flags (+in,+out,+fps) when a user on a ShotGrid site selects
    // 'Play in RV' or 'Play in Screening Room for RV':
    // sg_first_frame/sg_last_frame/sg_uploaded_movie_frame_rate
    if (hasCutPoints(sargs))
    {
        source->declareProperty<Vec2iProperty>("proxy.range", Vec2i(sargs.cutIn, sargs.cutOut));
    }
    if (sargs.fps!=0.0)
    {
        source->declareProperty<FloatProperty>("proxy.fps", sargs.fps);
    }

    source->propertyChanged(movie);


    //
    //  Send the user an event
    //

    userGenericEvent("new-node", source->name());

    //
    //  Handle new-source event
    //

    ostringstream nscontents;
    string media = (movies.size() > 0) ? movies[0] : "";
    nscontents << source->name() << ";;RVSource;;" << media;
    userGenericEvent("new-source", nscontents.str());

    //
    //  Check and see if stereo needs to be turned on
    //

    IPGraph::NodeVector nodes;
    graph().findNodesByTypeName(nodes, "RVDisplayStereo");

    if (!nodes.empty())
    {
        if (DisplayStereoIPNode* stereo = dynamic_cast<DisplayStereoIPNode*>(nodes.front()))
        {
            if (stereo->stereoType() == "hardware") send(stereoHardwareOnMessage());
            else send(stereoHardwareOffMessage());
        }
    }

    //
    //  Apply any source args
    //

    applySingleSourceArgs(sargs, source);

    return source;
}

SourceIPNode*
RvSession::addImageSource(const string& mediaName, const MovieInfo& info)
{
    StringVector movies(1);
    movies.front() = mediaName;
    Options::SourceArgs sargs;
    SourceIPNode* sipn = addSource(movies, sargs, "", "RVImageSource");
    ImageSourceIPNode* source = dynamic_cast<ImageSourceIPNode*>(sipn);
    source->set(mediaName, info);

    return source;
}

void
RvSession::newMediaLoaded(SourceIPNode* node)
{
    if (SourceGroupIPNode* group = dynamic_cast<SourceGroupIPNode*>(node->group()))
    {
        //
        //  If there isn't a name already give it one
        //

        if (!(group->property<StringProperty>("ui.name"))) group->setUINameFromMedia();

        const RvGraph::Sources& sources = rvgraph().imageSources();
        Options& opts = Options::sharedOptions();
        if (opts.playMode == 0 && sources.size() == 1 && node->hasAudio())
        {
            m_realtime = true;
        }

        //
        // Notify the source node to avoid a dead lock
        //
        node->onNewMediaComplete();

        //
        //  Handle source-group-complete event
        //

        ostringstream sgccontents;
        string action = node->updateAction();
        sgccontents << group->name() << ";;" << action;

        if (Options::sharedOptions().progressiveSourceLoading)
        {
            if (action != "modified")
                userGenericEvent("source-group-proxy-complete", sgccontents.str());
            else
                userGenericEvent("source-group-complete", sgccontents.str());
        }
        else
        {
            // Note: It was decided to trigger the source-group-proxy-complete
            // event even when progressive source loading is disabled
            userGenericEvent("source-group-proxy-complete", sgccontents.str());

            userGenericEvent("source-group-complete", sgccontents.str());
        }

        if ( (sources.size() == 1 && action == "new") || m_conductorSource == node)
        {
            updateRange();
            m_inc     = graph().root()->imageRangeInfo().inc;

            if (m_conductorSource == nullptr)
            {
                // We want to change the playhead only during the first load
                m_frame = rangeStart();
            }

            m_fps     = sources.front()->imageRangeInfo().fps;
            m_conductorSource = node;
        }

        if (m_gtoSourceCount == m_gtoSourceTotal)
        {
            updateRange();
            if (graph().hasAudio()) ensureAudioRenderer();
        }
    }
}

vector<FileSourceIPNode*>
RvSession::findSourcesByMedia(const string& filename, const string& node)
{
    vector<FileSourceIPNode*> sources;

    if (node == "")
    {
        NodeVector nodes;
        graph().findNodesByTypeName(nodes, "RVFileSource");

        for (NodeVector::iterator i = nodes.begin(); i != nodes.end(); ++i)
        {
            if (FileSourceIPNode* n = dynamic_cast<FileSourceIPNode*>(*i))
            {
                if (n->filename() == filename) sources.push_back(n);
            }
        }
    }
    else
    {
        if (IPNode* ipn = graph().findNode(node))
        {
            if (FileSourceIPNode* n = dynamic_cast<FileSourceIPNode*>(ipn))
            {
                if (n->filename() == filename) sources.push_back(n);
            }
        }
    }

    return sources;
}

void
RvSession::relocateSource(const string& oldFilename,
                        const string& newFilename,
                        const string& srcNode)
{
    string newF = pathConform(IPCore::Application::mapFromVar(newFilename));

    vector<FileSourceIPNode*> sources = findSourcesByMedia(oldFilename, srcNode);

    for (int i = 0; i < sources.size(); ++i)
    {
        FileSourceIPNode* source = sources[i];

        ostringstream mrcontents;
        mrcontents << source->name() << ";;" << oldFilename << ";;" << newF;
        userGenericEvent("media-relocated", mrcontents.str());

        StringProperty* movie = source->property<StringProperty>("media.movie");
        movie->front() = newFilename;
        IntProperty* mediaActive = source->property<IntProperty>("media.active");
        mediaActive->front() = 1;
        source->propertyChanged(movie);

        ostringstream nscontents;
        nscontents << source->name() << ";;RVSource;;" << newFilename;
        userGenericEvent("new-source", nscontents.str());

    }
}

void
RvSession::setSourceMedia(const std::string& srcNode,
                          const StringVector& files,
                          const std::string& tag)
{

    if (IPNode* node = graph().findNode(srcNode))
    {
        StringVector mediaFiles;
        buildFileList(files, tag, mediaFiles);
        if (!mediaFiles.empty())
        {
            if (FileSourceIPNode* fsipn = dynamic_cast<FileSourceIPNode*>(node))
            {
                const bool isProgressiveLoadingEnabled = fsipn->hasProgressiveSourceLoading();
                fsipn->setProgressiveSourceLoading(false);

                fsipn->clearMedia();
                StringProperty* movie = fsipn->property<StringProperty>("media.movie");
                movie->resize(0);
                for (int i = 0; i < mediaFiles.size(); ++i) movie->push_back(mediaFiles[i]);
                IntProperty* mediaActive = fsipn->property<IntProperty>("media.active");
                mediaActive->front() = 1;
                fsipn->propertyChanged(movie);

                ostringstream nscontents;
                string media = (mediaFiles.size() > 0) ? mediaFiles[0] : "";
                nscontents << srcNode << ";;RVSource;;" << media;
                userGenericEvent("new-source", nscontents.str());

                ostringstream smscontents;
                smscontents << srcNode << ";;" << tag;
                userGenericEvent("source-media-set", smscontents.str());

                fsipn->setProgressiveSourceLoading(isProgressiveLoadingEnabled);
           }
            else
            {
                TWK_THROW_EXC_STREAM("Bad node type " << srcNode);
            }
        }
    }
    else
    {
        TWK_THROW_STREAM(IPCore::ReadFailedExc, "Can't find node " << srcNode);
    }

}

SourceIPNode*
RvSession::addSourceMediaRep(const std::string& srcNodeName,
                             const std::string& mediaRepName,
                             const StringVector& mediaRepPathsAndOptionsArg,
                             const std::string& tag)
{
    Options::SourceArgs sargs;
    if (!mediaRepPathsAndOptionsArg.empty())
    {
        StringVector sourceArgs;
        sourceArgs.push_back("[");
        for (int i=0; i<mediaRepPathsAndOptionsArg.size(); i++)
        {
            sourceArgs.push_back(mediaRepPathsAndOptionsArg[i]);
        }
        sourceArgs.push_back("]");
        Options::SourceArgsVector srcArgsVector = Options::sharedOptions().parseSourceArgs(sourceArgs);
        sargs = srcArgsVector[0];
    }

    StringVector mediaRepPaths;
    buildFileList(sargs.files, tag, mediaRepPaths);
    if (mediaRepPaths.empty()) TWK_THROW_EXC_STREAM("ERROR: No files for reading" << endl);

    auto result = addSource(
        mediaRepPaths,
        sargs,
        "" /*nodeName*/,
        "RVFileSource" /*nodeType*/,
        mediaRepName,
        srcNodeName);

    return result;
}

// Returns true if the switch node as parameter belongs to an RVSwitchGroup
static bool isGroupedSwitchNode( SwitchIPNode const * const switchNode)
{
    return switchNode && switchNode->group()!=nullptr &&
           switchNode->group()->protocol() == "RVSwitchGroup";
}

// Find and return the Switch node(s) associated with the specified source node name if any
vector<SwitchIPNode*>
RvSession::findSwitchIPNodes(const string& srcNodeOrSwitchNodeName)
{
    vector<SwitchIPNode*> switchNodes;

    NodeVector nodes;
    if (srcNodeOrSwitchNodeName == "")
    {
        findCurrentNodesByTypeName(nodes, "RVSwitch");
    }
    else if (srcNodeOrSwitchNodeName == "all")
    {
        graph().findNodesByTypeName(nodes, "RVSwitch");
    }
    else if (IPNode* srcNode = graph().findNode(srcNodeOrSwitchNodeName))
    {
        SwitchIPNode* switchNode = dynamic_cast<SwitchIPNode*>(srcNode);
        if (!switchNode && srcNode->group())
        {
            switchNode = dynamic_cast<SwitchIPNode*>(
                graph().findNodeAssociatedWith(srcNode->group(), "RVSwitch"));
        }

        // Make sure that the switch node (if any) is part of a switch group
        if (switchNode && isGroupedSwitchNode(switchNode))
        {
            switchNodes.push_back(switchNode);
        }
    }

    for (NodeVector::iterator i = nodes.begin(); i != nodes.end(); ++i)
    {
        // Make sure that the switch node (if any) is part of a switch group
        SwitchIPNode* switchNode = dynamic_cast<SwitchIPNode*>(*i);
        if (switchNode && isGroupedSwitchNode(switchNode))
        {
            switchNodes.push_back(switchNode);
        }
    }

    return switchNodes;
}

// Find and return the source group node corresponding to the requested source
// media representation if any
static SourceGroupIPNode*
sourceGroupFromSwitchInput(IPNode* switchInput)
{
    if (AdaptorIPNode* adaptor = dynamic_cast<AdaptorIPNode*>(switchInput))
    {
        switchInput = adaptor->groupInputNode();
    }
    if (SourceGroupIPNode* srcGroup = dynamic_cast<SourceGroupIPNode*>(switchInput))
    {
        return srcGroup;
    }

    return nullptr;
}

// Find and return the source node corresponding to the requested source media
// representation if any
static SourceIPNode*
sourceFromSwitchInput(IPNode* switchInput)
{
    if (SourceGroupIPNode* srcGroup = sourceGroupFromSwitchInput(switchInput))
    {
        if (FileSourceIPNode* srcNode =
            dynamic_cast<FileSourceIPNode*>(srcGroup->memberByType("RVFileSource")))
        {
            return srcNode;
        }
        if (ImageSourceIPNode* srcNode =
            dynamic_cast<ImageSourceIPNode*>(srcGroup->memberByType("RVImageSource")))
        {
            return srcNode;
        }
    }

    return nullptr;
}

// Finds and returns the source group corresponding to the requested source
// media representation if any
static SourceGroupIPNode*
findSourceGroupMediaRep(SwitchIPNode* switchNode, const std::string& mediaRepName, SourceIPNode*& out_srcNode)
{
    for (int i = 0; i < switchNode->inputs().size(); ++i)
    {
        if (SourceIPNode* srcNode = sourceFromSwitchInput(switchNode->inputs()[i]))
        {
            if (srcNode->mediaRepName() == mediaRepName)
            {
                out_srcNode = srcNode;
                return dynamic_cast<SourceGroupIPNode*>(srcNode->group());
            }
        }
    }

    return nullptr;
}

// Returns the currently selected source media representation's name
std::string RvSession::sourceMediaRep(
    const string& srcNodeOrSwitchNodeName
)
{
    // Find the Switch node associated with the specified switch or source node if any
    SwitchIPNode* switchNode = this->switchNode(srcNodeOrSwitchNodeName);
    if (!switchNode)
    {
        std::vector<SwitchIPNode*> switchNodes = findSwitchIPNodes(srcNodeOrSwitchNodeName);
        if (!switchNodes.empty())
        {
            switchNode = switchNodes[0];
        }
    }

    // If there is no associated switch nodes then it means there is only a
    // single source media representation
    SourceIPNode* srcNode = nullptr;
    if (!switchNode)
    {
        srcNode = sourceNode(srcNodeOrSwitchNodeName);
    }
    else
    {
        srcNode = sourceFromSwitchInput(switchNode->activeInput());
    }

    return srcNode ? srcNode->mediaRepName() : "";
}

// Finds and returns the Source Media Representation names available
void RvSession::sourceMediaReps(
    const string& srcNodeOrSwitchNodeName,
    StringVector& sourceMediaReps,
    StringVector* sourceNodes /* = nullptr */
)
{
    sourceMediaReps.clear();
    if (sourceNodes) sourceNodes->clear();

    // Find the Switch node associated with the specified switch or source node if any
    SwitchIPNode* switchNode = this->switchNode(srcNodeOrSwitchNodeName);
    if (!switchNode)
    {
        std::vector<SwitchIPNode*> switchNodes = findSwitchIPNodes(srcNodeOrSwitchNodeName);
        if (!switchNodes.empty())
        {
            switchNode = switchNodes[0];
        }
    }

    // If there is no associated switch nodes then it means there is only a
    // single source media representation
    if (!switchNode)
    {
        SourceIPNode* srcNode = sourceNode(srcNodeOrSwitchNodeName);
        if (srcNode)
        {
            sourceMediaReps.push_back(srcNode->mediaRepName());
            if (sourceNodes) sourceNodes->push_back(srcNode->name());
        }
        return;
    }

    for (size_t i = 0; i < switchNode->inputs().size(); ++i)
    {
        if (SourceIPNode* srcNode = sourceFromSwitchInput(switchNode->inputs()[i]))
        {
            sourceMediaReps.push_back(srcNode->mediaRepName());
            if (sourceNodes) sourceNodes->push_back(srcNode->name());
        }
    }
}

// Returns the source media rep switch node associated with the source if any
std::string RvSession::sourceMediaRepSwitchNode(
    const string& srcNodeName
)
{
    // Find the Switch node(s) associated with the specified source node if any
    std::vector<SwitchIPNode*> switchNodes = findSwitchIPNodes(srcNodeName);

    return switchNodes.empty() ? "" : switchNodes[0]->name();
}

// Returns the source media rep's switch node's first input associated with the source if any
std::string RvSession::sourceMediaRepSourceNode(
    const string& srcNodeName
)
{
    // Find the Switch node(s) associated with the specified source node if any
    std::vector<SwitchIPNode*> switchNodes = findSwitchIPNodes(srcNodeName);

    // If there is no associated switch nodes then it means there is only a
    // single source media representation
    if (!switchNodes.empty() && switchNodes[0]->inputs().size()>=1)
    {
        SourceIPNode* srcNode = sourceFromSwitchInput(switchNodes[0]->inputs()[0]);
        if (srcNode)
        {
            return srcNode->name();
        }
    }

    return "";
}

void
RvSession::setActiveSourceMediaRep(const std::string& srcNodeOrSwitchNodeName,
                                   const std::string& mediaRepName,
                                   const std::string& tag)
{
    // Find the Switch node associated with the specified switch or source node if any
    vector<SwitchIPNode*> switchNodes;
    SwitchIPNode* switchNode = this->switchNode(srcNodeOrSwitchNodeName);
    if (switchNode)
    {
        switchNodes.push_back(switchNode);
    }
    else
    {
        switchNodes = findSwitchIPNodes(srcNodeOrSwitchNodeName);
    }

    // Nothing to do if there isn't any
    if (switchNodes.empty())
    {
        if (srcNodeOrSwitchNodeName!="" && srcNodeOrSwitchNodeName!="all")
        {
            // Log an error but only if the requested media rep to activate is
            // not the currently active one
            SourceIPNode* srcNode = sourceNode(srcNodeOrSwitchNodeName);
            if (!srcNode || srcNode->mediaRepName()!=mediaRepName)
            {
                TWK_THROW_STREAM(IPCore::ReadFailedExc, "Can't find switch node associated with source node "
                    << srcNodeOrSwitchNodeName << " to select the '" << mediaRepName << "' media representation.");
            }
        }

        return;
    }

    // Swap media for all the specified switch node(s)
    for( const auto& switchNode : switchNodes )
    {
        if (SourceIPNode* activeSrcNode = sourceFromSwitchInput(switchNode->activeInput()))
        {
            // Select the requested media representation if this not the currently selected one.
            if (activeSrcNode->mediaRepName() != mediaRepName)
            {
                SourceIPNode* newSrcNode = nullptr;
                if (SourceGroupIPNode* newSrcGroup = findSourceGroupMediaRep(switchNode, mediaRepName, newSrcNode))
                {
                    if (StringProperty *switchInputProp = switchNode->property<StringProperty>("output.input"))
                    {
                        if ((*switchInputProp)[0]!=newSrcGroup->name())
                        {
                            // Select the new source group
                            switchNode->propertyWillChange(switchInputProp);
                            (*switchInputProp)[0]=newSrcGroup->name();
                            switchNode->propertyChanged(switchInputProp);

                            ostringstream eventContents;
                            eventContents << newSrcNode->name() << ";;" << newSrcNode->mediaRepName() << ";;"
                                            << activeSrcNode->name() << ";;" << activeSrcNode->mediaRepName();
                            userGenericEvent("source-media-rep-activated", eventContents.str());
                        }
                    }
                }
                else
                {
                    if (srcNodeOrSwitchNodeName!="" && srcNodeOrSwitchNodeName!="all")
                    {
                        TWK_THROW_STREAM(IPCore::ReadFailedExc,
                            "Can't find source group associated with "
                            << srcNodeOrSwitchNodeName << " to select the '"
                            << mediaRepName << "' media representation.");
                    }
                    else
                    {
                        // Otherwise, make sure the active source node's media is active
                        // Note that this will trigger the multiple source media fallback
                        // mechanism if the current source cannot be activated (ie if it
                        // is not available)
                        activeSrcNode->setMediaActive(true);
                    }
                }
            }
            // Otherwise make sure the active source node's media is active
            else
            {
                activeSrcNode->setMediaActive(true);
            }
        }
    }
}

void
RvSession::write(const string& infilename, const WriteRequest& request)
{
    const string filename    = pathConform(infilename);
    const string ext         = extension(filename);
    const bool   writeAsCopy = request.optionValue<bool>("writeAsCopy", false);

    if (ext == "rv")
    {
        //
        //  Make a new WriteRequest based on what we get that has
        //  internal options set to write an rv style session file
        //

        WriteRequest newRequest = request;

        //
        //  If there's an ImageSourceIPNode in the session than force
        //  the use of compressed binary output
        //

        const IPGraph::NodeMap& nodeMap = graph().nodeMap();
        bool bigFile = false;

        for (NodeMap::const_iterator i = nodeMap.begin();
             i != nodeMap.end();
             ++i)
        {
            IPNode* n = i->second;
            if (dynamic_cast<ImageSourceIPNode*>(n)) bigFile = true;
        }

        //
        //  Set the RV specific write options
        //

        newRequest.setOption("writerID", string("RV"));
        newRequest.setOption("sessionProtocol", string("RVSession"));
        newRequest.setOption("sessionProtocolVersion", 4);
        newRequest.setOption("sessionName", string("rv"));
        newRequest.setOption("version", 2);
        if (bigFile) newRequest.setOption("compressed", true);

        //
        //  Output pre events
        //

        if (writeAsCopy) userGenericEvent("before-session-write-copy", filename);
        else userGenericEvent("before-session-write", filename);

        //
        //  Call the generic session writer
        //

        Session::write(filename, newRequest);
        setFileName(filename.c_str());

        //
        //  post write events
        //

        if (writeAsCopy) userGenericEvent("after-session-write-copy", filename);
        else userGenericEvent("after-session-write", filename);
    }
    else
    {
        Session::write(infilename, request);
    }
}

struct FrameBufferFinder
{
    FrameBufferFinder() : fbImage(0) {}
    const IPImage* fbImage;

    void operator () (IPImage* i)
    {
        if (fbImage) return;
        if (i->destination != IPImage::OutputTexture)
        {
            fbImage = i;
        }
    }
};

const TwkFB::FrameBuffer*
RvSession::currentFB() const
{
    FrameBufferFinder finder;
    foreach_ip (m_displayImage, finder);
    return finder.fbImage ? finder.fbImage->fb : 0;
}

SourceIPNode*
RvSession::sourceNode(const std::string& sourceName) const
{
    StringVector tokens;
    stl_ext::tokenize(tokens, sourceName, "./");

    if (!tokens.empty())
    {
        if (IPNode* node = graph().findNode(tokens.front()))
        {
            if (SourceIPNode* source = dynamic_cast<SourceIPNode*>(node))
            {
                return source;
            }
        }
    }

    return 0;
}

SwitchIPNode*
RvSession::switchNode(const std::string& switchNodeName) const
{
    StringVector tokens;
    stl_ext::tokenize(tokens, switchNodeName, "./");

    if (!tokens.empty())
    {
        if (IPNode* node = graph().findNode(tokens.front()))
        {
            if (SwitchIPNode* switchNode = dynamic_cast<SwitchIPNode*>(node))
            {
                return switchNode;
            }
        }
    }

    return 0;
}

const TwkFB::FrameBuffer*
RvSession::currentFB(const std::string& sourceName) const
{
    if (m_displayImage)
    {
        StringVector tokens;
        stl_ext::tokenize(tokens, sourceName, "/");

        for (size_t i = 0; i < m_displayFBStatus.size(); i++)
        {
            if (TwkFB::FrameBuffer* fb = m_displayFBStatus[i].fb)
            {
                string n = fb->attribute<string>("RVSource");

                if (n == sourceName)
                {
                    return fb;
                }
                else
                {
                    StringVector stokens;
                    stl_ext::tokenize(stokens, n, "/");

                    if (stokens.size() >= tokens.size())
                    {
                        bool matches = true;

                        for (size_t q = 0; q < tokens.size(); q++)
                        {
                            if (stokens[q] != tokens[q])
                            {
                                matches = false;
                                break;
                            }
                        }

                        if (matches) return fb;
                    }
                }
            }
        }

        //
        //  If user passed in a simple name, and we did not find a match above,
        //  look again and return fb if leading substr matches.
        //

        if (tokens.size() == 1)
        {
            for (size_t i = 0; i < m_displayFBStatus.size(); i++)
            {
                if (TwkFB::FrameBuffer* fb = m_displayFBStatus[i].fb)
                {
                    string n = fb->attribute<string>("RVSource");

                    if (sourceName.compare(0, string::npos, n, 0, sourceName.size()) == 0)
                    {
                        return fb;
                    }
                }
            }
        }
    }

    return 0;
}

bool
RvSession::isEmpty() const
{
    return rvgraph().imageSources().empty();
}


void
RvSession::postInitialize()
{
    if (Function* F = sessionFunction("initialize"))
    {
        makeActive();
        Context::ArgumentVector v;
        TypedValue tv = TwkApp::muContext()->evalFunction(TwkApp::muProcess(), F, v);

        if (m_data = (Object*)tv._value._Pointer)
        {
            m_data->retainExternal();
        }
    }

    //
    //  Same thing but with python
    //

    m_pydata = callPythonFunction("initialize");

    userGenericEvent("state-intialized", "");
    userGenericEvent("state-initialized", "");

    //
    //  Mu setup
    //

    if (Function* F = sessionFunction("setup"))
    {
        makeActive();
        Context::ArgumentVector v;
        TwkApp::muContext()->evalFunction(TwkApp::muProcess(), F, v);
    }

    //
    //  Same thing but with python
    //

    void* ignored = callPythonFunction("setup");
    disposeOfPythonObject(ignored);

    //
    //  Mu init / command-line eval
    //

    if (m_initEval != "")
    {
        string output = muEval(muContext(),
                               muProcess(),
                               muModuleList(),
                               m_initEval.c_str(),
                               "Command line eval");

        if ("string => \"noprint\"" != output)
        {
            cout << "INFO: eval returned: " << output << endl;
        }
        m_initEval.clear();
    }

    //
    //  Same thing but with python
    //

    if (m_pyInitEval != "")
    {
        TwkApp::evalPython(m_pyInitEval.c_str());
    }
}

string
RvSession::currentSourceName() const
{
    string name;

    try
    {
        if (!currentFB())
        {
            name = "Untitled.rv";
        }
        else
        {
            name = currentFB()->attribute<string>("Sequence");
        }
    }
    catch (...)
    {
        name = "?";
    }

    return name;
}


//----------------------------------------------------------------------

void
RvSession::readGTOSessionContainer(PropertyContainer* pc)
{

    StringProperty* vnp  = pc->property<StringProperty>("session.viewNode");
    StringProperty* sp   = pc->property<StringProperty>("session.sources");
    IntProperty*    mp   = pc->property<IntProperty>("session.marks");
    IntProperty*    tp   = pc->property<IntProperty>("session.type");
    Vec2iProperty*  rp   = pc->property<Vec2iProperty>("session.range");
    Vec2iProperty*  iop  = pc->property<Vec2iProperty>("session.region");
    FloatProperty*  fpsp = pc->property<FloatProperty>("session.fps");
    IntProperty*    rtp  = pc->property<IntProperty>("session.realtime");
    IntProperty*    incp = pc->property<IntProperty>("session.inc");
    IntProperty*    fp   = pc->property<IntProperty>("session.currentFrame");
    IntProperty*    bg   = pc->property<IntProperty>("session.background");

    if (sp)
    {
        //
        //  Backwards compat
        //

        Options::SourceArgs sargs;
        for (int i=0; i < sp->size(); i++)
        {
            addSource((*sp)[i], sargs);
            cout << "INFO: added " << (*sp)[i] << endl;
        }
    }

    for (int i=0; mp && i < mp->size(); i++)
    {
        markFrame((*mp)[i]);
    }

    if (fpsp && !fpsp->empty()) setFPS(fpsp->front());
    if (rtp && !rtp->empty()) setRealtime(rtp->front() ? true : false);
    if (incp && !incp->empty()) setInc(incp->front());
    if (rp && !rp->empty()) setRangeStart(rp->front().x);
    if (rp && !rp->empty()) setRangeEnd(rp->front().y);
    if (iop && !iop->empty()) setInPoint(iop->front().x);
    if (iop && !iop->empty()) setOutPoint(iop->front().y);

    if (vnp && !vnp->empty())
    {
        setViewNode(vnp->front(), true);
    }

    if (bg && !bg->empty())
    {
        m_renderer->setBGPattern((ImageRenderer::BGPattern)bg->front());
    }
}

namespace {

void
makeTopLevelSet(PropertyContainer* connections,
                GTOReader::Containers& containers,
                set<string>& topLevelSet)
{
    StringProperty* lhs      = connections ? connections->property<StringProperty>("evaluation.lhs") : 0;
    StringProperty* rhs      = connections ? connections->property<StringProperty>("evaluation.rhs") : 0;
    StringPairProperty* cons = connections ? connections->property<StringPairProperty>("evaluation.connections") : 0;
    StringProperty* top      = connections ? connections->property<StringProperty>("top.nodes") : 0;

    if (top)
    {
        std::copy(top->valueContainer().begin(),
                  top->valueContainer().end(),
                  std::inserter(topLevelSet, topLevelSet.begin()));
    }

    if (lhs && rhs)
    {
        std::copy(lhs->valueContainer().begin(),
                  lhs->valueContainer().end(),
                  std::inserter(topLevelSet, topLevelSet.begin()));

        std::copy(rhs->valueContainer().begin(),
                  rhs->valueContainer().end(),
                  std::inserter(topLevelSet, topLevelSet.begin()));
    }

    if (cons)
    {
        const StringPairProperty::container_type& array = cons->valueContainer();

        for (size_t i = 0; i < array.size(); i++)
        {
            topLevelSet.insert(array[i].first);
            topLevelSet.insert(array[i].second);
        }
    }

    for (size_t i = 0; i < containers.size(); i++)
    {
        PropertyContainer* pc = containers[i];
        string name = TwkContainer::name(pc);

        if (pc->property<StringProperty>("ui.name"))
        {
            //
            //  If ui.name *and* has no _ in the name (meaning its one of ours?)
            //

            if (name.find("_") == string::npos) topLevelSet.insert(name);
        }
    }
}

void
inputsForNode(PropertyContainer* connections,
              IPGraph& graph,
              const string& name,
              IPGraph::IPNodes& inputs)
{
    if (!connections) return;

    StringProperty*     lhs          = connections->property<StringProperty>("evaluation.lhs");
    StringProperty*     rhs          = connections->property<StringProperty>("evaluation.rhs");
    StringPairProperty* cons         = connections->property<StringPairProperty>("evaluation.connections");

    if (!cons)
    {
        if (!lhs || !rhs || lhs->size() != rhs->size()) return;
    }

    inputs.clear();

    if (cons)
    {
        for (size_t i = 0; i < cons->size(); i++)
        {
            const pair<string,string>& arrow = (*cons)[i];

            if (arrow.second == name)
            {
                if (IPNode* n = graph.findNode(arrow.first))
                {
                    inputs.push_back(n);
                }
                else
                {
                    cout << "WARNING: could not find node "
                         << arrow.first << " when trying to connect inputs for "
                         << name
                         << endl;
                }
            }
        }
    }
    else
    {
        for (size_t i = 0; i < rhs->size(); i++)
        {
            if ((*rhs)[i] == name)
            {
                if (IPNode* n = graph.findNode((*lhs)[i]))
                {
                    inputs.push_back(n);
                }
                else
                {
                    cout << "WARNING: could not find node "
                         << (*lhs)[i] << " when trying to connect inputs for "
                         << name
                         << endl;
                }
            }
        }
    }
}

typedef set<string> NameSet;
typedef map<string, std::string> NameMap;

#ifdef PLATFORM_WINDOWS
#define PAD "%06d"
#else
#define PAD "%06zd"
#endif

string
uniqueName (string name, NameSet& disallowedNames)
{
    static char buf[16];
    static RegEx endRE("(.*[^0-9])([0-9]+)");
    static RegEx midRE("(.*[^0-9])([0-9][0-9][0-9][0-9][0-9][0-9])([^0-9].*)");

    if (disallowedNames.count(name) > 0)
    {
        //
        //  Find a 6-zero-padded number in the middle.
        //
        Match midMatch(midRE, name);

        if (midMatch)
        {
            string finalName;
            istringstream istr (midMatch.subStr(1));
            int n;
            istr >> n;
            do
            {
                ++n;
                sprintf (buf, PAD, size_t(n));
                ostringstream str;
                str << midMatch.subStr(0) << buf << midMatch.subStr(2);
                finalName = str.str();
            }
            while (disallowedNames.count (finalName) > 0);

            return finalName;
        }

        //
        //  Find a possibly zero-padded number on the end.
        //
        Match endMatch(endRE, name);

        if (endMatch)
        {
            string finalName;
            istringstream istr (endMatch.subStr(1));
            int n;
            istr >> n;
            do
            {
                ++n;
                sprintf (buf, PAD, size_t(n));
                ostringstream str;
                str << endMatch.subStr(0) << buf;
                finalName = str.str();
            }
            while (disallowedNames.count (finalName) > 0);

            return finalName;
        }

        //
        //  Fallback: add a number to the end and recurse
        //
        ostringstream str;
        str << name << "000002";
        return uniqueName(str.str(), disallowedNames);
    }

    return name;
}

void
uniqifyContainerNames (GTOReader::Containers& containers, NameSet& existingNames)
{
    NameSet origNames;
    NameSet conflictingNames;
    NameMap newNameMap;

    //
    //  run through containers that correspond to top-level nodes, identify those that
    //  conflict with existingNames
    //

    for (size_t i = 0; i < containers.size(); i++)
    {
        PropertyContainer* pc = containers[i];

        if (protocol(pc) == "RVSession") continue;

        string nm = TwkContainer::name(pc);

        if (nm.find('_') == string::npos && existingNames.count(nm))
        {
            conflictingNames.insert(nm);
        }
        origNames.insert(nm);
    }

    //
    //  for every conflicting top-level name, choose a new name (same type!) that isn't
    //  in either the original list, or the new one.
    //


    NameSet disallowedNames = existingNames;
    disallowedNames.insert (origNames.begin(), origNames.end());

    for (NameSet::iterator i = conflictingNames.begin(); i != conflictingNames.end(); ++i)
    {

        string newName = uniqueName (*i, disallowedNames);

        newNameMap[*i] = newName;
        disallowedNames.insert (newName);
    }

    //
    //  run through all containers, add mappings for non-top-level containers,
    //  based on the mapping for the corresponding top-level container(s).
    //

    for (size_t i = 0; i < containers.size(); i++)
    {
        PropertyContainer* pc = containers[i];

        if (protocol(pc) == "RVSession") continue;

        string nm = TwkContainer::name(pc);

        int gp = nm.find('_');
        if (gp != string::npos)
        {
            string newName;

            string groupName = nm.substr (0, gp);
            if (newNameMap.count (groupName)) groupName = newNameMap[groupName];

            int ip = nm.rfind('_');
            string inputName = nm.substr (ip+1);
            if (newNameMap.count (inputName)) inputName = newNameMap[inputName];

            newName = groupName + nm.substr(gp, ip-gp+1) + inputName;

            newNameMap[nm] = newName;
        }
    }

    //  run through all containers, change conflicting container name to new
    //  name, and any string properites that contain conflicting container name
    //  to new name
    //

    for (int i = 0; i < containers.size(); i++)
    {
        PropertyContainer* pc = containers[i];

        string nm = TwkContainer::name(pc);

        if (newNameMap.count (nm)) pc->setName (newNameMap[nm]);

        PropertyContainer::Components& comps = pc->components();
        for (int j = 0; j < comps.size(); ++j)
        {
            if (comps[j]->name() == "object") continue;

            Component::Container& props = comps[j]->properties();
            for (int k = 0; k < props.size(); ++k)
            {
                if (StringProperty* sp = dynamic_cast <StringProperty*> (props[k]))
                {
                    for (StringProperty::iterator l = sp->begin(); l != sp->end(); ++l)
                    {
                        if (newNameMap.count(*l))
                        {
                            *l = newNameMap[*l];
                        }
                    }
                }
                else
                if (StringPairProperty* spp = dynamic_cast <StringPairProperty*> (props[k]))
                {
                    for (StringPairProperty::iterator l = spp->begin(); l != spp->end(); ++l)
                    {
                        if (newNameMap.count(l->first))
                        {
                            l->first = newNameMap[l->first];
                        }
                        if (newNameMap.count(l->second))
                        {
                            l->second = newNameMap[l->second];
                        }
                    }
                }
            }
        }
    }
}

template <typename T>
inline T* nonEmptyProperty(PropertyContainer* pc, const char* comp, const char* prop)
{
    T* p = pc->property<T>(comp, prop);
    if (p && p->size() == 0) p = 0;
    return p;
}

} // anon namespace


static bool isProgressionProtocol(const std::string &protoName)
{
    return (protoName == "RVSource" || protoName == "RVFileSource" || protoName == "RVImageSource");
}

void
RvSession::readGTO(const string& infilename, bool merge, Options::SourceArgs& sargs)
{
    HOP_PROF_DYN_NAME(
        std::string(std::string("RvSession::readGTO : ") +
                    infilename).c_str());

    const VideoDevice* outputDevice = outputVideoDevice();

    typedef std::set<PropertyContainer*> PropertyContainerSet;

    string filename = pathConform(infilename);

    userGenericEvent("before-session-read", filename);

    GTOReader reader;
    GTOReader::Containers containers;
    {
        HOP_PROF_DYN_NAME(
            std::string(std::string("RvSession::readGTO - GTOReader.read : ") +
                        filename).c_str());

        containers = reader.read(filename.c_str());
    }

    merge = merge && (sources().size());

    if (merge)
    {
        NameSet existingNames;

        for (IPGraph::NodeMap::const_iterator i = graph().viewableNodes().begin();
             i != graph().viewableNodes().end();
             ++i)
        {
            string n = (*i).second->name();
            if (! graph().isDefaultView(n)) existingNames.insert (n);
        }

        uniqifyContainerNames (containers, existingNames);
        applyRvSessionSpecificOptions();
    }
    else
    {
        clear();
    }

    PropertyContainer* session = 0;
    PropertyContainer* connections = 0;

    vector<int> v1InsVec;
    vector<int> v1OutsVec;
    string v1CTypeStr;
    float v1FPS = 0.0;

    m_inputFileVersion = 0;

    Property::Info* notPersistent = new Property::Info();
    notPersistent->setPersistent(false);

    for (size_t i = 0; i < containers.size(); i++)
    {
        PropertyContainer* pc = containers[i];
        string p = protocol(pc);

        if (p == "RVSession")
        {
            session = pc;
            m_inputFileVersion = TwkContainer::protocolVersion(pc);
        }

        if (p == "IPNodeDefinition")
        {
            pc->declareProperty<StringProperty>("node.origin", infilename, notPersistent, true);
            NodeDefinition* def = new NodeDefinition(pc);
            IPCore::App()->nodeManager()->addDefinition(def);
        }
    }

    switch (m_inputFileVersion)
    {
      case 0:
          cout << "WARNING: no file version: assuming version 1 or 2 file format" << endl;
          m_inputFileVersion = 1;
          break;
      case 1:
      case 2:
      case 4:
          cout << "INFO: reading version "
               << m_inputFileVersion
               << " session file format" << endl;
          break;
      case 3:
          cout << "INFO: reading version "
               << m_inputFileVersion
               << " session file format (which never existed)" << endl;
          m_inputFileVersion = 4;
          break;
      default:
          cout << "WARNING: trying to read version "
               << m_inputFileVersion
               << " session file format";

          if (m_inputFileVersion > 4) cout << " created by a newer version of RV";
          else cout << " which corresponds to no RV version";
          cout << endl;

          break;
    }

    //
    //  NOTE: IF YOU ARE MAKING BACKWARDS COMPATIBLE FILE CHANGES IN
    //  THIS FUNCTION:
    //
    //  The correct place to make changes which are localized to a
    //  node's layout differing between file versions is in the node's
    //  readCompleted() function NOT here. Only add backwards
    //  compatible changes here if its a file scope change. For
    //  example default nodes got renamed, or a node was split into
    //  two separate nodes.
    //
    //  For an example of localized node versioning look in
    //  PaintIPNode.
    //

    //
    //  Pass 1: get the sources and the session information. Set
    //  protocols to backwards compat names
    //

    set<string> topLevelSet;

    {
        HOP_PROF( "Rv::RvSession::readGTO : PASS 1" );

        RvGraph::FastAddSourceGuard fastAddSourceGuard(rvgraph());

        m_gtoSourceTotal = 0;

        for (int i=0; i < containers.size(); i++)
        {
            PropertyContainer* pc = containers[i];
            string p = protocol(pc);
            if ( isProgressionProtocol(p) )
                ++m_gtoSourceTotal;
        }

        m_gtoSourceCount = 0;
        m_readingGTO = true;

        for (int i=0; i < containers.size(); i++)
        {
            PropertyContainer* pc     = containers[i];
            string             p      = protocol(pc);
            size_t             pver   = protocolVersion(pc);
            string             pcname = TwkContainer::name(pc);

            //
            //  Backwards compat
            //

            if (m_inputFileVersion == 1)
            {
                if (p == "RVStereo")
                {
                    setProtocol(*pc, "RVDisplayStereo");
                    p = "RVDisplayStereo";
                }

                Match rem(oldContainerNameRE, pcname);

                if (rem)
                {
                    pcname = "sourceGroup000";
                    pcname += rem.subStr(1);
                    pcname += "_";
                    pcname += rem.subStr(0);
                    TwkContainer::setName(*pc, pcname);
                }
                else if (pcname == "sequence")
                {
                    TwkContainer::setName(*pc, defaultSequenceSequenceIPNodeName);

                    IntProperty* v1Ins    = pc->property<IntProperty>("edl", "in");
                    if (v1Ins)  v1InsVec  = v1Ins->valueContainer();

                    IntProperty* v1Outs   = pc->property<IntProperty>("edl", "out");
                    if (v1Outs) v1OutsVec = v1Outs->valueContainer();
                }
                else if (pcname == "composite")
                {
                    //
                    //  NOTE: this is on purpose. There's nothing at this
                    //  point preventing two containers from sharing a
                    //  name and 3.10 removed the composite node in favor
                    //  of doing everything in the stack
                    //

                    TwkContainer::setName(*pc, "defaultStack_stack");
                    TwkContainer::setProtocol(*pc, "RVStack");
                    pc->removeProperty<IntProperty>("single", "index");
                    pc->removeProperty<FloatProperty>("pip", "x");
                    pc->removeProperty<FloatProperty>("pip", "y");
                    pc->removeProperty<FloatProperty>("pip", "width");

                    StringProperty* v1CType = pc->property<StringProperty>("composite", "type");
                    if (v1CType)
                    {
                        if (v1CType->front() == "pip") v1CType->front() = "replace";
                        v1CTypeStr = v1CType->front();
                    }
                }
                else if (pcname == "stack")
                {
                    TwkContainer::setName(*pc, "defaultStack_stack");
                }
                else if (pcname == "stereo")
                {
                    TwkContainer::setName(*pc, "displayGroup0_stereo");
                }
                else if (pcname == "display")
                {
                    TwkContainer::setName(*pc, "displayGroup0_display");
                }
                else if (pcname == "displayTransform")
                {
                    TwkContainer::setName(*pc, "viewGroup_dxform");
                }
                else if (pcname == "soundtrack")
                {
                    TwkContainer::setName(*pc, "viewGroup_soundtrack");
                }

                if (p == "RVSource")
                {
                    setProtocol(*pc, "RVFileSource");
                    p = "RVFileSource";
                }
            }

            if (m_inputFileVersion == 1 || m_inputFileVersion == 2)
            {
                if (p == "RVColor" && pver == 1)
                {
                    //
                    //  RVColor is split into RVColor in the source group's
                    //  color pipeline and RVLinearize in the source group's
                    //  linearize pipeline.
                    //

                    string pcname = TwkContainer::name(pc);

                    if (pver != 1)
                    {
                        cout << "WARNING: RVColor node in version 2 session file has unknown version "
                             << pver << endl;
                    }

                    PropertyContainer* nl = new PropertyContainer();

                    string groupName = pcname.substr(0, pcname.size() - 6);
                    string nlName = groupName + "_tolinPipeline_0";
                    string cName = groupName + "_colorPipeline_0";

                    TwkContainer::setProtocol(*nl, "RVLinearize");
                    TwkContainer::setName(*nl, nlName);

                    TwkContainer::setProtocolVersion(*pc, 2);
                    TwkContainer::setName(*pc, cName);

                    Component* clut    = pc->component("lut");
                    Component* ccolor  = pc->component("color");
                    Component* ccineon = pc->component("cineon");
                    Component* clumlut = pc->component("luminanceLUT");
                    Component* nllut   = (clut) ?     clut->copy() : 0;
                    Component* nlcolor = (ccolor) ?   ccolor->copy() : 0;
                    Component* nlcineon = (ccineon) ? ccineon->copy() : 0;

                    //
                    //  Remove non-linear and LUT props from color node
                    //

                    if (clut)    pc->remove(clut); delete clut;
                    if (ccineon) pc->remove(ccineon); delete ccineon;

                    if (clumlut) clumlut->remove("type");

                    if (ccolor)
                    {
                        ccolor->remove("alphaType");
                        ccolor->remove("logtype");
                        ccolor->remove("YUV");
                        ccolor->remove("YIQ");
                        ccolor->remove("sRGB2linear");
                        ccolor->remove("Rec709ToLinear");
                        ccolor->remove("fileGamma");
                        ccolor->remove("ignoreChromaticities");
                    }

                    //
                    //  Remove the color props the linearize node will not
                    //  use (to prevent file pollution if the session is
                    //  written out again).
                    //

                    if (nlcolor)
                    {
                        nlcolor->remove("lut");
                        nlcolor->remove("invert");
                        nlcolor->remove("gamma");
                        nlcolor->remove("YIQ");
                        nlcolor->remove("offset");
                        nlcolor->remove("scale");
                        nlcolor->remove("exposure");
                        nlcolor->remove("contrast");
                        nlcolor->remove("saturation");
                        nlcolor->remove("hue");
                        nlcolor->remove("normalize");
                    }

                    if (nllut)    nl->add(nllut);
                    if (nlcolor)  nl->add(nlcolor);
                    if (nlcineon) nl->add(nlcineon);

                    containers.push_back(nl);
                }
                else if (p == "RVTransform2D")
                {
                    //
                    // The pixel aspect ratio is now handled by the RVLensWarp node
                    // so we need to move it over.
                    //

                    string pcname = TwkContainer::name(pc);

                    PropertyContainer* nl = new PropertyContainer();

                    //
                    // Note: using the second slot of the tolinPipeline this way is
                    // fragile and will need updating if the order of pipeline
                    // nodes ever changes.
                    //

                    string groupName = pcname.substr(0, pcname.size() - 12); // Minus "_transform2D"
                    string nlName = groupName + "_tolinPipeline_1";

                    TwkContainer::setProtocol(*nl, "RVLensWarp");
                    TwkContainer::setName(*nl, nlName);

                    if (FloatProperty* oldP = pc->property<FloatProperty>("pixel", "aspectRatio"))
                    {
                        FloatProperty* newP = nl->createProperty<FloatProperty>("warp", "pixelAspectRatio");
                        newP->resize(1);
                        newP->front() = oldP->front();

                        //
                        // "aspectRatio" is the only property of the old "pixel"
                        // component. Therefore after we copy out the value we must
                        // delete the component.
                        //

                        if (Component* pixelcomp = pc->component("pixel"))
                        {
                            pc->remove(pixelcomp);
                            delete pixelcomp;
                        }
                    }

                    containers.push_back(nl);
                }
                else if (p == "RVFileSource")
                {
                    if (StringProperty* oldP = pc->property<StringProperty>("request", "stereoViews"))
                    {
                        oldP->clearToDefaultValue();
                    }

                    if (StringProperty* sp = pc->property<StringProperty>("request.imageLayerSelection"))
                    {
                        if (!sp->empty())
                        {
                            StringVector v(3);
                            v[0] = "layer";
                            v[1] = "";
                            v[2] = sp->front();
                            pc->declareProperty<StringProperty>("request.imageComponent", v);
                        }

                        pc->removeProperty(sp);
                    }

                    if (StringProperty* sp = pc->property<StringProperty>("request.imageViewSelection"))
                    {
                        if (!sp->empty())
                        {
                            StringVector v(2);
                            v[0] = "view";
                            v[1] = sp->front();
                            pc->declareProperty<StringProperty>("request.imageComponent", v);
                        }

                        pc->removeProperty(sp);
                    }

                    if (StringProperty* sp = pc->property<StringProperty>("request.imageChannelSelection"))
                    {
                        if (!sp->empty())
                        {
                            StringVector v(4);
                            v[0] = "channel";
                            v[1] = "";
                            v[2] = "";
                            v[3] = sp->front();

                            pc->declareProperty<StringProperty>("request.imageComponent", v);
                        }

                        pc->removeProperty(sp);
                    }
                }
                else if (p == "RVSoundTrack")
                {
                    //
                    //  Used to be part of DisplayGroupIPNode but is now part of
                    //  ViewGroupIPNode
                    //

                    TwkContainer::setName(*pc, "viewGroup_soundtrack");
                }
                else if (p == "RVDispTransform2D")
                {
                    //
                    //  Same as RVSoundTrack
                    //

                    TwkContainer::setName(*pc, "viewGroup_dxform");
                }
            }

            //
            //  Add sources
            //
            //  Generally, we'd like to get rid of the way sources are
            //  created. They should be created just like any other node
            //  and load files / hook up props once that process is
            //  complete. The current way this is done puts a lot of
            //  constraints on timing + initialization which are
            //  unnecessarily complex.
            //

            if (p == "RVFileSource" || p == "RVImageSource")
            {
                if (SourceIPNode* n = rvgraph().addSource(p, pcname))
                {
                    n->copy(pc);
                    addNodeCreationContext(n);
                    userGenericEvent("new-node", n->name());

                    n->mediaChangedSignal().connect(boost::bind(&RvSession::newMediaLoaded, this, n));
                    StringProperty* movie = n->property<StringProperty>("media.movie");
                    if (FileSourceIPNode* fsipn = dynamic_cast<FileSourceIPNode*>(n))
                    {
                        fsipn->storeInputParameters(sargs.inparams);
                        StringVector movies, userMovies;
                        for (int i = 0; i < movie->size(); ++i) movies.push_back((*movie)[i]);
                        buildFileList(movies, "session", userMovies);
                        movie->resize(0);
                        for (int j = 0; j < userMovies.size(); ++j) movie->push_back(userMovies[j]);
                    }

                    //
                    //  Handle new-source event
                    //

                    ostringstream nscontents;
                    string media = (movie->size() > 0) ? (*movie)[0] : "";
                    nscontents << n->name() << ";;RVSource;;" << media;
                    cout << "INFO: loading " << media << endl;
                    userGenericEvent("new-source", nscontents.str());
                }
            }
            else if (p == "connection")
            {
                connections = pc;
            }
        }

        makeTopLevelSet(connections, containers, topLevelSet);
    } // HOP_PROF_DYN for PASS 1

    //
    //  Pass 2: build any user created top level nodes (if we know how)
    //

    {
        HOP_PROF( "Rv::RvSession::readGTO : PASS 2" );
        for (int i=0; i < containers.size(); i++)
        {
            PropertyContainer* pc               = containers[i];
            string             fprotocol        = protocol(pc);
            unsigned int       fprotocolVersion = protocolVersion(pc);

            if (fprotocol == "RVSession" || fprotocol == "connection") continue;

            string nodeName = TwkContainer::name(pc);
            IPGraph::IPNodes inputs;
            ostringstream msg;

            const NodeDefinition* def = graph().nodeDefinition(fprotocol);

            if (def && topLevelSet.count(nodeName) > 0)
            {
                if (IPNode* n = graph().findNode(nodeName))
                {
                    //
                    //  Its a automatically created node, copy the state over
                    //

                    n->copy(pc);
                    addNodeCreationContext(n);
                }
                else
                {
                    if (IPNode* n = graph().newNode(fprotocol, nodeName))
                    {
                        addNodeCreationContext(n);

                        //
                        // need to copy the state here also because some of
                        // the state (group nodes) will affect the topology
                        //

                        // if (n->protocol() == fprotocol)
                        // {
                        //     n->copy(pc);
                        // }
                    }
                    else
                    {
                        cout << "WARNING: cannot make a node of type "
                             << fprotocol
                             << " for node "
                             << nodeName
                             << " in .rv file: ignoring"
                             << endl;
                    }
                }
            }
        }
    } // HOP_PROF_DYN for PASS 2

    PropertyContainerSet unusedContainers;
    PropertyContainerSet usedContainers;
    vector<pair<string, int> > readCompletedNodes;

    //
    //  Pass 3: connect and copy state into top level nodes
    //

    {
        HOP_PROF( "Rv::RvSession::readGTO : PASS 3" );

        for (int i=0; i < containers.size(); i++)
        {
            PropertyContainer* pc               = containers[i];
            string             fprotocol        = protocol(pc);
            unsigned int       fprotocolVersion = protocolVersion(pc);
            string             nodeName         = TwkContainer::name(pc);

            if (fprotocol == "RVSession"    || fprotocol == "connection")
            {
                usedContainers.insert(pc);
            }
            else if (IPNode* n = graph().findNode(nodeName))
            {
                addNodeCreationContext(n);

                //
                //  Does this node have top level connections? If so
                //  connect it up. All connectable nodes should exist by
                //  this pass.
                //

                if (topLevelSet.count(nodeName) > 0)
                {
                    IPGraph::IPNodes inputs;
                    inputsForNode(connections, graph(), nodeName, inputs);

                    //
                    //  If it's a default view, it has connected itself
                    //  automatically.  But this may not be what the user saved in
                    //  the session file, since we allow editing of the default
                    //  views.  So in the usual case, set the inputs for the
                    //  default nodes like other nodes, from whatever is specified
                    //  in the connections list.
                    //
                    //  But two special cases:  If we're merging there's not really
                    //  anything sensible to be done and if there are no
                    //  connections for the default view (IE it's a stub session
                    //  file), so in those cases allow default views to build
                    //  themselves.
                    //

                    if (!graph().isDefaultView(n->name()) || (!merge && inputs.size())) n->setInputs(inputs);

                    try
                    {
                        if (n->protocol() == fprotocol)
                        {
                            n->copy(pc);
                            readCompletedNodes.push_back(make_pair(n->name(),fprotocolVersion));
                        }
                    }
                    catch (std::exception& e)
                    {
                        cerr << "ERROR: in file, node " << TwkContainer::name(pc)
                             << ": " << endl
                             << e.what();
                    }
                }
                else
                {
                    unusedContainers.insert(pc);
                }
            }
            else
            //
            //  The node has not been created yet, but it may be later (EG pipeline
            //  group contents), so be sure to add to unused list so we have a
            //  chance to copy state in pass 4.
            //
            {
                unusedContainers.insert(pc);
            }
        }
    } // HOP_PROF_DYN for PASS 3

    //
    //  Pass 4: iterate on all unused property containers until either
    //  there are none left or we can't find nodes that correspond to the
    //  containers.
    //

    {
        HOP_PROF( "Rv::RvSession::readGTO : PASS 4" );
        while (!unusedContainers.empty())
        {
            // Pass 4a
            {
                HOP_PROF( "Rv::RvSession::readGTO : PASS 4.A" );
                usedContainers.clear();

                for (PropertyContainerSet::const_iterator i = unusedContainers.begin();
                     i != unusedContainers.end();
                     ++i)
                {
                    PropertyContainer* pc               = *i;
                    string             fprotocol        = protocol(pc);
                    unsigned int       fprotocolVersion = protocolVersion(pc);
                    string             nodeName         = TwkContainer::name(pc);

                    if (IPNode* n = graph().findNode(nodeName))
                    {
                        //
                        //  Restore the state from the file. This may overwrite
                        //  any default state created when the connections where
                        //  made.
                        //

                        try
                        {
                            if (n->protocol() == fprotocol)
                            {
                                //
                                //  Don't recopy sources. They have already been copied
                                //  above in pass 1 and possibly modified their properties.
                                //

                                if (n->protocol() != "RVFileSource" &&
                                    n->protocol() != "RVImageSource")
                                {
                                    n->copy(pc);
                                }

                                addNodeCreationContext(n);
                                readCompletedNodes.push_back(make_pair(n->name(),fprotocolVersion));
                                usedContainers.insert(pc);
                            }
                        }
                        catch (std::exception& e)
                        {
                            cerr << "ERROR: in file, node " << TwkContainer::name(pc)
                                 << ": " << endl
                                 << e.what();
                        }
                    }
                }

                for (PropertyContainerSet::const_iterator i = usedContainers.begin();
                     i != usedContainers.end();
                     ++i)
                {
                    unusedContainers.erase(*i);
                }
            } // HOP_PROF_DYN for PASS 4.A

            //
            //  Pass 4b: Now that the graph is assembled and all reads are completed
            //  it is safe to trigger readCompleted for each node.
            //
            //  Note this for-loop has to be within the pass4 while() loop as
            //  there are unusedContainers nodes that arent found in graph until
            //  nodes from a previous while-loop iteration have been readCompleted().

            {
                HOP_PROF_DYN_NAME(
                    std::string(
                        std::string( "Rv::RvSession::readGTO : PASS 4.B (readCompletedNodes size = " ) +
                        std::to_string( readCompletedNodes.size() ) ).c_str() );
                for (int i=0; i < readCompletedNodes.size(); i++)
                {
                    HOP_ZONE( (i % 2== 0) ? HOP_ZONE_COLOR_10 :HOP_ZONE_COLOR_11 );
                    IPNode* n(nullptr);
                    {
                        HOP_PROF_DYN_NAME(
                            std::string(
                                std::string( "findNode : " ) +
                                readCompletedNodes[i].first ).c_str() );

                        n = graph().findNode(readCompletedNodes[i].first);
                    }
                    if (n)
                    {
                        HOP_PROF( "readCompleted" );
                        n->readCompleted(n->protocol(), readCompletedNodes[i].second);
                        if (isProgressionProtocol(n->protocol()))
                        {
                            send(updateLoadingMessage());
                            ++m_gtoSourceCount;
                        }
                    }
                }
                readCompletedNodes.clear();

                if (usedContainers.empty()) break;
            } // HOP_PROF_DYN for PASS 4.B
        }
    } // HOP_PROF_DYN for PASS 4

    //
    //  Pass 5: Safe to trigger readCompleted for each node not already done in
    //          pass4b
    //

    {
        HOP_PROF( "Rv::RvSession::readGTO : PASS 5" );

        for (int i=0; i < readCompletedNodes.size(); i++)
        {
            if (IPNode* n = graph().findNode(readCompletedNodes[i].first))
            {
                n->readCompleted(n->protocol(), readCompletedNodes[i].second);
            }
        }
    } // HOP_PROF_DYN for PASS 5

    //
    //  Pass 6: set the session state
    //

    {
        HOP_PROF( "Rv::RvSession::readGTO : PASS 6" );
        for (int i=0; i < containers.size(); i++)
        {
            PropertyContainer* pc = containers[i];
            string p = protocol(pc);

            if (p == "RVSession")
            {
                //
                //  If we're merging a session file in, assume that the
                //  previous reads set up the session correctly.
                //

                if (!merge)
                {
                    readGTOSessionContainer(pc);

                    SessionIPNode* sessionNode = graph().sessionNode();
                    sessionNode->copy(pc);
                    sessionNode->remove(sessionNode->component("session"));
                }
            }
        }
    } // HOP_PROF_DYN for PASS 6

    {
        HOP_PROF( "Rv::RvSession::readGTO : TEARDOWN" );
        //
        //  Delete the containers we got from the file
        //

        for (int i=0; i < containers.size(); i++)
        {
            delete containers[i];
        }

        if (v1FPS != 0.0)
        {
            //
            //  we read a v1 session file, so update all session/outputFPSs
            //

            for (IPGraph::NodeMap::const_iterator i = graph().nodeMap().begin();
                 i != graph().nodeMap().end();
                 ++i)
            {
                FloatProperty* ofpsp = (*i).second->property<FloatProperty>("timing.outputFPS");
                FloatProperty* sfpsp = (*i).second->property<FloatProperty>("session.fps");

                if (ofpsp) ofpsp->front() = v1FPS;
                if (sfpsp) sfpsp->front() = v1FPS;
            }
        }

        m_readingGTO     = false;
        m_gtoSourceTotal = 0;
        m_gtoSourceCount = 0;

        setFileName(filename);

        //
        //  Restore presentation device, if any.
        //
        setOutputVideoDevice(outputDevice);

        //
        //  Send the file read event
        //

        userGenericEvent("after-session-read", filename);
    }  // HOP_PROF_DYN for TEARDOWN
}

void
RvSession::readLUTOnAll(string name, const string& nodeType, bool activate)
{
    NodeVector nodes;
    string filename = pathConform(IPCore::Application::mapFromVar(name));
    graph().findNodesByTypeName(nodes, nodeType);

    if (!TwkUtil::fileExists(filename.c_str()))
    {
        if (const char *path = getenv("RV_LUT_PATH"))
        {
            StringVector names = TwkUtil::findInPath(filename, path);
            if (names.size()) filename = names[0];
        }
    }

    for (int i=0; i < nodes.size(); i++)
    {
        readLUT(filename, nodes[i]->name(), activate);
    }
}

void
RvSession::readCDL(string name, const string& nodeName, bool activate)
{
    string filename = pathConform(IPCore::Application::mapFromVar(name));
    PropertyVector props;
    graph().findProperty(m_frame, props, nodeName + ".CDL.active");

    if (props.empty())
    {
        TWK_THROW_EXC_STREAM("Bad node: " << nodeName);
    }

    CDL::ColorCorrectionCollection ccc = CDL::readCDL(name);
    if (ccc.size() > 1)
    {
        cout << "WARNING: Found more than one cdl in " << name
             << ". Using first one found." << endl;
    }

    if (ccc.size() <= 0) return;

    NodeVector cdlNodes;
    for (size_t i = 0; i < props.size(); i++)
    {
        if (FileSpaceLinearizeIPNode* node = dynamic_cast<FileSpaceLinearizeIPNode*>(props[i]->container()))
        {
            cdlNodes.push_back(node);

            Vec3fProperty*  slope      = node->property<Vec3fProperty>("CDL.slope");
            Vec3fProperty*  offset     = node->property<Vec3fProperty>("CDL.offset");
            Vec3fProperty*  power      = node->property<Vec3fProperty>("CDL.power");
            FloatProperty*  saturation = node->property<FloatProperty>("CDL.saturation");
            IntProperty*    active     = node->property<IntProperty>("CDL.active");

            CDL::ColorCorrection cc = ccc.front();

            slope->front()      = cc.slope;
            offset->front()     = cc.offset;
            power->front()      = cc.power;
            saturation->front() = cc.saturation;
            active->front()     = (active) ? 1 : 0;
        }
        else if (ColorIPNode* node = dynamic_cast<ColorIPNode*>(props[i]->container()))
        {
            cdlNodes.push_back(node);

            Vec3fProperty*  slope      = node->property<Vec3fProperty>("CDL.slope");
            Vec3fProperty*  offset     = node->property<Vec3fProperty>("CDL.offset");
            Vec3fProperty*  power      = node->property<Vec3fProperty>("CDL.power");
            FloatProperty*  saturation = node->property<FloatProperty>("CDL.saturation");
            IntProperty*    active     = node->property<IntProperty>("CDL.active");

            CDL::ColorCorrection cc = ccc.front();

            slope->front()      = cc.slope;
            offset->front()     = cc.offset;
            power->front()      = cc.power;
            saturation->front() = cc.saturation;
            active->front()     = (active) ? 1 : 0;
        }
    }

    for (int i = 0; i < cdlNodes.size(); ++i)
    {
        string contents = filename + ";;" + cdlNodes[i]->name();
        userGenericEvent("read-cdl-complete", contents);
    }
}

void
RvSession::readLUT(string filename, const string& nodeName, bool activate)
{
    PropertyVector props;
    graph().findProperty(m_frame, props, nodeName + ".lut.active");

    NodeVector lutNodes;
    for (size_t i = 0; i < props.size(); i++)
    {
        if (LUTIPNode* node = dynamic_cast<LUTIPNode*>(props[i]->container()))
        {
            lutNodes.push_back(node);

            StringProperty* lutFile = node->property<StringProperty>("lut.file");
            lutFile->front() = filename;

            if (activate)
            {
                if (IntProperty* p = node->property<IntProperty>("lut.active"))
                {
                    p->resize(1);
                    p->front() = 1;
                }
            }
            node->propertyChanged(lutFile);
        }
    }

    for (int i = 0; i < lutNodes.size(); ++i)
    {
        string contents = filename + ";;" + lutNodes[i]->name();
        userGenericEvent("read-lut-complete", contents);

    }
}

void
RvSession::readEDL(const char* filename)
{
    cerr << "ERROR: Unsupported file format" << endl;
}

void
RvSession::setScaleOnAll(float scale)
{
    Rv::setScaleOnAll(graph(), scale);
    reload(currentFrame(), currentFrame());
}

static string
swapV(const string& filename, const string& pat, const string& v)
{
    string vfilename = filename;
    string::size_type p;

    while ((p = vfilename.find(pat)) != string::npos)
    {
        vfilename.replace(p, 2, v);
    }

    return vfilename;
}

void
RvSession::checkForStereoPaths(const string& filename, StringVector& movies)
{
    if (isStereoSequence(filename))
    {
        for (size_t i = 0; i < m_vStrings.size(); i+=2)
        {
            string vfilename = swapV(filename, "%v", m_vStrings[i]);

            for (size_t j = 0; j < m_VStrings.size(); j+=2)
            {
                string Vfilename = swapV(vfilename, "%V", m_VStrings[j]);
                ExistingFileList Vfiles = existingFilesInSequence(Vfilename);
                if (Vfiles.size() > 0 && TwkUtil::fileExists(Vfiles[0].name.c_str()))
                {
                    movies.push_back(Vfilename);
                    string rfilename = swapV(swapV(filename, "%v", m_vStrings[i+1]), "%V", m_VStrings[j+1]);
                    ExistingFileList rfiles = existingFilesInSequence(rfilename);
                    if (rfiles.size() > 0 && TwkUtil::fileExists(rfiles[0].name.c_str()))
                    {
                        movies.push_back(rfilename);
                    }
                    else
                    {
                        cerr << "ERROR: can't find right eye in " << filename << endl;
                    }

                    return;
                }
            }
        }
    }
    else
    {
        movies.push_back(filename);
    }
}

void
RvSession::findProperty(PropertyVector& props, const std::string& name)
{
    StringVector parts;
    algorithm::split(parts, name, is_any_of(string(".")));

    if (parts[0] == "#RVSource" && parts.size() == 3)
    {
        ostringstream fname;
        ostringstream iname;

        fname << "#RVFileSource." << parts[1] << "." << parts[2];
        iname << "#RVImageSource." << parts[1] << "." << parts[2];

        graph().findProperty(m_frame, props, fname.str());
        graph().findProperty(m_frame, props, iname.str());
    }
    else
    {
        graph().findProperty(m_frame, props, name);
    }
}

void
RvSession::findCurrentNodesByTypeName(NodeVector& nodes, const string& typeName)
{
    if (typeName == "RVSource")
    {
        graph().findNodesByTypeName(m_frame, nodes, "RVFileSource");
        graph().findNodesByTypeName(m_frame, nodes, "RVImageSource");
    }
    else
    {
        graph().findNodesByTypeName(m_frame, nodes, typeName);
    }
}

void
RvSession::findNodesByTypeName(NodeVector& nodes, const string& typeName)
{
    if (typeName == "RVSource")
    {
        graph().findNodesByTypeName(nodes, "RVFileSource");
        graph().findNodesByTypeName(nodes, "RVImageSource");
    }
    else
    {
        graph().findNodesByTypeName(nodes, typeName);
    }
}

IPNode*
RvSession::newNode(const std::string& typeName, const std::string& nodeName)
{
    IPNode* node = graph().newNode(typeName, nodeName);

    if (node) userGenericEvent("new-node", node->name());

    return node;
}

void
RvSession::setRendererType(const std::string& type)
{
    Session::setRendererType(type);

    if (renderer())
    {
        Rv::Options& opts = Rv::Options::sharedOptions();
        renderer()->setFiltering(opts.imageFilter);
    }
}

void
RvSession::deleteNode(IPNode* node)
{
    bool isSource = dynamic_cast<SourceGroupIPNode*>(node) || dynamic_cast<SourceIPNode*>(node);
    string name = node->name();

    if (isSource) userGenericEvent("before-source-delete", name);

    if (node == m_conductorSource) m_conductorSource = nullptr;
    if (node == m_sequenceIPNode) setSequenceIPNode(nullptr);

    Session::deleteNode(node);

    if (isSource) userGenericEvent("after-source-delete", name);
}

void
RvSession::readProfile(const string& infilename,
                       IPNode* node,
                       const ReadRequest& request)
{
    Session::readProfile(infilename, node, request);
}

// onGraphFastAddSourceChanged is called after the graph changes the state of fastAddSourceEnabled
//
void
RvSession::onGraphFastAddSourceChanged(bool begin, int newFastAddSourceEnabled)
{
    if (begin && (newFastAddSourceEnabled == 1))
        unsetSequenceEvents();
    else if(!begin && (newFastAddSourceEnabled == 0))
        setSequenceEvents();
}

void
RvSession::onGraphMediaSetEmpty()
{
    userGenericEvent("after-progressive-loading", "");
}

// connects events from the sequence IP node
//
void
RvSession::setSequenceEvents()
{
    if ( ! evUseSequenceEventsTracking.getValue() ) return;

    auto sequenceIPNode = dynamic_cast<SequenceIPNode*>(m_graph->findNode(defaultSequenceSequenceIPNodeName));
    setSequenceIPNode(sequenceIPNode);
    m_firstSequenceChange = true;
}

// disconnects events from the sequence IP node
//
void
RvSession::unsetSequenceEvents()
{
   if ( ! evUseSequenceEventsTracking.getValue() ) return;

    setSequenceIPNode(nullptr);
}

// connect or disconnect a new sequence IP node
//
void
RvSession::setSequenceIPNode(IPCore::SequenceIPNode* newSequenceIPNode)
{
    if ( m_sequenceIPNode == newSequenceIPNode)
        return;

    if ( m_sequenceIPNode)
    {
        m_sequenceIPNode->changingSignal().disconnect(boost::bind(&RvSession::onSequenceChanging, this));
        m_sequenceIPNode->changedSignal().disconnect(boost::bind(&RvSession::onSequenceChanged, this));
    }

    m_sequenceIPNode = newSequenceIPNode;

    if ( m_sequenceIPNode)
    {
        m_sequenceIPNode->changingSignal().connect(boost::bind(&RvSession::onSequenceChanging, this));
        m_sequenceIPNode->changedSignal().connect(boost::bind(&RvSession::onSequenceChanged, this));
    }
}

// onSequenceChanging is called before all state modification
// in the sequence IP node.
//
// It saves the state of the source where the playhead is located.
//
void
RvSession::onSequenceChanging()
{
    auto root = rootNode();
    if (!root || m_readingGTO || !m_sequenceIPNode)
        return;

    if (m_firstSequenceChange)
    {
        m_firstSequenceChange = false;
        return;
    }

    int frameNum = currentFrame();
    m_currentSourceIndex = m_sequenceIPNode->indexAtFrame(frameNum);
    if (m_currentSourceIndex >= 0)
    {
        m_sequenceIPNode->getSourceRange(m_currentSourceIndex, m_currentSourceRangeInfo, m_currentSourceOffset);
    }
}

// onSequenceChanged is called after all state modification in the
// sequence IP node.
//
// It loads the new state of the source where the playhead was  before the layout
// and replaces the playhead to the same absolute position if the source is
// discovered or the same relative position if not.
//
// It changes the playhead position to avoid that it dances in the timeline.
void
RvSession::onSequenceChanged()
{
    if (m_currentSourceIndex < 0)
        return;
    auto root = rootNode();
    if (! root)
    {
        m_currentSourceIndex = -1;
        return;
    }

    IPCore::IPNode::ImageRangeInfo newSourceRangeInfo;
    int newSourceOffset;
    if(!m_sequenceIPNode->getSourceRange(m_currentSourceIndex, newSourceRangeInfo, newSourceOffset))
    {
        m_currentSourceIndex = -1;
        return;
    }

    int currFrameNum = currentFrame();
    int newFrameNum = currFrameNum;
    if (m_currentSourceRangeInfo.isUndiscovered)
    {
        // we want that the playhead stays at the same relative position in the source
        double ratio = static_cast<double>(currFrameNum - m_currentSourceOffset) / (m_currentSourceRangeInfo.cutOut - m_currentSourceRangeInfo.cutIn + 1);
        newFrameNum = newSourceOffset + static_cast<int>(ratio * (newSourceRangeInfo.cutOut - newSourceRangeInfo.cutIn + 1));
    }
    else
    {
       // we want that the playhead stays at the same absolute position in the source
       int frameOffset = currFrameNum - m_currentSourceOffset;
       newFrameNum = newSourceOffset + frameOffset;
    }

    if (newFrameNum != currFrameNum)
    {
        setFrame(newFrameNum);
    }

    m_currentSourceIndex = -1;
}

// onGraphNodeWillRemove is called when a node will be remove in the graph.
//
void RvSession::onGraphNodeWillRemove(IPCore::IPNode * node)
{
    // unlink all rendered image linked to this node
    auto imageRenderer = renderer();
    if (imageRenderer)
        imageRenderer->unlinkNode(node);
}

} // Rv

