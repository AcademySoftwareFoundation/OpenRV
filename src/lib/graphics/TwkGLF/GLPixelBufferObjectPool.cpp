///*****************************************************************************/
// Copyright (c) 2019 Autodesk, Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//*****************************************************************************/

#include <TwkGLF/GLPixelBufferObjectPool.h>

#include <TwkGLF/GLSyncObject.h>

#include <TwkUtil/EnvVar.h>
#include <TwkUtil/Macros.h>
#include <TwkUtil/sgcHop.h>
#include <TwkUtil/sgcHopTools.h>
#include <TwkUtil/SystemInfo.h>

#include <deque>
#include <map>
#include <mutex>
#include <string.h>

namespace
{
    void printVarType(const char* sVar, const char* sType)
    {
        printf("environment variable \"%s\" should be a %s\n", sVar, sType);
    }

    template <class T, T Ini_> struct Initializer
    {
        Initializer(T* pv, const char*) { *pv = Ini_; }
    };

    template <bool Ini_> struct Initializer<bool, Ini_>
    {
        Initializer(bool* pv, const char* sName)
        {
            *pv = Ini_;
            const char* sEnv = getenv(sName);
            if (sEnv)
            {
                if (strstr("on+yes+true+1+ON+YES+TRUE", sEnv) != 0)
                    *pv = true;
                else if (strstr("off-no-false-0-OFF-NO-FALSE", sEnv) != 0)
                    *pv = false;
                else
                {
                    printVarType(sName, "binary flag");
                    return;
                }
                printf("%s = %s\n", sName, *pv ? "ON" : "OFF");
            }
        }
    };

    template <unsigned Ini_> struct Initializer<unsigned, Ini_>
    {
        Initializer(unsigned* pv, const char* sName)
        {
            *pv = Ini_;
            const char* sEnv = getenv(sName);
            if (sEnv)
            {
                if (sscanf(sEnv, "%u", pv) < 1)
                    printVarType(sName, "number");
                else
                    printf("%s = %s\n", sName, sEnv);
            }
        }
    };

    template <size_t Ini_> struct Initializer<size_t, Ini_>
    {
        Initializer(size_t* pv, const char* sName)
        {
            *pv = Ini_;
            const char* sEnv = getenv(sName);
            if (sEnv)
            {
                if (sscanf(sEnv, "%zi", pv) < 1)
                    printVarType(sName, "number");
                else
                    printf("%s = %s\n", sName, sEnv);
            }
        }
    };

    // A SafeStatic is guaranteed to be initialized when you use it.
    template <const char S_[], class T, T Ini_> class SafeStatic
    {
        static const T& _set(const T* pv)
        {
            static T _v;
            static Initializer<T, Ini_> init(&_v, getName());
            return pv ? _v = *pv : _v;
        }

    public:
        const T& get() const { return _set(0x0); }

        void set(const T& v) { _set(&v); }

        operator const T() const { return get(); }

        SafeStatic& operator=(const T& v) { return set(v), *this; }

        static const char* getName() { return S_; }
    };
} // namespace

//------------------------------------------------------------------------------
//

namespace TwkGLF
{

#define RV_MAKE_STATIC(name, type, init, var) \
    char name[] = RV_STRING(name);            \
    SafeStatic<name, type, init> var

    RV_MAKE_STATIC(RV_PREFETCH_USE_PBOS, bool, true, prefetchUsePBOs);
    RV_MAKE_STATIC(s, size_t, 1024UL, prefetchPoolMB);
    RV_MAKE_STATIC(RV_PREFETCH_POOL_MAX_NB_BUFFERS, unsigned, 7U,
                   prefetchPoolMaxNbBuffers);
    RV_MAKE_STATIC(RV_PREFETCH_USE_FIXED_SIZE_PBOS, bool, true,
                   prefetchUseFixedSizePBOs);
    RV_MAKE_STATIC(RV_PREFETCH_FIXED_SIZE_PBOS_MAX_SIZE, size_t,
                   4032 * 4536 * 6, prefetchFixedSizePBOsMaxSize);
    RV_MAKE_STATIC(RV_PREFETCH_FIXED_SIZE_PBOS_MIN_SIZE, size_t,
                   1920 * 1080 * 6, prefetchFixedSizePBOsMinSize);
    RV_MAKE_STATIC(RV_PREFETCH_FIXED_SIZE_PBOS_MIN_NB_BUFFERS, unsigned, 3U,
                   prefetchFixedSizePBOsMinNbBuffers);
    RV_MAKE_STATIC(RV_WRITE_BEHIND_USE_PBOS, bool, false, writeBehindUsePBOs);
    RV_MAKE_STATIC(RV_WRITE_BEHIND_QUEUE_LENGTH, unsigned, 3U,
                   asyncOutputQueueLength);

    static ENVVAR_INT(evDebug, "RV_PBO_POOL_DEBUG", 0);

    //------------------------------------------------------------------------------
    //
    unsigned PipelinePrefetchPoolMaxNbBuffers()
    {
        return prefetchPoolMaxNbBuffers;
    }

    //------------------------------------------------------------------------------
    //
    unsigned setPipelinePrefetchPoolMaxNbBuffers(unsigned maxNbBuffers)
    {
        if (maxNbBuffers != prefetchPoolMaxNbBuffers)
        {
            unsigned prev = prefetchPoolMaxNbBuffers;
            prefetchPoolMaxNbBuffers = maxNbBuffers;
            RV_LOG("Pipeline : Frame prefetching pool max nb buffers = %u\n",
                   maxNbBuffers);
            return prev;
        }
        return prefetchPoolMaxNbBuffers;
    }

    //------------------------------------------------------------------------------
    //
    size_t PipelinePrefetchPoolBytes() { return prefetchPoolMB << 20; }

    //------------------------------------------------------------------------------
    //
    size_t setPipelinePrefetchPoolMB(size_t nbMegaBytes)
    {
        if (nbMegaBytes != prefetchPoolMB)
        {
            size_t prev = prefetchPoolMB;
            prefetchPoolMB = nbMegaBytes;
            RV_LOG("Pipeline : Frame prefetching pool size = %zd (%zd MB)\n",
                   PipelinePrefetchPoolBytes(), nbMegaBytes);
            return prev;
        }
        return prefetchPoolMB;
    }

    //------------------------------------------------------------------------------
    //
    bool PipelinePrefetchUseFixedSizePBOs() { return prefetchUseFixedSizePBOs; }

    //------------------------------------------------------------------------------
    //
    void printPrefetchUseFixedSizePBOs()
    {
        RV_LOG("Pipeline : %s fixed size PBOs in frame prefetching\n",
               PipelinePrefetchUseFixedSizePBOs() ? "enabled" : "disabled");
    }

    //------------------------------------------------------------------------------
    //
    void setPipelinePrefetchUseFixedSizePBOs(bool fOn)
    {
        const bool prev = prefetchUseFixedSizePBOs;
        prefetchUseFixedSizePBOs = fOn;
        if (prev != fOn)
            printPrefetchUseFixedSizePBOs();
    }

    //------------------------------------------------------------------------------
    //
    size_t PipelinePrefetchFixedSizePBOsMaxSize()
    {
        return prefetchFixedSizePBOsMaxSize;
    }

    //------------------------------------------------------------------------------
    //
    size_t setPipelinePrefetchFixedSizePBOsMaxSize(size_t maxSize)
    {
        if (maxSize != prefetchFixedSizePBOsMaxSize)
        {
            size_t prev = prefetchFixedSizePBOsMaxSize;
            prefetchFixedSizePBOsMaxSize = maxSize;
            return prev;
        }
        return prefetchFixedSizePBOsMaxSize;
    }

    //------------------------------------------------------------------------------
    //
    size_t PipelinePrefetchFixedSizePBOsMinSize()
    {
        return prefetchFixedSizePBOsMinSize;
    }

    //------------------------------------------------------------------------------
    //
    size_t setPipelinePrefetchFixedSizePBOsMinSize(size_t minSize)
    {
        if (minSize != prefetchFixedSizePBOsMinSize)
        {
            size_t prev = prefetchFixedSizePBOsMinSize;
            prefetchFixedSizePBOsMinSize = minSize;
            return prev;
        }
        return prefetchFixedSizePBOsMinSize;
    }

    //------------------------------------------------------------------------------
    //
    unsigned PipelinePrefetchFixedSizePBOsMinNbBuffers()
    {
        return prefetchFixedSizePBOsMinNbBuffers;
    }

    //------------------------------------------------------------------------------
    //
    unsigned setPipelinePrefetchFixedSizePBOsMinNbBuffers(unsigned minNbBuffers)
    {
        if (minNbBuffers != prefetchFixedSizePBOsMinNbBuffers)
        {
            unsigned prev = prefetchFixedSizePBOsMinNbBuffers;
            prefetchFixedSizePBOsMinNbBuffers = minNbBuffers;
            return prev;
        }
        return prefetchFixedSizePBOsMinNbBuffers;
    }

    //------------------------------------------------------------------------------
    //
    unsigned PipelineWriteBehindQueueLength() { return asyncOutputQueueLength; }

    //------------------------------------------------------------------------------
    //
    unsigned setPipelineWriteBehindQueueLength(unsigned queueLength)
    {
        if (queueLength != asyncOutputQueueLength)
        {
            unsigned prev = asyncOutputQueueLength;
            asyncOutputQueueLength = queueLength;
            RV_LOG("Pipeline : Write behind queue length = %u\n", queueLength);
            return prev;
        }
        return asyncOutputQueueLength;
    }

    //------------------------------------------------------------------------------
    //
    class GLPixelBufferObjectPool
    {
        GLPixelBufferObject::PackDir _dir;

        size_t _softMaxSize;      // 0 means no max.
        size_t _softMaxNbBuffers; // 0 means no max.
        float _hardMaxMinFreeMemory;

        bool _recycleExactSizeOnly;
        float _invRecycleMinPercentage;

        size_t _maxPBOSize; // 0 means no max.

        struct PBOInfo
        {
            PBOInfo(GLPixelBufferObject* p, GLSyncObject* f, size_t s, bool g)
                : pbo(p)
                , fence(f)
                , stamp(s)
                , purgeable(g)
            {
            }

            GLPixelBufferObject* pbo;
            GLSyncObject* fence;
            size_t stamp;
            bool purgeable;
        };

        typedef std::deque<PBOInfo> PBOQueue;
        typedef std::map<size_t, PBOQueue> QueueMap;

        struct PBOInfoCompare
        {
        public:
            explicit PBOInfoCompare(GLPixelBufferObject* pbo)
                : _pbo(pbo)
            {
            }

            bool operator()(const PBOInfo& elem) const
            {
                return _pbo == elem.pbo;
            }

        private:
            GLPixelBufferObject* _pbo{nullptr};
        };

        mutable std::mutex _mutex;
        QueueMap _freePool;
        QueueMap _usedPool;
        size_t _curStamp;
        size_t _allocSize;
        size_t _allocNbBuffers;

        // Copy and assignment are prohibited.
        GLPixelBufferObjectPool(const GLPixelBufferObjectPool&);
        GLPixelBufferObjectPool& operator=(const GLPixelBufferObjectPool&);

        GLPixelBufferObject* _recycle(size_t size)
        {
            std::unique_lock<decltype(_mutex)> guard(_mutex);

            GLPixelBufferObject* pbo = NULL;

            const size_t maxSize =
                _recycleExactSizeOnly
                    ? size
                    : static_cast<size_t>(size * _invRecycleMinPercentage);

            QueueMap::iterator p = _freePool.lower_bound(size),
                               pEnd = _freePool.end();

            if (p != pEnd && (maxSize == 0.0f || p->first <= maxSize))
            {
                PBOQueue& q = p->second;
                PBOInfo& pboInfo = q.front(); // LRU.

                // Since we recycle the LRU there's no point in trying another
                // PBO if the fence for the LRU hasn't been reached.
                if (pboInfo.fence->testFence())
                {
                    pbo = pboInfo.pbo;

                    pboInfo.stamp = ++_curStamp;
                    _usedPool[p->first].push_back(pboInfo);

                    q.pop_front();
                    if (q.empty())
                        _freePool.erase(p);
                }
            }

            return pbo;
        }

        bool _purgeNoLock(size_t minSize, size_t minNbBuffers)
        {
            size_t purgedSize = 0;
            size_t purgedNbBuffer = 0;

            while (!_freePool.empty() && (!minSize || (purgedSize < minSize))
                   && (!minNbBuffers || (purgedNbBuffer < minNbBuffers)))
            {
                QueueMap::iterator p, pEnd = _freePool.end();

                QueueMap::iterator minPool = pEnd;
                PBOQueue::iterator minPBO;

                size_t minStamp = 0;

                for (p = _freePool.begin(); p != pEnd; ++p)
                {
                    PBOQueue& q = p->second;

                    // LRU.
                    PBOQueue::iterator it, end = q.end();
                    for (it = q.begin(); it != end; ++it)
                    {
                        if (it->purgeable
                            && (minPool == pEnd || it->stamp < minStamp))
                        {
                            minPool = p;
                            minPBO = it;
                            minStamp = it->stamp;

                            break;
                        }
                    }
                }

                if (minPool != pEnd)
                {
                    delete minPBO->pbo;
                    delete minPBO->fence;

                    purgedSize += minPool->first;
                    ++purgedNbBuffer;

                    PBOQueue& q = minPool->second;
                    q.erase(minPBO);
                    if (q.empty())
                        _freePool.erase(minPool);
                }
                else
                {
                    break;
                }
            }

            _allocSize -= purgedSize;
            _allocNbBuffers -= purgedNbBuffer;

            return (purgedSize >= minSize) && (purgedNbBuffer >= minNbBuffers);
        }

        void _dumpNoLock(bool details)
        {
            printf("%s pool state, size %zd:\n",
                   _dir == GLPixelBufferObject::TO_GPU ? "To GPU" : "From GPU",
                   _allocSize);

            printf("   %zd free queues\n", _freePool.size());
            QueueMap::const_iterator p, pEnd = _freePool.end();
            for (p = _freePool.begin(); p != pEnd; ++p)
            {
                printf("      %zd of size %zd\n", p->second.size(), p->first);
                if (details)
                {
                    PBOQueue::const_iterator q, qEnd = p->second.end();
                    for (q = p->second.begin(); q != qEnd; ++q)
                    {
                        printf("        free, size %u stamp %zd purgeable %d "
                               "(%p)\n",
                               q->pbo->getSize(), q->stamp, q->purgeable,
                               q->pbo);
                    }
                }
            }

            printf("   %zd used queues\n", _usedPool.size());
            pEnd = _usedPool.end();
            for (p = _usedPool.begin(); p != pEnd; ++p)
            {
                printf("      %zd of size %zd\n", p->second.size(), p->first);
                if (details)
                {
                    PBOQueue::const_iterator q, qEnd = p->second.end();
                    for (q = p->second.begin(); q != qEnd; ++q)
                    {
                        printf("        used, size %u stamp %zd purgeable %d "
                               "(%p)\n",
                               q->pbo->getSize(), q->stamp, q->purgeable,
                               q->pbo);
                    }
                }
            }
        }

        void _cleanupNoLock(const QueueMap& pool)
        {
            QueueMap::const_iterator p, pEnd = pool.end();
            for (p = pool.begin(); p != pEnd; ++p)
            {
                PBOQueue::const_iterator q, qEnd = p->second.end();
                for (q = p->second.begin(); q != qEnd; ++q)
                {
                    delete q->pbo;
                    delete q->fence;
                }
            }
        }

        void _checkNoLock()
        {
            // Check that PBOs are either in the free or used pool.
            QueueMap::const_iterator p, pEnd = _freePool.end();
            for (p = _freePool.begin(); p != pEnd; ++p)
            {
                PBOQueue::const_iterator q, qEnd = p->second.end();
                for (q = p->second.begin(); q != qEnd; ++q)
                {
                    QueueMap::const_iterator r, rEnd = _usedPool.end();
                    for (r = _usedPool.begin(); r != rEnd; ++r)
                    {
                        PBOQueue::const_iterator s, sEnd = r->second.end();
                        for (s = r->second.begin(); s != sEnd; ++s)
                            assert(q->pbo != s->pbo);
                    }
                }
            }
        }

    public:
        explicit GLPixelBufferObjectPool(GLPixelBufferObject::PackDir dir)
            : _dir(dir)
            , _softMaxSize(0)
            , _softMaxNbBuffers(0)
            , _hardMaxMinFreeMemory(0.0f)
            , _recycleExactSizeOnly(false)
            , _invRecycleMinPercentage(1.0f)
            , _maxPBOSize(0)
            , _curStamp(0)
            , _allocSize(0)
            , _allocNbBuffers(0)
        {
        }

        ~GLPixelBufferObjectPool()
        {
            std::unique_lock<decltype(_mutex)> guard(_mutex);
            _cleanupNoLock(_freePool);
            _cleanupNoLock(_usedPool);
        }

        void setSoftMaxSize(size_t softMaxSize)
        {
            std::unique_lock<decltype(_mutex)> guard(_mutex);
            _softMaxSize = softMaxSize;
        }

        void setSoftMaxNbBuffers(size_t softMaxNbBuffers)
        {
            std::unique_lock<decltype(_mutex)> guard(_mutex);
            _softMaxNbBuffers = softMaxNbBuffers;
        }

        void setHardMaxMinFreeMemory(float minFreeMemory)
        {
            std::unique_lock<decltype(_mutex)> guard(_mutex);
            _hardMaxMinFreeMemory = minFreeMemory;
        }

        void setRecycleParams(bool exactSizeOnly, float minPercentage = 1.0f)
        {
            std::unique_lock<decltype(_mutex)> guard(_mutex);
            _recycleExactSizeOnly = exactSizeOnly;
            if (minPercentage == 0.0f)
                _invRecycleMinPercentage = 0.0f; // threated as inf.
            else
                _invRecycleMinPercentage = 1.0f / minPercentage;
        }

        void setMaxPBOSize(size_t maxPBOSize)
        {
            std::unique_lock<decltype(_mutex)> guard(_mutex);
            _maxPBOSize = maxPBOSize;
        }

        GLPixelBufferObject*
        pop(size_t size,
            bool purgeable = true /* only used when first allocated */
        )
        {
            if (_maxPBOSize != 0 && size > _maxPBOSize)
                return NULL;

            GLPixelBufferObject* pbo = NULL;

            // Recycle.
            pbo = _recycle(size);

            if (!pbo)
            {
                // Allocate new PBO.

                // Try to purge if the new allocation is going to exceed the
                // soft maximums.
                std::unique_lock<decltype(_mutex)> guard(_mutex);

                bool overSoftMaxSize = false;
                if (_softMaxSize > 0 && (_allocSize + size) > _softMaxSize)
                {
                    overSoftMaxSize =
                        !_purgeNoLock((_allocSize + size) - _softMaxSize, 0);
                }

                bool overSoftMaxNbBuffer = false;
                if (_softMaxNbBuffers > 0
                    && (_allocNbBuffers + 1) > _softMaxNbBuffers)
                {
                    overSoftMaxNbBuffer = !_purgeNoLock(
                        0, (_allocNbBuffers + 1) - _softMaxNbBuffers);
                }

                if (overSoftMaxSize || overSoftMaxNbBuffer)
                {
                    // Go over the soft limits as long as the hard limit is not
                    // reached.
                    size_t physTotal = 0, physFree = 0;
                    TwkUtil::SystemInfo::getSystemMemoryInfo(
                        &physTotal, &physFree, nullptr, nullptr, nullptr,
                        nullptr, nullptr, nullptr);

                    float freeMemory = static_cast<float>(physFree) / physTotal;
                    if (freeMemory < _hardMaxMinFreeMemory)
                        return NULL;
                }

                pbo = new GLPixelBufferObject(_dir, size);

                _usedPool[size].push_back(
                    PBOInfo(pbo, new GLSyncObject, ++_curStamp, purgeable));
                _allocSize += size;
                ++_allocNbBuffers;
            }
            else
            {
                // If the pool went above one of the soft maximums try to purge.
                std::unique_lock<decltype(_mutex)> guard(_mutex);

                if (_softMaxSize > 0 && _allocSize > _softMaxSize)
                    _purgeNoLock(_allocSize - _softMaxSize, 0);

                if (_softMaxNbBuffers > 0
                    && _allocNbBuffers > _softMaxNbBuffers)
                    _purgeNoLock(0, _allocNbBuffers - _softMaxNbBuffers);
            }

            if (evDebug.getValue())
            {
                dump();
            }

            return pbo;
        }

        void push(GLPixelBufferObject* pbo)
        {
            {
                size_t size = pbo->getSize();

                std::unique_lock<decltype(_mutex)> guard(_mutex);

                QueueMap::iterator p = _usedPool.find(size);
                assert(p != _usedPool.end());

                PBOQueue& q = p->second;
                PBOQueue::iterator it =
                    std::find_if(q.begin(), q.end(), PBOInfoCompare(pbo));
                assert(it != q.end());

                PBOInfo& pboInfo = *it;

                pboInfo.fence->setFence();
                pboInfo.stamp = ++_curStamp;
                _freePool[size].push_back(pboInfo);

                q.erase(it);
                if (q.empty())
                    _usedPool.erase(p);
            }
        }

        void dump()
        {
            std::unique_lock<decltype(_mutex)> guard(_mutex);
            _dumpNoLock(evDebug.getValue() > 1);
        }

        unsigned numPBOs()
        {
            std::unique_lock<decltype(_mutex)> guard(_mutex);

            unsigned totalNumPBOs = 0;

            QueueMap::const_iterator p, pEnd = _freePool.end();
            for (p = _freePool.begin(); p != pEnd; ++p)
                totalNumPBOs += p->second.size();

            pEnd = _usedPool.end();
            for (p = _usedPool.begin(); p != pEnd; ++p)
                totalNumPBOs += p->second.size();

            return totalNumPBOs;
        }
    };

    //----------------------------------------------------------------------------
    //
    class PBOWrap
    {
        static bool gPoolToGPUInitialized;
        static GLPixelBufferObjectPool gPoolToGPU;
        static bool gPoolFromGPUInitialized;
        static GLPixelBufferObjectPool gPoolFromGPU;

        GLPixelBufferObject* _pPBO;

    public:
        PBOWrap(GLPixelBufferObject::PackDir dir, unsigned int num_bytes)
        {
            if (dir == GLPixelBufferObject::TO_GPU)
            {
                if (!gPoolToGPUInitialized)
                    initPBOPool(dir);

                _pPBO = gPoolToGPU.pop(num_bytes);
            }
            else
            {
                if (!gPoolFromGPUInitialized)
                    initPBOPool(dir);

                _pPBO = gPoolFromGPU.pop(num_bytes);
            }
        }

        ~PBOWrap()
        {
            if (_pPBO)
            {
                if (_pPBO->getPackDir() == GLPixelBufferObject::TO_GPU)
                    gPoolToGPU.push(_pPBO);
                else
                    gPoolFromGPU.push(_pPBO);
            }
        }

        static void initPBOPool(GLPixelBufferObject::PackDir dir)
        {
            HOP_PROF_FUNC();

            if (dir == GLPixelBufferObject::TO_GPU)
            {
                if (!gPoolToGPUInitialized)
                {
                    if (PipelinePrefetchUseFixedSizePBOs())
                    {
                        assert(PipelinePrefetchPoolMaxNbBuffers() > 0);

                        // We need to allocate more buffers in the pool than
                        // the prefetcher can use to account for buffers needed
                        // for processing.
                        const float extraNbBuffers = 1.05f;

                        const size_t pboMinSize =
                            PipelinePrefetchFixedSizePBOsMinSize();
                        const unsigned poolMinNbBuffers = static_cast<unsigned>(
                            ceilf(PipelinePrefetchFixedSizePBOsMinNbBuffers()
                                  * extraNbBuffers));
                        const size_t poolMinSize =
                            pboMinSize * poolMinNbBuffers;

                        size_t pboSize = PipelinePrefetchFixedSizePBOsMaxSize();

                        unsigned poolNbBuffers = static_cast<unsigned>(
                            ceilf(PipelinePrefetchPoolMaxNbBuffers()
                                  * extraNbBuffers));
                        size_t poolSize = pboSize * poolNbBuffers;

                        size_t poolUpperLimit = 0;
                        // TODO: Insert resource manager here
                        if (poolUpperLimit == 0)
                        {
                            poolUpperLimit = poolSize;
                        }

                        // If minimum size for the fixed size PBO pool exceeds
                        // the limits use the regular PBO pool.
                        if (poolMinSize > poolUpperLimit)
                        {
                            setPipelinePrefetchUseFixedSizePBOs(false);
                            initPBOPool(
                                dir); // Re-init without fixed size PBO pool.

                            return;
                        }

                        // Adjust the number of buffers and/or the buffer size
                        // to ensure we respect the limits.
                        if (poolSize > poolUpperLimit)
                        {
                            // Maximum size for the fixed size PBO pool exceed
                            // the limits. First, try to reduce the buffer size
                            // up to the buffer minimum size.
                            poolSize = poolUpperLimit;
                            pboSize = poolSize / poolNbBuffers;

                            if (pboMinSize > pboSize)
                            {
                                // Reducing the buffer size wasn't enough reduce
                                // the number of buffer. We are guaranteed that
                                // this will succeed since we already ensured
                                // that (poolMinSize <= poolUpperLimit).
                                poolNbBuffers = poolSize / pboMinSize;
                                pboSize = poolSize / poolNbBuffers;
                                setPipelinePrefetchPoolMaxNbBuffers(
                                    poolNbBuffers);
                            }
                        }

                        gPoolToGPU.setSoftMaxNbBuffers(poolNbBuffers);
                        gPoolToGPU.setRecycleParams(false, 0.0f);
                        gPoolToGPU.setMaxPBOSize(pboSize);

                        // Allocate the fixed size buffers.
                        std::vector<GLPixelBufferObject*> pbos;
                        for (int i = 0; i < poolNbBuffers; ++i)
                        {
                            GLPixelBufferObject* pbo =
                                gPoolToGPU.pop(pboSize, false);
                            pbo->map();
                            pbo->unmap();
                            pbos.push_back(pbo);
                        }

                        for (int i = 0; i < poolNbBuffers; ++i)
                            gPoolToGPU.push(pbos[i]);

                        // Need to be set after the fixed size buffers are
                        // allocated to ensure we allocate the fixed size
                        // buffers regardless.
                        gPoolToGPU.setHardMaxMinFreeMemory(0.10f);
                    }
                    else
                    {
                        // The 30% extra is to allocate space for the buffers
                        // needed for processing.
                        size_t poolSize = PipelinePrefetchPoolBytes() * 1.3f;

                        size_t poolUpperLimit = 0;
                        // TODO: Insert resource manager here
                        if (poolUpperLimit == 0)
                            poolUpperLimit = poolSize;

                        gPoolToGPU.setSoftMaxSize(
                            static_cast<size_t>(poolUpperLimit));
                        gPoolToGPU.setHardMaxMinFreeMemory(0.10f);
                        gPoolToGPU.setRecycleParams(false, 0.75f);
                    }

                    glFinish();

                    gPoolToGPUInitialized = true;
                }
            }
            else
            {
                if (!gPoolFromGPUInitialized)
                {
                    // The +2 is because two extra buffers can be allocated by
                    // the write-behind for requests waiting to be queued.
                    gPoolFromGPU.setSoftMaxNbBuffers(
                        PipelineWriteBehindQueueLength() + 2);
                    gPoolFromGPU.setHardMaxMinFreeMemory(0.10f);
                    gPoolFromGPU.setRecycleParams(true);

                    gPoolFromGPUInitialized = true;
                }
            }
        }

        static void uninitPBOPool(GLPixelBufferObject::PackDir dir)
        {
            if (dir == GLPixelBufferObject::TO_GPU)
            {
                if (gPoolToGPUInitialized)
                {
                    gPoolToGPUInitialized = false;
                }
            }
            else
            {
                if (gPoolFromGPUInitialized)
                {
                    gPoolFromGPUInitialized = false;
                }
            }
        }

        operator GLPixelBufferObject*() const { return _pPBO; }

        void bind()
        {
            if (_pPBO)
                _pPBO->bind();
        }

        void unbind()
        {
            if (_pPBO)
                _pPBO->unbind();
        }

        void* map() { return _pPBO ? _pPBO->map() : NULL; }

        // Return the PBO mapped address only if it is already mapped. We
        // want to have this option since doing a map/write/unmap/map
        // doesn't guarantee that the content of the mapped memory will
        // still contain the data you written. Moreover, doing
        // map/write/unmap/upload/map/unmap/upload can result in uploading
        // undefined data.
        void* getMappedPtr() { return _pPBO ? _pPBO->getMappedPtr() : NULL; }

        void unmap()
        {
            if (_pPBO)
                _pPBO->unmap();
        }

        unsigned int getSize() const { return _pPBO ? _pPBO->getSize() : 0; }

        void copyBufferData(void* data, unsigned size)
        {
            if (_pPBO)
                _pPBO->copyBufferData(data, size);
        }

        GLPixelBufferObject::PackDir getPackDir() const
        {
            return _pPBO ? _pPBO->getPackDir() : GLPixelBufferObject::TO_GPU;
        }
    };

    bool PBOWrap::gPoolToGPUInitialized = false;
    bool PBOWrap::gPoolFromGPUInitialized = false;
    GLPixelBufferObjectPool PBOWrap::gPoolToGPU(GLPixelBufferObject::TO_GPU);
    GLPixelBufferObjectPool
        PBOWrap::gPoolFromGPU(GLPixelBufferObject::FROM_GPU);

    //-----------------------------------------------------------------------------
    //
    GLPixelBufferObjectFromPool::GLPixelBufferObjectFromPool(
        GLPixelBufferObject::PackDir dir, unsigned int num_bytes)
    {
        _pPBOWrap = new PBOWrap(dir, num_bytes);
    }

    GLPixelBufferObjectFromPool::~GLPixelBufferObjectFromPool()
    {
        delete _pPBOWrap;
    }

    void GLPixelBufferObjectFromPool::bind() { return _pPBOWrap->bind(); }

    void GLPixelBufferObjectFromPool::unbind() { return _pPBOWrap->unbind(); }

    void* GLPixelBufferObjectFromPool::map() { return _pPBOWrap->map(); }

    void* GLPixelBufferObjectFromPool::getMappedPtr()
    {
        return _pPBOWrap->getMappedPtr();
    }

    void GLPixelBufferObjectFromPool::unmap() { _pPBOWrap->unmap(); }

    unsigned int GLPixelBufferObjectFromPool::getSize() const
    {
        return _pPBOWrap->getSize();
    }

    void GLPixelBufferObjectFromPool::copyBufferData(void* data, unsigned size)
    {
        _pPBOWrap->copyBufferData(data, size);
    }

    GLPixelBufferObject::PackDir GLPixelBufferObjectFromPool::getPackDir() const
    {
        return _pPBOWrap->getPackDir();
    }

    //-----------------------------------------------------------------------------
    //
    void InitPBOPools()
    {
        if (prefetchUsePBOs)
            PBOWrap::initPBOPool(GLPixelBufferObject::TO_GPU);

        if (writeBehindUsePBOs)
            PBOWrap::initPBOPool(GLPixelBufferObject::FROM_GPU);
    }

    //-----------------------------------------------------------------------------
    //
    void UninitPBOPools()
    {
        if (prefetchUsePBOs)
            PBOWrap::uninitPBOPool(GLPixelBufferObject::TO_GPU);

        if (writeBehindUsePBOs)
            PBOWrap::uninitPBOPool(GLPixelBufferObject::FROM_GPU);
    }

} // namespace TwkGLF
