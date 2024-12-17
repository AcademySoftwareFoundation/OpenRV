//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCore/Application.h>
#include <IPCore/AudioRenderer.h>
#include <IPCore/ImageRenderer.h>
#include <IPCore/DefaultMode.h>
#include <IPCore/Exception.h>
#include <IPCore/SessionIPNode.h>
#include <IPCore/Session.h>
#include <IPCore/PerFrameAudioRenderer.h>
#include <IPCore/GroupIPNode.h>
#include <IPCore/SoundTrackIPNode.h>
#include <IPCore/IPInstanceNode.h>
#include <IPCore/DisplayGroupIPNode.h>
#include <IPCore/NodeDefinition.h>
#include <IPCore/Profile.h>
#include <IPCore/NodeManager.h>
#include <IPCore/FBCache.h>
#include <TwkApp/Event.h>
#include <TwkContainer/GTOReader.h>
#include <TwkContainer/GTOWriter.h>
#include <TwkContainer/PropertyContainer.h>
#include <TwkMath/Function.h>
#include <TwkMath/Math.h>
#include <TwkMath/Iostream.h>
#include <TwkUtil/EnvVar.h>
#include <TwkUtil/File.h>
#include <TwkUtil/TwkRegEx.h>
#include <TwkUtil/PathConform.h>
#include <TwkUtil/sgcHop.h>
#include <TwkUtil/sgcHopTools.h>
#include <TwkUtil/Clock.h>
#include <Mu/GarbageCollector.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <limits>
#include <sstream>
#include <stl_ext/stl_ext_algo.h>
#include <stl_ext/string_algo.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <numeric>
#include <boost/thread/condition_variable.hpp>
#include <boost/functional/hash.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <iterator>

#ifndef PLATFORM_WINDOWS
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>
#else
#include <windows.h>
#endif

// Flush the audio cache when stopping the playback when ON (default=OFF)
static ENVVAR_BOOL(evFlushAudioCacheWhenStoppingPlayback,
                   "RV_FLUSH_AUDIO_CACHE_WHEN_STOPPING_PLAYBACK", false);

// Disable automatic garbage collection during playback to make sure the
// playback doesn't get interrupted which could cause skipped frames.
// The garbage collection will resume as soon as the playback is stopped.
static ENVVAR_BOOL(evDisableGarbageCollectionDuringPlayback,
                   "RV_DISABLE_GARBAGE_COLLECTION_DURING_PLAYBACK", true);

template <typename T> inline T mod(const T& a, const T& by)
{
    T rem = a % by;
    return rem < T(0) ? rem + by : rem;
}

// NOT A FRAME
#define NAF (std::numeric_limits<int>::min())

//  float worstFpsError = 0.0;

namespace IPCore
{

#if 0
#define DB_GENERAL 0x01
#define DB_EDGES 0x02
#define DB_ALL 0xff

//  #define DB_LEVEL        (DB_ALL & (~ DB_EDGES))
#define DB_LEVEL DB_ALL

#define DB(x)                  \
    if (DB_GENERAL & DB_LEVEL) \
    cerr << "Session: " << x << endl
#define DBL(level, x)     \
    if (level & DB_LEVEL) \
    cerr << "Session: " << x << endl
#else
#define DB(x)
#define DBL(level, x)
#endif

//
// 0 = new vsync estimation/old target frame scheme; default RV 6.2.8, 7.0.0
// 1 = new vsync estimation/new target frame scheme; pending QA on non-60hz
// displays. 2 = old vsync estimation/old target frame scheme; default RV 6.2.7
// and earlier scheme, 7.0.1, 7.1.x
//
#define DEFAULT_AVPLAYBACK_VERSION 2

    using namespace TwkMath;
    using namespace TwkApp;
    using namespace TwkUtil;
    using namespace TwkAudio;
    using namespace TwkContainer;
    using namespace std;
    using namespace boost;

    bool debugProfile = false;
    bool debugPlayback = false;
    bool debugPlaybackVerbose = false;

    Session* Session::m_currentSession = 0;
    float Session::m_maxBufferedWaitSeconds = 1.0;
    float Session::m_cacheLookBehindFraction = 0.0;
    size_t Session::m_maxGreedyCacheSize =
        size_t(8.5 * 1024.0 * 1024.0 * 1024.0);
    size_t Session::m_maxBufferCacheSize =
        size_t(0.5 * 1024.0 * 1024.0 * 1024.0);
    float Session::m_audioDrift =
        1.0; // amount in frames audio allowed to drift before correction
    bool Session::m_usePreEval = false;
    Session::StringVector Session::m_vStrings;
    Session::StringVector Session::m_VStrings;

    NOTIFIER_MESSAGE_IMP(Session, startPlayMessage,
                         "session start play message")
    NOTIFIER_MESSAGE_IMP(Session, stopPlayMessage, "session stop play message")
    NOTIFIER_MESSAGE_IMP(Session, frameChangedMessage,
                         "session frame changed message")
    NOTIFIER_MESSAGE_IMP(Session, updateMessage, "session update message")
    NOTIFIER_MESSAGE_IMP(Session, updateLoadingMessage,
                         "session update loading message")
    NOTIFIER_MESSAGE_IMP(Session, forcedUpdateMessage,
                         "session force update message")
    NOTIFIER_MESSAGE_IMP(Session, fullScreenOnMessage,
                         "session full screen on message")
    NOTIFIER_MESSAGE_IMP(Session, fullScreenOffMessage,
                         "session full screen off message")
    NOTIFIER_MESSAGE_IMP(Session, sessionChangedMessage,
                         "session changed message")
    NOTIFIER_MESSAGE_IMP(Session, stereoHardwareOnMessage,
                         "session stereo hardware on message")
    NOTIFIER_MESSAGE_IMP(Session, stereoHardwareOffMessage,
                         "session stereo hardware off message")
    NOTIFIER_MESSAGE_IMP(Session, audioUnavailbleMessage,
                         "session audio unavailable");
    NOTIFIER_MESSAGE_IMP(Session, eventDeviceChangedMessage,
                         "session event device changed");

    static deque<int> m_frameHistory;

    static unsigned char tinyBlackImage[] = {123, 0, 0, 0};

    static RegEx oldContainerNameRE("^([a-zA-Z0-9]+[^0-9])([0-9][0-9][0-9])$");

    static bool debugGC = false;

    class AuxUserRender : public ImageRenderer::AuxRender
    {
    public:
        AuxUserRender(Session* s)
            : session(s)
        {
        }

        virtual ~AuxUserRender() {}

        virtual void render(VideoDevice::DisplayMode mode, bool leftEye,
                            bool rightEye, bool forController,
                            bool forOutput) const;
        Session* session;
    };

    void AuxUserRender::render(VideoDevice::DisplayMode mode, bool leftEye,
                               bool rightEye, bool forController,
                               bool forOutput) const
    {
        GLPushAttrib attr(GL_ALL_ATTRIB_BITS);
        string contents = (leftEye) ? ((rightEye) ? "both" : "left") : "right";
        double startTime = 0;

        if (debugProfile)
        {
            Session::ProfilingRecord& trecord =
                session->currentProfilingSample();
            startTime = session->profilingElapsedTime();
            if (trecord.userRenderStart == 0.0)
                trecord.userRenderStart = startTime;
        }

        if (forController)
        {
            session->setEventVideoDevice(session->controlVideoDevice());
            session->userRender(session->controlVideoDevice(), "render",
                                contents);
        }

        if (forOutput)
        {
            session->setEventVideoDevice(session->outputVideoDevice());
            session->userRender(session->outputVideoDevice(),
                                "render-output-device", contents);
            session->setEventVideoDevice(session->controlVideoDevice());
        }

        if (debugProfile)
        {
            Session::ProfilingRecord& trecord =
                session->currentProfilingSample();
            double endTime = session->profilingElapsedTime();

            //
            //  Note, this func may be called multiple times per frame, if so we
            //  extend the endTime appropriately for each call (so the end time
            //  is not accurate, but the difference with the start time _does_
            //  reflect the amount of time we spend in this function for this
            //  frame).
            //

            if (trecord.userRenderEnd == 0.0)
                trecord.userRenderEnd = endTime;
            else
                trecord.userRenderEnd += endTime - startTime;
        }
    }

    class AuxAudioRenderer : public ImageRenderer::AuxAudio
    {
    public:
        AuxAudioRenderer(Session* s)
            : AuxAudio()
            , session(s)
        {
        }

        virtual ~AuxAudioRenderer() {}

        virtual void* audioForFrame(int frame, size_t index, size_t& n) const;

        virtual bool isAvailable() const { return session->isUpdating(); }

        // virtual bool isAvailable() const { return session->isPlaying(); }

        Session* session;
    };

    void* AuxAudioRenderer::audioForFrame(int f, size_t seqindex,
                                          size_t& n) const
    {
        n = 0;

        PerFrameAudioRenderer* a =
            dynamic_cast<PerFrameAudioRenderer*>(AudioRenderer::renderer());

        if (!a)
            return 0;

        // If we are not playing and not scrubbing then we don't expect to fill
        // the buffer
        bool silence =
            !session->isPlaying()
            && !(session->audioCachingMode() == Session::GreedyCache);
        PerFrameAudioRenderer::FrameData d =
            a->dataForFrame(f - session->rangeStart(), seqindex, silence);
        n = d.numSamples;

        return d.data;
    };

    class Session::FpsCalculator
    {
    public:
        FpsCalculator(int n);

        void setTargetFps(float fps);
        void addSample(float fps);
        void reset(bool soft = false);
        float fps(double currentTime);

    private:
        double m_lastUpdateTime;
        int m_numSamples;
        int m_maxNumSamples;
        int m_nextSample;
        float m_targetFps;
        float m_fps;
        bool m_firstSample;
        vector<float> m_samples;
    };

    struct Session::FBStatusCheck
    {
        FBStatusCheck(Session::FBStatusVector& vec)
            : statusVec(vec)
        {
        }

        Session::FBStatusVector& statusVec;

        void operator()(IPImage* i)
        {
            if (i && i->fb && i->destination != IPImage::OutputTexture)
            {
                FBStatus status;

                string type;

                if (i->fb->hasAttribute("Type"))
                {
                    type = i->fb->attribute<string>("Type");
                }

                status.partial = i->fb->hasAttribute("PartialImage");
                status.loading = i->fb->hasAttribute("RequestedFrameLoading");
                status.error = type == "Error";
                status.noImage = type == "NoImage";
                status.warning = type == "Warning";
                status.fb = i->fb;

                statusVec.push_back(status);
            }
        }
    };

    static float RV_VSYNC_MAX_INTERVAL = 0.0;
    static float RV_VSYNC_MIN_INTERVAL = 0.0;
    static float RV_VSYNC_RATE_FORCE = 0.0;
    static bool RV_VSYNC_ADD_OFFSET = false;
    static bool RV_VSYNC_IGNORE_DEVICE = false;

    Session::Session(IPGraph* graph)
        : TwkApp::Document()
        , m_waitForUploadThreadPrefetch(false)
        , m_readingGTO(false)
        , m_sessionType(SequenceSession)
        , m_rangeStart(1)
        , m_rangeEnd(2)
        , m_outputVideoDevice(0)
        , m_controlVideoDevice(0)
        , m_narrowedRangeStart(1)
        , m_narrowedRangeEnd(2)
        , m_inPoint(1)
        , m_outPoint(2)
        , m_notPersistent(0)
        , m_inc(1)
        ,
        // m_loadingError(false),
        m_cacheMode(NeverCache)
        , m_displayImage(0)
        , m_preDisplayImage(0)
        , m_proxyImage(0)
        , m_displayFrame(-numeric_limits<int>::max())
        , m_preDisplayFrame(-numeric_limits<int>::max())
        , m_realtimeOverride(false)
        , m_fps(24.0)
        , m_rangeDirty(false)
        , m_playStartFrame(0)
        , m_overhead(0)
        , m_shift(0)
        , m_skipped(0)
        , m_frame(1)
        , m_fastStart(true)
        , m_bufferWait(false)
        , m_latencyWait(false)
        , m_lastFrame(-1)
        , m_lastDevFrame(-1)
        , m_rendering(false)
        , m_wantsRedraw(false)
        , m_fullScreen(false)
        , m_renderer(0)
        , m_realfps(0)
        , m_lastCheckTime(0)
        , m_lastCheckFrame(0)
        , m_receivingEvents(false)
        , m_scrubAudio(false)
        , m_playMode(PlayLoop)
        , m_noSequences(false)
        , m_audioLoopDuration(0)
        , m_audioTimeShift(0)
        , m_audioFirstPass(true)
        , m_audioInitialSample(0)
        , m_audioPlay(false)
        , m_audioUnavailble(false)
        , m_preEval(false)
        , m_wrapping(false)
        , m_fpsCalc(new FpsCalculator(72))
        , m_beingDeleted(false)
        , m_viewStackIndex(-1)
        , m_syncInterval(0)
        , m_syncMaxSamples(10)
        , m_syncLastTime(0)
        , m_syncPredictionEnabled(true)
        , m_syncTargetRefresh(-1.0)
        , m_preFirstNonEmptyRender(false)
        , m_postFirstNonEmptyRender(false)
        , m_batchMode(false)
        , m_nextVSyncTime(0.0)
        , m_renderedVSyncTime(0.0)
        , m_useExternalVSyncTiming(false)
        , m_timerOffset(0.0)
        , m_graph(graph)
        , m_waitingOnSync(false)
        , m_avPlaybackVersion(DEFAULT_AVPLAYBACK_VERSION)
        , m_lastDrawingTime(0)
    {
        if (!m_graph)
            m_graph = new IPGraph(App()->nodeManager());

#ifndef PLATFORM_WINDOWS
        //
        //  Check the per-process limit on open file descriptors and reset the
        //  soft limit to the hard limit.
        //
        //  We do this again here (in addition to in main(), because some Qt
        //  code calls some OSX code that arbitrarilly resets this!
        //
        struct rlimit rlim;
        getrlimit(RLIMIT_NOFILE, &rlim);
        rlim_t target = rlim.rlim_max;
#ifdef PLATFORM_DARWIN
        if (OPEN_MAX < rlim.rlim_max)
            target = OPEN_MAX;
#endif
        rlim.rlim_cur = target;
        setrlimit(RLIMIT_NOFILE, &rlim);
        getrlimit(RLIMIT_NOFILE, &rlim);
        if (rlim.rlim_cur < target)
        {
            cerr << "WARNING: unable to increase open file limit above "
                 << rlim.rlim_cur << endl;
        }
#endif

        if (RV_VSYNC_MAX_INTERVAL == 0.0)
        {
            const char* var = getenv("RV_VSYNC_MAX_INTERVAL");
            if (var)
                RV_VSYNC_MAX_INTERVAL = atof(var);
            else
                RV_VSYNC_MAX_INTERVAL = 120.0;

            var = getenv("RV_VSYNC_MIN_INTERVAL");
            if (var)
                RV_VSYNC_MIN_INTERVAL = atof(var);
            else
                RV_VSYNC_MIN_INTERVAL = 20.0;

            var = getenv("RV_VSYNC_ADD_OFFSET");
            if (var)
                RV_VSYNC_ADD_OFFSET = bool(atoi(var));
            else
                RV_VSYNC_ADD_OFFSET = false;

            var = getenv("RV_VSYNC_RATE_FORCE");
            if (var)
                RV_VSYNC_RATE_FORCE = atof(var);
            else
                RV_VSYNC_RATE_FORCE = 0.0;

            var = getenv("RV_VSYNC_IGNORE_DEVICE");
            if (var)
                RV_VSYNC_IGNORE_DEVICE = bool(atoi(var));
            else
                RV_VSYNC_IGNORE_DEVICE = false;

            if (debugPlayback)
            {
                cerr << "INFO: RV_VSYNC_MAX_INTERVAL " << RV_VSYNC_MAX_INTERVAL
                     << endl;
                cerr << "INFO: RV_VSYNC_MIN_INTERVAL " << RV_VSYNC_MIN_INTERVAL
                     << endl;
                cerr << "INFO: RV_VSYNC_ADD_OFFSET " << RV_VSYNC_ADD_OFFSET
                     << endl;
                cerr << "INFO: RV_VSYNC_RATE_FORCE " << RV_VSYNC_RATE_FORCE
                     << endl;
                cerr << "INFO: RV_VSYNC_IGNORE_DEVICE "
                     << RV_VSYNC_IGNORE_DEVICE << endl;
            }
        }

        // Options& opts = Options::sharedOptions();
        // m_realtime = (opts.playMode == 2);

        Application::OptionMap& optionMap = Application::optionMap();

        if (optionMap.count("playMode") > 0)
        {
            m_realtime = any_cast<int>(optionMap["playMode"]) == 2;
        }
        else
        {
            m_realtime = true;
        }

        if (optionMap.count("loopMode") > 0)
        {
            m_playMode =
                static_cast<PlayMode>(any_cast<int>(optionMap["loopMode"]));
        }

        if (getenv("RV_DEBUG_GC"))
            debugGC = true;

        static unsigned int sessionNumber = 0;

        ostringstream str;
        str << "session" << sessionNumber;
        m_name = str.str();
        sessionNumber++;

        DefaultMode* defaultMode = new DefaultMode(this);
        addMode(defaultMode);
        activateMode(defaultMode);
        m_timer.stop();
        m_stopTimer.stop();

        pthread_mutex_init(&m_audioRendererMutex, 0);
        pthread_mutex_init(&m_rangeDirtyMutex, 0);

        m_graph->setCacheModeSize(IPGraph::GreedyCache, m_maxGreedyCacheSize);
        m_graph->setCacheModeSize(IPGraph::BufferCache, m_maxBufferCacheSize);
        m_graph->setLookBehindFraction(m_cacheLookBehindFraction);
        listenTo(m_graph);

        //
        //  This is returned when a frame is unavailable
        //

        m_proxyImage = new IPImage(0);
        m_proxyImage->fb = new TwkFB::FrameBuffer();
        m_proxyImage->fb->newAttribute("RequestedFrameLoading", true);

        m_errorImage = new IPImage(0);
        m_errorImage->fb = new TwkFB::FrameBuffer();
        m_errorImage->fb->newAttribute("Error", string(""));

//
//  Display thread get's highest priority; audio and caching threads will
//  use lower priorities.
//
#ifdef PLATFORM_WINDOWS
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
#endif

        if (debugProfile)
            m_graph->setProfilingClock(&m_profilingTimer);

        if (const char* syncEnable = getenv("RV_VSYNC_NO_SYNC_PREDICTION"))
        {
            cout << "INFO: sync prediction disabled" << endl;
            m_syncPredictionEnabled = false;
        }

        if (m_VStrings.empty())
        {
            if (const char* pairs = getenv("RV_STEREO_NAME_PAIRS"))
            {
                vector<string> tokens;
                stl_ext::tokenize(tokens, pairs, ":");

                if (tokens.size() % 2 != 0)
                    tokens.pop_back();

                for (size_t i = 0; i < tokens.size(); i += 2)
                {
                    m_VStrings.push_back(tokens[i]);
                    m_VStrings.push_back(tokens[i + 1]);
                }
            }
            else
            {
                m_VStrings.push_back("left");
                m_VStrings.push_back("right");
                m_VStrings.push_back("Left");
                m_VStrings.push_back("Right");
            }
        }

        if (m_vStrings.empty())
        {
            if (const char* pairs = getenv("RV_STEREO_CHAR_PAIRS"))
            {
                vector<string> tokens;
                stl_ext::tokenize(tokens, pairs, ":");

                if (tokens.size() % 2 != 0)
                    tokens.pop_back();

                for (size_t i = 0; i < tokens.size(); i += 2)
                {
                    m_vStrings.push_back(tokens[i]);
                    m_vStrings.push_back(tokens[i + 1]);
                }
            }
            else
            {
                m_vStrings.push_back("L");
                m_vStrings.push_back("R");
                m_vStrings.push_back("l");
                m_vStrings.push_back("r");
            }
        }

        //
        // This env var is really for debugging and early bird user testing
        // of changes we might make to AV playback; for example updates
        // to targetFrame().
        //
        if (const char* avPlaybackVersionStr = getenv("RV_AVPLAYBACK_VERSION"))
        {
            m_avPlaybackVersion = atoi(avPlaybackVersionStr);
        }

        if (debugPlayback)
        {
            cerr << "INFO: RV_AVPLAYBACK_VERSION " << m_avPlaybackVersion
                 << endl;
        }

        //
        // This env var is to enable/disable the closing of the audio when
        // the audio device is stopped during stop "turn-around" event.
        // NB: This only applies if preRoll mode is enabled.
        //
        if (const char* enableFastTurnAroundStr =
                getenv("RV_ENABLE_FAST_TURNAROUND"))

        {
            m_enableFastTurnAround =
                (atoi(enableFastTurnAroundStr) ? true : false);
        }
        else
        {
            m_enableFastTurnAround = true;
        }

        if (debugPlayback)
        {
            cerr << "INFO: RV_ENABLE_FAST_TURNAROUND "
                 << (int)m_enableFastTurnAround << endl;
        }

        setRendererType("Composite");
        setDebugOptions();
    }

    Session::~Session()
    {
        m_beingDeleted = true;

        breakVideoDeviceConnections();

        stop();
        stopCompletely();
        setCaching(Session::NeverCache);
        setRendererType("None");

        //
        //  Shut down the caching thread
        //

        clear();

        delete m_renderer;
        delete m_proxyImage;
        delete m_errorImage;
        delete m_fpsCalc;

        m_renderer = 0;
        m_proxyImage = 0;
        m_errorImage = 0;
        m_fpsCalc = 0;

        if (audioRenderer())
        {
            stopAudio();
            audioRenderer()->shutdownOnLast();
            resetAudio();
        }

        pthread_mutex_destroy(&m_audioRendererMutex);
        pthread_mutex_destroy(&m_rangeDirtyMutex);

        delete m_graph;
    }

    void Session::breakVideoDeviceConnections()
    {
        if (m_outputVideoDevice)
        {
            breakConnection(const_cast<VideoDevice*>(m_outputVideoDevice));
            m_outputVideoDevice = 0;
        }

        if (m_controlVideoDevice)
        {
            breakConnection(const_cast<VideoDevice*>(m_controlVideoDevice));
            m_controlVideoDevice = 0;
        }
    }

    void Session::setFilterLiveReviewEvents(bool shouldFilterEvents)
    {
        m_filterLiveReviewEvents = shouldFilterEvents;
    }

    bool Session::filterLiveReviewEvents() { return m_filterLiveReviewEvents; }

    void Session::setName(const string& n) { m_name = n; }

    string Session::name() const { return m_name; }

    void Session::setDebugOptions()
    {
        StringVector dflags = Application::optionValue("debug", StringVector());

        for (size_t i = 0; i < dflags.size(); i++)
        {
            const string& name = dflags[i];

            if (name == "events")
                TwkApp::Document::debugEvents();
            else if (name == "threads")
                stl_ext::thread_group::debug_all(true);
            else if (name == "gpu")
                ImageRenderer::reportGL(true);
            else if (name == "audio")
                AudioRenderer::setDebug(true);
            else if (name == "audioverbose")
            {
                AudioRenderer::setDebug(true);
                AudioRenderer::setDebugVerbose(true);
            }
            else if (name == "dumpaudio")
                AudioRenderer::setDumpAudio(true);
            else if (name == "shadercode")
            {
                Shader::setDebugging(Shader::ShaderCodeDebugInfo);
            }
            else if (name == "shaders")
            {
                Shader::setDebugging(Shader::AllDebugInfo);
            }
            else if (name == "profile")
                debugProfile = true;
            else if (name == "playback")
                debugPlayback = true;
            else if (name == "playbackverbose")
                debugPlaybackVerbose = debugPlayback = true;
            else if (name == "cache")
                TwkFB::Cache::debug() = true;
            else if (name == "dtree")
                IPGraph::setDebugTreeOutput(true);
            else if (name == "passes")
                ImageRenderer::setPassDebug(true);
            else if (name == "imagefbo")
                ImageRenderer::setOutputIntermediateDebug(true);
            else if (name == "nogpucache")
                ImageRenderer::setNoGPUCache(true);
            else if (name == "imagefbolog")
                ImageRenderer::setIntermediateLogging(true);
            else if (name == "nodes")
                IPCore::NodeManager::setDebug(true);
        }
    }

    void Session::setGlobalAudioOffset(float o, bool internal)
    {
        IPGraph::NodeVector nodes;
        graph().findNodesByTypeName(nodes, "RVSoundTrack");
        bool changed = false;

        for (size_t i = 0; i < nodes.size(); i++)
        {
            if (SoundTrackIPNode* snode =
                    dynamic_cast<SoundTrackIPNode*>(nodes[i]))
            {
                if (internal)
                {
                    if (snode->setInternalOffset(o))
                        changed = true;
                }
                else if (TwkContainer::FloatProperty* fp =
                             snode->property<FloatProperty>("audio.offset"))
                {
                    if (fp->front() != o)
                    {
                        fp->resize(1);
                        (*fp)[0] = o;
                        changed = true;
                    }
                }
            }
        }

        if (changed)
            graph().flushAudioCache();
    }

    void Session::setGlobalSwapEyes(bool b)
    {
        IPGraph::NodeVector nodes;
        graph().findNodesByTypeName(nodes, "RVDisplayStereo");

        for (size_t i = 0; i < nodes.size(); i++)
        {
            if (TwkContainer::IntProperty* ip =
                    nodes[i]->property<IntProperty>("stereo.swap"))
            {
                ip->resize(1);
                (*ip)[0] = b ? 1 : 0;
            }
        }
    }

    AudioRenderer* Session::audioRenderer() const
    {
        if (m_audioUnavailble || !hasAudio())
            return 0;
        return AudioRenderer::renderer();
    }

    void Session::resetAudio()
    {
        // m_audioUnavailble = true;
    }

    bool Session::isPlayingAudio() { return !audioFirstPass() && m_audioPlay; }

    void Session::audioVarLock() { pthread_mutex_lock(&m_audioRendererMutex); }

    void Session::audioVarUnLock()
    {
        pthread_mutex_unlock(&m_audioRendererMutex);
    }

    void Session::setAudioTimeShift(double d, double sensitivity)
    {
        //
        //  Don't allow the audio time shift to vary greatly over time to
        //  prevent jumping around too much. Usually it will converge to a
        //  nice value. Note that where the time shift is actually used
        //  there is also snapping so that only one increments of device
        //  hz are used.
        //
        if (audioTimeShiftValid() && d != std::numeric_limits<double>::max())
        {
            m_audioTimeShift =
                d * sensitivity + m_audioTimeShift * (1.0 - sensitivity);
        }
        else
        {
            m_audioTimeShift = d;
        }
    }

    void Session::playAudio()
    {
        m_audioPlay = true;
        setAudioTimeShift(std::numeric_limits<double>::max());
        if (audioRenderer())
            audioRenderer()->play(this);
    }

    void Session::stopAudio(const string& eventData)
    {
        if (audioRenderer())
        {
            if (m_enableFastTurnAround && m_playMode != PlayOnce
                && eventData == "turn-around")
            {
                audioRenderer()->stopOnTurnAround(this);
            }
            else
            {
                audioRenderer()->stop(this);
            }
        }

        // Note that systematically flushing the audio cache when stopping the
        // playback impacts performances especially when the audio does not
        // reside on local storage.
        // Making this behaviour optionally available via an environment
        // variable in the eventuality this could still be an issue for some
        // users.
        if (evFlushAudioCacheWhenStoppingPlayback.getValue())
        {
            // We flush the audio cache on stop because
            // the audio cache filled during the play maybe be
            // discontinuous with respect to new start play frame
            // because it was populated with  data in the reverse direction
            // or at start frames forward in time which can result in
            // discontinutities if the source != device rate.
            //
            if (m_audioPlay && audioCachingMode() == BufferCache)
                graph().flushAudioCache();
        }

        m_audioPlay = false;
    }

    Session::AudioRange Session::calculateAudioRange(double rate) const
    {
        bool playBackwards = (inc() < 0);
        SampleTime sessionStartSample =
            (playBackwards)
                ? timeToSamples(double(m_frame - rangeStart() + 1) / fps(),
                                rate)
                : timeToSamples(double(m_frame - rangeStart()) / fps(), rate);
        SampleTime sessionEndSample =
            timeToSamples(double(rangeEnd() - rangeStart()) / fps(), rate) - 1;

        return AudioRange(sessionStartSample, sessionEndSample);
    }

    void Session::audioConfigure()
    {
        const AudioRenderer::DeviceState& state =
            audioRenderer()->deviceState();
        AudioRange range = calculateAudioRange(state.rate);
        IPCore::IPGraph::AudioConfiguration config(
            state.rate, state.layout,
            state.framesPerBuffer / TwkAudio::channelsCount(state.layout),
            fps(), (inc() < 0), range.first, range.second);

        graph().audioConfigure(config);
    }

    void Session::setAudioFirstPass(bool b) { m_audioFirstPass = b; }

    void Session::ensureAudioRenderer()
    {
        try
        {
            audioRenderer();
        }
        catch (...)
        {
            send(audioUnavailbleMessage());
        }
    }

    void Session::setRendererBGType(unsigned int v)
    {
        m_renderer->setBGPattern((ImageRenderer::BGPattern)v);
        userGenericEvent("bg-pattern-change", "");
        m_bgChangedSignal();
    }

    void Session::setRendererType(const std::string& type)
    {
        if (m_renderer && m_renderer->name() == type)
            return;

        delete m_renderer;
        m_renderer = 0;

        if (type == "Composite")
        {
            m_renderer = new ImageRenderer("Composite");
        }
        // else if (type == "Direct")
        // {
        //     m_renderer = new DrawPixelsRenderer(m_fb);
        // }
        else if (type == "None")
        {
            /* Do nothing */
        }
        else
        {
            cerr << "WARNING: requested unknown renderer type " << type
                 << " using Composite" << endl;
            m_renderer = new ImageRenderer("Composite");
        }
    }

    void Session::chooseNextBestRenderer()
    {
        if (!m_renderer)
        {
            setRendererType("Composite");
        }

#if 0
    while (!m_renderer->supported() && m_renderer->nextBestRenderer() != "")
    {
        setRendererType(m_renderer->nextBestRenderer());
    }
#endif
    }

    void Session::setEventVideoDevice(const VideoDevice* d)
    {
        if (d != m_eventVideoDevice)
        {
            m_eventVideoDevice = d;
            send(eventDeviceChangedMessage());
        }
    }

    void Session::setControlVideoDevice(const VideoDevice* d)
    {
        if (m_controlVideoDevice && m_controlVideoDevice != d)
        {
            breakConnection(const_cast<VideoDevice*>(m_controlVideoDevice));
        }

        //
        //  Tell the graph about the new device
        //

        if (d)
            graph().deviceChanged(m_controlVideoDevice, d);
        graph().programFlush();

        m_controlVideoDevice = d;
        m_eventVideoDevice = d;
        m_renderer->setControlDevice(d);

        if (!m_outputVideoDevice)
            setOutputVideoDevice(d);
        if (d)
            listenTo(const_cast<VideoDevice*>(d));
    }

    void Session::setOutputVideoDevice(const VideoDevice* d)
    {
        const VideoDevice* old = m_outputVideoDevice;

        try
        {
            if (d != old)
                setGlobalAudioOffset(0.0f, true);

            if (old && old != m_controlVideoDevice && old->audioOutputEnabled())
            {
                if (d != old)
                    AudioRenderer::popPerFrameModule();

                if (d == m_controlVideoDevice)
                {
                    graph().disconnectDisplayGroup(old);
                }
            }

            m_outputVideoDevice = d;
            m_renderer->setOutputDevice(d);

            if (d)
            {
                if (old && old != m_controlVideoDevice)
                {
                    graph().disconnectDisplayGroup(old);
                }

                if (d != m_controlVideoDevice)
                {
                    graph().connectDisplayGroup(d);

                    //
                    //  Zero margins on presentation device, since we don't know
                    //  what modes might be drawing in the margins.  Each mode
                    //  will then expand its margin as necessary.
                    //
                    d->setMargins(Margins());
                }

                graph().setDevice(m_controlVideoDevice, m_outputVideoDevice);

                m_realtimeOverride =
                    m_outputVideoDevice != m_controlVideoDevice;

                if (d->audioOutputEnabled())
                {
                    if (d != old)
                    {
                        VideoDevice::AudioFormat aformat =
                            d->audioFormatAtIndex(d->currentAudioFormat());
                        AudioRenderer::RendererParameters params;

                        VideoDevice::AudioFrameSizeVector sizes;
                        d->audioFrameSizeSequence(sizes);
                        size_t totalFrames =
                            accumulate(sizes.begin(), sizes.end(), 0);

                        params.holdOpen = true;
                        params.hardwareLock = true;
                        params.format = aformat.format;
                        params.rate = aformat.hz;
                        params.layout = aformat.layout;
                        params.framesPerBuffer = totalFrames;
                        params.frameSizes = sizes;

                        if (AudioRenderer::debug)
                        {
                            cerr << "PerFrame AUDIO: init params.device="
                                 << params.device << endl;
                            cerr << "PerFrameAUDIO: init params.format="
                                 << (int)params.format << " ("
                                 << TwkAudio::formatString(params.format) << ")"
                                 << endl;
                            cerr << "PerFrameAUDIO: init params.rate="
                                 << (int)params.rate << endl;
                            cerr << "PerFrameAUDIO: init params.layout="
                                 << (int)params.layout << " ("
                                 << TwkAudio::channelsCount(params.layout)
                                 << " channels)" << endl;
                            cerr
                                << "PerFrameAUDIO: init params.framesPerBuffer="
                                << (int)params.framesPerBuffer << endl;
                        }

                        AudioRenderer::pushPerFrameModule(params);
                    }
                }
                else if (m_realtimeOverride && d->useLatencyForAudio())
                {
                    if (d != old)
                        setGlobalAudioOffset(d->totalLatencyInSeconds(), true);
                }
            }
            else
            {
                if (d != old)
                    setGlobalAudioOffset(0, true);
            }
        }
        catch (std::exception& exc)
        {
            m_outputVideoDevice = 0;

            if (old == m_controlVideoDevice && d != old)
            {
                setOutputVideoDevice(m_controlVideoDevice);
            }

            throw;
        }

        ostringstream s;
        if (m_outputVideoDevice != m_controlVideoDevice && m_outputVideoDevice)
        {
            s << m_outputVideoDevice->module()->name() << "/"
              << m_outputVideoDevice->name();
        }

        if (old != m_outputVideoDevice)
        {
            userGenericEvent("output-video-device-changed", s.str());
            m_outputVideoDeviceChangedSignal(m_outputVideoDevice);
        }
    }

    void Session::clearVideoDeviceCaches()
    {
        if (m_controlVideoDevice)
            m_controlVideoDevice->clearCaches();
        if (m_outputVideoDevice)
            m_outputVideoDevice->clearCaches();
        if (m_renderer)
            m_renderer->clearState();
    }

    void Session::deviceSizeChanged(const VideoDevice* d)
    {
        if (d == m_outputVideoDevice || d == m_controlVideoDevice)
        {
            m_renderer->flushImageFBOs();
        }

        if (d == m_outputVideoDevice)
        {
            setOutputVideoDevice(d);
        }
    }

    void Session::copySessionStateToNode(IPNode* node)
    {
        IntProperty* mp = node->createProperty<IntProperty>("session.marks");
        mp->resize(markedFrames().size());

        copy(markedFrames().begin(), markedFrames().end(), mp->begin());

        //
        //  We want the in/out to expand as inputs are added so only save it if
        //  it's been modified from the default rangeStart/End.
        //

        if (inPoint() != rangeStart() || outPoint() != rangeEnd())
        {
            node->declareProperty<Vec2iProperty>("session.region",
                                                 Vec2i(inPoint(), outPoint()));
        }
        else if (Vec2iProperty* rp =
                     node->property<Vec2iProperty>("session.region"))
        {
            node->removeProperty(rp);
        }

        node->declareProperty<IntProperty>("session.frame", currentFrame());
        node->declareProperty<FloatProperty>("session.fps", fps());
    }

    void Session::copyFullSessionStateToContainer(PropertyContainer& pc)
    {
        pc.declareProperty<StringProperty>("session.viewNode", viewNodeName());
        pc.declareProperty<Vec2iProperty>("session.range",
                                          Vec2i(rangeStart(), rangeEnd()));
        pc.declareProperty<Vec2iProperty>("session.region",
                                          Vec2i(inPoint(), outPoint()));
        pc.declareProperty<FloatProperty>("session.fps", fps());
        pc.declareProperty<IntProperty>("session.realtime", realtime());
        pc.declareProperty<IntProperty>("session.inc", inc());
        pc.declareProperty<IntProperty>("session.currentFrame", currentFrame());

        IntProperty* mp = pc.createProperty<IntProperty>("session.marks");

        for (int i = 0; i < markedFrames().size(); i++)
        {
            mp->push_back(markedFrames()[i]);
        }
    }

    void Session::copyConnectionsToContainer(PropertyContainer& cons)
    {
        StringPairProperty* connections =
            cons.createProperty<StringPairProperty>("evaluation.connections");
        StringProperty* roots =
            cons.createProperty<StringProperty>("evaluation.roots");
        StringProperty* top = cons.createProperty<StringProperty>("top.nodes");

        // only get top-level connections
        graph().nodeConnections(connections->valueContainer(), true);
        const IPGraph::NodeMap& viewableNodes = graph().viewableNodes();

        const IPNode::IPNodes& rootInputs = graph().root()->inputs();

        for (size_t i = 0; i < rootInputs.size(); i++)
        {
            if (rootInputs[i]->isWritable())
                roots->push_back(rootInputs[i]->name());
        }

        for (NodeMap::const_iterator i = viewableNodes.begin();
             i != viewableNodes.end(); ++i)
        {
            top->push_back(i->first);
        }
    }

    void Session::setSessionStateFromNode(IPNode* node)
    {
        IPNode::ImageRangeInfo info = graph().root()->imageRangeInfo();
        setFrameRange(info.start, info.end + 1);

        if (IntProperty* mp = node->property<IntProperty>("session.marks"))
        {
            m_markedFrames.resize(mp->size());
            copy(mp->begin(), mp->end(), m_markedFrames.begin());
        }
        else
        {
            m_markedFrames.clear();
        }

        if (Vec2iProperty* rp = node->property<Vec2iProperty>("session.region"))
        {
            setInPoint(rp->front().x);
            setOutPoint(rp->front().y);
        }
        else if (info.cutOut >= info.cutIn)
        {
            //
            //  No region property, which means we never "switched away" from
            //  this view interactively.  In this case the best guess is
            //  probably to use whatever in/out the node itself exports.  In
            //  most cases this will be the same as start/end, but for source
            //  nodes, the in/out may be useful.
            //

            setInPoint(info.cutIn);
            setOutPoint(info.cutOut + 1);
        }

        if (IntProperty* mp = node->property<IntProperty>("session.frame"))
        {
            setFrameInternal(mp->front());
        }
        else
        {
            setFrameInternal(rangeStart());
        }

        if (FloatProperty* fpsp =
                node->createProperty<FloatProperty>("session.fps"))
        {
            if (fpsp->empty() || fpsp->front() == 0.0)
            {
                fpsp->resize(1);
                fpsp->front() = info.fps;
            }

            if (fpsp->front() > 0.0)
                setFPS(fpsp->front());
        }
    }

    string Session::viewNodeName() const
    {
        if (graph().viewNode())
        {
            return graph().viewNode()->name();
        }
        else
        {
            return "";
        }
    }

    bool Session::setViewNode(const std::string& nodeName, bool force)
    {
        checkInPreDisplayImage();
        checkInDisplayImage();

        IPNode* node = graph().findNode(nodeName);
        if (!node)
            node = graph().findNodeByUIName(nodeName);

        if (node)
        {
            IPNode* oldNode = graph().viewNode();
            if (node != oldNode || force)
            {
                if (oldNode && node != oldNode)
                    copySessionStateToNode(oldNode);
                userGenericEvent("before-graph-view-change", nodeName);
                m_beforeGraphViewChangeSignal(node);
                graph().setViewNode(node);

                if (oldNode && node->name() == previousViewNode())
                    --m_viewStackIndex;
                else if (oldNode && node->name() == nextViewNode())
                    ++m_viewStackIndex;
                else if (oldNode != node)
                {
                    if (oldNode && m_viewStackIndex == -1)
                    {
                        m_viewStack.resize(0);
                        m_viewStack.push_back(oldNode->name());
                    }
                    else
                        m_viewStack.resize(m_viewStackIndex + 1);
                    m_viewStackIndex = m_viewStack.size();
                    m_viewStack.push_back(node->name());
                }

                if (node != oldNode)
                    setSessionStateFromNode(graph().viewNode());
                userGenericEvent("after-graph-view-change", nodeName);
                m_afterGraphViewChangeSignal(node);

                // We need to clear the audio cache if the view node has been
                // changed since the last audio play
                graph().requestClearAudioCache();
            }

            return true;
        }
        else
        {
            return false;
        }
    }

    std::string Session::nextViewNode()
    {
        if (m_viewStackIndex >= 0 && m_viewStackIndex < m_viewStack.size() - 1
            && graph().findNode(m_viewStack[m_viewStackIndex + 1]))
        {
            return m_viewStack[m_viewStackIndex + 1];
        }

        return "";
    }

    std::string Session::previousViewNode()
    {
        if (m_viewStackIndex > 0 && m_viewStackIndex <= m_viewStack.size() - 1
            && graph().findNode(m_viewStack[m_viewStackIndex - 1]))
        {
            return m_viewStack[m_viewStackIndex - 1];
        }

        return "";
    }

    IPNode* Session::newNode(const std::string& typeName,
                             const std::string& nodeName)
    {
        return graph().newNode(typeName, nodeName);
    }

    void Session::setSessionType(SessionType type)
    {
        // nothing anymore
    }

    void Session::makeActive()
    {
        m_currentSession = this;
        Document::makeActive();
    }

    void Session::forceNextEvaluation()
    {
        // m_stateFrame = numeric_limits<int>::min();
    }

    TwkMath::Time Session::elapsedPlaySeconds()
    {
        if (m_avPlaybackVersion == 2)
        {
            const double e = elapsedPlaySecondsRaw();

            if (audioRenderer() && hasAudio())
            {
                audioVarLock();
                double ts = audioTimeShift() - audioRenderer()->preRollDelay();
                audioVarUnLock();

                return e + ts;
            }
            else
            {
                return e;
            }
        }
        else
        {
            const double e = elapsedPlaySecondsRaw();

            if (audioRenderer() && hasAudio())
            {
                //
                //  Make the audio time shift in "sync" units. In other words:
                //  we only allow the audio to shift one sync's worth of time.
                //  If we do less or more the frame drift will result in
                //  stuttering.
                //

                const Time hz = outputDeviceHz();
                double ts = 0.0;

                audioVarLock();
                if (audioTimeShiftValid())
                {
#ifdef PLATFORM_LINUX
                    if (audioRenderer()->hasHardwareLock())
                    {
                        ts = std::floor((audioTimeShift()
                                         - audioRenderer()->preRollDelay())
                                        * hz)
                                 / hz
                             - m_timerOffset;
                    }
                    else
                    {
                        ts = std::floor((audioTimeShift()
                                         - audioRenderer()->preRollDelay())
                                        * hz)
                             / hz;
                    }
#else
                    ts = std::floor((audioTimeShift()
                                     - audioRenderer()->preRollDelay())
                                    * hz)
                             / hz
                         - m_timerOffset;
#endif
                }
                audioVarUnLock();

                return e + ts;
            }
            else
            {
                return e - m_timerOffset;
            }
        }
    }

    void Session::setInc(int i)
    {
        if (i == 0)
            i = 1;

        if (isPlaying())
        {
            stop();
            m_fastStart = true;
            m_inc = i;
            play();
        }
        else
        {
            m_inc = i;
            updateGraphInOut(false);
        }

        userGenericEvent("play-inc", "");
        m_playIncChangedSignal();
    }

    IPNode* Session::node(const std::string& name) const
    {
        return graph().findNode(name);
    }

    void Session::updateRange()
    {
        //
        //  This may need to specifically check all viewable nodes and see
        //  if they need the in/outs reset. Or at least any "smart" nodes.
        //

        // Reset the in/out points when they match the range or when they are
        // invalid
        const bool resetInOut =
            (inPoint() == rangeStart() && outPoint() == rangeEnd())
            || (inPoint() >= outPoint());

        //
        //  this will cause threads to stop. Its necessary because we're going
        //  to be doing a range evaluation and its basically not safe to have
        //  them running.
        //
        graph().beginGraphEdit();

        IPNode::ImageRangeInfo range = graph().root()->imageRangeInfo();
        setFrameRange(range.start, range.end + 1);

        graph().requestClearAudioCache();
        graph().endGraphEdit();

        if (resetInOut)
        {
            setInPoint(rangeStart());
            setOutPoint(rangeEnd());
        }

        if (currentFrame() < range.start || currentFrame() > range.end)
        {
            setFrameInternal(range.start);
        }

        lockRangeDirty();
        m_rangeDirty = false;
        unlockRangeDirty();
    }

    void Session::deleteNode(IPNode* node)
    {
        if (node == graph().viewNode())
        {
            const IPGraph::NodeMap& vnodes = graph().viewableNodes();

            if (vnodes.size() > 1)
            {
                if (previousViewNode() != "")
                    setViewNode(previousViewNode());
                else
                {
                    IPGraph::NodeMap::const_iterator i =
                        vnodes.find(node->name());
                    ++i;
                    if (i == vnodes.end())
                        i = vnodes.begin();
                    setViewNode((*i).second->name());
                }
            }
            else
            {
                //?
            }
        }
        string name = node->name();

        userGenericEvent("before-node-delete", name);

        graph().beginGraphEdit();
        try
        {
            graph().deleteNode(node);
        }
        catch (...)
        {
            graph().endGraphEdit();
            throw;
        }

        graph().requestClearAudioCache();
        graph().endGraphEdit();

        updateRange();
        clearVideoDeviceCaches();
        askForRedraw();

        userGenericEvent("after-node-delete", name);
    }

    void Session::updateGraphInOut(bool flushAudio)
    {
        graph().setCachingMode(graph().cachingMode(), inPoint(), outPoint(),
                               rangeStart(), rangeEnd(), inc(), currentFrame(),
                               fps());

        if (debugPlaybackVerbose)
        {
            cout << "DEBUG: updateGraphInOut("
                 << (flushAudio ? "flushAudio" : "") << ")" << endl;
        }

        if (flushAudio && graph().isAudioConfigured())
        {
            const IPCore::IPGraph::AudioConfiguration& oldConfig =
                graph().lastAudioConfiguration();
            IPCore::IPGraph::AudioConfiguration newConfig = oldConfig;

            AudioRange range = calculateAudioRange(oldConfig.rate);
            newConfig.startSample = range.first;
            newConfig.endSample = range.second;
            newConfig.fps = fps();
            newConfig.backwards = (inc() < 0);

            if (newConfig.fps != oldConfig.fps
                || newConfig.backwards != oldConfig.backwards
                || newConfig.startSample != oldConfig.startSample
                || newConfig.endSample != oldConfig.endSample)
            {
                if (debugPlaybackVerbose)
                    cout << "DEBUG: audioConfigure" << endl;
                graph().requestClearAudioCache();
                graph().audioConfigure(newConfig);
            }
        }
    }

    void Session::setOutPoint(int frame)
    {
        if (frame > m_narrowedRangeEnd)
            frame = m_narrowedRangeEnd;
        bool sendEvent = (m_outPoint != frame);
        m_outPoint = frame;
        updateGraphInOut();

        if (sendEvent)
        {
            ostringstream str;
            str << frame;
            userGenericEvent("new-out-point", str.str());

            m_outPointChangedSignal(m_outPoint);
        }
    }

    void Session::setInPoint(int frame)
    {
        if (frame < m_narrowedRangeStart)
            frame = m_narrowedRangeStart;
        bool sendEvent = (m_inPoint != frame);
        m_inPoint = frame;
        updateGraphInOut();

        if (sendEvent)
        {
            ostringstream str;
            str << frame;
            userGenericEvent("new-in-point", str.str());

            m_inPointChangedSignal(m_inPoint);
        }
    }

    int Session::inPoint() const { return m_inPoint; }

    int Session::outPoint() const { return m_outPoint; }

    int Session::rangeStart() const { return m_rangeStart; }

    int Session::rangeEnd() const { return m_rangeEnd; }

    int Session::narrowedRangeStart() const { return m_narrowedRangeStart; }

    int Session::narrowedRangeEnd() const { return m_narrowedRangeEnd; }

    void Session::setFrameRange(int start, int end)
    {
        m_rangeStart = start;
        m_rangeEnd = end;
        m_narrowedRangeStart = start;
        m_narrowedRangeEnd = end;

        if (m_frame < start)
            m_frame = start;
        if (m_frame >= end)
            m_frame = end - 1;
        if (m_inPoint < m_rangeStart)
            m_inPoint = m_rangeStart;
        if (m_outPoint >= m_rangeEnd)
            m_outPoint = m_rangeEnd;
        updateGraphInOut();
        userGenericEvent("range-changed", "");

        m_rangeChangedSignal(start, end);
    }

    void Session::setNarrowedRange(int start, int end)
    {
        if (start < m_rangeStart)
            start = m_rangeStart;
        if (end > m_rangeEnd)
            end = m_rangeEnd;
        m_narrowedRangeStart = start;
        m_narrowedRangeEnd = end;
        if (m_inPoint < m_narrowedRangeStart)
            setInPoint(m_narrowedRangeStart);
        if (m_outPoint > m_narrowedRangeEnd)
            setOutPoint(m_narrowedRangeEnd);
        userGenericEvent("narrowed-range-changed", "");

        m_rangeNarrowedSignal(start, end);
    }

    double Session::currentTargetFPS() const
    {
        // double rate = graph().cache().fpsAtFrame(m_frame);
        // double rate = 0.0;
        // if (rate > 0.0) return rate;
        return fps();
    }

    void Session::clear()
    {
        if (!m_beingDeleted)
        {
            userGenericEvent("before-clear-session", "");
            m_beforeSessionClearSignal();
        }

        if (m_renderer)
            m_renderer->clearState();
        clearHistory();

        //
        //  Give code a chance to react to the viewnode changing away
        //  from the current, and to the defaultSequence.  If we do this
        //  later, the viewnode will already have changed
        //
        setViewNode("defaultSequence", true);

        stop();
        m_timer.stop();
        m_userTimer.stop();
        m_stopTimer.stop();
        m_markedFrames.clear();
        m_sessionType = SequenceSession;

        checkInDisplayImage();

        m_inc = 1;
        setCaching(NeverCache);
        graph().cache().lock();
        graph().cache().clearAllButFrame(currentFrame(), true);
        graph().cache().unlock();
        graph().setAudioCachingMode(IPGraph::BufferCache);
        graph().flushAudioCache();
        if (!m_beingDeleted)
            graph().reset(App()->videoModules());
        setFileName("Untitled");
        setInPoint(1);
        setOutPoint(2);
        setRangeStart(1);
        setRangeEnd(2);
        setFrameInternal(1);

        if (!m_beingDeleted)
        {
            clearVideoDeviceCaches();
            send(sessionChangedMessage());
            if (graph().viewNode())
                setViewNode(graph().viewNode()->name(), true);

            userGenericEvent("after-clear-session", "");

            m_afterSessionClearSignal();

            if (m_controlVideoDevice != m_outputVideoDevice)
            {
                graph().connectDisplayGroup(m_controlVideoDevice);
                graph().setDevice(m_controlVideoDevice, m_outputVideoDevice);
            }
        }
    }

    void Session::setRealtime(bool r)
    {
        if (r == m_realtime)
            return;
        m_realtime = r;
        bool playing = isPlaying();

        if (playing)
            stop();

        if (m_realtime)
            userGenericEvent("realtime-play-mode", "");
        else
            userGenericEvent("play-all-frames-mode", "");

        m_realtimeModeChangedSignal();

        if (playing)
            play();
    }

    void Session::setPlayMode(PlayMode mode)
    {
        if (mode != m_playMode)
        {
            m_playMode = mode;
            updateGraphInOut();
            userGenericEvent("play-mode-changed", std::to_string((int)mode));
            m_playModeChangedSignal();
        }
    }

    bool Session::isCaching() const
    {
        //
        //  Cache threads now run forever (sleeping if they have nothing
        //  to do.  So the relevent condition is running and not full.
        //
        return graph().isCacheThreadRunning();
    }

    void Session::scrubAudio(bool b, Time duration, int count)
    {
        // if (b == m_scrubAudio) return;

        //
        //  This is needed because scrubbing may get turned on before the
        //  first play. Since the audio is lazily built it will not exist
        //  yet and may cause trouble in one of the readers (since its
        //  audioConfigure() would not yet have been called).
        //

        bool playing = isPlaying();
        m_scrubAudio = b;

        audioVarLock();
        setAudioLoopDuration(duration);
        setAudioLoopCount(count);
        audioVarUnLock();

        if (m_scrubAudio)
        {
            if (audioRenderer() && hasAudio())
            {
                if (!isPlayingAudio())
                {
                    try
                    {
                        playAudio();
                    }
                    catch (std::exception& exc)
                    {
                        resetAudio();
                    }
                }
            }
        }
        else
        {
            if (!isPlaying())
            {
                if (isPlayingAudio())
                {
                    try
                    {
                        stopAudio();
                    }
                    catch (std::exception& exc)
                    {
                        resetAudio();
                    }
                }
            }
        }
    }

    static float elapsedOffset = 0.0;

    void Session::play(string eventData)
    {
        if (debugPlayback)
        {
            m_frameHistory.clear();
            m_frameRuns.clear();
            m_lastFData.clear();
            m_framePatternFailCount = 0;
        }

        if (m_avPlaybackVersion == 2)
        {
            return play_v2(eventData);
        }
        else
        {
            return play_v1(eventData);
        }
    }

    void Session::play_v2(string& eventData)
    {
        //  worstFpsError = 0.0;
        //  Mu::GarbageCollector::disable();
        //
        //  Update the graph's in/out, etc.
        //

        //
        //  Make sure graph in/out points are up-to-date, but only if we are
        //  starting from a real stop.
        //
        if (eventData != "buffering")
        {
            updateGraphInOut(
                false); // flushAudio=false (not needed, and causes inf loop)
        }
        m_preEval = m_usePreEval;
        if (isPlaying() || rangeStart() == (rangeEnd() - 1))
            return;
        m_bufferWait = false;
        elapsedOffset = 0.0;

        bool isOutputDevice =
            multipleVideoDevices() && outputVideoDevice()->hasClock();

        if (m_playMode == PlayOnce)
        {
            if (m_frame == outPoint() - 1 && m_inc > 0)
                setFrameInternal(inPoint(), eventData);
            else if (m_frame == inPoint() && m_inc < 0)
                setFrameInternal(outPoint() - 1, eventData);
        }
        if (m_frame >= outPoint() || m_frame < inPoint())
        {
            setFrameInternal((m_inc > 0) ? inPoint() : outPoint() - 1,
                             eventData);
        }

        userGenericEvent("before-play-start", eventData);
        m_beforePlayStartSignal(eventData);

        // Disable automatic garbage collection during playback to make sure the
        // playback doesn't get interrupted which could cause skipped frames.
        if (evDisableGarbageCollectionDuringPlayback.getValue())
        {
            Mu::GarbageCollector::disable();
        }

        m_syncLastTime = 0.0;
        m_timer.start();

        m_shift = m_frame - rangeStart();

        if (isOutputDevice)
        {
            outputVideoDevice()->resetClock();
        }

        ensureAudioRenderer();

        if (audioRenderer() && hasAudio())
        {
            try
            {
                playAudio();
            }
            catch (...)
            {
                send(audioUnavailbleMessage());
            }
        }

        App()->startPlay(this);

        if (m_stopTimer.isRunning())
        {
            m_stopTimer.stop();
        }
        else
        {
            send(startPlayMessage());
        }

        userGenericEvent("play-start", eventData);
        m_playStartSignal(eventData);

        m_lastCheckTime = m_timer.elapsed();
        m_lastCheckFrame = m_frame;
        // m_realfps        = m_fps;
        m_fpsCalc->reset(m_wrapping);
    }

    void Session::play_v1(string& eventData)
    {
        //  worstFpsError = 0.0;
        //  Mu::GarbageCollector::disable();
        //
        //  Update the graph's in/out, etc.
        //

        //
        //  Make sure graph in/out points are up-to-date, but only if we
        //  are starting from a real stop.
        //

        if (eventData != "buffering")
        {
            updateGraphInOut(
                false); // flushAudio=false (not needed, and causes inf loop)
        }

        m_preEval = m_usePreEval;
        if (isPlaying() || rangeStart() == (rangeEnd() - 1))
            return;
        m_bufferWait = false;

        bool isOutputDevice =
            multipleVideoDevices() && outputVideoDevice()->hasClock();

        if (m_playMode == PlayOnce)
        {
            if (m_frame == outPoint() - 1 && m_inc > 0)
                setFrameInternal(inPoint(), eventData);
            else if (m_frame == inPoint() && m_inc < 0)
                setFrameInternal(outPoint() - 1, eventData);
        }
        if (m_frame >= outPoint() || m_frame < inPoint())
        {
            setFrameInternal((m_inc > 0) ? inPoint() : outPoint() - 1,
                             eventData);
        }

        userGenericEvent("before-play-start", eventData);
        m_beforePlayStartSignal(eventData);

        // Disable automatic garbage collection during playback to make sure the
        // playback doesn't get interrupted which could cause skipped frames.
        if (evDisableGarbageCollectionDuringPlayback.getValue())
        {
            Mu::GarbageCollector::disable();
        }

        m_syncLastTime = 0.0;
        m_timer.start();

        m_shift = m_frame - rangeStart();

        if (isOutputDevice)
        {
            outputVideoDevice()->resetClock();
        }

        ensureAudioRenderer();

        if (audioRenderer() && hasAudio())
        {
            try
            {
                playAudio();
            }
            catch (...)
            {
                send(audioUnavailbleMessage());
            }
        }

        App()->startPlay(this);

        if (m_stopTimer.isRunning())
        {
            m_stopTimer.stop();
        }
        else
        {
            send(startPlayMessage());
        }

        userGenericEvent("play-start", eventData);
        m_playStartSignal(eventData);

        m_lastCheckTime = elapsedPlaySecondsRaw() - m_timerOffset;
        m_lastCheckFrame = m_frame;
        // m_realfps        = m_fps;
        m_fpsCalc->reset(m_wrapping);

        m_syncTimes.clear(); // profiling

        ScopedLock lock(m_vSyncMutex);
        m_nextVSyncTime = Time(0.0);
        m_renderedVSyncTime = Time(0.0);
        m_syncLastWaitTime = Time(0.0);
        m_vSyncSerialLast = m_vSyncSerial - 1;
    }

    void Session::stop(string eventData)
    {
        if (!isPlaying() && !isBuffering())
            return;

        m_timer.stop();
        m_stopTimer.start();
        m_preEval = false;

        if (m_cacheMode == GreedyCache)
        {
            graph().cache().setFreeMode(FBCache::ConservativeFreeMode);
        }
        else
        {
        }

        if (multipleVideoDevices() && outputVideoDevice()->hasClock())
        {
            outputVideoDevice()->resetClock();
        }

        FBCache::FreeMode fmode;

        switch (m_cacheMode)
        {
        case GreedyCache:
            fmode = FBCache::ConservativeFreeMode;
            break;
        case BufferCache:
            fmode = FBCache::GreedyFreeMode;
            break;
        default:
            fmode = FBCache::ActiveFreeMode;
            break;
        }

        graph().cache().setFreeMode(fmode);

        m_shift = m_frame - rangeStart();
        //  cerr << "stop() setting(2) m_shift to " << m_shift << ".  m_frame "
        //  << m_frame << " rs " << rangeStart() << endl;
        m_lastCheckTime = 0.0;
        m_bufferWait = false; // in case stopped during buffer wait

        if (m_scrubAudio)
        {
            ;
        }
        else
        {
            ensureAudioRenderer();

            if (audioRenderer())
            {
                try
                {
                    stopAudio(eventData);
                }
                catch (...)
                {
                    resetAudio();
                }
            }
        }

        send(stopPlayMessage());
        userGenericEvent("play-stop", eventData);
        m_playStopSignal(eventData);

        // Re-enable automatic garbage collection after playback.
        if (evDisableGarbageCollectionDuringPlayback.getValue())
        {
            Mu::GarbageCollector::enable();
        }

        if (debugPlayback)
        {
            cout << "Total PATTERN FAIL count = " << m_framePatternFailCount
                 << endl;
        }
    }

    void Session::stopCompletely()
    {
        stop();

        if (m_stopTimer.isRunning())
        {
            m_stopTimer.stop();
            App()->stopPlay(this);
        }

        m_fastStart = true;
    }

    void Session::checkInDisplayImage()
    {
        if (m_displayImage && m_displayImage != m_errorImage
            && m_displayImage != m_proxyImage)
        {
            graph().checkInImage(m_displayImage);
        }

        m_displayImage = 0;
        m_displayFBStatus.clear();
    }

    void Session::checkInPreDisplayImage()
    {
        if (m_preDisplayImage && m_preDisplayImage != m_errorImage
            && m_preDisplayImage != m_proxyImage)
        {
            //
            //  wait until the upload thread is done with it.
            //

            waitForUploadToFinish();
            graph().checkInImage(m_preDisplayImage);
        }

        m_preDisplayImage = 0;
    }

    void Session::swapDisplayImage()
    {
        if (m_displayImage && m_displayImage != m_errorImage
            && m_displayImage != m_proxyImage)
        {
            graph().checkInImage(m_displayImage, true, m_preDisplayFrame);
        }

        m_displayImage = m_preDisplayImage;
        m_preDisplayImage = 0;
        m_preDisplayFrame = successorFrame(m_preDisplayFrame);
    }

    void Session::reload(int start, int end)
    {
        if (start < rangeStart())
            start = rangeStart();
        if (end >= rangeEnd())
            end = rangeEnd() - 1;

        checkInDisplayImage();
        checkInPreDisplayImage();

        graph().flushRange(start, end);
        clearVideoDeviceCaches();

        // We need to manually free the already uploaded textures here because
        // their content risks of no longer matching the reloaded media.
        if (m_renderer)
            m_renderer->freeUploadedTextures();

        updateGraphInOut();
        askForRedraw();
    }

    void Session::markFrame(int f)
    {
        if (isMarked(f))
            return;
        m_markedFrames.push_back(f);
        sort(m_markedFrames.begin(), m_markedFrames.end());
        ostringstream str;
        str << f;
        userGenericEvent("mark-frame", str.str());
        m_markFrameSignal(f);
    }

    void Session::unmarkFrame(int f)
    {
        Frames::iterator i =
            find(m_markedFrames.begin(), m_markedFrames.end(), f);

        if (i != m_markedFrames.end())
        {
            m_markedFrames.erase(i);
        }

        ostringstream str;
        str << f;
        userGenericEvent("unmark-frame", str.str());
        m_unmarkFrameSignal(f);
    }

    bool Session::isMarked(int f)
    {
        Frames::iterator i =
            find(m_markedFrames.begin(), m_markedFrames.end(), f);

        return i != m_markedFrames.end();
    }

    void Session::askForRedraw(bool force)
    {
        if (m_beingDeleted)
            return;

        m_wantsRedraw = true;

        if (!isPlaying())
        {
            if (!m_stopTimer.isRunning())
            {
                m_stopTimer.stop();
                m_stopTimer.start();
                App()->startPlay(this);
                send(startPlayMessage());
            }
        }

        if (!isRendering() || force)
        {
            if (const TwkGLF::GLVideoDevice* d =
                    dynamic_cast<const TwkGLF::GLVideoDevice*>(
                        controlVideoDevice()))
            {
                d->redraw();
            }

            if (const TwkGLF::GLVideoDevice* d =
                    dynamic_cast<const TwkGLF::GLVideoDevice*>(
                        outputVideoDevice()))
            {
                d->redraw();
            }

            if (outputVideoDevice()
                && outputVideoDevice() != controlVideoDevice())
            {
                if (DisplayGroupIPNode* dgroup =
                        graph().findDisplayGroupByDevice(outputVideoDevice()))
                {
                    dgroup->incrementRenderHashCount();
                }
            }
        }
    }

    void Session::setFrameInternal(int frame, string eventData)
    {
        bool sendEvent = (frame != m_frame);
        m_frame = frame;
        if (sendEvent)
        {
            send(frameChangedMessage());
            size_t f0;
            // if (debugGC) f0 = GC_get_free_bytes();
            userGenericEvent("frame-changed", eventData);
            m_frameChangedSignal(frame, eventData);

            // if (debugGC)
            //{
            //     size_t d = f0 - GC_get_free_bytes();
            //     if (d > 0) cerr << "GC: setFrameInternal frame-changed
            //     allocated " << d << " bytes." << endl;
            // }
        }
        // graph().cache().setCacheMark(frame);
    }

    void Session::setFrame(int frame)
    {
        if (frame < narrowedRangeStart())
            frame = narrowedRangeStart();
        if (frame >= narrowedRangeEnd())
            frame = narrowedRangeEnd() - 1;
        setFrameInternal(frame);

        if (graph().cachingMode() == IPGraph::BufferCache)
        {
            m_fastStart = true;
            updateGraphInOut(false); // Set to false to not affect audio
        }

        if (isPlaying())
        {
            //
            //  Recompute shift and restart
            //

            stop();
            play();
        }
        else
        {
            if (audioRenderer() && hasAudio())
            {
                if (m_scrubAudio)
                {
                    m_shift = m_frame - rangeStart();
                    //  cerr << "setFrame() setting(4) m_shift to " << m_shift
                    //  << ".  m_frame " << m_frame << " rs " << rangeStart() <<
                    //  endl;

                    audioVarLock();
                    setAudioLoopDuration(m_audioLoopDuration);
                    audioVarUnLock();

                    if (!isPlayingAudio())
                    {
                        try
                        {
                            playAudio();
                        }
                        catch (std::exception& exc)
                        {
                            resetAudio();
                        }
                    }
                }
                else
                {
                    // Prime the audio cache so that it is ready when the
                    // playback starts.
                    graph().primeAudioCache(frame, fps());
                }
            }

            send(updateMessage());
        }
    }

    double Session::outputDeviceHz() const
    {
        return (m_outputVideoDevice ? m_outputVideoDevice->timing().hz : 60.0);
    }

    void Session::setFPS(double f)
    {
        m_fps = f;
        // m_realfps = f;

        bool playing = isPlaying();

        //
        //  This will flush the audio and restart it
        //  making sure that everything sinks regardless
        //

        if (playing)
        {
            stop();
            graph().flushAudioCache();
        }
        if (audioCachingMode() == GreedyCache)
        {
            play();
            stop();
            setAudioCaching(BufferCache);
            setAudioCaching(GreedyCache);
        }

        userGenericEvent("fps-changed", "");

        if (playing)
            play();
    }

    int Session::targetFrame(double elapsed) const
    {
        if (m_avPlaybackVersion == 2)
        {
            return targetFrame_v2(elapsed);
        }
        else if (m_avPlaybackVersion == 1)
        {
            return targetFrame_v1(elapsed);
        }

        return targetFrame_v2(elapsed);
    }

    int Session::targetFrame_v2(double elapsed) const
    {
        //
        //  old targetFrame
        //

        int newFrame;

        if (inc() > 0)
        {
            const int cframe = int(elapsed * double(fps()) + 0.5);

            if (debugPlayback)
            {
                framePatternCheck(cframe);
                if (m_lastFData.size() > 10)
                    m_lastFData.pop_back();
            }

            newFrame = cframe + m_shift + rangeStart();
        }
        else
        {
            const int cframe = int(-elapsed * double(fps()) + 0.5);
            newFrame = cframe + m_shift + rangeStart();
        }

        return newFrame;
    }

    int Session::targetFrame_v1(double elapsed) const
    {
        const Time fps = Time(this->fps());
        const Time hz = outputDeviceHz();
        const Time halfHz = Time(1.0) / (Time(2.0) * hz);

        //
        //  Compute a "nice" fps for the actual display Hz. This will be
        //  something close to the actual fps but a rational ratio of the
        //  form X:10. So e.g. we allow 25:10 as the ratio for 24 in
        //  60. In that case we'd get a pattern like 2, 3, 2, 3, ...
        //

        const Time ratio = hz / fps;
        const Time precision = 10.0;
        const Time ratio_a = std::floor(ratio * precision + 0.5) / precision;
        const Time fps_a = hz / ratio_a;

        //
        //  Compute the frame based on the elapsed time for both the "nice" fps
        //  and the real fps
        //

        const Time s = elapsed * hz;
        const int sync = std::floor(s + 0.5);
        const Time elapsedHzUnits = double(sync) / hz;
        const int rframe = std::floor(elapsedHzUnits * fps_a);
        const int frame = std::floor(elapsedHzUnits * fps);

        const Time elapsedRaw = elapsedPlaySecondsRaw();

        //
        //  Compute the time at which the approximate fps and the real fps
        //  overlap by exactly 1 frame. At that interval we'll add/subtract a
        //  "correction" frame to the approximate fps to bring it back to the
        //  actual fps. Basically we're going to save up all the error until
        //  that time.
        //
        //  Time where "nice fps" + 1 is equal to real fps:
        //
        //      a * t + 1 = b * t
        //
        //             1
        //      t = -------
        //           b - a
        //
        //  where
        //
        //      a = nice_fps
        //      b = fps
        //
        //  In the case where a is larger than b, t will be negative and it all
        //  works out in the wash (you get a negative error)
        //

        const Time errorTime = Time(1.0) / (fps - fps_a);
        const int errorFrame = std::floor(elapsedHzUnits / errorTime);

        //
        //  Compute the coverage this frame will have in the next sync. If its
        //  smaller than half we use the next frame
        //

        const Time duration = Time(rframe + 1) / fps_a - elapsedHzUnits;
        const Time halfSync = Time(1.0) / (Time(2.0) * hz);
        const int direction = inc() > 0 ? 1 : -1;
        const Time epsmult = 1.00001; // precision slop when dealing with 25:10
        const int newFrame =
            (duration < (halfSync * epsmult)) ? (rframe + 1) : rframe;
        const int cframe = newFrame + errorFrame;

        if (debugPlayback)
        {
            if (!m_lastFData.empty())
            {
                const int sync0 = m_lastFData.front().sync;

                if ((sync - sync0) > 1)
                {
                    const Time lelapsed = m_lastFData.front().elapsed;

                    for (int i = 0; i < (sync - sync0 - 1); i++)
                    {
                        Fdata t = m_lastFData.front();
                        t.sync++;
                        m_lastFData.push_front(t);
                    }

                    if (debugPlaybackVerbose)
                    {
                        cout << "DEBUG: missed " << (sync - sync0 - 1)
                             << " sync points"
                             << ", " << (elapsed - lelapsed) << "s"
                             << " at " << cframe << " (" << m_frame << ")"
                             << " sdiff = " << (s - m_lastFData.front().s)
                             << " rawdiff = "
                             << (elapsedRaw - m_lastFData.front().elapsedRaw)
                             << endl;
                    }
                }
            }

            framePatternCheck(cframe);

            m_lastFData.push_front(Fdata(s, sync, elapsed, elapsedHzUnits,
                                         elapsedRaw, rframe, frame, errorFrame,
                                         duration, newFrame, cframe));

#if 0
        if (frame > 100 && m_framePatternFail)
        {
            cout << "DEBUG: fps = " << fps
                 << ", fps_a = " << fps_a
                 << ", hz = " << hz
                 << ", ratio = " << ratio
                 << ", ratio_a = " << ratio_a
                 << endl;

            for (size_t i = 0; i < m_lastFData.size(); i++)
            {
                const Fdata& d = m_lastFData[i];
                cout << "DEBUG: s: " << d.s << ", " << d.sync
                     << " e: " << d.elapsed0 << ", " << d.elapsed
                     << " f: " << d.rframe << ", " << d.frame << ", " << errorFrame
                     << " dur: " << d.duration
                     << " new: " << d.newFrame << ", " << d.cframe
                     << endl;
            }
        }
#endif

            while (m_lastFData.size() > 10)
                m_lastFData.pop_back();
        }

        return cframe * direction + m_shift + rangeStart();
    }

    Session::CachingMode Session::audioCachingMode() const
    {
        switch (graph().audioCachingMode())
        {
        default:
        case IPGraph::BufferCache:
            return BufferCache;
        case IPGraph::GreedyCache:
            return GreedyCache;
        }
    }

    void Session::setAudioCaching(CachingMode mode)
    {
        if (!graph().isAudioConfigured())
        {
            play();
            stop();
        }

        switch (mode)
        {
        default:
        case BufferCache:
            graph().setAudioCachingMode(IPGraph::BufferCache);
            break;
        case GreedyCache:
            graph().setAudioCachingMode(IPGraph::GreedyCache);
            break;
        }
    }

    void Session::setCaching(CachingMode mode)
    {
        if (mode != m_cacheMode)
        {
            bool active = false;
            IPGraph::CachingMode gmode;

            switch (mode)
            {
            case BufferCache:
                gmode = IPGraph::BufferCache;
                active = true;
                break;
            case GreedyCache:
                gmode = IPGraph::GreedyCache;
                active = true;
                break;
            default:
            case NeverCache:
                gmode = IPGraph::NeverCache;
                active = false;
                checkInDisplayImage();
                break;
            }

            graph().setCacheNodesActive(active);
            graph().setCachingMode(gmode, inPoint(), outPoint(), rangeStart(),
                                   rangeEnd(), inc(), currentFrame(), fps());

            m_cacheMode = mode;
            askForRedraw();

            string contents;

            switch (gmode)
            {
            case IPGraph::CachingMode::BufferCache:
                contents = "buffer";
                break;
            case IPGraph::CachingMode::GreedyCache:
                contents = "region";
                break;
            case IPGraph::CachingMode::NeverCache:
                contents = "off";
                break;
            default:
                contents = "";
                break;
            }

            userGenericEvent("cache-mode-changed", contents);
        }
    }

    Session::CachingMode Session::cachingMode() const { return m_cacheMode; }

    void Session::characterizeDisplayImage()
    {
        m_displayFBStatus.clear();

        if (m_displayImage)
        {
            FBStatusCheck check(m_displayFBStatus);
            foreach_ip(m_displayImage, check);
        }
    }

    //
    // Used to be called syncTimeUntilExpected
    //
    TwkMath::Time Session::predictedTimeUntilSync_v2() const
    {
        float interval = 0.0;

        //
        //  Determine a refresh rate to use in predictin sync times.
        //

        if (RV_VSYNC_RATE_FORCE != 0.0)
        //
        //  If the user has dictated a rate use that.
        //
        {
            interval = 1.0 / RV_VSYNC_RATE_FORCE;
        }
        else if (!RV_VSYNC_IGNORE_DEVICE && m_outputVideoDevice
                 && m_outputVideoDevice->timing().hz != 0.0)
        //
        //  Else if the device provides a rate use that.
        //
        {
            float hz = m_outputVideoDevice->timing().hz;
            if (hz != m_syncTargetRefresh)
            {
                m_syncTargetRefresh = hz;
                if (debugPlayback)
                    cerr << "INFO: new target refresh rate " << hz << endl;
            }
            interval = 1.0 / hz;
        }
        else if (m_syncLastTime > 0 && m_syncInterval > 0)
        //
        //  Otherwise fallback to sync prediction resulting from averaging
        //  samples.
        //
        {
            float hz = 1.0 / m_syncInterval;
            if (fabs(hz - m_syncTargetRefresh) > 0.1)
            {
                m_syncTargetRefresh = hz;
                if (debugPlayback)
                    cerr << "INFO: new computed target refresh rate " << hz
                         << endl;
            }
            interval = m_syncInterval;
        }
        else
        {
            return 0.0;
        }

        if (m_syncLastTime <= 0)
            return interval;

        //
        //  If we know when the last sync time happened, use the current time to
        //  predict the time until the next sync time.
        //

        Time t = interval - (m_timer.elapsed() - m_syncLastTime);

        if (t > interval)
            t = interval;

        return t;
    }

    TwkMath::Time Session::predictedTimeUntilSync() const
    {
        Time interval = 0.0;
        ScopedLock lock(m_vSyncMutex);

        if (m_nextVSyncTime > 0.0)
        {
            return m_nextVSyncTime - elapsedPlaySecondsRaw();
        }

        //
        //  Determine a refresh rate to use in predictin sync times.
        //

        if (RV_VSYNC_RATE_FORCE != 0.0)
        //
        //  If the user has dictated a rate use that.
        //
        {
            interval = 1.0 / RV_VSYNC_RATE_FORCE;
        }
        else if (!RV_VSYNC_IGNORE_DEVICE && m_outputVideoDevice
                 && m_outputVideoDevice->timing().hz != 0.0)
        //
        //  Else if the device provides a rate use that.
        //
        {
            Time hz = outputDeviceHz();

            if (hz != m_syncTargetRefresh)
            {
                m_syncTargetRefresh = hz;
                if (debugPlayback)
                    cout << "INFO: new target refresh rate " << hz << endl;
            }

            interval = 1.0 / hz;
        }
        else if (m_syncLastTime > 0 && m_syncInterval > 0)
        //
        //  Otherwise fallback to sync prediction resulting from averaging
        //  samples.
        //
        {
            Time hz = Time(1.0) / m_syncInterval;

            if (fabs(hz - m_syncTargetRefresh) > 0.1)
            {
                m_syncTargetRefresh = hz;
                if (debugPlayback)
                    cout << "INFO: new computed target refresh rate " << hz
                         << endl;
            }
            interval = m_syncInterval;
        }
        else
        {
            return 0.0;
        }

        if (m_syncLastTime <= 0)
            return interval;

        //
        //  If we know when the last sync time happened, use the current time to
        //  predict the time until the next sync time.
        //

        const Time elapsed = elapsedPlaySecondsRaw() - m_timerOffset;
        const Time snapped = std::floor(elapsed / interval + 0.5) * interval;
        const Time t = interval - (elapsed - snapped);

        // Time t = interval - (elapsed - m_syncLastTime);
        // if (t > interval) t = interval;

        return t;
    }

    void Session::addSyncSample()
    {

        if (m_avPlaybackVersion == 2)
        {
            return addSyncSample_v2();
        }
        else
        {
            return addSyncSample_v1();
        }
    }

    void Session::addSyncSample_v2()
    {
        if (!m_timer.isRunning())
            return;

        const Time t = m_timer.elapsed();
        const Time interval = t - m_syncLastTime;
        m_syncLastTime = t;

        if (!m_syncPredictionEnabled)
            return;

        if (interval > Time(1.0 / RV_VSYNC_MIN_INTERVAL)
            || interval < Time(1.0 / RV_VSYNC_MAX_INTERVAL))
        {
            //
            //  If the time since the last sync is unreasonable (slower
            //  refresh rate than 20hz or faster than 120hz) just bail
            //

            return;
        }

        m_syncTimes.push_front(interval);

        size_t size = m_syncTimes.size();

        //
        //  Compute RMS of sync times. Not sure if this a good use of RMS, but
        //  since there appears to be some noise in the sync timing this is
        //  used to dampen the outliers.
        //

        Time accum = Time(0);

        for (size_t i = 0; i < size; i++)
        {
            Time dt = m_syncTimes[i];
            accum += dt * dt;
        }

        accum = sqrt(accum / Time(size));
        m_syncInterval = accum;

        if (size >= m_syncMaxSamples)
        {
            m_syncTimes.pop_back();
        }
    }

    void Session::addSyncSample_v1()
    {
        if (!m_timer.isRunning())
            return;

        const bool firstTime = m_syncTimes.empty();
        const Time t = elapsedPlaySecondsRaw() - m_timerOffset;
        const Time interval = t - m_syncLastTime;

        m_syncLastTime = t;

        // if (!m_syncPredictionEnabled) return;

        if (!firstTime
            && (interval > Time(1.0 / RV_VSYNC_MIN_INTERVAL)
                || interval < Time(1.0 / RV_VSYNC_MAX_INTERVAL)))
        {
            //
            //  If the time since the last sync is unreasonable (slower
            //  refresh rate than 20hz or faster than 120hz) just bail
            //

            return;
        }

        m_syncTimes.push_front(interval);

        const size_t size = m_syncTimes.size();

        //
        //  Compute RMS of sync times. Not sure if this a good use of RMS, but
        //  since there appears to be some noise in the sync timing this is
        //  used to dampen the outliers.
        //

        Time accum = Time(0);

        for (size_t i = 0; i < size; i++)
        {
            Time dt = m_syncTimes[i];
            accum += dt * dt;
        }

        accum = sqrt(accum / Time(size));
        m_syncInterval = accum;

        if (size >= m_syncMaxSamples)
        {
            m_syncTimes.pop_back();
        }
    }

    void Session::evaluateForDisplay()
    {
        HOP_PROF_FUNC();

        IPGraph::EvalResult result(IPGraph::EvalError, (IPImage*)0);

        try
        {
            //
            //  The session is not in multithreaded evaluation
            //  mode. Use this thread to evaluate directly with
            //  "local" caching. (local caching means that the IPNodes
            //  are allowed to cache pixels if the want).
            //

            checkInDisplayImage();
            checkInPreDisplayImage();

            result = graph().evaluateAtFrame(m_frame, true);
            m_displayImage = result.second;

            characterizeDisplayImage();

            if (!m_postFirstNonEmptyRender && currentFrameState() == OkStatus)
            {
                // In progressive source loading, we need to wait until the
                // first load is actually completed
                static const bool progressiveSourceLoading =
                    Application::optionValue<bool>("progressiveSourceLoading",
                                                   false);
                if (!progressiveSourceLoading)
                {
                    m_postFirstNonEmptyRender = true;
                }
            }
        }
        catch (const std::exception& exc)
        {
            m_errorImage->fb->attribute<string>("Error") = exc.what();
            m_displayImage = m_errorImage;
            return;
        }
        catch (...)
        {
            m_errorImage->fb->attribute<string>("Error") = "Failed Evaluation";
            m_displayImage = m_errorImage;
            return;
        }

        switch (result.first)
        {
        default:
        case IPGraph::EvalNormal:
            break;

        case IPGraph::EvalBufferNeedsRefill:
            throw BufferNeedsRefillExc();
            break;

        case IPGraph::EvalError:
            m_errorImage->fb->attribute<string>("Error") =
                "Graph Evaluation Error";
            m_displayImage = m_errorImage;
            break;
        }

        m_displayFrame = m_frame;
    }

    IPImage* Session::evaluate(int frame)
    {
        return graph().evaluateAtFrame(frame).second;
    }

    void Session::checkIn(IPImage* img) { graph().checkInImage(img); }

    int Session::successorFrame(int frame) const
    {
        int nextFrame = frame + inc();
        if (inc() > 0 && nextFrame >= outPoint())
            nextFrame = inPoint();
        else if (inc() < 0 && nextFrame < inPoint())
            nextFrame = outPoint() - 1;
        return nextFrame;
    }

    void Session::prefetch()
    {
        HOP_ZONE(HOP_ZONE_COLOR_13);
        HOP_PROF_FUNC();

        preEval();

        if (m_preDisplayImage)
        {
#if defined(HOP_ENABLED)
            std::string hopMsg = std::string("Session::prefetch() - frame=")
                                 + std::to_string(m_preDisplayFrame);
            HOP_PROF_DYN_NAME(hopMsg.c_str());
#endif

            if (debugProfile)
            {
                ProfilingRecord& trecord = currentProfilingSample();
                trecord.prefetchRenderStart = profilingElapsedTime();
                m_renderer->initProfilingState(&m_profilingTimer);
            }

            m_renderer->prepareTextureDescriptionsForUpload(m_preDisplayImage);
            m_renderer->prefetch(m_preDisplayImage);

            if (debugProfile)
            {
                ProfilingRecord& trecord = currentProfilingSample();
                trecord.prefetchRenderEnd = profilingElapsedTime();
                trecord.prefetchUploadPlaneTotal =
                    m_renderer->profilingState().uploadPlaneTime;
            }
        }

        if (debugProfile)
        {
            ProfilingRecord& trecord = currentProfilingSample();
            trecord.internalPrefetchEnd = profilingElapsedTime();
        }
    }

    void Session::preEval()
    {
        HOP_PROF_FUNC();

        //
        //  Try and pre-evaluate
        //

        if (debugProfile)
        {
            ProfilingRecord& trecord = currentProfilingSample();
            trecord.internalPrefetchStart = profilingElapsedTime();
        }

        IPGraph::EvalResult preEvalResult(IPGraph::EvalError, (IPImage*)0);

        try
        {
            m_preDisplayFrame = successorFrame(m_frame);
            preEvalResult = graph().evaluateAtFrame(m_preDisplayFrame, false);
            m_preDisplayImage = preEvalResult.second;
        }
        catch (...)
        {
            //
            //  If pre-eval failed for any reason just reset it
            //

            m_preDisplayFrame = m_frame;
            assert(m_preDisplayImage == 0);
        }
    }

    void Session::queryAndStoreGLInfo()
    {
        ImageRenderer::queryGL();
        ImageRenderer::queryGLIntoContainer(graph().sessionNode());
    }

    void Session::setNextVSyncOffset(Time t, size_t serial)
    {
        double e = elapsedPlaySecondsRaw();
        ScopedLock lock(m_vSyncMutex);

        if (m_nextVSyncTime == Time(0.0))
        {
            if (debugPlaybackVerbose)
                cout << "DEBUG: setting m_timerOffset to " << e << endl;
            m_timerOffset = e;

            if (audioRenderer())
            {
                //
                //  Adjust the audio shift to account for the time when
                //  the first sync took place
                //

                audioVarLock();
                setAudioTimeShift(m_timerOffset);
                audioVarUnLock();
            }
        }

        if (debugPlaybackVerbose && m_nextVSyncTime != m_renderedVSyncTime)
        {
            cout << "DEBUG: missed a render: "
                 << "rendered = " << m_renderedVSyncTime
                 << ", current = " << m_nextVSyncTime << endl;
        }

        m_nextVSyncTime = e + t;

        if (debugPlaybackVerbose && m_vSyncSerial != m_vSyncSerialLast
            && m_useExternalVSyncTiming)
        {
            cout << "DEBUG: DL thread missed: " << m_vSyncSerial
                 << " != " << m_vSyncSerialLast << endl;
        }

        m_vSyncSerial = serial;
        m_vSyncCond.notify_one();
    }

    void Session::waitOnVSync()
    {
        if (isPlaying())
        {
            ScopedLock lock(m_vSyncMutex);
            Timer timer;
            timer.start();

            const Time hz = outputDeviceHz();
            const Time t = 1.0 / hz * 0.5;

            m_waitingOnSync = true;
            if (debugPlaybackVerbose && m_syncLastWaitTime >= t)
                cout << "DEBUG: skipping wait on vsync" << endl;

            while (m_syncLastWaitTime < t && m_nextVSyncTime != Time(0.0)
                   && m_vSyncSerial == m_vSyncSerialLast)
            {
                m_vSyncCond.wait(lock);
            }

            m_waitingOnSync = false;
            m_syncLastWaitTime = timer.elapsed();
            if (m_nextVSyncTime != Time(0.0))
                m_vSyncSerialLast = m_vSyncSerial;
        }
    }

    void Session::postRender()
    {
        if (m_avPlaybackVersion == 2)
        {
            return postRender_v2();
        }
        else
        {
            return postRender_v1();
        }
    }

    void Session::postRender_v2()
    {
        if (!useThreadedUpload() && m_preEval)
            prefetch(); // otherwise it is done currently with rendering
        graph().postEvaluation();

        userRenderEvent("post-render", "");
    }

    void Session::postRender_v1()
    {
        if (!m_useExternalVSyncTiming && isPlaying())
        {
            //
            //  In the non-external case, its assumed that the Session is
            //  taking care of the sync timing itself in the same thread
            //  that called render. So locking m_vSyncMutex is unncessary.
            //

            const Time t = 1.0 / outputDeviceHz();
            setNextVSyncOffset(t, m_vSyncSerial++);
        }

        if (!useThreadedUpload() && m_preEval)
        {
            prefetch(); // otherwise it is done currently with rendering
        }

        graph().postEvaluation();

        userRenderEvent("post-render", "");
    }

    void Session::update(double minElapsedTime, bool force)
    {
        //
        //  This is the "heartbeat" entry point in the session
        //

        if (force || isUpdating() || m_rangeDirty)
        {
            if (multipleVideoDevices()
                && outputVideoDevice()->willBlockOnTransfer()
                && (outputVideoDevice()->capabilities()
                    & TwkApp::VideoDevice::ASyncReadBack))
            {
                return;
            }

            if (const TwkGLF::GLVideoDevice* d =
                    dynamic_cast<const TwkGLF::GLVideoDevice*>(
                        controlVideoDevice()))
            {
                double now = TwkUtil::SystemClock().now();
                if (now >= (m_lastDrawingTime + minElapsedTime))
                {
                    d->redrawImmediately();
                    m_lastDrawingTime = now;
                }
            }
        }
    }

    void Session::advanceState()
    {
        if (!ImageRenderer::queryGLFinished())
        {
            queryAndStoreGLInfo();
        }

        if (!m_renderer->supported())
            chooseNextBestRenderer();

        if (m_rangeDirty)
            updateRange();

        if (m_bufferWait)
        {
            //
            //  This is a workflow optimization: if setFrame was called
            //  (so there was a jump to a new frame) try and start playing
            //  after 0.5 seconds of cache are available instead of
            //  waiting until its filled. This makes the playback more
            //  responsive. However, if the load time is too long, a full
            //  (no fast start) buffer cache may result. Note that initial
            //  playback is also fast start except in that case, no frames
            //  are buffered.
            //
            //  Modifing the above slightly.  We want to compare
            //  against min(0.5,maxBufferedWaitSeconds), since the user
            //  may have asked for an even shorter wait time.
            //

            if (debugPlaybackVerbose)
                cout << "DEBUG: buffer wait..." << endl;

            if (!graph().isCacheThreadRunning())
            {
                play("buffering");
            }
            else
            {
                float targetSeconds = m_maxBufferedWaitSeconds;
                if (m_fastStart)
                    targetSeconds = min(0.5f, targetSeconds);

                float la = 1.0, timeTillTurnaround = 0.0;
                if (targetSeconds > 0.0)
                {
                    graph().cache().cacheStats(m_cacheStats);
                    la = m_cacheStats.lookAheadSeconds;

                    if (inc() > 0)
                        timeTillTurnaround =
                            float(outPoint() - 1 - m_frame) / m_fps;
                    else
                        timeTillTurnaround = float(m_frame - inPoint()) / m_fps;
                }

                if (la >= targetSeconds || m_frame >= outPoint()
                    || m_frame < inPoint() || la >= timeTillTurnaround)
                {
                    play("buffering");
                    m_fastStart = false;
                    m_bufferWait = false;
                }
            }
        }

        if (m_stopTimer.isRunning())
        {
            if (m_stopTimer.elapsed() > 2.0 && !isEvalRunning()
                && !isBuffering() && !currentStateIsError())
            {
                stopCompletely();
            }
        }

        double elapsed = 0.0;
        bool playing = isPlaying();
        int currentFrame = m_frame;
        int frameShift = 0;
        m_skipped = 0;

        bool outDeviceClock = m_realtimeOverride && multipleVideoDevices()
                              && outputVideoDevice()->hasClock();

        if (playing)
        {
            if (m_useExternalVSyncTiming)
                waitOnVSync();

            //
            //  Compute the amount of time elapsed since beginning or when
            //  the last play started
            //

            const Time predictedSyncTime = predictedTimeUntilSync();

            elapsed = elapsedPlaySeconds() + predictedSyncTime;

            if (debugProfile)
            {
                ProfilingRecord& trecord = currentProfilingSample();
                trecord.expectedSyncTime =
                    profilingElapsedTime() + predictedSyncTime;
                if (outDeviceClock)
                {
                    trecord.deviceClockOffset =
                        elapsed - outputVideoDevice()->nextFrameTime();
                }
            }

            if (outDeviceClock)
            {
                elapsed = outputVideoDevice()->nextFrameTime();
            }

            audioVarLock();
            const bool tsvalid = audioTimeShiftValid();
            audioVarUnLock();

            if (!audioRenderer() || (elapsed >= 0.0 && tsvalid)
                || outDeviceClock)
            {
                int newFrame = targetFrame(elapsed);
                const int nextFrame = m_frame + inc();

                if (m_bufferWait || realtime())
                {
                    m_skipped = (newFrame - nextFrame) / inc();
                    if (inc() * m_skipped < 0)
                        m_skipped = 0;
                }
                else if (outDeviceClock)
                {
                    // nothing
                }
                else if ((inc() > 0 && nextFrame < newFrame)
                         || (inc() < 0 && nextFrame > newFrame))
                {
                    //
                    //  If the newFrame is not the next frame then the
                    //  shift needs to be adjusted.
                    //

                    const int newShift = m_shift + (nextFrame - newFrame);

                    if (debugPlaybackVerbose && newShift != m_shift)
                    {
                        cout << "DEBUG: frame shift adjusted by "
                             << (newShift - m_shift) << endl;
                    }

                    m_shift = newShift;
                    frameShift = (nextFrame - newFrame);
                    newFrame = nextFrame;

                    //
                    //  Adjust audio
                    //
                }

                if ((newFrame - rangeStart()) % inc())
                {
                    //
                    //  Frame is not included in the play range -- just skip it
                    //

                    return;
                }

                //
                //  Do a sanity check for messed up timers. This can
                //  happen when trying to lock to the audio timer and the
                //  sound card is crappy -- its timer will not be
                //  stable. To remidy that, don't allow the frame to ever
                //  move backwards (or forwards if we're playing
                //  backwards). This prevents a visible jerk, but it
                //  doesn't completely solve the problem. We should
                //  probably notify the user somehow that this is
                //  happening or fall back to the cpu timer.
                //

                if (inc() > 0)
                {
                    if (newFrame < m_frame)
                        newFrame = m_frame;
                }
                else
                {
                    if (newFrame > m_frame)
                        newFrame = m_frame;
                }

                m_frame = newFrame;
            }
        }

        //
        //  Wrap frames
        //

        m_wrapping = false;

        if (playing)
        {
            if (m_frame >= outPoint() && inc() > 0)
            {
                switch (m_playMode)
                {
                case PlayPingPong:
                    stop("turn-around");
                    m_inc *= -1;
                    setFrameInternal(outPoint() - 1, "turn-around");
                    m_wrapping = true;
                    play("turn-around");
                    break;
                case PlayLoop:
                    stop("turn-around");
                    setFrameInternal(inPoint(), "turn-around");
                    m_wrapping = true;
                    play("turn-around");
                    break;
                case PlayOnce:
                    stop("turn-around");
                    setFrameInternal(outPoint() - 1, "turn-around");
                    m_wrapping = true;
                    break;
                }
            }
            else if (m_frame < inPoint() && inc() < 0)
            {
                switch (m_playMode)
                {
                case PlayLoop:
                    stop("turn-around");
                    setFrameInternal(outPoint() - 1, "turn-around");
                    m_wrapping = true;
                    play("turn-around");
                    break;
                case PlayPingPong:
                    stop("turn-around");
                    setFrameInternal(inPoint() + 1, "turn-around");
                    m_inc *= -1;
                    m_wrapping = true;
                    play("turn-around");
                    break;
                case PlayOnce:
                    stop("turn-around");
                    setFrameInternal(inPoint(), "turn-around");
                    m_wrapping = true;
                    break;
                }
            }
        }

        if (m_frame >= rangeEnd())
            setFrameInternal(rangeEnd() - 1);
        if (m_frame < rangeStart())
            setFrameInternal(rangeStart());

        //
        //  The overhead (reading and drawing). Throw the frame up and
        //  figure outhow long it took. If we're reading a complex file
        //  format off disk (like exr) this can take a while. Compute the
        //  overhead time for next frame prediction -- note that the
        //  constant in the lerp is completely subjective.
        //

        userRenderEvent("pre-render", "");

        if (playing && m_cacheMode == BufferCache)
        {
            if (m_maxBufferedWaitSeconds > 0.0
                && !graph().cache().isFrameCached(successorFrame(m_frame))
                && isCaching())
            {
                stop("buffering");
                m_bufferWait = true;
            }
        }
        m_rendering = true;
        m_wantsRedraw = false;

        if (debugProfile)
        {
            ProfilingRecord& trecord = currentProfilingSample();
            trecord.evaluateStart = profilingElapsedTime();
            trecord.evaluateEnd = trecord.evaluateStart;
            graph().beginProfilingSample();
        }

        try
        {
            evaluateForDisplay();
        }
        catch (BufferNeedsRefillExc& exc)
        {
            //
            //  Don't wait here unless there's actually video to cache (maybe
            //  this evaluation is only generating audio.
            //

            bool hasFB = false;
            for (size_t i = 0; i < m_displayFBStatus.size(); i++)
            {
                if (m_displayFBStatus[i].fb)
                {
                    hasFB = true;
                    break;
                }
            }

            if (maxBufferedWaitTime() != 0.0 && hasFB)
            {
                stop("buffering");
                if (!m_wrapping)
                    setFrameInternal(currentFrame);

                if (playing)
                {
                    m_bufferWait = true;
                }
                try
                {
                    evaluateForDisplay();
                }
                catch (...)
                {
                }
            }
        }

        if (debugProfile)
        {
            ProfilingRecord& trecord = currentProfilingSample();
            trecord.evaluateEnd = profilingElapsedTime();
            graph().endProfilingSample();
        }

        const float clockMult = fps() / currentTargetFPS();

        if (hasAudio() && !outDeviceClock
            && ((!realtime() && frameShift) || clockMult != 1.0))
        {
            //
            //  This is the multiplier on the clock. If we're playing
            //  at 48 fps and the frame's fps is 24.0 then we're
            //  playing with a clock multiple of 2.0. That means
            //  seeking into the audio may require skipping that far
            //  ahead into the samples. (dropping samples)
            //

            // NB: elapsedPlaySeconds() goes to max numeric value when playback
            //     wraps backwards from frame zero. So we check against this.
            const double elapsedPlaySecs = elapsedPlaySeconds();

            if (currentFrame != m_frame && audioRenderer()
                && elapsedPlaySecs < numeric_limits<double>::max())
            {
                const bool forwards = (inc() > 0);
                const double atime =
                    (m_frame - rangeStart()) / fps() * clockMult;
                const double elapsed =
                    (frameShift ? frameShift : m_shift) / fps() * clockMult
                    + (forwards ? elapsedPlaySecs : -elapsedPlaySecs);
                const double diff = ::fabs(atime - elapsed);

                if (diff > (1 / (currentTargetFPS() * m_audioDrift)))
                {
                    if (debugPlaybackVerbose)
                    {
                        cout << "DEBUG: audio ";
                        if (diff > 0)
                            cout << "+";
                        cout << diff << " seconds" << endl;
                    }

                    //
                    //  NOTE: missing the case when we're playing
                    //  backwards. The backwards playback may drift beyond
                    //  a reasonable amount.
                    //

                    if (forwards)
                    {
                        audioVarLock();
                        setAudioTimeSnapped(atime);
                        audioVarUnLock();
                    }
                }
            }
        }

        m_wrapping = false;

        if (m_lastFrame != m_frame && isPlaying())
            m_frameChangedSignal(m_frame, "");
    }

    void Session::renderState()
    {
        const bool playing = isPlaying();
        const bool outDeviceClock = m_realtimeOverride && multipleVideoDevices()
                                    && outputVideoDevice()->hasClock();

        try
        {
            if (debugProfile)
            {
                ProfilingRecord& trecord = currentProfilingSample();
                trecord.internalRenderStart = profilingElapsedTime();
                m_renderer->initProfilingState(&m_profilingTimer);
            }

            AuxUserRender auxRender(this);
            AuxAudioRenderer auxAudio(this);

            waitForUploadToFinish();
            m_waitForUploadThreadPrefetch = m_preEval && useThreadedUpload();

            if (m_waitForUploadThreadPrefetch)
            {
                //
                //  In this case we need to evaluate next next frame
                //  before rendering
                //
                //

                preEval();

                m_renderer->render(m_frame, m_displayImage, &auxRender,
                                   &auxAudio, m_preDisplayImage);
            }
            else
            {
                m_renderer->render(m_frame, m_displayImage, &auxRender,
                                   &auxAudio);
            }

            if (debugProfile)
            {
                ProfilingRecord& trecord = currentProfilingSample();
                trecord.internalRenderEnd = profilingElapsedTime();
                trecord.frame = m_frame;
                trecord.renderUploadPlaneTotal =
                    m_renderer->profilingState().uploadPlaneTime;
                trecord.renderFenceWaitTotal =
                    m_renderer->profilingState().fenceWaitTime;
            }
        }
        catch (RendererNotSupportedExc& exc)
        {
        }
        catch (std::exception& exc)
        {
            m_displayImage = m_errorImage;
            m_errorImage->fb->attribute<string>("Error") = exc.what();
        }
        catch (...)
        {
            m_displayImage = m_errorImage;
            m_errorImage->fb->attribute<string>("Error") =
                "Error during rendering";
        }
    }

    void Session::recordRenderState()
    {
        const bool playing = isPlaying();
        const bool outDeviceClock = m_realtimeOverride && multipleVideoDevices()
                                    && outputVideoDevice()->hasClock();
        const bool calcFPSWithDevClock =
            outDeviceClock && outputVideoDevice()->timing().hz != 0.0;

        if (!playing)
        {
            m_fpsCalc->reset();
            m_realfps = 0.0;
            m_lastDevFrame = -1;
        }
        else if (calcFPSWithDevClock)
        {
            float devFrame = outputVideoDevice()->nextFrame();

            if (m_lastDevFrame == -1)
            {
                m_lastViewFrame = m_frame;
                m_lastDevFrame = devFrame;
            }
            else if (abs(m_frame - m_lastViewFrame) >= 4)
            {
                float framesSample = ::fabs(float(m_frame - m_lastViewFrame));
                float timeSample = float(devFrame - m_lastDevFrame)
                                   / outputVideoDevice()->timing().hz;
                float fpsSample = framesSample / timeSample;

                m_fpsCalc->setTargetFps(m_fps);
                m_fpsCalc->addSample(fpsSample);
                m_realfps =
                    m_fpsCalc->fps(outputVideoDevice()->nextFrameTime());
                // m_realfps = m_fpsCalc->fps (elapsedPlaySeconds());
                if (fabs(m_realfps - m_fps) < 0.125)
                    m_realfps = m_fps;

                m_lastViewFrame = m_frame;
                m_lastDevFrame = devFrame;
            }
        }
        else if (abs(m_frame - m_lastCheckFrame) >= 4)
        {
            double nelapsed = elapsedPlaySeconds();
            double fps = double(abs(m_frame - m_lastCheckFrame))
                         / (nelapsed - m_lastCheckTime);
            //  cout << "fps " << fps << " f " << m_frame << " lcf " <<
            //  m_lastCheckFrame << endl;

            m_fpsCalc->setTargetFps(m_fps);
            m_fpsCalc->addSample(fps);
            m_realfps = m_fpsCalc->fps(nelapsed);

            //
            //  We decide arbitrarily here that a differences of
            //  less than 0.125 is more distracting than interesting.
            //

            if (fabs(m_realfps - m_fps) < 0.125)
                m_realfps = m_fps;

            m_lastCheckTime = nelapsed;
            m_lastCheckFrame = m_frame;
        }

        if (m_lastFrame != m_frame && playing)
        {
            if (debugProfile)
            {
                ProfilingRecord& trecord = currentProfilingSample();
                trecord.frameChangeEventStart = profilingElapsedTime();
            }

            send(frameChangedMessage());
            size_t f0;
            // if (debugGC) f0 = GC_get_free_bytes();
            userGenericEvent("frame-changed", "");
            // m_frameChangedSignal(m_frame, "");
            // if (debugGC)
            //{
            //     size_t d = f0 - GC_get_free_bytes();
            //     if (d > 0) cout << "GC: render frame-changed allocated " << d
            //     << " bytes." << endl;
            // }

            if (debugProfile)
            {
                ProfilingRecord& trecord = currentProfilingSample();
                trecord.frameChangeEventEnd = profilingElapsedTime();
            }
        }

        m_lastFrame = m_frame;
        m_rendering = false;
        m_renderedVSyncTime = m_nextVSyncTime;
    }

    void Session::render()
    {
        if (m_avPlaybackVersion == 2)
        {
            HOP_CALL(glFinish();)
            HOP_PROF("Session::render - render_v2");

            render_v2();

            HOP_CALL(glFinish();)
        }
        else
        {
            {
                HOP_CALL(glFinish();)
                HOP_PROF("Session::render - advanceState");

                advanceState();

                HOP_CALL(glFinish();)
            }
            {
                HOP_CALL(glFinish();)
                HOP_PROF("Session::render - renderState");

                renderState();

                HOP_CALL(glFinish();)
            }
            {
                HOP_CALL(glFinish();)

                HOP_PROF("Session::render - recordRenderState");

                recordRenderState();

                HOP_CALL(glFinish();)
            }
        }
    }

    void Session::render_v2()
    {
        if (!ImageRenderer::queryGLFinished())
        {
            queryAndStoreGLInfo();
        }

        if (!m_renderer->supported())
            chooseNextBestRenderer();

        if (m_rangeDirty)
            updateRange();

        if (m_bufferWait)
        {
            //
            //  This is a workflow optimization: if setFrame was called
            //  (so there was a jump to a new frame) try and start playing
            //  after 0.5 seconds of cache are available instead of
            //  waiting until its filled. This makes the playback more
            //  responsive. However, if the load time is too long, a full
            //  (no fast start) buffer cache may result. Note that initial
            //  playback is also fast start except in that case, no frames
            //  are buffered.
            //
            //  Modifing the above slightly.  We want to compare
            //  against min(0.5,maxBufferedWaitSeconds), since the user
            //  may have asked for an even shorter wait time.
            //

            if (!graph().isCacheThreadRunning())
            {
                play("buffering");
            }
            else
            {
                float targetSeconds = m_maxBufferedWaitSeconds;
                if (m_fastStart)
                    targetSeconds = min(0.5f, targetSeconds);

                float la = 1.0, timeTillTurnaround = 0.0;
                if (targetSeconds > 0.0)
                {
                    graph().cache().cacheStats(m_cacheStats);
                    la = m_cacheStats.lookAheadSeconds;

                    if (inc() > 0)
                        timeTillTurnaround =
                            float(outPoint() - 1 - m_frame) / m_fps;
                    else
                        timeTillTurnaround = float(m_frame - inPoint()) / m_fps;
                }

                if (la >= targetSeconds || m_frame >= outPoint()
                    || m_frame < inPoint() || la >= timeTillTurnaround)
                {
                    play("buffering");
                    m_fastStart = false;
                    m_bufferWait = false;
                }
            }
        }

        if (m_stopTimer.isRunning())
        {
            if (m_stopTimer.elapsed() > 2.0 && !isEvalRunning()
                && !isBuffering() && !currentStateIsError())
            {
                stopCompletely();
            }
        }

        double elapsed = 0.0;
        bool playing = isPlaying();
        int currentFrame = m_frame;
        m_skipped = 0;

        bool outDeviceClock = m_realtimeOverride && multipleVideoDevices()
                              && outputVideoDevice()->hasClock();

        // if (m_outputVideoDevice)  cerr << "ovd timing " <<
        // m_outputVideoDevice->videoFormatAtIndex(m_outputVideoDevice->currentVideoFormat()).hz
        // << "Hz" << endl; if (m_outputVideoDevice)  cerr << "ovd timing " <<
        // m_outputVideoDevice->timing().hz << "Hz" << endl;

        if (playing)
        {
            //
            //  Compute the amount of time elapsed since beginning or when
            //  the last play started
            //

            int nextFrame = m_frame + inc();
            int expectedFrame = m_frame;
            int newFrame = m_frame;

            Time untilPredicted = predictedTimeUntilSync_v2();
            elapsed = elapsedPlaySeconds() + untilPredicted + elapsedOffset;

            if (debugProfile)
            {
                ProfilingRecord& trecord = currentProfilingSample();
                trecord.expectedSyncTime =
                    profilingElapsedTime() + untilPredicted;
                if (outDeviceClock)
                {
                    trecord.deviceClockOffset =
                        elapsed - outputVideoDevice()->nextFrameTime();
                }
            }

            if (outDeviceClock)
            {
                elapsed = outputVideoDevice()->nextFrameTime();
            }

            audioVarLock();
            const bool tsvalid = audioTimeShiftValid();
            audioVarUnLock();

            if (!audioRenderer() || (elapsed >= 0.0 && tsvalid)
                || outDeviceClock)
            {
                newFrame = targetFrame(elapsed);

                // cerr << "m_frame " << m_frame << " newFrame " << newFrame <<
                // " elap " << elapsed << " shft " << m_shift << " fps " <<
                // fps() << " untilPred " << untilPredicted << " tsvalid " <<
                // tsvalid << endl;

                if (RV_VSYNC_ADD_OFFSET && elapsedOffset == 0.0
                    && newFrame != m_frame)
                {
                    elapsedOffset =
                        float(newFrame - rangeStart() - m_shift) / fps()
                        - elapsedPlaySeconds();
                }

                if (m_bufferWait || realtime())
                {
                    m_skipped = (newFrame - nextFrame) / inc();
                    if (inc() * m_skipped < 0)
                        m_skipped = 0;
                }
                else if (outDeviceClock)
                {
                    //
                }
                else
                {
                    bool shiftCheck = false;

                    if ((inc() > 0 && nextFrame < newFrame)
                        || (inc() < 0 && nextFrame > newFrame))
                    {
                        //
                        //  Don't allow a skip
                        //

                        newFrame = nextFrame;
                        shiftCheck = true;
                    }
                    else if (newFrame != currentFrame)
                    {
                        shiftCheck = true;
                    }

                    if (shiftCheck)
                    {
                        //
                        //  For play-all-frames, this resets the relative
                        //  time to the current frame. So the next frame
                        //  only has to show up 1/fps seconds from now
                        //  instead of relative to the play start
                        //  time. This prevents the "speeding up" behavior
                        //  in which the fps sky rockets to catch up to
                        //  the elapsed time.
                        //

                        int lastShift = m_shift;

                        if (inc() > 0)
                        {
                            m_shift = int(newFrame - rangeStart()
                                          - elapsed * fps() + 0.5);

                            if (m_shift < lastShift)
                            {
                                if (debugPlayback)
                                    cout << "DEBUG: negative shift old="
                                         << lastShift << ", new=" << m_shift
                                         << endl;
                            }
                        }
                        else
                        {
                            m_shift = int(newFrame - rangeStart()
                                          + elapsed * fps() - 0.5);

                            if (m_shift > lastShift)
                            {
                                if (debugPlayback)
                                    cout << "DEBUG: positive shift old="
                                         << lastShift << ", new=" << m_shift
                                         << endl;
                            }
                        }

                        if (debugPlayback && lastShift != m_shift)
                        {
                            cout << "DEBUG: shift correction (play-all-frames) "
                                    "from "
                                 << lastShift << " to " << m_shift << endl;
                        }
                    }
                }

                if ((newFrame - rangeStart()) % inc())
                {
                    //
                    //  Frame is not included in the play range -- just skip it
                    //
                    return;
                }

                //
                //  Do a sanity check for messed up timers. This can
                //  happen when trying to lock to the audio timer and the
                //  sound card is crappy -- its timer will not be
                //  stable. To remidy that, don't allow the frame to ever
                //  move backwards (or forwards if we're playing
                //  backwards). This prevents a visible jerk, but it
                //  doesn't completely solve the problem. We should
                //  probably notify the user somehow that this is
                //  happening or fall back to the cpu timer.
                //

                if (inc() > 0)
                {
                    if (newFrame < m_frame)
                        newFrame = m_frame;
                }
                else
                {
                    if (newFrame > m_frame)
                        newFrame = m_frame;
                }

                m_frame = newFrame;
            }
        }

        //
        //  Wrap frames
        //

        m_wrapping = false;

        if (playing)
        {
            if (m_frame >= outPoint() && inc() > 0)
            {
                switch (m_playMode)
                {
                case PlayPingPong:
                    stop("turn-around");
                    m_inc *= -1;
                    setFrameInternal(outPoint() - 1, "turn-around");
                    m_wrapping = true;
                    play("turn-around");
                    break;
                case PlayLoop:
                    stop("turn-around");
                    setFrameInternal(inPoint(), "turn-around");
                    m_wrapping = true;
                    play("turn-around");
                    break;
                case PlayOnce:
                    stop("turn-around");
                    setFrameInternal(outPoint() - 1, "turn-around");
                    m_wrapping = true;
                    break;
                }
            }
            else if (m_frame < inPoint() && inc() < 0)
            {
                switch (m_playMode)
                {
                case PlayLoop:
                    stop("turn-around");
                    setFrameInternal(outPoint() - 1, "turn-around");
                    m_wrapping = true;
                    play("turn-around");
                    break;
                case PlayPingPong:
                    stop("turn-around");
                    setFrameInternal(inPoint() + 1, "turn-around");
                    m_inc *= -1;
                    m_wrapping = true;
                    play("turn-around");
                    break;
                case PlayOnce:
                    stop("turn-around");
                    setFrameInternal(inPoint(), "turn-around");
                    m_wrapping = true;
                    break;
                }
            }
        }

        if (m_frame >= rangeEnd())
            setFrameInternal(rangeEnd() - 1);
        if (m_frame < rangeStart())
            setFrameInternal(rangeStart());

        //
        //  The overhead (reading and drawing). Throw the frame up and
        //  figure outhow long it took. If we're reading a complex file
        //  format off disk (like exr) this can take a while. Compute the
        //  overhead time for next frame prediction -- note that the
        //  constant in the lerp is completely subjective.
        //

        userRenderEvent("pre-render", "");

        if (true)
        {
            if (playing && m_cacheMode == BufferCache)
            {
                if (m_maxBufferedWaitSeconds > 0.0
                    && !graph().cache().isFrameCached(successorFrame(m_frame))
                    && isCaching())
                {
                    stop("buffering");
                    m_bufferWait = true;
                }
            }
            m_rendering = true;
            m_wantsRedraw = false;

            if (debugProfile)
            {
                ProfilingRecord& trecord = currentProfilingSample();
                trecord.evaluateStart = profilingElapsedTime();
                trecord.evaluateEnd = trecord.evaluateStart;
                graph().beginProfilingSample();
            }

            try
            {
                HOP_CALL(glFinish();)
                HOP_PROF("Session::render - render_v2 - evaluateForDisplay");

                evaluateForDisplay();

                HOP_CALL(glFinish();)
            }
            catch (BufferNeedsRefillExc& exc)
            {
                //
                //  Don't wait here unless there's actually video to cache
                //  (maybe this evaluation is only generating audio.
                //

                bool hasFB = false;
                for (size_t i = 0; i < m_displayFBStatus.size(); i++)
                {
                    if (m_displayFBStatus[i].fb)
                    {
                        hasFB = true;
                        break;
                    }
                }

                if (maxBufferedWaitTime() != 0.0 && hasFB)
                {
                    stop("buffering");
                    if (!m_wrapping)
                        setFrameInternal(currentFrame);

                    if (playing)
                    {
                        m_bufferWait = true;
                    }
                    try
                    {
                        evaluateForDisplay();
                    }
                    catch (...)
                    {
                    }
                }
            }

            if (debugProfile)
            {
                ProfilingRecord& trecord = currentProfilingSample();
                trecord.evaluateEnd = profilingElapsedTime();
                graph().endProfilingSample();
            }

            const float clockMult = fps() / currentTargetFPS();

            if (hasAudio() && (!realtime() || clockMult != 1.0)
                && !outDeviceClock)
            {
                //
                //  This is the multiplier on the clock. If we're playing
                //  at 48 fps and the frame's fps is 24.0 then we're
                //  playing with a clock multiple of 2.0. That means
                //  seeking into the audio may require skipping that far
                //  ahead into the samples. (dropping samples)
                //

                // NB: elapsedPlaySeconds() goes to max numeric value when
                // playback
                //     wraps backwards from frame zero. So we check against
                //     this.
                const double elapsedPlaySecs = elapsedPlaySeconds();
                if ((currentFrame != m_frame && audioRenderer())
                    && (elapsedPlaySecs < numeric_limits<double>::max()))
                {
                    const bool forwards = (inc() > 0);
                    const double atime =
                        (m_frame - rangeStart()) / fps() * clockMult;
                    const double elapsed =
                        m_shift / fps() * clockMult
                        + (forwards ? elapsedPlaySecs : -elapsedPlaySecs);
                    const double diff = ::fabs(atime - elapsed);

                    if (diff > (1 / (currentTargetFPS() * m_audioDrift)))
                    {
                        if (AudioRenderer::debug)
                        {
                            cout << "DEBUG: audio ";
                            if (diff > 0)
                                cout << "+";
                            cout << diff << " seconds" << endl;
                        }

                        //
                        //  NOTE: missing the case when we're playing
                        //  backwards. The backwards playback may drift beyond
                        //  a reasonable amount.
                        //

                        if (forwards)
                        {
                            audioVarLock();
                            setAudioTimeSnapped(atime);
                            audioVarUnLock();
                        }
                    }
                }
            }

            m_wrapping = false;

            try
            {
                HOP_CALL(glFinish();)
                HOP_PROF("Session::render - render_v2 - internalRender");

                if (debugProfile)
                {
                    ProfilingRecord& trecord = currentProfilingSample();
                    trecord.internalRenderStart = profilingElapsedTime();
                    m_renderer->initProfilingState(&m_profilingTimer);
                }

                AuxUserRender auxRender(this);
                AuxAudioRenderer auxAudio(this);

                waitForUploadToFinish();
                m_waitForUploadThreadPrefetch =
                    m_preEval && useThreadedUpload();

                if (m_waitForUploadThreadPrefetch)
                {
                    //
                    //  In this case we need to evaluate next next frame
                    //  before rendering
                    //
                    //

                    preEval();

                    m_renderer->render(m_frame, m_displayImage, &auxRender,
                                       &auxAudio, m_preDisplayImage);
                }
                else
                {
                    m_renderer->render(m_frame, m_displayImage, &auxRender,
                                       &auxAudio);
                }

                if (debugProfile)
                {
                    ProfilingRecord& trecord = currentProfilingSample();
                    trecord.internalRenderEnd = profilingElapsedTime();
                    trecord.frame = m_frame;
                    trecord.renderUploadPlaneTotal =
                        m_renderer->profilingState().uploadPlaneTime;
                    trecord.renderFenceWaitTotal =
                        m_renderer->profilingState().fenceWaitTime;
                }

                HOP_CALL(glFinish();)
            }
            catch (RendererNotSupportedExc& exc)
            {
            }
            catch (std::exception& exc)
            {
                m_displayImage = m_errorImage;
                m_errorImage->fb->attribute<string>("Error") = exc.what();
            }
            catch (...)
            {
                m_displayImage = m_errorImage;
                m_errorImage->fb->attribute<string>("Error") =
                    "Error during rendering";
            }

            bool calcFPSWithDevClock =
                (outDeviceClock && outputVideoDevice()->timing().hz != 0.0);

            if (!playing)
            {
                m_fpsCalc->reset();
                m_realfps = 0.0;
                m_lastDevFrame = -1;
            }
            else if (calcFPSWithDevClock)
            {
                float devFrame = outputVideoDevice()->nextFrame();
                if (m_lastDevFrame == -1)
                {
                    m_lastViewFrame = m_frame;
                    m_lastDevFrame = devFrame;
                }
                else if (abs(m_frame - m_lastViewFrame) >= 4)
                {
                    float framesSample =
                        ::fabs(float(m_frame - m_lastViewFrame));
                    double nelapsed = elapsedPlaySeconds();
                    float fpsSample =
                        framesSample / (nelapsed - m_lastCheckTime);

                    m_fpsCalc->setTargetFps(m_fps);
                    m_fpsCalc->addSample(fpsSample);
                    m_realfps =
                        m_fpsCalc->fps(outputVideoDevice()->nextFrameTime());
                    if (fabs(m_realfps - m_fps) < 0.125)
                        m_realfps = m_fps;

                    m_lastCheckTime = nelapsed;
                    m_lastViewFrame = m_frame;
                    m_lastDevFrame = devFrame;
                }
            }
            else if (abs(m_frame - m_lastCheckFrame) >= 4)
            {
                double nelapsed = elapsedPlaySeconds();
                double fps = double(abs(m_frame - m_lastCheckFrame))
                             / (nelapsed - m_lastCheckTime);
                //  cerr << "fps " << fps << " f " << m_frame << " lcf " <<
                //  m_lastCheckFrame << endl;

                m_fpsCalc->setTargetFps(m_fps);
                /*
                float relError = fabs((fps-m_fps)/m_fps);
                if (relError > 0.1)
                {
                    cerr << "frame " << m_frame << " hiccup " << relError <<
                endl;
                }
                if (relError > worstFpsError)
                {
                    worstFpsError = relError;
                    cerr << "frame " << m_frame << " worst " << relError <<
                endl;
                }
                cerr << "frame " << m_frame << " fps " << fps <<
                        " relError " << relError << endl;
                */
                m_fpsCalc->addSample(fps);
                m_realfps = m_fpsCalc->fps(nelapsed);

                //
                //  We decide arbitrarily here that a differences of
                //  less than 0.125 is more distracting than interesting.
                //

                if (fabs(m_realfps - m_fps) < 0.125)
                    m_realfps = m_fps;

                m_lastCheckTime = nelapsed;
                m_lastCheckFrame = m_frame;
            }
        }

        if (m_lastFrame != m_frame && playing)
        {
            HOP_CALL(glFinish();)
            HOP_PROF("Session::render - render_v2 - frameChangeEvent");

            if (debugProfile)
            {
                ProfilingRecord& trecord = currentProfilingSample();
                trecord.frameChangeEventStart = profilingElapsedTime();
            }

            send(frameChangedMessage());
            size_t f0;
            // if (debugGC) f0 = GC_get_free_bytes();
            userGenericEvent("frame-changed", "");
            // m_frameChangedSignal(m_frame, "");
            // if (debugGC)
            //{
            //     size_t d = f0 - GC_get_free_bytes();
            //     if (d > 0) cout << "GC: render frame-changed allocated " << d
            //     << " bytes." << endl;
            // }

            if (debugProfile)
            {
                ProfilingRecord& trecord = currentProfilingSample();
                trecord.frameChangeEventEnd = profilingElapsedTime();
            }
            HOP_CALL(glFinish();)
        }

        m_lastFrame = m_frame;
        m_rendering = false;
    }

    void Session::waitForUploadToFinish()
    {
        // if last frame was uploading textures using copy engine & prefetch
        // make sure it has finished before we destroy the IPImage
        if (m_renderer && m_waitForUploadThreadPrefetch)
        {
            m_renderer->waitToDraw();
            m_waitForUploadThreadPrefetch = false;
        }
    }

    string Session::userGenericEvent(const string& eventName,
                                     const string& contents,
                                     const string& senderName)
    {
        if (m_beingDeleted)
            return "";

        if (m_filterLiveReviewEvents
            && (eventName == "mode-manager-toggle-mode"
                || (eventName == "remote-eval"
                    && (contents == "commands.stop()"
                        || contents
                               == "commands.scrubAudio(false); "
                                  "commands.setInc(-1); commands.play();"
                        || contents == "commands.setInc(-1); commands.play();"
                        || contents
                               == "commands.scrubAudio(false); "
                                  "commands.setInc(1); commands.play();"
                        || contents == "commands.setInc(1); commands.play();"
                        || contents == "rvui.previousMarkedFrame()"
                        || contents == "rvui.nextMarkedFrame()"
                        || contents == "extra_commands.stepForward(1)"
                        || contents == "extra_commands.stepBackward(1)"
                        || contents == "commands.setPlayMode(PlayOnce)"
                        || contents == "commands.setPlayMode(PlayPingPong)"
                        || contents == "commands.setPlayMode(PlayLoop)"))))
        {
            GenericStringEvent event("live-review-blocked-event", this, "");
            sendEvent(event);
            return "";
        }

        // cout << "userGenericEvent: " << eventName << " '" << contents << "'"
        // << endl;
        Session* s = currentSession();
        m_currentSession = this;
        GenericStringEvent event(eventName, this, contents, senderName);
        sendEvent(event);
        m_currentSession = s;

        if (eventName == "before-progressive-loading")
        {
            m_preFirstNonEmptyRender = true;
        }

        if (m_preFirstNonEmptyRender && !m_postFirstNonEmptyRender
            && eventName == "after-progressive-loading")
        {
            m_postFirstNonEmptyRender = true;
        }

        return event.returnContent();
    }

    string Session::userRawDataEvent(const string& eventName,
                                     const string& contentType,
                                     const char* data, size_t n,
                                     const char* utf8, const string& senderName)
    {
        if (m_beingDeleted)
            return "";

        Session* s = currentSession();
        m_currentSession = this;
        RawDataEvent event(eventName, this, contentType, data, n, utf8, 0,
                           senderName);
        sendEvent(event);
        m_currentSession = s;
        return event.returnContent();
    }

    void Session::pixelBlockEvent(const string& eventName, const string& interp,
                                  const char* data, size_t size)
    {
        if (m_beingDeleted)
            return;

        vector<string> tokens;
        stl_ext::tokenize(tokens, interp, "(), ");

        //
        //  Default values
        //

        string ename = eventName; // could be overriden by interp
        string media = "";
        string layer = "-";
        string view = "-";
        int x = 0;    // tile's x offset
        int y = 0;    // tile's y offset
        size_t w = 0; // tile width
        size_t h = 0; // tile height
        int f = 0;    // frame

        for (size_t i = 0; i < tokens.size(); i++)
        {
            vector<string> attr;
            stl_ext::tokenize(attr, tokens[i], "=");

            if (!attr.empty())
            {
                if (attr.size() == 2)
                {
                    const string& n = attr[0];
                    const string& v = attr[1];

                    if (n == "w")
                        w = atoi(v.c_str());
                    else if (n == "h")
                        h = atoi(v.c_str());
                    else if (n == "x")
                        x = atoi(v.c_str());
                    else if (n == "y")
                        y = atoi(v.c_str());
                    else if (n == "f")
                        f = atoi(v.c_str());
                    else if (n == "event")
                        ename = v;
                    else if (n == "media")
                        media = v;
                    else if (n == "layer")
                        layer = v;
                    else if (n == "view")
                        view = v;
                }
            }
        }

        PixelBlockTransferEvent event(ename, this, media, layer, view, f, x, y,
                                      w, h, data, size, 0);

        sendEvent(event);
    }

    void Session::fullScreenMode(bool b)
    {
        if (b == m_fullScreen)
            return;
        m_fullScreen = !m_fullScreen;

        if (m_fullScreen)
        {
            send(fullScreenOnMessage());
        }
        else
        {
            send(fullScreenOffMessage());
        }
    }

    bool Session::isFullScreen() { return m_fullScreen; }

    void Session::physicalDeviceChanged(const VideoDevice* device)
    {
        if (device == m_controlVideoDevice)
        {
            graph().setPrimaryDisplayGroup(device);
        }

        if (m_avPlaybackVersion != 2)
        {
            m_physicalVideoDeviceChangedSignal(device);
        }
    }

    EventNode::Result Session::propagateEvent(const Event& event)
    {
        if (m_beingDeleted)
            return EventIgnored;

        //
        //  All events are handled by the init script except for
        //  RenderContextChangeEvent which is handled directly by
        //  Session. (Any cached GL state needs to be deleted)
        //

        // Session* s = activeSession();
        Session* s = currentSession();
        m_currentSession = this;

        if (const ModifierEvent* me =
                dynamic_cast<const ModifierEvent*>(&event))
        {
            m_receivingEvents = true;
            event.handled = true;
        }
        else if (const PointerEvent* pe =
                     dynamic_cast<const PointerEvent*>(&event))
        {
            m_receivingEvents = pe->name() != "pointer--leave";
            event.handled = true;
        }
        else if (const RenderContextChangeEvent* e =
                     dynamic_cast<const RenderContextChangeEvent*>(&event))
        {
            clearVideoDeviceCaches();
            event.handled = true;
        }
        else if (const VideoDeviceContextChangeEvent* e =
                     dynamic_cast<const VideoDeviceContextChangeEvent*>(&event))
        {
            physicalDeviceChanged(e->m_device);
        }

        if (const GenericStringEvent* se =
                dynamic_cast<const GenericStringEvent*>(&event))
        {
            if (se->name() == "graph-range-change")
            {
                lockRangeDirty();
                m_rangeDirty = true;
                unlockRangeDirty();
                return EventAccept;
            }
            else if (se->name() == "graph-node-inputs-changed")
            {
                askForRedraw();
            }
            else if (se->name() == "graph-before-node-deleted")
            {
                //
                //  Just in case there's an image in there that was owned
                //  by this node
                //

                checkInPreDisplayImage();
                checkInDisplayImage();
            }
            else if (se->name() == "graph-validation-begin")
            {
                // maybe push some validation context
            }
            else if (se->name() == "graph-validation-end")
            {
                // pop validation context from above
            }

            else if (se->name() == "graph-program-flush")
            {
                renderer()->flushProgramCache();
                // should prob have a boost signal for this as well
            }
        }

        m_currentSession = s;
        EventNode::Result res = Document::propagateEvent(event);
        return res;
    }

    void Session::findCurrentNodesByTypeName(NodeVector& nodes,
                                             const string& typeName)
    {
        graph().findNodesByTypeName(m_frame, nodes, typeName);
    }

    void Session::findNodesByTypeName(NodeVector& nodes, const string& typeName)
    {
        graph().findNodesByTypeName(nodes, typeName);
    }

    void Session::findProperty(PropertyVector& props, const string& name)
    {
        graph().findProperty(m_frame, props, name);
    }

    unsigned int Session::currentFrameState() const
    {
        unsigned int status = 0;

        for (size_t i = 0; i < m_displayFBStatus.size(); i++)
        {
            if (m_displayFBStatus[i].partial)
                status |= PartialStatus;
            if (m_displayFBStatus[i].loading)
                status |= LoadingStatus;
            if (m_displayFBStatus[i].error)
                status |= ErrorStatus;
            if (m_displayFBStatus[i].warning)
                status |= WarningStatus;
            if (m_displayFBStatus[i].noImage)
                status |= NoImageStatus;
        }

        return status;
    }

    bool Session::currentStateIsIncomplete() const
    {
        for (size_t i = 0; i < m_displayFBStatus.size(); i++)
        {
            if (m_displayFBStatus[i].partial || m_displayFBStatus[i].loading)
            {
                return true;
            }
        }

        return m_displayFBStatus.empty();
    }

    bool Session::currentStateIsError() const
    {
        for (size_t i = 0; i < m_displayFBStatus.size(); i++)
        {
            if (m_displayFBStatus[i].error)
                return true;
        }

        return false;
    }

    //----------------------------------------------------------------------

    Vec2i Session::maxSize() const
    {

        //
        //  This seems wrong.  The previous code (below) checked all
        //  frames, which could be different sizes.  The new code
        //  assumes that the size is not dependent on the frame, which
        //  is maybe correct in 3.10, but if so, it shouldn't depend on
        //  pixelAspect, which could certainly change from frame to
        //  frame.
        //

        IPNode::ImageStructureInfo info = graph().root()->imageStructureInfo(
            graph().contextForFrame(rangeStart()));

        return Vec2i(info.width, info.height);
    }

    //----------------------------------------------------------------------
    //
    //  Some actual GL in Session here. In the event that we need a non-GL
    //  renderer (DX11, etc). This would need to change (as would all of
    //  the GL in Mu?)
    //

#include <TwkGLF/GL.h>

    void Session::userRenderEvent(const std::string& name,
                                  const std::string& contents)
    {
        if (m_beingDeleted)
            return;

        // const VideoDevice* d = m_outputVideoDevice;
        const VideoDevice* d =
            m_eventVideoDevice ? m_eventVideoDevice : m_outputVideoDevice;
        VideoDevice::Resolution r = d->resolution();
        Session* s = currentSession();
        m_currentSession = this;

        RenderEvent event(name, this, r.width, r.height, contents);
        sendEvent(event);

        m_currentSession = s;
    }

    struct MissingImageChecker
    {
        MissingImageChecker(Session* _session, const VideoDevice* _d, int w,
                            int h)
            : session(_session)
            , d(_d)
            , width(w)
            , height(h)
        {
        }

        Session* session;
        const VideoDevice* d;
        int width;
        int height;

        void operator()(IPImage* i)
        {
            if (i->missing)
            {
                ostringstream contents;
                contents << i->missingLocalFrame;
                contents << ";";

                if (i->fb->hasAttribute("RVSource"))
                {
                    contents << i->fb->attribute<string>("RVSource");
                }
                if (session->batchMode())
                {
                    session->addMissingInfo(contents.str());
                }
                else
                {
                    RenderEvent event("missing-image", session, d, width,
                                      height, contents.str());
                    session->sendEvent(event);
                }
            }
        }
    };

    void Session::userRender(const VideoDevice* d, const char* eventName,
                             const string& contents)
    {
        if (ImageRenderer::reportGL())
        {
            // these calls are expensive should only be called in debug mode

            if (GLuint err = glGetError())
            {
                cerr << "GL ERROR: *BEFORE* userRender: event=" << eventName
                     << " : " << TwkGLF::errorString(err) << endl;
            }
        }

        Session* s = currentSession();
        m_currentSession = this;

        if (m_batchMode)
            m_missingFrameInfos.clear();
        VideoDevice::Resolution r = d->resolution();
        MissingImageChecker checker(this, d, r.width, r.height);
        foreach_ip(m_displayImage, checker);

        RenderEvent event(eventName, this, d, r.width, r.height, contents);
        sendEvent(event);

        m_currentSession = s;

        if (ImageRenderer::reportGL())
        {
            if (GLuint err = glGetError())
            {
                cerr << "GL ERROR: after userRender: event=" << eventName
                     << " : " << TwkGLF::errorString(err) << endl;
            }
        }
    }

    void Session::setAudioTime(double t)
    {
        m_audioStartSample =
            timeToSamples(t, audioRenderer()->deviceState().rate);
    }

    void Session::setAudioTimeSnapped(double t)
    {
        if (m_avPlaybackVersion == 2)
        {
            setAudioTime(t);
        }
        else
        {
            const Time hz = outputDeviceHz();
            const Time stime = std::floor(t * hz) / hz;
            setAudioTime(stime);
        }

        const AudioRenderer::DeviceState& state =
            audioRenderer()->deviceState();
        if (size_t bufferSize =
                state.framesPerBuffer / TwkAudio::channelsCount(state.layout))
        {
            size_t mod = m_audioStartSample % bufferSize;
            m_audioStartSample -= mod;

            if (mod >= bufferSize / 2)
            {
                m_audioStartSample += bufferSize;
            }
        }
    }

    const Session::CacheStats& Session::cacheStats()
    {
        FBCache& cache = graph().cache();

        //
        //  the cacheStats() function will try and lock, but if it cannot
        //  it will return false. its not important that this info ever be
        //  precise, but its important that the display thread (which
        //  calls this) is not stopped.
        //

        if (cache.cacheStats(m_cacheStats))
        {
            // succeeded
        }

        //
        //  Its assumed that audioSecondsCached() somehow manages to use
        //  atomic operations, but at this point we don't care
        //

        m_cacheStats.audioSecondsCached = graph().audioSecondsCached();

        return m_cacheStats;
    }

    Session::FpsCalculator::FpsCalculator(int n)
        : m_maxNumSamples(n)
        , m_targetFps(0.0)
    {
        reset();
        m_samples.resize(n);
    }

    void Session::FpsCalculator::addSample(float fps)
    {
        if (m_firstSample)
        {
            m_firstSample = false;
            return;
        }
        //  cerr << "addSample fps " << fps << " nextSample " << m_nextSample <<
        //  " numSamples " << m_numSamples << endl;
        m_samples[m_nextSample] = fps;
        m_nextSample = (m_nextSample + 1) % m_maxNumSamples;
        m_numSamples = (m_numSamples == m_maxNumSamples) ? m_maxNumSamples
                                                         : m_numSamples + 1;
    }

    float Session::FpsCalculator::fps(double time)
    {
        //  cerr << "Session::FpsCalculator::fps called, time " << time << ",
        //  numSamples " << m_numSamples << endl;
        if (fabs(time - m_lastUpdateTime) > 0.2 && m_numSamples)
        {
            m_lastUpdateTime = time;

            // float ave  = 0.0;
            float wAve = 0.0;
            float wSum = 0.0;
            float w, diff;
            //  float confidence = 1.0;//0.5 +
            //  0.5*float(m_numSamples)/float(m_maxNumSamples);
            for (int i = 0; i < m_numSamples; ++i)
            {
                diff = m_samples[i] - m_targetFps;
                w = fabs(diff) / m_targetFps;
                //
                //  We get outliers, that we want to weight lightly, but
                //  if it's near the target, do unwieghted averaging.
                //  But only when we have some samples.
                //  0.1 chosen empirically.
                //
                if (w < 0.1)
                    w = 0.1;

                /*
                if (w < 0.0001) w = 0.0001;
                if (w < 0.09)
                {
                    w = 0.09*confidence + w*(1.0-confidence);
                }
                */
                // w = w*w;
                wAve += m_samples[i] / w;
                wSum += 1.0 / w;
                // ave += m_samples[i];
            }
            // float ww = 0.75*(m_numSamples/m_maxNumSamples);
            // ww = 0.0;
            // m_fps = ww*ave/float(m_numSamples) + (1.0-ww)*wAve/wSum;
            m_fps = wAve / wSum;
            // cerr << "m_fps " << m_fps << " ns " << m_numSamples <<
            //         " maxns " << m_maxNumSamples <<
            //         " sz " << m_samples.size() << endl;
        }
        return m_fps;
    }

    void Session::FpsCalculator::reset(bool soft)
    {
        if (!soft)
        {
            /*
            if (m_numSamples != 0)
            {
                cerr << "FpsCalculator::reset m_numSamples " <<
                        m_numSamples << endl;
            }
            */
            m_numSamples = 0;
            m_nextSample = 0;
            m_lastUpdateTime = 0.0;
        }
        m_firstSample = true;
    }

    void Session::FpsCalculator::setTargetFps(float fps)
    {
        if (fps != m_targetFps)
        {
            //  cerr << "new target fps " << fps << endl;
            reset();
            m_targetFps = fps;
            m_fps = fps;
        }
    }

    //
    //  Set margins from new margins, except where new margin is -1.
    //

    static Session::Margins
    adjustMarginsWithDefaults(Session::Margins& newMargins,
                              Session::Margins& oldMargins)
    {
        Session::Margins m = oldMargins;

        if (newMargins.left != -1)
            m.left = newMargins.left;
        if (newMargins.right != -1)
            m.right = newMargins.right;
        if (newMargins.top != -1)
            m.top = newMargins.top;
        if (newMargins.bottom != -1)
            m.bottom = newMargins.bottom;

        return m;
    }

    //
    //  Set margins on the eventDevice unless allDevices is true
    //

    void Session::setMargins(float left, float right, float top, float bottom,
                             bool allDevices)
    {
        const VideoDevice* cd = m_controlVideoDevice;
        const VideoDevice* od = m_outputVideoDevice;
        const VideoDevice* ed = m_eventVideoDevice;

        if (allDevices || ed == cd)
        {
            Margins om = cd->margins();
            Margins m(left, right, top, bottom);
            m = adjustMarginsWithDefaults(m, om);

            cd->setMargins(m.left, m.right, m.top, m.bottom);

            //
            //  Only send margins-changed for controller.
            //
            ostringstream contents;
            contents << om.left << " " << om.right << " " << om.top << " "
                     << om.bottom;
            userGenericEvent("margins-changed", contents.str(), "session");
            m_marginsChangedSignal(cd);
        }
        if (allDevices || ed == od)
        {
            Margins om = od->margins();
            Margins m(left, right, top, bottom);
            m = adjustMarginsWithDefaults(m, om);

            od->setMargins(m.left, m.right, m.top, m.bottom);
        }
    }

    double Session::profilingElapsedTime()
    {
        return m_profilingTimer.elapsed();
    }

    Session::ProfilingRecord& Session::beginProfilingSample()
    {
        if (m_profilingSamples.empty())
        {
            m_profilingTimer.start();
            m_profilingSamples.reserve(1024);
        }

        m_profilingSamples.resize(m_profilingSamples.size() + 1);
        return m_profilingSamples.back();
    }

    Session::ProfilingRecord& Session::currentProfilingSample()
    {
        return m_profilingSamples.back();
    }

    void Session::endProfilingSample()
    {
        // m_profilingSamples.back().gccount = GC_gc_no;
        m_profilingSamples.back().gccount = 0;
    }

    void Session::dumpProfilingToFile(ostream& file)
    {
        file << "# fields are in pairs start,end,start,end, ... except last "
                "six are not pairs"
             << endl;
        file << "# render, swap, evaluate, user render, frame change event, "
                "internal render, internal prefetch, cache test, eval "
                "internal, eval ID, cacheQuery, cacheEval, io, expected vsync "
                "time, gc#, frame "
             << endl;

        const IPGraph::ProfilingVector& graphSamples =
            graph().profilingSamples();

        assert(graphSamples.size() == m_profilingSamples.size());
        file.precision(10);

        for (size_t i = 0; i < m_profilingSamples.size(); i++)
        {
            const ProfilingRecord& t = m_profilingSamples[i];
            const IPGraph::EvalProfilingRecord& gt = graphSamples[i];

            file << "R0=" << t.renderStart << ",R1=" << t.renderEnd
                 << ",S0=" << t.swapStart << ",S1=" << t.swapEnd
                 << ",E0=" << t.evaluateStart << ",E1=" << t.evaluateEnd
                 << ",U0=" << t.userRenderStart << ",U1=" << t.userRenderEnd
                 << ",FC0=" << t.frameChangeEventStart
                 << ",FC1=" << t.frameChangeEventEnd
                 << ",IR0=" << t.internalRenderStart
                 << ",IR1=" << t.internalRenderEnd
                 << ",PR0=" << t.internalPrefetchStart
                 << ",PR1=" << t.internalPrefetchEnd
                 << ",CT0=" << gt.cacheTestStart << ",CT1=" << gt.cacheTestEnd
                 << ",EI0=" << gt.evalInternalStart
                 << ",EI1=" << gt.evalInternalEnd << ",ID0=" << gt.evalIDStart
                 << ",ID1=" << gt.evalIDEnd << ",CQ0=" << gt.cacheQueryStart
                 << ",CQ1=" << gt.cacheQueryEnd << ",CE0=" << gt.cacheEvalStart
                 << ",CE1=" << gt.cacheEvalEnd << ",IO0=" << gt.ioStart
                 << ",IO1=" << gt.ioEnd << ",THA0=" << gt.restartThreadsAStart
                 << ",THA1=" << gt.restartThreadsAEnd
                 << ",THB0=" << gt.restartThreadsBStart
                 << ",THB1=" << gt.restartThreadsBEnd
                 << ",CTL0=" << gt.cacheTestLockStart
                 << ",CTL1=" << gt.cacheTestLockEnd
                 << ",SDSP0=" << gt.setDisplayFrameStart
                 << ",SDSP1=" << gt.setDisplayFrameEnd
                 << ",FCT0=" << gt.frameCachedTestStart
                 << ",FCT1=" << gt.frameCachedTestEnd
                 << ",WAK0=" << gt.awakenThreadsStart
                 << ",WAK1=" << gt.awakenThreadsEnd
                 << ",PRR0=" << t.prefetchRenderStart
                 << ",PRR1=" << t.prefetchRenderEnd
                 << ",PRUP=" << t.prefetchUploadPlaneTotal
                 << ",RRUP=" << t.renderUploadPlaneTotal
                 << ",RFW=" << t.renderFenceWaitTotal
                 << ",EST=" << t.expectedSyncTime
                 << ",DCO=" << t.deviceClockOffset << ",GC=" << t.gccount
                 << ",F=" << t.frame << endl;
        }
    }

    //----------------------------------------------------------------------
    //
    //  Session file format I/O
    //

    void Session::writeProfile(const string& filename, IPNode* node,
                               const WriteRequest& inrequest)
    {
        WriteObjectVector headerObjects;
        NodeSet nodes;
        nodes.insert(node);

        const string comments = inrequest.optionValue<string>("comments", "");

        WriteRequest request = inrequest;
        request.setOption("writeNodes", nodes);
        request.setOption("profile", true);

        PropertyContainer pc;

        StringPair replacePair;

        //
        //  NOTE: this may be an issue on non 64 bit machines or machines
        //  where sizeof(unsigned int) != 4
        //

        assert(sizeof(size_t) == 8);
        assert(sizeof(unsigned int) == 4);

        uuids::uuid u = uuids::random_generator()();
        vector<unsigned char> v(u.size());
        copy(u.begin(), u.end(), v.begin());
        unsigned int* ip = reinterpret_cast<unsigned int*>(&v[0]);
        unsigned int h32 = ip[0] ^ ip[1] ^ ip[2] ^ ip[3]; // 16 bytes total

        ostringstream profileObjName;
        profileObjName << "profile_" << hex << h32;

        headerObjects.push_back(
            WriteObject(&pc, profileObjName.str(), "Profile", 1));

        ostringstream newName;
        newName << hex << h32;

        replacePair.first = node->name();
        replacePair.second = newName.str();
        request.setOption("nameReplace", replacePair);

        pc.declareProperty<StringProperty>("root.name", newName.str());
        pc.declareProperty<StringProperty>("root.comment", comments);

        writeGTO(filename, headerObjects, request, false);
    }

    namespace
    {

        string mungeName(const StringPair& replacePair, const string& name)
        {
            return replacePair.first != ""
                       ? algorithm::replace_all_copy(name, replacePair.first,
                                                     replacePair.second)
                       : name;
        }

        void makeTopLevelSet(PropertyContainer* connections,
                             GTOReader::Containers& containers,
                             Session::NameSet& topLevelSet)
        {
            //
            //  NOTE: the RV version of this function allows all sorts of
            //  combinations of old and new style file format, this one
            //  requires the new style files that have both top nodes and
            //  string pair connections.
            //

            StringPairProperty* cons =
                connections->property<StringPairProperty>(
                    "evaluation.connections");
            StringProperty* top =
                connections->property<StringProperty>("top.nodes");

            std::copy(top->valueContainer().begin(),
                      top->valueContainer().end(),
                      std::inserter(topLevelSet, topLevelSet.begin()));

            const StringPairProperty::container_type& array =
                cons->valueContainer();

            for (size_t i = 0; i < array.size(); i++)
            {
                topLevelSet.insert(array[i].first);
                topLevelSet.insert(array[i].second);
            }
        }

        void inputsForNode(PropertyContainer* connections, IPGraph& graph,
                           const string& name, IPGraph::IPNodes& inputs)
        {
            StringPairProperty* cons =
                connections->property<StringPairProperty>(
                    "evaluation.connections");

            inputs.clear();

            for (size_t i = 0; i < cons->size(); i++)
            {
                const pair<string, string>& arrow = (*cons)[i];

                if (arrow.second == name)
                {
                    if (IPNode* n = graph.findNode(arrow.first))
                    {
                        inputs.push_back(n);
                    }
                    else
                    {
                        cout << "WARNING: could not find node " << arrow.first
                             << " when trying to connect inputs for " << name
                             << endl;
                    }
                }
            }
        }

#ifdef PLATFORM_WINDOWS
#define PAD "%06d"
#else
#define PAD "%06zd"
#endif

        string uniqueName(string name, Session::NameSet& disallowedNames)
        {
            static char buf[16];
            static RegEx endRE("(.*[^0-9])([0-9]+)");
            static RegEx midRE(
                "(.*[^0-9])([0-9][0-9][0-9][0-9][0-9][0-9])([^0-9].*)");

            if (disallowedNames.count(name) > 0)
            {
                //
                //  Find a 6-zero-padded number in the middle.
                //
                Match midMatch(midRE, name);

                if (midMatch)
                {
                    string finalName;
                    istringstream istr(midMatch.subStr(1));
                    int n;
                    istr >> n;
                    do
                    {
                        ++n;
                        sprintf(buf, PAD, size_t(n));
                        ostringstream str;
                        str << midMatch.subStr(0) << buf << midMatch.subStr(2);
                        finalName = str.str();
                    } while (disallowedNames.count(finalName) > 0);

                    return finalName;
                }

                //
                //  Find a possibly zero-padded number on the end.
                //

                Match endMatch(endRE, name);

                if (endMatch)
                {
                    string finalName;
                    istringstream istr(endMatch.subStr(1));
                    int n;
                    istr >> n;
                    do
                    {
                        ++n;
                        sprintf(buf, PAD, size_t(n));
                        ostringstream str;
                        str << endMatch.subStr(0) << buf;
                        finalName = str.str();
                    } while (disallowedNames.count(finalName) > 0);

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

        void uniqifyContainerNames(GTOReader::Containers& containers,
                                   Session::NameSet& existingNames,
                                   Session::NameSet& ignoreProtocols,
                                   Session::NameSet& topLevelSet)
        {
            Session::NameSet origNames;
            Session::NameSet conflictingNames;
            Session::NameMap newNameMap;

            //
            //  run through containers that correspond to top-level nodes,
            //  identify those that conflict with existingNames
            //

            for (size_t i = 0; i < containers.size(); i++)
            {
                PropertyContainer* pc = containers[i];

                if (ignoreProtocols.count(pc->protocol()) > 0)
                    continue;

                string nm = pc->name();

                if (topLevelSet.count(nm) && existingNames.count(nm))
                {
                    conflictingNames.insert(nm);
                }
                origNames.insert(nm);
            }

            //
            //  for every conflicting top-level name, choose a new name (same
            //  type!) that isn't in either the original list, or the new one.
            //

            Session::NameSet disallowedNames = existingNames;
            disallowedNames.insert(origNames.begin(), origNames.end());

            for (Session::NameSet::iterator i = conflictingNames.begin();
                 i != conflictingNames.end(); ++i)
            {
                string newName = uniqueName(*i, disallowedNames);

                newNameMap[*i] = newName;
                disallowedNames.insert(newName);
            }

            //
            //  run through all containers, add mappings for non-top-level
            //  containers, based on the mapping for the corresponding top-level
            //  container(s).
            //

            for (size_t i = 0; i < containers.size(); i++)
            {
                PropertyContainer* pc = containers[i];

                if (ignoreProtocols.count(protocol(pc)) > 0)
                    continue;

                string nm = pc->name();

                int gp = nm.find('_');

                if (gp != string::npos)
                {
                    string newName;

                    string groupName = nm.substr(0, gp);
                    if (newNameMap.count(groupName))
                        groupName = newNameMap[groupName];

                    int ip = nm.rfind('_');
                    string inputName = nm.substr(ip + 1);
                    if (newNameMap.count(inputName))
                        inputName = newNameMap[inputName];

                    newName =
                        groupName + nm.substr(gp, ip - gp + 1) + inputName;

                    newNameMap[nm] = newName;
                }
            }

            //  run through all containers, change conflicting container name to
            //  new name, and any string properites that contain conflicting
            //  container name to new name
            //

            for (int i = 0; i < containers.size(); i++)
            {
                PropertyContainer* pc = containers[i];

                string nm = pc->name();

                if (newNameMap.count(nm))
                {
                    pc->setName(newNameMap[nm]);
                }

                PropertyContainer::Components& comps = pc->components();
                for (int j = 0; j < comps.size(); ++j)
                {
                    if (comps[j]->name() == "object")
                        continue;

                    Component::Container& props = comps[j]->properties();
                    for (int k = 0; k < props.size(); ++k)
                    {
                        if (StringProperty* sp =
                                dynamic_cast<StringProperty*>(props[k]))
                        {
                            for (StringProperty::iterator l = sp->begin();
                                 l != sp->end(); ++l)
                            {
                                if (newNameMap.count(*l))
                                {
                                    *l = newNameMap[*l];
                                }
                            }
                        }
                        else if (StringPairProperty* sp =
                                     dynamic_cast<StringPairProperty*>(
                                         props[k]))
                        {
                            for (StringPairProperty::iterator l = sp->begin();
                                 l != sp->end(); ++l)
                            {
                                if (newNameMap.count(l->first))
                                    l->first = newNameMap[l->first];
                                if (newNameMap.count(l->second))
                                    l->second = newNameMap[l->second];
                            }
                        }
                    }
                }
            }
        }

    } // namespace

    void Session::unpackSessionContainer(PropertyContainer* pc)
    {
        if (!pc)
            return;
        StringProperty* vnp = pc->property<StringProperty>("session.viewNode");
        IntProperty* mp = pc->property<IntProperty>("session.marks");
        IntProperty* tp = pc->property<IntProperty>("session.type");
        Vec2iProperty* rp = pc->property<Vec2iProperty>("session.range");
        Vec2iProperty* iop = pc->property<Vec2iProperty>("session.region");
        FloatProperty* fpsp = pc->property<FloatProperty>("session.fps");
        IntProperty* rtp = pc->property<IntProperty>("session.realtime");
        IntProperty* incp = pc->property<IntProperty>("session.inc");
        IntProperty* fp = pc->property<IntProperty>("session.currentFrame");
        IntProperty* bg = pc->property<IntProperty>("session.background");

        for (int i = 0; mp && i < mp->size(); i++)
        {
            markFrame((*mp)[i]);
        }

        if (fpsp && !fpsp->empty())
            setFPS(fpsp->front());
        if (rtp && !rtp->empty())
            setRealtime(rtp->front() ? true : false);
        if (incp && !incp->empty())
            setInc(incp->front());
        if (rp && !rp->empty())
            setRangeStart(rp->front().x);
        if (rp && !rp->empty())
            setRangeEnd(rp->front().y);
        if (iop && !iop->empty())
            setInPoint(iop->front().x);
        if (iop && !iop->empty())
            setOutPoint(iop->front().y);

        if (vnp && !vnp->empty())
        {
            setViewNode(vnp->front(), true);
        }

        if (bg && !bg->empty())
        {
            m_renderer->setBGPattern((ImageRenderer::BGPattern)bg->front());
        }
    }

    void Session::addNodeCreationContext(PropertyContainer* pc, int value)
    {
        if (!m_notPersistent)
        {
            m_notPersistent = new PropertyInfo(PropertyInfo::NotPersistent
                                               | PropertyInfo::OutputOnly);
            //
            //  We intend for this PropInfo to never be deleted, so be sure that
            //  we ref() it once here.
            //
            m_notPersistent->ref();
        }

        pc->declareProperty<IntProperty>("internal.creationContext", value,
                                         m_notPersistent);
    }

    void Session::readProfile(const string& infilename, IPNode* node,
                              const ReadRequest& request)
    {
        Profile profile(infilename, &graph());
        profile.load();
        profile.apply(node);
    }

    void Session::read(const string& infilename, const ReadRequest& request)
    {
        //
        //  The base file format, unlike the original RV format, is
        //  literal: only top level nodes that appear in the file are
        //  created. You can't make a file with a partial list of sources,
        //  etc, etc. There are no assumptions about preexisting nodes in
        //  the graph like default sequence, and so on.
        //
        //  Like write() you can use a stream as the source, but you pass
        //  it in via the "stream" option. In that case the filename is
        //  the the stream name (for error reporting).
        //

        const bool merge = request.optionValue<bool>("merge", false);
        const bool doClear = request.optionValue<bool>("clear", false);
        const bool clipboard = request.optionValue<bool>("clipboard", false);
        istream* inStream = request.optionValue<istream*>("stream", NULL);

        typedef std::set<PropertyContainer*> PropertyContainerSet;

        string filename = pathConform(infilename);

        m_beforeSessionReadSignal(filename);

        GTOReader reader;
        GTOReader::Containers containers;

        if (inStream)
        {
            containers = reader.read(*inStream, filename.c_str());
        }
        else
        {
            containers = reader.read(filename.c_str());
        }

        PropertyContainer* session = 0;
        PropertyContainer* connections = 0;

        m_inputFileVersion = 0;
        const int readableVersion = 1;

        Property::Info* notPersistent = new Property::Info();
        notPersistent->setPersistent(false);

        //
        //  Pass #1:
        //
        //  Locate the file version, session object, connection object,
        //  and create any node definitions that came with the file
        //

        for (size_t i = 0; i < containers.size(); i++)
        {
            PropertyContainer* pc = containers[i];
            string p = protocol(pc);

            if (p == "Session")
            {
                session = pc;
                m_inputFileVersion = TwkContainer::protocolVersion(pc);
            }
            else if (p == "IPNodeDefinition")
            {
                pc->declareProperty<StringProperty>("node.origin", infilename,
                                                    notPersistent, true);
                NodeDefinition* def = new NodeDefinition(pc);
                App()->nodeManager()->addDefinition(def);
            }
            else if (p == "connection")
            {
                connections = pc;
            }
        }

        //
        //  Clear existing session
        //

        NameSet ignoreProtocols;

        if (merge)
        {
            NameSet existingNames;
            NameSet topLevelSet;
            makeTopLevelSet(connections, containers, topLevelSet);

            for (IPGraph::NodeMap::const_iterator i =
                     graph().viewableNodes().begin();
                 i != graph().viewableNodes().end(); ++i)
            {
                string n = (*i).second->name();
                if (!graph().isDefaultView(n))
                    existingNames.insert(n);
            }

            uniqifyContainerNames(containers, existingNames, ignoreProtocols,
                                  topLevelSet);
        }
        else if (doClear)
        {
            clear();
        }

        if (clipboard
            && (!session
                || session->propertyValue<IntProperty>("session.clipboard", 0)
                       != 1))
        {
            //
            //  This was intended to be a clipboard read but the GTO
            //  stream is not marked as such
            //

            TWK_THROW_EXC_STREAM(
                "ERROR: clipboard does not contain clipboard GTO format");
        }

        switch (m_inputFileVersion)
        {
        case 0:
            cout << "WARNING: no file version: assuming version 1 or 2 file "
                    "format"
                 << endl;
            m_inputFileVersion = 1;
            break;
        case 1:
            cout << "INFO: reading version " << m_inputFileVersion
                 << " session file format" << endl;
            break;
        default:
            cout << "WARNING: trying to read version " << m_inputFileVersion
                 << " session file format";

            if (m_inputFileVersion > readableVersion)
            {
                cout << " which is newer than the current version: "
                     << readableVersion;
            }

            cout << endl;
            break;
        }

        //
        //  Pass #2
        //
        //  Create top level nodes
        //

        NameSet topLevelSet;
        makeTopLevelSet(connections, containers, topLevelSet);

        for (int i = 0; i < containers.size(); i++)
        {
            PropertyContainer* pc = containers[i];
            string fprotocol = protocol(pc);
            unsigned int fprotocolVersion = protocolVersion(pc);

            if (fprotocol == "Session" || fprotocol == "connection")
                continue;

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
                             << fprotocol << " for node " << nodeName
                             << " in session file: ignoring" << endl;
                    }
                }
            }
        }

        //
        //  Pass #3
        //
        //  connect and copy state into top level nodes
        //

        PropertyContainerSet unusedContainers;
        PropertyContainerSet usedContainers;
        vector<pair<string, int>> readCompletedNodes;

        for (int i = 0; i < containers.size(); i++)
        {
            PropertyContainer* pc = containers[i];
            string fprotocol = protocol(pc);
            unsigned int fprotocolVersion = protocolVersion(pc);
            string nodeName = TwkContainer::name(pc);

            if (pc == session || pc == connections)
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
                    //  automatically.  But this may not be what the user saved
                    //  in the session file, since we allow editing of the
                    //  default views.  So in the usual case, set the inputs for
                    //  the default nodes like other nodes, from whatever is
                    //  specified in the connections list.
                    //
                    //  But two special cases:  If we're merging there's not
                    //  really anything sensible to be done and if there are no
                    //  connections for the default view (IE it's a stub session
                    //  file), so in those cases allow default views to build
                    //  themselves.
                    //

                    if (!graph().isDefaultView(n->name())
                        || (!merge && inputs.size()))
                        n->setInputs(inputs);

                    try
                    {
                        if (n->protocol() == fprotocol)
                        {
                            n->copy(pc);
                            readCompletedNodes.push_back(
                                make_pair(n->name(), fprotocolVersion));
                        }
                    }
                    catch (std::exception& e)
                    {
                        cout << "ERROR: in file, node "
                             << TwkContainer::name(pc) << ": " << endl
                             << e.what();
                    }
                }
                else
                {
                    unusedContainers.insert(pc);
                }
            }
        }

        //
        //  Pass #4:
        //
        //  iterate on all unused property containers until either there
        //  are none left or we can't find nodes that correspond to the
        //  containers.
        //

        while (!unusedContainers.empty())
        {
            usedContainers.clear();

            for (PropertyContainerSet::const_iterator i =
                     unusedContainers.begin();
                 i != unusedContainers.end(); ++i)
            {
                PropertyContainer* pc = *i;
                string fprotocol = protocol(pc);
                unsigned int fprotocolVersion = protocolVersion(pc);
                string nodeName = TwkContainer::name(pc);

                if (IPNode* n = graph().findNode(nodeName))
                {
                    //
                    //  Restore the state from the file. This may overwrite
                    //  any default state created when the connections were
                    //  made.
                    //

                    try
                    {
                        if (n->protocol() == fprotocol)
                        {
                            n->copy(pc);
                            addNodeCreationContext(n);
                            readCompletedNodes.push_back(
                                make_pair(n->name(), fprotocolVersion));
                            usedContainers.insert(pc);
                        }
                    }
                    catch (std::exception& e)
                    {
                        cout << "ERROR: in file, node "
                             << TwkContainer::name(pc) << ": " << endl
                             << e.what();
                    }
                }
            }

            for (PropertyContainerSet::const_iterator i =
                     usedContainers.begin();
                 i != usedContainers.end(); ++i)
            {
                unusedContainers.erase(*i);
            }

            //
            //  Pass 4b: Now that the graph is assembled and all reads are
            //  completed it is safe to trigger readCompleted for each node.
            //
            //  Note this for-loop has to be within the pass4 while() loop as
            //  there are unusedContainers nodes that arent found in graph until
            //  nodes from a previous while-loop iteration have been
            //  readCompleted().
            for (int i = 0; i < readCompletedNodes.size(); i++)
            {
                if (IPNode* n = graph().findNode(readCompletedNodes[i].first))
                {
                    n->readCompleted(n->protocol(),
                                     readCompletedNodes[i].second);
                }
            }
            readCompletedNodes.clear();

            if (usedContainers.empty())
                break;
        }

        //
        //  Pass 5: Safe to trigger readCompleted for each node not already done
        //  in
        //          pass4b
        //
        for (int i = 0; i < readCompletedNodes.size(); i++)
        {
            if (IPNode* n = graph().findNode(readCompletedNodes[i].first))
            {
                n->readCompleted(n->protocol(), readCompletedNodes[i].second);
            }
        }

        //
        //  Set session state
        //

        if (!merge)
        {
            setFileName(filename);
            unpackSessionContainer(session);
        }

        //
        //  Clean up
        //
        //  Delete the containers we got from the file
        //

        for (int i = 0; i < containers.size(); i++)
        {
            delete containers[i];
        }

        m_readingGTO = false;

        //
        //  Set session state from container
        //

        m_afterSessionReadSignal(filename);
    }

    void Session::write(const string& filename, const WriteRequest& request)
    {
        const bool debug = request.optionValue<bool>("debug", false);
        const bool saveAs = request.optionValue<bool>("saveAs", false);
        const bool clipboard = request.optionValue<bool>("clipboard", false);
        const string sessionP =
            request.optionValue<string>("sessionProtocol", "Session");
        const int sessionPV =
            request.optionValue<int>("sessionProtocolVersion", 1);
        const string sessionName =
            request.optionValue<string>("sessionName", "session");
        const int version = request.optionValue<int>("version", 1);

        m_beforeSessionWriteSignal(filename);

        //
        //  May call various other flavors of writing depending on the
        //  extension or request options. But for now we only write out an
        //  IP level session file
        //

        WriteObjectVector headerObjects;

        copySessionStateToNode(graph().viewNode());
        SessionIPNode* sessionNode = graph().sessionNode();
        PropertyContainer pc;

        pc.copy(sessionNode);
        if (!debug)
            pc.remove(
                pc.component("opengl")); // may want to keep this if debugging
        copyFullSessionStateToContainer(pc);
        pc.declareProperty<IntProperty>("session.version", version);
        if (clipboard)
            pc.declareProperty<IntProperty>("session.clipboard", 1);
        headerObjects.push_back(
            WriteObject(&pc, sessionName, sessionP, sessionPV));

        //
        //  Top level connections
        //

        PropertyContainer cons;
        copyConnectionsToContainer(cons);
        headerObjects.push_back(
            WriteObject(&cons, "connections", "connection", 2));

        string conformedPath = pathConform(filename);

        writeGTO(conformedPath, headerObjects, request, true);

        if (!saveAs && filename != "")
            setFileName(conformedPath);

        m_afterSessionWriteSignal(filename);
    }

    namespace
    {

        class SessionGTOWriter : public TwkContainer::GTOWriter
        {
        public:
            SessionGTOWriter(bool isProfile, const char* stamp = 0)
                : GTOWriter(stamp)
                , m_profile(isProfile)
            {
            }

            virtual ~SessionGTOWriter() {}

            virtual bool shouldWriteProperty(const Property* property) const
            {
                if (const Property::Info* info = property->info())
                {
                    if (info->isPersistent())
                    {
                        if (m_profile)
                        {
                            if (const IPCore::PropertyInfo* cinfo =
                                    dynamic_cast<const IPCore::PropertyInfo*>(
                                        info))
                            {
                                return !cinfo->excludedFromProfile();
                            }
                        }

                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }

                return true;
            }

        private:
            bool m_profile;
        };

    } // namespace

    void Session::writeGTO(const string& filename,
                           const WriteObjectVector& headerObjects,
                           const WriteRequest& request, const bool writeSession)
    {
        //
        //  Base options for file writing.
        //

        const bool compressed = request.optionValue<bool>("compressed", false);
        const bool sparse = request.optionValue<bool>("sparse", false);
        const bool binary =
            request.optionValue<bool>("binary", false) || compressed;
        const bool debug = request.optionValue<bool>("debug", false);
        const bool connections =
            request.optionValue<bool>("connections", false);
        const bool membership = request.optionValue<bool>("membership", false);
        const bool recursive = request.optionValue<bool>("recursive", true);
        const bool clipboard = request.optionValue<bool>("clipboard", true);
        const bool profile = request.optionValue<bool>("profile", false);
        const string writerID = request.optionValue<string>("writerID", "IP");
        const StringPair nameReplace =
            request.optionValue<StringPair>("nameReplace", StringPair());
        ostream* outStream = request.optionValue<ostream*>("stream", NULL);
        NodeSet writeNodes = request.optionValue<NodeSet>("writeNodes");

        //
        //  Initialize the writeNodes
        //

        const IPGraph::NodeMap& nodeMap = graph().nodeMap();

        if (writeNodes.empty())
        {
            for (NodeMap::const_iterator i = nodeMap.begin();
                 i != nodeMap.end(); ++i)
            {
                writeNodes.insert(i->second);
            }
        }
        else if (recursive)
        {
            NodeSet tempSet = writeNodes;

            for (NodeSet::const_iterator i = tempSet.begin();
                 i != tempSet.end(); ++i)
            {
                (*i)->collectMemberNodes(writeNodes);
            }
        }

        GTOWriter::ObjectVector objects;
        SessionGTOWriter writer(profile, writerID.c_str());

        //
        //  Copy over the header objects to the write objects
        //

        for (size_t i = 0; i < headerObjects.size(); i++)
        {
            const WriteObject& obj = headerObjects[i];

            objects.push_back(GTOWriter::Object(obj.container, obj.name,
                                                obj.protocol, obj.version));
        }

        //
        //  Sort nodes alphabetically by name before output, so that Session
        //  file structure is "stable".
        //

        //  XXX why does this not result in a sorted/complete set of the nodes
        //  in writeNodes ? SortedNodeSet sortedWriteNodes =
        //  SortedNodeSet(writeNodes.begin(), writeNodes.end());

        SortedNodeSet sortedWriteNodes;

        for (NodeSet::const_iterator i = writeNodes.begin();
             i != writeNodes.end(); ++i)
        {
            sortedWriteNodes.insert(*i);
        }

        //
        //  All nodes
        //

        set<const NodeDefinition*> outputDefinitions;
        vector<PropertyContainer*> tempContainers;

        for (SortedNodeSet::const_iterator i = sortedWriteNodes.begin();
             i != sortedWriteNodes.end(); ++i)
        {
            IPNode* n = *i;
            string ntype = n->protocol();

            if (writeSession && !n->isWritable())
                continue;
            n->prepareForWrite();

            //
            //  If the node is a group and we may need to add membership
            //  and/or member connection information to it.
            //

            if (GroupIPNode* group = dynamic_cast<GroupIPNode*>(n))
            {
                if (membership)
                {
                    StringProperty* sp = n->declareProperty<StringProperty>(
                        "membership.contains");
                    const NodeSet& members = group->members();

                    for (NodeSet::const_iterator i = members.begin();
                         i != members.end(); ++i)
                    {
                        string name = (*i)->name();
                        sp->push_back(mungeName(nameReplace, name));
                    }
                }

                if (connections)
                {
                    string rootName =
                        mungeName(nameReplace, group->rootNode()->name());

                    StringPairProperty* sp =
                        n->declareProperty<StringPairProperty>(
                            "evaluation.connections");
                    StringProperty* rp = n->declareProperty<StringProperty>(
                        "evaluation.root", rootName);
                    const NodeSet& members = group->members();

                    for (NodeSet::const_iterator i = members.begin();
                         i != members.end(); ++i)
                    {
                        IPNode* member = *i;
                        const IPNode::IPNodes& inputs = member->inputs();

                        for (size_t q = 0; q < inputs.size(); q++)
                        {
                            //
                            //  NOTE: the connection is the data flow
                            //  direction
                            //

                            string iname = inputs[q]->name();
                            sp->push_back(StringPair(
                                mungeName(nameReplace, iname),
                                mungeName(nameReplace, member->name())));
                        }
                    }
                }
            }

            //
            //
            //

            GTOWriter::Object obj(n, mungeName(nameReplace, n->name()),
                                  n->protocol(), n->protocolVersion());

            if (sparse)
            {
                if (PropertyContainer* sc = graph().sparseContainer(n))
                {
                    if (!n->group() && // top level
                        sc->components().size() == 1
                        && sc->components().front()->properties().size() == 3)
                    {
                        //
                        //  In this case, the object doesn't really
                        //  contain any useful information, but because
                        //  the text GTO writer decides not to write empty
                        //  objects, it disappears. So just in case we
                        //  care about the existence of top level objects
                        //  lets just write the whole thing out. Currently
                        //  I don't think this can happen (because of the
                        //  ui name), but just in case.
                        //

                        tempContainers.push_back(sc);
                        objects.push_back(obj);
                    }
                    else
                    {
                        tempContainers.push_back(sc);
                        obj.container = sc;
                        objects.push_back(obj);
                    }
                }
                else
                {
                    objects.push_back(obj);
                }
            }
            else
            {
                objects.push_back(obj);
            }

            //
            //  Add inlined definitions if the original comes from the
            //  current file
            //

            if (IPInstanceNode* inode = dynamic_cast<IPInstanceNode*>(n))
            {
                const NodeDefinition* definition = inode->definition();

                if (definition->stringValue("node.origin") == fileName())
                {
                    if (outputDefinitions.count(definition) == 0)
                    {
                        PropertyContainer* tempPC = definition->copy();
                        tempContainers.push_back(tempPC);
                        objects.push_back(GTOWriter::Object(tempPC));
                        outputDefinitions.insert(definition);
                    }
                }
            }
        }

        Gto::Writer::FileType outType = Gto::Writer::TextGTO;
        if (compressed)
            outType = Gto::Writer::CompressedGTO;
        else if (binary)
            outType = Gto::Writer::BinaryGTO;

        bool ok = outStream ? writer.write(*outStream, objects, outType)
                            : writer.write(filename.c_str(), objects, outType);

        for (SortedNodeSet::const_iterator i = sortedWriteNodes.begin();
             i != sortedWriteNodes.end(); ++i)
        {
            IPNode* n = *i;

            if (GroupIPNode* group = dynamic_cast<GroupIPNode*>(n))
            {
                if (membership)
                {
                    n->removeProperty<StringProperty>("membership.contains");
                }
                if (connections)
                {
                    n->removeProperty<StringPairProperty>(
                        "evaluation.connections");
                    n->removeProperty<StringProperty>("evaluation.root");
                }
            }

            //  Note: this condition must match the above prepareForWrite
            //  condition exactly.
            //
            if (!writeSession || n->isWritable())
                n->writeCompleted();
        }

        for (size_t i = 0; i < tempContainers.size(); i++)
            delete tempContainers[i];

        if (!ok)
            TWK_THROW_EXC_STREAM("Failed to write " << filename);
    }

    void Session::readGTO(const string& filename, bool merge) {}

    bool Session::useThreadedUpload()
    {
        return ImageRenderer::useThreadedUpload();
    }

    bool Session::framePatternCheck(int frame) const
    {
        m_frameHistory.push_front(frame);
        m_frameRuns.clear();
        m_framePatternFail = false;

        const bool newFrame =
            m_frameHistory.size() > 1 && m_frameHistory[0] != m_frameHistory[1];

        for (size_t i = 0; i < m_frameHistory.size(); i++)
        {
            if (!i)
            {
                m_frameRuns.push_back(1);
            }
            else if (m_frameHistory[i] != m_frameHistory[i - 1])
            {
                if (m_frameRuns.size() > 10)
                    break;
                m_frameRuns.push_back(1);
            }
            else
            {
                m_frameRuns.back()++;
            }
        }

        if (newFrame && m_frameRuns.size() >= 3)
        {
            const float vsyncFPSRatio = outputDeviceHz() / currentTargetFPS();

            //
            // If the perroll is sufficiently large to last one frame then
            // we force the render cycles of the first frame to be the desired
            // count required for a successfull pattern check since there is
            // more that sufficient time.
            //
            bool isFirstPreRollPlayFrame =
                (m_frameRuns.size() == 3
                 && (audioRenderer() && hasAudio()
                     && (audioRenderer()->preRollDelay()
                             > 1.0 / currentTargetFPS()
                         || audioRenderer()->deviceState().latency
                                < -1.0 / currentTargetFPS())));
            if (isFirstPreRollPlayFrame)
            {
                if (fabsf(vsyncFPSRatio - 2.5f) < 0.1f)
                {
                    // Set the first frame to 3 render cycles for 3-2
                    m_frameRuns[2] = 3;
                }
                else if (fabsf(vsyncFPSRatio - 2.0f) < 0.1f)
                {
                    // Set the first frame to 2 render cycles for 2-2
                    m_frameRuns[2] = 2;
                }
            }

            bool patternFailed = false;

            // Check for 3-2 render cycle i.e. 24fps at 60hz
            if (fabsf(vsyncFPSRatio - 2.5f) < 0.1f
                && (!(m_frameRuns[1] == 2 && m_frameRuns[2] == 3)
                    && !(m_frameRuns[1] == 3 && m_frameRuns[2] == 2)))
            {
                if (!m_framePatternFailCount)
                {
                    cout << "PATTERN check using a 3-2 render/refresh cycle."
                         << endl;
                }

                patternFailed = true;
            }
            else if (fabsf(vsyncFPSRatio - 2.0f) < 0.1f
                     && (m_frameRuns[1] != 2 || m_frameRuns[2] != 2))
            {
                // Check for 2-2 render cycle i.e. 24fps at 48hz or
                // 30fps at 60 Hz.
                if (!m_framePatternFailCount)
                {
                    cout << "PATTERN check using a 2-2 render/refresh cycle."
                         << endl;
                }

                patternFailed = true;
            }
            else if (vsyncFPSRatio < 1.5f
                     && (m_frameRuns[1] != 1 || m_frameRuns[2] != 1))
            {
                // Check for FPS = Vsync Rate pattern
                if (!m_framePatternFailCount)
                {
                    cout << "PATTERN check using a 1-1 render/refresh cycle."
                         << endl;
                }

                patternFailed = true;
            }

            if (patternFailed)
            {
                m_framePatternFail = true;
                ++m_framePatternFailCount;
                cout << "PATTERN FAIL #" << m_framePatternFailCount << " : ";
                for (size_t i = 1; i < m_frameRuns.size(); i++)
                    cout << " " << m_frameRuns[i];
                cout << " // frame = " << frame << " (" << currentFrame() << ")"
                     << endl;
            }
        }

        return m_framePatternFail;
    }

} // namespace IPCore
