//******************************************************************************
// Copyright (c) 2007 Tweak Inc. 
// All rights reserved.
// 
// SPDX-License-Identifier: Apache-2.0
// 
//******************************************************************************
#ifndef __IPCore__IPGraph__h__
#define __IPCore__IPGraph__h__
#include <TwkApp/EventNode.h>
#include <IPCore/IPImage.h>
#include <IPCore/IPNode.h>
#include <IPCore/FBCache.h>
#include <IPCore/IPProperty.h>
#include <TwkAudio/AudioCache.h>
#include <TwkContainer/PropertyContainer.h>
#include <stl_ext/thread_group.h>
#include <TwkUtil/Timer.h>
#include <boost/signals2.hpp>
#include <boost/thread.hpp>
#include <atomic>
#include <deque>
#include <mutex>
#include <set>

namespace TwkAudio {
class AudioBuffer;
}

namespace TwkApp {
class VideoModule;
class VideoDevice;
}

namespace IPCore {
class AudioTextureIPNode;
class CacheIPNode;
class DisplayGroupIPNode;
class ViewGroupIPNode;
class DispTransform2DIPNode;
class OutputGroupIPNode;
class RootIPNode;
class SoundTrackIPNode;
class SessionIPNode;
class NodeManager;
class NodeDefinition;

//
//  IPGraph
//
//  This class controls the IPNode graph. It also owns one or more
//  evaluation threads which are the only threads allowed to evaluate
//  the graph. The IPGraph can also cache results and guarantee that
//  pixels will be available for a certain amount of time.
//

class IPGraph : public TwkApp::EventNode
{
public:
    //
    //  Types
    //

    enum EvaluationMode
    {
        EvalSynchornous,
        EvalAsynchornous
    };

    enum CachingMode
    {
        NeverCache,
        BufferCache,
        GreedyCache
    };

    enum EvalStatus
    {
        EvalNormal,
        EvalError,
        EvalBufferNeedsRefill
    };

    struct EvalThreadData
    {
        size_t   id;
        IPGraph* graph;
        bool     running;
        pthread_t mythread;
    };

    typedef boost::function<void()> VoidFunction;

    struct WorkItem
    {
        WorkItem(const VoidFunction& F, const char* t = 0)
            : function(F),
              tag(t) {}

        VoidFunction function;
        const char*  tag;
    };
    typedef boost::recursive_mutex              RecursiveMutex;
    typedef boost::recursive_mutex::scoped_lock ScopedRecursiveLock;
    typedef boost::thread                       Thread;
    typedef stl_ext::thread_group               ThreadGroup;
    typedef std::map<std::string, IPNode*>      NodeMap;
    typedef TwkContainer::Property              Property;
    typedef TwkContainer::PropertyContainer     Container;
    typedef std::vector<Property*>              PropertyVector;
    typedef std::vector<IPNode*>                NodeVector;
    typedef std::set<IPNode*>                   NodeSet;
    typedef std::pair<EvalStatus, IPImage*>     EvalResult;
    typedef std::map<CachingMode, size_t>       CacheSizeMap;
    typedef TwkAudio::AudioBuffer               AudioBuffer;
    typedef TwkAudio::AudioCache                AudioCache;
    typedef TwkContainer::FloatProperty         FloatProperty;
    typedef TwkContainer::IntProperty           IntProperty;
    typedef TwkAudio::SampleTime                SampleTime;
    typedef std::vector<EvalThreadData>         ThreadDataVector;
    typedef TwkUtil::Timer                      Timer;
    typedef IPNode::TestEvaluationResult        TestEvalResult;
    typedef std::vector<std::string>            StringVector;
    typedef std::pair<std::string,std::string>  StringPair;
    typedef std::vector<StringPair>             StringPairVector;
    typedef IPNode::IPNodes                     IPNodes;
    typedef TwkApp::VideoModule                 VideoModule;
    typedef TwkApp::VideoDevice                 VideoDevice;
    typedef std::vector<std::shared_ptr<VideoModule>> VideoModules;
    typedef std::vector<DisplayGroupIPNode*>    DisplayGroups;
    typedef IPImage::InternalGLMatricesContext  InternalGLMatricesContext;
    typedef int                                 WorkItemID;
    typedef std::set<WorkItemID>                WorkItemIDSet;

    typedef boost::signals2::signal<void ()>                    VoidSignal;
    typedef boost::signals2::signal<void (IPNode*)>             NodeSignal;
    typedef boost::signals2::signal<void (const Property*)>     PropertySignal;
    typedef boost::signals2::signal<void (const std::string&,
                                          const std::string&)>  NodeNameProtocolSignal;
    typedef boost::signals2::signal<void (const VideoDevice*)>  DeviceSignal;
    typedef boost::signals2::signal<void (const VideoDevice*, 
                                          const VideoDevice*)>  DeviceChangedSignal;

    IPGraph(const NodeManager*);
    ~IPGraph();

    //
    //  SIGNALS
    //

    NodeSignal&             newNodeSignal() { return m_newNodeSignal; }
    NodeSignal&             nodeInputsChangedSignal() { return m_nodeInputsChangedSignal; }
    NodeSignal&             nodeImageStructureChangedSignal() { return m_nodeImageStructureChangedSignal; }
    NodeSignal&             nodeRangeChangedSignal() { return m_nodeRangeChangedSignal; }
    NodeSignal&             nodeMediaChangedSignal() { return m_nodeMediaChangedSignal; }
    NodeSignal&             nodeWillDeleteSignal() { return m_nodeWillDeleteSignal; }
    NodeNameProtocolSignal& nodeDidDeleteSignal() { return m_nodeDidDeleteSignal; }
    NodeSignal&             viewNodeChangedSignal() { return m_viewNodeChangedSignal; }
    VoidSignal&             graphEditedSignal() { return m_graphEditSignal; }
    VoidSignal&             textureCacheUpdated() { return m_textureCacheUpdated; }
    DeviceChangedSignal&    videoDeviceChangedSignal() { return m_deviceChangedSignal; }
    DeviceSignal&           primaryVideoDeviceChangedSignal() { return m_primaryDeviceChangedSignal; }
    PropertySignal&         propertyChangedSignal() { return m_propertyChangedSignal; }

    // nodeWillRemoveSignal() is raised when a node will be remove from the graph
    NodeSignal&             nodeWillRemoveSignal() { return m_nodeWillRemoveSignal; }


    //
    //  Work Items
    //
    //  You can submit an work item to IPGraph which may run
    //  asynchronously. On completion your VoidFunction will be
    //  called. For example for an IPNode to load a file async and
    //  have the node's member function called on completion by the
    //  async thread:
    //
    //  {
    //      string filename = ...;
    //      size_t workID = graph().addWorkItem(bind(&MyIPNode::read, this, filename), "openfile")
    //  }
    //
    //  void MyIPNode::read(std::string& filename)
    //  {
    //      // read will be called by some work thread
    //  }
    //
    //  NOTE: this function *may* immediately call the passed in
    //  VoidFunction and block so don't expect any particular timing
    //  to occur. 
    //

    // adds a new work item job to execute
    WorkItemID addWorkItem(const VoidFunction&, const char* tag=0);

    // removes a work item in job in the pending queue
    void removeWorkItem(WorkItemID id);

    // waits for the completion of the work item job
    void waitWorkItem(WorkItemID id);

    // proritize a work item in the pending queue
    void prioritizeWorkItem(WorkItemID id);


    //
    //  If forDisplay is true, the cache "display frame" will be
    //  updated. This is the frame number it uses as a boundary to
    //  determine which frames it can discard, etc. So if you're
    //  prefetching images do not set forDisplay to true.
    //

    EvalResult evaluateAtFrame(int frame, bool forDisplay=true);

    //
    //  Post evaluation should be called at a minimally critical
    //  time. This function will possibly awaken sleeping threads and
    //  could cause a context switch (or 2 or 3). So e.g. right after
    //  a swapBuffers() call is a good time to call this.
    //

    void postEvaluation();

    //
    //  After you evaluateAtFrame and you no longer need the result,
    //  you sould check the IPImage* back in. If you are using a pre
    //  evaluated image and need the display frame updated, set that
    //  to true and provide the proper frame number.
    //

    void checkInImage(IPImage*, bool updateDisplayframe=false, int frame=0);
    //
    //  Similar to above, but for audio. The IPGraph evaluates audio
    //  asynchronously and caches the results. If block is true, the
    //  thread will not return until the requested audio is
    //  available. Otherwise, the thread will return with silence
    //  immediately.
    //

    size_t audioFillBuffer(const IPNode::AudioContext&);

    //
    //  Configure the audio in the graph
    //

    struct AudioConfiguration : IPNode::AudioConfiguration
    {
        AudioConfiguration()
            : IPNode::AudioConfiguration(0, TwkAudio::UnknownLayout, 0),
              fps(0),
              backwards(false),
              startSample(0),
              endSample(0) {}

        AudioConfiguration(double deviceRate,
                           TwkAudio::Layout layout,
                           size_t numSamples,
                           double fps_,
                           bool backwards_,
                           size_t startSample_,
                           size_t endSample_)
            : IPNode::AudioConfiguration(deviceRate, layout, numSamples),
              fps(fps_),
              backwards(backwards_),
              startSample(startSample_),
              endSample(endSample_) {}

        double fps;
        bool   backwards;
        size_t startSample;
        size_t endSample;

        bool operator == (const AudioConfiguration& c)
        {
            return c.fps            == fps &&
                   c.backwards      == backwards &&
                   // c.startSample == startSample &&
                   c.endSample      == endSample &&
                   c.layout         == layout &&
                   c.rate           == rate &&
                   c.samples        == samples;
        }
    };

    void audioConfigure(const AudioConfiguration&);
    const AudioConfiguration& lastAudioConfiguration () const { return m_lastAudioConfiguration; }
    void setAudioCachingMode(CachingMode);
    CachingMode audioCachingMode() const { return m_audioCacheMode; }
    void flushAudioCache();
    void primeAudioCache(int frame, float frameRate);
    double audioSecondsCached() const;
    void setAudioCacheExtents(TwkAudio::Time minSec, TwkAudio::Time maxSec);
    void setAudioThreading(bool);
    bool isAudioConfigured() const { return m_audioConfigured; }
    AudioCache& audioCache() const { return m_audioCache; }

    //
    //  Node Manager
    //

    const NodeManager* nodeManager() const { return m_nodeManager; }

    //
    //  Reset or just clear the graph. reset() will add all available
    //  output devices and retrieve their settings.
    //

    void clear();
    void reset(const VideoModules&);

    //
    //  Add physical devices to graph -- the default values for these
    //  may be determined by external db
    //

    void setPhysicalDevices(const VideoModules&);

    //
    //  Create a NodeValidation object on the stack. A node validation
    //  context will become current. When complete the former context
    //  will become current again.
    //

    struct NodeValidation
    {
        NodeValidation(IPNode* n) : node(n) { node->graph()->beginNodeValidation(node); }
        ~NodeValidation() { node->graph()->endNodeValidation(node); }
        IPNode* node;
    };

    void beginNodeValidation(IPNode*);
    void endNodeValidation(IPNode*);

    //
    //  Editing: if you change add/remove/change a node in the graph,
    //  you must do it between calls to these functions.
    //
    //  The GraphEdit class should be created on the stack to make the
    //  begin/end graph edit exception safe (don't call the functions
    //  directly use the class). If lock is false, the GraphEdit
    //  object will not lock the graph. The second constructor will
    //  test the properties in the PropertyVector to see if any of
    //  them requires locking and if so it will lock.
    //

    class GraphEdit
    {
      public:
        GraphEdit(IPGraph& graph, bool lock = true, bool programFlush = false, bool audioFlush = false);
        template <class T> GraphEdit(IPGraph& graph, std::vector<T*>& props);
        ~GraphEdit();

      private:
        bool     m_lock;
        bool     m_programFlush;
        bool     m_audioFlush;
        IPGraph& m_graph;
    };

    void beginGraphEdit();
    void endGraphEdit();
    void programFlush();

    //
    //  Caching Mode
    //

    void setCachingMode(CachingMode mode, 
                        int inframe, int outframe, 
                        int minFrame, int maxFrame,
                        int inc, int curframe,
                        float fps);
    void setNumEvalThreads(size_t n);
    size_t numEvalThreads() const { return ((m_threadGroup) ? m_threadGroup->num_threads() : 0) + 1; }
    void setCacheModeSize(CachingMode mode, size_t size);
    CachingMode cachingMode() const { return m_cacheMode; }
    bool isCacheThreadRunning() const;
    FBCache& cache() { return m_fbcache; }
    const FBCache& cache() const { return m_fbcache; }
    void flushRange(int start, int end);

    //
    //  Return a usable context for evaluation-like operations for the
    //  given *global* frame number.
    //

    IPNode::Context contextForFrame(int frame, 
                                    IPNode::ThreadType t = IPNode::DisplayEvalThread,
                                    bool stereo=false) const;


    //
    //  The root node, and the session information
    //

    IPNode* root() const { return m_rootNode; }
    const NodeMap& nodeMap() const { return m_nodeMap; }
    SessionIPNode* sessionNode() const { return m_sessionNode; }

    //
    //  Setting the primary display group will 
    //

    void setPrimaryDisplayGroup(DisplayGroupIPNode*);
    void setPrimaryDisplayGroup(const TwkApp::VideoDevice* device);
    DisplayGroupIPNode* primaryDisplayGroup() const { return m_displayGroups.empty() ? 0 : m_displayGroups.front(); }
    DisplayGroupIPNode* findDisplayGroupByDevice(const TwkApp::VideoDevice*) const;
    const DisplayGroups& displayGroups() const { return m_displayGroups; }

    void addDisplayGroup(DisplayGroupIPNode*);
    void connectDisplayGroup(DisplayGroupIPNode*, bool atbeginning=true, bool connectToView=true);
    void connectDisplayGroup(const TwkApp::VideoDevice*, bool atbeginning=true, bool connectToView=true);
    void disconnectDisplayGroup(DisplayGroupIPNode*);
    void disconnectDisplayGroup(const TwkApp::VideoDevice*);
    void removeDisplayGroup(DisplayGroupIPNode*);
    void removeDisplayGroup(const TwkApp::VideoDevice*);

    void deviceChanged(const TwkApp::VideoDevice* oldDevice,
                       const TwkApp::VideoDevice* newDevice) const;

    //
    //  NOTE: setting root nodes will remove all other inputs
    //

    void setRootNodes1(IPNode*);
    void setRootNodes(const IPNodes&);

    //
    //  This is the default output group for batch output. It is not a
    //  "real" display
    //

    OutputGroupIPNode* defaultOutputGroup() const { return m_defaultOutputGroup; }

    //
    //  Turn caching on/off for the whole graph
    //

    void setCacheNodesActive(bool);
    void dispatchCachingThread(int inframe,
                               int outframe,
                               int minFrame,    // whole range
                               int maxFrame,
                               int inc, 
                               int frame, 
                               float fps,
                               bool modeChanged);
    void finishCachingThread();

    //
    //  If the graph will produce Audio this will be true
    //

    bool hasAudio() const { return m_audioSources.size() > 0; }
    void updateHasAudioStatus(IPNode*);

    //
    //  Set graphic configuration. New API is setViewNode()
    //

    void setViewNode(IPNode* node);

    IPNode* viewNode() const { return m_viewNode; }
    const NodeMap& viewableNodes() const { return m_viewNodeMap; }

    //
    //  This is the viewGroup not the viewNode (which is the input to the viewgroup).
    //

    ViewGroupIPNode* viewGroup() const { return m_viewGroupNode; }

    void setDevice(const TwkApp::VideoDevice* controlDevice, const TwkApp::VideoDevice* outputDevice);

    //
    //  Generate a unqiue name from the given one (could be the same as
    //  input)
    //

    std::string uniqueName(const std::string&) const;
    std::string canonicalNodeName(const std::string& nodeName) const;

    //
    //  Find property special cookies:
    //
    //  0) nodeName.*   -> prop in Node
    //  1) #Type.*      -> all props in Node of Type along all rendering paths
    //  2) @Type.*      -> first prop in Node of Type, along all rendering paths
    //  3) :Type.*      -> prop in definition of Type
    //  4) nodeName/... -> prefixed by nodeName/ root lookup at nodeName
    //                     e.g. sourceGroup00001/#Color.color.exposure
    //

    IPNode* findNode(const std::string& name) const;
    IPNode* findNodeByUIName(const std::string& name) const;
    IPNode* findNodeAssociatedWith(IPNode*, const std::string& type) const;
    void findProperty(int frame, PropertyVector& props, const std::string& name);
    void findNodesByTypeName(int frame, NodeVector& nodes, const std::string& typeName) const;
    void findNodesByTypeName(NodeVector& nodes, const std::string& typeName) const;
    void findNodesByTypeNameWithProperty(NodeVector& nodes, 
                                         const std::string& typeName,
                                         const std::string& propName);

    void findNodesByPattern(int frame, NodeVector& nodes, const std::string& pattern);

    template <typename T>
    T* findNodeOfType(const std::string& name) const;

    template <typename T>
    void findPropertyOfType(int frame,
                            std::vector<T*>& props,
                            const std::string& name);

    //
    //  Evaluation Thread Only
    //

    void evalThreadMain(EvalThreadData*);

    //
    //  Audio eval
    //

    void evalAudioThreadMain();

    //
    //  debug
    //

    static void setDebugTreeOutput(bool b) { m_debugTreeOutput = b; }


    bool cacheThreadContinue();

    float lookBehindFraction() { return m_fbcache.lookBehindFraction(); };
    void setLookBehindFraction(float f);

    static void setMaxBufferedWaitTime(float f) { m_maxBufferedWaitTime = f; };
    float maxBufferedWaitTime() { return m_maxBufferedWaitTime; };
    static void setMinCacheSize(size_t bytes) { m_minCacheSize = bytes; };

    void nodeConnections(StringPairVector& pairs, bool toplevel=false) const;

    bool hasConstructor(const std::string& typeName);
    const NodeDefinition* nodeDefinition(const std::string& typeName);
    IPNode* newNode(const std::string& typeName, const std::string& nodeName, GroupIPNode* group=0);

    template <class T>
    T* newNodeOfType(const std::string& typeName, const std::string& nodeName)
    {
        return dynamic_cast<T*>(newNode(typeName, nodeName));
    }

    bool isDefaultView(std::string name) { return (m_defaultViewsMap.find(name) != m_defaultViewsMap.end()); };

    //
    //  Profiling mirrors what the session has. The session passes in
    //  its profiling clock so that the graph can use it too. The graph
    //  records its own profile data which is merged in when the
    //  session writes out its file.
    //

    struct EvalProfilingRecord
    {
        EvalProfilingRecord()
            : cacheTestStart(0), cacheTestEnd(0),
              evalInternalStart(0), evalInternalEnd(0),
              evalIDStart(0), evalIDEnd(0),
              cacheQueryStart(0), cacheQueryEnd(0),
              cacheEvalStart(0), cacheEvalEnd(0),
              ioStart(0), ioEnd(0),
              restartThreadsAStart(0), restartThreadsAEnd(0),
              restartThreadsBStart(0), restartThreadsBEnd(0),
              cacheTestLockStart(0), cacheTestLockEnd(0),
              setDisplayFrameStart(0), setDisplayFrameEnd(0),
              frameCachedTestStart(0), frameCachedTestEnd(0),
              awakenThreadsStart(0), awakenThreadsEnd(0)
            {}

        double  cacheTestStart;
        double  cacheTestEnd;
        double  evalInternalStart;
        double  evalInternalEnd;
        double  evalIDStart;
        double  evalIDEnd;
        double  cacheQueryStart;
        double  cacheQueryEnd;
        double  cacheEvalStart;
        double  cacheEvalEnd;
        double  ioStart;
        double  ioEnd;

        double  restartThreadsAStart;
        double  restartThreadsAEnd;
        double  restartThreadsBStart;
        double  restartThreadsBEnd;
        double  cacheTestLockStart;
        double  cacheTestLockEnd;
        double  setDisplayFrameStart;
        double  setDisplayFrameEnd;
        double  frameCachedTestStart;
        double  frameCachedTestEnd;
        double  awakenThreadsStart;
        double  awakenThreadsEnd;
    };

    typedef std::vector<EvalProfilingRecord> ProfilingVector;
    
    bool needsProfilingSamples() const { return m_profilingTimer != 0; }
    void setProfilingClock(Timer*);
    double profilingElapsedTime();
    EvalProfilingRecord& beginProfilingSample();
    EvalProfilingRecord& currentProfilingSample();
    void endProfilingSample() {}
    const ProfilingVector& profilingSamples() const { return m_profilingSamples; }

    // Asynchronous media loading detection - Progressive source loading mode
    // When loading sources in progressive source loading mode, the actual media 
    // loading is deferred until after the RvSession::continueLoading() has 
    // completed. Now the problem here, is that the caching might be restarted 
    // too soon and the media loading and the cache would be fighting which 
    // would lead to the main thread being unnecessarily blocked for long 
    // periods of time which would defeat the purpose of the cache which is to 
    // optimize performances.
    // This is especially an issue when playing back media from URL or high
    // latency media storage. 
    // By keeping track of when we still have media loading asynchronously, we 
    // will be able to restart caching at the right time.
    // Note that the following three methods are thread safe with respect to the
    // set container used to hold the list of async media loading in progress.

    // Returns true if there is still media being loaded asynchronously
    // otherwise returns false. 
    bool isMediaLoading() const;

    // Indicate that the specified media has been scheduled for async loading
    void mediaLoadingBegin(WorkItemID mediaWorkItemId);

    // Indicate that the specified media has completed async loading
    void mediaLoadingEnd(WorkItemID mediaWorkItemId);

    // this signal is raised when the last source loading is complete
    VoidSignal & mediaLoadingSetEmptySignal() { return m_mediaLoadingSetEmptySignal; }

    //----------------------------------------------------------------------

    //
    //  API
    //

    //
    //  Create a sparse node from the given node. This will share prop
    //  data via referencing
    //

    virtual TwkContainer::PropertyContainer* sparseContainer(IPNode*);

    virtual void initializeIPTree(const VideoModules&);
    virtual void addNode(IPNode*);
    virtual void removeNode(IPNode*);
    virtual DisplayGroupIPNode* newDisplayGroup(const std::string& nodeName, const TwkApp::VideoDevice* d = 0);
    virtual OutputGroupIPNode* newOutputGroup(const std::string& nodeName, const TwkApp::VideoDevice* d = 0);

    // Undo/Redo support
    virtual void isolateNode(IPNode*);
    virtual void restoreIsolatedNode(IPNode*);

    virtual void addIsolatedNode(IPNode*);
    virtual void removeIsolatedNode(IPNode*);
    IPNode* findNodePossiblyIsolated(const std::string&);

    virtual void propertyChanged(const Property*);
    virtual void newPropertyCreated(const Property*);
    virtual void propertyWillBeDeleted(const Property*);
    virtual void propertyDeleted(const std::string&);
    virtual void propertyDidInsert(const Property*, size_t, size_t);
    virtual void propertyWillInsert(const Property*, size_t, size_t);
    virtual void inputsChanged(IPNode*);

    virtual void rangeChanged(IPNode*);
    virtual void imageStructureChanged(IPNode*);
    virtual void mediaChanged(IPNode*);

    virtual void deleteNode(IPNode*);

    void redispatchCachingThread();

    bool noPromotion() { return m_noPromotion; }
    void setNoPromotion(bool b) { m_noPromotion = b; }

    void requestClearAudioCache()  { m_clearAudioCacheRequested = true; }

    //----------------------------------------------------------------------

  protected:
    //
    //  Called by IPNode constructor/destructor
    //

    IPImage* evaluate(int, IPNode::ThreadType thread, size_t n=0);
    TestEvalResult testEvaluate(int, IPNode::ThreadType thread, size_t n=0);
    size_t audioFillBufferInternal(const IPNode::AudioContext&);

    void finishCachingThreadASync();

    void lockInternal() const;
    void unlockInternal() const;

    void lockAudioInternal() const;
    void unlockAudioInternal() const;

    bool tryLockAudioFill() const;
    void lockAudioFill() const;
    void unlockAudioFill() const;

    void dispatchAudioThread();
    void maybeDispatchAudioThread();
    void finishAudioThread();

    TwkAudio::Time globalAudioOffset() const;
    
    void stopCachingInternal();

    void awakenAllCachingThreads();

    void promoteFBsInFrameRange (int beg, int mid, int end, TwkUtil::Timer t);

    void setPhysicalDevicesInternal(const VideoModules&);

    void dispatchCachingThreadsSafely();

    void updateHasAudio() const;

    int getMaxGroupSize(bool slowMedia) const;

    //--------------------------------------------------------------------------
    // Audio related helper methods
    //
    void _audioClearCacheOutsideHeadAndTail();
    void _audioMapLinearVolumeToPerceptual( AudioBuffer& inout_buffer );
    bool _isCurrentAudioSampleBeyondHead( const SampleTime audioHeadSamples ) const;
    //--------------------------------------------------------------------------

  protected:
    const NodeManager*          m_nodeManager;
    IPNode*                     m_rootNode;
    IPNode*                     m_viewNode;
    ViewGroupIPNode*            m_viewGroupNode;
    NodeMap                     m_nodeMap;
    NodeMap                     m_isolatedNodes;
    NodeMap                     m_defaultViewsMap;
    NodeMap                     m_viewNodeMap;
    NodeVector                  m_renderOutputNodeVector;
    CachingMode                 m_cacheMode;
    mutable FBCache             m_fbcache;
    CacheSizeMap                m_cacheSizeMap;
    int                         m_editing;
    WorkItemIDSet               m_mediaLoadingSet;
    mutable std::mutex          m_mediaLoadingSetMutex;
    FloatProperty*              m_volume;
    FloatProperty*              m_balance;
    IntProperty*                m_mute;
    IntProperty*                m_audioSoftClamp;
    FloatProperty*              m_audioOffset;
    FloatProperty*              m_audioOffset2;
    SessionIPNode*              m_sessionNode;
    DisplayGroups               m_displayGroups;
    OutputGroupIPNode*          m_defaultOutputGroup;
    bool                        m_cacheMissed;
    bool                        m_cacheStop;
    ThreadDataVector            m_threadData;
    ThreadGroup*                m_threadGroup;
    ThreadGroup*                m_threadGroupSingle;
    mutable pthread_mutex_t     m_internalLock;
    mutable pthread_mutex_t     m_dispatchLock;
    const IPNode::AudioContext* m_audioRequestContext;
    bool                        m_inAudioConfig;
    bool                        m_inFinishAudioThread;
    double                      m_audioFPS;
    std::atomic<SampleTime>     m_minAudioSample;
    std::atomic<SampleTime>     m_maxAudioSample;
    std::atomic<SampleTime>     m_currentAudioSample;
    std::atomic<SampleTime>     m_lastFillSample;
    std::atomic<SampleTime>     m_restartAudioThreadSample;
    NodeSet                     m_audioSources;
    mutable AudioCache          m_audioCache;
    CachingMode                 m_audioCacheMode;
    CachingMode                 m_audioPendingCacheMode;
    bool                        m_audioBackwards;
    bool                        m_audioThreadStop;
    bool                        m_audioConfigured;
    bool                        m_audioCacheComplete;
    bool                        m_audioThreading;
    bool                        m_noPromotion;
    std::atomic<double>         m_audioMinCache;
    std::atomic<double>         m_audioMaxCache;
    size_t                      m_audioPacketSize;
    AudioBuffer                 m_audioLastBuffer;
    ThreadGroup                 m_audioThreadGroup;
    mutable pthread_mutex_t     m_audioInternalLock;
    mutable pthread_mutex_t     m_audioFillLock;
    bool                        m_frameCacheInvalid;
    bool                        m_postFirstEval;
    bool                        m_topologyChanged;
    bool                        m_cacheTimingOutput;
    AudioConfiguration          m_lastAudioConfiguration;
    Timer                       m_timer;
    bool                        m_newFrame;
    Timer*                      m_profilingTimer;
    ProfilingVector             m_profilingSamples;
    static float                m_maxBufferedWaitTime;
    static size_t               m_minCacheSize;
    static bool                 m_debugTreeOutput;
    static int                  m_maxCacheGroupSize;
    const TwkApp::VideoDevice*  m_controlDevice;
    const TwkApp::VideoDevice*  m_outputDevice;
    NodeSignal                  m_newNodeSignal;
    NodeSignal                  m_nodeInputsChangedSignal;
    NodeSignal                  m_nodeWillDeleteSignal;
    NodeNameProtocolSignal      m_nodeDidDeleteSignal;
    NodeSignal                  m_viewNodeChangedSignal;
    VoidSignal                  m_graphEditSignal;
    VoidSignal                  m_textureCacheUpdated;
    DeviceChangedSignal         m_deviceChangedSignal;
    DeviceSignal                m_primaryDeviceChangedSignal;
    PropertySignal              m_propertyChangedSignal;
    NodeSignal                  m_nodeImageStructureChangedSignal;
    NodeSignal                  m_nodeRangeChangedSignal;
    NodeSignal                  m_nodeMediaChangedSignal;
    VoidSignal                  m_mediaLoadingSetEmptySignal;
    NodeSignal                  m_nodeWillRemoveSignal;
    std::atomic_bool            m_evalSlowMedia;
    void *                      m_jobDispatcher; // opaque pointer SGC::JobDispatcher
    std::atomic_bool            m_clearAudioCacheRequested;
    
    friend class FBCache;
    friend class IPNode;
};


template <typename T>
void
IPGraph::findPropertyOfType(int frame,
                            std::vector<T*>& props,
                            const std::string& name)
{
    PropertyVector p;
    findProperty(frame, p, name);

    if (!p.empty())
    {
        for (int i=0; i < p.size(); i++)
        {
            if (T* tp = dynamic_cast<T*>(p[i]))
            {
                props.push_back(tp);
            }
        }
    }
}

template <typename T>
T* 
IPGraph::findNodeOfType(const std::string& name) const
{
    if (IPNode* n = findNode(name))
    {
        return dynamic_cast<T*>(n);
    }
    else
    {
        return 0;
    }
}

template<class T>
IPGraph::GraphEdit::GraphEdit(IPGraph& graph, std::vector<T*>& props)
    : m_graph(graph),
      m_lock(false),
      m_programFlush(false),
      m_audioFlush(false)
{
    for (size_t i = 0, s = props.size(); i < s; i++)
    {
        if (const PropertyInfo* pinfo = dynamic_cast<const PropertyInfo*>(props[i]->info()))
        {
            if (pinfo->requiresGraphEdit()) { m_lock = true; }
            if (pinfo->requiresProgramFlush()) { m_programFlush = true; }
            if (pinfo->requiresAudioFlush()) { m_audioFlush = true; }
        }
    }

    if (m_lock) m_graph.beginGraphEdit();
}

} // Rv

#endif // __IPCore__IPGraph__h__
