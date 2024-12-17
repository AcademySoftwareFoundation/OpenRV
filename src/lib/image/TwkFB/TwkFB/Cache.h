//******************************************************************************
// Copyright (c) 2006 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkFB__Cache__h__
#define __TwkFB__Cache__h__
#include <pthread.h>
#include <TwkFB/dll_defs.h>
#include <TwkFB/FrameBuffer.h>
#include <map>
#include <deque>
#include <string>
#include <iostream>

namespace TwkFB
{

    //
    //  Cache
    //
    //  This cache uses the Wavefront Composer caching scheme of
    //  constructing an identifier string from all parameters that created
    //  the image and the original filename. This makes the cache key
    //  independant of any type of application.
    //
    //  The identifier string is retrieved from the identifier() function
    //  on the FrameBuffer before caching. Once an FB is cached, its
    //  pixels and identifier string should not be touched. The cache will
    //  add a property (Cached) to the fb when it caches it.
    //
    //  The cache can also be used for memory management. A garbage
    //  collection policy on the cache (requires sub-classing) can be used
    //  to find unused or old already allocated FrameBuffers. The cache
    //  will give up ownership of those fbs to the caller and remove the
    //  Cached attribute.
    //
    //  This scheme is possibly unique in that it the cache can retrieve
    //  images which are identical eventhough they may have resulted from
    //  very different evaluation paths. For example if there are two
    //  image readers which are using the same input image by
    //  conincidence, they will share the same cache entry!
    //

    class TrashCan;

    class TWKFB_EXPORT Cache
    {
    public:
        //
        //  Types
        //

        typedef std::string IDString;
        typedef TwkFB::FrameBuffer FrameBuffer;
        typedef FrameBuffer::DataType DataType;
        typedef std::map<IDString, FrameBuffer*> FBMap;
        typedef std::deque<std::pair<std::string, int>> LockLog;

        //
        //  Constructors
        //

        Cache();
        virtual ~Cache();

        //
        //  Set the maximum memory allowed to be used by the cache. This
        //  is actually a floor not a ceiling: in some cases you can
        //  continue to stuff fbs into the cache by forcing them in. Each
        //  time, the cache tries to free memory if it can.
        //

        virtual void setMemoryUsage(size_t bytes);

        size_t capacity() const { return m_maxBytes; }

        size_t used() const { return m_currentBytes; }

        //
        //  clear() deletes all images from the cache.
        //

        void clear();

        //
        //  No additional images may be added once the cache is full
        //

        bool isFull() const { return m_full; }

        //
        //  Add a frame buffer to the cache. The identifier string must be
        //  up-to-date for correct caching. If there is not enough room
        //  for the fb, free() will be called to make room for it. If it
        //  cannot make room, it will not add the image and return false.
        //
        //  If force is true, freeing doesn't produce enough memory, and
        //  adding the fb would exceed the limit, it will add the fb
        //  anyway and the total usage will be beyond the limit -- the
        //  function will return false.
        //

        bool add(FrameBuffer*, bool force = false);

        //
        //  Test for cached id
        //

        bool isCached(const IDString&) const;

        //
        //  Get the FrameBuffer with the specified id string. If there is
        //  none, return 0. The lock count on the returned image will be
        //  incremented. For every checkOut() call there must a
        //  corresponding checkIn() call. If you just added an fb and want
        //  to immediately check it out call the second version.
        //
        //  IF the idstring has special meaning, the returned
        //  FrameBufferProxy object will indicate that and the returned fb
        //  may be a substitute for the actual image.
        //

        FrameBuffer* checkOut(const IDString&);
        void checkOut(FrameBuffer*);

        //
        //  Return a previously checked out fb to the cache. If the fb was
        //  not previously added to the cache it will throw.
        //

        void checkIn(FrameBuffer*);

        //
        //  To get rid of a particular item in the cache call flush. If
        //  the item is cache locked it will return false (failed). If the
        //  item is successfully flushed or does not exist in the cache
        //  true is returned.
        //

        virtual bool flush(const IDString&);

        //
        //  Get a possibly planar FrameBuffer that matches the passed in
        //  sizes from the free list. You will need to call restructure()
        //  on the FB planes after this call. A derived cache could have a
        //  policy of not actually free fbs, but instead recycling the
        //  existing ones. This makes it possible to do so.
        //
        //  The default behavior is not to recycle but to return a new
        //  single plane FB with no pixels. If one or more planes cannot
        //  be allocated, zero sized fbs will be returned for those
        //  planes. if allocSize is 0 no planes are specified.
        //

        virtual FrameBuffer* recycledFB(size_t allocSize[4]);

        //
        //  Lock and unlock are necessary in multithreaded code. You
        //  should lock before making Cache calls and unlock when through.
        //

        bool tryLock() const { return pthread_mutex_trylock(&m_mutex) == 0; }

        void lock() const { pthread_mutex_lock(&m_mutex); }

        void unlock() const { pthread_mutex_unlock(&m_mutex); }

        static LockLog& locklog();

        void freePending();

        bool freeAllTrash();

        //
        //  Call the below to do everything possible to bring the
        //  current cache usage below the max.  Should really only call
        //  in case of emergency (like alloc failure).
        //

        virtual void emergencyFree();

    protected:
        //
        //  Locking and unlocking cached fbs (for check out and check
        //  in). Slightly different from ref and deref
        //

        static void unlock(FrameBuffer* fb) { fb->unlockCache(); }

        static void lock(FrameBuffer* fb) { fb->lockCache(); }

        virtual void clearInternal();

        //
        //  Override this to change the collector policy. This function
        //  should free at least the number of bytes passed to it. If the
        //  cache has overflowed past its max, it should try to free the
        //  overage in addition to the passed in bytes.
        //

        virtual bool free(size_t bytes);

        bool freeTrash(size_t bytes);
        int trashCount() const;
        bool trashContains(FrameBuffer* fb);
        size_t deleteFB(FrameBuffer* fb);

        static bool retrievalCompare(FrameBuffer* a, FrameBuffer* b)
        {
            return a->m_retrievalTime > b->m_retrievalTime;
        }

        //
        //  Derived classes can access FB cache members via these
        //  functions. The base class references the fb in add and
        //  dereferences it in flush.
        //

        void referenceFB(FrameBuffer* fb);
        void dereferenceFB(FrameBuffer* fb);

        size_t fbReferenceCount(FrameBuffer* fb) const
        {
            return fb->m_cacheRef;
        }

        bool hasOneReference(FrameBuffer* fb) const
        {
            return fb->m_cacheRef == 1;
        }

    protected:
        bool m_full;
        size_t m_maxBytes;
        size_t m_currentBytes;
        size_t m_retrieveTime;
        FBMap m_map;
        mutable pthread_mutex_t m_mutex;

        TrashCan* m_trashCan;

    public:
        static bool& debug();
    };

    //
    //  This is for debugging
    //

#ifdef TWK_CACHE_LOCK_DEBUG
#include <sstream>
#define TWK_CACHE_LOCK(C, M)                                \
    do                                                      \
    {                                                       \
        C.lock();                                           \
        TwkFB::Cache::LockLog& l = TwkFB::Cache::locklog(); \
        std::ostringstream str;                             \
        str << __FILE__ << " : " << M;                      \
        l.push_back(std::make_pair(str.str(), __LINE__));   \
        if (l.size() > 10)                                  \
            l.pop_front();                                  \
    } while (false)

#define TWK_CACHE_UNLOCK(C, M)                              \
    do                                                      \
    {                                                       \
        TwkFB::Cache::LockLog& l = TwkFB::Cache::locklog(); \
        std::ostringstream str;                             \
        str << __FILE__ << " : " << M;                      \
        l.push_back(std::make_pair(str.str(), -__LINE__));  \
        if (l.size() > 10)                                  \
            l.pop_front();                                  \
        C.unlock();                                         \
    } while (false)
#else
#define TWK_CACHE_LOCK(C, M) C.lock()
#define TWK_CACHE_UNLOCK(C, M) C.unlock()
#endif

} // namespace TwkFB

extern "C"
{
    TWKFB_EXPORT void outputLockLog(void);
}

#endif // __TwkFB__Cache__h__
