//******************************************************************************
// Copyright (c) 2005 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPCore__Session__h__
#define __IPCore__Session__h__
#include <TwkApp/VideoDevice.h>
#include <IPCore/AudioRenderer.h>
#include <IPCore/IPGraph.h>
#include <IPCore/IPImage.h>
#include <IPCore/IPNode.h>
#include <TwkApp/Document.h>
#include <TwkContainer/PropertyContainer.h>
#include <TwkMath/Time.h>
#include <TwkUtil/Timer.h>
#include <map>
#include <limits>
#include <deque>
#include <boost/signals2.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>

namespace IPCore
{
    class AudioRenderer;
    class IPNode;
    class ImageRenderer;
    class PropertyInfo;

    //
    //  class Session
    //
    //  This object represents the "document" in as much as Rv has a
    //  "document". The Session can be a single image, a movie, an edit, a
    //  composite, etc, etc.
    //

    class Session : public TwkApp::Document
    {
    public:
        //
        //  Types
        //

        typedef std::vector<std::string> StringVector;
        typedef std::map<std::string, std::string> StringMap;
        typedef std::vector<int> Frames;
        typedef TwkUtil::Timer Timer;
        typedef TwkMath::Time Time;
        typedef TwkMath::Vec4f Pixel;
        typedef std::map<std::string, IPNode*> NodeMap;
        typedef TwkContainer::Property Property;
        typedef TwkContainer::PropertyContainer PropertyContainer;
        typedef std::vector<PropertyContainer*> PropertyContainerVector;
        typedef stl_ext::thread_group ThreadGroup;
        typedef std::vector<Property*> PropertyVector;
        typedef std::vector<IPNode*> NodeVector;
        typedef TwkMath::Vec2i Vec2i;
        typedef FBCache::FrameRangeVector CachedRanges;
        typedef std::deque<Time> SyncTimes;
        typedef TwkApp::VideoDevice VideoDevice;
        typedef std::set<const VideoDevice*> VideoDevices;
        typedef TwkApp::VideoDevice::Margins Margins;
        typedef IPNode::IPNodeSet NodeSet;
        typedef IPNode::SortedIPNodeSet SortedNodeSet;
        typedef std::set<std::string> NameSet;
        typedef std::map<std::string, std::string> NameMap;
        typedef boost::mutex Mutex;
        typedef boost::mutex::scoped_lock ScopedLock;
        typedef boost::condition_variable Condition;
        typedef std::deque<int> IntDeque;
        typedef std::vector<int> IntVector;

        typedef std::pair<TwkAudio::SampleTime, TwkAudio::SampleTime>
            AudioRange;

        struct WriteObject
        {
            WriteObject(PropertyContainer* p, const std::string& n,
                        const std::string& pr, unsigned int v)
                : container(p)
                , name(n)
                , protocol(pr)
                , version(v)
            {
            }

            PropertyContainer* container;
            std::string name;
            std::string protocol;
            unsigned int version;
        };

        typedef std::vector<WriteObject> WriteObjectVector;

        // deprecated needed for reading older files
        enum SessionType
        {
            SequenceSession,
            StackSession
        };

        enum CachingMode
        {
            NeverCache,
            BufferCache,
            GreedyCache
        };

        enum PlayMode
        {
            PlayLoop = 0,
            PlayOnce = 1,
            PlayPingPong = 2
        };

        struct CacheStats : public FBCache::CacheStats
        {
            Time audioSecondsCached;

            void operator=(const FBCache::CacheStats& s)
            {
                capacity = s.capacity;
                used = s.used;
                lookAheadSeconds = s.lookAheadSeconds;
                cachedRanges = s.cachedRanges;
            }
        };

        struct FBStatus
        {
            TwkFB::FrameBuffer* fb;
            bool partial;
            bool loading;
            bool error;
            bool warning;
            bool noImage;
        };

        enum CurrentFrameStatus
        {
            OkStatus = 0,
            ErrorStatus = 1 << 0,
            NoImageStatus = 1 << 1,
            WarningStatus = 1 << 2,
            PartialStatus = 1 << 3,
            LoadingStatus = 1 << 4
        };

        typedef std::vector<FBStatus> FBStatusVector;

        typedef boost::signals2::signal<void()> VoidSignal;
        typedef boost::signals2::signal<void(int)> FrameSignal;
        typedef boost::signals2::signal<void(double)> TimeSignal;
        typedef boost::signals2::signal<void(int, int)> RangeSignal;
        typedef boost::signals2::signal<void(IPNode*)> NodeSignal;
        typedef boost::signals2::signal<void(const std::string&)> StringSignal;
        typedef boost::signals2::signal<void(int, const std::string&)>
            FrameAndStringSignal;
        typedef boost::signals2::signal<void(const VideoDevice*)>
            VideoDeviceSignal;

        //
        //  SIGNALS
        //

        FrameAndStringSignal& frameChangedSignal()
        {
            return m_frameChangedSignal;
        }

        VoidSignal& updateSignal() { return m_updateSignal; }

        VoidSignal& updateLoadingSignal() { return m_updateLoadingSignal; }

        VoidSignal& forcedUpdateSignal() { return m_forcedUpdateSignal; }

        VoidSignal& fullScreenOnSignal() { return m_fullScreenOnSignal; }

        VoidSignal& fullScreenOffSignal() { return m_fullScreenOffSignal; }

        VoidSignal& sessionChangedSignal() { return m_sessionChangedSignal; }

        VoidSignal& stereoHardwareOnSignal()
        {
            return m_stereoHardwareOnSignal;
        }

        VoidSignal& stereoHardwareOffSignal()
        {
            return m_stereoHardwareOffSignal;
        }

        VoidSignal& audioUnavailableSignal()
        {
            return m_audioUnavailableSignal;
        }

        VoidSignal& eventDeviceChangedSignal()
        {
            return m_eventDeviceChangedSignal;
        }

        VoidSignal& glQueryCompleteSignal() { return m_glQueryCompleteSignal; }

        FrameSignal& inPointChangedSignal() { return m_inPointChangedSignal; }

        FrameSignal& outPointChangedSignal() { return m_outPointChangedSignal; }

        RangeSignal& rangeChangedSignal() { return m_rangeChangedSignal; }

        RangeSignal& rangeNarrowedSignal() { return m_rangeNarrowedSignal; }

        VoidSignal& beforeSessionClearSignal()
        {
            return m_beforeSessionClearSignal;
        }

        VoidSignal& afterSessionClearSignal()
        {
            return m_afterSessionClearSignal;
        }

        VoidSignal& realtimeModeChangedSignal()
        {
            return m_realtimeModeChangedSignal;
        }

        VoidSignal& playModeChangedSignal() { return m_playModeChangedSignal; }

        StringSignal& beforePlayStartSignal()
        {
            return m_beforePlayStartSignal;
        }

        StringSignal& playStartSignal() { return m_playStartSignal; }

        StringSignal& playStopSignal() { return m_playStopSignal; }

        FrameSignal& markFrameSignal() { return m_markFrameSignal; }

        FrameSignal& unmarkFrameSignal() { return m_unmarkFrameSignal; }

        TimeSignal& fpsChangedSignal() { return m_fpsChangedSignal; }

        StringSignal& cacheModeChangedSignal()
        {
            return m_cacheModeChangedSignal;
        }

        VoidSignal& bgChangedSignal() { return m_bgChangedSignal; }

        VideoDeviceSignal& outputVideoDeviceChangedSignal()
        {
            return m_outputVideoDeviceChangedSignal;
        }

        VideoDeviceSignal& physicalVideoDeviceChangedSignal()
        {
            return m_physicalVideoDeviceChangedSignal;
        }

        NodeSignal& beforeGraphViewChangeSignal()
        {
            return m_beforeGraphViewChangeSignal;
        }

        NodeSignal& afterGraphViewChangeSignal()
        {
            return m_afterGraphViewChangeSignal;
        }

        VoidSignal& playIncChangedSignal() { return m_playIncChangedSignal; }

        VideoDeviceSignal& marginsChangedSignal()
        {
            return m_marginsChangedSignal;
        }

        StringSignal& beforeSessionReadSignal()
        {
            return m_beforeSessionReadSignal;
        }

        StringSignal& afterSessionReadSignal()
        {
            return m_afterSessionReadSignal;
        }

        StringSignal& beforeSessionWriteSignal()
        {
            return m_beforeSessionWriteSignal;
        }

        StringSignal& afterSessionWriteSignal()
        {
            return m_afterSessionWriteSignal;
        }

        //
        //  Messages (deprecated, use signals)
        //

        NOTIFIER_MESSAGE(startPlayMessage);
        NOTIFIER_MESSAGE(stopPlayMessage);
        NOTIFIER_MESSAGE(frameChangedMessage);
        NOTIFIER_MESSAGE(updateMessage);
        NOTIFIER_MESSAGE(updateLoadingMessage);
        NOTIFIER_MESSAGE(forcedUpdateMessage);
        NOTIFIER_MESSAGE(fullScreenOnMessage);
        NOTIFIER_MESSAGE(fullScreenOffMessage);
        NOTIFIER_MESSAGE(sessionChangedMessage);
        NOTIFIER_MESSAGE(stereoHardwareOnMessage);
        NOTIFIER_MESSAGE(stereoHardwareOffMessage);
        NOTIFIER_MESSAGE(audioUnavailbleMessage);
        NOTIFIER_MESSAGE(eventDeviceChangedMessage);

        //
        //  Constructors
        //

        Session(IPGraph* graph = 0);
        virtual ~Session();

        //
        // Return the RV_AVPLAYBACK_VERSION value.
        // This might needed to be accessed from the
        // audio rendererers.
        //
        int avPlaybackVersion() const { return m_avPlaybackVersion; }

        //
        //  Break connections to outputVideoDevice and controlVideoDevice.
        //  This call is only to be called on cleanup i.e.
        //  called by the destructor of Session or maybe be called
        //  on exit from main() prior to calling OutputVideoModule::close().
        //  See iprender's main.cpp.
        //
        void breakVideoDeviceConnections();

        //
        //  Set the name of this session. This is not the same as the file
        //  name. This is used primarily by networking apps to identify a
        //  particular session running in RV. The default name is just a
        //  session number.
        //

        void setName(const std::string& name);
        std::string name() const; // overrides Notifier until we get rid of it

        //
        //  Document I/O API
        //
        //  write tags the base format recognizes:
        //
        //      binary                  - GTO binary output (defaults to false)
        //      compressed              - use gzipped binary output
        //      sparse                  - only non-default values are written
        //      debug                   - include debug info in file
        //      connections             - include all internal input connections
        //      membership              - include membership connections
        //      recursive               - recursively write out members
        //      writeNodes              - NodeSet constraining writing
        //      clipboard               - format for clipboard (read/write)
        //
        //      writeID                 - name of GTOWriter
        //      sessionProtocol         - session object protocol name
        //      sessionProtocolVersion  - session object protocol version
        //      sessionName             - name of session object in file
        //      version                 - version number in session object
        //

        virtual void read(const std::string& filename, const ReadRequest&);
        virtual void write(const std::string& filename, const WriteRequest&);

        //
        //  Profiles. NOTE: reading a profile causes it to be applied.
        //

        virtual void writeProfile(const std::string& filename, IPNode* node,
                                  const WriteRequest&);

        virtual void readProfile(const std::string& filename, IPNode* node,
                                 const WriteRequest&);

        //
        //  Batch mode means we're running without events
        //

        void setBatchMode(bool) { m_batchMode = true; }

        bool batchMode() const { return m_batchMode; }

        //
        //  Become current
        //

        virtual void makeActive();

        //
        //  Whether or not this session should try and infer sequences
        //  from filenames
        //

        void setInferSequences(bool b) { m_noSequences = !b; }

        bool inferSequences() { return !m_noSequences; }

        //
        //  Renderer type
        //

        virtual void setRendererType(const std::string&);

        ImageRenderer* renderer() const { return m_renderer; }

        void setRendererBGType(unsigned int);

        void setMargins(float left, float right, float top, float bottom,
                        bool allDevices = false);

        //
        //  Video devices
        //

        const VideoDevice* controlVideoDevice() const
        {
            return m_controlVideoDevice;
        }

        const VideoDevice* outputVideoDevice() const
        {
            return m_outputVideoDevice;
        }

        const VideoDevice* eventVideoDevice() const
        {
            return m_eventVideoDevice;
        }

        void setControlVideoDevice(const VideoDevice*);
        void setOutputVideoDevice(const VideoDevice*);
        void setEventVideoDevice(const VideoDevice*);

        void deviceSizeChanged(const VideoDevice*);

        bool multipleVideoDevices() const
        {
            return m_outputVideoDevice
                   && m_controlVideoDevice != m_outputVideoDevice;
        }

        void clearVideoDeviceCaches();

        //
        //  Looks in application option map for debug options and sets the
        //  global state for those
        //

        void setDebugOptions();

        //
        //  Deletes the contents of the document.
        //

        virtual void clear();
        bool isEmpty() const;

        //
        //  Delete a node. This can be a source (group) node or whatever
        //

        virtual void deleteNode(IPNode*);

        //
        //  Evaluation at current running time
        //

        void evaluateForDisplay();
        void maybeDispatchCacheThread();

        bool isEvalRunning() const { return false; }

        //
        //  This form of evaluate returns the current image. This is the only
        //  eternal API to evaluation. Its used by rvio or other off line
        //  programs.
        //

        IPImage* evaluate(int frame);

        //
        //  checkIn() must be called to release the return value of evaluate()
        //

        void checkIn(IPImage*);

        //
        //  Sync info, called by UI
        //

        void addSyncSample();
        Time predictedTimeUntilSync() const;

        //
        //  Render Entry point(s)
        //

        // update the video on its view
        // param minElapsedTime minimum elapsed time since last drawing
        // param force force to refresh the display if the isUpdating is false
        virtual void update(double minElapsedTime = 0.0, bool force = false);

        virtual void render();

        //
        //  Use these two separately when advancing the frame and rendering
        //  need to be separated. render() just calls advanceState() followed by
        //  renderState() and recordRenderState()
        //

        void advanceState();
        void renderState();
        void recordRenderState();

        void userRender(const VideoDevice*, const char* event,
                        const std::string& contents = "");

        //
        //  Post render calls IPGraph::postEvaluation.
        //
        void postRender();

        void prefetch();
        void preEval();

        //
        //  Generic event production. Will return the the event return
        //  content if there is any.
        //

        std::string userGenericEvent(const std::string& eventName,
                                     const std::string& contents,
                                     const std::string& senderName = "");

        //
        //  Similar to userGenericEvent but has render information (like
        //  the domain) as well.
        //

        void userRenderEvent(const std::string& eventName,
                             const std::string& contents = "");

        //
        //  Used to send raw data. Will return the the event return
        //  content if there is any.
        //

        std::string userRawDataEvent(const std::string& eventName,
                                     const std::string& contentType,
                                     const char* data, size_t size,
                                     const char* utf8 = 0,
                                     const std::string& senderName = "");

        //
        //  Send a PixelBlockTransferEvent. The interp string is parsed to
        //  produce the event fields.
        //

        void pixelBlockEvent(const std::string& eventName,
                             const std::string& interp, const char* data,
                             size_t dataSize);

        //
        //  Evaluation
        //

        void forceNextEvaluation();

        //
        //  Playing
        //

        void play(std::string eventData = "");
        void stop(std::string eventData = "");
        void setPlayMode(PlayMode);

        PlayMode playMode() const { return m_playMode; }

        void stopCompletely();
        void restart();

        bool isPlaying() const { return m_timer.isRunning(); }

        bool isUpdating() const
        {
            return isPlaying() || m_stopTimer.isRunning();
        }

        void setFrame(int f);
        void setFrameInternal(int f, std::string eventData = "");

        int currentFrame() const { return m_frame; }

        double realFPS() const { return m_realfps; }

        Vec2i maxSize() const;

        int rangeStart() const;
        int rangeEnd() const;

        void setFrameRange(int start, int end);

        void setRangeStart(int frame) { setFrameRange(frame, rangeEnd()); }

        void setRangeEnd(int frame) { setFrameRange(rangeStart(), frame); }

        void setNarrowedRange(int start, int end);
        int narrowedRangeStart() const;
        int narrowedRangeEnd() const;

        void setRealtime(bool r);

        bool realtime() const { return m_realtime && !m_realtimeOverride; }

        const Timer& timer() const { return m_timer; }

        int shift() const { return m_shift; }

        void setFPS(double f);

        double fps() const { return m_fps; }

        double outputDeviceHz() const;

        void setInPoint(int f);
        int inPoint() const;

        void setOutPoint(int f);
        int outPoint() const;

        void setInc(int);

        int inc() const { return (m_inc == 0) ? 1 : m_inc; }

        bool isRendering() const { return m_rendering; }

        void askForRedraw(bool force = false);

        bool wantsRedraw() const { return m_wantsRedraw; }

        void fullScreenMode(bool);
        bool isFullScreen();

        int skipped() const { return m_skipped; }

        int targetFrame(double elapsed) const;

        void updateRange();

        //
        //  Audio
        //

        AudioRenderer* audioRenderer() const;
        void resetAudio();

        bool hasAudio() const { return graph().hasAudio(); }

        void playAudio();
        void stopAudio(const std::string& eventData = "");
        bool isPlayingAudio();

        //
        //  Helper methods for configuring the graph's audio. Useful for
        //  AudioRenders.
        //
        //  calculateAudioRange - finds the appropriate first and last sample
        //  whether the playback is forwards or in reverse. It needs the audio
        //  rate to make the calculation.
        //

        AudioRange calculateAudioRange(double rate) const;

        //  audioConfigure - sets the graph's audioConfigure state in a
        //  centralized method so that each AudioRender does not have to
        //  calculate the range individually.
        //

        void audioConfigure();

        void audioVarLock();
        void audioVarUnLock();

        double audioLoopDuration() const { return m_audioLoopDuration; }

        void setAudioLoopDuration(double d) { m_audioLoopDuration = d; }

        int audioLoopCount() const { return m_audioLoopCount; }

        void setAudioLoopCount(int c) { m_audioLoopCount = c; }

        double audioTimeShift() const { return m_audioTimeShift; }

        void setAudioTimeShift(double d, double sensitivity = 0.1);

        bool audioTimeShiftValid() const
        {
            return m_audioTimeShift != std::numeric_limits<double>::max();
        }

        void setAudioFirstPass(bool b);

        bool audioFirstPass() const { return m_audioFirstPass; }

        void setAudioInitialSample(size_t s) { m_audioInitialSample = s; }

        size_t audioInitialSample() const { return m_audioInitialSample; }

        Timer& audioPlayTimer() { return m_audioPlayTimer; }

        void accumulateAudioStartSample(size_t s) { m_audioStartSample += s; }

        void setAudioStartSample(size_t s) { m_audioStartSample = s; }

        size_t audioStartSample() const { return m_audioStartSample; }

        void setAudioTime(double t);

        void setAudioTimeSnapped(double t);

        void setAudioSampleShift(double s) { m_audioSampleShift = s; }

        double audioSampleShift() const { return m_audioSampleShift; }

        void accumulateAudioElapsedTime(double t) { m_audioElapsedTime += t; }

        void setAudioElapsedTime(double t) { m_audioElapsedTime = t; }

        double audioElapsedTime() const { return m_audioElapsedTime; }

        void ensureAudioRenderer();

        void setGlobalAudioOffset(float, bool internal = false);
        void setGlobalSwapEyes(bool);

        void setFilterLiveReviewEvents(bool shouldFilterEvents = false);
        bool filterLiveReviewEvents();

        //
        //  Marks
        //

        void markFrame(int);
        void unmarkFrame(int);
        bool isMarked(int);

        const Frames& markedFrames() const { return m_markedFrames; }

        //  Reload all source images

        void reload(int start, int end);

        //
        //  Sources / Nodes
        //

        IPNode* rootNode() const { return graph().root(); }

        IPNode* node(const std::string&) const;

        const IPGraph& graph() const { return *m_graph; }

        IPGraph& graph() { return *m_graph; }

        //
        //  Arrangement
        //

        void setSessionType(SessionType);

        SessionType getSessionType() const { return m_sessionType; }

        bool setViewNode(const std::string& nodeName, bool force = false);
        std::string viewNodeName() const;

        virtual IPNode* newNode(const std::string& typeName,
                                const std::string& nodeName);

        //
        //  Current IPNode::State
        //

        bool currentStateIsIncomplete() const;
        bool currentStateIsError() const;

        unsigned int currentFrameState() const; // |'d CurrentFrameStatus enum

        //
        //  Get all of the current display images
        //

        IPImage* displayImage() const { return m_displayImage; }

        //
        //  All of the current FBs in order
        //

        double currentTargetFPS() const;

        //
        //  Cache Activation
        //

        void setAudioCaching(CachingMode mode);
        CachingMode audioCachingMode() const;

        void setCaching(CachingMode mode);
        CachingMode cachingMode() const;

        bool isCacheFull() const { return false; }

        bool isCaching() const;

        bool isBuffering() const { return m_bufferWait; }

        void updateGraphInOut(bool flushAudio = true);

        //
        //  Elapsed play time
        //

        Time elapsedPlaySeconds();

        Time elapsedPlaySecondsRaw() const { return m_timer.elapsed(); }

        void setNextVSyncOffset(Time, size_t);
        void waitOnVSync();

        bool waitingOnSync() const { return m_waitingOnSync; }

        void useExternalVSyncTiming(bool b) { m_useExternalVSyncTiming = b; }

        static Session* currentSession() { return m_currentSession; }

        static Session* activeSession()
        {
            return static_cast<Session*>(Document::activeDocument());
        }

        void makeCurrentSession() { m_currentSession = this; }

        //
        //  Access to IP graph
        //
        //  If #Type is passed as the name (like #RVColor) the name is used
        //  as a type name and the node matching that type name is returned.
        //
        //  findProperty() and findPropertyOfType<>() return the *leftmost*
        //  node's property with the specified name if the name is #Type
        //  otherwise the node with the exact name is returned.
        //

        virtual void findProperty(PropertyVector& props,
                                  const std::string& name);

        template <typename T>
        void findPropertyOfType(std::vector<T*>& props,
                                const std::string& name);

        //
        //  Finds all nodes that have a specific type name (like RVSource)
        //  in the current evaluation path.
        //

        virtual void findCurrentNodesByTypeName(NodeVector& nodes,
                                                const std::string& typeName);

        //
        //  Finds all nodes that have a specific type name (like RVSource)
        //

        virtual void findNodesByTypeName(NodeVector& nodes,
                                         const std::string& typeName);

        //
        //  User timer
        //

        Timer& userTimer() { return m_userTimer; }

        const Timer& userTimer() const { return m_userTimer; }

        //
        //  Some hackery necessary for FLTK
        //

        void receivingEvents(bool b) { m_receivingEvents = b; }

        //
        //  audio
        //

        bool isScrubbingAudio() const { return m_scrubAudio; }

        void scrubAudio(bool b, Time duration, int count);

        //
        //  Cache statistics
        //

        const CacheStats& cacheStats();

        //
        //  Cache RAM usage and wait time
        //

        static void setCacheLookBehindFraction(float percent)
        {
            m_cacheLookBehindFraction = percent;
        }

        static void setMaxBufferedWaitTime(float seconds)
        {
            m_maxBufferedWaitSeconds = seconds;
            IPGraph::setMaxBufferedWaitTime(seconds);
        }

        float maxBufferedWaitTime() const { return m_maxBufferedWaitSeconds; };

        static void setMaxGreedyCacheSize(size_t bytes)
        {
            m_maxGreedyCacheSize = bytes;
        }

        static void setMaxBufferCacheSize(size_t bytes)
        {
            m_maxBufferCacheSize = bytes;
        }

        static void setUsePreEval(bool b) { m_usePreEval = b; }

        static bool usingPreEval() { return m_usePreEval; }

        static bool useThreadedUpload();

        std::string nextViewNode();
        std::string previousViewNode();

        //
        //  Playback timing debugging
        //

        struct ProfilingRecord
        {
            ProfilingRecord()
                : renderStart(0)
                , renderEnd(0)
                , swapStart(0)
                , swapEnd(0)
                , evaluateStart(0)
                , evaluateEnd(0)
                , userRenderStart(0)
                , userRenderEnd(0)
                , internalRenderStart(0)
                , internalRenderEnd(0)
                , internalPrefetchStart(0)
                , internalPrefetchEnd(0)
                , frameChangeEventStart(0)
                , frameChangeEventEnd(0)
                , prefetchRenderStart(0)
                , prefetchRenderEnd()
                , prefetchUploadPlaneTotal(0)
                , renderUploadPlaneTotal(0)
                , renderFenceWaitTotal(0)
                , expectedSyncTime(0)
                , deviceClockOffset(0)
                , gccount(0)
                , frame(-std::numeric_limits<int>::max())
            {
            }

            double renderStart;
            double renderEnd;
            double swapStart;
            double swapEnd;
            double evaluateStart;
            double evaluateEnd;
            double userRenderStart;
            double userRenderEnd;
            double frameChangeEventStart;
            double frameChangeEventEnd;
            double internalRenderStart;
            double internalRenderEnd;
            double internalPrefetchStart;
            double internalPrefetchEnd;
            double prefetchRenderStart;
            double prefetchRenderEnd;
            double prefetchUploadPlaneTotal;
            double renderUploadPlaneTotal;
            double renderFenceWaitTotal;
            double expectedSyncTime;
            double deviceClockOffset;

            int gccount;

            int frame;
        };

        typedef std::vector<ProfilingRecord> ProfilingRecordVector;

        double profilingElapsedTime();
        ProfilingRecord& beginProfilingSample();
        ProfilingRecord& currentProfilingSample();
        void endProfilingSample();
        void dumpProfilingToFile(std::ostream&);

        bool postFirstNonEmptyRender() { return m_postFirstNonEmptyRender; }

        void addMissingInfo(const std::string s)
        {
            m_missingFrameInfos.push_back(s);
        }

        const std::vector<std::string>& missingFrameInfo() const
        {
            return m_missingFrameInfos;
        }

        void setSessionStateFromNode(IPNode*);

        void queryAndStoreGLInfo();

        void waitForUploadToFinish();

        bool beingDeleted() { return m_beingDeleted; }

        void physicalDeviceChanged(const VideoDevice*);

    protected:
        virtual TwkApp::EventNode::Result propagateEvent(const TwkApp::Event&);

        void writeGTO(const std::string& filename,
                      const WriteObjectVector& headerObjects,
                      const WriteRequest& request,
                      const bool writeSession = true);

        void unpackSessionContainer(PropertyContainer*);

        virtual void readGTO(const std::string& filename, bool merge = false);

        void chooseNextBestRenderer();

        void checkInDisplayImage();
        void checkInPreDisplayImage();
        void swapDisplayImage();
        int successorFrame(int) const;

        void lockRangeDirty() { pthread_mutex_lock(&m_rangeDirtyMutex); }

        void unlockRangeDirty() { pthread_mutex_unlock(&m_rangeDirtyMutex); }

        //
        //  Thread entry points
        //

        static void audioTrampoline(Session*);
        void audioMain();

        void copySessionStateToNode(IPNode*);
        void copyFullSessionStateToContainer(PropertyContainer&);
        void copyConnectionsToContainer(PropertyContainer&);

        void characterizeDisplayImage();
        void addNodeCreationContext(PropertyContainer* pc, int value = 1);

        bool framePatternCheck(int frame) const;

        struct Fdata
        {
            Fdata(double a, int b, double c, double d, double d2, int e, int f,
                  int g, double h, int i, int j)
                : s(a)
                , sync(b)
                , elapsed0(c)
                , elapsed(d)
                , elapsedRaw(d2)
                , rframe(e)
                , frame(f)
                , errorFrame(g)
                , duration(h)
                , newFrame(i)
                , cframe(j)
            {
            }

            double s;
            int sync;
            double elapsed0;
            double elapsed;
            double elapsedRaw;
            int rframe;
            int frame;
            int errorFrame;
            double duration;
            int newFrame;
            int cframe;
        };

        typedef std::deque<Fdata> FdataDeque;

        // RV_AVPLAYBACK_VERSION=1 calls
        void play_v1(std::string& eventData);
        void addSyncSample_v1();
        void postRender_v1();
        int targetFrame_v1(double elapsed) const;

        // RV_AVPLAYBACK_VERSION=2 calls
        void play_v2(std::string& eventData);
        void addSyncSample_v2();
        Time predictedTimeUntilSync_v2() const;
        void render_v2();
        void postRender_v2();
        int targetFrame_v2(
            double elapsed) const; // also applies to RV_AVPLAYBACK_VERSION=0

    protected:
        bool m_waitForUploadThreadPrefetch;
        IPGraph* m_graph;
        std::string m_name;
        ImageRenderer* m_renderer;
        Timer m_timer;
        Timer m_stopTimer;
        Timer m_userTimer;
        Time m_timerOffset;

        bool m_useExternalVSyncTiming;
        Time m_nextVSyncTime;
        Time m_renderedVSyncTime;
        size_t m_vSyncSerial;
        size_t m_vSyncSerialLast;
        mutable Mutex m_vSyncMutex;
        Condition m_vSyncCond;
        bool m_waitingOnSync;

        Frames m_markedFrames;
        SessionType m_sessionType;
        FBStatusVector m_displayFBStatus;
        pthread_mutex_t m_rangeDirtyMutex;
        bool m_rangeDirty;
        IPImage* m_proxyImage;
        IPImage* m_errorImage;
        IPImage* m_displayImage;
        int m_displayFrame;
        IPImage* m_preDisplayImage;
        int m_preDisplayFrame;
        CachingMode m_cacheMode;
        PlayMode m_playMode;
        bool m_preEval;
        int m_rangeStart;
        int m_rangeEnd;
        int m_narrowedRangeStart;
        int m_narrowedRangeEnd;
        int m_inPoint;
        int m_outPoint;
        int m_playStartFrame;
        int m_inc;
        float m_fps;
        int m_frame;
        double m_overhead;
        int m_shift;
        bool m_realtime;
        bool m_realtimeOverride;
        int m_skipped;
        int m_lastFrame;
        double m_lastCheckTime;
        int m_lastCheckFrame;
        int m_lastDevFrame;
        int m_lastViewFrame;
        bool m_rendering;
        bool m_wantsRedraw;
        bool m_fullScreen;
        double m_realfps;
        bool m_receivingEvents;
        bool m_scrubAudio;
        bool m_bufferWait;
        bool m_latencyWait;
        bool m_wrapping;
        bool m_batchMode;
        StringVector m_missingFrameInfos;
        bool m_noSequences;
        bool m_fastStart;
        pthread_mutex_t m_audioRendererMutex;
        bool m_audioPlay;
        bool m_audioFirstPass;
        double m_audioLoopDuration;
        size_t m_audioLoopCount;
        double m_audioTimeShift;
        size_t m_audioInitialSample;
        size_t m_audioStartSample;
        double m_audioSampleShift;
        double m_audioElapsedTime;
        Timer m_audioPlayTimer;
        bool m_audioUnavailble;
        static float m_audioDrift;
        bool m_beingDeleted;
        CacheStats m_cacheStats;
        Time m_syncOffset;
        SyncTimes m_syncTimes;
        size_t m_syncMaxSamples;
        size_t m_syncSamples;
        Time m_syncInterval;
        Time m_syncLastTime;
        Time m_syncLastWaitTime;
        bool m_syncPredictionEnabled;
        mutable float m_syncTargetRefresh;
        int m_inputFileVersion;
        int m_avPlaybackVersion;
        bool m_enableFastTurnAround;
        double m_lastDrawingTime;
        bool m_filterLiveReviewEvents{false};

        class FpsCalculator;
        struct FBStatusCheck;

        FpsCalculator* m_fpsCalc;

        StringVector m_viewStack;
        int m_viewStackIndex;

        ProfilingRecordVector m_profilingSamples;
        Timer m_profilingTimer;

        bool m_preFirstNonEmptyRender;
        bool m_postFirstNonEmptyRender;

        const VideoDevice* m_controlVideoDevice;
        const VideoDevice* m_outputVideoDevice;
        const VideoDevice* m_eventVideoDevice;

        static float m_maxBufferedWaitSeconds;
        static float m_cacheLookBehindFraction;
        static size_t m_maxGreedyCacheSize;
        static size_t m_maxBufferCacheSize;
        static bool m_usePreEval;
        static bool m_threadedUpload;
        static StringVector m_vStrings;
        static StringVector m_VStrings;

        VoidSignal m_updateSignal;
        VoidSignal m_updateLoadingSignal;
        VoidSignal m_forcedUpdateSignal;
        VoidSignal m_fullScreenOnSignal;
        VoidSignal m_fullScreenOffSignal;
        VoidSignal m_sessionChangedSignal;
        VoidSignal m_stereoHardwareOnSignal;
        VoidSignal m_stereoHardwareOffSignal;
        VoidSignal m_audioUnavailableSignal;
        VoidSignal m_eventDeviceChangedSignal;
        FrameSignal m_inPointChangedSignal;
        FrameSignal m_outPointChangedSignal;
        RangeSignal m_rangeChangedSignal;
        RangeSignal m_rangeNarrowedSignal;
        StringSignal m_beforeSessionReadSignal;
        StringSignal m_afterSessionReadSignal;
        VoidSignal m_beforeSessionClearSignal;
        VoidSignal m_afterSessionClearSignal;
        StringSignal m_beforeSessionWriteSignal;
        StringSignal m_afterSessionWriteSignal;
        VoidSignal m_realtimeModeChangedSignal;
        VoidSignal m_playModeChangedSignal;
        StringSignal m_beforePlayStartSignal;
        StringSignal m_playStartSignal;
        StringSignal m_playStopSignal;
        FrameSignal m_markFrameSignal;
        FrameSignal m_unmarkFrameSignal;
        FrameAndStringSignal m_frameChangedSignal;
        TimeSignal m_fpsChangedSignal;
        StringSignal m_cacheModeChangedSignal;
        VoidSignal m_bgChangedSignal;
        VideoDeviceSignal m_outputVideoDeviceChangedSignal;
        VideoDeviceSignal m_physicalVideoDeviceChangedSignal;
        NodeSignal m_beforeGraphViewChangeSignal;
        NodeSignal m_afterGraphViewChangeSignal;
        VoidSignal m_playIncChangedSignal;
        VideoDeviceSignal m_marginsChangedSignal;
        VoidSignal m_glQueryCompleteSignal;
        IPCore::PropertyInfo* m_notPersistent;
        bool m_readingGTO;
        mutable IntDeque m_frameHistory;
        mutable IntVector m_frameRuns;
        mutable bool m_framePatternFail;
        mutable int m_framePatternFailCount;
        mutable FdataDeque m_lastFData;

    public:
        static Session* m_currentSession;
    };

    extern bool debugProfile;
    extern bool debugPlayback;
    extern bool debugPlaybackVerbose;

    template <typename T>
    void Session::findPropertyOfType(std::vector<T*>& props,
                                     const std::string& name)
    {
        PropertyVector p;
        findProperty(p, name);

        if (!p.empty())
        {
            for (int i = 0; i < p.size(); i++)
            {
                if (T* tp = dynamic_cast<T*>(p[i]))
                {
                    props.push_back(tp);
                }
            }
        }

        return;
    }

} // namespace IPCore

#endif // __IPCore__Session__h__
