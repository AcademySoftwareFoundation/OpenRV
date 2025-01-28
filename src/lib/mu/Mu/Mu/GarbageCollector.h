//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __Mu__GarbageCollector__h__
#define __Mu__GarbageCollector__h__
#include <Mu/config.h>
#include <sys/types.h>
#include <vector>

namespace Mu
{
    class Process;

    //
    //  class GarbageCollector
    //
    //  This class is either the implementation of the garbage collector
    //  (by default) or a wrapper around another implemenation (boehm).
    //

    class GarbageCollector
    {
    public:
        typedef void (*CollectCallback)();
        typedef std::vector<CollectCallback> CollectCallbackFuncs;

        class API
        {
        public:
            API(API* next);
            virtual Pointer allocate(size_t) = 0;
            virtual Pointer allocateAtomic(size_t) = 0;
            virtual Pointer allocateAtomicOffPage(size_t) = 0;
            virtual Pointer allocateOffPage(size_t) = 0;
            virtual Pointer allocateStubborn(size_t) = 0;
            virtual void beginChangeStubborn(Pointer) = 0;
            virtual void endChangeStubborn(Pointer) = 0;
            virtual void free(Pointer) = 0;

            API* next() const { return _next; }

        protected:
            virtual ~API();

        private:
            API* _next;
            friend class GarbageCollector;
        };

        static void init();
        static void collect();

        static API* api() { return _api; }

        //
        //  Add/Remove roots
        //

        static void clearRoots();
        static void addRoot(char*, size_t);
        static void removeRoot(char*, size_t);

        //
        //  If you create a Barrier struct on the stack, it will disable
        //  the garbage collector until it goes out of scope. In the event
        //  of a thrown exception the collector will be turned back on.
        //

        static void disable();
        static void enable();
        static bool isEnabled();
        static bool needsCollection();

        struct Barrier
        {
            Barrier() { GarbageCollector::disable(); }

            ~Barrier()
            {
                GarbageCollector::enable();
                if (GarbageCollector::needsCollection())
                    GarbageCollector::collect();
            }
        };

        static void debugFinalizer(void* obj, void* data);

        //
        //  Main API
        //

        template <typename T> static T* allocate(size_t s)
        {
            return reinterpret_cast<T*>(_api->allocate(s));
        }

        template <typename T> static T* allocateAtomic(size_t s)
        {
            return reinterpret_cast<T*>(_api->allocateAtomic(s));
        }

        template <typename T> static T* allocateAtomicOffPage(size_t s)
        {
            return reinterpret_cast<T*>(_api->allocateAtomicOffPage(s));
        }

        template <typename T> static T* allocateOffPage(size_t s)
        {
            return reinterpret_cast<T*>(_api->allocateOffPage(s));
        }

        template <typename T> static T* allocateStubborn(size_t s)
        {
            return reinterpret_cast<T*>(_api->allocateStubborn(s));
        }

        template <typename T> static void beginChangeStubborn(T* p)
        {
            return _api->beginChangeStubborn((void*)(p));
        }

        template <typename T> static void endChangeStubborn(T* p)
        {
            return _api->endChangeStubborn((void*)(p));
        }

        template <typename T> static void free(T* p)
        {
            return _api->free(reinterpret_cast<void*>(p));
        }

        static void pushMainHeapAPI();
        static void pushMainHeapNoOptAPI();
        static void pushAutoreleaseAPI();
        static void pushMallocAutoreleaseAPI();
        static void pushStaticAutoreleaseAPI();
        static void pushStatAPI();
        static void popAPI();

        //
        //  Query Objects
        //

        static bool isGCPointer(Pointer);
        static bool isGCPointerSymbol(Pointer);

        //
        //  Old API
        //

        static bool mark() { return _mark; }

        static size_t collectionThreshold();
        static void setCollectionThreshold(size_t);
        static size_t secondaryCollectionThreshold();
        static void setSecondaryCollectionThreshold(size_t);
        static size_t mainArenaSize();
        static size_t auxArenaSize();

        static void addCollectCallback(CollectCallback cb)
        {
            _collectCallbacks.push_back(cb);
        }

        static size_t objectsScanned() { return _objectsScanned; }

        static size_t objectsReclaimed() { return _objectsReclaimed; }

        static void markNeedsCollecting() { _needsCollection = true; }

    private:
        static void mark(const Process*);

    private:
        static API* _api;
        static bool _mark;
        static int _disabled;
        static bool _needsCollection;
        static size_t _objectsScanned;
        static size_t _objectsReclaimed;
        static CollectCallbackFuncs _collectCallbacks;
    };

    //
    //  A C++ allocator suitable for use with STL that calls the
    //  GarbageCollector API instead of calling the GC directly.
    //

    // In the following GC_Tp is GC_true_type iff we are allocating a
    // pointerfree object.
    template <class GC_Tp>
    inline void* MU_GC_selective_alloc(size_t n, GC_Tp, bool ignore_off_page)
    {
        return ignore_off_page ? MU_GC_ALLOC_IGNORE_OFF_PAGE(n)
                               : MU_GC_ALLOC(n);
    }

    template <>
    inline void* MU_GC_selective_alloc<GC_true_type>(size_t n, GC_true_type,
                                                     bool ignore_off_page)
    {
        return ignore_off_page ? MU_GC_ALLOC_ATOMIC_IGNORE_OFF_PAGE(n)
                               : MU_GC_ALLOC_ATOMIC(n);
    }

    template <class GC_Tp> class MuGCAPI_allocator
    {
    public:
        typedef size_t size_type;
        typedef ptrdiff_t difference_type;
        typedef GC_Tp* pointer;
        typedef const GC_Tp* const_pointer;
        typedef GC_Tp& reference;
        typedef const GC_Tp& const_reference;
        typedef GC_Tp value_type;

        template <class GC_Tp1> struct rebind
        {
            typedef MuGCAPI_allocator<GC_Tp1> other;
        };

        MuGCAPI_allocator() {}

        MuGCAPI_allocator(const MuGCAPI_allocator&) throw() {}
#if !(GC_NO_MEMBER_TEMPLATES || 0 < _MSC_VER && _MSC_VER <= 1200)
        // MSVC++ 6.0 do not support member templates
        template <class GC_Tp1>
        MuGCAPI_allocator(const MuGCAPI_allocator<GC_Tp1>&) throw()
        {
        }
#endif
        ~MuGCAPI_allocator() throw() {}

        pointer address(reference GC_x) const { return &GC_x; }
#ifndef _MSC_VER
        const_pointer address(const_reference GC_x) const { return &GC_x; }
#endif

        // GC_n is permitted to be 0.  The C++ standard says nothing about what
        // the return value is when GC_n == 0.
        GC_Tp* allocate(size_type GC_n, const void* = 0)
        {
            GC_type_traits<GC_Tp> traits;
            return static_cast<GC_Tp*>(MU_GC_selective_alloc(
                GC_n * sizeof(GC_Tp), traits.GC_is_ptr_free, false));
        }

        // __p is not permitted to be a null pointer.
        void deallocate(pointer __p, size_type /* GC_n */) { MU_GC_FREE(__p); }

        size_type max_size() const throw()
        {
            return size_t(-1) / sizeof(GC_Tp);
        }

        void construct(pointer __p, const GC_Tp& __val)
        {
            new (__p) GC_Tp(__val);
        }

        void destroy(pointer __p) { __p->~GC_Tp(); }
    };

    template <> class MuGCAPI_allocator<void>
    {
        typedef size_t size_type;
        typedef ptrdiff_t difference_type;
        typedef void* pointer;
        typedef const void* const_pointer;
        typedef void value_type;

        template <class GC_Tp1> struct rebind
        {
            typedef MuGCAPI_allocator<GC_Tp1> other;
        };
    };

    template <class GC_T1, class GC_T2>
    inline bool operator==(const MuGCAPI_allocator<GC_T1>&,
                           const MuGCAPI_allocator<GC_T2>&)
    {
        return true;
    }

    template <class GC_T1, class GC_T2>
    inline bool operator!=(const MuGCAPI_allocator<GC_T1>&,
                           const MuGCAPI_allocator<GC_T2>&)
    {
        return false;
    }

    namespace APIAllocatable
    {
        template <typename T> struct STLVector
        {
            typedef MU_STL_MUAPI_ALLOCATOR<T> Allocator;
            typedef std::vector<T, Allocator> Type;
        };

        template <typename T> struct STLList
        {
            typedef MU_STL_MUAPI_ALLOCATOR<T> Allocator;
            typedef std::list<T, Allocator> Type;
        };

        template <typename T> struct STLSet
        {
            typedef MU_STL_MUAPI_ALLOCATOR<T> Allocator;
            typedef std::less<T> Compare;
            typedef std::set<T, Compare, Allocator> Type;
        };

        template <typename K, typename V> struct STLMap
        {
            typedef std::pair<const K, V> Pair;
            typedef MU_STL_MUAPI_ALLOCATOR<Pair> Allocator;
            typedef std::less<K> Compare;
            typedef std::map<K, V, Compare, Allocator> Type;
        };

        template <typename C> struct STLString
        {
            typedef std::char_traits<C> Traits;
            typedef MU_STL_MUAPI_ALLOCATOR<C> Allocator;
            typedef std::basic_string<C, Traits, Allocator> Type;
        };

        template <typename T> struct STLEXTBarray
        {
            typedef MU_STL_MUAPI_ALLOCATOR<T*> Allocator;
            typedef stl_ext::barray<T, 8, Allocator> Type;
        };

        typedef STLString<char>::Type UTF8String;

#ifdef _MSC_VER
        typedef STLString<wchar_t>::Type UTF16String;
#else
        typedef STLString<unsigned short>::Type UTF16String;
#endif

#ifdef _MSC_VER
        typedef unsigned int UTF32Char;
#else
        typedef wchar_t UTF32Char;
#endif

        typedef STLString<wchar_t>::Type UTF32String;

        //
        //  Storage choice for symbol names and strings.
        //

        typedef UTF8String String;
        typedef UTF8String SymbolStorage;
    } // namespace APIAllocatable

} // namespace Mu

#endif // __Mu__GarbageCollector__h__
