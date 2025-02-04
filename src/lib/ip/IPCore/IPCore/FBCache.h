//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __IPCore__FBCache__h__
#define __IPCore__FBCache__h__
#include <TwkFB/Cache.h>
#include <set>
#include <map>

namespace IPCore
{
    class IPGraph;
    class IPImage;
    class IPImageID;
    class IPNode;
    class CacheEdges;

    //
    //  FBCache is a modification of TwkFB::Cache that handles cross
    //  referencing by frame (sort of). The TwkFB::Cache doesn't have a
    //  good policy for IPGraph. This version has a SparseIntervalSet of
    //  frames per cache item which makes is possible for it to know which
    //  items are outside of in/out bounds and which items are not.
    //

    class PerNodeCache;

    class FBCache : public TwkFB::Cache
    {
    public:
        typedef TwkFB::FrameBuffer FrameBuffer;
        typedef std::vector<FrameBuffer*> FBVector;
        typedef FrameBuffer::DataType DataType;
        typedef std::set<int> FrameSet;
        typedef std::map<FrameBuffer*, FrameSet> ItemFrames;
        typedef std::vector<int> FrameVector;
        typedef std::set<std::string> IDSet;
        typedef std::map<int, IDSet> FrameMap;
        typedef std::map<size_t, FBVector> FreeLists;
        typedef std::vector<std::string> IDStringVector;
        typedef std::vector<IDStringVector> IDTree;
        typedef std::pair<int, int> FrameRange;
        typedef std::vector<FrameRange> FrameRangeVector;

        enum FreeMode
        {
            ConservativeFreeMode, /// never frees in in/out range
            ActiveFreeMode, /// allows freeing in in/out range behind display
                            /// frame
            GreedyFreeMode  /// allows freeing in in/out range in FRONT of
                            /// display frame
        };

        enum TreeResults
        {
            HasAllIDs,  /// All of the ids passed in are in the cache
            HasSomeIDs, /// Some of the ids passed in are in the cache
            HasNoIDs    /// None of the ids are in the cache
        };

        struct CacheStats
        {
            size_t capacity;        /// as returned by capacity() function
            size_t used;            /// as returned by used() function
            float lookAheadSeconds; /// as returned by
                                    /// computeLookAheadSecondsStat()
            FrameRangeVector
                cachedRanges; /// as returned by computeCachedRangesStat()

            CacheStats()
                : capacity(0)
                , used(0)
                , lookAheadSeconds(0.0)
            {
            }
        };

        FBCache(IPGraph*);
        ~FBCache();

        //
        //  Fill in a CacheStats struct. If the stats are being updated
        //  when this function is called, it may return false meaning that
        //  it was unable to fill in the struct. In that case, it will not
        //  touch the contents of the passed in struct.
        //

        bool cacheStats(CacheStats&) const;

        //
        //  These functions will lock and unlock the cache themselves
        //  Cached frames returns a vector of frames, but these are
        //  actually frame pairs.
        //

        //  This may be dangerous now, and no-one is using it.  take it
        //  out for now.
        //  void clearFrameInfo();

        //
        //  These functions require calling lock() and unlock() before
        //  calling them.
        //
        //  frameItems() is a bit complicated. It returns true if all
        //  items for the frame are in the cache. In addition, it fills
        //  the passed in FBVector with items that are actually in the
        //  cache. It does not tell you how many items were missing.
        //

        bool add(FrameBuffer*, int frame, bool force = false,
                 const IPNode* node = 0);
        bool flush(const IDString&);
        bool flushIDSetSubstr(const IDSet& subStrings);

        bool frameItems(int frame, FBVector& items) const;

        bool isFrameCached(int frame) const
        {
            return m_frames.find(frame) != m_frames.end();
        }

        bool hasPartialFrameCache(int frame) const;

        TreeResults testInCache(const IDTree&);

        void setCacheFrame(int f) { m_cacheFrame = f; }

        void setCacheWrapFrame(int f) { m_cacheWrapFrame = f; }

        void setDisplayFPS(float f) { m_displayFPS = f; }

        int cacheFrame() const { return m_cacheFrame; }

        int cacheWrapFrame() const { return m_cacheWrapFrame; }

        int displayFrame() const { return m_displayFrame; }

        int displayInc() const { return m_displayInc; }

        float displayFPS() const { return m_displayFPS; }

        int inFrame() const { return m_inFrame; }

        int outFrame() const { return m_outFrame; }

        int minFrame() const { return m_minFrame; }

        int maxFrame() const { return m_maxFrame; }

        void checkInAndDelete(IPImage*);
        void checkInAndDelete(const std::vector<IPImage*>&);

        void showCacheContents() const;

        void setFreeMode(FreeMode m) { m_freeMode = m; }

        bool allowGreedyFree() const { return m_freeMode == GreedyFreeMode; }

        bool allowActiveFree() const { return m_freeMode == ActiveFreeMode; }

        void clearAllButFrame(int, bool force = false);
        void clearFrameCaches();
        void garbageCollect(bool force = false);

        virtual void setMemoryUsage(size_t bytes);

        //
        //  Call the below to do everything possible to bring the
        //  current cache usage below the max.  Should really only call
        //  in case of emergency (like alloc failure).
        //

        virtual void emergencyFree();

        //
        //  Overflowing() returns true when the current cache usage is
        //  at or above the level it was at when an attempt to cache a
        //  frame last failed.
        //

        bool overflowing() const;

        //
        //  These functions do NOT lock -- you must lock the cache
        //  yourself before calling them
        //

        void initiateCachingOfBestFrameGroup(FrameVector& frames,
                                             int maxGroupSize);
        void completeCachingOfFrame(int frame);

        //
        //  Add a reference to this fb at this frame, and inc ref the
        //  fb.  But do this only if it wasn't already referenced by
        //  this frame.  Return true iff we actually added it.
        //
        bool referenceFrame(int frame, FrameBuffer* fb);

        //
        //  If you know that the frame cache for <frame> should not contain any
        //  other FB references than those in <img> call trimFBsOfFrame() to
        //  ensure that.
        //
        void trimFBsOfFrame(int frame, IPImage* img);

        //
        //  Especially when a node is deleted, ensure that corresponding FBs are
        //  dereferenced.
        //
        void flushPerNodeCache(const IPNode* node);

        FrameBuffer* perNodeCacheContents(const IPNode* node) const;

        void pushCachableOutputItem(std::string name);
        std::string popCachableOutputItem();

        bool cacheStatsDisabled() { return m_cacheStatsDisabled; }

        void setCacheStatsDisabled(bool v) { m_cacheStatsDisabled = v; }

        static void setCacheOutsideRegion(bool b) { m_cacheOutsideRegion = b; }

        static bool cacheOutsideRegion() { return m_cacheOutsideRegion; };

    protected:
        struct CacheFrame
        {
            int frame;
            int inc;
            float utility;

            CacheFrame()
                : frame(0)
                , utility(0.0)
                , inc(0)
            {
            }

            CacheFrame(int f, float u)
                : frame(f)
                , utility(u)
                , inc(0)
            {
            }
        };

        CacheFrame findBestCacheTarget();
        CacheFrame findBestFreeTarget(const CacheFrame& cacheTarget);
        void initCacheFreePair(int cacheFrame, int freeFrame);

        void dereferenceFrame(int frame, FrameBuffer* fb);
        //
        //  These 3 methods can alter the utility values, so if you call
        //  them you may need to wake up sleeping caching threads.
        //  (Since a frame previously "undesirable" for caching may now
        //  be "desirable" and vice versa.)
        //
        //  They should only be called by IPGraph.  After you call them,
        //  you should call utilityStateChanged(), and if it's true,
        //  call resetUtilityState(), then wake up any caching threads.
        //
        void setDisplayFrame(int f);
        void setDisplayInc(int i);
        void setInOutFrames(int a, int b, int c, int d);

        virtual void clearInternal();
        virtual bool free(size_t bytes);
        virtual bool freeInternal(size_t bytes, bool freeMemory = true);
        bool freeIDSet(const IDSet&, int frame);
        bool freeInRangeForwards(size_t, FrameVector&, int, int, bool);
        bool freeInRangeBackwards(size_t, FrameVector&, int, int, bool);
        void setCacheStatsDirty();
        void updateCacheStatsIfDirty();
        void computeCachedRangesStat(FrameRangeVector&) const;
        float computeLookAheadSecondsStat(const FrameRangeVector&) const;

        // utility() returns a weighted value of the relative importance of a
        // frame with respect to the cache. The weighted value is adapted based
        // on whether the weighted value needs to be computed for caching a
        // frame or freeing a frame from the cache.
        enum UtilityMode
        {
            FOR_CACHING,
            FOR_FREEING
        };

        float utility(int frames, UtilityMode mode);

        int cachedFrameOfLesserUtility(int frame);

        bool frameIsBeingCached(int frame) const
        {
            return (m_framesBeingCached.find(frame)
                    != m_framesBeingCached.end());
        }

        bool utilityStateChanged() const { return m_utilityStateChanged; };

        void resetUtilityState() { m_utilityStateChanged = false; };

        void considerFrameForFreeing(int f, int& targetFrame,
                                     float& targetUtility);

        float lookBehindFraction() { return m_lookBehindFraction; };

        void setLookBehindFraction(float f);

        // When active tail caching is enabled, findBestCacheTarget() will
        // consider frames both in the head and tail of the display frame. When
        // active tail caching is disabled, findBestCacheTarget() will only
        // consider frames in the head of the display frame. However already
        // generated tail frames will be preserved with respect to the current
        // look behind fraction.
        bool isActiveTailCachingEnabled() const
        {
            return m_activeTailCachingEnabled;
        }

        void enableActiveTailCaching(const bool enable);

        //
        //  If all the given ids are already cached in the low-level fb
        //  cache, add them to the frame-level caches, so that the
        //  "frame" is now cached.
        //
        bool promoteFrame(int frame, IPImageID* idTree);

        int framesInItemMap(FrameBuffer* fb)
        {
            return ((m_itemMap.find(fb) != m_itemMap.end())
                        ? m_itemMap[fb].size()
                        : 0);
        }

        //
        //  Check metadata of both frame and fb caches and error if any
        //  discrepancies found.
        //
        void checkMetadata();

    private:
        IPGraph* m_graph;
        FrameMap m_frames;
        ItemFrames m_itemMap;
        int m_cacheFrame;
        int m_cacheWrapFrame;
        int m_displayFrame;
        int m_displayInc;
        float m_displayFPS;
        int m_minFrame;
        int m_maxFrame;
        int m_inFrame;
        int m_outFrame;
        FreeMode m_freeMode;
        CacheStats m_cacheStats;
        mutable pthread_mutex_t m_statMutex;
        size_t m_overflowBoundary;
        std::map<int, int> m_framesBeingCached;
        std::set<int> m_framesScheduledForFreeing;
        bool m_utilityStateChanged;
        float m_targetCacheFrameUtility;
        CacheEdges* m_cacheEdges;
        PerNodeCache* m_perNodeCache;
        float m_lookBehindFraction;
        bool m_activeTailCachingEnabled;
        bool m_cacheStatsDisabled;
        bool m_cacheStatsDirty;

        static bool m_cacheOutsideRegion;

        friend class IPGraph;
        friend class CacheEdges;
        friend class CheckInEach;
        friend class PerNodeCache;
    };

} // namespace IPCore

#endif // __IPCore__FBCache__h__
