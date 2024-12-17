//******************************************************************************
// Copyright (c) 2008 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <TwkExc/Exception.h>
#include <stl_ext/replace_alloc.h>
#include <TwkUtil/FileStream.h>
#include <TwkUtil/File.h>
#include <TwkUtil/Timer.h>
#include <TwkUtil/MemPool.h>
// #include <TwkUtil/Interrupt.h>
#include <pthread.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string>
#include <vector>
#include <sstream>
#include <sys/types.h>
#ifdef WIN32
#include <windows.h>
#include <posix_file.h>
#else
#include <sys/mman.h>
#endif
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stl_ext/replace_alloc.h>
#include <string.h>
#include <algorithm>

#if defined(PLATFORM_APPLE_MACH_BSD)
//
//  Darwin doesn't have O_DIRECT or posix_fadvise (or any fadvise) so
//  we'll just make those no-ops to keep the code simple.
//
#ifndef O_DIRECT
#define O_DIRECT 0
#endif
#define posix_fadvise(A, B, C, D) ;
#endif

namespace TwkUtil
{
    using namespace std;

    //
    //  The following is an attempt to measure the actual MB/sec that
    //  FileStream is achieving regardless of how many threads are
    //  making FileStream readers.
    //

    class MbpsCalculator
    {
    public:
        //
        //  A Monitor class object, whose construction and deletion
        //  trigger the timing mechanisms in MbpsCalculator proper.
        //  A given Monitor instance will contribute nothing to the
        //  total MB/sec unless setBytes() is called.
        //

        class Monitor
        {
        public:
            Monitor(MbpsCalculator& calc);
            ~Monitor();

            void setBytes(size_t bytes) { m_bytes = bytes; };

        private:
            MbpsCalculator& m_calc;
            size_t m_bytes;
        };

        MbpsCalculator();

        float mbps() const
        {
            return (0.0 != m_totalTime)
                       ? 1.0e-6 * float(m_totalBytes) / m_totalTime
                       : 0.0;
        };

        void resetMbps()
        {
            m_totalBytes = 0;
            m_totalTime = 0.0;
        };

    private:
        void start();
        void update(size_t bytes);

        TwkUtil::Timer m_timer;
        size_t m_totalBytes;
        float m_totalTime;
        pthread_mutex_t m_mutex;
        int m_activeCount;
    };

    MbpsCalculator::Monitor::Monitor(MbpsCalculator& calc)
        : m_calc(calc)
        , m_bytes(0)
    {
        m_calc.start();
    }

    MbpsCalculator::Monitor::~Monitor() { m_calc.update(m_bytes); }

    MbpsCalculator::MbpsCalculator()
        : m_totalBytes(0)
        , m_totalTime(0.0)
        , m_activeCount(0)
    {
        pthread_mutex_init(&m_mutex, 0);
    }

    void MbpsCalculator::start()
    {
        pthread_mutex_lock(&m_mutex);

        if (0 == m_activeCount)
            m_timer.start();
        ++m_activeCount;

        pthread_mutex_unlock(&m_mutex);
    }

    void MbpsCalculator::update(size_t bytes)
    {
        pthread_mutex_lock(&m_mutex);

        --m_activeCount;
        float time = m_timer.stop();
        m_totalTime += time;
        m_totalBytes += bytes;
        if (0 < m_activeCount)
            m_timer.start();

        pthread_mutex_unlock(&m_mutex);
        /*
        fprintf (stderr, "MbpsCalculator::update() time %g bytes %d\n",
        m_totalTime, m_totalBytes); fflush (stderr);
        */
    }

    MbpsCalculator mbpsCalc;

    double FileStream::mbps() { return mbpsCalc.mbps(); }

    void FileStream::resetMbps() { mbpsCalc.resetMbps(); }

#ifndef WIN32

    //----------------------------------------------------------------------
    //  UNIX
    //----------------------------------------------------------------------

#include <unistd.h>

#ifdef _POSIX_ASYNCHRONOUS_IO
#ifdef PLATFORM_LINUX

    //
    // async io
    // this works on 2.6 kernels
    //

#include <libaio.h>

    class KernelReadRequest : public iocb
    {
    public:
        KernelReadRequest();

        void clear();
    };

    KernelReadRequest::KernelReadRequest() { clear(); }

    void KernelReadRequest::clear()
    {
        //
        //  Don't really know what all the fields here are, or what they
        //  do, so zero everything.  Also, there are private fields in
        //  here that may confuse things lower down if not reset.
        //
        bzero((char*)this, sizeof(KernelReadRequest));
    }

    class KernelReadRequestList
    {
    public:
        KernelReadRequestList(int fd, size_t cs, int nr, size_t total,
                              void* buffer);
        ~KernelReadRequestList();

        //
        //  Check for any completed requests, make new ones, etc.
        //  return true when we've read everything
        //
        bool update();

    private:
        void handleCompleted(KernelReadRequest* req, long bytesRead,
                             long errorCode);

        KernelReadRequest* nextFreeRequest();

        //
        //  Actual request structures
        //
        typedef vector<KernelReadRequest> RecVec;
        RecVec requests;

        //
        //  Pointers to request structures (system calls require arrays
        //  of pointers).  We put pointers in free list, then in current
        //  list, then back in free list (when the request completes).
        //
        typedef vector<KernelReadRequest*> RecPVec;
        RecPVec freeRequests;
        RecPVec currentRequests;

        //
        //  The kernel informs us of completed requests through
        //  io_events.
        //
        vector<struct io_event> requestEvents;

        int fileDescriptor;
        size_t chunkSize;
        int numRequests;         //  Max number of simultaneous requests
        int requestsInFlight;    //  Number of currently active requests
        int maxRequestsInFlight; //  Number of currently active requests
        size_t totalSize;        //  Total bytes to be read
        size_t completedSize;    //  Bytes read so far
        size_t requestedSize;    //  Bytes requested so far
        char* buffer;

        io_context_t context;
    };

    KernelReadRequestList::KernelReadRequestList(int fd, size_t cs, int nr,
                                                 size_t total, void* buf)
        : fileDescriptor(fd)
        , chunkSize(cs)
        , numRequests(nr)
        , totalSize(total)
        , buffer((char*)buf)
    {
        requestsInFlight = 0;
        requestedSize = 0;
        completedSize = 0;
        maxRequestsInFlight = numRequests;

        requests.resize(numRequests);
        freeRequests.resize(numRequests);
        currentRequests.resize(numRequests);
        requestEvents.resize(numRequests);

        //
        //  Initialize free list with all requests
        //
        for (int i = 0; i < requests.size(); ++i)
            freeRequests[i] = &(requests[i]);

        //
        //  Get async io context from kernel
        //
        bzero((char*)(&context), sizeof(context));
        int ret = io_queue_init(numRequests, &context);
        if (0 > ret)
        {
            TWK_THROW_EXC_STREAM("io_queue_init returned "
                                 << ret << ". ASync IO unavailable");
        }
    }

    KernelReadRequestList::~KernelReadRequestList()
    {
        //
        //  Destroy async io context, and hopefully cancel any active
        //  requests.
        //
        io_destroy(context);
    }

    //
    //  Take a request off the free list.
    //

    KernelReadRequest* KernelReadRequestList::nextFreeRequest()
    {
        if (freeRequests.size() == 0)
        {
            TWK_THROW_EXC_STREAM("KernelReadRequestList ran out of requests!");
        }
        KernelReadRequest* req = freeRequests.back();
        freeRequests.pop_back();

        return req;
    }

    //
    //  Handle a completed io request
    //

    void KernelReadRequestList::handleCompleted(KernelReadRequest* req,
                                                long bytesRead, long errorCode)
    {
        if (0 != errorCode)
        {
            TWK_THROW_EXC_STREAM(
                "KernelReadRequestList async read failed: " << errorCode);
        }
        if (req->u.c.nbytes != bytesRead)
        {
            TWK_THROW_EXC_STREAM("KernelReadRequestList async read incomplete ("
                                 << bytesRead << " bytes out of "
                                 << req->u.c.nbytes << ")");
        }

        completedSize += bytesRead;
        --requestsInFlight;
        freeRequests.push_back(req);
    }

    //
    //  Deal with completed requests, make new requests,
    //  return true if all bytes have been read;
    //

    bool KernelReadRequestList::update()
    {
        if (requestsInFlight)
        //
        //  Have requests in process, wait for at least one to finish,
        //  then process all completed requests.
        //
        {
            int n = io_getevents(context, 1, requestsInFlight,
                                 &(requestEvents[0]), 0);
            if (1 > n)
            {
                TWK_THROW_EXC_STREAM("io_getevents returned "
                                     << n << ". ASync IO unavailable");
            }

            for (int i = 0; i < n; ++i)
            {
                struct io_event& e = requestEvents[i];

                handleCompleted((KernelReadRequest*)(e.obj), e.res, e.res2);
            }
        }

        //
        //  Fill the queue with requests
        //

        int requestsToStart = numRequests - requestsInFlight;
        currentRequests.resize(0);
        for (int i = 0; i < requestsToStart; ++i)
        {
            if (requestedSize < totalSize)
            {
                KernelReadRequest* req = nextFreeRequest();

                size_t reqSize = min(chunkSize, totalSize - requestedSize);

                io_prep_pread(req, fileDescriptor, buffer + requestedSize,
                              reqSize, requestedSize);

                currentRequests.push_back(req);
                requestedSize += reqSize;
            }
        }

        //
        //  Launch requests
        //
        int n = io_submit(context, currentRequests.size(),
                          (struct iocb**)&(currentRequests[0]));
        if (currentRequests.size() != n)
        {
            TWK_THROW_EXC_STREAM("io_submit returned "
                                 << n << ", expected " << currentRequests.size()
                                 << ". ASync IO unavailable");
        }
        requestsInFlight += n;

        return (completedSize == totalSize);
    }

#define ReadRequestList KernelReadRequestList

#else //  PLATFORM_LINUX

#include <aio.h>

    class PosixReadRequest : public aiocb
    {
    public:
        PosixReadRequest();

        void clear();

        bool inUse() { return aio_fildes != -1; };
    };

    PosixReadRequest::PosixReadRequest() { clear(); }

    void PosixReadRequest::clear()
    {
        //
        //  Don't really know what all the fields here are, or what they
        //  do, so zero everything.  Also, there are private fields in
        //  here that may confuse things lower down if not reset.
        //
        bzero((char*)this, sizeof(PosixReadRequest));

        //
        //  Set invalid fd, so we know this request is not in use.
        //
        aio_fildes = -1;
    }

    class PosixReadRequestList
    {
    public:
        PosixReadRequestList(int fd, size_t cs, int nr, size_t total,
                             void* buffer);
        ~PosixReadRequestList();

        //
        //  Check for any completed requests, make new ones, etc.
        //  return true when we've read everything
        //
        bool update();

    private:
        void handleCompleted(PosixReadRequest* req);

        typedef std::vector<PosixReadRequest> RecVec;
        RecVec requests;

        int fileDescriptor;
        size_t chunkSize;
        int numRequests;      //  Max number of simultaneous requests
        int requestsInFlight; //  Number of currently active requests
        size_t totalSize;     //  Total bytes to be read
        size_t completedSize; //  Bytes read so far
        size_t requestedSize; //  Bytes requested so far
        char* buffer;
    };

    PosixReadRequestList::PosixReadRequestList(int fd, size_t cs, int nr,
                                               size_t total, void* buf)
        : fileDescriptor(fd)
        , chunkSize(cs)
        , numRequests(nr)
        , totalSize(total)
        , buffer((char*)buf)
    {
        requestedSize = 0;
        requestsInFlight = 0;
        completedSize = 0;

        requests.resize(numRequests);
    }

    PosixReadRequestList::~PosixReadRequestList()
    {
        //  Cancel outstanding requests

        for (RecVec::iterator i = requests.begin(); i != requests.end(); ++i)
        {
            if (i->inUse())
            {
                aio_cancel(i->aio_fildes, &(*i));
                i->clear();
            }
        }
    }

    //
    //  Handle a completed io request
    //

    void PosixReadRequestList::handleCompleted(PosixReadRequest* req)
    {
        int bytesRead = aio_return(req);

        if (0 > bytesRead)
        //
        //  We had an error during read.
        //
        {
            TWK_THROW_EXC_STREAM("aio_return failed, Async IO unavailable");
        }
        else if (req->aio_nbytes > bytesRead)
        //
        //  We read something, but didn't fill the buffer,
        //  so we request another read for the remainder.
        //  I don't think this should ever happen, but just in case ...
        //
        {
            completedSize += bytesRead;
            req->aio_nbytes = req->aio_nbytes - bytesRead;
            req->aio_buf = (char*)(req->aio_buf) + bytesRead;
            req->aio_offset = req->aio_offset + bytesRead;

            int ret = aio_read(req);
            if (0 > ret)
            {
                TWK_THROW_EXC_STREAM("aio_read failed (1), returned "
                                     << errno << ", " << strerror(errno)
                                     << ". Async IO unavailable");
            }
        }
        else
        //
        //  We filled the buffer, mark this request for reuse.
        //
        {
            completedSize += bytesRead;
            req->clear();
        }
    }

    //
    //  Deal with completed requests, make new requests,
    //  return true if all bytes have been read;
    //

    bool PosixReadRequestList::update()
    {
        for (RecVec::iterator i = requests.begin(); i != requests.end(); ++i)
        {
            if (i->inUse())
            {
                //  Check for completed requests

                int ret = aio_error(&(*i));
                if (0 == ret)
                //
                //  This request is complete
                //
                {
                    handleCompleted(&(*i));
                    --requestsInFlight;
                }
                else if (EINPROGRESS != ret)
                //
                //  This request is neither complete, nor in progress
                //
                {
                    TWK_THROW_EXC_STREAM("aio_error failed, returned "
                                         << errno << ", " << strerror(errno)
                                         << ". ASync IO unavailable");
                }
            }

            if (!i->inUse() && requestedSize < totalSize
                && requestsInFlight < numRequests)
            //
            //  This request structure is not in use, and we still need data.
            //
            {
                //  Make a new request

                i->aio_fildes = fileDescriptor;
                i->aio_nbytes = min(chunkSize, totalSize - requestedSize);
                i->aio_buf = buffer + requestedSize;
                i->aio_offset = requestedSize;

                int ret = aio_read(&(*i));
                if (0 > ret)
                {
                    if (EAGAIN == errno && 1 < numRequests
                        && 0 < requestsInFlight)
                    //
                    //  aio_read failed due to lack of resources.  Drop
                    //  the number of in-flight requests allowed and go on.
                    //
                    {
                        i->clear();
                        numRequests = requestsInFlight;
                    }
                    else
                    {
                        TWK_THROW_EXC_STREAM("aio_read failed (2), returned "
                                             << errno << ", " << strerror(errno)
                                             << ". ASync IO unavailable");
                    }
                }
                else
                {
                    requestedSize += i->aio_nbytes;
                    ++requestsInFlight;
                }
            }
        }

        return (completedSize == totalSize);
    }

#define ReadRequestList PosixReadRequestList

#endif //  PLATFORM_LINUX
#endif //  _POSIX_ASYNCHRONOUS_IO

    static int bufferingMessageCount = 0;

    FileStream::FileStream(const string& filename, Type type, size_t size,
                           int maxInFlight, bool deleteOnDestruction)
        : m_filename(filename)
        , m_deleteOnDestruction(deleteOnDestruction)
        , m_rawdata(0)
        , m_startOffset(0)
        , m_readSize(0)
        , m_fileSize(0)
        , m_type(type)
        , m_private(0)
        , m_chunkSize(size)
        , m_maxInFlight(maxInFlight)
    {
        initialize();
    }

    FileStream::FileStream(const string& filename, size_t startOffset,
                           size_t readSize, Type type, size_t size,
                           int maxInFlight, bool deleteOnDestruction)
        : m_filename(filename)
        , m_deleteOnDestruction(deleteOnDestruction)
        , m_rawdata(0)
        , m_startOffset(startOffset)
        , m_readSize(readSize)
        , m_fileSize(0)
        , m_type(type)
        , m_private(0)
        , m_chunkSize(size)
        , m_maxInFlight(maxInFlight)
    {
        initialize();
    }

    void FileStream::initialize()
    {
        if (m_type == ASyncBuffering)
        {
            //
            //  Async only works on descriptors opened with O_DIRECT.
            //
            m_type = ASyncNonBuffering;
        }

        /*
        cerr << "FileStream, size " << m_chunkSize <<
                " m_type " << m_type <<
                " thread " << pthread_self() << "   " <<
                mbps() << " MB/sec " << endl;
        */

        MbpsCalculator::Monitor mon(mbpsCalc);

        //
        //  Open the file
        //

        int direct = (m_type == ASyncNonBuffering || m_type == NonBuffering)
                         ? O_DIRECT
                         : 0;
        m_file = TwkUtil::open(m_filename.c_str(), O_RDONLY | direct);

        if (m_file == -1)
        {
            if (direct)
            {
                if (bufferingMessageCount++ < 5)
                {
                    cerr
                        << "WARNING: filesystem does not support direct "
                           "(unbuffered) reads, falling back to buffered reads."
                        << endl;
                }
                direct = 0;
                m_type = Buffering;
                m_file = TwkUtil::open(m_filename.c_str(), O_RDONLY);
            }

            if (m_file == -1)
            {
                TWK_THROW_EXC_STREAM("Stream: cannot open " << m_filename
                                                            << " errno "
                                                            << strerror(errno));
            }
        }

        //
        //  Find file size. Adjust the offset and total size if user specified
        //  some
        //

        m_fileSize = lseek(m_file, 0, SEEK_END);
        lseek(m_file, m_startOffset, SEEK_SET);
        if (!m_fileSize)
        {
            TWK_THROW_EXC_STREAM("Stream: empty file " << m_filename);
        }
        m_fileSize = m_fileSize - m_startOffset;
        if (m_readSize)
            m_fileSize = std::min(m_readSize, size_t(m_fileSize));

        /*
        fprintf (stderr, "%p FileStream type %d request for %d byte file\n",
        pthread_self(), m_type, m_fileSize); fflush (stderr);
        */

        //
        //  In linux 2.6 and later, O_DIRECT io must be
        //  512-boundary-aligned.  This means buffers, offsets, and
        //  counts !
        //

        if (direct && m_chunkSize % 512)
        {
            m_chunkSize = max(size_t(512), 512 * (m_chunkSize / 512));
        }

        //
        //  We can't read non-multiple-of-512-sized files with
        //  O_DIRECT, so read in two steps.  First the multiple of
        //  512, then whatever's left over.
        //

        size_t leftOverSize = 0;
        size_t readableSize = m_fileSize;
        if (direct)
        {
            leftOverSize = m_fileSize % 512;
            readableSize = m_fileSize - leftOverSize;
        }
        if (m_type == ASyncNonBuffering)
        {
            m_rawdata = MemPool::alloc(m_fileSize);
            if (!m_rawdata)
                TWK_THROW_EXC_STREAM("Out of memory");
            posix_madvise(m_rawdata, m_fileSize, POSIX_MADV_WILLNEED);

            if (readableSize)
            {
                ReadRequestList list(m_file, m_chunkSize, m_maxInFlight,
                                     readableSize, m_rawdata);

                while (!list.update())
                    ;
            }
            close(m_file);
        }
        else if (m_type == Buffering || m_type == NonBuffering)
        {
            m_rawdata = MemPool::alloc(m_fileSize);
            if (!m_rawdata)
                TWK_THROW_EXC_STREAM("Out of memory");
            posix_madvise(m_rawdata, m_fileSize,
                          POSIX_MADV_SEQUENTIAL | POSIX_MADV_WILLNEED);

            if (readableSize)
            {
                if (-1 == read(m_file, m_rawdata, readableSize))
                {
                    close(m_file);
                    TWK_THROW_EXC_STREAM("read1: " << strerror(errno) << ": "
                                                   << m_filename);
                }
            }
            close(m_file);
        }
        else if (m_type == MemoryMap)
        {
            m_rawdata = mmap(0, m_fileSize, PROT_READ, MAP_SHARED, m_file, 0);
            posix_madvise(m_rawdata, m_fileSize,
                          POSIX_MADV_SEQUENTIAL | POSIX_MADV_WILLNEED);

            if (m_rawdata == MAP_FAILED)
            {
                close(m_file);
                TWK_THROW_EXC_STREAM("MMap: " << strerror(errno) << ": "
                                              << m_filename);
            }
        }

        //
        //  If we had to split the read, read the leftover part now.
        //
        if (leftOverSize)
        {
            m_file = TwkUtil::open(m_filename.c_str(), O_RDONLY);
            lseek(m_file, readableSize, SEEK_SET);
            if (-1
                == read(m_file, ((char*)(m_rawdata)) + readableSize,
                        leftOverSize))
            {
                close(m_file);
                TWK_THROW_EXC_STREAM("read2: " << strerror(errno) << ": "
                                               << m_filename);
            }
            close(m_file);
        }

        mon.setBytes(m_fileSize);
    }

    FileStream::~FileStream()
    {
        if (m_type == MemoryMap)
        {
            if (m_rawdata && m_fileSize)
            {
                munmap(m_rawdata, m_fileSize);
                close(m_file);
            }
        }
        else if (m_deleteOnDestruction)
        {
            deleteDataPointer(m_rawdata);
        }
    }

    void FileStream::deleteDataPointer(void* p) { MemPool::dealloc(p); }

#else

    struct WinStreamPrivate;

#define Copy64(Dest, Src) *((PULONGLONG) & (Dest)) = *((PULONGLONG) & (Src))

    struct WinStreamPrivate
    {
        WinStreamPrivate()
            : m_file(0)
            , m_map(0)
            , m_doneEvent(0)
        {
        }

        HANDLE m_file;
        HANDLE m_map;
        OVERLAPPED m_overlapData[8];
        char* m_startPointer;
        char* m_currentPointer;
        char* m_endPointer;
        DWORD m_requestSize;
        HANDLE m_doneEvent;
        size_t m_numActive;
    };

    VOID WINAPI completeFunc(DWORD dwError,       // I/O completion status
                             DWORD dwTransferred, // Bytes read/written
                             LPOVERLAPPED o)      // Overlapped I/O structure
    {
        WinStreamPrivate* imp = (WinStreamPrivate*)o->hEvent;
        imp->m_numActive--;

        //  cerr << "completeFunc transferrred " << dwTransferred << endl;

        if (dwTransferred == imp->m_requestSize
            && imp->m_currentPointer < imp->m_endPointer)
        {
            void* readLocation = imp->m_currentPointer;
            imp->m_currentPointer += imp->m_requestSize;
            memset(o, 0, sizeof(OVERLAPPED));
            ULONGLONG d = (char*)readLocation - (char*)imp->m_startPointer;
            Copy64(o->Offset, d);
            o->hEvent = imp;

            size_t readSize = imp->m_requestSize;

            if (ReadFileEx(imp->m_file, readLocation, readSize, o,
                           completeFunc))
            {
                imp->m_numActive++;
            }
            else
            {
                // cout << "bad " << readSize << endl;
            }
        }
        else
        {
            // cout << "dwTransferred = " << dwTransferred << endl;
        }

        if (imp->m_numActive == 0)
        {
            SetEvent(imp->m_doneEvent);
        }
    }

    FileStream::FileStream(const string& filename, Type type, size_t size,
                           int maxInFlight, bool deleteOnDestruction)
        : m_filename(filename)
        , m_deleteOnDestruction(deleteOnDestruction)
        , m_rawdata(0)
        , m_startOffset(0)
        , m_readSize(0)
        , m_fileSize(0)
        , m_type(type)
        , m_private(0)
        , m_chunkSize(size)
        , m_maxInFlight(maxInFlight)
    {
        initialize();
    }

    FileStream::FileStream(const string& filename, size_t startOffset,
                           size_t readSize, Type type, size_t size,
                           int maxInFlight, bool deleteOnDestruction)
        : m_filename(filename)
        , m_deleteOnDestruction(deleteOnDestruction)
        , m_rawdata(0)
        , m_startOffset(startOffset)
        , m_readSize(readSize)
        , m_fileSize(0)
        , m_type(type)
        , m_private(0)
        , m_chunkSize(size)
        , m_maxInFlight(maxInFlight)
    {
        initialize();
    }

    void FileStream::initialize()
    {
        WinStreamPrivate* imp = new WinStreamPrivate;
        m_private = imp;

        // if (getenv("TWEAK_ASYNC_PACKET_SIZE"))
        //{
        // imp->m_requestSize = atoi(getenv("TWEAK_ASYNC_PACKET_SIZE"));
        //}

        if (m_chunkSize % 512)
        {
            m_chunkSize = max(size_t(512), 512 * (m_chunkSize / 512));
        }

        // imp->m_requestSize = (1 << 16) - (1 << 12); // TCP payload
        // imp->m_requestSize = (1 << 16);
        imp->m_requestSize = m_chunkSize;
        imp->m_numActive = 0;

        DWORD flags = FILE_FLAG_SEQUENTIAL_SCAN;

        struct __stat64 fileStat;
        int err = _wstat64(UNICODE_C_STR(m_filename.c_str()), &fileStat);

        if (err)
        {
            TWK_THROW_EXC_STREAM("FileStream: no stat");
        }

        m_fileSize = fileStat.st_size;
        if (!m_fileSize)
        {
            TWK_THROW_EXC_STREAM("Stream: empty file " << m_filename);
        }

        //
        //  If the file is small, just use memory mapping instead of Async
        //

        //  cerr << "m_type " << m_type << " asyncnonbuffering " <<
        //  ASyncNonBuffering << endl;
        if ((m_type == ASyncBuffering || m_type == ASyncNonBuffering)
            && m_fileSize < imp->m_requestSize * 8)
        {
            //  cerr << "switching to mmap, size " << m_fileSize << endl;
            m_type = MemoryMap;
        }

        OFSTRUCT of;

        if (m_type == ASyncNonBuffering)
        {
            flags |= FILE_FLAG_NO_BUFFERING;
            flags |= FILE_FLAG_OVERLAPPED;
        }
        else if (m_type == ASyncBuffering)
        {
            flags |= FILE_FLAG_OVERLAPPED;
        }

        MbpsCalculator::Monitor mon(mbpsCalc);

#ifdef _MSC_VER
        imp->m_file = CreateFileW(
#else
        imp->m_file = CreateFile(
#endif
            UNICODE_C_STR(m_filename.c_str()), GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
            OPEN_EXISTING, flags, NULL);

        //
        //  Move the file pointer if needed
        //

        if (m_startOffset > 0)
        {
            LARGE_INTEGER inout;
            inout.LowPart = 0;
            inout.HighPart = 0;

            LARGE_INTEGER offset;
            offset.HighPart = 0;
            offset.LowPart = m_startOffset;

            cout << "ABOUT TO SET POINTER" << endl;

            SetFilePointerEx(imp->m_file, offset, &inout, FILE_BEGIN);
        }

        m_fileSize = m_fileSize - m_startOffset;

        if (m_readSize != 0 && m_readSize < m_fileSize)
        {
            m_fileSize = m_readSize;
        }

        if (imp->m_file == INVALID_HANDLE_VALUE)
        {
            cout << "THROWING" << endl;
            TWK_THROW_EXC_STREAM("CreateFile: cannot open "
                                 << m_filename << " error: " << GetLastError());
        }

        if (m_type == Buffering || m_type == NonBuffering)
        {
            DWORD nread = 0;
            m_rawdata = MemPool::alloc(m_fileSize);
            if (!m_rawdata)
                TWK_THROW_EXC_STREAM("Out of memory");

            if (!ReadFile(imp->m_file, m_rawdata, m_fileSize, &nread, NULL))
            {
                CloseHandle(imp->m_file);
                TWK_THROW_EXC_STREAM("ReadFile: failed reading " << m_filename);
            }

            CloseHandle(imp->m_file);
        }
        else if (m_type == MemoryMap)
        {
            imp->m_map =
                CreateFileMapping(imp->m_file, NULL, PAGE_READONLY, 0, 0, NULL);

            if (!imp->m_map || imp->m_map == INVALID_HANDLE_VALUE)
            {
                CloseHandle(imp->m_file);
                TWK_THROW_EXC_STREAM("CreateFileMapping: cannot open "
                                     << m_filename);
            }

            m_rawdata =
                (void*)MapViewOfFile(imp->m_map, FILE_MAP_READ, 0, 0, 0);

            if (!m_rawdata)
            {
                CloseHandle(imp->m_file);
                TWK_THROW_EXC_STREAM("MapViewOfFile: cannot open "
                                     << m_filename);
            }
        }
        else if (m_type == ASyncBuffering || m_type == ASyncNonBuffering)
        {
            m_rawdata = MemPool::alloc(m_fileSize);
            if (!m_rawdata)
                TWK_THROW_EXC_STREAM("Out of memory");
            imp->m_startPointer = (char*)m_rawdata;
            imp->m_currentPointer = (char*)m_rawdata;
            imp->m_endPointer = imp->m_currentPointer + m_fileSize;
            imp->m_doneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

            if (imp->m_doneEvent == INVALID_HANDLE_VALUE)
            {
                TWK_THROW_EXC_STREAM("Event creation failed");
            }

            if (imp->m_doneEvent == INVALID_HANDLE_VALUE)
                abort();

            for (size_t i = 0; i < 8; i++)
            {
                OVERLAPPED* o = &imp->m_overlapData[i];
                memset(o, 0, sizeof(OVERLAPPED));
                o->hEvent = imp;

                void* readLocation = imp->m_currentPointer;
                imp->m_currentPointer += imp->m_requestSize;

                ULONGLONG d = (char*)readLocation - (char*)imp->m_startPointer;
                Copy64(o->Offset, d);

                int remaining = m_fileSize - d;

                if (remaining > 0)
                {
                    //  cerr << "calling ReadFileEx i " << i << " remaining " <<
                    //  remaining << endl;
                    if (!ReadFileEx(imp->m_file, readLocation,
                                    imp->m_requestSize, o, completeFunc))
                    {
                        TWK_THROW_EXC_STREAM("CreateFile: cannot read "
                                             << m_filename
                                             << " error: " << GetLastError());
                    }

                    imp->m_numActive++;
                }
            }

            //
            // Wait for all to complete
            //

            for (bool done = false; !done;)
            {
                switch (WaitForSingleObjectEx(imp->m_doneEvent, INFINITE, TRUE))
                {
                case WAIT_OBJECT_0:
                    done = true;
                    // cout << "finished" << endl;
                    break;
                case WAIT_IO_COMPLETION:
                    // cout << "completion occured" << endl;
                    break;
                case WAIT_ABANDONED:
                    // cout << "abandoned" << endl;
                    break;
                case WAIT_TIMEOUT:
                    // cout << "timeout" << endl;
                    break;
                }
            }

            // abort();

            CloseHandle(imp->m_doneEvent);
            CloseHandle(imp->m_file);
        }

        mon.setBytes(m_fileSize);
    }

    FileStream::~FileStream()
    {
        WinStreamPrivate* imp = (WinStreamPrivate*)m_private;

        if (imp->m_map)
        {
            if (m_rawdata)
                UnmapViewOfFile(m_rawdata);
            if (imp->m_map != INVALID_HANDLE_VALUE)
                CloseHandle(imp->m_map);
            if (imp->m_file && imp->m_file != INVALID_HANDLE_VALUE)
                CloseHandle(imp->m_file);
        }
        else if (m_deleteOnDestruction)
        {
            deleteDataPointer(m_rawdata);
        }

        delete imp;
    }

    void FileStream::deleteDataPointer(void* p) { MemPool::dealloc(p); }

#endif

} // namespace TwkUtil
