/*This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>
*/
// clang-format off
#ifndef HOP_H_
#define HOP_H_

// You can disable completly HOP by setting this variable
// to false
#if !defined( HOP_ENABLED )

// Stubbing all profiling macros so they are disabled
// when HOP_ENABLED is false
#define HOP_PROF( x )
#define HOP_PROF_FUNC()
#define HOP_PROF_SPLIT( x )
#define HOP_PROF_DYN_NAME( x )
#define HOP_PROF_MUTEX_LOCK( x )
#define HOP_PROF_MUTEX_UNLOCK( x )
#define HOP_ZONE( x )
#define HOP_SET_THREAD_NAME( x )

#else  // We do want to profile

///////////////////////////////////////////////////////////////
/////       THESE ARE THE MACROS YOU CAN MODIFY     ///////////
///////////////////////////////////////////////////////////////

// Total maximum of thread being traced
#if !defined( HOP_MAX_THREAD_NB )
#define HOP_MAX_THREAD_NB 64
#endif

// Total size of the shared memory ring buffer. This does not
// include the meta-data size
#define HOP_SHARED_MEM_SIZE 32000000

// Minimum cycles for a lock to be considered in the profiled data
#define HOP_MIN_LOCK_CYCLES 1000

// These are the zone that can be used. You can change the name
// but you must not change the values.
enum { HOP_MAX_ZONE_COLORS = 16 };
enum HopZoneColor
{
   HOP_ZONE_COLOR_NONE = 0xFFFF,
   HOP_ZONE_COLOR_1   = 1 << 0,
   HOP_ZONE_COLOR_2   = 1 << 1,
   HOP_ZONE_COLOR_3   = 1 << 2,
   HOP_ZONE_COLOR_4   = 1 << 3,
   HOP_ZONE_COLOR_5   = 1 << 4,
   HOP_ZONE_COLOR_6   = 1 << 5,
   HOP_ZONE_COLOR_7   = 1 << 6,
   HOP_ZONE_COLOR_8   = 1 << 7,
   HOP_ZONE_COLOR_9   = 1 << 8,
   HOP_ZONE_COLOR_10   = 1 << 9,
   HOP_ZONE_COLOR_11   = 1 << 10,
   HOP_ZONE_COLOR_12   = 1 << 11,
   HOP_ZONE_COLOR_13   = 1 << 12,
   HOP_ZONE_COLOR_14   = 1 << 13,
   HOP_ZONE_COLOR_15   = 1 << 14,
   HOP_ZONE_COLOR_16   = 1 << 15,
};

///////////////////////////////////////////////////////////////
/////       THESE ARE THE MACROS YOU SHOULD USE     ///////////
///////////////////////////////////////////////////////////////

// Create a new profiling trace with specified name. Name must be static
#define HOP_PROF( x ) HOP_PROF_GUARD_VAR( __LINE__, ( __FILE__, __LINE__, (x) ) )

// Create a new profiling trace with the compiler provided name
#define HOP_PROF_FUNC() HOP_PROF_ID_GUARD( hop__, ( __FILE__, __LINE__, HOP_FCT_NAME ) )

// Split a profiling trace with a new provided name. Name must be static.
#define HOP_PROF_SPLIT( x ) HOP_PROF_ID_SPLIT( hop__, ( __FILE__, __LINE__, (x) ) )

// Create a new profiling trace for dynamic strings. Please use sparingly as they will incur more slowdown
#define HOP_PROF_DYN_NAME( x ) HOP_PROF_DYN_STRING_GUARD_VAR( __LINE__, ( __FILE__, __LINE__, (x) ) )

// Create a trace that represent the time waiting for a mutex. You need to provide
// a pointer to the mutex that is being locked
#define HOP_PROF_MUTEX_LOCK( x ) HOP_MUTEX_LOCK_GUARD_VAR( __LINE__,( x ) )

// Create an event that correspond to the unlock of the specified mutex. This is
// used to provide stall region. You should provide a pointer to the mutex that
// is being unlocked.
#define HOP_PROF_MUTEX_UNLOCK( x ) HOP_MUTEX_UNLOCK_EVENT( x )

#define HOP_ZONE( x ) HOP_ZONE_GUARD( __LINE__, ( x ) )

// Set the name of the current thread in the profiler. Only the first call will
// be considered for each thread.
#define HOP_SET_THREAD_NAME( x ) hop::ClientManager::SetThreadName( (x) )

///////////////////////////////////////////////////////////////
/////     EVERYTHING AFTER THIS IS IMPL DETAILS        ////////
///////////////////////////////////////////////////////////////

/*
                              /NN\
                              :NN:
                           ..-+NN+-..
                      ./mmmNNNNNNNNNmmmm\.
                   .mmNNNNNNNNNNNNNNNNNNNmm.
                 .nNNNNNNNNNNNNNNNNNNNNNNNNNNn.
               .nmNNNNNNNNNNNNNNNNNNNNNNNNNNNNmn.
              .nNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNn.
              .oydmmdhs+:`+NNNNNNNNNNh.-+shdmmdy+.
                 .////shmd.-dNNNNNNN+.dmhs+/:/.
                `dNdmdNNdmo `+mdNhs` ommdNNNdmd`
                sNmdmmdmds`ss--sh--hs`ommNNNNNdo
                dNmNNNmd:.yNNmy--ymmNd.:hNNNmmmm
                dNNNds:-/`yNNNNmmNNNNh`/-:sdNNNd
                -:-./ohmms`omNNNNNNmo`smmho/.-:-
                   .mNNNNmh-:hNNNNh:-hmNNNNm.
                   `mmmmmmmd `+dd+` dmmmmmmm`
                    smmmmmd--d+--+d--dmmmmms
                    `hmmms-/dmmddmmd/-smmmh`
                     `o+. ommmmmmmmmmo .+o`
                          .ymmmmmmmmy.
                           `smmmmmms`
                             \dmmd/
                              -yy-
                               ``
                       | || |/ _ \| _ \
                       | __ | (_) |  _/
                       |_||_|\___/|_|
*/
#include <stdint.h>
#include <cmath> // for abs()

// Useful macros
#define HOP_VERSION 0.61f
#define HOP_CONSTEXPR constexpr
#define HOP_NOEXCEPT noexcept
#define HOP_STATIC_ASSERT static_assert

/* Windows specific macros and defines */
#if defined(_MSC_VER)
#ifndef NOMINMAX
#define NOMINMAX
#endif

#if defined(HOP_IMPLEMENTATION)
#define HOP_API __declspec(dllexport)
#else
#define HOP_API
#endif

#include <tchar.h>
#include <intrin.h> // __rdtscp
typedef void* sem_handle; // HANDLE is a void*
typedef void* shm_handle; // HANDLE is a void*
typedef TCHAR HOP_CHAR;

// Type defined in unistd.h
#ifdef _WIN64
#define ssize_t __int64
#else
#define ssize_t long
#endif // _WIN64

#else /* Unix (Linux & MacOs) specific macros and defines */

#include <semaphore.h>
typedef sem_t* sem_handle;
typedef int shm_handle;
typedef char HOP_CHAR;

#define HOP_API

#endif

// -----------------------------
// Forward declarations of type used by ringbuffer as adapted from
// Mindaugas Rasiukevicius. See below for Copyright/Disclaimer
typedef struct ringbuf ringbuf_t;
typedef struct ringbuf_worker ringbuf_worker_t;
// -----------------------------

namespace hop
{

// Custom trace types
using TimeStamp    = uint64_t;
using TimeDuration = int64_t;
using StrPtr_t     = uint64_t;
using LineNb_t     = uint32_t;
using Core_t       = uint32_t;
using ZoneId_t     = uint16_t;
using Depth_t      = uint16_t;

inline TimeStamp rdtscp( uint32_t& aux )
{
#if defined(_MSC_VER)
   return __rdtscp( &aux );
#else
   uint64_t rax, rdx;
   asm volatile( "rdtscp\n" : "=a"( rax ), "=d"( rdx ), "=c"( aux ) : : );
   return ( rdx << 32 ) + rax;
#endif
}

inline TimeStamp getTimeStamp( Core_t& core )
{
   // We return the tsc with the first bit set to 0. We do not require this last cycle
   // of precision. It will instead be used to flag if a trace uses dynamic strings or not in its
   // start time. See hop::StartProfileDynString
   return rdtscp( core ) & ~1ull;;
}

inline TimeStamp getTimeStamp()
{
   uint32_t dummyCore;
   return getTimeStamp( dummyCore );
}

enum class MsgType : uint32_t
{
   PROFILER_TRACE,
   PROFILER_STRING_DATA,
   PROFILER_WAIT_LOCK,
   PROFILER_UNLOCK_EVENT,
   PROFILER_HEARTBEAT,
   PROFILER_CORE_EVENT,
   INVALID_MESSAGE,
};

struct TracesMsgInfo
{
   uint32_t count;
};

struct StringDataMsgInfo
{
   uint32_t size;
};

struct LockWaitsMsgInfo
{
   uint32_t count;
};

struct UnlockEventsMsgInfo
{
   uint32_t count;
};

struct CoreEventMsgInfo
{
   uint32_t count;
};

HOP_CONSTEXPR uint32_t EXPECTED_MSG_INFO_SIZE = 40;
struct MsgInfo
{
   MsgType type;
   // Thread id from which the msg was sent
   uint32_t threadIndex;
   uint64_t threadId;
   TimeStamp timeStamp;
   StrPtr_t threadName;
   // Specific message data
   union {
      TracesMsgInfo traces;
      StringDataMsgInfo stringData;
      LockWaitsMsgInfo lockwaits;
      UnlockEventsMsgInfo unlockEvents;
      CoreEventMsgInfo coreEvents;
   };
};
HOP_STATIC_ASSERT( sizeof(MsgInfo) == EXPECTED_MSG_INFO_SIZE, "MsgInfo layout has changed unexpectedly" );

HOP_CONSTEXPR uint32_t EXPECTED_TRACE_SIZE = 40;
struct Trace
{
   TimeStamp start, end;   // Timestamp for start/end of this trace
   StrPtr_t fileNameId;   // Index into string array for the file name
   StrPtr_t fctNameId;    // Index into string array for the function name
   LineNb_t lineNumber;   // Line at which the trace was inserted
   ZoneId_t zone;         // Zone to which this trace belongs
   Depth_t depth;         // The depth in the callstack of this trace
};
HOP_STATIC_ASSERT( sizeof(Trace) == EXPECTED_TRACE_SIZE, "Trace layout has changed unexpectedly" );

HOP_CONSTEXPR uint32_t EXPECTED_LOCK_WAIT_SIZE = 32;
struct LockWait
{
   void* mutexAddress;
   TimeStamp start, end;
   Depth_t depth;
   uint16_t padding;
};
HOP_STATIC_ASSERT( sizeof(LockWait) == EXPECTED_LOCK_WAIT_SIZE, "Lock wait layout has changed unexpectedly" );

HOP_CONSTEXPR uint32_t EXPECTED_UNLOCK_EVENT_SIZE = 16;
struct UnlockEvent
{
   void* mutexAddress;
   TimeStamp time;
};
HOP_STATIC_ASSERT( sizeof(UnlockEvent) == EXPECTED_UNLOCK_EVENT_SIZE, "Unlock Event layout has changed unexpectedly" );

struct CoreEvent
{
   TimeStamp start, end;
   Core_t core;
};

class Client;
class SharedMemory;

class HOP_API ClientManager
{
  public:
   static Client* Get();
   static ZoneId_t StartProfile();
   static StrPtr_t StartProfileDynString( const char*, ZoneId_t* );
   static void EndProfile(
       StrPtr_t fileName,
       StrPtr_t fctName,
       TimeStamp start,
       TimeStamp end,
       LineNb_t lineNb,
       ZoneId_t zone,
       Core_t core );
   static void EndLockWait(
      void* mutexAddr,
      TimeStamp start,
      TimeStamp end );
   static void UnlockEvent( void* mutexAddr, TimeStamp time );
   static void SetThreadName( const char* name ) HOP_NOEXCEPT;
   static ZoneId_t PushNewZone( ZoneId_t newZone );
   static bool HasConnectedConsumer() HOP_NOEXCEPT;
   static bool HasListeningConsumer() HOP_NOEXCEPT;

   static SharedMemory& sharedMemory() HOP_NOEXCEPT;
};

class ProfGuard
{
  public:
    ProfGuard( const char* fileName, LineNb_t lineNb, const char* fctName ) HOP_NOEXCEPT
    {
      open( fileName, lineNb, fctName );
    }
    ~ProfGuard()
    {
      close();
    }
    inline void reset( const char* fileName, LineNb_t lineNb, const char* fctName )
    {
      // Please uncomment the following line if close() is made public!
      // if ( _fctName )
      close();
      open( fileName, lineNb, fctName );
    }

  private:
    inline void open( const char* fileName, LineNb_t lineNb, const char* fctName )
    {
      _start = getTimeStamp();
      _fileName = reinterpret_cast<StrPtr_t>(fileName);
      _fctName = reinterpret_cast<StrPtr_t>(fctName);
      _lineNb = lineNb;
      _zone = ClientManager::StartProfile();
    }
    inline void close()
    {
      uint32_t core;
      const auto end = getTimeStamp( core );
      ClientManager::EndProfile( _fileName, _fctName, _start, end, _lineNb, _zone, core );
      // Please uncomment the following line if close() is made public!
      // _fctName = nullptr;
    }

    TimeStamp _start;
    StrPtr_t _fileName, _fctName;
    LineNb_t _lineNb;
    ZoneId_t _zone;
};

class LockWaitGuard
{
  public:
   LockWaitGuard( void* mutAddr )
       : start( getTimeStamp() ),
         mutexAddr( mutAddr )
   {
   }
   ~LockWaitGuard()
   {
      ClientManager::EndLockWait( mutexAddr, start, getTimeStamp() );
   }

   TimeStamp start;
   void* mutexAddr;
};

class ProfGuardDynamicString
{
  public:
   ProfGuardDynamicString( const char* fileName, LineNb_t lineNb, const char* fctName ) HOP_NOEXCEPT
       : _start( getTimeStamp() | 1 ), // Set the first bit to 1 to flag the use of dynamic strings
         _fileName( reinterpret_cast<StrPtr_t>(fileName) ),
         _lineNb( lineNb )
   {
      _fctName = ClientManager::StartProfileDynString( fctName, &_zone );
   }
   ~ProfGuardDynamicString()
   {
      ClientManager::EndProfile( _fileName, _fctName, _start, getTimeStamp(), _lineNb, _zone, 0 );
   }

  private:
   TimeStamp _start;
   StrPtr_t _fileName;
   StrPtr_t _fctName;
   LineNb_t _lineNb;
   ZoneId_t _zone;
};

class ZoneGuard
{
 public:
   ZoneGuard( HopZoneColor newZone ) HOP_NOEXCEPT
   {
      _prevZoneId = ClientManager::PushNewZone( static_cast<ZoneId_t>(newZone) );
   }
   ~ZoneGuard()
   {
      ClientManager::PushNewZone( _prevZoneId );
   }

  private:
   ZoneId_t _prevZoneId;
};

#define HOP_PROF_GUARD_VAR( LINE, ARGS ) \
   hop::ProfGuard HOP_COMBINE( hopProfGuard, LINE ) ARGS
#define HOP_PROF_ID_GUARD( ID, ARGS ) \
   hop::ProfGuard ID ARGS
#define HOP_PROF_ID_SPLIT( ID, ARGS ) \
   ID.reset ARGS
#define HOP_PROF_DYN_STRING_GUARD_VAR( LINE, ARGS ) \
   hop::ProfGuardDynamicString HOP_COMBINE( hopProfGuard, LINE ) ARGS
#define HOP_MUTEX_LOCK_GUARD_VAR( LINE, ARGS ) \
   hop::LockWaitGuard HOP_COMBINE( hopMutexLock, LINE ) ARGS
#define HOP_MUTEX_UNLOCK_EVENT( x ) \
   hop::ClientManager::UnlockEvent( x, hop::getTimeStamp() );
#define HOP_ZONE_GUARD( LINE, ARGS ) \
   hop::ZoneGuard HOP_COMBINE( hopZoneGuard, LINE ) ARGS

#define HOP_COMBINE( X, Y ) X##Y
#if defined(_MSC_VER)
#define HOP_FCT_NAME __FUNCTION__
#else
#define HOP_FCT_NAME __PRETTY_FUNCTION__
#endif

}  // namespace hop


/*
 * Copyright (c) 2016 Mindaugas Rasiukevicius <rmind at noxt eu>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
*/
int ringbuf_setup( ringbuf_t*, unsigned, size_t );
void ringbuf_get_sizes( unsigned, size_t*, size_t* );

ringbuf_worker_t* ringbuf_register( ringbuf_t*, unsigned );
void ringbuf_unregister( ringbuf_t*, ringbuf_worker_t* );

ssize_t ringbuf_acquire( ringbuf_t*, ringbuf_worker_t*, size_t );
void ringbuf_produce( ringbuf_t*, ringbuf_worker_t* );
size_t ringbuf_consume( ringbuf_t*, size_t* );
void ringbuf_release( ringbuf_t*, size_t );

/* ======================================================================
                    End of public declarations
   ==================================================================== */

#if defined(HOP_VIEWER) || defined(HOP_IMPLEMENTATION)
#include <atomic>
#include <mutex>

// On MacOs the max name length seems to be 30...
#define HOP_SHARED_MEM_MAX_NAME_SIZE 30
namespace hop
{
class SharedMemory
{
  public:
   enum ConnectionState
   {
      NOT_CONNECTED,
      CONNECTED,
      CONNECTED_NO_CLIENT,
      PERMISSION_DENIED,
      UNKNOWN_CONNECTION_ERROR
   };

   ConnectionState create( const HOP_CHAR* path, size_t size, bool isConsumer );
   void destroy();

   struct SharedMetaInfo
   {
      enum Flags
      {
         CONNECTED_PRODUCER = 1 << 0,
         CONNECTED_CONSUMER = 1 << 1,
         LISTENING_CONSUMER = 1 << 2,
      };
      std::atomic< uint32_t > flags{0};
      float clientVersion{0.0f};
      uint32_t maxThreadNb{0};
      size_t requestedSize{0};
      std::atomic< TimeStamp > lastResetTimeStamp{0};
   };

   bool hasConnectedProducer() const HOP_NOEXCEPT;
   void setConnectedProducer( bool ) HOP_NOEXCEPT;
   bool hasConnectedConsumer() const HOP_NOEXCEPT;
   void setConnectedConsumer( bool ) HOP_NOEXCEPT;
   bool hasListeningConsumer() const HOP_NOEXCEPT;
   void setListeningConsumer( bool ) HOP_NOEXCEPT;
   TimeStamp lastResetTimestamp() const HOP_NOEXCEPT;
   void setResetTimestamp( TimeStamp t ) HOP_NOEXCEPT;
   ringbuf_t* ringbuffer() const HOP_NOEXCEPT;
   uint8_t* data() const HOP_NOEXCEPT;
   bool valid() const HOP_NOEXCEPT;
   sem_handle semaphore() const HOP_NOEXCEPT;
   bool tryWaitSemaphore() const HOP_NOEXCEPT;
   void signalSemaphore() const HOP_NOEXCEPT;
   const SharedMetaInfo* sharedMetaInfo() const HOP_NOEXCEPT;
   ~SharedMemory();

  private:
   // Pointer into the shared memory
   SharedMetaInfo* _sharedMetaData{NULL};
   ringbuf_t* _ringbuf{NULL};
   uint8_t* _data{NULL};
   // ----------------
   sem_handle _semaphore{NULL};
   bool _isConsumer;
   shm_handle _sharedMemHandle{};
   HOP_CHAR _sharedMemPath[HOP_SHARED_MEM_MAX_NAME_SIZE];
   HOP_CHAR _sharedSemPath[HOP_SHARED_MEM_MAX_NAME_SIZE+5];
   std::atomic< bool > _valid{false};
   std::mutex _creationMutex;
};
} // namespace hop
#endif // defined(HOP_VIEWER)

/* ======================================================================
                    End of private declarations
   ==================================================================== */

#if defined(HOP_IMPLEMENTATION)

// standard includes
#include <algorithm>
#include <cassert>
#include <memory>
#include <unordered_set>
#include <vector>

#define HOP_MIN(a,b) (((a)<(b))?(a):(b))
#define HOP_MAX(a,b) (((a)>(b))?(a):(b))
#define HOP_UNUSED(x) (void)(x)

#if !defined( _MSC_VER )

// Unix shared memory includes
#include <fcntl.h> // O_CREAT
#include <cstring> // memcpy
#include <pthread.h> // pthread_self
#include <sys/mman.h> // shm_open
#include <sys/stat.h> // stat
#include <unistd.h> // ftruncate

const HOP_CHAR HOP_SHARED_MEM_PREFIX[] = "/hop_";
const HOP_CHAR HOP_SHARED_SEM_SUFFIX[] = "_sem";
#define HOP_STRLEN( str ) strlen( (str) )
#define HOP_STRNCPYW( dst, src, count ) strncpy( (dst), (src), (count) )
#define HOP_STRNCATW( dst, src, count ) strncat( (dst), (src), (count) )
#define HOP_STRNCPY( dst, src, count ) strncpy( (dst), (src), (count) )
#define HOP_STRNCAT( dst, src, count ) strncat( (dst), (src), (count) )

#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)

#define HOP_GET_THREAD_ID() reinterpret_cast<size_t>(pthread_self())
#define HOP_SLEEP_MS( x ) usleep( x * 1000 )

extern HOP_CHAR* __progname;
inline const HOP_CHAR* HOP_GET_PROG_NAME() HOP_NOEXCEPT
{
   return __progname;
}

#else // !defined( _MSC_VER )

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

const HOP_CHAR HOP_SHARED_MEM_PREFIX[] = _T("/hop_");
const HOP_CHAR HOP_SHARED_SEM_SUFFIX[] = _T("_sem");
#define HOP_STRLEN( str ) _tcslen( (str) )
#define HOP_STRNCPYW( dst, src, count ) _tcsncpy_s( (dst), (count), (src), (count) )
#define HOP_STRNCATW( dst, src, count ) _tcsncat_s( (dst), (src), (count) )
#define HOP_STRNCPY( dst, src, count ) strncpy_s( (dst), (count), (src), (count) )
#define HOP_STRNCAT( dst, src, count ) strncat_s( (dst), (count), (src), (count) )

#define likely(x)   (x)
#define unlikely(x) (x)

#define HOP_GET_THREAD_ID() (size_t)GetCurrentThreadId()
#define HOP_SLEEP_MS( x ) Sleep( x )

inline const HOP_CHAR* HOP_GET_PROG_NAME() HOP_NOEXCEPT
{
   static HOP_CHAR fullname[MAX_PATH];
   static HOP_CHAR* shortname = []() {
      DWORD size = GetModuleFileName( NULL, fullname, MAX_PATH );
      while ( size > 0 && fullname[size] != '\\' ) --size;
      return &fullname[size + 1];
   }();
   return shortname;
}

#endif // !defined( _MSC_VER )

namespace
{
    hop::SharedMemory::ConnectionState errorToConnectionState( uint32_t err )
    {
#if defined( _MSC_VER )
       if( err == ERROR_FILE_NOT_FOUND ) return hop::SharedMemory::NOT_CONNECTED;
       if( err == ERROR_ACCESS_DENIED ) return hop::SharedMemory::PERMISSION_DENIED;
       return hop::SharedMemory::UNKNOWN_CONNECTION_ERROR;
#else
       if( err == ENOENT ) return hop::SharedMemory::NOT_CONNECTED;
       if( err == EACCES ) return hop::SharedMemory::PERMISSION_DENIED;
       return hop::SharedMemory::UNKNOWN_CONNECTION_ERROR;
#endif
    }

    sem_handle openSemaphore( const HOP_CHAR* name, hop::SharedMemory::ConnectionState* state )
    {
       sem_handle sem = NULL;
#if defined( _MSC_VER )
       sem = CreateSemaphore( NULL, 0, LONG_MAX, name );
       if ( !sem )
       {
          *state = errorToConnectionState( GetLastError() );
       }
#else
       sem = sem_open( name, O_CREAT, S_IRUSR | S_IWUSR, 1 );
       if( !sem && state ) {
          *state = errorToConnectionState( errno );
       }
#endif
       return sem;
    }

    void closeSemaphore( sem_handle sem, const HOP_CHAR* semName )
    {
#if defined( _MSC_VER )
       CloseHandle( sem );
#else
       if ( sem_close( sem ) != 0 )
       {
          perror( "HOP - Could not close semaphore" );
       }
       if ( sem_unlink( semName ) < 0 )
       {
          perror( "HOP - Could not unlink semaphore" );
       }
#endif
   }

   void* createSharedMemory(const HOP_CHAR* path, uint64_t size, shm_handle* handle, hop::SharedMemory::ConnectionState* state )
   {
       uint8_t* sharedMem = NULL;
#if defined ( _MSC_VER )
       *handle = CreateFileMapping(
           INVALID_HANDLE_VALUE,    // use paging file
           NULL,                    // default security
           PAGE_READWRITE,          // read/write access
           size >> 32,              // maximum object size (high-order DWORD)
           size & 0xFFFFFFFF,       // maximum object size (low-order DWORD)
           path);                   // name of mapping object

       if (*handle == NULL)
       {
           *state = errorToConnectionState( GetLastError() );
           return NULL;
       }
       sharedMem = (uint8_t*)MapViewOfFile(
           *handle,
           FILE_MAP_ALL_ACCESS, // read/write permission
           0,
           0,
           size);

       if (sharedMem == NULL)
       {
           *state = errorToConnectionState( GetLastError() );
           CloseHandle(*handle);
           return NULL;
       }
#else
       *handle = shm_open(path, O_CREAT | O_RDWR, 0666);
       if (*handle < 0)
       {
           *state = errorToConnectionState( errno );
           return NULL;
       }

       int truncRes = ftruncate(*handle, size);
       if( truncRes != 0 )
       {
          *state = errorToConnectionState( errno );
           return NULL;
       }

       sharedMem = reinterpret_cast<uint8_t*>(mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, *handle, 0));
#endif
       if( sharedMem ) *state = hop::SharedMemory::CONNECTED;
       return sharedMem;
   }

   void* openSharedMemory( const HOP_CHAR* path, shm_handle* handle, uint64_t* totalSize, hop::SharedMemory::ConnectionState* state )
   {
       uint8_t* sharedMem = NULL;
#if defined ( _MSC_VER )
       *handle = OpenFileMapping(
           FILE_MAP_ALL_ACCESS,   // read/write access
           FALSE,                 // do not inherit the name
           path);               // name of mapping object

       if (*handle == NULL)
       {
           *state = errorToConnectionState( GetLastError() );
           return NULL;
       }

       sharedMem = (uint8_t*)MapViewOfFile(
           *handle,
           FILE_MAP_ALL_ACCESS, // read/write permission
           0,
           0,
           0);

       if (sharedMem == NULL)
       {
           *state = errorToConnectionState( GetLastError() );
           CloseHandle(*handle);
           return NULL;
       }

       MEMORY_BASIC_INFORMATION memInfo;
       if (!VirtualQuery(sharedMem, &memInfo, sizeof(memInfo)))
       {
          *state = errorToConnectionState( GetLastError() );
          UnmapViewOfFile(sharedMem);
          CloseHandle(*handle);
          return NULL;
       }
       *totalSize = memInfo.RegionSize;
#else
      *handle = shm_open( path, O_RDWR, 0666 );
      if ( *handle < 0 )
      {
         *state = errorToConnectionState( errno );
         return NULL;
      }

      struct stat fileStat;
      if(fstat(*handle,&fileStat) < 0)
      {
         *state = errorToConnectionState( errno );
         return NULL;
      }

      *totalSize = fileStat.st_size;

      sharedMem = reinterpret_cast<uint8_t*>(mmap( NULL, fileStat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, *handle, 0 ));
      *state = sharedMem ? hop::SharedMemory::CONNECTED : hop::SharedMemory::UNKNOWN_CONNECTION_ERROR;
#endif
      return sharedMem;
   }

   void closeSharedMemory( const HOP_CHAR* name, shm_handle handle, void* dataPtr )
   {
#if defined( _MSC_VER )
      UnmapViewOfFile( dataPtr );
      CloseHandle( handle );
#else
      HOP_UNUSED(handle);  // Remove unuesed warning
      HOP_UNUSED(dataPtr);
      if ( shm_unlink( name ) != 0 ) perror( " HOP - Could not unlink shared memory" );
#endif
   }
}

namespace hop
{
// The call stack depth of the current measured trace. One variable per thread
static thread_local int tl_traceLevel = 0;
static thread_local uint32_t tl_threadIndex = 0; // Index of the tread as they are coming in
static thread_local uint64_t tl_threadId = 0;    // ID of the thread as seen by the OS
static thread_local ZoneId_t tl_zoneId = HOP_ZONE_COLOR_NONE;
static thread_local char tl_threadNameBuffer[64];
static thread_local StrPtr_t tl_threadName = 0;

static std::atomic<bool> g_done{false}; // Was the shared memory destroyed? (Are we done?)

SharedMemory::ConnectionState SharedMemory::create( const HOP_CHAR* exeName, size_t requestedSize, bool isConsumer )
{
   ConnectionState state = CONNECTED;
   std::lock_guard<std::mutex> g( _creationMutex );

   // Create the shared data if it was not already created
   if ( !_sharedMetaData )
   {
      _isConsumer = isConsumer;

      // Create shared mem name
     HOP_STRNCPYW( _sharedMemPath, HOP_SHARED_MEM_PREFIX, HOP_STRLEN( HOP_SHARED_MEM_PREFIX ) + 1 );
     HOP_STRNCATW(
          _sharedMemPath,
          exeName,
          HOP_SHARED_MEM_MAX_NAME_SIZE - HOP_STRLEN( HOP_SHARED_MEM_PREFIX ) - 1 );

     HOP_STRNCPYW( _sharedSemPath, _sharedMemPath, HOP_SHARED_MEM_MAX_NAME_SIZE );
     HOP_STRNCATW( _sharedSemPath, HOP_SHARED_SEM_SUFFIX, HOP_SHARED_MEM_MAX_NAME_SIZE - HOP_STRLEN( _sharedSemPath ) -1 );

      // Open semaphore
      _semaphore = openSemaphore( _sharedSemPath, &state );
      if ( _semaphore == NULL )
      {
         return state;
      }

      // Try to open shared memory
      uint64_t totalSize = 0;
      uint8_t* sharedMem =
          reinterpret_cast<uint8_t*>(openSharedMemory( _sharedMemPath, &_sharedMemHandle, &totalSize, &state ));

      // If we are the producer and we were not able to open the shared memory, we create it
      if ( !isConsumer && !sharedMem )
      {
         size_t ringBufSize;
         ringbuf_get_sizes( HOP_MAX_THREAD_NB, &ringBufSize, NULL );
         totalSize = ringBufSize + requestedSize + sizeof( SharedMetaInfo );
         sharedMem = reinterpret_cast<uint8_t*>(createSharedMemory( _sharedMemPath, totalSize, &_sharedMemHandle, &state ));
         if( sharedMem ) new(sharedMem) SharedMetaInfo; // Placement new for initializing values
      }

      if ( !sharedMem )
      {
         closeSemaphore( _semaphore, _sharedSemPath );
         return state;
      }

      SharedMetaInfo* metaInfo = reinterpret_cast<SharedMetaInfo*>(sharedMem);

      // Only the first producer setups the shared memory
      if( !isConsumer )
      {
         // Set client's info in the shared memory for the viewer to access
         metaInfo->clientVersion = HOP_VERSION;
         metaInfo->maxThreadNb = HOP_MAX_THREAD_NB;
         metaInfo->requestedSize = HOP_SHARED_MEM_SIZE;
         metaInfo->lastResetTimeStamp = getTimeStamp();

         // Take a local copy as we do not want to expose the ring buffer before it is
         // actually initialized
         ringbuf_t* localRingBuf = reinterpret_cast<ringbuf_t*>( sharedMem + sizeof( SharedMetaInfo ) );

         // Then setup the ring buffer
         memset( localRingBuf, 0, totalSize - sizeof( SharedMetaInfo ) );
         if ( ringbuf_setup( localRingBuf, HOP_MAX_THREAD_NB, requestedSize ) < 0 )
         {
            assert( false && "Ring buffer creation failed" );
            closeSharedMemory( _sharedMemPath, _sharedMemHandle, sharedMem );
            closeSemaphore( _semaphore, _sharedSemPath);
            return UNKNOWN_CONNECTION_ERROR;
         }
      }
      else // Check if client has compatible version
      {
         if ( std::abs( metaInfo->clientVersion - HOP_VERSION ) > 0.001f )
         {
            printf(
                "HOP - Client's version (%f) does not match HOP viewer version (%f)\n",
                static_cast<double>(metaInfo->clientVersion),
                static_cast<double>(HOP_VERSION) );
            destroy();
            exit(0);
         }
      }

      // Get the size needed for the ringbuf struct
      size_t ringBufSize;
      ringbuf_get_sizes( metaInfo->maxThreadNb, &ringBufSize, NULL );

      // Get pointers inside the shared memory once it has been initialized
      _sharedMetaData = reinterpret_cast<SharedMetaInfo*>(sharedMem);
      _ringbuf = reinterpret_cast<ringbuf_t*>( sharedMem + sizeof( SharedMetaInfo ) );
      _data = sharedMem + sizeof( SharedMetaInfo ) + ringBufSize;
      _valid = true;

      if ( isConsumer )
      {
         setResetTimestamp( getTimeStamp() );
         // We can only have one consumer
         if( hasConnectedConsumer() )
         {
            printf(
                "/!\\ HOP WARNING /!\\ \n"
                "Cannot have more than one instance of the consumer at a time."
                " You might be trying to run the consumer application twice or"
                " have a dangling shared memory segment. hop might be unstable"
                " in this state. You could consider manually removing the shared"
                " memory, or restart this excutable cleanly.\n\n" );
            // Force resetting the listening state as this could cause crash. The side
            // effect would simply be that other consumer would stop listening. Not a
            // big deal as there should not be any other consumer...
            _sharedMetaData->flags &= ~( SharedMetaInfo::LISTENING_CONSUMER );
         }
      }

      isConsumer ? setConnectedConsumer( true ) : setConnectedProducer( true );
   }

   return state;
}

bool SharedMemory::hasConnectedProducer() const HOP_NOEXCEPT
{
   return (sharedMetaInfo()->flags & SharedMetaInfo::CONNECTED_PRODUCER) > 0;
}

void SharedMemory::setConnectedProducer( bool connected ) HOP_NOEXCEPT
{
   if( connected )
      _sharedMetaData->flags |= SharedMetaInfo::CONNECTED_PRODUCER;
   else
      _sharedMetaData->flags &= ~SharedMetaInfo::CONNECTED_PRODUCER;
}

bool SharedMemory::hasConnectedConsumer() const HOP_NOEXCEPT
{
   return (sharedMetaInfo()->flags & SharedMetaInfo::CONNECTED_CONSUMER) > 0;
}

void SharedMemory::setConnectedConsumer( bool connected ) HOP_NOEXCEPT
{
   if( connected )
      _sharedMetaData->flags |= SharedMetaInfo::CONNECTED_CONSUMER;
   else
      _sharedMetaData->flags &= ~SharedMetaInfo::CONNECTED_CONSUMER;
}

bool SharedMemory::hasListeningConsumer() const HOP_NOEXCEPT
{
   const uint32_t mask = SharedMetaInfo::CONNECTED_CONSUMER | SharedMetaInfo::LISTENING_CONSUMER;
   return (sharedMetaInfo()->flags.load() & mask) == mask;
}

void SharedMemory::setListeningConsumer( bool listening ) HOP_NOEXCEPT
{
   if(listening)
      _sharedMetaData->flags |= SharedMetaInfo::LISTENING_CONSUMER;
   else
      _sharedMetaData->flags &= ~(SharedMetaInfo::LISTENING_CONSUMER);
}

TimeStamp SharedMemory::lastResetTimestamp() const HOP_NOEXCEPT
{
   return _sharedMetaData->lastResetTimeStamp.load();
}

void SharedMemory::setResetTimestamp(TimeStamp t) HOP_NOEXCEPT
{
   _sharedMetaData->lastResetTimeStamp.store( t );
}

uint8_t* SharedMemory::data() const HOP_NOEXCEPT
{
   return _data;
}

bool SharedMemory::valid() const HOP_NOEXCEPT
{
   return _valid;
}

ringbuf_t* SharedMemory::ringbuffer() const HOP_NOEXCEPT
{
   return _ringbuf;
}

sem_handle SharedMemory::semaphore() const HOP_NOEXCEPT
{
   return _semaphore;
}

bool SharedMemory::tryWaitSemaphore() const HOP_NOEXCEPT
{
#if defined(_MSC_VER)
    return WaitForSingleObject( _semaphore, 0 ) == WAIT_OBJECT_0;
#else
    return sem_trywait( _semaphore ) == 0;
#endif
}

void SharedMemory::signalSemaphore() const HOP_NOEXCEPT
{
#if defined(_MSC_VER)
        ReleaseSemaphore(_semaphore, 1, NULL);
#else
        sem_post( _semaphore );
#endif
}

const SharedMemory::SharedMetaInfo* SharedMemory::sharedMetaInfo() const HOP_NOEXCEPT
{
   return _sharedMetaData;
}

void SharedMemory::destroy()
{
   if ( valid() )
   {
      if( _isConsumer )
      {
         setListeningConsumer( false );
         setConnectedConsumer( false );
      }
      else
      {
         setConnectedProducer( false );
      }

      // If we are the last one accessing the shared memory, clean it.
      if ( ( _sharedMetaData->flags.load() &
             ( SharedMetaInfo::CONNECTED_PRODUCER | SharedMetaInfo::CONNECTED_CONSUMER ) ) == 0 )
      {
         printf("HOP - Cleaning up shared memory...\n");
         closeSemaphore( _semaphore, _sharedSemPath);
         closeSharedMemory( _sharedMemPath, _sharedMemHandle, _sharedMetaData );
         _sharedMetaData->~SharedMetaInfo();
      }

      _data = NULL;
      _ringbuf = NULL;
      _semaphore = NULL;
      _valid = false;
      g_done.store(true);
   }
}

SharedMemory::~SharedMemory()
{
   destroy();
}

// C-style string hash inspired by Stackoverflow question
// based on the Java string hash fct. If its good enough
// for java, it should be good enough for me...
static StrPtr_t cStringHash( const char* str, size_t strLen )
{
   StrPtr_t result = 0;
   HOP_CONSTEXPR StrPtr_t prime = 31;
   for ( size_t i = 0; i < strLen; ++i )
   {
      result = str[i] + ( result * prime );
   }
   return result;
}

static uint32_t alignOn( uint32_t val, uint32_t alignment )
{
   return (( val + alignment-1) & ~(alignment-1));
}

class Client
{
  public:
   Client()
   {
      _traces.reserve( 256 );
      _cores.reserve( 64 );
      _lockWaits.reserve( 64 );
      _unlockEvents.reserve( 64 );
      _stringPtr.reserve( 256 );
      _stringData.reserve( 256 * 32 );

      resetStringData();
   }

   void addProfilingTrace(
       StrPtr_t fileName,
       StrPtr_t fctName,
       TimeStamp start,
       TimeStamp end,
       LineNb_t lineNb,
       ZoneId_t zone )
   {
      _traces.push_back( Trace{ start, end, fileName, fctName, lineNb, zone, static_cast<Depth_t>(tl_traceLevel) } );
   }

   void addCoreEvent( Core_t core, TimeStamp startTime, TimeStamp endTime )
   {
      _cores.emplace_back( CoreEvent{ startTime, endTime, core } );
   }

   void addWaitLockTrace( void* mutexAddr, TimeStamp start, TimeStamp end, Depth_t depth )
   {
      _lockWaits.push_back( LockWait{ mutexAddr, start, end, depth, 0 /*padding*/ } );
   }

   void addUnlockEvent( void* mutexAddr, TimeStamp time )
   {
      _unlockEvents.push_back( UnlockEvent{ mutexAddr, time } );
   }

   void setThreadName( StrPtr_t name )
   {
      if( !tl_threadName )
      {
         HOP_STRNCPY( &tl_threadNameBuffer[0], reinterpret_cast<const char*>(name), sizeof( tl_threadNameBuffer ) );
         tl_threadNameBuffer[ sizeof(tl_threadNameBuffer) - 1 ] = '\0';
         tl_threadName = addDynamicStringToDb( tl_threadNameBuffer );
      }
   }

   StrPtr_t addDynamicStringToDb( const char* dynStr )
   {
      // Should not have null as dyn string, but just in case...
      if ( dynStr == NULL ) return 0;

      const size_t strLen = strlen( dynStr );

      const StrPtr_t hash = cStringHash( dynStr, strLen );

      auto res = _stringPtr.insert( hash );
      // If the string was inserted (meaning it was not already there),
      // add it to the database, otherwise return its hash
      if ( res.second )
      {
         const size_t newEntryPos = _stringData.size();
         assert( (newEntryPos & 7) == 0 ); // Make sure we are 8 byte aligned
         const size_t alignedStrLen = alignOn( static_cast<uint32_t>(strLen) + 1, 8 );

         _stringData.resize( newEntryPos + sizeof( StrPtr_t ) + alignedStrLen );
         StrPtr_t* strIdPtr = reinterpret_cast<StrPtr_t*>(&_stringData[newEntryPos]);
         *strIdPtr = hash;
         HOP_STRNCPY( &_stringData[newEntryPos + sizeof( StrPtr_t )], dynStr, alignedStrLen );
      }

      return hash;
   }

   bool addStringToDb( StrPtr_t strId )
   {
      // Early return on NULL. The db should always contains NULL as first
      // entry
      if( strId == 0 ) return false;

      auto res = _stringPtr.insert( strId );
      // If the string was inserted (meaning it was not already there),
      // add it to the database, otherwise do nothing
      if( res.second )
      {
         const size_t newEntryPos = _stringData.size();
         assert( (newEntryPos & 7) == 0 ); // Make sure we are 8 byte aligned

         const size_t alignedStrLen = alignOn( static_cast<uint32_t>(strlen( reinterpret_cast<const char*>(strId) )) + 1, 8 );

         _stringData.resize( newEntryPos + sizeof( StrPtr_t ) + alignedStrLen );
         StrPtr_t* strIdPtr = reinterpret_cast<StrPtr_t*>(&_stringData[newEntryPos]);
         *strIdPtr = strId;
         HOP_STRNCPY( &_stringData[newEntryPos + sizeof( StrPtr_t ) ], reinterpret_cast<const char*>(strId), alignedStrLen );
      }

      return res.second;
   }

   void resetStringData()
   {
      _stringPtr.clear();
      _stringData.clear();
      _sentStringDataSize = 0;
      _clientResetTimeStamp = ClientManager::sharedMemory().lastResetTimestamp();

      // Push back first name as empty string
      _stringPtr.insert( 0 );
      _stringData.insert( _stringData.begin(), sizeof(StrPtr_t), '\0' );
      // Push back thread name
      const auto hash = addDynamicStringToDb( tl_threadNameBuffer );
      HOP_UNUSED(hash);
      assert( hash == tl_threadName );
   }

   void resetPendingTraces()
   {
      _traces.clear();
      _cores.clear();
      _lockWaits.clear();
      _unlockEvents.clear();
   }

   TimeStamp getMsgTimeStamp() const
   {
      if( _traces.empty() )
      {
         return getTimeStamp();
      }
      else
      {
         return _traces.back().start;
      }
   }

   uint8_t* acquireSharedChunk( ringbuf_t* ringbuf, size_t size )
   {
      uint8_t* data = NULL;
      const bool msgWayToBig = size > HOP_SHARED_MEM_SIZE;
      if( !msgWayToBig )
      {
         const size_t paddedSize = alignOn( static_cast<uint32_t>(size), 8 );
         const ssize_t offset = ringbuf_acquire( ringbuf, _worker, paddedSize );
         if( offset != -1 )
         {
            data = &ClientManager::sharedMemory().data()[offset];
         }
      }

      return data;
   }

   bool sendStringData()
   {
      // Add all strings to the database
      for( const auto& t : _traces  )
      {
         addStringToDb( t.fileNameId );

         // String that were added dynamically are already in the
         // database and are flaged with the first bit of their start
         // time being 1. Therefore we only need to add the
         // non-dynamic strings. (first bit of start time being 0)
         if( (t.start & 1) == 0 )
            addStringToDb( t.fctNameId );
      }

      const uint32_t stringDataSize = static_cast<uint32_t>(_stringData.size());
      assert( stringDataSize >= _sentStringDataSize );
      const uint32_t stringToSendSize = stringDataSize - _sentStringDataSize;
      const size_t msgSize = sizeof( MsgInfo ) + stringToSendSize;

      ringbuf_t* ringbuf = ClientManager::sharedMemory().ringbuffer();
      uint8_t* bufferPtr = acquireSharedChunk( ringbuf, msgSize );

      if ( !bufferPtr )
      {
         printf("HOP - String to send are bigger than shared memory size. Consider"
                " increasing shared memory size \n");
         return false;
      }

      // Fill the buffer with the string data
      {
         // The data layout is as follow:
         // =========================================================
         // msgInfo     = Profiler specific infos  - Information about the message sent
         // stringData  = String Data              - Array with all strings referenced by the traces
         MsgInfo* msgInfo = reinterpret_cast<MsgInfo*>(bufferPtr);
         char* stringData = reinterpret_cast<char*>( bufferPtr + sizeof( MsgInfo ) );

         msgInfo->type = MsgType::PROFILER_STRING_DATA;
         msgInfo->threadId = tl_threadId;
         msgInfo->threadName = tl_threadName;
         msgInfo->threadIndex = tl_threadIndex;
         msgInfo->timeStamp = getMsgTimeStamp();
         msgInfo->stringData.size = stringToSendSize;

         // Copy string data into its array
         const auto itFrom = _stringData.begin() + _sentStringDataSize;
         std::copy( itFrom, itFrom + stringToSendSize, stringData );
      }

      ringbuf_produce( ringbuf, _worker );
      ClientManager::sharedMemory().signalSemaphore();

      // Update sent array size
      _sentStringDataSize = stringDataSize;

      return true;
   }

   bool sendTraces()
   {
      // Get size of profiling traces message
      const size_t profilerMsgSize = sizeof( MsgInfo ) + sizeof( Trace ) * _traces.size();

      ringbuf_t* ringbuf = ClientManager::sharedMemory().ringbuffer();
      uint8_t* bufferPtr = acquireSharedChunk( ringbuf, profilerMsgSize );
      if ( !bufferPtr )
      {
         printf("HOP - Failed to acquire enough shared memory. Consider increasing"
              "shared memory size if you see this message more than once\n");
         _traces.clear();
         return false;
      }

      // Fill the buffer with the profiling trace message
      {
         // The data layout is as follow:
         // =========================================================
         // msgInfo     = Profiler specific infos  - Information about the message sent
         // traceToSend = Traces                   - Array containing all of the traces
         MsgInfo* tracesInfo = reinterpret_cast<MsgInfo*>(bufferPtr);
         Trace* traceToSend = reinterpret_cast<Trace*>( bufferPtr + sizeof( MsgInfo ) );

         tracesInfo->type = MsgType::PROFILER_TRACE;
         tracesInfo->threadId = tl_threadId;
         tracesInfo->threadName = tl_threadName;
         tracesInfo->threadIndex = tl_threadIndex;
         tracesInfo->timeStamp = getMsgTimeStamp();
         tracesInfo->traces.count = static_cast<uint32_t>(_traces.size());

         // Copy trace information into buffer to send
         std::copy( _traces.begin(), _traces.end(), traceToSend );
      }

      ringbuf_produce( ringbuf, _worker );
      ClientManager::sharedMemory().signalSemaphore();

      // Free the buffers
      _traces.clear();

      return true;
   }

   bool sendCores()
   {
      if( _cores.empty() ) return false;

      const size_t coreMsgSize = sizeof( MsgInfo ) + _cores.size() * sizeof( CoreEvent );

      ringbuf_t* ringbuf = ClientManager::sharedMemory().ringbuffer();
      uint8_t* bufferPtr = acquireSharedChunk( ringbuf, coreMsgSize );
      if ( !bufferPtr )
      {
         printf("HOP - Failed to acquire enough shared memory. Consider increasing shared memory size\n");
         _cores.clear();
         return false;
      }

      // Fill the buffer with the core event message
      {
         MsgInfo* coreInfo = reinterpret_cast<MsgInfo*>(bufferPtr);
         coreInfo->type = MsgType::PROFILER_CORE_EVENT;
         coreInfo->threadId = tl_threadId;
         coreInfo->threadName = tl_threadName;
         coreInfo->threadIndex = tl_threadIndex;
         coreInfo->timeStamp = getMsgTimeStamp();
         coreInfo->coreEvents.count = static_cast<uint32_t>(_cores.size());
         bufferPtr += sizeof( MsgInfo );
         memcpy( bufferPtr, _cores.data(), _cores.size() * sizeof( CoreEvent ) );
      }

      ringbuf_produce( ringbuf, _worker );
      ClientManager::sharedMemory().signalSemaphore();

      const auto lastEntry = _cores.back();
      _cores.clear();
      _cores.emplace_back( lastEntry );

      return true;
   }

   bool sendLockWaits()
   {
      if( _lockWaits.empty() ) return false;

      const size_t lockMsgSize = sizeof( MsgInfo ) + _lockWaits.size() * sizeof( LockWait );

      ringbuf_t* ringbuf = ClientManager::sharedMemory().ringbuffer();
      uint8_t* bufferPtr = acquireSharedChunk( ringbuf, lockMsgSize );
      if ( !bufferPtr )
      {
         printf("HOP - Failed to acquire enough shared memory. Consider increasing shared memory size\n");
         _lockWaits.clear();
         return false;
      }

      // Fill the buffer with the lock message
      {
         MsgInfo* lwInfo = reinterpret_cast<MsgInfo*>(bufferPtr);
         lwInfo->type = MsgType::PROFILER_WAIT_LOCK;
         lwInfo->threadId = tl_threadId;
         lwInfo->threadName = tl_threadName;
         lwInfo->threadIndex = tl_threadIndex;
         lwInfo->timeStamp = _lockWaits.back().start;
         lwInfo->lockwaits.count = static_cast<uint32_t>(_lockWaits.size());
         bufferPtr += sizeof( MsgInfo );
         memcpy( bufferPtr, _lockWaits.data(), _lockWaits.size() * sizeof( LockWait ) );
      }

      ringbuf_produce( ringbuf, _worker );
      ClientManager::sharedMemory().signalSemaphore();

      _lockWaits.clear();

      return true;
   }

   bool sendUnlockEvents()
   {
      if( _unlockEvents.empty() ) return false;

      const size_t unlocksMsgSize = sizeof( MsgInfo ) + _unlockEvents.size() * sizeof( UnlockEvent );

      ringbuf_t* ringbuf = ClientManager::sharedMemory().ringbuffer();
      uint8_t* bufferPtr = acquireSharedChunk( ringbuf, unlocksMsgSize );
      if ( !bufferPtr )
      {
         printf("HOP - Failed to acquire enough shared memory. Consider increasing shared memory size\n");
         _unlockEvents.clear();
         return false;
      }

      // Fill the buffer with the lock message
      {
         MsgInfo* uInfo = reinterpret_cast<MsgInfo*>(bufferPtr);
         uInfo->type = MsgType::PROFILER_UNLOCK_EVENT;
         uInfo->threadId = tl_threadId;
         uInfo->threadName = tl_threadName;
         uInfo->threadIndex = tl_threadIndex;
         uInfo->timeStamp = _unlockEvents.back().time;
         uInfo->unlockEvents.count = static_cast<uint32_t>(_unlockEvents.size());
         bufferPtr += sizeof( MsgInfo );
         memcpy( bufferPtr, _unlockEvents.data(), _unlockEvents.size() * sizeof( UnlockEvent ) );
      }

      ringbuf_produce( ringbuf, _worker );
      ClientManager::sharedMemory().signalSemaphore();

      _unlockEvents.clear();

      return true;
   }

   bool sendHeartbeat()
   {
      const size_t heartbeatSize = sizeof( MsgInfo );

      ringbuf_t* ringbuf = ClientManager::sharedMemory().ringbuffer();
      uint8_t* bufferPtr = acquireSharedChunk( ringbuf, heartbeatSize );
      if ( !bufferPtr )
      {
         printf("HOP - Failed to acquire enough shared memory. Consider increasing shared memory size\n");
         return false;
      }

      // Fill the buffer with the lock message
      {
         MsgInfo* hbInfo = reinterpret_cast<MsgInfo*>(bufferPtr);
         hbInfo->type = MsgType::PROFILER_HEARTBEAT;
         hbInfo->threadId = tl_threadId;
         hbInfo->threadName = tl_threadName;
         hbInfo->threadIndex = tl_threadIndex;
         hbInfo->timeStamp = getTimeStamp();
         bufferPtr += sizeof( MsgInfo );
      }

      ringbuf_produce( ringbuf, _worker );
      ClientManager::sharedMemory().signalSemaphore();

      return true;
   }

   void flushToConsumer()
   {
      // If we have a consumer, send life signal
      if( ClientManager::HasConnectedConsumer() )
      {
         sendHeartbeat();
      }

      // If no one is there to listen, no need to send any data
      if( !ClientManager::HasListeningConsumer() )
      {
         resetPendingTraces();
         return;
      }

      // If the shared memory reset timestamp more recent than our local one
      // it means we need to clear our string table. Otherwise it means we
      // already took care of it. Since some traces might depend on strings
      // that were added dynamically (ie before clearing the db), we cannot
      // consider them and need to return here.
      TimeStamp resetTimeStamp = ClientManager::sharedMemory().lastResetTimestamp();
      if( _clientResetTimeStamp < resetTimeStamp)
      {
         resetStringData();
         resetPendingTraces();
         return;
      }

      sendStringData(); // Always send string data first
      sendTraces();
      sendLockWaits();
      sendUnlockEvents();
      sendCores();
   }

   std::vector< Trace > _traces;
   std::vector< CoreEvent > _cores;
   std::vector< LockWait > _lockWaits;
   std::vector< UnlockEvent > _unlockEvents;
   std::unordered_set< StrPtr_t > _stringPtr;
   std::vector< char > _stringData;
   TimeStamp _clientResetTimeStamp{0};
   ringbuf_worker_t* _worker{NULL};
   uint32_t _sentStringDataSize{0}; // The size of the string array on viewer side
};

Client* ClientManager::Get()
{
   thread_local std::unique_ptr< Client > threadClient;

   if( unlikely( g_done.load() ) ) return nullptr;
   if( likely( threadClient.get() ) ) return threadClient.get();

   // If we have not yet created our shared memory segment, do it here
   if( !ClientManager::sharedMemory().valid() )
   {
      SharedMemory::ConnectionState state =
          ClientManager::sharedMemory().create( HOP_GET_PROG_NAME(), HOP_SHARED_MEM_SIZE, false );
      if ( state != SharedMemory::CONNECTED )
      {
         const char* reason = "";
         if( state == SharedMemory::PERMISSION_DENIED )
            reason = " : Permission Denied";
            
         printf("HOP - Could not create shared memory%s. HOP will not be able to run\n", reason );
         return NULL;
      }
   }

   // Atomically get the next thread id from the static atomic count
   static std::atomic< uint32_t > threadCount{0};
   tl_threadIndex = threadCount.fetch_add(1);
   tl_threadId = HOP_GET_THREAD_ID();

   if( tl_threadIndex > HOP_MAX_THREAD_NB )
   {
      printf( "Maximum number of threads reached. No trace will be available for this thread\n" );
      return nullptr;
   }

   // Register producer in the ringbuffer
   auto ringBuffer = ClientManager::sharedMemory().ringbuffer();
   if( ringBuffer )
   {
      threadClient.reset( new Client() );
      threadClient->_worker = ringbuf_register( ringBuffer, tl_threadIndex);
      if ( threadClient->_worker == NULL )
      {
         assert( false && "ringbuf_register" );
      }
   }

   return threadClient.get();
}

ZoneId_t ClientManager::StartProfile()
{
   ++tl_traceLevel;
   return tl_zoneId;
}

StrPtr_t ClientManager::StartProfileDynString( const char* str, ZoneId_t* zone )
{
   ++tl_traceLevel;
   Client* client = ClientManager::Get();

   if( unlikely( !client ) ) return 0;

   *zone = tl_zoneId;
   return client->addDynamicStringToDb( str );
}

void ClientManager::EndProfile(
    StrPtr_t fileName,
    StrPtr_t fctName,
    TimeStamp start,
    TimeStamp end,
    LineNb_t lineNb,
    ZoneId_t zone,
    Core_t core )
{
   const int remainingPushedTraces = --tl_traceLevel;
   Client* client = ClientManager::Get();

   if( unlikely( !client ) ) return;

   if( end - start > 50 ) // Minimum trace time is 50 ns
   {
      client->addProfilingTrace( fileName, fctName, start, end, lineNb, zone );
      client->addCoreEvent( core, start, end );
   }
   if ( remainingPushedTraces <= 0 )
   {
      client->flushToConsumer();
   }
}

void ClientManager::EndLockWait( void* mutexAddr, TimeStamp start, TimeStamp end )
{
   // Only add lock wait event if the lock is coming from within
   // measured code
   if( tl_traceLevel > 0 && end - start >= HOP_MIN_LOCK_CYCLES )
   {
      auto client = ClientManager::Get();
      if( unlikely( !client ) ) return;

      client->addWaitLockTrace( mutexAddr, start, end, static_cast<unsigned short>(tl_traceLevel) );
   }
}

void ClientManager::UnlockEvent( void* mutexAddr, TimeStamp time )
{
   if( tl_traceLevel > 0 )
   {
      auto client = ClientManager::Get();
      if( unlikely( !client ) ) return;

      client->addUnlockEvent( mutexAddr, time );
   }
}

void ClientManager::SetThreadName( const char* name ) HOP_NOEXCEPT
{
   auto client = ClientManager::Get();
   if( unlikely( !client ) ) return;

   client->setThreadName( reinterpret_cast<StrPtr_t>( name ) );
}

ZoneId_t ClientManager::PushNewZone( ZoneId_t newZone )
{
   ZoneId_t prevZone = tl_zoneId;
   tl_zoneId = newZone;
   return prevZone;
}

bool ClientManager::HasConnectedConsumer() HOP_NOEXCEPT
{
   return ClientManager::sharedMemory().valid() &&
          ClientManager::sharedMemory().hasConnectedConsumer();
}

bool ClientManager::HasListeningConsumer() HOP_NOEXCEPT
{
   return ClientManager::sharedMemory().valid() &&
          ClientManager::sharedMemory().hasListeningConsumer();
}

SharedMemory& ClientManager::sharedMemory() HOP_NOEXCEPT
{
   static SharedMemory _sharedMemory;
   return _sharedMemory;
}

} // end of namespace hop

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <inttypes.h>
#include <string.h>
#include <assert.h>
#include <algorithm>

/*
 * Exponential back-off for the spinning paths.
 */
#define SPINLOCK_BACKOFF_MIN 4
#define SPINLOCK_BACKOFF_MAX 128
#if defined( __x86_64__ ) || defined( __i386__ )
#define SPINLOCK_BACKOFF_HOOK __asm volatile( "pause" ::: "memory" )
#else
#define SPINLOCK_BACKOFF_HOOK
#endif
#define SPINLOCK_BACKOFF( count )                                     \
   do                                                                 \
   {                                                                  \
      for ( int __i = ( count ); __i != 0; __i-- )                    \
      {                                                               \
         SPINLOCK_BACKOFF_HOOK;                                       \
      }                                                               \
      if ( ( count ) < SPINLOCK_BACKOFF_MAX ) ( count ) += ( count ); \
   } while ( /* CONSTCOND */ 0 );

#define RBUF_OFF_MASK ( 0x00000000ffffffffUL )
#define WRAP_LOCK_BIT ( 0x8000000000000000UL )
#define RBUF_OFF_MAX ( UINT64_MAX & ~WRAP_LOCK_BIT )

#define WRAP_COUNTER ( 0x7fffffff00000000UL )
#define WRAP_INCR( x ) ( ( ( x ) + 0x100000000UL ) & WRAP_COUNTER )

typedef uint64_t ringbuf_off_t;

struct ringbuf_worker
{
   volatile ringbuf_off_t seen_off;
   int registered;
};

#if defined(_MSC_VER)
#pragma warning( push )
#pragma warning( disable : 4200) // Warning C4200 nonstandard extension used: zero-sized array in struct/union
#endif
struct ringbuf
{
   /* Ring buffer space. */
   size_t space;

   /*
    * The NEXT hand is atomically updated by the producer.
    * WRAP_LOCK_BIT is set in case of wrap-around; in such case,
    * the producer can update the 'end' offset.
    */
   std::atomic< ringbuf_off_t > next;
   ringbuf_off_t end;

   /* The following are updated by the consumer. */
   ringbuf_off_t written;
   unsigned nworkers;
   ringbuf_worker_t workers[];
};
#if defined(_MSC_VER)
#pragma warning( pop )
#endif

/*
 * ringbuf_setup: initialise a new ring buffer of a given length.
 */
int ringbuf_setup( ringbuf_t* rbuf, unsigned nworkers, size_t length )
{
   if ( length >= RBUF_OFF_MASK )
   {
      return -1;
   }
   memset( rbuf, 0, sizeof( ringbuf_t ) );
   rbuf->space = length;
   rbuf->end = RBUF_OFF_MAX;
   rbuf->nworkers = nworkers;
   return 0;
}

/*
 * ringbuf_get_sizes: return the sizes of the ringbuf_t and ringbuf_worker_t.
 */
void ringbuf_get_sizes( const unsigned nworkers, size_t* ringbuf_size, size_t* ringbuf_worker_size )
{
   if ( ringbuf_size )
   {
      *ringbuf_size = offsetof( ringbuf_t, workers ) + sizeof( ringbuf_worker_t ) * nworkers;
   }
   if ( ringbuf_worker_size )
   {
      *ringbuf_worker_size = sizeof( ringbuf_worker_t );
   }
}

/*
 * ringbuf_register: register the worker (thread/process) as a producer
 * and pass the pointer to its local store.
 */
ringbuf_worker_t* ringbuf_register( ringbuf_t* rbuf, unsigned i )
{
   ringbuf_worker_t* w = &rbuf->workers[i];

   w->seen_off = RBUF_OFF_MAX;
   std::atomic_thread_fence( std::memory_order_release );
   w->registered = true;
   return w;
}

void ringbuf_unregister( ringbuf_t*, ringbuf_worker_t* w )
{
   w->registered = false;
}

/*
 * stable_nextoff: capture and return a stable value of the 'next' offset.
 */
static inline ringbuf_off_t stable_nextoff( ringbuf_t* rbuf )
{
   unsigned count = SPINLOCK_BACKOFF_MIN;
   ringbuf_off_t next;

   while ( ( next = rbuf->next ) & WRAP_LOCK_BIT )
   {
      SPINLOCK_BACKOFF( count );
   }
   std::atomic_thread_fence( std::memory_order_acquire );
   assert( ( next & RBUF_OFF_MASK ) < rbuf->space );
   return next;
}

/*
 * ringbuf_acquire: request a space of a given length in the ring buffer.
 *
 * => On success: returns the offset at which the space is available.
 * => On failure: returns -1.
 */
ssize_t ringbuf_acquire( ringbuf_t* rbuf, ringbuf_worker_t* w, size_t len )
{
   ringbuf_off_t seen, next, target;

   assert( len > 0 && len <= rbuf->space );
   assert( w->seen_off == RBUF_OFF_MAX );

   do
   {
      ringbuf_off_t written;

      /*
       * Get the stable 'next' offset.  Save the observed 'next'
       * value (i.e. the 'seen' offset), but mark the value as
       * unstable (set WRAP_LOCK_BIT).
       *
       * Note: CAS will issue a std::memory_order_release for us and
       * thus ensures that it reaches global visibility together
       * with new 'next'.
       */
      seen = stable_nextoff( rbuf );
      next = seen & RBUF_OFF_MASK;
      assert( next < rbuf->space );
      w->seen_off = next | WRAP_LOCK_BIT;

      /*
       * Compute the target offset.  Key invariant: we cannot
       * go beyond the WRITTEN offset or catch up with it.
       */
      target = next + len;
      written = rbuf->written;
      if ( unlikely( next < written && target >= written ) )
      {
         /* The producer must wait. */
         w->seen_off = RBUF_OFF_MAX;
         return -1;
      }

      if ( unlikely( target >= rbuf->space ) )
      {
         const bool exceed = target > rbuf->space;

         /*
          * Wrap-around and start from the beginning.
          *
          * If we would exceed the buffer, then attempt to
          * acquire the WRAP_LOCK_BIT and use the space in
          * the beginning.  If we used all space exactly to
          * the end, then reset to 0.
          *
          * Check the invariant again.
          */
         target = exceed ? ( WRAP_LOCK_BIT | len ) : 0;
         if ( ( target & RBUF_OFF_MASK ) >= written )
         {
            w->seen_off = RBUF_OFF_MAX;
            return -1;
         }
         /* Increment the wrap-around counter. */
         target |= WRAP_INCR( seen & WRAP_COUNTER );
      }
      else
      {
         /* Preserve the wrap-around counter. */
         target |= seen & WRAP_COUNTER;
      }
   } while ( !std::atomic_compare_exchange_weak( &rbuf->next, &seen, target ) );

   /*
    * Acquired the range.  Clear WRAP_LOCK_BIT in the 'seen' value
    * thus indicating that it is stable now.
    */
   w->seen_off &= ~WRAP_LOCK_BIT;

   /*
    * If we set the WRAP_LOCK_BIT in the 'next' (because we exceed
    * the remaining space and need to wrap-around), then save the
    * 'end' offset and release the lock.
    */
   if ( unlikely( target & WRAP_LOCK_BIT ) )
   {
      /* Cannot wrap-around again if consumer did not catch-up. */
      assert( rbuf->written <= next );
      assert( rbuf->end == RBUF_OFF_MAX );
      rbuf->end = next;
      next = 0;

      /*
       * Unlock: ensure the 'end' offset reaches global
       * visibility before the lock is released.
       */
      std::atomic_thread_fence( std::memory_order_release );
      rbuf->next = ( target & ~WRAP_LOCK_BIT );
   }
   assert( ( target & RBUF_OFF_MASK ) <= rbuf->space );
   return static_cast<ssize_t>(next);
}

/*
 * ringbuf_produce: indicate the acquired range in the buffer is produced
 * and is ready to be consumed.
 */
void ringbuf_produce( ringbuf_t* , ringbuf_worker_t* w )
{
   assert( w->registered );
   assert( w->seen_off != RBUF_OFF_MAX );
   std::atomic_thread_fence( std::memory_order_release );
   w->seen_off = RBUF_OFF_MAX;
}

/*
 * ringbuf_consume: get a contiguous range which is ready to be consumed.
 */
size_t ringbuf_consume( ringbuf_t* rbuf, size_t* offset )
{
   ringbuf_off_t written = rbuf->written, next, ready;
   size_t towrite;
retry:
   /*
    * Get the stable 'next' offset.  Note: stable_nextoff() issued
    * a load memory barrier.  The area between the 'written' offset
    * and the 'next' offset will be the *preliminary* target buffer
    * area to be consumed.
    */
   next = stable_nextoff( rbuf ) & RBUF_OFF_MASK;
   if ( written == next )
   {
      /* If producers did not advance, then nothing to do. */
      return 0;
   }

   /*
    * Observe the 'ready' offset of each producer.
    *
    * At this point, some producer might have already triggered the
    * wrap-around and some (or all) seen 'ready' values might be in
    * the range between 0 and 'written'.  We have to skip them.
    */
   ready = RBUF_OFF_MAX;

   for ( unsigned i = 0; i < rbuf->nworkers; i++ )
   {
      ringbuf_worker_t* w = &rbuf->workers[i];
      unsigned count = SPINLOCK_BACKOFF_MIN;
      ringbuf_off_t seen_off;

      /* Skip if the worker has not registered. */
      if ( !w->registered )
      {
         continue;
      }

      /*
       * Get a stable 'seen' value.  This is necessary since we
       * want to discard the stale 'seen' values.
       */
      while ( ( seen_off = w->seen_off ) & WRAP_LOCK_BIT )
      {
         SPINLOCK_BACKOFF( count );
      }

      /*
       * Ignore the offsets after the possible wrap-around.
       * We are interested in the smallest seen offset that is
       * not behind the 'written' offset.
       */
      if ( seen_off >= written )
      {
         ready = HOP_MIN( seen_off, ready );
      }
      assert( ready >= written );
   }

   /*
    * Finally, we need to determine whether wrap-around occurred
    * and deduct the safe 'ready' offset.
    */
   if ( next < written )
   {
      const ringbuf_off_t end = HOP_MIN( static_cast<ringbuf_off_t>(rbuf->space), rbuf->end );

      /*
       * Wrap-around case.  Check for the cut off first.
       *
       * Reset the 'written' offset if it reached the end of
       * the buffer or the 'end' offset (if set by a producer).
       * However, we must check that the producer is actually
       * done (the observed 'ready' offsets are clear).
       */
      if ( ready == RBUF_OFF_MAX && written == end )
      {
         /*
          * Clear the 'end' offset if was set.
          */
         if ( rbuf->end != RBUF_OFF_MAX )
         {
            rbuf->end = RBUF_OFF_MAX;
            std::atomic_thread_fence( std::memory_order_release );
         }
         /* Wrap-around the consumer and start from zero. */
         rbuf->written = written = 0;
         goto retry;
      }

      /*
       * We cannot wrap-around yet; there is data to consume at
       * the end.  The ready range is smallest of the observed
       * 'ready' or the 'end' offset.  If neither is set, then
       * the actual end of the buffer.
       */
      assert( ready > next );
      ready = HOP_MIN( ready, end );
      assert( ready >= written );
   }
   else
   {
      /*
       * Regular case.  Up to the observed 'ready' (if set)
       * or the 'next' offset.
       */
      ready = HOP_MIN( ready, next );
   }
   towrite = ready - written;
   *offset = written;

   assert( ready >= written );
   assert( towrite <= rbuf->space );
   return towrite;
}

/*
 * ringbuf_release: indicate that the consumed range can now be released.
 */
void ringbuf_release( ringbuf_t* rbuf, size_t nbytes )
{
   const size_t nwritten = rbuf->written + nbytes;

   assert( rbuf->written <= rbuf->space );
   assert( rbuf->written <= rbuf->end );
   assert( nwritten <= rbuf->space );

   rbuf->written = ( nwritten == rbuf->space ) ? 0 : nwritten;
}

#endif  // end HOP_IMPLEMENTATION

#endif  // !defined(HOP_ENABLED)

#endif  // HOP_H_
