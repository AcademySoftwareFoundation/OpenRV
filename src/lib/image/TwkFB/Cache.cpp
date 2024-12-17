//******************************************************************************
// Copyright (c) 2006 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkFB/Exception.h>
#include <TwkFB/Cache.h>
#include <algorithm>
#include <iostream>
#include <list>

namespace TwkFB
{
    using namespace std;

#if 0
#define DB_GENERAL 0x01
#define DB_FLUSH 0x02
#define DB_TC 0x04
#define DB_TCFREE 0x08
#define DB_REF 0x10
#define DB_ALL 0xff

//#define DB_LEVEL        (DB_ALL & (~ DB_EDGES))
//#define DB_LEVEL        DB_FLUSH
//#define DB_LEVEL        (DB_TC | DB_TCFREE | DB_REF)
//#define DB_LEVEL        DB_REF
//#define DB_LEVEL        DB_TC
//#define DB_LEVEL        DB_GENERAL | DB_TC | DB_TCFREE
//#define DB_LEVEL          DB_TCFREE | DB_TC
//#define DB_LEVEL          DB_TCFREE | DB_TC
//#define DB_LEVEL          DB_TCFREE | DB_FLUSH
#define DB_LEVEL DB_ALL

#define DB(x)                    \
    if (DB_GENERAL & (DB_LEVEL)) \
    cerr << dec << "TwkFB::Cache: " << x << endl
#define DBL(level, x)         \
    if ((level) & (DB_LEVEL)) \
    cerr << dec << "TwkFB::Cache: " << x << endl
#else
#define DB(x)
#define DBL(level, x)
#define DB_LEVEL 0x0
#endif

    static Cache::LockLog _locklog;
    static bool _debug = false;

    bool& Cache::debug() { return _debug; }

    Cache::LockLog& Cache::locklog() { return _locklog; }

    class TrashCan
    {
    public:
        TrashCan();
        ~TrashCan();

        void add(FrameBuffer* fb);
        void remove(FrameBuffer* fb);
        bool contains(FrameBuffer* fb);
        FrameBuffer* popOldest();
        int size();

    private:
        typedef std::list<FrameBuffer*> TrashList;
        typedef std::map<FrameBuffer*, TrashList::iterator> TrashMap;

        TrashList trashList;
        TrashMap trashMap;
    };

    TrashCan::TrashCan() {}

    TrashCan::~TrashCan() {}

    void TrashCan::add(FrameBuffer* fb)
    {
        if (!contains(fb))
        {
            DBL(DB_TC, "TrashCan: add    fb "
                           << fb->identifier() << " to   trashCan with "
                           << trashMap.size() << " elements");
            trashList.push_back(fb);
            trashMap[fb] = --trashList.end();
        }
    }

    void TrashCan::remove(FrameBuffer* fb)
    {
        TrashMap::iterator tmi = trashMap.find(fb);
        if (trashMap.end() != tmi)
        {
            DBL(DB_TC, "TrashCan: remove fb "
                           << fb->identifier() << " from trashCan with "
                           << trashMap.size() << " elements");
            trashList.erase(tmi->second);
            trashMap.erase(tmi);
        }
    }

    bool TrashCan::contains(FrameBuffer* fb)
    {
        TrashMap::iterator tmi = trashMap.find(fb);

        return (trashMap.end() != tmi);
    }

    FrameBuffer* TrashCan::popOldest()
    {
        FrameBuffer* fb = 0;
        if (trashList.size())
        {
            fb = trashList.front();
            DBL(DB_TC, "TrashCan: pop    fb "
                           << fb->identifier() << " from trashCan with "
                           << trashMap.size() << " elements");
            trashList.pop_front();
            trashMap.erase(fb);
        }
        return fb;
    }

    int TrashCan::size() { return trashList.size(); }

    Cache::Cache()
        : m_full(false)
        , m_maxBytes(1024 * 1024 * 250)
        , // default is 250Mb
        m_currentBytes(0)
        , m_retrieveTime(0)
    {
        pthread_mutex_init(&m_mutex, 0);

        m_trashCan = new TrashCan();
    }

    Cache::~Cache()
    {
        //
        //  Note: the clearInternal() call here will not call a derived
        //  class's function because the derived class has been destroyed
        //  at this point.
        //

        lock();
        clearInternal();
        unlock();
        delete m_trashCan;
        pthread_mutex_destroy(&m_mutex);
    }

    void Cache::setMemoryUsage(size_t m)
    {
        //  cerr << "Cache::setMemoryUsage before m " << m << " " << m_maxBytes
        //  << endl;
        //
        //  With 32-bit vm, we can't reliably use more than 2.5GB in the
        //  cache, so trim it here.
        //
        if (4 == sizeof(void*))
            m = min(m, size_t(2560) * size_t(1024 * 1024));
        m_maxBytes = m;

        //  cerr << "Cache::setMemoryUsage after " << m_maxBytes << endl;
    }

    void Cache::clearInternal()
    {
        DB("clearInternal()");
        vector<FrameBuffer*> lockedFBs;

        //
        //  We can't safely delete locked fbs in the cache (there are
        //  pointers to them somewhere outside). So in this case we just
        //  delete everything except those fbs and hope for the best. If
        //

        for (FBMap::iterator i = m_map.begin(); i != m_map.end(); ++i)
        {
            if (i->second->isCacheLocked())
            {
                lockedFBs.push_back(i->second);
                DB("    fb is locked: " << i->second << " " << i->first);
            }
            else
            {
                if (Cache::debug())
                    cout << "DELETING: " << i->second << " : "
                         << i->second->identifier() << endl;
                m_trashCan->remove(i->second);
                delete i->second;
            }
        }

        m_map.clear();

        size_t bytes = 0;

        for (size_t i = 0; i < lockedFBs.size(); i++)
        {
            FrameBuffer* fb = lockedFBs[i];
            m_map[fb->identifier()] = fb;
            bytes += fb->totalImageSize();
        }
        DB("    after clearing, Cache holds " << m_map.size() << " fbs ("
                                              << bytes << " bytes)");

        m_currentBytes = bytes;
        m_full = (m_currentBytes >= m_maxBytes);
    }

    void Cache::clear() { clearInternal(); }

    bool Cache::flush(const IDString& idstring)
    {
        DBL(DB_FLUSH, "flush() id " << idstring);
        FBMap::iterator i = m_map.find(idstring);

        if (i != m_map.end())
        {
            FrameBuffer* fb = i->second;
            size_t refcount = fbReferenceCount(fb);
            DBL(DB_FLUSH, "flush() fb " << fb << " count " << refcount);

            if (refcount <= 1)
            {
                if (Cache::debug())
                    cout << "DELETING: " << fb << " : " << fb->identifier()
                         << endl;
                DBL(DB_FLUSH, "flush() deleting id " << idstring << " (fb "
                                                     << fb << " count "
                                                     << refcount << ")");
                m_trashCan->remove(fb);
                if (refcount)
                    fb->m_cacheRef--;
                m_currentBytes -= deleteFB(fb);
                m_map.erase(i);
                m_full = (m_currentBytes >= m_maxBytes);
                return true;
            }
            else
            {
                //
                //  Don't dec ref here.  Refs for other reasons are
                //  taken care of at other levels, and there is one ref
                //  for "inclusion in m_map" and since we're leaving
                //  this fb in the map, we must not remove that
                //  reference here.
                //
                // fb->m_cacheRef--;

                DBL(DB_FLUSH, "flush() skipping " << idstring << " refCount "
                                                  << fbReferenceCount(fb));

                if (Cache::debug())
                    cout << "NO FLUSH: " << fb->identifier()
                         << ", count = " << fbReferenceCount(fb) << endl;

                return false;
            }
        }

        return false;
    }

    bool Cache::isCached(const IDString& idstring) const
    {
        bool b = m_map.find(idstring) != m_map.end();
        return b;
    }

    FrameBuffer* Cache::checkOut(const IDString& idstring)
    {
        FrameBuffer* fb = 0;

        FBMap::iterator i = m_map.find(idstring);

        if (i != m_map.end())
        {
            fb = i->second;
            checkOut(fb);
        }
        else
        {
            if (Cache::debug())
                cout << "CACHE: missed " << fb << " : " << idstring << endl;
        }

        DB("checkOut idstring " << idstring << " returns " << fb);
        return fb;
    }

    void Cache::checkOut(FrameBuffer* fb)
    {
        DB("checkOut fb " << fb << " " << fb->identifier() << " refcnt "
                          << fb->m_cacheRef);
        if (!fb)
        {
            TWK_THROW_STREAM(CacheMismatchException, "null fb ");
        }

        if (!fb->inCache())
        {
            TWK_THROW_STREAM(CacheMismatchException,
                             "fb " << fb << " : " << fb->identifier()
                                   << " is not in cache " << this
                                   << ", lock = " << fb->m_cacheLock
                                   << ", refs = " << fb->m_cacheRef);
        }

        if (Cache::debug())
            cout << "CACHE: checking out " << fb << " : " << fb->identifier()
                 << endl;

        fb->m_retrievalTime = m_retrieveTime;
        m_retrieveTime++;
        lock(fb);
        referenceFB(fb);
    }

    void Cache::checkIn(FrameBuffer* fb)
    {
        DB("checkIn fb " << fb << " " << fb->identifier() << " refcnt "
                         << fb->m_cacheRef);
        if (!fb)
        {
            TWK_THROW_STREAM(CacheMismatchException, "null fb");
        }

        if (!fb->inCache())
        {
            TWK_THROW_STREAM(CacheMismatchException,
                             "fb " << fb << " : "
                                   << (fb ? fb->identifier() : "")
                                   << " is not in cache " << this);
        }

        if (Cache::debug())
            cout << "CACHE checking in " << fb << " : " << fb->identifier()
                 << endl;
        unlock(fb);
        dereferenceFB(fb);
    }

    void Cache::emergencyFree() { (void)free(0); }

    int Cache::trashCount() const { return m_trashCan->size(); }

    bool Cache::freeTrash(size_t bytes)
    {
        // DBL (DB_TCFREE, "freeTrash() bytes " << bytes << " trashCount " <<
        // trashCount());

        FrameBuffer* fb = 0;
        size_t freedBytes = 0;

        while (freedBytes < bytes && (fb = m_trashCan->popOldest()))
        {
            DBL(DB_TCFREE, "freeTrash() freeing bytes "
                               << fb->totalImageSize() << " ("
                               << fb->identifier() << ")");
            m_map.erase(fb->identifier());
            const size_t totalImagesize = deleteFB(fb);

            m_currentBytes -= totalImagesize;
            freedBytes += totalImagesize;
            m_full = (m_currentBytes >= m_maxBytes);
        }

        return (freedBytes >= bytes);
    }

    bool Cache::freeAllTrash() { return freeTrash(m_currentBytes); }

    bool Cache::free(size_t bytes)
    {
        //  XXX
        //
        //  We should never get to this point any more.  Cleanup needed.
        //

        abort();

        /*
        fprintf (stderr, "Cache::free %u\n", bytes);
        fflush (stderr);
        */
        size_t numfbs = m_map.size();
        vector<FrameBuffer*> fbs(numfbs);

        FBMap::iterator it = m_map.begin();
        for (int i = 0; i < numfbs; i++, it++)
            fbs[i] = it->second;

        size_t targetBytes = m_maxBytes - bytes;

        for (int i = 0; i < fbs.size() && (m_currentBytes > targetBytes); i++)
        {
            partial_sort(fbs.begin() + i, fbs.end(), fbs.begin() + (i + 1),
                         Cache::retrievalCompare);

            FrameBuffer* fb = fbs.front();

            if (hasOneReference(fb))
            {
                if (Cache::debug())
                    cout << "CACHE: freeing " << fb->allocSize()
                         << " of needed " << bytes
                         << ", fb = " << fb->identifier() << endl;

                m_map.erase(fb->identifier());
                fbs.front() = 0;
                m_currentBytes -= deleteFB(fb);
                m_full = (m_currentBytes >= m_maxBytes);
            }
        }

        if (m_currentBytes > m_maxBytes)
            return false;
        return m_maxBytes - m_currentBytes >= bytes;
    }

    bool Cache::add(FrameBuffer* fb, bool force)
    {
        DB("add() fb " << fb << " force " << force);
        bool ok = true;
        if (fb->inCache())
            return ok;

        size_t bytes = fb->totalImageSize();

        //  cerr << "Cache::add " << bytes << " to " << m_currentBytes << " of "
        //  << m_maxBytes << " full " << m_full << endl;

        if ((bytes + m_currentBytes) > m_maxBytes && !free(bytes))
        {
            m_full = (m_currentBytes >= m_maxBytes);
            if (!force)
                return false;
            if (Cache::debug())
                cout << "INFO: forcing " << fb->identifier() << endl;
            ok = false;
        }

        m_currentBytes += bytes;
        DB("add() calling referenceFB " << fb);
        referenceFB(fb);
        fb->ownData();
        FBMap::iterator i = m_map.find(fb->identifier());

        if (i != m_map.end())
        {
            if (hasOneReference(i->second))
            {
                if (Cache::debug())
                    cout << "WARNING: CACHE: removing cache locked fb" << endl;
            }

            if (Cache::debug())
                cout << "INFO: CACHE: removing old " << fb->identifier()
                     << endl;
            deleteFB(i->second);
        }

        m_map[fb->identifier()] = fb;
        if (Cache::debug())
            cout << "CACHE: added " << fb->identifier() << endl;

        m_full = (m_currentBytes >= m_maxBytes);
        return ok;
    }

    FrameBuffer* Cache::recycledFB(size_t allocSize[4])
    {
        return new FrameBuffer();
    }

    void Cache::referenceFB(FrameBuffer* fb)
    {
        fb->m_cacheRef++;
        DBL(DB_REF, "referenceFB: " << fb << " " << fb->identifier()
                                    << ", ref count now " << fb->m_cacheRef);
        if (fb->m_cacheRef > 1)
            m_trashCan->remove(fb);
        if (DB_LEVEL && fb->m_cacheRef > 150)
        {
            DB("          VERY HIGH REF COUNT " << fb->m_cacheRef << " "
                                                << fb->identifier());
        }
    }

    void Cache::dereferenceFB(FrameBuffer* fb)
    {
        if (fb->m_cacheRef > 0)
            fb->m_cacheRef--;
        DBL(DB_REF, "dereferenceFB: " << fb << " " << fb->identifier()
                                      << ", ref count now " << fb->m_cacheRef);
        //
        //  The below is dangerous.  flush may be FBCache::flush(), which
        //  calls Cache::dereferenceFB().  If all goes well, the second
        //  level recursive flush() will be a noop, but otherwise we can
        //  get infinite recusion.  It might be best to take this out,
        //  and just let the GC collect any fbs whose ref count drops to
        //  zero.
        //
        //  Addendum to above comment: I've changed FBCache
        //  flush() so that it will definitely be a noop when it's
        //  called recursively, so recursion won't go beyond second
        //  level and the below should not be dangerous now.
        //
        if (fb->m_cacheRef <= 0)
            flush(fb->identifier());
        if (fb->m_cacheRef == 1)
            m_trashCan->add(fb);
    }

    bool Cache::trashContains(FrameBuffer* fb)
    {
        return m_trashCan->contains(fb);
    }

    // This function handles the presence of "proxy buffers" (FrameBuffer) in
    // the cache. Those buffers have a data pointer that refers to another
    // buffer's data (eg.: tiles). When the cache wants to delete a FrameBuffer,
    // instead of simply calling 'delete' on it, it will use this function. The
    // function will then detect if the buffer is a proxy buffer or a master
    // buffer (ie.: a buffer referred-to by proxy buffers).
    //
    // If the buffer is a master buffer, it will proceed to also delete all its
    // associated proxy buffers and remove them from the cache.
    //
    // If the buffer is a proxy buffer, it will unregister itself from the list
    // of proxy buffers kept by its master buffer.
    //
    // The function returns the number of deleted bytes, including proxy
    // buffers, so the cache can properly keep trace of the allocated memory.
    // Note that sub-planes are accounted for in the totalImageSize() function
    // and are deleted through the master-tile destructor.
    size_t Cache::deleteFB(FrameBuffer* fb)
    {
        size_t deletedBytes(0);
        if (fb)
        {
            // Consider the frame buffer size (including its sub-planes).
            deletedBytes = fb->totalImageSize();

            // Check to see if this buffer has proxy-buffer that must also be
            // deleted.
            if (const TwkFB::TypedFBVectorAttribute<FrameBuffer*>*
                    proxyBuffersAtt = dynamic_cast<
                        const TwkFB::TypedFBVectorAttribute<FrameBuffer*>*>(
                        fb->findAttribute("ProxyBuffers")))
            {
                for (int proxyBufferIndex = 0;
                     proxyBufferIndex < proxyBuffersAtt->value().size();
                     proxyBufferIndex++)
                {
                    FrameBuffer* proxyBuffer =
                        proxyBuffersAtt->value()[proxyBufferIndex];
                    if (proxyBuffer)
                    {
                        FBMap::iterator i =
                            m_map.find(proxyBuffer->identifier());
                        // If the proxy buffer is not in the cache map, it has
                        // been deleted prior the master buffer.
                        if (i != m_map.end())
                        {
                            m_map.erase(i);
                            m_trashCan->remove(proxyBuffer);
                            // Consider the proxy buffers size (including their
                            // sub-planes).
                            deletedBytes += proxyBuffer->totalImageSize();

                            // Delete the proxy buffer.
                            delete proxyBuffer;
                        }
                    }
                }
            }

            // Check to see if the buffer is a proxy buffer owned by a master
            // buffer. If so, unregister from the master's list of proxies.
            if (const TwkFB::TypedFBAttribute<
                    FrameBuffer*>* proxyBufferOwnerAtt =
                    dynamic_cast<const TwkFB::TypedFBAttribute<FrameBuffer*>*>(
                        fb->findAttribute("ProxyBufferOwnerPtr")))
            {
                if (proxyBufferOwnerAtt && proxyBufferOwnerAtt->value())
                {
                    if (TwkFB::TypedFBVectorAttribute<
                            FrameBuffer*>* proxyBuffersAtt =
                            dynamic_cast<
                                TwkFB::TypedFBVectorAttribute<FrameBuffer*>*>(
                                proxyBufferOwnerAtt->value()->findAttribute(
                                    "ProxyBuffers")))
                    {
                        if (proxyBuffersAtt)
                        {
                            std::vector<FrameBuffer*>& proxyBuffers =
                                proxyBuffersAtt->value();
                            proxyBuffers.erase(std::remove(proxyBuffers.begin(),
                                                           proxyBuffers.end(),
                                                           fb),
                                               proxyBuffers.end());
                        }
                    }
                }
            }

            // Delete the main buffer.
            delete fb;
        }
        return deletedBytes;
    }

} // namespace TwkFB

extern "C"
{

    using namespace std;

    void outputLockLog(void)
    {
        typedef TwkFB::Cache::LockLog L;

        for (int i = 0; i < TwkFB::Cache::locklog().size(); i++)
        {
            L::value_type& v = TwkFB::Cache::locklog()[i];

            if (v.second < 0)
            {
                cout << "unlock " << v.first << ", line " << -v.second << endl;
            }
            else
            {
                cout << "lock " << v.first << ", line " << v.second << endl;
            }
        }
    }
}
