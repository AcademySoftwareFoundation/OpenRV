//******************************************************************************
// Copyright (c) 2008 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <TwkUtil/MemPool.h>
#include <TwkUtil/Timer.h>
#include <iostream>
#include <sys/types.h>
#include <string.h>
#include <stl_ext/replace_alloc.h>
#include <algorithm>

namespace TwkUtil
{
    using namespace std;

#define MP_POOL_SIZE (500 * 1024 * 1024)
#define MP_ALLOC_SLOP 0.1
#define MP_MIN_ELEM_SIZE (3 * 1024 * 1024)

    namespace
    {

        MemPool* globalMemPool = 0;

    };

    struct MemPool::PoolElem
    {
        PoolElem(MemPool& mp, void* p, size_t s)
            : memPool(mp)
            , ptr(p)
            , size(s)
            , next(0)
            , prev(0) {};

        ~PoolElem() { memPool.m_elemMap.erase(ptr); }

        float slop(size_t targetSize)
        {
            return (float(size - targetSize) / float(size));
        }

        void* ptr;
        size_t size;
        PoolElem* next;
        PoolElem* prev;

        MemPool& memPool;
    };

    struct MemPool::FreeList
    {
        FreeList(size_t sz, float slop)
            : head(0)
            , tail(0)
            , totalSize(0)
            , totalCount(0)
            , poolSize(sz)
            , allocSlop(slop) {};

        void addElem(PoolElem* elem);
        void* findAndUseElem(size_t size);

        PoolElem* head;
        PoolElem* tail;
        size_t totalSize;
        size_t totalCount;
        size_t poolSize;
        float allocSlop;
    };

    MemPool::MemPool(size_t poolSize, size_t minElemSize, float allocSlop)
        : m_poolSize(poolSize)
        , m_minElemSize(minElemSize)
        , m_allocSlop(allocSlop)
        , m_shortCircuit(false)
        , m_debugOutput(false)
    {
        m_freeList = new FreeList(m_poolSize, m_allocSlop);
    }

    void MemPool::initialize()
    {
        size_t poolSize = MP_POOL_SIZE;
        size_t minElemSize = MP_MIN_ELEM_SIZE;
        float allocSlop = MP_ALLOC_SLOP;

        if (getenv("TWK_MEM_POOL_SIZE"))
            poolSize = atoi(getenv("TWK_MEM_POOL_SIZE")) * 1024 * 1024;
        if (getenv("TWK_MEM_POOL_MIN_ELEM_SIZE"))
            minElemSize = size_t(atof(getenv("TWK_MEM_POOL_MIN_ELEM_SIZE"))
                                 * 1024.0 * 1024.0);
        if (getenv("TWK_MEM_POOL_ALLOC_SLOP"))
            allocSlop = atof(getenv("TWK_MEM_POOL_ALLOC_SLOP"));

        globalMemPool = new MemPool(poolSize, minElemSize, allocSlop);

        globalMemPool->m_shortCircuit = (getenv("TWK_MEM_POOL_DISABLE") != 0);

        if (getenv("TWK_MEM_POOL_DEBUG"))
            globalMemPool->m_debugOutput = true;

        if (globalMemPool->m_debugOutput)
        {
            cerr << "MP: size " << poolSize / (1024 * 1024)
                 << "MB, minElemSize " << float(minElemSize) / (1024.0 * 1024.0)
                 << ", slop " << allocSlop << ", shortCircuit "
                 << globalMemPool->m_shortCircuit << endl;
        }
    }

    void* MemPool::alloc(size_t size)
    {
        //  If not initialized, fallback to original behavior
        //
        if (!globalMemPool)
        {
            return TWK_ALLOCATE_ARRAY_PAGE_ALIGNED(unsigned char, size);
        }

        MemPool& mp(*globalMemPool);
        DebugTimer tmr(mp.m_debugOutput, true);
        bool hit = false;

        LockGuard lg(mp.m_mutex);

        void* ptr = 0;
        if (size < mp.m_minElemSize || mp.m_shortCircuit)
        {
            ptr = TWK_ALLOCATE_ARRAY_PAGE_ALIGNED(unsigned char, size);
        }
        else
        {
            ptr = mp.m_freeList->findAndUseElem(size);
            if (!ptr)
            {
                ptr = TWK_ALLOCATE_ARRAY_PAGE_ALIGNED(unsigned char, size);
                if (mp.m_debugOutput && mp.m_elemMap.count(ptr) != 0)
                {
                    cerr << "ERROR: ptr already in map! " << ptr << endl;
                }
                if (ptr)
                    mp.m_elemMap[ptr] = new PoolElem(mp, ptr, size);
            }
            else
                hit = true;
        }
        if (mp.m_debugOutput && size >= mp.m_minElemSize)
        {
            cerr << "MP: alloc " << size / (1024 * 1024) << "MB "
                 << ((hit) ? "hit, " : "miss, ") << 1000.0 * tmr.stop() << "ms"
                 << endl;
        }

        return ptr;
    }

    void MemPool::dealloc(void* ptr)
    {
        //  If not initialized, fallback to original behavior
        //
        if (!globalMemPool || globalMemPool->m_shortCircuit)
        {
            TWK_DEALLOCATE(ptr);
            return;
        }

        MemPool& mp(*globalMemPool);

        LockGuard lg(mp.m_mutex);

        ElemMap::iterator i = mp.m_elemMap.find(ptr);
        if (i == mp.m_elemMap.end())
        {
            TWK_DEALLOCATE(ptr);
        }
        else
        {
            mp.m_freeList->addElem(i->second);
        }
    }

    void MemPool::FreeList::addElem(PoolElem* elem)
    {
        while (tail && (totalSize + elem->size > poolSize))
        {
            //
            //  Chop off Tail
            //
            PoolElem* discard = tail;
            tail = discard->prev;
            if (tail)
                tail->next = 0;
            else
                head = 0;

            //
            //  Adjust metadata
            //
            totalSize -= discard->size;
            --totalCount;

            //
            //  Dealloc for reals
            //
            TWK_DEALLOCATE(discard->ptr);

            //
            //  Delete elem, which removes it from map
            //
            delete discard;
        }

        //
        //  Add new elem at head
        //
        elem->next = head;
        elem->prev = 0;
        if (head)
            head->prev = elem;
        head = elem;
        if (!tail)
            tail = elem;

        //
        //  Adjust metadata
        //
        totalSize += elem->size;
        ++totalCount;

        if (globalMemPool->m_debugOutput)
        {
            cerr << "MP: freelist total " << totalCount << " elems, "
                 << totalSize / (1024 * 1024) << "MB out of "
                 << poolSize / (1024 * 1024) << "MB" << endl;
        }
    }

    void* MemPool::FreeList::findAndUseElem(size_t size)
    {
        PoolElem* candidate = 0;

        int count = 0;
        for (PoolElem* i = head; i; i = i->next)
        {
            //
            //  Always use an exact match
            //
            if (i->size == size)
            {
                candidate = i;
                break;
            }
            else if (i->size > size)
            {
                //
                //  If we have nothing and this one is within slop allowance,
                //  use it.  If we already have something, use this candidate if
                //  it has less slop.
                //
                if ((!candidate && i->slop(size) < allocSlop)
                    || (candidate && i->slop(size) < candidate->slop(size)))
                {
                    candidate = i;
                }
            }
        }

        if (candidate)
        {
            //
            //  Remove candidate from free list
            //

            if (candidate->prev)
                candidate->prev->next = candidate->next;
            if (candidate->next)
                candidate->next->prev = candidate->prev;

            if (candidate == head)
                head = candidate->next;
            if (candidate == tail)
                tail = candidate->prev;

            candidate->prev = candidate->next = 0;

            //
            //  Adjust metadata
            //

            --totalCount;
            totalSize -= candidate->size;

            return candidate->ptr;
        }

        return 0;
    }

} //  namespace TwkUtil
