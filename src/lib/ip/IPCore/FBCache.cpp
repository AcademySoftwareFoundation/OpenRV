//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <IPCore/FBCache.h>
#include <IPCore/IPGraph.h>
#include <IPCore/IPImage.h>
#include <IPCore/Application.h>
#include <TwkUtil/EnvVar.h>
#include <TwkUtil/ThreadName.h>
#include <TwkUtil/Timer.h>

static ENVVAR_BOOL(evActiveTailCaching, "RV_ACTIVE_TAIL_CACHING", false);

namespace IPCore
{
    using namespace std;
    using namespace TwkMath;

#if 0
#define DB_GENERAL 0x01
#define DB_EDGES 0x02
#define DB_FLUSH 0x04
#define DB_REF 0x08
#define DB_FREE 0x10
#define DB_CLEAR 0x20
#define DB_PROMOTE 0x40
#define DB_SIZE 0x80
#define DB_UTIL 0x180
#define DB_ALL 0xfff

//  #define DB_LEVEL        (DB_FREE | DB_GENERAL | DB_EDGES)
//  #define DB_LEVEL        DB_FREE
//  #define DB_LEVEL        (DB_ALL & (~ DB_EDGES))
//  #define DB_LEVEL        (DB_FLUSH | DB_REF)
//  #define DB_LEVEL        DB_GENERAL
//  #define DB_LEVEL        (DB_EDGES | DB_GENERAL)
//#define DB_LEVEL        (DB_UTIL)
//#define DB_LEVEL        (DB_CLEAR | DB_FLUSH | DB_PROMOTE)
//#define DB_LEVEL        (DB_EDGES | DB_UTIL)
//#define DB_LEVEL        DB_EDGES
//#define DB_LEVEL        DB_ALL
#define DB_LEVEL DB_PROMOTE

#define DB(x)                  \
    if (DB_GENERAL & DB_LEVEL) \
    cerr << dec << "FBCache: " << x << endl
#define DBL(level, x)     \
    if (level & DB_LEVEL) \
    cerr << dec << "FBCache: " << x << endl
#else
#define DB(x)
#define DBL(level, x)
#define DB_LEVEL 0
#endif

#define NAF (std::numeric_limits<int>::min())

#define utilityMax std::numeric_limits<float>::max()
#define utilityMin std::numeric_limits<float>::min()

#define OPTIMIZED_UTILITY_CACHING

    bool FBCache::m_cacheOutsideRegion =
        false; // continue caching when in/out region is full ?

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

    class CacheEdges
    {
    public:
        enum EdgeType
        {
            LeftOnly,   // there are additional target frames only to the left
            RightOnly,  // there are additional target frames only to the right
            BothSides,  // there are additional target frames on both sides
            NeitherSide // there are additional target frames on neither side
        };

        class Edge
        {
        public:
            Edge()
                : frame(NAF)
                , type(NeitherSide) {};
            Edge(int f, EdgeType t)
                : frame(f)
                , type(t) {};

            int frame;
            EdgeType type;
        };

        typedef std::map<int, Edge> EdgeSet;
        typedef std::vector<Edge> EdgeVector;

        CacheEdges(FBCache* c)
            : m_cache(c) {};

        void addCacheEdge(int frame);
        void removeCacheEdge(int frame);
        void clear();

        //
        //  Fill edges with Edges that are candidates for freeing.
        //  The EdgeType describes the directions in which free frames
        //  lie. (so you need to go the opposite direction to find more
        //  frames to free.)
        //
        void possibleFreeTargets(EdgeVector& edges);

        //
        //  Fill edges with Edges that are candidates for caching.  The
        //  EdgeType describes the directions in which additional
        //  uncached  frames may lie.
        //
        void possibleCacheTargets(EdgeVector& edges);

        string typeToText(EdgeType t);

    private:
        void addEdge(Edge e);
        void removeEdge(int frame);
        void dumpEdges();
        void rationalizeEdges();

        FBCache* m_cache;
        //
        //  Each element of m_edges represents a cached frame with an
        //  uncached frame to one or both sides.  The EdgeType describes
        //  the direction in which the uncached frames lie.
        //
        EdgeSet m_edges;
    };

    void CacheEdges::addEdge(Edge e)
    {
        m_edges[e.frame] = e;
        DBL(DB_EDGES, "addEdge: added frame " << e.frame << " type "
                                              << typeToText(e.type) << " ( "
                                              << m_edges.size() << " edges)");
    }

    void CacheEdges::removeEdge(int frame)
    {
        EdgeSet::iterator i = m_edges.find(frame);
        if (m_edges.end() != i)
        {
            DBL(DB_EDGES, "removeEdge: removed frame "
                              << i->second.frame << " type "
                              << typeToText(i->second.type) << " ( "
                              << m_edges.size() << " edges)");
            m_edges.erase(i);
        }
        else
        {
            DBL(DB_EDGES, "removeEdge: removing frame "
                              << frame << ": not found! ( " << m_edges.size()
                              << " edges)");
        }
    }

    void CacheEdges::dumpEdges()
    {
        DBL(DB_EDGES, "---------------- " << m_edges.size()
                                          << " edges --------------------");
        for (EdgeSet::iterator i = m_edges.begin(); i != m_edges.end(); ++i)
        {
            DBL(DB_EDGES,
                "    " << i->second.frame << " " << typeToText(i->second.type));
        }
    }

    void CacheEdges::rationalizeEdges()
    {
        for (EdgeSet::iterator i = m_edges.begin(); i != m_edges.end();)
        {
            int f = i->second.frame;
            bool thisCached = m_cache->isFrameCached(f);
            bool leftIsCached = (f > m_cache->m_minFrame)
                                    ? m_cache->isFrameCached(f - 1)
                                    : true;
            bool rightIsCached = (f < m_cache->m_maxFrame - 1)
                                     ? m_cache->isFrameCached(f + 1)
                                     : true;
            EdgeType newType;

            if (!thisCached)
            {
                //  Usually not an error, since frames are freed and
                //  this function can be called before removeCachedEdge
                //  is called.
                //
                DBL(DB_EDGES, "ERROR: edge frame " << f << " is not cached!");
            }
            if (leftIsCached && rightIsCached)
                newType = NeitherSide;
            else if (leftIsCached)
                newType = RightOnly;
            else if (rightIsCached)
                newType = LeftOnly;
            else
                newType = BothSides;

            if (newType != i->second.type || newType == NeitherSide)
            {
                DBL(DB_EDGES, "ERROR: rationalizeEdges had to fix things up: "
                                  << f << " " << typeToText(newType));
            }
            EdgeSet::iterator tmp = i;
            ++i;
            if (newType == NeitherSide)
                m_edges.erase(tmp);
            else
                (i->second.type = newType);
        }
    }

    string CacheEdges::typeToText(EdgeType t)
    {
        switch (t)
        {
        case LeftOnly:
            return "LeftOnly";
        case RightOnly:
            return "RightOnly";
        case BothSides:
            return "BothSides";
        case NeitherSide:
            return "NeitherSide";
        }
        return "Unknown!";
    }

    void CacheEdges::addCacheEdge(int frame)
    {
        bool cached = m_cache->isFrameCached(frame);
        DBL(DB_EDGES, "addCacheEdge: frame " << frame << " cached: " << cached);
        if (!cached)
            return;
        if (0 != m_edges.count(frame))
        {
            // DBL(DB_EDGES, "ERROR: cached edge duplicate! (" << frame << ")");
            //
            //   This is not an error, addCachedEdge will be called
            //   multiple times for frames that have multiple
            //   framebuffers.
            //
            return;
        }

        EdgeType thisType = BothSides;

        //
        //  Adjust the types of (and possibly erase) any
        //  CacheEdges that neighbor this one, and determine the type of
        //  this edge.
        //

        if (frame > m_cache->m_minFrame)
        //
        //  Check frame to left of this one.
        //
        {
            EdgeSet::iterator leftEdge = m_edges.find(frame - 1);

            if (m_edges.end() != leftEdge)
            //
            // Frame to the left was a cached edge
            //
            {
                thisType = RightOnly;
                DBL(DB_EDGES,
                    "addCacheEdge: frame "
                        << frame << ", edge on left: " << leftEdge->second.frame
                        << " " << typeToText(leftEdge->second.type));

                if (BothSides == leftEdge->second.type)
                    leftEdge->second.type = LeftOnly;
                else if (RightOnly == leftEdge->second.type)
                    removeEdge(leftEdge->second.frame);
            }
            else if (m_cache->isFrameCached(frame - 1))
                thisType = RightOnly;
        }
        else
            thisType = RightOnly;

        if (frame < m_cache->m_maxFrame - 1)
        //
        //  Check frame to right of this one.
        //
        {
            EdgeSet::iterator rightEdge = m_edges.find(frame + 1);

            if (m_edges.end() != rightEdge)
            //
            //  Frame to the right was a cached edge
            //
            {
                thisType = (thisType == RightOnly) ? NeitherSide : LeftOnly;
                DBL(DB_EDGES, "addCacheEdge: frame "
                                  << frame << ", edge on right: "
                                  << rightEdge->second.frame << " "
                                  << typeToText(rightEdge->second.type));

                if (BothSides == rightEdge->second.type)
                    rightEdge->second.type = RightOnly;
                else if (LeftOnly == rightEdge->second.type)
                    removeEdge(rightEdge->second.frame);
            }
            else if (m_cache->isFrameCached(frame + 1))
                thisType = (thisType == RightOnly) ? NeitherSide : LeftOnly;
        }
        else
            thisType = (thisType == RightOnly) ? NeitherSide : LeftOnly;

        if (NeitherSide != thisType)
            addEdge(Edge(frame, thisType));

        DBL(DB_EDGES, "addCacheEdge: frame " << frame << " done, thisType "
                                             << typeToText(thisType));
//  rationalizeEdges();
#if 0
    //  sanity check
    if (!m_cache->isFrameCached(frame)) DBL (DB_EDGES, "INSANITY: new edge frame is not cached");
    EdgeSet::iterator leftEdge = m_edges.find (frame - 1);
    if (m_edges.end() != leftEdge)
    {
        if (BothSides == thisType || LeftOnly == thisType) 
            DBL (DB_EDGES, "INSANITY: new edge has wrong type (1)");
        if (leftEdge->second.type == BothSides || leftEdge->second.type == RightOnly)
            DBL (DB_EDGES, "INSANITY: new edge's left edge has wrong type");
    }
    EdgeSet::iterator rightEdge = m_edges.find (frame + 1);
    if (m_edges.end() != rightEdge)
    {
        if (BothSides == thisType || RightOnly == thisType) 
            DBL (DB_EDGES, "INSANITY: new edge has wrong type (2)");
        if (rightEdge->second.type == BothSides || rightEdge->second.type == LeftOnly)
            DBL (DB_EDGES, "INSANITY: new edge's right edge has wrong type");
    }
    dumpEdges();
#endif
    }

    void CacheEdges::removeCacheEdge(int frame)
    {
        DBL(DB_EDGES, "removeCacheEdge: frame " << frame);

        if (frame > m_cache->m_minFrame)
        //
        //  Check frame to left of this one.
        //
        {
            EdgeSet::iterator leftEdge = m_edges.find(frame - 1);

            if (m_edges.end() != leftEdge)
            //
            //  Adjust existing edge
            //
            {
                DBL(DB_EDGES, "removeCacheEdge: found edge to left: "
                                  << leftEdge->second.frame << " "
                                  << typeToText(leftEdge->second.type));
                if (LeftOnly == leftEdge->second.type)
                {
                    leftEdge->second.type = BothSides;
                    DBL(DB_EDGES,
                        "removeCacheEdge: adjusting edge to the left: "
                            << leftEdge->second.frame << " "
                            << typeToText(leftEdge->second.type));
                }
            }
            else if (m_cache->isFrameCached(frame - 1))
            //
            //  Make new edge
            //
            {
                EdgeType t = BothSides;

                if (frame < m_cache->m_minFrame + 2
                    || m_cache->isFrameCached(frame - 2))
                {
                    t = RightOnly;
                }
                DBL(DB_EDGES, "removeCacheEdge: adding edge to left: "
                                  << frame - 1 << " " << typeToText(t));
                addEdge(Edge(frame - 1, t));
            }
        }

        if (frame < m_cache->m_maxFrame - 1)
        //
        //  Check frame to right of this one.
        //
        {
            EdgeSet::iterator rightEdge = m_edges.find(frame + 1);

            if (m_edges.end() != rightEdge)
            //
            //  Adjust existing edge
            //
            {
                DBL(DB_EDGES, "removeCacheEdge: found edge to right: "
                                  << rightEdge->second.frame << " "
                                  << typeToText(rightEdge->second.type));
                if (RightOnly == rightEdge->second.type)
                {
                    rightEdge->second.type = BothSides;
                    DBL(DB_EDGES,
                        "removeCacheEdge: adjusting edge to the right: "
                            << rightEdge->second.frame << " "
                            << typeToText(rightEdge->second.type));
                }
            }
            else if (m_cache->isFrameCached(frame + 1))
            //
            //  Make new edge
            //
            {
                EdgeType t = BothSides;

                if (frame >= m_cache->m_maxFrame - 2
                    || m_cache->isFrameCached(frame + 2))
                {
                    t = LeftOnly;
                }
                DBL(DB_EDGES, "removeCacheEdge: adding edge to right: "
                                  << frame + 1 << " " << typeToText(t));
                addEdge(Edge(frame + 1, t));
            }
        }

        if (m_edges.find(frame) != m_edges.end())
            removeEdge(frame);
        DBL(DB_EDGES, "removeCacheEdge: frame " << frame << " done");
        //  rationalizeEdges();
        //  dumpEdges();
    }

    void CacheEdges::clear()
    {
        DBL(DB_EDGES, "clear: removing all edges");
        m_edges.clear();
    }

    void CacheEdges::possibleFreeTargets(EdgeVector& edges)
    {
        DBL(DB_EDGES, "possibleFreeTargets: " << m_edges.size() << " edges");
        edges.clear();

        int minF = m_cache->m_minFrame;
        int maxF = m_cache->m_maxFrame;
        int inF = m_cache->m_inFrame;
        int outF = m_cache->m_outFrame;

        if (m_cache->isFrameCached(minF))
            edges.push_back(Edge(minF, RightOnly));

        if (m_cache->isFrameCached(maxF - 1))
            edges.push_back(Edge(maxF - 1, LeftOnly));

        if (inF != minF && m_cache->isFrameCached(inF))
            edges.push_back(Edge(inF, BothSides));

        if (outF != maxF && m_cache->isFrameCached(outF - 1))
            edges.push_back(Edge(outF - 1, BothSides));

        int dp1 = m_cache->m_displayFrame + 1;
        int dm1 = m_cache->m_displayFrame - 1;

        if (dp1 < maxF - 1 && m_cache->isFrameCached(dp1))
            edges.push_back(Edge(dp1, RightOnly));
        if (dm1 > minF && m_cache->isFrameCached(dm1))
            edges.push_back(Edge(dm1, LeftOnly));

        for (EdgeSet::iterator i = m_edges.begin(); i != m_edges.end(); ++i)
        {
            edges.push_back(i->second);
        }
        DBL(DB_EDGES,
            "possibleFreeTargets: returning " << edges.size() << " edges");
    }

    void CacheEdges::possibleCacheTargets(EdgeVector& edges)
    {
        DBL(DB_EDGES, "possibleCacheTargets: " << m_edges.size() << " edges");
        edges.clear();

        int minF = m_cache->m_minFrame;
        int maxF = m_cache->m_maxFrame;
        int inF = m_cache->m_inFrame;
        int outF = m_cache->m_outFrame;

        if (m_cache->isActiveTailCachingEnabled())
        {
            if (!m_cache->isFrameCached(m_cache->m_displayFrame))
                edges.push_back(Edge(m_cache->m_displayFrame, BothSides));

            if (!m_cache->isFrameCached(minF))
                edges.push_back(Edge(minF, RightOnly));

            if (!m_cache->isFrameCached(maxF - 1))
                edges.push_back(Edge(maxF - 1, LeftOnly));

            if (inF != minF && !m_cache->isFrameCached(inF))
                edges.push_back(Edge(inF, BothSides));

            if (outF != maxF && !m_cache->isFrameCached(outF - 1))
                edges.push_back(Edge(outF - 1, BothSides));
        }
        else
        {
            const bool forward = m_cache->m_displayInc >= 0;

            if (!m_cache->isFrameCached(m_cache->m_displayFrame))
            {
                edges.push_back(Edge(m_cache->m_displayFrame,
                                     forward ? RightOnly : LeftOnly));
            }

            if (!m_cache->isFrameCached(minF) && forward)
            {
                edges.push_back(Edge(minF, RightOnly));
            }

            if (!m_cache->isFrameCached(maxF - 1) && !forward)
            {
                edges.push_back(Edge(maxF - 1, LeftOnly));
            }

            if (inF != minF && !m_cache->isFrameCached(inF))
            {
                edges.push_back(Edge(inF, forward ? RightOnly : LeftOnly));
            }

            if (outF != maxF && !m_cache->isFrameCached(outF - 1))
            {
                edges.push_back(Edge(outF - 1, forward ? RightOnly : LeftOnly));
            }
        }

        for (EdgeSet::iterator i = m_edges.begin(); i != m_edges.end(); ++i)
        {
            Edge& e = i->second;

            if (LeftOnly == e.type || BothSides == e.type)
            {
                edges.push_back(Edge(e.frame - 1, LeftOnly));
            }
            if (RightOnly == e.type || BothSides == e.type)
            {
                edges.push_back(Edge(e.frame + 1, RightOnly));
            }
        }
        DBL(DB_EDGES,
            "possibleCacheTargets: returning " << edges.size() << " edges");
    }

    //
    //  The "per-node cache" is a supplementary special-purpose cache designed
    //  to hold framebuffers required for rendering, but which have no obvious
    //  relation to the frames in the timeline.  In particular the Crank
    //  "preview" image and the "tiles" in the shelf.  These framebuffers must
    //  be cached, but there is no point in caching more than one framebuffer
    //  per node.  Hence this extra cache.
    //
    //  Notes:
    //
    //  * XXX  Beware: If a node uses the per-node cache, the subtree rooted at
    //  the
    //    node can only produce at most one FB (IE no layers, stereo, combines,
    //    merges, etc).
    //
    //  * The per-node cache is like the frame cache component of the FBCache in
    //    that it really only manages referencing.  The lower-level TwkFB::Cache
    //    is the thing that will delete framebuffers whose ref count falls too
    //    low, etc.
    //
    //  * Caching threads do not "see" the per-node cache.  IE they do not add
    //    framebuffers to it, or cause referrencing of FBs in it.
    //

    class PerNodeCache
    {
    public:
        typedef TwkFB::FrameBuffer FrameBuffer;
        typedef std::map<const IPNode*, FrameBuffer*> FromNodeMap;
        typedef std::map<string, FrameBuffer*> FromIDMap;
        typedef std::set<string> CachableItemMap;

        PerNodeCache(FBCache* c);
        ~PerNodeCache();

        //
        //  Returns true only if the fb was actually added to the per-node
        //  cache.
        //
        bool add(FrameBuffer*, const IPNode*);

        void flush(FrameBuffer*, const IPNode*);
        FrameBuffer* fbOfNode(const IPNode*);
        void clear();
        // bool isCached(string id) const { return (m_idMap.find(id) !=
        // m_idMap.end()); }

        //
        //  CachableItems management
        //

        void pushItem(string name);
        string popItem();

    private:
        void flushInternal(FrameBuffer*, const IPNode*);

        //
        //  Internal data held in two unrelated chunks, each with it's own lock.
        //  mapsLock for the actual cache, and cachableItemsLock for the list of
        //  names of cachable items.
        //

        FromNodeMap m_nodeMap;
        FromIDMap m_idMap;

        mutable pthread_mutex_t m_mapsLock;

        CachableItemMap m_cachableItems;

        mutable pthread_mutex_t m_cachableItemsLock;

        FBCache* m_cache;
    };

    PerNodeCache::PerNodeCache(FBCache* c)
        : m_cache(c)
    {
        pthread_mutex_init(&m_cachableItemsLock, 0);
        pthread_mutex_init(&m_mapsLock, 0);
    }

    PerNodeCache::~PerNodeCache()
    {
        clear();
        pthread_mutex_destroy(&m_cachableItemsLock);
        pthread_mutex_destroy(&m_mapsLock);
    }

    bool PerNodeCache::add(FrameBuffer* fb, const IPNode* node)
    {
        LockObject l(m_mapsLock);

        FromNodeMap::iterator i = m_nodeMap.find(node);

        if (i != m_nodeMap.end())
        {
            if (fb && i->second->identifier() == fb->identifier())
                return false;

            //  cerr << "per-node: flushing " << i->second->identifier() <<
            //  endl;
            flushInternal(i->second, node);
        }

        if (fb)
        {
            m_cache->referenceFB(fb);

            m_nodeMap[node] = fb;
            m_idMap[fb->identifier()] = fb;
        }

        //
        //  "true" hear means we changed the cache, by flushing the old fb
        //  reference and possibly adding a new one.
        //
        return true;
    }

    void PerNodeCache::flush(FrameBuffer* fb, const IPNode* node)
    {
        LockObject l(m_mapsLock);

        PerNodeCache::flushInternal(fb, node);
    }

    void PerNodeCache::flushInternal(FrameBuffer* fb, const IPNode* node)
    {
        m_nodeMap.erase(node);

        //   XXX this is wrong!  What if two nodes end up caching same FB hence
        //   same identifier.  then this erases that from idMap and dereferences
        //   it, so could cause deletion ?

        m_idMap.erase(fb->identifier());

        m_cache->dereferenceFB(fb);
    }

    TwkFB::FrameBuffer* PerNodeCache::fbOfNode(const IPNode* node)
    {
        LockObject l(m_mapsLock);

        FromNodeMap::iterator i = m_nodeMap.find(node);

        if (i != m_nodeMap.end())
            return i->second;

        return 0;
    }

    void PerNodeCache::clear()
    {
        //  No need to deleted anything here, since the maps just hold pointers,
        //  but we must deref framebuffers.

        LockObject l(m_mapsLock);

        for (FromIDMap::iterator i = m_idMap.begin(); i != m_idMap.end(); ++i)
        {
            m_cache->dereferenceFB(i->second);
        }

        m_idMap.clear();
        m_nodeMap.clear();
    }

    void PerNodeCache::pushItem(string name)
    {
        LockObject l(m_cachableItemsLock);

        m_cachableItems.insert(name);
    }

    string PerNodeCache::popItem()
    {
        string name;

        LockObject l(m_cachableItemsLock);

        if (m_cachableItems.size())
        {
            CachableItemMap::iterator i = m_cachableItems.begin();
            name = *i;
            m_cachableItems.erase(i);
        }

        return name;
    }

    FBCache::FBCache(IPGraph* g)
        : TwkFB::Cache()
        , m_graph(g)
        , m_displayFrame(NAF)
        , m_displayInc(0)
        , m_displayFPS(0)
        , m_minFrame(NAF)
        , m_maxFrame(NAF)
        , m_inFrame(NAF)
        , m_outFrame(NAF)
        , m_cacheFrame(NAF)
        , m_freeMode(ConservativeFreeMode)
        , m_overflowBoundary(0)
        , m_utilityStateChanged(false)
        , m_targetCacheFrameUtility(utilityMax)
        , m_lookBehindFraction(25.0)
        , m_activeTailCachingEnabled(false)
        , m_cacheStatsDisabled(false)
        , m_cacheStatsDirty(true)
    {
        m_cacheEdges = new CacheEdges(this);
        m_perNodeCache = new PerNodeCache(this);
        pthread_mutex_init(&m_statMutex, 0);

        m_cacheStatsDisabled =
            IPCore::App()->optionValue<bool>("disableCacheStats", false);

        m_activeTailCachingEnabled = evActiveTailCaching.getValue();
        if (m_activeTailCachingEnabled)
        {
            cout << "INFO: Active tail caching enabled" << std::endl;
        }
    }

    FBCache::~FBCache()
    {
        lock();
        clearInternal();
        delete m_cacheEdges;
        delete m_perNodeCache;
        unlock();
        pthread_mutex_destroy(&m_statMutex);
    }

    bool FBCache::hasPartialFrameCache(int frame) const
    {
        FBVector cached;

        (void)frameItems(frame, cached);
        for (int i = 0; i < cached.size(); ++i)
        {
            ItemFrames::const_iterator ifi = m_itemMap.find(cached[i]);
            if (ifi != m_itemMap.end())
            {
                //
                //  Some of the items in this map are just "references"
                //  to images cached originally for other frames.  If
                //  so, they will be referenced by more than one frame.
                //  If on the other hand, there is exactly one frame
                //  referencing this image, we know it is this one, and
                //  the frame is truly "partially cached".
                //
                if (1 == ifi->second.size())
                    return true;
            }
        }

        return false;
    }

    void FBCache::setMemoryUsage(size_t bytes)
    {
        size_t oldCapacity = capacity();
        TwkFB::Cache::setMemoryUsage(bytes);

        if (capacity() != oldCapacity)
        {
            DBL(DB_SIZE, "setMemoryUsage() bytes " << bytes << " was "
                                                   << oldCapacity << " used "
                                                   << used());
            m_utilityStateChanged = true;
            m_overflowBoundary = 0;
        }
    }

    bool FBCache::overflowing() const
    {
        //
        //  overflowing() is used to tell whether we can continue
        //  caching additional frames, so if we previously exceeded the
        //  cache size when caching a frame, and set m_overflowBoundary
        //  to the cache size we were at before we cached that frame,
        //  use that size instead of the max size.
        //
        //  If we've never exceeded the cache size before, assume we're
        //  not overflowing.
        //
        //  _But_ if we have trash items to free, assume we're not
        //  overflowing even if the cache is full.
        //
        size_t u = used();
        size_t o = m_overflowBoundary;
        return (0 != o) ? (u >= o && (trashCount() == 0)) : false;
    }

    struct FramePromoter
    {
        FramePromoter(FBCache& c)
            : cache(c)
            , missedOne(false)
        {
        }

        FBCache& cache;
        FBCache::IDSet ids;
        bool missedOne;

        void operator()(IPImageID* idp)
        {
            DBL(DB_PROMOTE,
                "FramePromoter idp " << idp << ", fb id " << idp->id);

            if (missedOne || idp->id == "")
                return;

            if (!cache.isCached(idp->id))
            {
                missedOne = true;
                DBL(DB_PROMOTE, "    missed!");
            }
            else
            {
                ids.insert(idp->id);
                DBL(DB_PROMOTE, "    hit!");
            }
        }
    };

    bool FBCache::promoteFrame(int frame, IPImageID* idTree)
    {
        FramePromoter fp(*this);

        foreach_ip(idTree, fp);
        DBL(DB_PROMOTE, "promoteFrame " << frame << ", " << fp.ids.size()
                                        << " cached, missedOne "
                                        << fp.missedOne);

        if (fp.missedOne)
            return false;

        //
        //  Add fbs to list of fbs for this frame.
        //
        for (IDSet::iterator i = fp.ids.begin(); i != fp.ids.end(); ++i)
        {
            referenceFrame(frame, m_map[*i]);
        }
        m_cacheEdges->addCacheEdge(frame);

        return true;
    }

    bool FBCache::add(FrameBuffer* fb, int frame, bool force,
                      const IPNode* node)
    {
        if (node)
        {
            if (!fb->inCache())
                TwkFB::Cache::add(fb, true);
            bool added = m_perNodeCache->add(fb, node);
            if (added)
                m_graph->textureCacheUpdated()();
            //
            //  Allways return true, since the fb _is_ cached at this point.
            //
            return true;
        }
        DB("add() frame " << frame << " (fb " << fb << ", " << fb->identifier()
                          << ") force " << force << " frame in cache "
                          << isFrameCached(frame));
        if (frame == NAF)
        {
            cout << "Frame == NAF" << endl;
            abort();
        }
        bool itemInCache = m_itemMap.count(fb) > 0;
        bool partiallyCached = hasPartialFrameCache(frame);

        /*
        FrameMap::const_iterator i = m_frames.find(frame);
        if (i != m_frames.end()) cerr << "FRAME " << frame << ", " <<
        i->second.size() << " items" << endl;
        */

        DB("    itemInCache " << itemInCache << " this frame in itemMap entry: "
                              << m_itemMap[fb].count(frame));
        if (itemInCache && m_itemMap[fb].count(frame) > 0)
        {
            //
            //  This frame is already referenced for this fb. Make sure
            //  its also referenced by this frame. But if this is the
            //  *first* item for this frame and the cache is already full,
            //  we need to return false immediately.
            //

            DB("    m_frames count "
               << m_frames[frame].count(fb->identifier()));
            if (m_full && !partiallyCached)
                return false;

            DBL(DB_REF,
                "add() possibly reffing " << fb << " " << fb->identifier());
            referenceFrame(frame, fb);

            checkMetadata();
            return true;
        }

        DB("add()     cached " << fb->inCache());
        DB("add()     current " << m_currentBytes << " max " << m_maxBytes
                                << " "
                                << double(m_currentBytes) / double(m_maxBytes));

        if (!fb->inCache())
        {
            if ((fb->totalImageSize() + used()) > capacity())
            //
            //  If cache is overflowing, try to make room by freeing
            //  trash fbs first.
            //
            {
                freeTrash(fb->totalImageSize());
            }

            if ((fb->totalImageSize() + used()) > capacity()
                && (0 == m_overflowBoundary || used() < m_overflowBoundary))
            //
            //  If the above didn't help, and we're going to overflow
            //  the cache with this addition, set the overfloat boundary
            //  to the current cache size.
            //
            {
                m_overflowBoundary = used();
            }

            //
            //  If the frame is partially complete, just let it cache the
            //  rest of it.  And we always allow caching of a single
            //  frame, no matter how big it is.
            //

            bool forceadd = partiallyCached || force || m_map.size() == 0;

            DB("attempting TwkFB::add() forceadd "
               << forceadd << " partially cached " << partiallyCached);
            m_targetCacheFrameUtility = utility(frame, FOR_CACHING);
            bool notfull = TwkFB::Cache::add(fb, forceadd);
            //
            //  Note that TwkFB::Cache::add may or may not have added
            //  the fb.  If "notfull" then it added the fb (or it was already
            //  there).  if "!notfull" then the cache is full and add()
            //  added the fb anyway if and only if "forceadd".
            //

            if (forceadd || notfull)
            //
            //  Then the fb was added to the cache, or was already
            //  there, even if the cache is full.
            //
            {
                if (Cache::debug())
                    cout << "ADDED: " << fb << " : " << fb->identifier()
                         << endl;
                DB("add() forceadd || notfull, " << fb);
                DB("    added frame " << frame << " id " << fb->identifier());

                DBL(DB_REF, "add() (2) possibly reffing " << fb << " "
                                                          << fb->identifier());
                referenceFrame(frame, fb);

                if (Cache::debug())
                {
                    for (IDSet::iterator i = m_frames[frame].begin();
                         i != m_frames[frame].end(); ++i)
                    {
                        cout << "  " << frame << " --> " << *i << endl;
                    }
                }

                setCacheStatsDirty();
                if (isFrameCached(frame))
                {
                    DBL(DB_EDGES, "add() calling addCacheEdge (1)"
                                      << frame << ", " << m_frames[frame].size()
                                      << " ids for this frame");
                    m_cacheEdges->addCacheEdge(frame);
                }

                //
                // WTF.  Code that calls this add() thinks we only
                // return false if we didn't add the fb to the cache.
                // but we did add it, even if the cache is full.  so
                // return true goddamnit.
                //
                // return notfull;
                checkMetadata();
                return true;
            }
            else
            {
                checkMetadata();
                return false;
            }
        }
        else
        {
            DB("add() fb was in cache " << fb);
            DBL(DB_REF,
                "add() (3) possibly reffing " << fb << " " << fb->identifier());
            referenceFrame(frame, fb);

            setCacheStatsDirty();
            if (isFrameCached(frame))
            {
                m_cacheEdges->addCacheEdge(frame);
                DBL(DB_EDGES, "add() calling addCacheEdge (2)"
                                  << frame << ", " << m_frames[frame].size()
                                  << " ids for this frame");
            }
        }

        checkMetadata();
        return true;
    }

    void FBCache::flushPerNodeCache(const IPNode* node)
    {
        m_perNodeCache->add(0, node);
    }

    TwkFB::FrameBuffer* FBCache::perNodeCacheContents(const IPNode* node) const
    {
        return m_perNodeCache->fbOfNode(node);
    }

    bool FBCache::freeIDSet(const IDSet& idset, int frame)
    {
        DB("freeIDSet frame " << frame << " cached: " << isFrameCached(frame));

        IDSet myIDset = idset;
        set<string> idsToFlush;

        for (IDSet::const_iterator id = myIDset.begin(); id != myIDset.end();
             ++id)
        {
            FBMap::iterator i = m_map.find(*id);

            if (i != m_map.end())
            {
                FrameBuffer* fb = i->second;
                ItemFrames::iterator item = m_itemMap.find(fb);

                if (item != m_itemMap.end())
                {
                    //
                    //  First remmove frame from m_itemMap for this id
                    //
                    if (item->second.count(frame))
                    {
                        item->second.erase(frame);
                    }

                    //
                    //  Now remove id from m_frames map entry for this
                    //  frame.
                    //
                    IDSet& idsetForFrame = m_frames[frame];
                    if (idsetForFrame.count(*id))
                    {
                        idsetForFrame.erase(*id);
                        DBL(DB_REF, "freeIDSet() dereferencing");
                        dereferenceFB(fb);
                    }

                    //
                    //  If that was the last frame that reffed this id,
                    //  mark id for flushing from all FBCache
                    //  (frame-level) data structures.
                    //
                    if (item->second.empty())
                    {
                        DB("freeIDSet() add to flush list: " << *id);
                        idsToFlush.insert(*id);
                    }
                }
            }
        }
        DB("freeIDSet() ready to flush " << idsToFlush.size() << " ids");
        for (set<string>::iterator itf = idsToFlush.begin();
             itf != idsToFlush.end(); ++itf)
        {
            DB("freeIDSet() flushing: " << *itf);
            flush(*itf);
        }

        DB("freeIDSet done with frame " << frame
                                        << " cached: " << isFrameCached(frame));

        return true;
    }

    //  XXX  these freeInRange functions no longer free anything,
    //  they're used only to clear frame-level info from the data
    //  structures.  should be rewritten or replaced.
    //  In the current usage we don't want them to free anything since
    //  it's used to clear the cache when turning off the cache, and you
    //  might just turn it on again.

    bool FBCache::freeInRangeForwards(size_t bytes, FrameVector& toBeErased,
                                      int minFrame, int maxFrame, bool greedy)
    {
        DBL(DB_FREE, "free forwards " << minFrame << " - " << maxFrame
                                      << " greedy " << greedy);

        bool enough = m_currentBytes < m_maxBytes
                      && (m_maxBytes - m_currentBytes >= bytes);

        size_t targetBytes = (m_maxBytes > bytes) ? m_maxBytes - bytes : 0;

        for (FrameMap::iterator fi = m_frames.begin();
             fi != m_frames.end() && (!enough || greedy); ++fi)
        {
            int frame = fi->first;

            //
            //  Note that this range must be inclusive, since functions
            //  that call this one believe it to be inclusive.
            //
            if (frame >= minFrame && frame <= maxFrame)
            {
                //
                //  Iterate over the ids that were cached for this frame
                //

                const IDSet& idset = fi->second;

                if (freeIDSet(idset, frame))
                {
                    toBeErased.push_back(frame);
                }
                //
                //  At best freeIDSet just flushes ids from the frame-level
                //  data structures, adding them to the Cache-level
                //  TrashCan, so call freeTrash to "really" free something.
                //
                /*
                if (m_currentBytes > targetBytes)
                {
                    TwkFB::Cache::freeTrash (m_currentBytes - targetBytes);
                }
                */

                enough = m_currentBytes < m_maxBytes
                         && (m_maxBytes - m_currentBytes >= bytes);
            }
        }

        setCacheStatsDirty();

        return (m_maxBytes - m_currentBytes >= bytes);
    }

    bool FBCache::freeInRangeBackwards(size_t bytes, FrameVector& toBeErased,
                                       int minFrame, int maxFrame, bool greedy)
    {
        DBL(DB_FREE, "free backwards " << minFrame << " - " << maxFrame
                                       << " greedy " << greedy);

        bool enough = m_currentBytes < m_maxBytes
                      && (m_maxBytes - m_currentBytes >= bytes);

        size_t targetBytes = (m_maxBytes > bytes) ? m_maxBytes - bytes : 0;

        for (FrameMap::reverse_iterator fi = m_frames.rbegin();
             fi != m_frames.rend() && (!enough || greedy); ++fi)
        {
            int frame = fi->first;

            //
            //  Note that this range must be inclusive, since functions
            //  that call this one believe it to be inclusive.
            //
            if (frame >= minFrame && frame <= maxFrame)
            {
                //
                //  Iterate over the ids that were cached for this frame
                //

                const IDSet& idset = fi->second;

                if (freeIDSet(idset, frame))
                {
                    toBeErased.push_back(frame);
                }
                //
                //  At best freeIDSet just flushes ids from the frame-level
                //  data structures, adding them to the Cache-level
                //  TrashCan, so call freeTrash to "really" free something.
                //
                /*
                if (m_currentBytes > targetBytes)
                {
                    TwkFB::Cache::freeTrash (m_currentBytes - targetBytes);
                }
                */

                enough = m_currentBytes < m_maxBytes
                         && (m_maxBytes - m_currentBytes >= bytes);
            }
        }

        setCacheStatsDirty();

        return (m_maxBytes - m_currentBytes >= bytes);
    }

    namespace
    {

        struct FrameCompare
        {
            FrameCompare(int df)
                : dFrame(df)
            {
            }

            bool operator()(int a, int b)
            {
                return (abs(dFrame - a) > abs(dFrame - b));
            }

            int dFrame;
        };

    }; // namespace

    void FBCache::clearFrameCaches()
    {
        m_framesScheduledForFreeing.clear();
        m_framesBeingCached.clear();

        FrameVector sortedFrames;
        for (FrameMap::iterator i = m_frames.begin(); i != m_frames.end(); ++i)
        {
            sortedFrames.push_back(i->first);
        }
        FrameCompare compOp(m_displayFrame);
        std::sort(sortedFrames.begin(), sortedFrames.end(), compOp);

        for (int i = 0; i < sortedFrames.size(); ++i)
        {
            freeIDSet(m_frames[sortedFrames[i]], sortedFrames[i]);
        }

        /* CBB

           It's possible that the above sorting, which helps things go into and
        out of the TrashCan in the right order (IE prevent thrashing), is
           unnecessary.  Maybe promotion just needs to be smarter.  If we figure
           that out, we should go back to the below instead of sorting.

        for (FrameMap::iterator i = m_frames.begin();
             i != m_frames.end();
             ++i)
        {
            freeIDSet(i->second, i->first);
        }
        */

        m_frames.clear();
        m_cacheEdges->clear();
        setCacheStatsDirty();
    }

    void FBCache::clearInternal()
    {
        DBL(DB_CLEAR, "clearInternal() current "
                          << m_currentBytes << " max " << m_maxBytes << " "
                          << double(m_currentBytes) / double(m_maxBytes));

        clearFrameCaches();
        //
        //  We no longer clear the lower-level cache here, since it has
        //  a trash collection scheme that should let us reuse fb's that
        //  are now associated with different frames.
        //
        // TwkFB::Cache::clearInternal();
    }

    void FBCache::clearAllButFrame(int frame, bool force)
    {
        DBL(DB_CLEAR, "clearAllButFrame() frame "
                          << frame << " m_frames size " << m_frames.size()
                          << " current " << m_currentBytes << " max "
                          << m_maxBytes << " "
                          << double(m_currentBytes) / double(m_maxBytes));

        //
        //  This only clears frame-level data unless "force" is true.
        //

        if (m_frames.size() == 0
            || (m_frames.size() == 1 && m_frames.begin()->first == frame))
        {
            //
            //  Nothing to do.
            //
            if (force)
                TwkFB::Cache::freeTrash(m_currentBytes);
            return;
        }

        FrameVector toBeErased;

        freeInRangeForwards(numeric_limits<size_t>::max(), toBeErased,
                            m_minFrame, frame - 1, true);

        freeInRangeBackwards(numeric_limits<size_t>::max(), toBeErased,
                             frame + 1, m_maxFrame - 1, true);

        for (size_t i = 0; i < toBeErased.size(); i++)
        {
            int si = toBeErased[i];
            m_frames.erase(si);
            m_cacheEdges->removeCacheEdge(si);
        }
        if (force)
            TwkFB::Cache::freeTrash(m_currentBytes);
    }

    //
    //  The value of the utility function determines caching policy:
    //
    //  * we try to cache frames with high utility
    //  * we try to free frames with low utility
    //  * we never cache frames with utility == 0.0
    //
    //  A valid utility function has these properties:
    //
    //  * produces values in the range: [0.0, utilityMax]
    //  * for a given set of cached frames, the utility functions
    //    extrema must occur on one of these frames: max/min, in/out,
    //    displayFrame, a frame the lies on the edge of a region of
    //    cached frames.
    //

    inline float FBCache::utility(int frame, UtilityMode mode)
    {
        if (m_graph->cachingMode() == IPGraph::GreedyCache)
        {
            //
            //  Region Cache
            //
            float d;

            if (m_inFrame > frame)
            {
                d = (m_cacheOutsideRegion) ? 1.0 / (1.0 + m_inFrame - frame)
                                           : 0.0;
            }
            else if (m_outFrame <= frame)
            {
                d = (m_cacheOutsideRegion) ? 1.0 / (2.0 + frame - m_outFrame)
                                           : 0.0;
            }
            else if (frame == m_inFrame)
            {
                d = utilityMax;
            }
            else
            {
                d = 1.0 + 1.0 / abs(frame - m_inFrame);
            }

            DBL(DB_UTIL, "utility(" << frame << ") = " << d << ", dsp "
                                    << m_displayFrame << " cacheOutside "
                                    << m_cacheOutsideRegion);
            return d;
        }

        //
        //  Lookahead Cache
        //

        if (m_displayFrame == frame)
            return utilityMax;

        int lastFrame = m_outFrame - 1;
        int firstFrame = m_inFrame;
        bool forward = (frame > m_displayFrame);

        const float fact =
            (mode == FOR_CACHING && !isActiveTailCachingEnabled())
                ? 0.001f
                : min(max(m_lookBehindFraction / 100.0f, 0.001f), 0.999f);
        //
        //  we apply fact if frame is in direction of playback,
        //  ffact incorporates that policy
        //

        float ffact = fact;
        if ((m_displayInc < 0 && forward) || (m_displayInc > 0 && !forward))
        {
            ffact = 1.0 - fact;
        }

        float d;
        if (frame < firstFrame)
        {
            d = (m_cacheOutsideRegion) ? 1.0 / (1.0 + firstFrame - frame) : 0.0;
        }
        else if (frame > lastFrame)
        {
            d = (m_cacheOutsideRegion) ? 1.0 / (1.0 + frame - lastFrame) : 0.0;
        }
        else
        {
            //
            //  If we are in the in/out range, but the display frame is not, we
            //  want to cache "relative" to the _next_ display frame if we
            //  start playback, which will be the in point.
            //

            int testFrame = m_displayFrame;
            if (m_displayFrame < firstFrame || m_displayFrame > lastFrame)
                testFrame = firstFrame;

            d = ffact * fabs(float(frame - testFrame));

            //
            //  Since frame is within the in/out range, test the "modulo"
            //  distances that wrap round the front or back of the
            //  range.  Add one, or a distance of zero is possible.
            //
            float dRoundFront = 1 + testFrame - firstFrame + lastFrame - frame;
            float dRoundBack = 1 + lastFrame - testFrame + frame - firstFrame;

            //
            //  apply fact if this alternate path is "forward"
            //
            if (m_displayInc < 0)
                dRoundFront *= fact, dRoundBack *= (1.0 - fact);
            else
                dRoundFront *= (1.0 - fact), dRoundBack *= fact;

            DBL(DB_UTIL, "d " << 1.0 / d << " drf " << 1.0 / dRoundFront
                              << " drb " << 1.0 / dRoundBack << " ffact "
                              << ffact << " inc " << m_displayInc << " fwd "
                              << forward);

            if (dRoundFront < d)
                d = dRoundFront;
            if (dRoundBack < d)
                d = dRoundBack;
            d = 1.0 + 1.0 / d;
        }

        DBL(DB_UTIL, "utility(" << frame << ") = " << d << ", dsp "
                                << m_displayFrame << " cacheOutside "
                                << m_cacheOutsideRegion);
        return d;
    }

    FBCache::CacheFrame FBCache::findBestCacheTarget()
    {
        int targetCacheFrame = NAF;
        float targetCacheFrameUtility = utilityMin;

        CacheEdges::EdgeVector edges;
        m_cacheEdges->possibleCacheTargets(edges);
        CacheFrame result;

        //
        //  First find caching target frame.
        //
        for (CacheEdges::EdgeVector::iterator i = edges.begin();
             i != edges.end(); ++i)
        {
            int f = i->frame, f2 = NAF;
            CacheEdges::EdgeType type = i->type;

            DBL(DB_EDGES,
                " f " << f << " type " << m_cacheEdges->typeToText(type));

            //
            //  Skip (in the right direction) past any frame we're
            //  already attempting to cache, or that is scheduled
            //  for freeing.
            //
            if (CacheEdges::LeftOnly == type)
            {
                while (isFrameCached(f) || m_framesBeingCached.count(f)
                       || m_framesScheduledForFreeing.count(f)
                       || f == m_displayFrame)
                    --f;
            }
            else if (CacheEdges::RightOnly == type)
            {
                while (isFrameCached(f) || m_framesBeingCached.count(f)
                       || m_framesScheduledForFreeing.count(f)
                       || f == m_displayFrame)
                    ++f;
            }
            else if (CacheEdges::BothSides == type)
            {
                f2 = f;
                while (isFrameCached(f) || m_framesBeingCached.count(f)
                       || m_framesScheduledForFreeing.count(f)
                       || f == m_displayFrame)
                    --f;
                while (isFrameCached(f2) || m_framesBeingCached.count(f2)
                       || m_framesScheduledForFreeing.count(f2)
                       || f2 == m_displayFrame)
                    ++f2;
            }
            else
                continue; /* unexpected type !*/

            DBL(DB_EDGES, " f " << f << " f2 " << f2);
            if (f >= m_minFrame && f < m_maxFrame && !isFrameCached(f))
            {
                float u = utility(f, FOR_CACHING);
                DBL(DB_EDGES, "consider frame for caching: frame "
                                  << f << " utility " << u << " target "
                                  << targetCacheFrame << " utility "
                                  << targetCacheFrameUtility);
                if (u > targetCacheFrameUtility)
                {
                    targetCacheFrame = f;
                    targetCacheFrameUtility = u;
                    result.inc = (type == CacheEdges::RightOnly) ? 1 : -1;
                }
            }
            if (NAF != f2 && f2 >= m_minFrame && f2 < m_maxFrame
                && !isFrameCached(f2))
            {
                float u = utility(f2, FOR_CACHING);
                DBL(DB_EDGES, "consider frame for caching: frame "
                                  << f << " utility " << u << " target "
                                  << targetCacheFrame << " utility "
                                  << targetCacheFrameUtility);
                if (u > targetCacheFrameUtility)
                {
                    targetCacheFrame = f2;
                    targetCacheFrameUtility = u;
                    result.inc = 1;
                }
            }
        }

        result.frame = targetCacheFrame;
        result.utility = targetCacheFrameUtility;

        return result;
    }

    FBCache::CacheFrame
    FBCache::findBestFreeTarget(const CacheFrame& cacheTarget)
    {
        //
        //  Then find freeing target frame.
        //
        int targetFreeFrame = NAF;
        float targetFreeFrameUtility = utilityMax;
        float targetCacheFrameUtility = cacheTarget.utility;

        CacheFrame result;

        CacheEdges::EdgeVector edges;
        m_cacheEdges->possibleFreeTargets(edges);

        for (CacheEdges::EdgeVector::iterator i = edges.begin();
             i != edges.end(); ++i)
        {
            int f = i->frame;
            CacheEdges::EdgeType type = i->type;

            //
            //  Skip (in the right direction) past any frame we've
            //  already scheduled for freeing.  Note we go in the
            //  opposite direction because the direction describes
            //  where free frames lie and we're looking for cached
            //  frames.
            //
            if (CacheEdges::LeftOnly == type)
            {
                while (m_framesScheduledForFreeing.count(f))
                    ++f;
            }
            else if (CacheEdges::RightOnly == type)
            {
                while (m_framesScheduledForFreeing.count(f))
                    --f;
            }
            else if (CacheEdges::BothSides == type)
            {
                //  There are free frames on both sides of this one,
                //  so we only need to test this one.

                if (m_framesScheduledForFreeing.count(f))
                    continue;
            }
            else
                continue; /* unexpected type !*/

            considerFrameForFreeing(f, targetFreeFrame, targetFreeFrameUtility);
        }

        result.frame = targetFreeFrame;
        result.utility = targetFreeFrameUtility;

        return result;
    }

    void FBCache::initCacheFreePair(int cacheFrame, int freeFrame)
    {
        if (NAF != cacheFrame)
        {
            m_framesBeingCached[cacheFrame] = freeFrame;
            if (NAF != freeFrame)
            {
                m_framesScheduledForFreeing.insert(freeFrame);
            }

            DB(endl
               << "************** initiateCachingOfFrame cacheTarget "
               << cacheFrame << " freeTarget " << freeFrame << " over "
               << overflowing());
            DB("    freeingList size " << m_framesScheduledForFreeing.size()
                                       << " cachingList size "
                                       << m_framesBeingCached.size());
            //  DB ("    target cached " << isFrameCached(targetCacheFrame));
        }
    }

    void FBCache::initiateCachingOfBestFrameGroup(FrameVector& frames,
                                                  int maxGroupSize)
    {
        frames.clear();

        CacheFrame cacheTarget = findBestCacheTarget();

        if (0.0 == cacheTarget.utility || cacheTarget.frame == NAF)
            return;

        CacheFrame freeTarget = findBestFreeTarget(cacheTarget);

        if (overflowing())
        {
            if (isActiveTailCachingEnabled())
            {
                if (freeTarget.utility >= cacheTarget.utility)
                    return;
            }
            else
            {
                if (freeTarget.utility
                    >= (utility(cacheTarget.frame, FOR_FREEING) - 0.001))
                    return;
            }
        }

        //
        //  We have at least one frame to cache, see if we have more ...
        //

        int cacheFrameCount = 1;
        initCacheFreePair(cacheTarget.frame, freeTarget.frame);

        for (int testFrame = cacheTarget.frame + cacheTarget.inc;
             cacheFrameCount < maxGroupSize;
             testFrame += cacheTarget.inc, ++cacheFrameCount)
        {
            if (m_framesBeingCached.count(testFrame)
                || m_framesScheduledForFreeing.count(testFrame)
                || testFrame == m_displayFrame || isFrameCached(testFrame))
            {
                break;
            }

            //
            //  testFrame is cachable, now see if we can find corresponding free
            //  target.
            //

            CacheFrame testCF(testFrame, utility(testFrame, FOR_CACHING));

            if (testCF.utility == 0.0)
                break;

            CacheFrame freeCF = findBestFreeTarget(testCF);

            if (overflowing())
            {
                if (isActiveTailCachingEnabled())
                {
                    if (freeCF.utility >= testCF.utility)
                        break;
                }
                else
                {
                    if (freeCF.utility
                        >= (utility(testFrame, FOR_FREEING) - 0.001))
                        break;
                }
            }

            initCacheFreePair(testCF.frame, freeCF.frame);
        }
        //  if (cacheFrameCount > 1) cerr << "cacheFrameCount " <<
        //  cacheFrameCount << endl;

        //
        //  Now fill return vector of frame numbers to cache, always in
        //  decreasing order.
        //

        for (int f = (cacheTarget.inc > 0)
                         ? cacheTarget.frame + cacheFrameCount - 1
                         : cacheTarget.frame,
                 lim = f - cacheFrameCount;
             f > lim; --f)
        {
            frames.push_back(f);
        }

        return;
    }

    void FBCache::completeCachingOfFrame(int frame)
    {
        DB("----- completeCachingOfFrame " << frame);
        std::map<int, int>::iterator i = m_framesBeingCached.find(frame);
        if (i != m_framesBeingCached.end())
        {
            DB("-----     found in beingCached map");
            if (NAF != i->second)
            {
                DB("removing " << i->second
                               << " from scheduledForFreeing list");
                m_framesScheduledForFreeing.erase(i->second);
            }
            m_framesBeingCached.erase(i);
        }
    }

    void FBCache::considerFrameForFreeing(int f, int& targetFrame,
                                          float& targetUtility)
    {
        DBL(DB_EDGES, "considerFrameForFreeing: frame "
                          << f << " target " << targetFrame << " utility "
                          << targetUtility);
        if (f < m_minFrame || f >= m_maxFrame || m_framesBeingCached.count(f)
            || f == m_displayFrame)
        {
            return;
        }

        if (isFrameCached(f))
        //
        //  Consider this frame for freeing
        //
        {
            float u = utility(f, FOR_FREEING);

            if (u < targetUtility)
            {
                targetFrame = f;
                targetUtility = u;
                DBL(DB_EDGES, "considerFrameForFreeing: targeting new frame "
                                  << f << " utility " << u);
            }
        }
    }

    //
    //  free() really tries to ensure that the amount of space requested
    //  is available in the cache.  so it may free nothing if that space
    //  is already available, or it may free more than requested if the
    //  cache is over-full.
    //
    bool FBCache::free(size_t inbytes) { return freeInternal(inbytes, true); }

    bool FBCache::freeInternal(size_t inbytes, bool freeMemory)
    {
        DBL(DB_FREE, "free() " << inbytes << " current " << m_currentBytes
                               << " max " << m_maxBytes << " "
                               << double(m_currentBytes) / double(m_maxBytes));
        if (m_maxBytes > m_currentBytes
            && m_maxBytes - m_currentBytes >= inbytes)
        {
            return true;
        }

        size_t targetBytes = (m_maxBytes > inbytes) ? m_maxBytes - inbytes : 0;
        if (freeMemory)
        {
            TwkFB::Cache::freeTrash(m_currentBytes - targetBytes);
        }

        size_t incoming = m_currentBytes;
        FrameVector toBeErased;

        std::set<int> alreadyTried;
        bool enough = true;

        CacheEdges::EdgeVector edges;

        while (m_maxBytes < m_currentBytes
               || m_maxBytes - m_currentBytes < inbytes)
        {
            m_cacheEdges->possibleFreeTargets(edges);
            int targetFreeFrame = NAF;
            float targetFreeFrameUtility = m_targetCacheFrameUtility;

            for (CacheEdges::EdgeVector::iterator i = edges.begin();
                 i != edges.end(); ++i)
            {
                int f = i->frame;
                CacheEdges::EdgeType type = i->type;

                //
                //  Skip (in the right direction) past any frame we've
                //  already attempted to free.  Note we go in the
                //  opposite direction because the directions describes
                //  where free frames lie and we're looking for cached
                //  frames.
                //
                if (CacheEdges::LeftOnly == type)
                {
                    while (alreadyTried.count(f)
                           || m_framesBeingCached.count(f))
                    {
                        ++f;
                    }
                }
                else if (CacheEdges::RightOnly == type)
                {
                    while (alreadyTried.count(f)
                           || m_framesBeingCached.count(f))
                    {
                        --f;
                    }
                }
                else if (CacheEdges::BothSides == type)
                {
                    //  There are free frames on both sides of this one,
                    //  so we only need to test this one.

                    if (alreadyTried.count(f))
                        continue;
                }
                else
                    continue; /* unexpected type !*/

                considerFrameForFreeing(f, targetFreeFrame,
                                        targetFreeFrameUtility);
            }
            DBL(DB_FREE, "free targetFreeFrame "
                             << targetFreeFrame << " utility "
                             << targetFreeFrameUtility << " < "
                             << m_targetCacheFrameUtility);

            if (NAF == targetFreeFrame)
            //
            //  There's no frame we can legitimately free (that has a
            //  utility less than the m_targetCacheFrameUtility).
            //
            {
                enough = false;
                break;
            }
            alreadyTried.insert(targetFreeFrame);
            //
            //  Free the ids that were cached for this frame, this may
            //  free nothing of other frames are using these ids.
            //
            FrameMap::iterator fi = m_frames.find(targetFreeFrame);
            const IDSet& idset = fi->second;

            if (freeIDSet(idset, targetFreeFrame))
            {
                toBeErased.push_back(targetFreeFrame);
            }

            DBL(DB_FREE, "free m_currentBytes " << m_currentBytes
                                                << " targetBytes "
                                                << targetBytes);
            //
            //  At best freeIDSet just flushes ids from the frame-level
            //  data structures, adding them to the Cache-level
            //  TrashCan, so call freeTrash to "really" free something.
            //
            if (freeMemory && m_currentBytes > targetBytes)
            {
                TwkFB::Cache::freeTrash(m_currentBytes - targetBytes);
            }
        }

        for (int i = 0; i < toBeErased.size(); i++)
        {
            m_frames.erase(toBeErased[i]);
        }
        checkMetadata();

        if (Cache::debug())
            showCacheContents();

        setCacheStatsDirty();

        for (std::set<int>::iterator i = alreadyTried.begin();
             i != alreadyTried.end(); ++i)
        {
            DB("    frame " << *i << " still cached: " << isFrameCached(*i));
            m_cacheEdges->removeCacheEdge(*i);
        }

        bool ok = (m_maxBytes - m_currentBytes >= inbytes);

        DBL(DB_FREE, "free() complete"
                         << " current " << m_currentBytes << " max "
                         << m_maxBytes << ", "
                         << double(m_currentBytes) / double(m_maxBytes)
                         << ", ok " << ok);

        return ok;
    }

    void FBCache::emergencyFree()
    {
        DBL(DB_FREE, "******************* emergencyFree "
                         << " current " << m_currentBytes << " max "
                         << m_maxBytes << " "
                         << double(m_currentBytes) / double(m_maxBytes)
                         << " trash count " << trashCount());

        if (m_maxBytes < m_currentBytes)
        {
            //  Set the utility bar as high as possible (any edge frame with
            //  utility less than utilityMax will be considered for
            //  freeing).
            //
            m_targetCacheFrameUtility = utilityMax;
            free(0);
            DBL(DB_FREE, "******************* emergencyFree "
                             << " current " << m_currentBytes << " max "
                             << m_maxBytes << " "
                             << double(m_currentBytes) / double(m_maxBytes)
                             << " trash count " << trashCount());
        }
        setCacheStatsDirty();
    }

    bool FBCache::cacheStats(CacheStats& c) const
    {
        // if (m_cacheStatsDisabled) cerr << "ERROR: cache stats requested, but
        // have been disabled" << endl;

        LockObject sl(m_statMutex, true);

        if (sl.locked())
        {
            c = m_cacheStats;
            return true;
        }
        else
        {
            return false;
        }
    }

    void FBCache::setCacheStatsDirty()
    {
        if (m_cacheStatsDisabled)
            return;

        LockObject sl(m_statMutex, true);

        if (sl.locked())
            m_cacheStatsDirty = true;

        //
        //  If we failed to lock, something else is setting the dirty flag _or_
        //  the stats are being updated.  There's a chance here that the flag
        //  could get out of sync (IE be set to true even though we just called
        //  setCacheStatsDirty, we just don't care enough that the stats be
        //  accurate.  We'd rather eliminate the possibility of a perf hit due
        //  to lock contention.
        //
    }

    void FBCache::updateCacheStatsIfDirty()
    {
        if (m_cacheStatsDisabled)
            return;

        LockObject sl(m_statMutex);

        if (!m_cacheStatsDirty)
            return;

        //
        //  This returns immediately
        //

        m_cacheStats.capacity = capacity();
        m_cacheStats.used = used();

        //
        //  This requires some work
        //

        lock();
        try
        {
            computeCachedRangesStat(m_cacheStats.cachedRanges);
            m_cacheStats.lookAheadSeconds =
                computeLookAheadSecondsStat(m_cacheStats.cachedRanges);

            m_cacheStatsDirty = false;
            unlock();
        }
        catch (...)
        {
            unlock();
        }
    }

    //------------------------------------------------------------------------------
    //
    void FBCache::computeCachedRangesStat(FrameRangeVector& array) const
    {
        array.clear();

        for (FrameMap::const_iterator i = m_frames.begin(); i != m_frames.end();
             ++i)
        {
            int frame = i->first;

            if (array.empty())
            {
                array.push_back(make_pair(frame, frame));
            }
            else
            {
                FrameRange& range = array.back();

                if (range.second == frame - 1)
                {
                    range.second = frame;
                }
                else
                {
                    array.push_back(make_pair(frame, frame));
                }
            }
        }
    }

    //------------------------------------------------------------------------------
    //
    float FBCache::computeLookAheadSecondsStat(
        const FrameRangeVector& vecFrameRange) const
    {
        if (m_displayFrame == NAF || m_inFrame == NAF || m_outFrame == NAF
            || m_displayInc == 0 || vecFrameRange.empty())
        {
            return 0.0f;
        }

        // Determine the look ahead time in seconds from the display frame
        if (m_displayInc > 0)
        {
            // Positive increment
            for (FrameRangeVector::const_iterator i = vecFrameRange.begin();
                 i != vecFrameRange.end(); ++i)
            {
                // Locate the range where the display frame is located if any
                if (m_displayFrame >= i->first && m_displayFrame <= i->second)
                {
                    // Locate the last matching range wrt m_displayInc
                    FrameRangeVector::const_iterator endFrameRangeIter = i;
                    while (i != vecFrameRange.end()
                           && (i->first
                               <= (endFrameRangeIter->second + m_displayInc)))
                    {
                        endFrameRangeIter = i;
                        i++;
                    };

                    return (endFrameRangeIter->second - m_displayFrame)
                           / m_displayFPS;
                }
            }
        }
        else
        {
            // Negative increment
            for (FrameRangeVector::const_reverse_iterator i =
                     vecFrameRange.rbegin();
                 i != vecFrameRange.rend(); ++i)
            {
                // Locate the range where the display frame is located if any
                if (m_displayFrame >= i->first && m_displayFrame <= i->second)
                {
                    // Locate the last matching range wrt m_displayInc
                    FrameRangeVector::const_reverse_iterator endFrameRangeIter =
                        i;
                    while (i != vecFrameRange.rend()
                           && (i->second
                               <= (endFrameRangeIter->first + m_displayInc)))
                    {
                        endFrameRangeIter = i;
                        i++;
                    };

                    return (m_displayFrame - endFrameRangeIter->first)
                           / m_displayFPS;
                }
            }
        }

        return 0.0f;
    }

    struct CheckInEach
    {
        CheckInEach(FBCache& c)
            : cache(c)
        {
        }

        FBCache& cache;

        void operator()(IPImage* l)
        {
            DB("checkInAndDelete() fb " << l->fb << " in cache "
                                        << ((l->fb) ? l->fb->inCache() : 0));
            if (l->fb && l->fb->inCache())
            {
                DB("checkInAndDelete() fb "
                   << l->fb << " ref count " << cache.fbReferenceCount(l->fb)
                   << " frames in itemMap " << cache.framesInItemMap(l->fb));
                cache.Cache::checkIn(l->fb);
                l->fb = 0;
            }
        }
    };

    void FBCache::checkInAndDelete(IPImage* img)
    {
        if (img)
        {
            CheckInEach checkInEach(*this);
            foreach_ip(img, checkInEach);
            delete img;
        }
    }

    void FBCache::checkInAndDelete(const IPImageVector& images)
    {
        for (size_t i = 0; i < images.size(); i++)
            checkInAndDelete(images[i]);
    }

    /*

    I think this is wrong.  can't just clear thes caches without
    dereferencing fbs, etc.

    void
    FBCache::clearFrameInfo()
    {
        lock();
        m_framesScheduledForFreeing.clear();
        m_framesBeingCached.clear();
        m_frames.clear();
        m_cacheEdges->clear();
        unlock();
    }
    */

    bool FBCache::frameItems(int frame, FBVector& items) const
    {
        if (m_frames.empty())
            return false;

        FrameMap::const_iterator i = m_frames.find(frame);
        bool missed = false;
        bool found = false;

        if (i != m_frames.end())
        {
            const IDSet& idset = i->second;

            for (IDSet::const_iterator id = idset.begin(); id != idset.end();
                 ++id)
            {
                //
                //  Is the id still in the cache? If so add it to the
                //  items vector. Otherwise make a note of the fact that
                //  there was a cache miss and continue.
                //

                FBMap::const_iterator mi = m_map.find(*id);

                if (mi == m_map.end())
                {
                    missed = true;
                }
                else
                {
                    found = true;
                    if (mi->second)
                        items.push_back(mi->second);
                }
            }
        }

        return !missed && found;
    }

    //
    //  Update frame-level caching.  Note that we only reference a FB once, no
    //  matter how many times it's used by this frame.
    //

    //
    //  Collect cached FBs from IPImage
    //
    struct IDSetCollector
    {
        IDSetCollector(FBCache::IDSet& s)
            : ids(s)
        {
        }

        FBCache::IDSet& ids;

        void operator()(IPImage* img)
        {
            if (img->fb && img->fb->inCache())
            {
                ids.insert(img->fb->identifier());
            }
        }
    };

    void FBCache::trimFBsOfFrame(int frame, IPImage* img)
    {
        DB("trimFBsOfFrame " << frame);
        //
        //  Build set of IDs of FBs used by this frame (according to incoming
        //  image).
        //

        IDSet imgIDs;
        IDSetCollector collector(imgIDs);
        foreach_ip(img, collector);

        //
        //  For each FB the cache thinks this frame uses, if it's not in the
        //  incoming list, remove it from frame-level caching for this frame.
        //

        IDSet& frameIDs = m_frames[frame];
        FBVector derefFBs;

        for (IDSet::iterator i = frameIDs.begin(); i != frameIDs.end(); ++i)
        {
            if (imgIDs.count(*i) == 0)
            {
                derefFBs.push_back(m_map[*i]);
            }
        }
        for (FBVector::iterator i = derefFBs.begin(); i != derefFBs.end(); ++i)
        {
            dereferenceFrame(frame, *i);
        }
        DB("trimFBsOfFrame " << frame << " complete");
    }

    void FBCache::dereferenceFrame(int frame, FrameBuffer* fb)
    {
        //
        //  We assume fb is cached.
        //

        bool reffed = (m_frames[frame].count(fb->identifier()) != 0);

        DB("dereferernceFrame frame "
           << frame << " fb " << fb << " id " << fb->identifier()
           << " reffed: " << reffed << " refcount " << fbReferenceCount(fb));

        if (reffed)
        {
            //  Remove frame from list of frames _using_ this FB.
            ///
            m_itemMap[fb].erase(frame);

            //  Remove FB's identifier from list of those _used by_ this Frame.
            //
            m_frames[frame].erase(fb->identifier());

            //  DeReference FB so it can be discarded from cache if this was
            //  last external ref.
            //
            dereferenceFB(fb);
            DB("frame " << frame << " size " << m_frames[frame].size()
                        << " de-reffed '" << fb->identifier() << "' new count "
                        << fbReferenceCount(fb));
        }
    }

    bool FBCache::referenceFrame(int frame, FrameBuffer* fb)
    {
        bool reffed = (m_frames[frame].count(fb->identifier()) != 0);

        DB("referenceFrame frame "
           << frame << " fb " << fb << " id " << fb->identifier()
           << " already reffed: " << reffed << " refcount "
           << fbReferenceCount(fb));

        if (!reffed)
        {
            //  Add frame to list of frames that use this FB.
            ///
            m_itemMap[fb].insert(frame);

            //  Add FB's identifier to list of those used by this Frame.
            //
            m_frames[frame].insert(fb->identifier());

            //  Reference FB so will not be discarded from cache, until we
            //  decide to discard this frame.
            //
            referenceFB(fb);

            return true;
        }
        return false;
    }

    bool FBCache::flush(const IDString& idstring)
    {
        DBL(DB_FLUSH, "flush() " << idstring);
        FBMap::iterator i = m_map.find(idstring);
        FrameVector toBeErased;

        if (i != m_map.end())
        {
            FrameBuffer* fb = i->second;
            ItemFrames::iterator q = m_itemMap.find(fb);

            if (q != m_itemMap.end())
            {
                FrameSet fset = q->second;
                //
                //  erase q now, since the deref below may lead us back
                //  to FBCache::flush, and if so we want to be sure that
                //  the above find() fails at that time.
                //
                m_itemMap.erase(q);
                DBL(DB_FLUSH, "flush() fset size " << fset.size());

                for (FrameSet::iterator fs = fset.begin(); fs != fset.end();
                     ++fs)
                //
                //  For every frame that references this fb
                //
                {
                    //
                    //  Get the set of identifiers associated with this
                    //  frame, and remove this one, if it's in there
                    //
                    DBL(DB_FLUSH, "flush() checking idset of frame " << *fs);
                    IDSet& idset = m_frames[*fs];
                    if (idset.count(idstring))
                    {
                        idset.erase(idstring);
                        if (idset.empty())
                            toBeErased.push_back(*fs);

                        DBL(DB_FLUSH | DB_REF, "flush() dereferencing");
                        dereferenceFB(fb);
                    }
                }
            }
            for (FrameVector::iterator fvi = toBeErased.begin();
                 fvi != toBeErased.end(); ++fvi)
            {
                m_frames.erase(*fvi);
                m_cacheEdges->removeCacheEdge(*fvi);
            }

            /*
            //
            //  Don't call Cache::flush here, since we want to hold on
            //  to frame bufferes as long as possible (until we need the
            //  memory).  The dereference above will (probably) push the
            //  fb into the TrashCan, from which it can be reclaimed if
            //  another frame wants to use it.
            //
            bool ret = Cache::flush(idstring);

            #ifdef DB_LEVEL
                std::set<std::string> ids;
                for (FrameMap::iterator fmi = m_frames.begin(); fmi !=
            m_frames.end(); ++fmi)
                {
                    IDSet& idset = fmi->second;
                    for (IDSet::iterator i = idset.begin(); i != idset.end();
            ++i)
                    {
                        ids.insert(*i);
                    }
                }
                std::set<std::string> cids;
                DBL (DB_FLUSH, "after flushing (ret " << ret << ") " << idstring
            <<
                        ", FBCache holds " << ids.size() << " ids, Cache holds "
            << m_map.size()); #endif

            return ret;
            */
            return true;
        }

        return false;
    }

    bool FBCache::flushIDSetSubstr(const IDSet& subStrings)
    {
        IDSet idsToBeFlushed;

        for (FBMap::iterator i = m_map.begin(); i != m_map.end(); ++i)
        {
            for (IDSet::const_iterator ss = subStrings.begin();
                 ss != subStrings.end(); ++ss)
            {
                if (i->first.find(*ss) != string::npos)
                    idsToBeFlushed.insert(i->first);
            }
        }

        bool ret = false;
        for (IDSet::iterator id = idsToBeFlushed.begin();
             id != idsToBeFlushed.end(); ++id)
        {
            bool success = flush(*id);
            ret = ret || success;
        }

        return ret;
    }

    FBCache::TreeResults FBCache::testInCache(const IDTree& tree)
    {
        int n = 0, c = 0;

        for (size_t i = 0; i < tree.size(); i++)
        {
            const IDStringVector& ids = tree[i];

            for (size_t q = 0; q < ids.size(); q++, n++)
            {
                if (isCached(ids[q]))
                    c++;
            }
        }

        TreeResults result = HasAllIDs;
        if (c == 0)
            result = HasNoIDs;
        else if (c != n)
            result = HasSomeIDs;

        return result;
    }

    void FBCache::garbageCollect(bool force)
    {
        //
        //  I think garbage collection was compensating for problems
        //  with caching data structure management that caused us to
        //  loose track of fbs.  That's tighter now, so we shouldn't
        //  need GC at all.  (but leaving this code in in case I'm
        //  wrong ;-) )
        //
        return;

        //
        //  Loop over the items and check to see if the frames are
        //  pointing back at them
        //
        DB("garbageCollect() force " << force);

        if (Cache::debug())
        {
            cout << "INFO: GC: frames currently active: " << dec;

            for (FrameMap::iterator i = m_frames.begin(); i != m_frames.end();
                 ++i)
            {
                cout << " " << i->first;
            }

            cout << endl;
        }

        //
        //  Find any orphaned images in the cache.
        //

        vector<string> orphans;

        for (FBMap::const_iterator i = m_map.begin(); i != m_map.end(); ++i)
        {
            if (fbReferenceCount(i->second) == 0)
            {
                assert(i->second->isCacheLocked() == false);
                orphans.push_back(i->first);
            }
        }

        DB("    found " << orphans.size() << " orphans, flushing");
        for (size_t i = 0; i < orphans.size(); i++)
        {
            flush(orphans[i]);
        }

        if (Cache::debug())
        {
            if (!orphans.empty())
            {
                cout << "INFO: FB GC flushed " << orphans.size()
                     << " orphaned frames" << endl;
            }
        }

        vector<FrameBuffer*> garbageItems;

        for (ItemFrames::iterator i = m_itemMap.begin(); i != m_itemMap.end();
             ++i)
        {
            FrameBuffer* fb = i->first;
            FrameSet& fset = i->second;

            vector<int> garbageFrames;

            for (FrameSet::iterator q = fset.begin(); q != fset.end(); ++q)
            {
                int f = *q;

                FrameMap::iterator fi = m_frames.find(f);

                if (fi != m_frames.end())
                {
                    IDSet& ids = fi->second;

                    if (ids.count(fb->identifier()) == 0)
                    //
                    //  If the identifier does not appear in this
                    //  frame's entry, the m_itemMap entry is
                    //  inconsistant, so mark this frame for removal
                    //  from the m_itemMap entry.
                    {
                        garbageFrames.push_back(f);
                    }
                }
                else
                {
                    //
                    //  Likewise if there is no entry for this frame at
                    //  all.
                    //
                    garbageFrames.push_back(f);
                }
            }

            //
            //  Now remove frames from fb's item map entry that were
            //  discovered above.
            //
            for (size_t q = 0; q < garbageFrames.size(); q++)
            {
                if (fset.count(garbageFrames[q]))
                {
                    fset.erase(garbageFrames[q]);
                    //
                    //  Don't deref here because we don't add a ref when
                    //  we add frames to fb's entry in m_itemMap
                    //
                    //  dereferenceFB(fb);
                }
            }

            //
            //  If this fb is no longer linked to any frame, mark it
            //  for removal from m_itemMap.
            //
            if (fset.empty())
                garbageItems.push_back(fb);
        }
        DB("    found " << garbageItems.size()
                        << " 'garbage' items in m_itemMap");

        if (Cache::debug() && garbageItems.size())
            cout << "INFO: GC: " << garbageItems.size() << " items found"
                 << endl;

        size_t collected = 0;

        for (size_t i = 0; i < garbageItems.size(); i++)
        {
            FrameBuffer* fb = garbageItems[i];

            if (!fb)
            {
                // cout << "INFO: GC: no item at " << i << endl;
                continue;
            }

            if (fb->isCacheLocked())
            {
                // cout << "INFO: GC: lock on " << fb->identifier() << endl;
                continue;
            }

            ItemFrames::iterator item = m_itemMap.find(fb);

            if (item != m_itemMap.end())
            {
                while (force && fbReferenceCount(fb) > 1)
                {
                    DBL(DB_REF,
                        "garbageCollect() dereferencing in GC 2, fb " << fb);
                    DBL(DB_REF, "                 id " << fb->identifier()
                                                       << " ref count "
                                                       << fbReferenceCount(fb));
                    dereferenceFB(fb);
                }

                DB("GC: flushing " << fb << " " << fb->identifier() << " with "
                                   << item->second.size() << " frames ");
                if (!flush(fb->identifier()))
                {
#if 0
                cerr << "WARNING: GC: unflushed item " 
                     << fb->identifier()
                     << endl;
#endif
                }
                else
                {
                    m_itemMap.erase(item);
                    collected++;
                }
            }
            else
            {
                cerr << "WARNING: CACHE GC: found disconnected item "
                     << fb->identifier() << endl;
            }
        }

        if (Cache::debug() && collected)
            cout << "INFO: CACHE GC: collected " << collected << " items"
                 << endl;

#if 0
    if (!force && collected != garbageItems.size())
    {
        cout << "INFO: STOPPING: " << garbageItems.size() << " items were found, "
             << collected << " of them were collected"
             << endl;

        exit(-1);
    }
#endif
    }

    void FBCache::showCacheContents() const
    {
        cout << "CACHE CONTENTS:" << endl;
        cout << "---------------" << endl;

        for (FBMap::const_iterator i = m_map.begin(); i != m_map.end(); ++i)
        {
            cout << i->second->identifier()
                 << (i->second->isCacheLocked() ? " -locked- " : "") << " -"
                 << fbReferenceCount(i->second) << "- " << endl;
        }

        cout << "---------------" << endl;
    }

    void FBCache::setDisplayFrame(int f)
    {
        if (f != m_displayFrame
            && m_graph->cachingMode() == IPGraph::BufferCache)
            m_utilityStateChanged = true;
        m_displayFrame = f;
    }

    void FBCache::setDisplayInc(int i)
    {
        if (i != m_displayInc && m_graph->cachingMode() == IPGraph::BufferCache)
            m_utilityStateChanged = true;
        m_displayInc = i;
    }

    void FBCache::setInOutFrames(int a, int b, int c, int d)
    {
        DBL(DB_EDGES,
            "setInOutFrames " << a << " " << b << " " << c << " " << d << endl);
        if (a != m_inFrame || b != m_outFrame || c != m_minFrame
            || d != m_maxFrame)
        {
            m_utilityStateChanged = true;
        }
        m_inFrame = a;
        m_outFrame = b;
        m_minFrame = c;
        m_maxFrame = d;
    }

    void FBCache::setLookBehindFraction(float f)
    {
        if (f != m_lookBehindFraction
            && m_graph->cachingMode() == IPGraph::BufferCache)
            m_utilityStateChanged = true;
        m_lookBehindFraction = f;
    }

    void FBCache::enableActiveTailCaching(const bool enable)
    {
        if (enable != m_activeTailCachingEnabled)
        {
            m_activeTailCachingEnabled = enable;

            if (m_graph->cachingMode() == IPGraph::BufferCache)
            {
                m_utilityStateChanged = true;
            }
        }
    }

    void FBCache::pushCachableOutputItem(string name)
    {
        m_perNodeCache->pushItem(name);

        m_utilityStateChanged = true;
    }

    string FBCache::popCachableOutputItem()
    {
        return m_perNodeCache->popItem();
    }

    void FBCache::checkMetadata()
    {
#if (DB_LEVEL)
        for (FBMap::iterator i = m_map.begin(); i != m_map.end(); ++i)
        {
            FrameBuffer* fb = i->second;
            int refs = fbReferenceCount(fb);
            if (framesInItemMap(fb) == 0 && refs > 1)
            {
                cerr << "FB LOST: " << i->first << "(" << fb->identifier()
                     << "), refs " << refs << ", in trash " << trashContains(fb)
                     << endl;
            }
        }
#endif
    }

} // namespace IPCore
