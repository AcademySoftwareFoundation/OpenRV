//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <Mu/GarbageCollector.h>
#include <Mu/ClassInstance.h>
#include <Mu/Object.h>
#include <Mu/MuProcess.h>
#include <Mu/Thread.h>
#include <Mu/Type.h>
#include <pthread.h>
#include <stl_ext/block_alloc_arena.h>
#include <stl_ext/stl_ext_algo.h>
#include <iostream>

// mR - 10/28/07
#ifndef _MSC_VER
// #include <ucontext.h>
#endif

#ifdef PLATFORM_DARWIN
#include <mach/mach.h>
#include <mach/thread_act.h>
#include <mach/mach_error.h>
#endif

namespace Mu
{
    using namespace stl_ext;
    using namespace std;

    GarbageCollector::API* GarbageCollector::_api = 0;

    bool GarbageCollector::_mark = false;
    bool GarbageCollector::_needsCollection = false;
    int GarbageCollector::_disabled = 0;
    size_t GarbageCollector::_objectsScanned = 0;
    size_t GarbageCollector::_objectsReclaimed = 0;
    GarbageCollector::CollectCallbackFuncs GarbageCollector::_collectCallbacks;

    GarbageCollector::API::API(GarbageCollector::API* next)
        : _next(next)
    {
    }

    GarbageCollector::API::~API() {}

    //----------------------------------------------------------------------

    class GCHeapAPI : public GarbageCollector::API
    {
    public:
        GCHeapAPI(API* next)
            : API(next)
        {
        }

        virtual ~GCHeapAPI() {}

        virtual Pointer allocate(size_t);
        virtual Pointer allocateAtomic(size_t);
        virtual Pointer allocateAtomicOffPage(size_t);
        virtual Pointer allocateOffPage(size_t);
        virtual Pointer allocateStubborn(size_t);
        virtual void beginChangeStubborn(Pointer);
        virtual void endChangeStubborn(Pointer);
        virtual void free(Pointer);
    };

    Pointer GCHeapAPI::allocate(size_t s) { return GC_malloc(s); }

    Pointer GCHeapAPI::allocateAtomic(size_t s) { return GC_malloc_atomic(s); }

    Pointer GCHeapAPI::allocateAtomicOffPage(size_t s)
    {
        return GC_malloc_atomic_ignore_off_page(s);
    }

    Pointer GCHeapAPI::allocateOffPage(size_t s)
    {
        return GC_malloc_ignore_off_page(s);
    }

    Pointer GCHeapAPI::allocateStubborn(size_t s)
    {
        return GC_malloc_stubborn(s);
    }

    void GCHeapAPI::beginChangeStubborn(Pointer p) { GC_change_stubborn(p); }

    void GCHeapAPI::endChangeStubborn(Pointer p) { GC_end_stubborn_change(p); }

    void GCHeapAPI::free(Pointer p) { GC_free(GC_base(p)); }

    //----------------------------------------------------------------------

    class GCHeapNoOptAPI : public GarbageCollector::API
    {
    public:
        GCHeapNoOptAPI(API* next)
            : API(next)
        {
        }

        virtual ~GCHeapNoOptAPI() {}

        virtual Pointer allocate(size_t);
        virtual Pointer allocateAtomic(size_t);
        virtual Pointer allocateAtomicOffPage(size_t);
        virtual Pointer allocateOffPage(size_t);
        virtual Pointer allocateStubborn(size_t);
        virtual void beginChangeStubborn(Pointer);
        virtual void endChangeStubborn(Pointer);
        virtual void free(Pointer);
    };

    Pointer GCHeapNoOptAPI::allocate(size_t s) { return GC_malloc(s); }

    Pointer GCHeapNoOptAPI::allocateAtomic(size_t s) { return GC_malloc(s); }

    Pointer GCHeapNoOptAPI::allocateAtomicOffPage(size_t s)
    {
        return GC_malloc(s);
    }

    Pointer GCHeapNoOptAPI::allocateOffPage(size_t s) { return GC_malloc(s); }

    Pointer GCHeapNoOptAPI::allocateStubborn(size_t s) { return GC_malloc(s); }

    void GCHeapNoOptAPI::beginChangeStubborn(Pointer p) {}

    void GCHeapNoOptAPI::endChangeStubborn(Pointer p) {}

    void GCHeapNoOptAPI::free(Pointer p) {}

    //----------------------------------------------------------------------

    class GCPoolAPI : public GCHeapAPI
    {
    public:
        GCPoolAPI(API* next)
            : GCHeapAPI(next)
            , _total(0)
        {
        }

        virtual ~GCPoolAPI();
        virtual Pointer allocate(size_t);
        virtual Pointer allocateAtomic(size_t);
        virtual Pointer allocateAtomicOffPage(size_t);
        virtual Pointer allocateOffPage(size_t);

        vector<Pointer> _objects;
        size_t _total;
    };

    GCPoolAPI::~GCPoolAPI()
    {
        for (size_t i = 0; i < _objects.size(); i++)
        {
            Pointer obj = _objects[i];
            Pointer base = GC_base(obj);
            Pointer p = *(Pointer*)base;

            if (Pointer t = GC_base(p))
            {
                if (t == p && GarbageCollector::isGCPointerSymbol(p))
                {
                    if (GC_size(t) >= sizeof(Type))
                    {
                        cout << reinterpret_cast<const Type*>(t)
                                    ->fullyQualifiedName()
                             << endl;
                    }
                }
            }

            GC_free(GC_base(obj));
        }

        // cout << "freed " << _total << " bytes in " << _objects.size() << "
        // objects" << endl;
    }

    Pointer GCPoolAPI::allocate(size_t s)
    {
        Pointer p = GC_malloc(s);
        _objects.push_back(p);
        _total += s;
        return p;
    }

    Pointer GCPoolAPI::allocateAtomic(size_t s)
    {
        Pointer p = GC_malloc_atomic(s);
        _objects.push_back(p);
        _total += s;
        return p;
    }

    Pointer GCPoolAPI::allocateAtomicOffPage(size_t s)
    {
        Pointer p = GC_malloc_atomic_ignore_off_page(s);
        _objects.push_back(p);
        _total += s;
        return p;
    }

    Pointer GCPoolAPI::allocateOffPage(size_t s)
    {
        Pointer p = GC_malloc_ignore_off_page(s);
        _objects.push_back(p);
        _total += s;
        return p;
    }

    //----------------------------------------------------------------------

    class GCMallocPoolAPI : public GarbageCollector::API
    {
    public:
        GCMallocPoolAPI(API* next)
            : API(next)
            , _total(0)
        {
        }

        virtual ~GCMallocPoolAPI();
        virtual Pointer allocate(size_t);
        virtual Pointer allocateAtomic(size_t);
        virtual Pointer allocateAtomicOffPage(size_t);
        virtual Pointer allocateOffPage(size_t);
        virtual Pointer allocateStubborn(size_t);
        virtual void beginChangeStubborn(Pointer);
        virtual void endChangeStubborn(Pointer);
        virtual void free(Pointer);

        vector<Pointer> _objects;
        size_t _total;
    };

    GCMallocPoolAPI::~GCMallocPoolAPI()
    {
        for (size_t i = 0; i < _objects.size(); i++)
            free(_objects[i]);
        // cout << "freed " << _total << " bytes in " << _objects.size() << "
        // objects" << endl;
    }

    Pointer GCMallocPoolAPI::allocate(size_t s)
    {
        Pointer p = malloc(s);
        _objects.push_back(p);
        _total += s;
        return p;
    }

    Pointer GCMallocPoolAPI::allocateAtomic(size_t s) { return allocate(s); }

    Pointer GCMallocPoolAPI::allocateAtomicOffPage(size_t s)
    {
        return allocate(s);
    }

    Pointer GCMallocPoolAPI::allocateOffPage(size_t s) { return allocate(s); }

    Pointer GCMallocPoolAPI::allocateStubborn(size_t s) { return allocate(s); }

    void GCMallocPoolAPI::beginChangeStubborn(Pointer p) {}

    void GCMallocPoolAPI::endChangeStubborn(Pointer) {}

    void GCMallocPoolAPI::free(Pointer p) {}

    //----------------------------------------------------------------------

    class GCStaticPoolAPI : public GarbageCollector::API
    {
    public:
        GCStaticPoolAPI(API* next)
            : API(next)
            , _total(0)
        {
        }

        virtual ~GCStaticPoolAPI();
        virtual Pointer allocate(size_t);
        virtual Pointer allocateAtomic(size_t);
        virtual Pointer allocateAtomicOffPage(size_t);
        virtual Pointer allocateOffPage(size_t);
        virtual Pointer allocateStubborn(size_t);
        virtual void beginChangeStubborn(Pointer);
        virtual void endChangeStubborn(Pointer);
        virtual void free(Pointer);

        stl_ext::block_alloc_arena _arena;
        vector<Pointer> _objects;
        size_t _total;
    };

    GCStaticPoolAPI::~GCStaticPoolAPI() {}

    Pointer GCStaticPoolAPI::allocate(size_t s) { return _arena.allocate(s); }

    Pointer GCStaticPoolAPI::allocateAtomic(size_t s) { return allocate(s); }

    Pointer GCStaticPoolAPI::allocateAtomicOffPage(size_t s)
    {
        return allocate(s);
    }

    Pointer GCStaticPoolAPI::allocateOffPage(size_t s) { return allocate(s); }

    Pointer GCStaticPoolAPI::allocateStubborn(size_t s) { return allocate(s); }

    void GCStaticPoolAPI::beginChangeStubborn(Pointer p) {}

    void GCStaticPoolAPI::endChangeStubborn(Pointer) {}

    void GCStaticPoolAPI::free(Pointer p) {}

    //----------------------------------------------------------------------

    class GCStatAPI : public GarbageCollector::API
    {
    public:
        GCStatAPI(API* next)
            : API(next)
        {
        }

        virtual ~GCStatAPI();
        virtual Pointer allocate(size_t);
        virtual Pointer allocateAtomic(size_t);
        virtual Pointer allocateAtomicOffPage(size_t);
        virtual Pointer allocateOffPage(size_t);
        virtual Pointer allocateStubborn(size_t);
        virtual void beginChangeStubborn(Pointer);
        virtual void endChangeStubborn(Pointer);
        virtual void free(Pointer);

        typedef std::map<size_t, size_t> StatMap;
        StatMap _map;
    };

    GCStatAPI::~GCStatAPI()
    {
        cout << "---memstats---" << endl;

        size_t total = 0;

        for (StatMap::iterator i = _map.begin(); i != _map.end(); ++i)
        {
            cout << i->first << " -> " << i->second << endl;
            total += i->first * i->second;
        }

        cout << "total = " << total << endl;
    }

    Pointer GCStatAPI::allocate(size_t s)
    {
        _map[s]++;
        return next()->allocate(s);
    }

    Pointer GCStatAPI::allocateAtomic(size_t s)
    {
        _map[s]++;
        return next()->allocateAtomic(s);
    }

    Pointer GCStatAPI::allocateAtomicOffPage(size_t s)
    {
        _map[s]++;
        return next()->allocateAtomicOffPage(s);
    }

    Pointer GCStatAPI::allocateOffPage(size_t s)
    {
        _map[s]++;
        return next()->allocateOffPage(s);
    }

    Pointer GCStatAPI::allocateStubborn(size_t s)
    {
        _map[s]++;
        return next()->allocateStubborn(s);
    }

    void GCStatAPI::beginChangeStubborn(Pointer p)
    {
        next()->beginChangeStubborn(p);
    }

    void GCStatAPI::endChangeStubborn(Pointer p)
    {
        next()->endChangeStubborn(p);
    }

    void GCStatAPI::free(Pointer p) { next()->free(p); }

    //----------------------------------------------------------------------

    static void arenaCollector(block_alloc_arena*)
    {
        if (GarbageCollector::isEnabled())
        {
            GarbageCollector::Barrier barrier;
            GarbageCollector::collect();
        }
        else
        {
            GarbageCollector::markNeedsCollecting();
        }
    }

    void GarbageCollector::init()
    {
        static bool initialized = false;

#ifdef MU_USE_BOEHM_COLLECTOR
#endif

        if (!initialized)
        {
            //
            //  Make sure that this is set. VariantType requires that any GC
            //  can correctly identify interior pointers (pointers into
            //  existing allocated objects) not just pointers to the original
            //  object.
            //

            if (getenv("MU_GC_INCREMENTAL"))
                GC_enable_incremental();
            GC_all_interior_pointers = 1;
            GC_init();
            if (!_api)
                pushMainHeapAPI();

            initialized = true;
        }
    }

    void GarbageCollector::mark(const Process* process) {}

    void GarbageCollector::collect() { GC_gcollect(); }

    size_t GarbageCollector::collectionThreshold() { return 0; }

    void GarbageCollector::setCollectionThreshold(size_t s) {}

    size_t GarbageCollector::secondaryCollectionThreshold() { return 0; }

    void GarbageCollector::setSecondaryCollectionThreshold(size_t s) {}

    size_t GarbageCollector::mainArenaSize() { return 0; }

    size_t GarbageCollector::auxArenaSize() { return 0; }

#if defined(MU_USE_BASE_COLLECTOR) || defined(MU_USE_NO_COLLECTOR)
    void GarbageCollector::disable() { _disabled++; }

    void GarbageCollector::enable() { _disabled--; }

    bool GarbageCollector::needsCollection() { return _needsCollection; }

    void GarbageCollector::clearRoots() {}

    void GarbageCollector::addRoot(char* p, size_t s) {}

    void GarbageCollector::removeRoot(char* p, size_t s) {}
#endif

#if defined(MU_USE_BOEHM_COLLECTOR)
    void GarbageCollector::disable()
    {
        _disabled++;
        GC_disable();
    }

    void GarbageCollector::enable()
    {
        _disabled--;
        GC_enable();
    }

    bool GarbageCollector::needsCollection() { return _needsCollection; }

    void GarbageCollector::clearRoots() { GC_clear_roots(); }

    void GarbageCollector::addRoot(char* p, size_t s)
    {
        GC_add_roots(p, p + s);
    }
#if defined(_WIN32)
    // Note: removeRoot() is not available from the windows gc
    void GarbageCollector::removeRoot(char* p, size_t s) {}
#else
    void GarbageCollector::removeRoot(char* p, size_t s)
    {
        GC_remove_roots(p, p + s);
    }
#endif
#endif

    bool GarbageCollector::isEnabled() { return _disabled == 0; }

    void GarbageCollector::debugFinalizer(void* obj, void* data)
    {
        cout << "DEBUG: finalizer " << obj;
        if (data)
            cout << ", name = " << (char*)data;
        cout << endl;
    }

    void GarbageCollector::pushMainHeapAPI() { _api = new GCHeapAPI(_api); }

    void GarbageCollector::pushMainHeapNoOptAPI()
    {
        _api = new GCHeapNoOptAPI(_api);
    }

    void GarbageCollector::pushAutoreleaseAPI() { _api = new GCPoolAPI(_api); }

    void GarbageCollector::pushMallocAutoreleaseAPI()
    {
        _api = new GCMallocPoolAPI(_api);
    }

    void GarbageCollector::pushStaticAutoreleaseAPI()
    {
        _api = new GCStaticPoolAPI(_api);
    }

    void GarbageCollector::pushStatAPI() { _api = new GCStatAPI(_api); }

    void GarbageCollector::popAPI()
    {
        if (API* p = _api)
        {
            _api = p->next();
            delete p;
        }
    }

    bool GarbageCollector::isGCPointer(Pointer p) { return GC_base(p) != 0; }

    bool GarbageCollector::isGCPointerSymbol(Pointer p)
    {
        Pointer b = GC_base(p);
        if (b != p)
            return false;

        size_t size = GC_size(p);
        if (size < sizeof(Symbol))
            return false;

        Pointer* objp = (Pointer*)p;
        for (size_t i = 0; i < 5; i++)
            if (!isGCPointer(objp[i]))
                return false;

        return true;
    }

} // namespace Mu
