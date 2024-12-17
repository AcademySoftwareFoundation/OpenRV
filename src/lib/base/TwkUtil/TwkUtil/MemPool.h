//******************************************************************************
// Copyright (c) 2008 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __TwkUtil__MemPool__h__
#define __TwkUtil__MemPool__h__
#include <string>
#ifdef WIN32
#define ssize_t long
#endif
#include <pthread.h>
#include <TwkUtil/dll_defs.h>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>

#include <map>

namespace TwkUtil
{

    //
    //  A Pool of large blocks of memory used in file IO and as FrameBuffers.
    //
    //  Allocs of blocks larger than the min elem size are remembered, and when
    //  they are freed they are added to the free list (and not really freed).
    //  If there is not enough space in the free list (list > PoolSize) the
    //  oldest (least-recently freed) blocks in the list are freed and removed
    //  from the list until there is room for the incoming block.
    //
    //  When a new large block is requested, the free list is checked first and
    //  if an appropriately-sized (relative size difference is < AllocSlop)
    //  block is found, that block is returned.  If not we do a "real" alloc.
    //
    //  We assume that the target pool size and minimum element size is such
    //  that the pool will have a small number of elements (< 100).
    //

    class TWKUTIL_EXPORT MemPool
    {
    public:
        MemPool(size_t poolSize, size_t minElemSize, float allocSlop);
        ~MemPool();

        //  Find and reuse a free block, or alloc a block (if we can't reuse
        //  one, or if the block is small).
        //
        static void* alloc(size_t size);

        //  Add a block to the FreeList (or "really free" if the block is
        //  small).
        //
        static void dealloc(void* ptr);

        static void initialize();

    private:
        class FreeList;
        class PoolElem;

        typedef boost::mutex Mutex;
        typedef boost::lock_guard<Mutex> LockGuard;
        typedef std::map<const void*, PoolElem*> ElemMap;

        size_t m_poolSize;    //  Max total size (bytes) of mem pool
        size_t m_minElemSize; //  Minimum pool element size (bytes)
        float m_allocSlop;    //  Allowed slop in re-used block size (relative)

        bool m_shortCircuit; //  Fallback to former behavior
        bool m_debugOutput;

        FreeList* m_freeList; //  List of blocks available for re-use
        ElemMap m_elemMap;    //  Map of ptrs to all PoolElems (free or in-use)

        Mutex m_mutex;
    };

} // namespace TwkUtil

#endif // __TwkUtil__MemPool__h__
