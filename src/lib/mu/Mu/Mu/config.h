#ifndef __Mu__config__h__
#define __Mu__config__h__
//
// Copyright (c) 2009, Jim Hourihan
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <string>
#include <vector>
#include <map>
#include <set>
#include <list>
#include <stl_ext/barray.h>

//
//  Which garbage collector to use. One of these has to be defined.
//

#define MU_USE_BOEHM_COLLECTOR
// #define MU_USE_BASE_COLLECTOR
// #define MU_USE_NO_COLLECTOR

//
//  Determines whether Mu is compiled to use the Function union method
//  or value union method for node return values.
//

#define MU_FUNCTION_UNION

//
//  Determines if interpreted nodes that don't care about return value
//  should be safe about register allocation or should take chances
//  for speed's sake.

#define MU_SAFE_ARGUMENTS 1

//
//  Use setjmp/longjmp instead of c++ exceptions for flow
//  control. Note that for BSDish systems, _setjmp and _longjmp should
//  be used instead: these do not save the signal masks and therefor
//  avoid a system call.
//

#define MU_FLOW_SETJMP 1

#ifdef PLATFORM_DARWIN
#define SETJMP _setjmp
#define LONGJMP _longjmp
#else
#define SETJMP setjmp
#define LONGJMP longjmp
#endif

#ifdef _MSC_VER
//
//  Don't complain about:
//  - 4291 possible lack of free/delete in GC code
//  - 4181 const applied to reference type
//  - 4355 this used in constructor initializer list
//
#pragma warning(disable : 4291 4181 4355)
#pragma warning(once : 4996)

#if _MSC_VER > 1300
#define strtoll(a, b, c) _strtoi64(a, b, c)
#endif

#endif

//
//  Use of Altivec or SSE registers and alignment
//

#define MU_USE_ALTIVEC 0
#define MU_USE_SSE 0

//
//  Machine word size, number of significant bits in a memory pointer.
//

#define MU_WORD_SIZE 32
#define MU_NUM_0BITS 2
#define MU_NUM_SIGNIFICANT_BITS (MU_WORD_SIZE - MU_NUM_OBITS)
#define MU_TYPE_ID_BITS 14

//
// Use pcre library or posix regex
//
#if defined(_MSC_VER) || defined(PLATFORM_WINDOWS)
#define MU_USE_PCRE
#endif

//
//  Macros
//

#define MU_DELETED 0xdeadbeefLL

//
//  Maximum number of arguments to a function. This value is defined
//  in terms of the number of bits stored in a bit field.
//

#define MU_MAX_ARGUMENT_BITS 16

//
//  This should be removed from the Language::matchFunction code and
//  replaced by a dynamically allocated array or pvector.
//

#define MU_MAX_FUNCTIONS_CONSIDERED 30

//
//  Set up memory allocator
//

#ifdef MU_USE_BOEHM_COLLECTOR
// #define GC_PTHREADS 1
#define GC_THREADS 1
// Note: windows.h includes winsock.h by default (if WIN32_LEAN_AND_MEAN is not
// defined). Any time winsock.h gets included before winsock2.h, there will be
// compiler errors. The reason is because the two files DO NOT co-exist very
// well. winsock2.h was designed to replace winsock.h, not extend it. Everything
// that is defined in winsock.h is also defined in winsock2.h.
#ifdef _MSC_VER
#include <winsock2.h>
#endif
#include <gc/gc.h>
#include <gc/gc_allocator.h>
#endif

//----------------------------------------------------------------------
//
//  Types
//

namespace Mu
{

    //
    //  Memory allocator definition.
    //  Currently this is the old gc, the boehm gc, or no gc (leak).
    //
    //  Some NOTES on Boehm GC and Mu.
    //
    //  * The goal is to make the GC minimally conservative. This means
    //    identifying references as precisely as possible and preventing
    //    "black-listing" of the heap to prevent fragmentation.
    //
    //  * The gc_allocator has a traits struct which decides whether the
    //    allocated type should be atomic or not. So don't use an STL
    //    container if you're aliasing one type onto another. E.g., in
    //    DynamicArray we were using gc_allocator vector for "byte"
    //    type. That implied that the memory was *not* being scanned
    //    because gc_allocator.h declared unsigned char as being "pointer
    //    free". So don't use an STL container unless you explicitly tell
    //    it the type you're storing (e.g. STLVector<Symbol*>::Type
    //    instead of STLVector::<byte>::Type casing from something else).
    //
    //  * The gc has four different "basic" allocation functions:
    //
    //      1) GC_MALLOC gives back memory that is GC'd and traced
    //      2) GC_MALLOC_ATOMIC gives back GC'd memory that's *not* traced
    //      3) GC_MALLOC_IGNORE_OFF_PAGE memory is like GC_MALLOC but pointers
    //         to it must be pointing somewhere in the first 64 words
    //      4) GC_MALLOC_ATOMIC_IGNORE_OFF_PAGE is like #3 + #2
    //
    //  * The variant (union) type and ClassInstances with multiple
    //    inheritance are (currently) the only types in Mu that will have
    //    references to locations inside the object.
    //
    //    In the case of a variant type, its a wrapper around another
    //    object. So the first word of a variant is a pointer to the
    //    general VariantType and the next word is a pointer to the
    //    specific VariantTagType. Its not uncommon to have a pointer to
    //    the tag type part of the object instead of the general variant
    //    type. So we definitely *require* interior pointers but really
    //    only that one case. This could be handled by the
    //    GC_register_displacement() function which allows a small region
    //    near the head of the object to be considered as a pointer to the
    //    object (similar to ignore_off_page).
    //
    //    In the case of multiple inheritance, its possible to have a
    //    pointer to a base class down one of the lineages other than the
    //    primary one. In this case the pointer can be almost anywhere
    //    inside the object.
    //
    //    Because the Boehm GC is conservative these types of tuning
    //    refinements are very important. All of that being said: the GC
    //    is compiled with interior pointers on by default.
    //
    //  * There are a few spots where we try and figure out what "kind" of
    //    object we should allocate (atomic or not). This is helped along
    //    by the isGCAtomic() flag in Type. If a type is GC atomic it
    //    means that heap allocated objects of the type will *not* have
    //    references to other objects and can be allocated with one of the
    //    ATOMIC allocation functions. This is used by Class to determine
    //    if a whole class is atomic by check to see if it has any members
    //    that are MachineRep::Pointer. If so then the class is not atomic
    //    otherwise it is. This can eliminate a lot of temp small objects
    //    from being scanned.
    //
    //  * Its possible to do a few more things that we are not doing:
    //
    //      1) Use a custom mark routine which knows the structure of a
    //         type and will only consider real pointers
    //      2) Create bitmap type descriptors (GC_make_descriptor()) to
    //         let the GC handle small/medium sized objects itself.
    //
    //    Using these techniques its possible to narrow down the GC from
    //    being hugely conservative to being much more precise.
    //

    //----------------------------------------------------------------------

#define MU_GC_ALLOC(t) Mu::GarbageCollector::api()->allocate(t)
#define MU_GC_ALLOC_STUBBORN(t) Mu::GarbageCollector::api()->allocateStubborn(t)
#define MU_GC_ALLOC_ATOMIC(t) Mu::GarbageCollector::api()->allocateAtomic(t)
#define MU_GC_ALLOC_ATOMIC_IGNORE_OFF_PAGE(t) \
    Mu::GarbageCollector::api()->allocateAtomicOffPage(t)
#define MU_GC_ALLOC_IGNORE_OFF_PAGE(t) \
    Mu::GarbageCollector::api()->allocateOffPage(t)
#define MU_GC_FREE(t) Mu::GarbageCollector::api()->free(t)
#define MU_GC_END_CHANGE_STUBBORN(x) \
    Mu::GarbageCollector::api()->endChangeStubborn((void*)x)
#define MU_GC_BEGIN_CHANGE_STUBBORN(x) \
    Mu::GarbageCollector::api()->beginChangeStubborn((void*)x)

//----------------------------------------------------------------------
#ifdef MU_USE_BOEHM_COLLECTOR

#define MU_GCAPI_NEW_DELETE                                                \
    static void* operator new(size_t s, void* p) { return p; }             \
    static void* operator new(size_t s) { return MU_GC_ALLOC(s); }         \
    static void* operator new[](size_t size) { return MU_GC_ALLOC(size); } \
    static void operator delete(void* p, size_t s) {}                      \
    static void operator delete[](void* pnt) {}
#define MU_GCAPI_STUBBORN_NEW_DELETE                                        \
    static void* operator new(size_t s, void* p) { return p; }              \
    static void* operator new(size_t s) { return MU_GC_ALLOC_STUBBORN(s); } \
    static void* operator new[](size_t size)                                \
    {                                                                       \
        return MU_GC_ALLOC_STUBBORN(size);                                  \
    }                                                                       \
    static void operator delete(void* p, size_t s) {}                       \
    static void operator delete[](void* pnt) {}
#define MU_GCAPI_ATOMIC_NEW_DELETE                                        \
    static void* operator new(size_t s, void* p) { return p; }            \
    static void* operator new(size_t s) { return MU_GC_ALLOC_ATOMIC(s); } \
    static void* operator new[](size_t size)                              \
    {                                                                     \
        return MU_GC_ALLOC_ATOMIC(size);                                  \
    }                                                                     \
    static void operator delete(void* p, size_t s) {}                     \
    static void operator delete[](void* pnt) {}
#define MU_GC_NEW_DELETE                                                 \
    static void* operator new(size_t s, void* p) { return p; }           \
    static void* operator new(size_t s) { return GC_MALLOC(s); }         \
    static void* operator new[](size_t size) { return GC_MALLOC(size); } \
    static void operator delete(void* p, size_t s) {}                    \
    static void operator delete[](void* pnt) {}
#define MU_GC_STUBBORN_NEW_DELETE                                         \
    static void* operator new(size_t s, void* p) { return p; }            \
    static void* operator new(size_t s) { return GC_MALLOC_STUBBORN(s); } \
    static void* operator new[](size_t size)                              \
    {                                                                     \
        return GC_MALLOC_STUBBORN(size);                                  \
    }                                                                     \
    static void operator delete(void* p, size_t s) {}                     \
    static void operator delete[](void* pnt) {}
#define MU_GC_ATOMIC_NEW_DELETE                                         \
    static void* operator new(size_t s, void* p) { return p; }          \
    static void* operator new(size_t s) { return GC_MALLOC_ATOMIC(s); } \
    static void* operator new[](size_t size)                            \
    {                                                                   \
        return GC_MALLOC_ATOMIC(size);                                  \
    }                                                                   \
    static void operator delete(void* p, size_t s) {}                   \
    static void operator delete[](void* pnt) {}
#define MU_STL_ALLOCATOR gc_allocator
#define MU_STL_MUAPI_ALLOCATOR MuGCAPI_allocator
#endif

//----------------------------------------------------------------------
#ifdef MU_USE_NO_COLLECTOR
#define MU_STL_ALLOCATOR std::allocator
#define MU_STL_MUAPI_ALLOCATOR std::allocator
#endif

    //----------------------------------------------------------------------
    //
    //  Template Typedefs
    //
    //  This exists to avoid using macros. In order to use one of the stl
    //  containers and have the garbage collector scan the contents for
    //  root pointers, you need to use a special allocator. This is
    //  selected by the macro MU_STL_ALLOCATOR from above. These typedefs
    //  then indicate the actual std container class via that mechanism.
    //
    //  NOTE: no need for pair<> or any other STL class which lacks an
    //  allocator. The std versions can be used instead.
    //
    //  SEE ALSO: GarbageCollector.h which has another set of these in
    //  namespace APIAllocatable. Those use the GarbageCollector::API
    //  instead of calling the GC directly. That makes it possible to
    //  route their allocation to alternate pools. E.g. you'd probably
    //  want to use those if you're making a temp container on the
    //  stack. That way all of their memory can be freed at once.
    //

    template <typename T> struct STLVector
    {
        typedef MU_STL_ALLOCATOR<T> Allocator;
        typedef std::vector<T, Allocator> Type;
    };

    template <typename T> struct STLList
    {
        typedef MU_STL_ALLOCATOR<T> Allocator;
        typedef std::list<T, Allocator> Type;
    };

    template <typename T> struct STLSet
    {
        typedef MU_STL_ALLOCATOR<T> Allocator;
        typedef std::less<T> Compare;
        typedef std::set<T, Compare, Allocator> Type;
    };

    template <typename K, typename V> struct STLMap
    {
        typedef std::pair<const K, V> Pair;
        typedef MU_STL_ALLOCATOR<Pair> Allocator;
        typedef std::less<K> Compare;
        typedef std::map<K, V, Compare, Allocator> Type;
    };

    template <typename C> struct STLString
    {
        typedef std::char_traits<C> Traits;
        typedef MU_STL_ALLOCATOR<C> Allocator;
        typedef std::basic_string<C, Traits, Allocator> Type;
    };

    template <typename T> struct STLEXTBarray
    {
        typedef MU_STL_ALLOCATOR<T*> Allocator;
        typedef stl_ext::barray<T, 8, Allocator> Type;
    };

    //----------------------------------------------------------------------
    //
    //  String definition
    //

    //
    //  Mu uses UTF32 for characters and UTF-8 for strings. The use of
    //  wstring is problematic since it will be UTF16 on windows and UTF32
    //  elsewhere. Also, UTF-8 seems to be easiest for UNIX geeks at this
    //  point in time.
    //

    typedef STLString<char>::Type UTF8String;
    typedef std::string stdUTF8String;

#ifdef _MSC_VER
    typedef STLString<wchar_t>::Type UTF16String;
    typedef std::basic_string<wchar_t> stdUTF16String;
#else
    typedef STLString<unsigned short>::Type UTF16String;
    typedef std::basic_string<unsigned short> stdUTF16String;
#endif

#ifdef _MSC_VER
    typedef unsigned int UTF32Char;
    typedef STLString<UTF32Char>::Type UTF32String;
    typedef std::basic_string<UTF32Char> stdUTF32String;
#else
    typedef wchar_t UTF32Char;
    typedef STLString<wchar_t>::Type UTF32String;
    typedef std::basic_string<wchar_t> stdUTF32String;
#endif

    //
    //  Storage choice for symbol names and strings.
    //

    typedef UTF8String String;
    typedef UTF8String SymbolStorage;

    //
    //  Other types
    //

    typedef void* Pointer;
    typedef unsigned char* Structure;
    typedef short int16;
    typedef long int int32;
    typedef unsigned char byte;

#if defined _MSC_VER
    typedef __int64 int64;
    typedef unsigned __int64 uint64;
#else
    typedef signed long long int int64;
    typedef unsigned long long int uint64;
#endif

} // namespace Mu

//----------------------------------------------------------------------
//
//      DLL export
//

#ifdef WIN32
#define MU_EXPORT_DYNAMIC __declspec(dllexport)
#else
#define MU_EXPORT_DYNAMIC
#endif

#include <Mu/GarbageCollector.h>

#endif // __Mu__config__h__
