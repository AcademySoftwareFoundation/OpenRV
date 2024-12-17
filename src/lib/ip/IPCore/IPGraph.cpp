//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <IPCore/AdaptorIPNode.h>
#include <IPCore/Application.h>
#include <IPCore/AudioRenderer.h>
#include <IPCore/AudioTextureIPNode.h>
#include <IPCore/CacheIPNode.h>
#include <IPCore/DispTransform2DIPNode.h>
#include <IPCore/DisplayGroupIPNode.h>
#include <IPCore/ViewGroupIPNode.h>
#include <IPCore/Exception.h>
#include <IPCore/IPGraph.h>
#include <IPCore/IPProperty.h>
#include <IPCore/NodeManager.h>
#include <IPCore/OutputGroupIPNode.h>
#include <IPCore/RootIPNode.h>
#include <IPCore/SessionIPNode.h>
#include <IPCore/ShaderProgram.h>
#include <IPCore/SoundTrackIPNode.h>
#include <IPCore/Transform2DIPNode.h>
#include <IPCore/TextureOutputGroupIPNode.h>
#include <IPCore/CoreDefinitions.h>
#include <TwkApp/Event.h>
#include <TwkApp/VideoDevice.h>
#include <TwkApp/VideoModule.h>
#include <TwkAudio/Filters.h>
#include <TwkAudio/Mix.h>
#include <TwkMath/Function.h>
#include <TwkUtil/File.h>
#include <TwkUtil/ThreadName.h>
#include <TwkUtil/FrameUtils.h>
#include <TwkUtil/Macros.h>
#include <TwkUtil/TwkRegEx.h>
#include <TwkUtil/sgcHop.h>
#include <TwkUtil/sgcHopTools.h>
#include <TwkUtil/SystemInfo.h>
#include <TwkUtil/Timer.h>
#include <TwkUtil/Log.h>
#include <TwkUtil/sgcJobDispatcher.h>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <cmath>

#define MIN_WORK_ITEM_THREADS 1
#define MAX_WORK_ITEM_THREADS 4

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#endif

// Specialize template methods to handle job wait, run and finish
// for job data.
//
namespace TwkUtil::JobOps
{
    template <>
    IPCore::IPGraph::WorkItem update<>(Id id, OpType op,
                                       IPCore::IPGraph::WorkItem info)
    {
        switch (op)
        {
        case RUN:
            info.function();
            break;

        case WAIT:
        case FINISH:
            // do nothing
            break;
        }

        return info;
    }
} // namespace TwkUtil::JobOps

namespace IPCore
{
    using namespace std;
    using namespace TwkMovie;
    using namespace TwkMath;
    using namespace TwkUtil;
    using namespace TwkContainer;
    using namespace TwkAudio;
    using namespace boost;

#ifdef PLATFORM_WINDOWS
#define PAD "%06d"
#else
#define PAD "%06zd"
#endif

#if 0
#define DB_GENERAL 0x01
#define DB_DISP 0x02
#define DB_PROMOTE 0x04
#define DB_SIZE 0x08
#define DB_CACHE 0x10
#define DB_ALL 0xff

//  #define DB_LEVEL        (DB_ALL & (~ DB_DISP))
//  #define DB_LEVEL        DB_ALL
//  #define DB_LEVEL        DB_DISP
//  #define DB_LEVEL        DB_GENERAL
//  #define DB_LEVEL        DB_CACHE
//  #define DB_LEVEL        DB_PROMOTE
//  #define DB_LEVEL        DB_ALL

#define DB(x)                  \
    if (DB_GENERAL & DB_LEVEL) \
    cerr << dec << "IPGraph: " << x << endl
#define DBL(level, x)     \
    if (level & DB_LEVEL) \
    cerr << dec << "IPGraph: " << x << endl
#else
#define DB(x)
#define DBL(level, x)
#define DB_LEVEL 0
#define DB_CACHE 0
#endif

    //----------------------------------------------------------------------
    //----------------------------------------------------------------------

    float IPGraph::m_maxBufferedWaitTime = 5.0;
    size_t IPGraph::m_minCacheSize = 150 * 1024 * 1024;
    bool IPGraph::m_debugTreeOutput = false;
    int IPGraph::m_maxCacheGroupSize = 0;

    static pthread_t notAThread;

    static void evalThreadTrampoline(IPGraph::EvalThreadData* d)
    {
        try
        {
            stringstream name;
            name << "IPGraph Eval #" << int(d->id);
            string nameString = name.str();
            setThreadName(nameString);
            HOP_SET_THREAD_NAME(nameString.c_str());
            HOP_ZONE(HOP_ZONE_COLOR_5);

            if (pthread_equal(d->mythread, notAThread))
            {
                d->mythread = pthread_self();
            }
            else if (!pthread_equal(d->mythread, pthread_self()))
            {
                cerr << "ERRROR: corrupted per-thread data." << endl;
            }

            d->graph->evalThreadMain(d);
        }
        catch (std::exception& exc)
        {
            cerr << "ERROR: evaluation thread " << d->id
                 << " exited via exception: " << exc.what() << endl;
            d->running = false;
        }
        catch (...)
        {
            cerr << "ERROR: evaluation thread exited via exception" << endl;
            d->running = false;
        }
    }

    static void evalAudioThreadTrampoline(void* s)
    {
        try
        {
            setThreadName("IPGraph Audio");

            ((IPGraph*)s)->evalAudioThreadMain();
        }
        catch (...)
        {
            cerr << "ERROR: audio evaluation thread exited via exception"
                 << endl;
        }
    }

    IPGraph::GraphEdit::GraphEdit(IPGraph& graph, bool lock, bool pflush,
                                  bool aflush)
        : m_graph(graph)
        , m_lock(lock)
        , m_programFlush(pflush)
        , m_audioFlush(aflush)
    {
        if (m_lock)
            m_graph.beginGraphEdit();
    }

    IPGraph::GraphEdit::~GraphEdit()
    {
        if (m_lock)
            m_graph.endGraphEdit();
        if (m_programFlush)
            m_graph.programFlush();
        if (m_audioFlush)
            m_graph.flushAudioCache();
    }

    namespace
    {

        struct LockObject
        {
            LockObject(pthread_mutex_t& l, bool tryOnly = false)
                : m_lock(l)
                , m_locked(true)
            {
                if (tryOnly)
                {
                    m_locked = (0 == pthread_mutex_trylock(&m_lock));
                }
                else
                    pthread_mutex_lock(&m_lock);
            }

            ~LockObject()
            {
                if (m_locked)
                    pthread_mutex_unlock(&m_lock);
            }

            bool locked() { return m_locked; }

            pthread_mutex_t& m_lock;
            bool m_locked;
        };

    }; // namespace

#define NAF (std::numeric_limits<int>::min())

    IPGraph::IPGraph(const NodeManager* nodeManager)
        : EventNode("IPGraph")
        , m_rootNode(0)
        , m_nodeManager(nodeManager)
        , m_editing(0)
        , m_audioThreadStop(false)
        , m_viewNode(0)
        , m_viewGroupNode(0)
        , m_threadGroup(0)
        , m_threadGroupSingle(0)
        , m_audioThreadGroup(1, 2)
        , m_cacheMode(NeverCache)
        , m_fbcache(this)
        , m_audioRequestContext(0)
        , m_audioFPS(0)
        , m_minAudioSample(0)
        , m_maxAudioSample(0)
        , m_inAudioConfig(false)
        , m_inFinishAudioThread(false)
        , m_audioBackwards(false)
        , m_currentAudioSample(0)
        , m_lastFillSample(0)
        , m_restartAudioThreadSample(0)
        , m_audioThreading(true)
        , m_audioMinCache(3.0)
        , m_audioMaxCache(6.0)
        ,
#ifdef WIN32
        m_audioPacketSize(512)
        ,
#else
        m_audioPacketSize(2048)
        ,
#endif
        m_audioCacheMode(BufferCache)
        , m_audioPendingCacheMode(BufferCache)
        , m_audioConfigured(false)
        , m_audioCacheComplete(false)
        , m_noPromotion(false)
        , m_volume(0)
        , m_balance(0)
        , m_mute(0)
        , m_audioSoftClamp(0)
        , m_frameCacheInvalid(false)
        , m_postFirstEval(false)
        , m_profilingTimer(0)
        , m_newFrame(false)
        , m_defaultOutputGroup(0)
        , m_topologyChanged(false)
        , m_cacheTimingOutput(false)
        , m_evalSlowMedia(false)
        , m_jobDispatcher(nullptr)
    {
        pthread_mutex_init(&m_internalLock, NULL);
        pthread_mutex_init(&m_dispatchLock, NULL);
        pthread_mutex_init(&m_audioInternalLock, NULL);
        pthread_mutex_init(&m_audioFillLock, NULL);

        //
        //  Allow some caching, even when cache is "off"
        //
        size_t minCache = m_minCacheSize;
        m_cacheSizeMap[NeverCache] = minCache;
        m_fbcache.lock();
        m_fbcache.setMemoryUsage(minCache);
        m_fbcache.unlock();

        //
        //  Move this to a later time during init
        //

        const char* maxSizeStr = getenv("TWK_MAX_CACHE_GROUP_SIZE");
        if (maxSizeStr)
            m_maxCacheGroupSize = atoi(maxSizeStr);

        if (getenv("TWK_CACHE_TIMING_OUTPUT"))
            m_cacheTimingOutput = true;

        // clear();
        size_t nthreads = Application::optionValue("evalThreads", size_t(1));
        setNumEvalThreads(nthreads);

        // The number of threads for workItem is based on 25% of the available
        // CPUs with a minimum of 1 and a maximum of 4. It is possible to force
        // this value with the workItemThreads option.
        auto workItemThreads = Application::optionValue<int>(
            "workItemThreads", std::clamp(TwkUtil::SystemInfo::numCPUs() / 4,
                                          (size_t)MIN_WORK_ITEM_THREADS,
                                          (size_t)MAX_WORK_ITEM_THREADS));
        auto jobDispatcher =
            new TwkUtil::JobDispatcher(workItemThreads, "graph job dispatcher");
        jobDispatcher->start();
        m_jobDispatcher = reinterpret_cast<void*>(jobDispatcher);
    }

    IPGraph::~IPGraph()
    {
        if (m_jobDispatcher)
        {
            auto jobDispatcher =
                reinterpret_cast<JobDispatcher*>(m_jobDispatcher);
            jobDispatcher->stop();
            delete jobDispatcher;
        }

        // m_fbcache.showCacheContents();

        setCachingMode(NeverCache, 1, 2, 1, 2, 1, 1, 24.0);
        finishCachingThread();
        finishAudioThread();

        pthread_mutex_destroy(&m_internalLock);
        pthread_mutex_destroy(&m_audioInternalLock);
        pthread_mutex_destroy(&m_audioFillLock);
    }

    void IPGraph::reset(const VideoModules& modules)
    {
        //
        //  Stash the existing primary display group device and reset it
        //  after the graph is clear
        //

        DisplayGroupIPNode* displayGroup = primaryDisplayGroup();
        const VideoDevice* d = displayGroup ? displayGroup->imageDevice() : 0;

        initializeIPTree(modules);

        if (d)
            setPrimaryDisplayGroup(d);
    }

    void IPGraph::clear()
    {
        VideoModules empty;
        initializeIPTree(empty);
    }

    void IPGraph::setDevice(const TwkApp::VideoDevice* controlDevice,
                            const TwkApp::VideoDevice* outputDevice)
    {
        m_controlDevice = controlDevice;
        m_outputDevice = outputDevice;
    }

    void IPGraph::addIsolatedNode(IPNode* node)
    {
        m_isolatedNodes[node->name()] = node;
    }

    void IPGraph::removeIsolatedNode(IPNode* node)
    {
        m_isolatedNodes.erase(node->name());
    }

    void IPGraph::isolateNode(IPNode* node)
    {
        beginGraphEdit();

        //
        //  Isolating a node means removing it from the graph and storing
        //  any relevant graph node state as node properties. This makes
        //  it possible to restore the graph's state completely when the
        //  node is put back in the graph.
        //
        //  This function and restoreIsolatedNode() are for undo/redo
        //

        if (node->group() != NULL)
        {
            TWK_THROW_EXC_STREAM(
                "IPGraph::isolateNode: only top level nodes can be used");
        }

        if (m_defaultViewsMap.find(node->name()) != m_defaultViewsMap.end())
        {
            TWK_THROW_EXC_STREAM(
                "IPGraph::isolateNode: default view cannot be used");
        }

        node->willDeleteSignal()();
        m_nodeWillDeleteSignal(node);

        node->isolate();

        m_nodeDidDeleteSignal(node->name(), node->protocol());

        //
        //  If its the current view node then we need to record that fact
        //

        if (node == viewNode())
        {
            node->createProperty<IntProperty>("__graph.viewNode");
        }

        endGraphEdit();
    }

    IPNode* IPGraph::findNodePossiblyIsolated(const string& name)
    {
        NodeMap::const_iterator i = m_nodeMap.find(name);
        if (i != m_nodeMap.end())
            return i->second;
        i = m_isolatedNodes.find(name);
        if (i != m_isolatedNodes.end())
            return i->second;
        return 0;
    }

    void IPGraph::restoreIsolatedNode(IPNode* node)
    {
        beginGraphEdit();

        //
        //  Try and bit a bit efficient with collecting the properties
        //  since this could be called on 100s of nodes at once
        //

        bool wasViewNode = node->find("__graph.viewNode") != NULL;

        addNode(node);
        node->restore();

        m_viewNodeMap[node->name()] = node;

        m_newNodeSignal(node);

        //
        //  Restore the node's inputs. The order is important
        //

        if (wasViewNode)
            setViewNode(node);

        endGraphEdit();
    }

    void IPGraph::addNode(IPNode* n)
    {
        if (n->graph() != this)
            n->setGraph(this);

        m_topologyChanged = true;
        m_nodeMap[n->name()] = n;

        if (n->hasAudio())
            m_audioSources.insert(n);
    }

    void IPGraph::removeNode(IPNode* n)
    {
        if (n->graph() != this)
            return;

        m_nodeWillRemoveSignal(n);

        m_topologyChanged = true;

        //
        //  Note: The API to this call provides a pointer, but we are looking up
        //  these map items by _name_.  If another node has the same name, we
        //  could remove it by mistake.  So we check the looked-up node's
        //  pointer to be sure.
        //

        NodeMap::iterator i = m_nodeMap.find(n->name());
        if (i != m_nodeMap.end() && i->second == n)
            m_nodeMap.erase(i);

        i = m_viewNodeMap.find(n->name());
        if (i != m_viewNodeMap.end() && i->second == n)
            m_viewNodeMap.erase(i);

        i = m_defaultViewsMap.find(n->name());
        if (i != m_defaultViewsMap.end() && i->second == n)
            m_defaultViewsMap.erase(i);

        if (DisplayGroupIPNode* dnode = dynamic_cast<DisplayGroupIPNode*>(n))
        {
            DisplayGroups::iterator i =
                find(m_displayGroups.begin(), m_displayGroups.end(), dnode);
            if (i != m_displayGroups.end())
            {
                m_displayGroups.erase(i);
            }
        }

        if (n == m_rootNode)
            m_rootNode = 0;

        if (n == m_viewNode)
        {
            m_viewNode = 0;

            if (!m_viewNodeMap.empty())
            {
                setViewNode((*m_viewNodeMap.begin()).second);
            }
        }

        m_frameCacheInvalid = true;

        //
        //  This node is not longer supplying audio, so take it out of list of
        //  audioSources, if we now have no audioSources, we flush audio cache
        //  to make sure we don't play left-over audio.
        //

        bool oldHasAudio = hasAudio();
        m_audioSources.erase(n);
        if (oldHasAudio != hasAudio())
        {
            flushAudioCache();
        }

        //
        //  In addition to removing the graph's references to this node, remove
        //  the node's reference to this graph, otherwise we will try to
        //  "remove" it from the graph again when it is deleted.
        //

        n->setGraph(0);
    }

    void IPGraph::setNumEvalThreads(size_t n)
    {
        finishCachingThread();
        delete m_threadGroup;
        delete m_threadGroupSingle;

        m_threadData.resize(n);

        //
        //  IDs start at 1, because display thread is ID 0
        //
        for (size_t i = 0; i < n; i++)
        {
            m_threadData[i].id = i + 1;
            m_threadData[i].graph = this;
            m_threadData[i].running = false;
            m_threadData[i].mythread = notAThread;
        }

        ThreadGroup::func_vector funcs;
        ThreadGroup::data_vector datas;

        //
        //  Single thread group, ID 1, for caching h264, etc.
        //

        funcs.resize(0);
        datas.resize(0);
        funcs.push_back((ThreadGroup::thread_function)evalThreadTrampoline);
        datas.push_back(&m_threadData[0]);

        m_threadGroupSingle = new ThreadGroup(1, 8, 0, &funcs, &datas);

        //
        //  Other caching threads.
        //
        if (n <= 1)
        {
            m_threadGroup = 0;
            return;
        }

        funcs.resize(0);
        datas.resize(0);

        for (size_t i = 1; i < n; i++)
        {
            funcs.push_back((ThreadGroup::thread_function)evalThreadTrampoline);
            datas.push_back(&m_threadData[i]);
        }

        m_threadGroup = new ThreadGroup(n - 1, 8, 0, &funcs, &datas);
    }

    void IPGraph::initializeIPTree(const VideoModules& modules)
    {
        lockAudioFill();
        finishAudioThread();
        unlockAudioFill();

        m_topologyChanged = true;

        if (m_cacheMode != NeverCache)
        {
            setCachingMode(NeverCache, 1, 2, 1, 2, 1, 1, 24.0);
            finishCachingThread();
        }

        //
        //  Unhook all inputs. This will prevent the need for recursive
        //  deletion -- at least at the top level.
        //

        for (NodeMap::iterator i = m_viewNodeMap.begin();
             i != m_viewNodeMap.end(); ++i)
        {
            IPNode* n = i->second;
            n->willDelete();
            n->disconnectInputs();
        }

        //
        //  Delete all nodes until none are left.
        //

        while (!m_viewNodeMap.empty())
        {
            IPNode* n = m_viewNodeMap.begin()->second;
            delete n;
        }

        m_displayGroups.clear();

        m_rootNode = 0;
        m_viewNode = 0;
        m_defaultOutputGroup = 0;

        m_nodeMap.clear();
        m_viewNodeMap.clear();
        m_defaultViewsMap.clear();

        m_rootNode = newNode("Root", "root");
        m_sessionNode = newNodeOfType<SessionIPNode>("Session", "session");
        m_viewGroupNode =
            newNodeOfType<ViewGroupIPNode>("ViewGroup", "viewGroup");
        m_defaultOutputGroup = newOutputGroup("defaultOutputGroup");
        m_viewNode = 0;

        m_volume = m_viewGroupNode->soundtrackNode()->property<FloatProperty>(
            "audio.volume");
        m_balance = m_viewGroupNode->soundtrackNode()->property<FloatProperty>(
            "audio.balance");
        m_mute = m_viewGroupNode->soundtrackNode()->property<IntProperty>(
            "audio.mute");
        m_audioSoftClamp =
            m_viewGroupNode->soundtrackNode()->property<IntProperty>(
                "audio.softClamp");
        m_audioOffset =
            m_viewGroupNode->soundtrackNode()->property<FloatProperty>(
                "audio.offset");
        m_audioOffset2 =
            m_viewGroupNode->soundtrackNode()->property<FloatProperty>(
                "audio.internalOffset");

        setPhysicalDevicesInternal(modules);

        m_defaultOutputGroup->setInputs1(m_viewGroupNode);
        m_rootNode->appendInput(m_viewGroupNode->waveformNode());
    }

    void IPGraph::setPhysicalDevices(const VideoModules& modules)
    {
        beginGraphEdit();
        setPhysicalDevicesInternal(modules);
        endGraphEdit();
    }

    void IPGraph::setPhysicalDevicesInternal(const VideoModules& modules)
    {

        //
        //  NOTE: this function can be called apart from the
        //  initializeIPTree() case. Because of the way the UI works the
        //  VideoModules can be initialized late.
        //

        for (size_t i = 0; i < m_displayGroups.size(); i++)
        {
            DisplayGroupIPNode* node = m_displayGroups[i];

            if (node == m_defaultOutputGroup)
            {
                if (m_rootNode->isInput(m_defaultOutputGroup))
                {
                    m_rootNode->removeInput(m_defaultOutputGroup);
                }
            }
            else
            {
                node->willDelete();
                node->disconnectInputs();
            }
        }

        for (size_t i = 0; i < m_displayGroups.size(); i++)
        {
            if (m_displayGroups[i] != m_defaultOutputGroup)
            {
                delete m_displayGroups[i];
            }
        }

        m_displayGroups.clear();

        if (modules.empty())
        {
            m_displayGroups.push_back(m_defaultOutputGroup);
        }
        else
        {
            for (size_t i = 0; i < modules.size(); i++)
            {
                const VideoModule* module = modules[i].get();
                const TwkApp::VideoModule::VideoDevices& devices =
                    module->devices();

                for (size_t j = 0; j < devices.size(); j++)
                {
                    //
                    //  Make DisplayGroupIPNode for each physical device and
                    //  get its default value.
                    //

                    TwkApp::VideoDevice* device = devices[j];
                    ostringstream str;
                    str << "displayGroup" << m_displayGroups.size();
                    string name = str.str(); // windows

                    m_displayGroups.push_back(newDisplayGroup(name, device));
                }
            }
        }

        DisplayGroupIPNode* displayGroup = primaryDisplayGroup();
        displayGroup->setInputs1(m_viewGroupNode);

        m_rootNode->appendInput(displayGroup);
    }

    void IPGraph::setPrimaryDisplayGroup(DisplayGroupIPNode* node)
    {
        DisplayGroups::iterator i =
            std::find(m_displayGroups.begin(), m_displayGroups.end(), node);

        //
        //  If already the current display group return
        //

        if (i == m_displayGroups.begin())
            return;

        if (i != m_displayGroups.end())
        {
            IPNode* current = m_displayGroups.front();
            node->setInputs(current->inputs());
            if (m_rootNode->isInput(current))
                m_rootNode->removeInput(current);

            swap(*i, m_displayGroups.front());

            m_rootNode->appendInput(m_displayGroups.front());
        }
    }

    void IPGraph::setRootNodes1(IPNode* node) { m_rootNode->setInputs1(node); }

    void IPGraph::setRootNodes(const IPNodes& nodes)
    {
        m_rootNode->setInputs(nodes);
    }

    void IPGraph::setPrimaryDisplayGroup(const TwkApp::VideoDevice* d)
    {
        //
        //  Look up the DisplayGroupIPNode we made for the physical device
        //  and switch to that.
        //

        if (DisplayGroupIPNode* node =
                findDisplayGroupByDevice(d->physicalDevice()))
        {
            if (DisplayGroupIPNode* d = primaryDisplayGroup())
                d->setOutputVideoDevice(0);
            setPrimaryDisplayGroup(node);
            node->setOutputVideoDevice(d);
            m_primaryDeviceChangedSignal(d);
        }
    }

    void IPGraph::deviceChanged(const VideoDevice* oldDevice,
                                const VideoDevice* newDevice) const
    {
        if (DisplayGroupIPNode* dnode = findDisplayGroupByDevice(oldDevice))
        {
            if (newDevice)
            {
                dnode->setOutputVideoDevice(newDevice);
                dnode->setPhysicalVideoDevice(newDevice->physicalDevice());
                m_deviceChangedSignal(oldDevice, newDevice);
            }
        }
    }

    DisplayGroupIPNode*
    IPGraph::findDisplayGroupByDevice(const TwkApp::VideoDevice* device) const
    {
        for (size_t i = 0; i < m_displayGroups.size(); i++)
        {
            if (m_displayGroups[i]->outputDevice() == device)
            {
                return m_displayGroups[i];
            }
        }

        for (size_t i = 0; i < m_displayGroups.size(); i++)
        {
            if (m_displayGroups[i]->physicalDevice() == device)
            {
                return m_displayGroups[i];
            }
        }

        return 0;
    }

    void IPGraph::addDisplayGroup(DisplayGroupIPNode* node)
    {
        DisplayGroups::iterator i =
            find(m_displayGroups.begin(), m_displayGroups.end(), node);

        if (i == m_displayGroups.end())
            m_displayGroups.push_back(node);
    }

    void IPGraph::disconnectDisplayGroup(DisplayGroupIPNode* node)
    {
        if (!m_rootNode->isInput(node))
            return;
        m_rootNode->removeInput(node);
        node->disconnectInputs();
    }

    void IPGraph::disconnectDisplayGroup(const TwkApp::VideoDevice* device)
    {
        if (DisplayGroupIPNode* node = findDisplayGroupByDevice(device))
        {
            disconnectDisplayGroup(node);
        }
    }

    void IPGraph::connectDisplayGroup(DisplayGroupIPNode* node,
                                      bool atbeginning, bool connectToView)
    {
        if (m_rootNode->isInput(node))
            return;
        if (atbeginning)
            m_rootNode->insertInput(node, 0);
        else
            m_rootNode->appendInput(node);
        if (connectToView)
            node->setInputs1(m_viewGroupNode);
    }

    void IPGraph::connectDisplayGroup(const TwkApp::VideoDevice* device,
                                      bool atbeginning, bool connectToView)
    {
        if (DisplayGroupIPNode* node = findDisplayGroupByDevice(device))
        {
            connectDisplayGroup(node, atbeginning, connectToView);
        }
    }

    void IPGraph::removeDisplayGroup(DisplayGroupIPNode* node)
    {
        for (size_t i = 0; i < m_displayGroups.size(); i++)
        {
            if (m_displayGroups[i] == node)
            {
                m_displayGroups.erase(m_displayGroups.begin() + i);
                if (m_rootNode->isInput(node))
                    m_rootNode->removeInput(node);
                return;
            }
        }
    }

    void IPGraph::removeDisplayGroup(const TwkApp::VideoDevice* device)
    {
        if (DisplayGroupIPNode* node = findDisplayGroupByDevice(device))
        {
            removeDisplayGroup(node);
        }
    }

    IPNode::Context IPGraph::contextForFrame(int frame,
                                             IPNode::ThreadType threadType,
                                             bool stereo) const
    {
        return IPNode::Context(frame, frame, m_fbcache.displayFPS(), 0, 0,
                               threadType, size_t(0), m_fbcache, stereo);
    }

    void IPGraph::beginGraphEdit()
    {
        //
        //  Shutdown all threads to ensure nothing is in the graph
        //

        if (!m_editing)
        {
            if (m_cacheMode != NeverCache)
                finishCachingThread();
            finishAudioThread();
            m_frameCacheInvalid = true;
            m_topologyChanged = false;
        }

        m_editing++;
    }

    void IPGraph::programFlush()
    {
        TwkApp::GenericStringEvent event("graph-program-flush", this, "");
        sendEvent(event);
    }

    void IPGraph::endGraphEdit()
    {
        m_editing--;

        m_graphEditSignal();

        if (m_editing <= 0)
        {
            m_editing = 0;

            if (m_topologyChanged)
            {
                m_audioCache.lock();
                m_audioCache.clear();
                lockAudioInternal();
                m_audioCacheComplete = false;
                unlockAudioInternal();
                m_audioCache.unlock();
            }

            if (m_audioConfigured && m_audioCacheMode == GreedyCache)
            {
                m_audioConfigured = false;
                audioConfigure(m_lastAudioConfiguration);
            }

            if (m_cacheMode != NeverCache)
                redispatchCachingThread();
        }
    }

    void IPGraph::setViewNode(IPNode* viewNode)
    {
        if (m_viewNode == viewNode)
            return;

        beginGraphEdit();

        m_fbcache.lock();
        m_fbcache.clear();
        m_fbcache.unlock();

        m_topologyChanged = true;

        //
        //  Set the top display node's input to the view node
        //

        if (m_viewGroupNode)
        {
            m_viewGroupNode->disconnectInputs();
            m_viewGroupNode->setInputs1(viewNode);
        }

        m_viewNode = viewNode;

        m_viewNodeChangedSignal(viewNode);

        endGraphEdit();
    }

    void IPGraph::deleteNode(IPNode* node)
    {
        m_nodeWillDeleteSignal(node);

        vector<string> info(4);
        ostringstream str;
        str << node->protocolVersion();
        info[0] = node->name();
        info[1] = node->protocol();
        info[2] = str.str();
        info[3] = node->group() ? node->group()->name() : "";

        TwkApp::GenericStringEvent bevent("graph-before-delete-node", this,
                                          info[0]);
        bevent.setStringContentVector(info);
        sendEvent(bevent);

        node->willDelete();
        node->disconnectInputs();
        delete node;
        m_frameCacheInvalid = true;
        m_topologyChanged = true;

        TwkApp::GenericStringEvent aevent("graph-after-delete-node", this,
                                          info[0]);
        aevent.setStringContentVector(info);
        sendEvent(aevent);

        m_nodeDidDeleteSignal(info[0], info[1]);
    }

    TwkContainer::PropertyContainer* IPGraph::sparseContainer(IPNode* node)
    {
        if (IPNode* genericNode = newNode(node->protocol(), node->name()))
        {
            PropertyContainer* pc = node->shallowDiffCopy(genericNode);
            deleteNode(genericNode);
            return pc;
        }

        cerr << "ERROR: couldn't make " << node->name() << " of type \""
             << node->protocol() << "\"" << endl;

        return 0;
    }

#if 0
void
IPGraph::findNodesByAbstractPath(int frame,
                                 NodeVector& nodes,
                                 const string& typeName)
{
    //
    //  This function will find any nodes in the eval path with one of
    //  the following syntax
    //
}
#endif

    void IPGraph::findNodesByPattern(int frame, NodeVector& nodes,
                                     const std::string& pattern)
    {
        if (pattern[0] == '#')
        {
            string typeName = pattern.substr(1, pattern.size() - 1);
            findNodesByTypeName(frame, nodes, typeName);
        }
        else
        {
            if (IPNode* node = findNode(pattern))
            {
                nodes.resize(1);
                nodes[0] = node;
            }
        }
    }

    void IPGraph::findNodesByTypeName(int frame, NodeVector& nodes,
                                      const string& typeName) const
    {
        if (typeName == "View")
        {
            nodes.push_back(viewNode());
        }
        else if (typeName == "Session")
        {
            nodes.push_back(sessionNode());
        }
        else
        {
            IPNode::MetaEvalInfoVector infos;
            IPNode::MetaEvalClosestByTypeName closest(infos, typeName);
            m_rootNode->metaEvaluate(contextForFrame(frame), closest);

            nodes.resize(infos.size());
            for (size_t i = 0; i < infos.size(); i++)
                nodes[i] = infos[i].node;
        }
    }

    void IPGraph::findNodesByTypeName(NodeVector& nodes,
                                      const string& typeName) const
    {
        if (typeName == "View")
        {
            nodes.push_back(viewNode());
        }
        else if (typeName == "Session")
        {
            nodes.push_back(sessionNode());
        }
        else
        {
            for (NodeMap::const_iterator i = m_nodeMap.begin();
                 i != m_nodeMap.end(); ++i)
            {
                IPNode* node = i->second;

                if (node->protocol() == typeName)
                {
                    nodes.push_back(node);
                }
            }
        }
    }

    void IPGraph::findNodesByTypeNameWithProperty(NodeVector& nodes,
                                                  const string& typeName,
                                                  const string& propName)
    {
        NodeVector temp;
        findNodesByTypeName(temp, typeName);

        for (size_t i = 0; i < temp.size(); i++)
        {
            if (temp[i]->find(propName))
                nodes.push_back(temp[i]);
        }
    }

    void IPGraph::findProperty(int frame, PropertyVector& props,
                               const string& name)
    {
        vector<string> buffer;
        algorithm::split(buffer, name, is_any_of("."), token_compress_on);

        if (buffer.size() < 2)
            return;
        IPNode* node = 0;
        IPNode* rootNode = m_rootNode;

        vector<string> prefixBuffer;
        algorithm::split(prefixBuffer, buffer[0], is_any_of("/"),
                         token_compress_on);

        if (prefixBuffer.size() > 1)
        {
            //
            //  This only handles the first prefix to the path. We could
            //  go further:  A/#B/#C.x.y.z could be A's member of type B
            //  with a member C that has the property x.y.z
            //

            NodeMap::const_iterator i = m_nodeMap.find(prefixBuffer[0]);
            if (i != m_nodeMap.end())
                rootNode = i->second;
        }

        string head = prefixBuffer.back();
        const char qualifier = head[0];
        buffer.erase(buffer.begin());
        string rest = algorithm::join(buffer, ".");

        if (qualifier == '#')
        {
            if (head == "#View")
            {
                if (Property* p = viewNode()->find(rest))
                {
                    props.push_back(p);
                }
            }
            else if (head == "#Session")
            {
                if (Property* p = sessionNode()->find(rest))
                {
                    props.push_back(p);
                }
            }
            else
            {
                IPNode::MetaEvalInfoVector infos;
                IPNode::MetaEvalClosestByTypeName closest(
                    infos, head.substr(1, head.size() - 1));
                rootNode->metaEvaluate(contextForFrame(frame), closest);

                if (!infos.empty())
                {
                    //
                    //  Ensure that set of returned properties has not
                    //  duplicates.
                    //
                    set<Property*> propSet;

                    for (int i = 0; i < infos.size(); i++)
                    {
                        node = infos[i].node;

                        if (Property* p = node->find(rest))
                        {
                            propSet.insert(p);
                        }
                    }
                    props.insert(props.end(), propSet.begin(), propSet.end());
                }
            }
        }
        else if (qualifier == '@')
        {
            IPNode::MetaEvalInfoVector infos;
            IPNode::MetaEvalFirstClosestByTypeName closest(
                infos, head.substr(1, head.size() - 1));
            rootNode->metaEvaluate(contextForFrame(frame), closest);

            if (!infos.empty())
            {
                node = infos.front().node;
                if (Property* p = node->find(rest))
                    props.push_back(p);
            }
        }
        else if (qualifier == ':')
        {
            string typeName = head.substr(1, head.size() - 1);

            if (const NodeDefinition* def = m_nodeManager->definition(typeName))
            {
                if (const Property* p = def->find(rest))
                {
                    props.push_back((Property*)p);
                }
            }
        }
        else
        {
            NodeMap::const_iterator i = m_nodeMap.find(head);

            if (i != m_nodeMap.end())
            {
                node = i->second;

                if (Property* p = node->find(rest))
                {
                    props.push_back(p);
                }
            }
        }
    }

    void IPGraph::setCacheNodesActive(bool b)
    {
        for (NodeMap::iterator i = m_nodeMap.begin(); i != m_nodeMap.end(); ++i)
        {
            if (CacheIPNode* n = dynamic_cast<CacheIPNode*>((*i).second))
            {
                n->setActive(b);
            }
        }
    }

    string IPGraph::canonicalNodeName(const std::string& nodeName) const
    {
        string cname = nodeName;
        string::size_type i = string::npos;
        size_t s = cname.size();

        for (i = 0; i < s; i++)
        {
            char c = cname[i];

            if ((c < 'a' || c > 'z') && (c < 'A' || c > 'Z')
                && (c < '0' || c > '9') && (c != '_'))
            {
                break;
            }
        }

        if (i != s)
            cname.erase(i, s);
        return cname;
    }

    IPNode* IPGraph::findNode(const std::string& name) const
    {
        NodeMap::const_iterator i = m_nodeMap.find(canonicalNodeName(name));

        if (i != m_nodeMap.end())
        {
            return i->second;
        }

        return 0;
    }

    IPNode* IPGraph::findNodeByUIName(const std::string& inname) const
    {
        string name = canonicalNodeName(inname);

        for (NodeMap::const_iterator i = m_nodeMap.begin();
             i != m_nodeMap.end(); ++i)
        {
            IPNode* node = i->second;
            if (node->uiName() == name)
                return node;
        }

        return 0;
    }

    IPNode* IPGraph::findNodeAssociatedWith(IPNode* source,
                                            const std::string& typeName) const
    {
        const IPNode::IPNodes& outputs = source->outputs();

        for (size_t i = 0; i < outputs.size(); i++)
        {
            IPNode* node = outputs[i];

            if (node->protocol() == typeName)
            {
                return node;
            }
            else if (GroupIPNode* group = dynamic_cast<GroupIPNode*>(node))
            {
                const IPNode::IPNodeSet& members = group->members();

                for (IPNode::IPNodeSet::const_iterator i = members.begin();
                     i != members.end(); ++i)
                {
                    IPNode* mnode = *i;

                    if (mnode->inputs().empty())
                    {
                        if (IPNode* n = findNodeAssociatedWith(mnode, typeName))
                        {
                            return n;
                        }
                    }
                }
                //
                //  We have checked the contents of the group node "group", but
                //  the group node _itself_ may have a output, so check that too
                //  ...
                //
                if (IPNode* n = findNodeAssociatedWith(group, typeName))
                {
                    return n;
                }
            }
            else
            {
                if (IPNode* n = findNodeAssociatedWith(node, typeName))
                {
                    return n;
                }
            }
        }

        return 0;
    }

    //----------------------------------------------------------------------

    void IPGraph::setCachingMode(CachingMode mode, int inframe, int outframe,
                                 int minFrame, int maxFrame, int inc, int frame,
                                 float fps)
    {
        //
        //  UTILITY_CACHING
        //  Don't halt caching threads and redispatch unless we really
        //  have to.
        //
        size_t memUsage = m_cacheSizeMap[mode];
        bool doDispatch = true;
        if (m_editing || isMediaLoading()
            || (m_fbcache.minFrame() == minFrame
                && m_fbcache.maxFrame() == maxFrame
                && m_fbcache.inFrame() == inframe
                && m_fbcache.outFrame() == outframe
                && m_fbcache.capacity() == memUsage && m_cacheMode == mode
                && isCacheThreadRunning()))
        {
            doDispatch = false;
        }

        DBL(DB_DISP, "setCachingMode mode "
                         << mode << " editing " << m_editing
                         << " isMediaLoading " << isMediaLoading()
                         << " doDispatch " << doDispatch << " min " << minFrame
                         << " max " << maxFrame << " in " << inframe << " out "
                         << outframe << " thread running "
                         << isCacheThreadRunning());

        if (doDispatch && m_cacheMode != NeverCache)
            finishCachingThread();

        bool modeChanged = (m_cacheMode != mode);
        m_cacheMode = mode;

        IPNode::GraphConfiguration gconfig(minFrame, maxFrame, inframe,
                                           outframe, fps);

        switch (mode)
        {
        case NeverCache:
            m_fbcache.lock();
            if (memUsage != 0)
                m_fbcache.setMemoryUsage(memUsage);
            m_fbcache.clearAllButFrame(frame);
            m_fbcache.setFreeMode(FBCache::ConservativeFreeMode);
            m_fbcache.setInOutFrames(inframe, outframe, minFrame, maxFrame);
            m_fbcache.unlock();
            break;

        case BufferCache:
            m_fbcache.lock();
            if (memUsage != 0)
                m_fbcache.setMemoryUsage(memUsage);
            m_fbcache.setFreeMode(FBCache::GreedyFreeMode);
            m_fbcache.unlock();
            if (doDispatch)
            {
                dispatchCachingThread(inframe, outframe, minFrame, maxFrame,
                                      inc, frame, fps, modeChanged);
            }
            else
            {
                TWK_CACHE_LOCK(m_fbcache, "inc");
                m_fbcache.setInOutFrames(inframe, outframe, minFrame, maxFrame);
                if (m_fbcache.displayInc() != inc)
                {
                    m_fbcache.setDisplayInc(inc);
                    awakenAllCachingThreads();
                }
                TWK_CACHE_UNLOCK(m_fbcache, "inc");
            }
            break;

        case GreedyCache:
            m_fbcache.lock();
            if (memUsage != 0)
                m_fbcache.setMemoryUsage(memUsage);
            m_fbcache.setFreeMode(FBCache::ConservativeFreeMode);
            m_fbcache.unlock();
            if (doDispatch)
            {
                dispatchCachingThread(inframe, outframe, minFrame, maxFrame,
                                      inc, frame, fps, modeChanged);
            }
            else
            {
                TWK_CACHE_LOCK(m_fbcache, "inc");
                m_fbcache.setInOutFrames(inframe, outframe, minFrame, maxFrame);
                if (m_fbcache.displayInc() != inc)
                {
                    m_fbcache.setDisplayInc(inc);
                    awakenAllCachingThreads();
                }
                TWK_CACHE_UNLOCK(m_fbcache, "inc");
            }
            break;
        }

        if (m_rootNode)
            m_rootNode->propagateGraphConfigToInputs(gconfig);
    }

    void IPGraph::lockInternal() const
    {
        //
        //  NOTE: m_internalLock is mutable
        //

        pthread_mutex_lock(&m_internalLock);
    }

    void IPGraph::unlockInternal() const
    {
        //
        //  NOTE: m_internalLock is mutable
        //

        pthread_mutex_unlock(&m_internalLock);
    }

    void IPGraph::finishCachingThreadASync()
    {
        lockInternal();
        m_cacheStop = true;
        unlockInternal();
    }

    void IPGraph::setCacheModeSize(CachingMode mode, size_t size)
    {
        DBL(DB_SIZE, "IPGraph::setCacheModeSize mode "
                         << mode << " size " << size << ", was "
                         << m_cacheSizeMap[mode] << ", used "
                         << m_fbcache.used());
        m_cacheSizeMap[mode] = size;

        if (m_cacheMode == mode)
        {
            TWK_CACHE_LOCK(m_fbcache, "setCacheModeSize");

            m_fbcache.setMemoryUsage(size);
            m_fbcache.emergencyFree();
            awakenAllCachingThreads();

            TWK_CACHE_UNLOCK(m_fbcache, "setCacheModeSize");
        }
    }

    bool IPGraph::isCacheThreadRunning() const
    {
        bool running = false;
        lockInternal();

        for (int i = 0; i < m_threadData.size(); i++)
        {
            if (m_threadData[i].running)
            {
                running = true;
                break;
            }
        }

        unlockInternal();
        return running;
    }

    bool IPGraph::cacheThreadContinue()
    {
        lockInternal();
        bool stop = m_cacheStop;
        unlockInternal();
        return !stop;
    }

    //----------------------------------------------------------------------
    //
    //  Remove all cache elements for frame
    //

    void IPGraph::flushRange(int start, int end)
    {
        if (isCacheThreadRunning())
            finishCachingThread();
        if (!m_rootNode)
            return;

        TWK_CACHE_LOCK(m_fbcache, "flush");

        for (int frame = start; frame <= end; frame++)
        {
            IPNode::Context context =
                contextForFrame(frame, IPNode::CacheLockedThread, false);

            try
            {
                m_rootNode->propagateFlushToInputs(context);
            }
            catch (...)
            {
            }
        }

        m_fbcache.garbageCollect();

        TWK_CACHE_UNLOCK(m_fbcache, "flush");
    }

    //----------------------------------------------------------------------
    //
    //  This is the control thread's function
    //

    void IPGraph::checkInImage(IPImage* img, bool updateDisplayframe, int frame)
    {
        TWK_CACHE_LOCK(m_fbcache, "");

        m_fbcache.checkInAndDelete(img);
        if (updateDisplayframe)
        {
            m_fbcache.setDisplayFrame(frame);
            awakenAllCachingThreads();
        }

        TWK_CACHE_UNLOCK(m_fbcache, "");
    }

    //----------------------------------------------------------------------
    //
    //  This is the control thread's function
    //

#define PROFILE_SAMPLE(X)                                  \
    if (needsProfilingSamples())                           \
    {                                                      \
        EvalProfilingRecord& r = currentProfilingSample(); \
        r.X = profilingElapsedTime();                      \
    }

    void IPGraph::postEvaluation()
    {
        if (m_newFrame)
        {
            PROFILE_SAMPLE(awakenThreadsStart);
            TWK_CACHE_LOCK(m_fbcache, "");
            awakenAllCachingThreads();
            TWK_CACHE_UNLOCK(m_fbcache, "");
            PROFILE_SAMPLE(awakenThreadsEnd);
        }
    }

    IPGraph::EvalResult IPGraph::evaluateAtFrame(int frame, bool forDisplay)
    {
        if (!m_rootNode)
            return make_pair(EvalError, (IPImage*)0);

        // DB ("evaluateAtFrame " << frame << " forDisp " << forDisplay);

        //
        //  Clean up previous evaluation and update cache about the frame
        //  we're about to evaluate. Unlock and delete the images from the
        //  last display evaluation.
        //

        EvalStatus status = EvalNormal;
        IPImage* img = 0;

        //
        //  Set the current frame in the cache and find out if its already
        //  cached
        //

        PROFILE_SAMPLE(cacheTestStart);

        PROFILE_SAMPLE(cacheTestLockStart);
        TWK_CACHE_LOCK(m_fbcache, "");
        PROFILE_SAMPLE(cacheTestLockEnd);

        //
        //  If we're evaluating for display and the frame is not the one the
        //  cache thinks we're currently viewing, then set a flag that will
        //  awaken caching threads later.
        //
        PROFILE_SAMPLE(setDisplayFrameStart);
        if (forDisplay)
        {
            m_newFrame = frame != m_fbcache.displayFrame();

            //
            //  Ensure that caching threads have the right idea of the current
            //  display frame.
            //
            if (m_newFrame)
            {
                m_fbcache.setDisplayFrame(frame);
            }
        }
        PROFILE_SAMPLE(setDisplayFrameEnd);

        PROFILE_SAMPLE(frameCachedTestStart);
        bool isCached = m_fbcache.isFrameCached(frame);
        PROFILE_SAMPLE(frameCachedTestEnd);

        TWK_CACHE_UNLOCK(m_fbcache, "");
        // DB ("evaluateAtFrame f " << frame << " forDisplay " << forDisplay <<
        // " isCached " << isCached);
        PROFILE_SAMPLE(cacheTestEnd);

        if (isCached)
        {
            //
            //  Evaluate: the cached frames will be picked up
            //
            //  NOTE: DisplayNoEvalThread means that this thread is NOT
            //  allowed to evaluate past the first CacheIPNode. It should
            //  find the fb items in the cache. The DisplayNoEvalThread
            //  thread context evaluates with the cache already locked.
            //

            bool cacheMiss = false;

            try
            {
                IPNode::ThreadType t = m_cacheMode == NeverCache
                                           ? IPNode::DisplayMaybeEvalThread
                                           : IPNode::DisplayNoEvalThread;

                PROFILE_SAMPLE(evalInternalStart);

                img = evaluate(frame, t);

                PROFILE_SAMPLE(evalInternalEnd);
            }
            catch (CacheMissExc& exc)
            {
                PROFILE_SAMPLE(evalInternalEnd);

                //
                //  We can get here a number of different ways:
                //
                //      1) A source had a layer added since last cached
                //
                //      2) A node is introducing a new image for this frame
                //         that we haven't seen before but expect to be in
                //         the cache. (Similar to #1)
                //
                //  If we're in one of the caching modes we need to
                //  restart the cache thread again if its stoped.
                //
                //  By detecting this at the latest possible time, we can
                //  avoid doing work to repair the cache until its
                //  actually needed (which could be never).
                //

                try
                {
                    img = evaluate(frame, IPNode::DisplayCacheEvalThread);
                    status = EvalNormal;
                }
                catch (std::exception& exc)
                {
                    cerr << "ERROR: after cache miss: " << exc.what() << endl;
                    status = EvalError;
                }

                cacheMiss = true;
            }
            catch (CacheFullExc& exc)
            {
                PROFILE_SAMPLE(evalInternalEnd);

                try
                {
                    img = evaluate(frame, IPNode::DisplayEvalThread);
                }
                catch (...)
                {
                }

                return make_pair(EvalBufferNeedsRefill, img);
            }
            catch (std::exception& exc)
            {
                PROFILE_SAMPLE(evalInternalEnd);
                cerr << "ERROR: display no-eval caught: " << exc.what() << endl;
                status = EvalError;
            }
            catch (...)
            {
                PROFILE_SAMPLE(evalInternalEnd);
                cerr << "ERROR: display no-eval caught something" << endl;
                status = EvalError;
            }

            //
            //  Hmm. We need to check if the below is sometimes slow seems like
            //  the common case (there's a frame or to near this the display
            //  frame that should be freed) could be made much faster.
            //
            //  Modifing this to only clear (and lock) if we actually need the
            //  space.
            //
            if (m_cacheMode == NeverCache && forDisplay)
            {
                if (m_fbcache.used() > m_fbcache.capacity())
                {
                    TWK_CACHE_LOCK(m_fbcache, "");
                    m_fbcache.clearAllButFrame(frame);
                    TWK_CACHE_UNLOCK(m_fbcache, "");
                }
            }
        }
        else if (m_cacheMode == BufferCache)
        {
            //
            //  We've overrun the lookahead cache.  If the requested wait time
            //  is > 0.0 (IE if the user expects us to stop playback at this
            //  point and cache more frames), we'll pause playback (this happens
            //  in the Session) and restart when we've waited long enough.  But
            //  if the user has asked to not pause, we'll just eval/cache this
            //  frame and go on.
            //
            bool willPause = (m_maxBufferedWaitTime > 0.0);

            if (willPause)
                finishCachingThread();

            try
            {
                DBL(DB_CACHE,
                    "overrun: evaling in display thread, frame " << frame);
                img = evaluate(frame, IPNode::DisplayCacheEvalThread);
                if (willPause)
                    status = EvalBufferNeedsRefill;
            }
            catch (...)
            {
                status = EvalError;
            }

            if (willPause)
                redispatchCachingThread();
        }
        else if (m_cacheMode == NeverCache)
        {
            //
            //  This is what happens if the cache is OFF. In this case just
            //  evaluate and allow caching of the current frame.
            //
            //  The current frame is cached because it will probably be asked
            //  for again.  Especially in the case when we're sitting on a frame
            //  and doing something interactive, we don't want to be reading the
            //  image from disk every render.
            //

            try
            {
                img = evaluate(frame, IPNode::DisplayCacheEvalThread);
            }
            catch (std::exception& exc)
            {
                cerr << "ERROR: display pass-through caught: " << exc.what()
                     << endl;
                status = EvalError;
            }

            if (forDisplay && m_fbcache.used() > m_fbcache.capacity())
            {
                TWK_CACHE_LOCK(m_fbcache, "");
                if (forDisplay)
                    m_fbcache.clearAllButFrame(frame);
                TWK_CACHE_UNLOCK(m_fbcache, "");
            }
        }
        else
        {
#if 0
        TwkFB::FrameBuffer* fb = new TwkFB::FrameBuffer(4, 4, 3, TwkFB::FrameBuffer::UCHAR);
        fb->newAttribute("RequestedFrameLoading", 1.0f);
        memset(fb->pixels<unsigned char>(), 0, fb->allocSize());

        img = new IPImage(fb);
#endif

#if 1
            try
            {
                img = evaluate(frame, IPNode::DisplayEvalThread);
            }
            catch (std::exception& exc)
            {
                cerr << "ERROR: display eval caught: " << exc.what() << endl;
            }
#endif
        }

        if (!m_postFirstEval)
        {
            m_postFirstEval = true;

            // No need to keep the frameCacheInvalid flag since we are going to
            // initialize it here. Otherwise we will waste time honoring this
            // flag by flushing the cache at the end of the first eval.
            m_frameCacheInvalid = false;

            if (m_cacheMode != NeverCache)
            {
                //  awakenAllCachingThreads();
                //  redispatchCachingThread();
                IPGraph::setCachingMode(
                    m_cacheMode, m_fbcache.inFrame(), m_fbcache.outFrame(),
                    m_fbcache.minFrame(), m_fbcache.maxFrame(),
                    m_fbcache.displayInc(), m_fbcache.displayFrame(),
                    m_fbcache.displayFPS());
            }
        }

        return make_pair(status, img);
    }

    void IPGraph::promoteFBsInFrameRange(int beg, int end, int inc,
                                         TwkUtil::Timer t)
    {
        if (m_noPromotion)
            return;

        int first = (inc > 0) ? beg : end;
        int last = (inc > 0) ? end : beg;
        DBL(DB_PROMOTE, "promoteFBsInFrameRange beg "
                            << beg << " end " << end << " inc " << inc
                            << " first " << first << " last " << last
                            << " trashCount " << m_fbcache.trashCount()
                            << " elapsed " << t.elapsed());

        if (!m_rootNode)
            return;

        int count = 0, promoteCount = 0;
        for (int f = first; f != last + inc; f += inc)
        {
            //
            //  All this is an optimization, so if it takes too long,
            //  abort.
            //
            if (0 == ++count % 100 && t.elapsed() > 0.1)
            {
                DBL(DB_PROMOTE, "promoteFBsInFrameRange out of time");
                break;
            }

            //
            //  Don't bother if there's nothing in the low-level cache
            //  that can be reused.
            //
            if (m_fbcache.trashCount() <= 0)
            {
                DBL(DB_PROMOTE, "promoteFBsInFrameRange TrashCan empty");
                break;
            }

            IPImageID* idTree;
            bool ok = true;
            try
            {
                IPNode::Context context(f, f, m_fbcache.displayFPS(), 0, 0,
                                        IPNode::DisplayNoEvalThread, 0,
                                        m_fbcache, false);

                idTree = m_rootNode->evaluateIdentifier(context);
            }
            catch (std::exception& exc)
            {
                /*
                cerr << "ERROR: while evaluateIdentifier in
                IPGraph::promoteFBsInFrameRange"
                    << endl
                    << "CAUGHT: " << exc.what()
                    << endl;
                */
                ok = false;
                break;
            }

            if (ok)
            {
                TWK_CACHE_LOCK(m_fbcache, "promoting");
                bool promoted = m_fbcache.promoteFrame(f, idTree);
                TWK_CACHE_UNLOCK(m_fbcache, "promoting");
                if (promoted)
                    ++promoteCount;
            }

            //
            //  Make sure cache stats reflect the frames we've promoted.
            //
            TWK_CACHE_LOCK(m_fbcache, "promoting");
            m_fbcache.setCacheStatsDirty();
            TWK_CACHE_UNLOCK(m_fbcache, "promoting");

            // Free memory returned by idTree.
            // Memory is allocated by the call evaluateIdentifier.
            // e.g. IPFileSourceNode::evaluateIdentifier().
            // And not free-ing this memory was causing leaks.
            if (idTree)
            {
                delete idTree;
            }
        }

        DBL(DB_PROMOTE,
            "tested count " << count << " promoteCount " << promoteCount);
    }

    void IPGraph::dispatchCachingThread(int inframe, int outframe, int minFrame,
                                        int maxFrame, int inc, int frame,
                                        float fps, bool modeChanged)
    {
        if (m_cacheMode == NeverCache || m_editing || isMediaLoading())
            return;
        if (!m_rootNode)
            return;
        if (!m_postFirstEval)
            return;

        LockObject dl(m_dispatchLock);

        TWK_CACHE_LOCK(m_fbcache, "");
        if (m_fbcache.displayFrame() == NAF)
        {
            m_fbcache.setDisplayFrame(frame);
        }
        m_fbcache.setDisplayInc(inc);
        m_fbcache.setDisplayFPS(fps);
        m_fbcache.setInOutFrames(inframe, outframe, minFrame, maxFrame);
        int cacheFrame = m_fbcache.displayFrame();
        int otherframe = inc > 0 ? inframe : outframe - 1;
        m_fbcache.setCacheFrame(m_cacheMode == BufferCache ? cacheFrame
                                                           : otherframe);
        m_fbcache.setCacheWrapFrame(otherframe);

        if (m_fbcache.used() > m_fbcache.capacity())
            m_fbcache.emergencyFree();

        DBL(DB_DISP, "dispatching, over " << m_fbcache.overflowing() << " min "
                                          << minFrame << " max " << maxFrame
                                          << " in " << inframe << " out "
                                          << outframe << " cacheInvalid "
                                          << m_frameCacheInvalid);
        TWK_CACHE_UNLOCK(m_fbcache, "");

        lockInternal();
        m_cacheStop = false;
        unlockInternal();

        static IPNode* savedViewNode = 0;
        if (modeChanged || m_viewNode != savedViewNode || m_frameCacheInvalid)
        {
            DBL(DB_PROMOTE, "dispatchCachingThread viewNode "
                                << m_viewNode->name() << " modeChanged "
                                << modeChanged << " cacheFrame " << cacheFrame
                                << " minFrame " << minFrame << " maxFrame "
                                << maxFrame << " cache invalid "
                                << m_frameCacheInvalid);

            //
            //  Caching should be stopped at this point, but if not, we need to
            //  stop it before we modify these data structures.
            //

            finishCachingThread();

            if (m_frameCacheInvalid)
            {
                TWK_CACHE_LOCK(m_fbcache, "");
                m_fbcache.clear();
                TWK_CACHE_UNLOCK(m_fbcache, "");
            }

            savedViewNode = m_viewNode;

            if (cacheFrame < minFrame)
                cacheFrame = minFrame;
            if (cacheFrame > maxFrame - 1)
                cacheFrame = maxFrame - 1;

            TwkUtil::Timer t;
            t.start();
            if (m_cacheMode == BufferCache)
            {
                if (inc > 0)
                {
                    promoteFBsInFrameRange(cacheFrame, maxFrame - 1, 1, t);
                    promoteFBsInFrameRange(minFrame, cacheFrame, -1, t);
                }
                else
                {
                    promoteFBsInFrameRange(minFrame, cacheFrame, -1, t);
                    promoteFBsInFrameRange(cacheFrame, maxFrame - 1, 1, t);
                }
            }
            else if (m_cacheMode == GreedyCache)
            {
                promoteFBsInFrameRange(minFrame, maxFrame - 1, 1, t);
            }
            m_frameCacheInvalid = false;
        }

        dispatchCachingThreadsSafely();
    }

    void IPGraph::redispatchCachingThread()
    {
        if (m_cacheMode == NeverCache || m_editing || isMediaLoading())
            return;

        HOP_PROF_FUNC();

        LockObject dl(m_dispatchLock);

        TWK_CACHE_LOCK(m_fbcache, "");
        int inframe = m_fbcache.inFrame();
        int outframe = m_fbcache.outFrame();
        int inc = m_fbcache.displayInc();
        int cacheFrame = m_fbcache.displayFrame();
        int otherframe = inc > 0 ? inframe : outframe - 1;
        m_fbcache.setCacheFrame(m_cacheMode == BufferCache ? cacheFrame
                                                           : otherframe);
        m_fbcache.setCacheWrapFrame(otherframe);

        if (m_fbcache.used() > m_fbcache.capacity())
            m_fbcache.emergencyFree();

        DBL(DB_DISP, "redispatching, over " << m_fbcache.overflowing());
        TWK_CACHE_UNLOCK(m_fbcache, "");

        lockInternal();
        m_cacheStop = false;
        unlockInternal();

        dispatchCachingThreadsSafely();
    }

    void IPGraph::dispatchCachingThreadsSafely()
    {
        HOP_PROF_FUNC();

        lockInternal();

        for (int i = 0; i < m_threadData.size(); i++)
        {
            if (m_threadData[i].running)
            {
                unlockInternal();
                return;
            }
        }

        m_threadGroupSingle->dispatch(0, 0);

        if (!m_evalSlowMedia)
        {
            for (size_t i = 1; i < m_threadData.size(); i++)
            {
                m_threadGroup->dispatch(0, 0);
            }
        }

        unlockInternal();
    }

    void IPGraph::finishCachingThread()
    {
        if (!m_rootNode)
            return;
        DBL(DB_DISP, "finishCachingThread start");
        finishCachingThreadASync();
        if (m_threadGroup)
            m_threadGroup->control_wait();
        if (m_threadGroupSingle)
            m_threadGroupSingle->control_wait();
        lockInternal();
        m_cacheStop = false;
        unlockInternal();
        TWK_CACHE_LOCK(m_fbcache, "finishCachingThread");
        m_fbcache.garbageCollect(true);
        TWK_CACHE_UNLOCK(m_fbcache, "finishCachingThread");
        DBL(DB_DISP, "finishCachingThread end");
    }

    void IPGraph::awakenAllCachingThreads()
    {
        if (m_editing || isMediaLoading() || m_cacheMode == NeverCache)
            return;
        if (m_fbcache.utilityStateChanged())
        {
            LockObject dl(m_dispatchLock, true);
            if (dl.locked())
            {
                //
                //  If not locked, we were unable to acquire the lock, so a
                //  dispatch is in progress, and there is no need to awaken
                //  caching threads.
                //

                DBL(DB_DISP,
                    "IPGraph::awakenAllCachingThreads workers arise !");
                m_fbcache.resetUtilityState();
                if (m_threadGroupSingle)
                    m_threadGroupSingle->awaken_all_workers();

                if (m_threadGroup && !m_evalSlowMedia)
                    m_threadGroup->awaken_all_workers();
            }
        }
    }

    //----------------------------------------------------------------------
    //
    //  Called by Evaluation Thread
    //

    IPGraph::TestEvalResult
    IPGraph::testEvaluate(int frame, IPNode::ThreadType thread, size_t n)
    {
        IPGraph::TestEvalResult result;

        if (m_rootNode)
        {
            IPNode::Context context(frame, frame, m_fbcache.displayFPS(), 0, 0,
                                    thread, n, m_fbcache, false);

            m_rootNode->testEvaluate(context, result);
        }

        return result;
    }

    IPImage* IPGraph::evaluate(int frame, IPNode::ThreadType thread, size_t n)
    {
        HOP_ZONE(HOP_ZONE_COLOR_8);
        HOP_PROF_FUNC();

        if (m_rootNode)
        {
            IPNode::Context context(frame, frame, m_fbcache.displayFPS(), 0, 0,
                                    thread, n, m_fbcache, false);

            IPImage* img = m_rootNode->evaluate(context);

            //
            //  The graphID can only be compute *after* the root image is
            //  assembled. This is because each IPImage is identified by
            //  an evaluation path to it. Since the IPImage tree is
            //  constructed leaves first we don't know any paths until the
            //  root is constucted.
            //
            //  The graphID is *not* the same as the renderID. The
            //  renderID incorporates *all* of the parameter values
            //  necessary to create the pixels. The renderID really only
            //  needs to incorporate IPImages between the IPImage in
            //  question and the leaves that contributed to it. The
            //  graphID on the other hand uniquely identifies an
            //  evaluation path that results in an IPImage (not the actual
            //  pixel contents which will probably change per frame). The
            //  graphID is relative to a given Shader::Program. So the
            //  same graphID (evaluation path) may be used in a different
            //  Shader::Program.
            //
            //  Similarily, the global IPImage transforms can only be
            //  computed after the entire tree is assembled because the
            //  leaf transforms depend on the root.
            //

            if (img)
            {
                img->computeGraphIDs();
                img->computeMatrices(m_controlDevice, m_outputDevice);
                img->assembleAuxFrameBuffers();
                img->computeRenderIDRecursive();

                if (m_debugTreeOutput && thread >= IPNode::DisplayNoEvalThread
                    && thread <= IPNode::DisplayThreadMarker)
                {
                    cout << endl;
                    printTreeStdout(img);
                }
            }

            return img;
        }
        else
        {
            return 0;
        }
    }

    void IPGraph::stopCachingInternal()
    {
        lockInternal();
        m_cacheStop = true;
        unlockInternal();
    }

    int IPGraph::getMaxGroupSize(bool slowMedia) const
    {
        if (slowMedia)
        {
            const int defaultSlowMaxGroupSize = 20;
            return (m_maxCacheGroupSize != 0) ? m_maxCacheGroupSize
                                              : defaultSlowMaxGroupSize;
        }
        else
        {
            const int defaultFastMaxGroupSize = 1;
            return (m_maxCacheGroupSize != 0) ? m_maxCacheGroupSize
                                              : defaultFastMaxGroupSize;
        }
    }

    void IPGraph::evalThreadMain(EvalThreadData* threadData)
    {
        const size_t nthreads = m_threadData.size();
        const int id = int(threadData->id);
        size_t initialCapacity = m_fbcache.capacity();
        int framesCached = 0;
        int texturesCached = 0;
        int lastFrameCached = 0;

//
//  Set caching thread priority to be less than display thread,
//  less than audio thread.
//
#ifdef PLATFORM_WINDOWS
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
#endif

        TwkUtil::Timer* timer = 0;
        if (m_cacheTimingOutput || DB_LEVEL & DB_CACHE)
        {
            timer = new TwkUtil::Timer();
            timer->start();
        }

        TwkUtil::Timer statsUpdateTimer;
        statsUpdateTimer.start();

#if (DB_LEVEL & DB_CACHE)
        {
            stringstream out;
            out << "evalThreadMain, thread " << id << " START, dispFrame "
                << m_fbcache.displayFrame() << ", cacheThreadContinue "
                << cacheThreadContinue() << endl;
            cerr << out.str();
        }
#endif

        FBCache::FrameVector frames;

        int maxGroupSize = getMaxGroupSize(m_evalSlowMedia);

        if (m_rootNode)
        {
//
//  Sometimes it seems necessary to stagger the start of the
//  thread.  Now I can't find my test case.  Leave it out
//  for now.
//
#if 0
#ifdef PLATFORM_WINDOWS
            Sleep (3*id);
#else
            usleep ((3*id)*1000);
#endif
#endif
            lockInternal();
            threadData->running = true;
            unlockInternal();

            int frame = 0;

            //
            //  cache threads never stop unless told to do so.
            //

            // while (m_imageSources.size() && cacheThreadContinue())
            while (cacheThreadContinue())
            {
                //
                //    first check for cachable output texture items.
                //

                string itemName;
                if ((itemName = m_fbcache.popCachableOutputItem()) != "")
                {
                    IPNode* itemNode = findNode(itemName);
                    TextureOutputGroupIPNode* textNode = 0;
                    if (textNode =
                            dynamic_cast<TextureOutputGroupIPNode*>(itemNode))
                    {
                        if (textNode->isActive())
                        {
                            /*
                            ostringstream str;
                            str << "thread " << id << ": output item to eval: "
                            << itemName << " tag " << textNode->tag() << endl;
                            cerr << str.str();
                            */

                            IPNode::Context context(
                                1, 1, m_fbcache.displayFPS(), 0, 0,
                                IPNode::CacheEvalThread, id, m_fbcache, false);

                            context.cacheNode = textNode;

                            try
                            {
                                IPImage* img = textNode->evaluate(context);
                                TWK_CACHE_LOCK(m_fbcache, "");
                                m_fbcache.checkInAndDelete(img);
                                TWK_CACHE_UNLOCK(m_fbcache, "");
                                ++texturesCached;
                            }
                            catch (...)
                            {
                                /*
                                ostringstream str;
                                str << "EXCEPTION: thread " << id << ": output
                                item to eval: " << itemName << " tag " <<
                                textNode->tag() << endl; cerr << str.str();
                                //  Just try again ?
                                m_fbcache.pushCachableOutputItem(itemName);
                                */
                            }
                        }
                    }
                    //
                    //  If we did find a texture item to cache, loop around
                    //  again, otherwise begin caching "frames" as usual.
                    //
                    continue;
                }

                bool skipThisFrame = false;

                if (frames.empty())
                {
                    TWK_CACHE_LOCK(m_fbcache, "");
                    m_fbcache.initiateCachingOfBestFrameGroup(frames,
                                                              maxGroupSize);
                    TWK_CACHE_UNLOCK(m_fbcache, "");
                }

                if (frames.empty())
                    frame = NAF;
                else
                {
                    frame = frames.back();
                    frames.pop_back();
                }

                //
                //  Run a test evaluation to see if we can do this frame random
                //  access. If not we only allow thread 1 to evaluate.
                //  (Thread 0 is the display thread.)
                //
                //  We also force 20-frame block caching when random access is
                //  poor.

                if (NAF != frame)
                {
                    bool poorPerf = false;
                    try
                    {
                        TestEvalResult r =
                            testEvaluate(frame, IPNode::CacheEvalThread, id);
                        poorPerf = r.poorRandomAccessPerformance;
                    }
                    catch (std::exception& exc)
                    {
                        //
                        //  This is an odd case. It shouldn't happen.
                        //

                        cerr << "ERROR: testEvaluate unexpected exception: "
                             << exc.what() << ", at frame " << frame << endl;

                        skipThisFrame = true;
                    }
                    DB("after testEvaluate, skipThisFrame " << skipThisFrame);

                    if (id == 1)
                    {
                        m_evalSlowMedia = poorPerf;
                        if (maxGroupSize != getMaxGroupSize(poorPerf))
                        //
                        //  Since we're changing our "blocking" policy, don't
                        //  bother trying to cache whatever frame(s) the old
                        //  policy gave us, just loop around again and get a
                        //  new block of frames.
                        //
                        {
                            TWK_CACHE_LOCK(m_fbcache, "");
                            m_fbcache.completeCachingOfFrame(frame);
                            for (int i = 0; i < frames.size(); ++i)
                            {
                                m_fbcache.completeCachingOfFrame(frames[i]);
                            }
                            TWK_CACHE_UNLOCK(m_fbcache, "");
                            frames.clear();
                            maxGroupSize = getMaxGroupSize(poorPerf);
                            continue;
                        }
                    }
                    else if (poorPerf)
                    {
                        skipThisFrame = true;
                    }

                    if (skipThisFrame)
                    {
                        TWK_CACHE_LOCK(m_fbcache, "");
                        m_fbcache.completeCachingOfFrame(frame);
                        TWK_CACHE_UNLOCK(m_fbcache, "");
                    }
                }

                if (NAF == frame || skipThisFrame)
                {
                    //
                    //  There is either nothing for this thread to do at
                    //  the moment (the cache is full, and the frame we
                    //  would cache is "less desireable" than the frame we
                    //  would have to free to make room for it) or it
                    //  can't evaluate because this frame has a source
                    //  with poor random access performance and this isn't
                    //  thread #0.
                    //
                    //  Breaking here returns the thread to worker_wait()
                    //  in the caching thread_group.  It will be restarted
                    //  by dispatch() or by awaken_all_workers() when the
                    //  frame changes or from some other state change.
                    //
                    DB("bad target frame: frame " << frame << " skip "
                                                  << skipThisFrame);

                    break;
                }

#ifdef PLATFORM_WINDOWS
                //
                //  Before we read/evaluate/cache anything, check that we're
                //  not running up against Windows' virtual memory.  If a
                //  big alloc fails, reduce the cache size to something less
                //  then whatever's in there now.
                //
                //  The 64MB number below is empirical.  It seems to be the
                //  smallest number you can use here that prevents RV from
                //  crashing when the max cache size is set "too high".
                //
                //  OK, well, maybe it's not windows so much as just trying
                //  to fit too much stuff in a 32-bit address space.
                //  Windows seems extra restrictive, but large caches will
                //  cause crashes on any 32-bit build.
                //
                //  Note: alloc/dealloc pair seems to take 100-200us on windows
                //  and mac.
                //
                //  XXX Note: since this takes some time, maybe we
                //  should provide a pref to turn this off, so that
                //  expert users (who know not to set their cache size
                //  too high) can turn it off for maximum performance.
                //

                /*
                TwkUtil::Timer allocTimer;
                allocTimer.start();
                */
                TWK_CACHE_LOCK(m_fbcache, "");
                size_t minBuffer = 64 * 1024 * 1024;
                unsigned char* chkData =
                    TWK_ALLOCATE_ARRAY_PAGE_ALIGNED(unsigned char, minBuffer);

                //  fprintf (stderr, "%p evaluate, frame %d\n", pthread_self(),
                //  frame);
                if (chkData)
                {
                    TWK_DEALLOCATE_ARRAY(chkData);
                    /*
                    float e = 1.0e6*allocTimer.elapsed();
                    if (e > allocTimeMax)
                        allocTimeMax = e;
                    if (e < allocTimeMin)
                        allocTimeMin = e;
                    allocTimeTotal += e;
                    allocTimeCount += 1.0;

                    fprintf (stderr, "%p alloc time: %g ave %g min %g max %g\n",
                            pthread_self(), e, allocTimeTotal/allocTimeCount,
                    allocTimeMin, allocTimeMax); fflush (stderr);
                    */
                }
                else
                {
                    DB("caching thread " << id << " failed to allocate "
                                         << minBuffer);

                    size_t newCacheSize = m_fbcache.used();
                    if (newCacheSize > minBuffer)
                        newCacheSize -= minBuffer;
                    newCacheSize = max(size_t(128 * 1024 * 1024), newCacheSize);
                    DB("caching thread " << id << " setting cache size to "
                                         << newCacheSize);

                    //
                    //  Can't call setCacheModeSize here, since it may
                    //  lock the cache.
                    //
                    m_cacheSizeMap[IPGraph::GreedyCache] = newCacheSize;
                    m_cacheSizeMap[IPGraph::BufferCache] = newCacheSize;
                    m_fbcache.setMemoryUsage(newCacheSize);
                    m_fbcache.emergencyFree();

                    DB("caching thread " << id << " current "
                                         << m_fbcache.used() << " max "
                                         << m_fbcache.capacity() << " "
                                         << double(m_fbcache.used())
                                                / double(m_fbcache.capacity()));
                }
                TWK_CACHE_UNLOCK(m_fbcache, "");
#endif

                try
                {
                    DB("thread " << id << " evaluate frame " << frame
                                 << ", overflowing "
                                 << m_fbcache.overflowing());
                    IPImage* img = evaluate(frame, IPNode::CacheEvalThread, id);

                    TWK_CACHE_LOCK(m_fbcache, "");
                    DB("thread " << id << " evaluate frame " << frame
                                 << " ok ");
                    //  m_fbcache.trimFBsOfFrame(frame, img);
                    m_fbcache.checkInAndDelete(img);
                    DB("thread " << id << " checkin frame " << frame << " ok ");
                    TWK_CACHE_UNLOCK(m_fbcache, "");
                }
                catch (CacheFullExc& exc)
                {
                    //
                    //  This will stop the cache thread. The display
                    //  thread will have to re-awaken it fetch another
                    //  frame.
                    //

                    DB("thread " << id << " cache is full, overflowing "
                                 << m_fbcache.overflowing());
                    //  UTILITY_CACHING
                    //
                    //  We no longer want to exit here, since although
                    //  we're here because frame A failed to cache, it
                    //  may be that some other thread, or even this one
                    //  once it tries again, will find frame B that
                    //  should be cached (at the expense of something in
                    //  the cache) so keep looking.  We only stop when
                    //  there's nothing to cache.
                    //
                    //  stopCachingInternal();
                }
                catch (std::exception& exc)
                {
                    //
                    //  Should never get here
                    //

                    cerr
                        << "ERROR: caching thread caught unexpected exception: "
                        << exc.what() << ", at frame " << frame << endl;

                    stopCachingInternal();
                }
                catch (...)
                {
                    //
                    //  Shouldn't get here either
                    //

                    cerr << "ERROR: unexpected exception in caching thread"
                         << ", at frame " << frame << endl;

                    stopCachingInternal();
                }

                TWK_CACHE_LOCK(m_fbcache, "");
                m_fbcache.completeCachingOfFrame(frame);
                TWK_CACHE_UNLOCK(m_fbcache, "");
                lastFrameCached = frame;
                ++framesCached;

                //
                //  Update cacheStats no more often than 10x a second.  Only
                //  caching thread #1 updates stats while caching.
                //

                if (id == 1 && statsUpdateTimer.elapsed() > 0.1)
                {
                    m_fbcache.updateCacheStatsIfDirty();
                    statsUpdateTimer.start();
                }
            }

            lockInternal();
            threadData->running = false;
            unlockInternal();
        }
        else
        {
            lockInternal();
            threadData->running = false;
            unlockInternal();
        }
        if (frames.size())
        {
            TWK_CACHE_LOCK(m_fbcache, "");
            while (frames.size())
            {
                m_fbcache.completeCachingOfFrame(frames.back());
                frames.pop_back();
            }
            TWK_CACHE_UNLOCK(m_fbcache, "");
        }

        //
        //  All threads update cache stats (if necessary) on exit.
        //
        m_fbcache.updateCacheStatsIfDirty();

#if (DB_LEVEL & DB_CACHE)
        {
            stringstream out;
            float elap = timer->elapsed();
            out << "evalThreadMain, thread " << id << " END";
            if (framesCached || texturesCached)
            {
                out << ", " << framesCached << " frames ("
                    << (1000.0 * elap / float(framesCached)) << " ms/frame, "
                    << texturesCached << " textures, " << (1000.0 * elap)
                    << " total), last frame: " << lastFrameCached;
            }
            out << endl;
            cerr << out.str();
        }
#endif

        if (m_cacheTimingOutput && (framesCached || texturesCached))
        {
            stringstream out;
            float elap = timer->elapsed();
            out << "Caching thread " << id << " END" << ", " << framesCached
                << " frames (" << (1000.0 * elap / float(framesCached))
                << " ms/frame, " << texturesCached << " textures, "
                << (1000.0 * elap) << " total), last frame: " << lastFrameCached
                << endl;
            cerr << out.str();
        }
        delete timer;

        m_textureCacheUpdated();
    }

    //----------------------------------------------------------------------

    void IPGraph::setAudioThreading(bool b)
    {
        if (b == false)
        {
            finishAudioThread();
            m_audioThreading = b;
            m_audioThreadStop = false;
        }
        else
        {
            m_audioThreading = true;
            m_audioThreadStop = false;
            maybeDispatchAudioThread();
        }
    }

    void IPGraph::setAudioCachingMode(CachingMode m)
    {
        //  Only Buffer/Greedy caching allowed for audio.
        //
        if (m != GreedyCache)
            m = BufferCache;

        m_audioPendingCacheMode = m;

        if (hasAudio() && m_audioCacheMode != m && m_audioConfigured)
        {
            lockAudioFill();
            lockAudioInternal();
            m_audioCacheMode = m;
            m_audioCache.lock();
            m_audioCache.clear();
            m_audioCache.unlock();
            m_inAudioConfig = true;
            unlockAudioInternal();

            finishAudioThread();

            IPNode::AudioConfiguration config(m_audioCache.rate(),
                                              m_audioCache.layout(),
                                              m_audioCache.packetSize());

            m_rootNode->propagateAudioConfigToInputs(config);

            lockAudioInternal();
            m_inAudioConfig = false;
            m_audioCacheMode = m;
            m_currentAudioSample = m_minAudioSample.load();
            m_audioCacheComplete = false;
            unlockAudioInternal();

            maybeDispatchAudioThread();
            unlockAudioFill();
        }
    }

    void IPGraph::setAudioCacheExtents(TwkAudio::Time minSec,
                                       TwkAudio::Time maxSec)
    {
        m_audioMinCache = minSec;
        m_audioMaxCache = maxSec;
    }

    void IPGraph::lockAudioInternal() const
    {
        pthread_mutex_lock(&m_audioInternalLock);
    }

    void IPGraph::unlockAudioInternal() const
    {
        pthread_mutex_unlock(&m_audioInternalLock);
    }

    void IPGraph::lockAudioFill() const
    {
        pthread_mutex_lock(&m_audioFillLock);
    }

    bool IPGraph::tryLockAudioFill() const
    {
        return pthread_mutex_trylock(&m_audioFillLock) == 0;
    }

    void IPGraph::unlockAudioFill() const
    {
        pthread_mutex_unlock(&m_audioFillLock);
    }

    double IPGraph::audioSecondsCached() const
    {
        //
        //  There really should be a lock or a try lock here. The problem
        //  is that we don't EVER want to interrupt the audio thread from
        //  the rendering thread (which calls this). So we'll just do the
        //  bad thing. totalSecondsCached() is returning a single float
        //  field value. It could be done atomically to prevent the locks.
        //

        // m_audioCache.lock();
        const double d = m_audioCache.totalSecondsCached();
        // m_audioCache.unlock();
        return d;
    }

    void IPGraph::maybeDispatchAudioThread()
    {
        bool ok = false;

        if (m_audioThreading == false || m_editing)
            return;

        lockAudioInternal();
        ok = !m_inAudioConfig && !m_inFinishAudioThread;
        if (ok)
            m_audioThreadStop = false;
        if (m_audioCacheComplete && m_audioCacheMode != BufferCache)
            ok = false;
        unlockAudioInternal();

        if (ok)
        {
            m_audioThreadGroup.maybe_dispatch(evalAudioThreadTrampoline, this,
                                              true);
        }
    }

    void IPGraph::dispatchAudioThread()
    {
        bool ok = false;
        if (m_audioThreading == false || m_editing)
            return;

        lockAudioInternal();
        ok = !m_inAudioConfig && !m_inFinishAudioThread;
        if (ok)
            m_audioThreadStop = false;
        if (m_audioCacheComplete && m_audioCacheMode != BufferCache)
            ok = false;
        unlockAudioInternal();

        if (ok)
            m_audioThreadGroup.dispatch(evalAudioThreadTrampoline, this);
    }

    void IPGraph::finishAudioThread()
    {
        if (m_audioThreading == false)
            return;

        lockAudioInternal();
        m_inFinishAudioThread = true;
        m_audioThreadStop = true;
        unlockAudioInternal();

        m_audioThreadGroup.control_wait();

        lockAudioInternal();
        m_inFinishAudioThread = false;
        unlockAudioInternal();
    }

    void IPGraph::flushAudioCache()
    {
        m_audioCache.lock();
        m_audioCache.clear();
        lockAudioInternal();
        SampleTime t = m_lastFillSample;
        m_audioCacheComplete = false;
        m_currentAudioSample =
            t + m_audioCache.packetSize() - (t % m_audioCache.packetSize());
        unlockAudioInternal();
        m_audioCache.unlock();

        if (m_audioCacheMode == GreedyCache)
            maybeDispatchAudioThread();
    }

    void IPGraph::primeAudioCache(int frame, float frameRate)
    {
        if (hasAudio() && (audioCachingMode() != NeverCache)
            && isAudioConfigured())
        {
            // Prime the audio cache so that it is ready when the playback
            // starts.
            m_audioCache.lock();
            lockAudioInternal();
            TwkAudio::Time audioTime = (TwkAudio::Time)frame / frameRate;
            TwkAudio::SampleTime audioSample =
                TwkAudio::timeToSamples(audioTime, audioCache().rate());
            m_audioCacheComplete = false;
            m_currentAudioSample = audioSample + m_audioCache.packetSize()
                                   - (audioSample % m_audioCache.packetSize());
            m_lastFillSample = audioSample;
            unlockAudioInternal();
            m_audioCache.unlock();

            _audioClearCacheOutsideHeadAndTail();

            maybeDispatchAudioThread();
        }
    }

    TwkAudio::Time IPGraph::globalAudioOffset() const
    {
        return m_audioOffset->front() + m_audioOffset2->front();
    }

    void IPGraph::audioConfigure(const AudioConfiguration& config)
    {
        //
        //  Check to see if there's any point in reconfiguration
        //

        bool notify = !m_audioConfigured
                      || m_lastAudioConfiguration.rate != config.rate
                      || m_lastAudioConfiguration.layout != config.layout
                      || m_lastAudioConfiguration.samples != config.samples;

        //
        //  Lock it up
        //

        lockAudioFill();
        lockAudioInternal();
        m_inAudioConfig = true;
        unlockAudioInternal();

        if (notify)
            finishAudioThread();

        m_audioPacketSize =
            Application::optionValue<size_t>("acachesize", 2048);
        m_audioMinCache =
            Application::optionValue<double>("audioMinCache", m_audioMinCache);
        m_audioMaxCache =
            Application::optionValue<double>("audioMaxCache", m_audioMaxCache);

        m_audioCache.lock();

        if (m_audioCache.packetSize() != m_audioPacketSize
            || m_audioCache.layout() != config.layout
            || m_audioCache.rate() != config.rate)
        {
            m_audioCache.configurePacket(m_audioPacketSize, config.layout,
                                         config.rate);
            m_audioCacheComplete = false;
        }

        if (m_clearAudioCacheRequested.exchange(false))
        {
            m_audioCache.clear();
            lockAudioInternal();
            m_audioCacheComplete = false;
            unlockAudioInternal();
        }

        m_audioCache.unlock();

        m_audioFPS = config.fps;
        m_audioBackwards = config.backwards;

        //
        // The current audio sample is the first sample of the cache packet
        // which contains the configuration start sample. To calculate that we
        // start with the configuration start sample and subtract the global
        // audio offset. Then we subtract the packet offset to find the start of
        // the containing packet boundary.
        //

        SampleTime globalOffset =
            timeToSamples(globalAudioOffset(), config.rate);
        SampleTime offsetStartSample = config.startSample - globalOffset;
        SampleTime boundaryBeginStart =
            offsetStartSample - offsetStartSample % m_audioPacketSize;

        m_currentAudioSample = boundaryBeginStart;
        m_lastFillSample = boundaryBeginStart;

        SampleTime minSample = 0 - globalOffset;
        m_minAudioSample = minSample - minSample % m_audioPacketSize;
        m_maxAudioSample = config.endSample - globalOffset;

        //
        //  Reconfigure the audio cache. Use at least one page (4k bytes)
        //  for the buffer size to minimize filesystem interaction. On
        //  Linux some of the audio drivers will get as small as 32
        //  samples per invocation. OS X seems to stay around 512.
        //

        if (notify)
        {
            IPNode::AudioConfiguration newConfig(config.rate, config.layout,
                                                 m_audioPacketSize);

            m_rootNode->propagateAudioConfigToInputs(newConfig);
        }

        lockAudioInternal();
        m_inAudioConfig = false;
        m_audioConfigured = true;
        unlockAudioInternal();

        maybeDispatchAudioThread();

        m_lastAudioConfiguration = config;

        unlockAudioFill();

        if (m_audioPendingCacheMode != m_audioCacheMode)
        {
            setAudioCachingMode(m_audioPendingCacheMode);
        }
    }

    size_t IPGraph::audioFillBuffer(const IPNode::AudioContext& incontext)
    {
        AudioBuffer abuffer(incontext.buffer, 0, incontext.buffer.size(),
                            incontext.buffer.startTime() - globalAudioOffset());

        IPNode::AudioContext context(abuffer, incontext.fps);

        if (m_audioThreading == false)
        {
            //
            //  In the synchronous case, we need to make sure we get the
            //  current audio sample set correctly. Keep the audio
            //  evaluation continuous to prevent and sampling errors if
            //  possible.
            //

            SampleTime s = timeToSamples(context.buffer.startTime(),
                                         context.buffer.rate());

            TwkMath::Time bufferDuration = context.buffer.duration();

            if (bufferDuration > m_audioMaxCache)
            {
                //
                //  Bump up the cache size to make sure that it can hold
                //  enough audio to fill the current buffer size.
                //

                m_audioMinCache = bufferDuration * 2.0;
                m_audioMaxCache = bufferDuration * 3.0;
            }

            const size_t d = s % m_audioPacketSize;

            s -= m_currentAudioSample > d ? d : s;

            if (audioCachingMode() != GreedyCache)
            {
                m_audioCache.clearBefore(samplesToTime(s, m_audioCache.rate()));
            }

            m_lastFillSample = s;

            evalAudioThreadMain();
        }

        return audioFillBufferInternal(context);
    }

    size_t IPGraph::audioFillBufferInternal(const IPNode::AudioContext& context)
    {
        AudioBuffer& inbuffer = context.buffer;

        if (m_audioConfigured && tryLockAudioFill() && m_rootNode)
        {
            m_audioCache.lock();
            bool found = m_audioCache.fillBuffer(inbuffer);
            double rate = m_audioCache.rate();
            m_audioCache.unlock();

            if (!found)
            {
                maybeDispatchAudioThread();

                //
                //  Just return silence if its not ready
                //

                if (AudioRenderer::debug)
                {
                    cerr << "DEBUG: audio cache miss, zeroing buffer!" << endl;
                }

                inbuffer.zero();
                found = true;
            }

            lockAudioInternal();
            m_lastFillSample = inbuffer.startSample();
            unlockAudioInternal();

            //
            // In look-ahead audio cache mode, potentially notify the audio
            // thread to cache more audio samples if applicable
            //

            if (m_audioCacheMode == BufferCache)
            {
                // If we have room in the cache for more audio samples
                // Or if we the lastly accessed audio sample is within a certain
                // threshold of the current head and tail
                if ((m_audioCache.totalSecondsCached() <= m_audioMinCache)
                    || (!m_audioBackwards
                        && m_lastFillSample >= m_restartAudioThreadSample)
                    || (m_audioBackwards
                        && m_lastFillSample <= m_restartAudioThreadSample))
                {
                    maybeDispatchAudioThread();
                }
            }

            unlockAudioFill();

            _audioMapLinearVolumeToPerceptual(inbuffer);
        }
        else
        {
            if (AudioRenderer::debug)
            {
                cerr << "DEBUG: audio locked out -- skipping" << endl;
            }

            inbuffer.zero();
        }

        return inbuffer.size();
    }

    //------------------------------------------------------------------------------
    //
    void IPGraph::_audioMapLinearVolumeToPerceptual(AudioBuffer& inout_buffer)
    {
        //
        //  This maps linear volume to perceptual
        //

        float v = m_volume->front();
        float b = m_balance->front();

        v = (exp(v) - 1.0) / (M_E - 1.0);

        if (v < 0.0f)
            v = 0.0f;
        if (b > 1.0f)
            b = 1.0f;
        if (b < -1.0f)
            b = -1.0f;

        if (m_mute->front())
            v = 0.0f;

        const bool clamp = m_audioSoftClamp->front() != 0;

        const float l = (b > 0.0 ? (1.0 - b) : 1.0) * v;
        const float r = (b < 0.0 ? (1.0 + b) : 1.0) * v;

        const float ll = clamp ? taperedClamp(l, 0.0f, 1.0f) : l;
        const float rl = clamp ? taperedClamp(r, 0.0f, 1.0f) : r;

        if (fabsf(1.0f - ll) > 1.0e-06f || fabsf(1.0f - rl) > 1.0e-06f)
        {
            mixChannels(inout_buffer, inout_buffer, ll, rl, false);
        }

        const float cutoff = inout_buffer.rate() / 2.0;

        // TODO(Improvement): RV does not consistently apply a low pass filter
        // on audio. A low pass filter will be applied only if a previous audio
        // buffer has been generated. On top of that, this previous audio buffer
        // handling is highly inefficient : we only need the previous audio
        // sample but we still memcpy the whole previous packet. We could
        // preserve the previous audio samples instead and when they don't exist
        // when we could simply duplicate the first sample instead as the
        // previous sample.

        static AudioBuffer s_audioLastBuffer;
        static std::mutex mutexAudioLastBuffer;
        std::unique_lock<std::mutex> audioLastBufferGuard(mutexAudioLastBuffer);

        // Validate that the s_audioLastBuffer is compatible and is actually the
        // previous buffer prior to using it
        if (s_audioLastBuffer.rate() == inout_buffer.rate()
            && s_audioLastBuffer.size() == inout_buffer.size()
            && s_audioLastBuffer.startTime() != inout_buffer.startTime())
        {
            const SampleTime expectedPrevFirstSample = timeToSamples(
                m_lastAudioConfiguration.backwards
                    ? (inout_buffer.startTime() + s_audioLastBuffer.duration())
                    : (inout_buffer.startTime() - s_audioLastBuffer.duration()),
                inout_buffer.rate());
            const SampleTime lastBufferFirstSample = timeToSamples(
                s_audioLastBuffer.startTime(), inout_buffer.rate());
            if (lastBufferFirstSample == expectedPrevFirstSample)
            {
                lowPassFilter(inout_buffer, s_audioLastBuffer, inout_buffer,
                              cutoff, m_lastAudioConfiguration.backwards);
            }
        }

        s_audioLastBuffer.reconfigure(
            inout_buffer.size(), inout_buffer.channels(), inout_buffer.rate(),
            inout_buffer.startTime());

        memcpy(reinterpret_cast<void*>(s_audioLastBuffer.pointer()),
               reinterpret_cast<void*>(inout_buffer.pointer()),
               inout_buffer.sizeInBytes());
    }

    //------------------------------------------------------------------------------
    // Clear cache entries outside the head and tail region if required
    //
    void IPGraph::_audioClearCacheOutsideHeadAndTail()
    {
        if (m_audioCacheMode == GreedyCache)
            return;

        lockAudioInternal();
        const double audioDuration = samplesToTime(
            m_maxAudioSample - m_minAudioSample + 1, m_audioCache.rate());
        const double audioMaxCache =
            (m_audioCacheMode == BufferCache)
                ? std::min(m_audioMaxCache.load(), audioDuration)
                : audioDuration;
        const double lastFillAudioBufferTime =
            samplesToTime(m_lastFillSample, m_audioCache.rate());
        const double minAudioTime =
            samplesToTime(m_minAudioSample, m_audioCache.rate());
        const double maxAudioTime =
            samplesToTime(m_maxAudioSample, m_audioCache.rate());
        const Time audioRate = m_audioCache.rate();
        unlockAudioInternal();

        // Nothing to do if the cache is bigger than the total audio duration
        if (audioMaxCache >= audioDuration)
            return;

        // Compute the head and tail durations
        double tailDuration =
            static_cast<double>(lookBehindFraction()) * audioMaxCache / 100.0;
        double headDuration = audioMaxCache - tailDuration;

        // Simply invert the head and tail if we are playing backwards
        if (m_audioBackwards)
        {
            std::swap(headDuration, tailDuration);
        }

        // Compute a threashold to be used later in audioFillBuffer() to
        // determine when the audio thread should go through another cache
        // clearing operation again so as to make it more efficient to fill in
        // the audio cache in chunks as opposed to 1 audio sample at a time.
        if (!m_audioBackwards)
        {
            double restartAudioThreadTime =
                lastFillAudioBufferTime + 0.5 * headDuration;
            if (restartAudioThreadTime > maxAudioTime)
            {
                restartAudioThreadTime =
                    restartAudioThreadTime - maxAudioTime + minAudioTime;
            }
            m_restartAudioThreadSample =
                timeToSamples(restartAudioThreadTime, audioRate);
        }
        else
        {
            double restartAudioThreadTime =
                lastFillAudioBufferTime - 0.5 * tailDuration;
            if (restartAudioThreadTime < minAudioTime)
            {
                restartAudioThreadTime =
                    restartAudioThreadTime + maxAudioTime - minAudioTime;
            }
            m_restartAudioThreadSample =
                timeToSamples(restartAudioThreadTime, audioRate);
        }

        AudioCache::AudioCacheLock lock(m_audioCache);

        const double tail = lastFillAudioBufferTime - tailDuration;
        const double head = lastFillAudioBufferTime + headDuration;

        // No cache wrap around case
        if (tail > 0.0 && head < maxAudioTime)
        {
            m_audioCache.clearBefore(tail);
            m_audioCache.clearAfter(head);

            if (AudioRenderer::debug)
            {
                const int clear_before_frame =
                    ROUND(tail * m_fbcache.displayFPS());
                const int clear_after_frame =
                    ROUND(head * m_fbcache.displayFPS());
                TwkUtil::Log("AUDIO")
                    << "_audioClearCacheOutsideHeadAndTail()-"
                    << "m_audioCache.clearBefore=" << clear_before_frame
                    << ", m_audioCache.clearAfter=" << clear_after_frame;
            }
        }
        // Cache wrap around case
        else
        {
            // Note that we have 2 possible wrap around cases here:
            // H = Head, T = Tail, C = Center
            //
            // Tail is greater than 0:
            // ---------------------------------
            // HHHHH         TTTTTTTTCHHHHHHHHHH
            // ---------------------------------
            // Left   = head-maxAudioTime
            // Right  = head
            //
            // Or not:
            // ---------------------------------
            // TTCHHHHHHHHHH              TTTTTT
            // ---------------------------------
            // Left   = head
            // Right  = maxAutioTime+tail
            //
            const double left = (tail > 0.0) ? (head - maxAudioTime) : head;
            const double right = (tail > 0.0) ? tail : (maxAudioTime + tail);
            m_audioCache.clear(left, right);

            if (AudioRenderer::debug)
            {
                const int clear_left_frame =
                    ROUND(left * m_fbcache.displayFPS());
                const int clear_right_frame =
                    ROUND(right * m_fbcache.displayFPS());
                TwkUtil::Log("AUDIO")
                    << "_audioClearCacheOutsideHeadAndTail() "
                    << "m_audioCache.clear(" << clear_left_frame << ","
                    << clear_right_frame << ")";
            }
        }
    }

    //------------------------------------------------------------------------------
    //
    bool IPGraph::_isCurrentAudioSampleBeyondHead(
        const SampleTime audioHeadSamples) const
    {
        if (!m_audioBackwards)
        {
            SampleTime head = m_lastFillSample + audioHeadSamples;
            if (head > m_maxAudioSample)
            {
                // Handle wrap around case
                head = head - m_maxAudioSample + m_minAudioSample;
                return (m_currentAudioSample > head
                        && m_currentAudioSample < m_lastFillSample);
            }
            else
            {
                // Handle regular case
                return (m_currentAudioSample > head);
            }
        }
        else
        {
            SampleTime head = m_lastFillSample - audioHeadSamples;
            if (head < m_minAudioSample)
            {
                // Handle wrap around case
                head = m_maxAudioSample - (m_minAudioSample - head);
                return (m_currentAudioSample < head
                        && m_currentAudioSample > m_lastFillSample);
            }
            else
            {
                // Handle regular case
                return (m_currentAudioSample < head);
            }
        }

        return false;
    }

    //------------------------------------------------------------------------------
    //
    void IPGraph::evalAudioThreadMain()
    {
        if (m_audioThreadStop || !m_audioConfigured || m_inAudioConfig
            || !m_rootNode)
        {
            return;
        }

//
//  Set audio thread priority to be less than display thread,
//  higher than caching threads.
//
#ifdef PLATFORM_WINDOWS
        if (m_audioThreading)
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
#endif

        lockAudioInternal();
        const double fps = m_audioFPS;
        const double audioDuration = samplesToTime(
            m_maxAudioSample - m_minAudioSample + 1, m_audioCache.rate());
        const double audioMaxCache =
            (m_audioCacheMode == BufferCache)
                ? std::min(m_audioMaxCache.load(), audioDuration)
                : audioDuration;
        const bool audioFitsIntoCache = audioDuration <= audioMaxCache;
        const SampleTime audioMaxCacheInSamples =
            timeToSamples(m_audioMaxCache, m_audioCache.rate());
        const SampleTime audioHeadSamples =
            audioMaxCacheInSamples / 100LL
            * static_cast<SampleTime>(100.0 - lookBehindFraction());
        unlockAudioInternal();

        // Make sure to free enough room in the cache
        _audioClearCacheOutsideHeadAndTail();

        m_audioCache.lock();
        TwkAudio::Time secondsBuffered = m_audioCache.totalSecondsCached();
        m_audioCache.unlock();

        bool cachedSome = false;

        //
        // So long as we have room in the cache (and the thread hasn't been told
        // to stop), then keep reading audio into the cache.
        //

        if (AudioRenderer::debug)
        {
            TwkUtil::Log("AUDIO")
                << "evalAudioThreadMain()-"
                << "secondsBuffered=" << secondsBuffered
                << ", audioMaxCache=" << audioMaxCache
                << ", m_audioThreadStop=" << m_audioThreadStop;
        }

        while (!m_audioThreadStop && secondsBuffered < audioMaxCache)
        {
            m_audioCache.lock();
            const bool exists = m_audioCache.exists(m_currentAudioSample);
            const size_t packetSize = m_audioCache.packetSize();
            const Layout layout = m_audioCache.layout();
            const double rate = m_audioCache.rate();
            m_audioCache.unlock();

            if (!exists)
            {
                if (AudioRenderer::debug)
                {
                    TwkUtil::Log("AUDIO") << "Audio sample not in cache, "
                                          << "requesting m_currentAudioSample="
                                          << m_currentAudioSample.load();
                }

                AudioBuffer abuffer(packetSize, layout, rate,
                                    samplesToTime(m_currentAudioSample, rate));

                IPNode::AudioContext context(abuffer, fps);

                try
                {
                    size_t nread = m_rootNode->audioFillBuffer(context);
                    if (AudioRenderer::debug && nread != packetSize)
                    {
                        cerr << "DEBUG: asked for " << packetSize << " samples,"
                             << " but received " << nread << endl;
                    }

                    m_audioCache.lock();
                    m_audioCache.add(context.buffer);
                    secondsBuffered = m_audioCache.totalSecondsCached();
                    m_audioCache.unlock();

                    m_audioCacheComplete = (secondsBuffered >= audioDuration);

                    cachedSome = true;
                }
                catch (std::exception& exc)
                {
                    cerr << "WARNING: audio eval thread: " << exc.what()
                         << endl;
                }
                catch (...)
                {
                    cerr << "ERROR: audio eval thread caught unknown exception"
                         << endl;
                }
            }

            // Stop fetching audio if we have enough head audio samples
            // Note: We actively fetch the head audio samples but not the tail
            // audio samples: those are passively generated by preserving the
            // tail audio samples in the cache as we go along.
            if (!audioFitsIntoCache
                && _isCurrentAudioSampleBeyondHead(audioHeadSamples))
            {
                if (AudioRenderer::debug)
                {
                    TwkUtil::Log("AUDIO")
                        << "Stop fetching audio, we have enough audio samples"
                        << " m_currentAudioSample="
                        << m_currentAudioSample.load() << " beyond head";
                }

                break;
            }

            m_currentAudioSample += m_audioBackwards ? -packetSize : packetSize;

            // Handle wrap around
            if ((m_currentAudioSample > m_maxAudioSample
                 || m_currentAudioSample < m_minAudioSample))
            {
                m_currentAudioSample =
                    !m_audioBackwards ? m_minAudioSample.load()
                                      : m_maxAudioSample + packetSize
                                            - (m_maxAudioSample % packetSize);
            }
        }

        if (AudioRenderer::debug)
        {
            if (cachedSome)
            {
                TwkUtil::Log("AUDIO")
                    << "After adding to audio cache:"
                    << " m_currentAudioSample = " << m_currentAudioSample
                    << ", m_minAudioSample = " << m_minAudioSample
                    << ", m_maxAudioSample = " << m_maxAudioSample
                    << ", secondsBuffered = " << secondsBuffered
                    << ", audioMaxCache = " << audioMaxCache
                    << ", m_audioThreadStop = " << m_audioThreadStop;
            }
        }
    }

    void IPGraph::beginNodeValidation(IPNode* node)
    {
        TwkApp::GenericStringEvent event("graph-validation-begin", this,
                                         node->name());
        sendEvent(event);
    }

    void IPGraph::endNodeValidation(IPNode* node)
    {
        TwkApp::GenericStringEvent event("graph-validation-end", this,
                                         node->name());
        sendEvent(event);
    }

    void IPGraph::propertyChanged(const Property* p)
    {
        m_propertyChangedSignal(p);

        ostringstream str;
        const IPNode* pc = dynamic_cast<const IPNode*>(p->container());
        string n = pc->propertyFullName(p);
        TwkApp::GenericStringEvent event("graph-state-change", this, n);
        sendEvent(event);
    }

    void IPGraph::rangeChanged(IPNode* n)
    {
        if (n == root())
        {
            TwkApp::GenericStringEvent event("graph-range-change", this, "");
            sendEvent(event);
            m_nodeRangeChangedSignal(n);
        }
    }

    void IPGraph::imageStructureChanged(IPNode* n)
    {
        if (n == root())
        {
            TwkApp::GenericStringEvent event("image-structure-change", this,
                                             "");
            sendEvent(event);
            m_nodeImageStructureChangedSignal(n);
        }
    }

    void IPGraph::mediaChanged(IPNode* n)
    {
        if (n == root())
        {
            TwkApp::GenericStringEvent event("media-change", this, "");
            sendEvent(event);
            m_nodeMediaChangedSignal(n);
        }
    }

    void IPGraph::inputsChanged(IPNode* n)
    {
        if (n && !n->group()) // top level only
        {
            TwkApp::GenericStringEvent event("graph-node-inputs-changed", this,
                                             n->name());
            sendEvent(event);
            m_nodeInputsChangedSignal(n);
        }
    }

    void IPGraph::propertyDidInsert(const Property* p, size_t index,
                                    size_t size)
    {
        ostringstream str;
        const IPNode* pc = dynamic_cast<const IPNode*>(p->container());
        const Component* c = pc->componentOf(p);

        str << pc->name() << "." << c->name() << "." << p->name();
        str << ";" << index << ";" << size;

        TwkApp::GenericStringEvent event("graph-state-change-insert", this,
                                         str.str());
        sendEvent(event);
    }

    void IPGraph::propertyWillInsert(const Property* p, size_t index,
                                     size_t size)
    {
        ostringstream str;
        const IPNode* pc = dynamic_cast<const IPNode*>(p->container());
        const Component* c = pc->componentOf(p);

        str << pc->name() << "." << c->name() << "." << p->name();
        str << ";" << index << ";" << size;

        TwkApp::GenericStringEvent event("graph-state-will-insert", this,
                                         str.str());
        sendEvent(event);
    }

    void IPGraph::newPropertyCreated(const Property* p)
    {
        ostringstream str;
        const IPNode* pc = dynamic_cast<const IPNode*>(p->container());
        const Component* c = pc->componentOf(p);

        str << Property::layoutAsString(p->layoutTrait()) << ";"
            << p->xsizeTrait() << "," << p->ysizeTrait() << ","
            << p->zsizeTrait() << "," << p->wsizeTrait() << ";";

        str << pc->name() << "." << c->name() << "." << p->name();

        TwkApp::GenericStringEvent event("graph-new-property", this, str.str());
        sendEvent(event);
    }

    void IPGraph::propertyWillBeDeleted(const Property* p)
    {
        ostringstream str;
        const IPNode* pc = dynamic_cast<const IPNode*>(p->container());
        const Component* c = pc->componentOf(p);

        str << Property::layoutAsString(p->layoutTrait()) << ";"
            << p->xsizeTrait() << "," << p->ysizeTrait() << ","
            << p->zsizeTrait() << "," << p->wsizeTrait() << ";";

        str << pc->name() << "." << c->name() << "." << p->name();

        TwkApp::GenericStringEvent event("graph-property-will-delete", this,
                                         str.str());
        sendEvent(event);
    }

    void IPGraph::propertyDeleted(const std::string& name)
    {
        TwkApp::GenericStringEvent event("graph-property-did-delete", this,
                                         name);
        sendEvent(event);
    }

    void IPGraph::setLookBehindFraction(float f)
    {
        TWK_CACHE_LOCK(m_fbcache, "lookBehind");

        m_fbcache.setLookBehindFraction(f);
        awakenAllCachingThreads();

        TWK_CACHE_UNLOCK(m_fbcache, "lookBehind");
    }

    void IPGraph::nodeConnections(StringPairVector& connections,
                                  bool toplevel) const
    {
        connections.clear();

        for (NodeMap::const_iterator i = m_nodeMap.begin();
             i != m_nodeMap.end(); ++i)
        {
            IPNode* n = (*i).second;

            if (n->protocol() == "Root" || n->protocol() == "RenderOutput"
                || n->protocol() == "AudioWaveform")
                continue;

            if (!n->isWritable())
                continue;

            if (!toplevel || !n->group())
            {
                const IPNode::IPNodes& inputs = n->inputs();

                for (size_t q = 0; q < inputs.size(); q++)
                {
                    IPNode* innode = inputs[q];
                    if (!innode->isWritable())
                        continue;
                    //
                    //  NOTE: data flow direction not dependency direction
                    //  (i.e. from input node to output node)
                    //
                    connections.push_back(
                        StringPair(innode->name(), n->name()));
                }
            }
        }
    }

    string IPGraph::uniqueName(const string& inname) const
    {
        string name = canonicalNodeName(inname);
        static char buf[16];
        static RegEx endRE("(.*[^0-9])([0-9]+)");
        static RegEx midRE(
            "(.*[^0-9])([0-9][0-9][0-9][0-9][0-9][0-9])([^0-9].*)");

        if (m_nodeMap.count(name) > 0)
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
                } while (m_nodeMap.count(finalName) > 0);

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
                } while (m_nodeMap.count(finalName) > 0);

                return finalName;
            }

            //
            //  Fallback: add a number to the end and recurse
            //
            ostringstream str;
            str << name << "000002";
            return uniqueName(str.str());
        }

        return name;
    }

    bool IPGraph::hasConstructor(const string& typeName)
    {
        return nodeDefinition(typeName) != NULL;
    }

    const NodeDefinition* IPGraph::nodeDefinition(const string& typeName)
    {
        return m_nodeManager->definition(typeName);
    }

    DisplayGroupIPNode*
    IPGraph::newDisplayGroup(const std::string& nodeName,
                             const TwkApp::VideoDevice* device)
    {
        DisplayGroupIPNode* group =
            newNodeOfType<DisplayGroupIPNode>("DisplayGroup", nodeName);
        if (group)
            group->setPhysicalVideoDevice(device);
        return group;
    }

    OutputGroupIPNode*
    IPGraph::newOutputGroup(const std::string& nodeName,
                            const TwkApp::VideoDevice* device)
    {
        OutputGroupIPNode* group =
            newNodeOfType<OutputGroupIPNode>("OutputGroup", nodeName);
        if (group)
            group->setPhysicalVideoDevice(device);
        return group;
    }

    IPNode* IPGraph::newNode(const string& typeName, const string& nodeName,
                             GroupIPNode* group)
    {
        if (IPNode* node =
                m_nodeManager->newNode(typeName, nodeName, this, group))
        {
            if (node->definition()->userVisible() && node->group() == NULL)
            {
                m_viewNodeMap[node->name()] = node;
            }

            m_topologyChanged = true;

            vector<string> info(4);
            ostringstream str;
            str << node->protocolVersion();
            info[0] = node->name();
            info[1] = node->protocol();
            info[2] = str.str();
            info[3] = group ? group->name() : "";

            TwkApp::GenericStringEvent event("graph-new-node", this,
                                             node->name());
            event.setStringContentVector(info);
            sendEvent(event);

            m_newNodeSignal(node);

            return node;
        }
        else
        {
            return 0;
        }
    }

    void IPGraph::setProfilingClock(Timer* t) { m_profilingTimer = t; }

    double IPGraph::profilingElapsedTime()
    {
        return m_profilingTimer->elapsed();
    }

    IPGraph::EvalProfilingRecord& IPGraph::beginProfilingSample()
    {
        m_profilingSamples.resize(m_profilingSamples.size() + 1);
        return m_profilingSamples.back();
    }

    IPGraph::EvalProfilingRecord& IPGraph::currentProfilingSample()
    {
        return m_profilingSamples.back();
    }

    void IPGraph::updateHasAudioStatus(IPNode* node)
    {
        bool oldHasAudio = hasAudio();

        if (node->hasAudio())
        {
            m_audioSources.insert(node);
        }
        else
        {
            m_audioSources.erase(node);
        }

        if (oldHasAudio != hasAudio())
        {
            flushAudioCache();
        }
    }

    bool IPGraph::isMediaLoading() const
    {
        std::unique_lock<std::mutex> lock{m_mediaLoadingSetMutex};
        return !m_mediaLoadingSet.empty();
    }

    void IPGraph::mediaLoadingBegin(WorkItemID mediaWorkItemId)
    {
        HOP_PROF_FUNC();

        // Add media to the set
        std::unique_lock<std::mutex> lock{m_mediaLoadingSetMutex};
        m_mediaLoadingSet.insert(mediaWorkItemId);
    }

    void IPGraph::mediaLoadingEnd(WorkItemID mediaWorkItemId)
    {
        HOP_PROF_FUNC();

        std::unique_lock<std::mutex> lock{m_mediaLoadingSetMutex};

        // Remove media from the set
        const bool mediaRemoved = m_mediaLoadingSet.erase(mediaWorkItemId) != 0;

        // Reenable caching if the media was actually removed and it was the
        // last one in the set.
        // Note that we check first if the media was actually removed in case it
        // gets removed twice which can possibly happens if a media loading gets
        // cancelled.
        if (mediaRemoved && m_mediaLoadingSet.empty())
        {
            lock.unlock();

            if (m_cacheMode != NeverCache)
            {
                redispatchCachingThread();
            }

            m_mediaLoadingSetEmptySignal();
        }
    }

    IPGraph::WorkItemID IPGraph::addWorkItem(const VoidFunction& function,
                                             const char* tag)
    {
        auto jobDispatcher = reinterpret_cast<JobDispatcher*>(m_jobDispatcher);

        return (WorkItemID)jobDispatcher->addJob(WorkItem(function, tag));
    }

    void IPGraph::removeWorkItem(WorkItemID id)
    {
        auto jobDispatcher = reinterpret_cast<JobDispatcher*>(m_jobDispatcher);
        jobDispatcher->removeJob(id);
    }

    void IPGraph::waitWorkItem(WorkItemID id)
    {
        auto jobDispatcher = reinterpret_cast<JobDispatcher*>(m_jobDispatcher);
        jobDispatcher->waitJob(id);
    }

    void IPGraph::prioritizeWorkItem(WorkItemID id)
    {
        if (id)
        {
            auto jobDispatcher =
                reinterpret_cast<JobDispatcher*>(m_jobDispatcher);
            jobDispatcher->prioritizeJob(id);
        }
    }

} // namespace IPCore
