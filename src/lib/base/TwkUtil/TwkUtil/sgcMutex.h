//-
//*****************************************************************************
// Copyright (c) 2018 Autodesk, Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//*****************************************************************************
//+

#ifndef SGC_THREAD_SAFETY_ANALYSIS_MUTEX_H
#define SGC_THREAD_SAFETY_ANALYSIS_MUTEX_H

#include <mutex>

// Enable thread safety attributes only with clang.
// The attributes can be safely erased when compiling with other compilers.
//
#if defined(__clang__) && (!defined(SWIG))
#define THREAD_ANNOTATION_ATTRIBUTE(x) __attribute__((x))
#else
#define THREAD_ANNOTATION_ATTRIBUTE(x) // no-op
#endif

#define CAPABILITY(x) THREAD_ANNOTATION_ATTRIBUTE(capability(x))

#define SCOPED_CAPABILITY THREAD_ANNOTATION_ATTRIBUTE(scoped_lockable)

#define GUARDED_BY(x) THREAD_ANNOTATION_ATTRIBUTE(guarded_by(x))

#define PT_GUARDED_BY(x) THREAD_ANNOTATION_ATTRIBUTE(pt_guarded_by(x))

#define ACQUIRED_BEFORE(...) \
    THREAD_ANNOTATION_ATTRIBUTE(acquired_before(__VA_ARGS__))

#define ACQUIRED_AFTER(...) \
    THREAD_ANNOTATION_ATTRIBUTE(acquired_after(__VA_ARGS__))

#define REQUIRES(...) \
    THREAD_ANNOTATION_ATTRIBUTE(requires_capability(__VA_ARGS__))

#define REQUIRES_SHARED(...) \
    THREAD_ANNOTATION_ATTRIBUTE(requires_shared_capability(__VA_ARGS__))

#define ACQUIRE(...) \
    THREAD_ANNOTATION_ATTRIBUTE(acquire_capability(__VA_ARGS__))

#define ACQUIRE_SHARED(...) \
    THREAD_ANNOTATION_ATTRIBUTE(acquire_shared_capability(__VA_ARGS__))

#define RELEASE(...) \
    THREAD_ANNOTATION_ATTRIBUTE(release_capability(__VA_ARGS__))

#define RELEASE_SHARED(...) \
    THREAD_ANNOTATION_ATTRIBUTE(release_shared_capability(__VA_ARGS__))

#define TRY_ACQUIRE(...) \
    THREAD_ANNOTATION_ATTRIBUTE(try_acquire_capability(__VA_ARGS__))

#define TRY_ACQUIRE_SHARED(...) \
    THREAD_ANNOTATION_ATTRIBUTE(try_acquire_shared_capability(__VA_ARGS__))

#define EXCLUDES(...) THREAD_ANNOTATION_ATTRIBUTE(locks_excluded(__VA_ARGS__))

#define ASSERT_CAPABILITY(x) THREAD_ANNOTATION_ATTRIBUTE(assert_capability(x))

#define ASSERT_SHARED_CAPABILITY(x) \
    THREAD_ANNOTATION_ATTRIBUTE(assert_shared_capability(x))

#define RETURN_CAPABILITY(x) THREAD_ANNOTATION_ATTRIBUTE(lock_returned(x))

#define NO_THREAD_SAFETY_ANALYSIS \
    THREAD_ANNOTATION_ATTRIBUTE(no_thread_safety_analysis)

// Defines an annotated interface for mutexes.
// These methods can be implemented to use any internal mutex implementation.
class CAPABILITY("mutex") Mutex
{
    std::mutex m_mutex;

public:
    // Acquire/lock this mutex exclusively.  Only one thread can have exclusive
    // access at any one time.  Write operations to guarded data require an
    // exclusive lock.
    void lock() ACQUIRE() { m_mutex.lock(); }

    // Release/unlock an exclusive mutex.
    void unlock() RELEASE() { m_mutex.unlock(); }

    // Try to acquire the mutex.  Returns true on success, and false on failure.
    bool tryLock() TRY_ACQUIRE(true) { return m_mutex.try_lock(); }

    // For negative capabilities.
    const Mutex& operator!() const { return *this; }

    const std::mutex& native() const { return m_mutex; }

    std::mutex& native() { return m_mutex; }
};

// MutexLocker is an RAII class that acquires a mutex in its constructor, and
// releases it in its destructor.
class SCOPED_CAPABILITY MutexGuard
{
    std::unique_lock<std::mutex> m_lock;

public:
    MutexGuard(Mutex* mu) ACQUIRE(mu)
        : m_lock{mu->native()}
    {
    }

    ~MutexGuard() RELEASE() {}

    const std::unique_lock<std::mutex>& native() const { return m_lock; }

    std::unique_lock<std::mutex>& native() { return m_lock; }
};

#ifdef USE_LOCK_STYLE_THREAD_SAFETY_ATTRIBUTES
// The original version of thread safety analysis the following attribute
// definitions.  These use a lock-based terminology.  They are still in use
// by existing thread safety code, and will continue to be supported.

// Deprecated.
#define PT_GUARDED_VAR THREAD_ANNOTATION_ATTRIBUTE(pt_guarded_var)

// Deprecated.
#define GUARDED_VAR THREAD_ANNOTATION_ATTRIBUTE(guarded_var)

// Replaced by REQUIRES
#define EXCLUSIVE_LOCKS_REQUIRED(...) \
    THREAD_ANNOTATION_ATTRIBUTE(exclusive_locks_required(__VA_ARGS__))

// Replaced by REQUIRES_SHARED
#define SHARED_LOCKS_REQUIRED(...) \
    THREAD_ANNOTATION_ATTRIBUTE(shared_locks_required(__VA_ARGS__))

// Replaced by CAPABILITY
#define LOCKABLE THREAD_ANNOTATION_ATTRIBUTE(lockable)

// Replaced by SCOPED_CAPABILITY
#define SCOPED_LOCKABLE THREAD_ANNOTATION_ATTRIBUTE(scoped_lockable)

// Replaced by ACQUIRE
#define EXCLUSIVE_LOCK_FUNCTION(...) \
    THREAD_ANNOTATION_ATTRIBUTE(exclusive_lock_function(__VA_ARGS__))

// Replaced by ACQUIRE_SHARED
#define SHARED_LOCK_FUNCTION(...) \
    THREAD_ANNOTATION_ATTRIBUTE(shared_lock_function(__VA_ARGS__))

// Replaced by RELEASE and RELEASE_SHARED
#define UNLOCK_FUNCTION(...) \
    THREAD_ANNOTATION_ATTRIBUTE(unlock_function(__VA_ARGS__))

// Replaced by TRY_ACQUIRE
#define EXCLUSIVE_TRYLOCK_FUNCTION(...) \
    THREAD_ANNOTATION_ATTRIBUTE(exclusive_trylock_function(__VA_ARGS__))

// Replaced by TRY_ACQUIRE_SHARED
#define SHARED_TRYLOCK_FUNCTION(...) \
    THREAD_ANNOTATION_ATTRIBUTE(shared_trylock_function(__VA_ARGS__))

// Replaced by ASSERT_CAPABILITY
#define ASSERT_EXCLUSIVE_LOCK(...) \
    THREAD_ANNOTATION_ATTRIBUTE(assert_exclusive_lock(__VA_ARGS__))

// Replaced by ASSERT_SHARED_CAPABILITY
#define ASSERT_SHARED_LOCK(...) \
    THREAD_ANNOTATION_ATTRIBUTE(assert_shared_lock(__VA_ARGS__))

// Replaced by EXCLUDE_CAPABILITY.
#define LOCKS_EXCLUDED(...) \
    THREAD_ANNOTATION_ATTRIBUTE(locks_excluded(__VA_ARGS__))

// Replaced by RETURN_CAPABILITY
#define LOCK_RETURNED(x) THREAD_ANNOTATION_ATTRIBUTE(lock_returned(x))

#endif // USE_LOCK_STYLE_THREAD_SAFETY_ATTRIBUTES

#endif // THREAD_SAFETY_ANALYSIS_MUTEX_H
