//******************************************************************************
//
// Copyright (C) 2026 Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __stream__StreamPreloadPool__h__
#define __stream__StreamPreloadPool__h__

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <list>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <utility>
#include <vector>

//
//  StreamerPool
//
//  Pulls whole media files into the FFmpeg "shared" protocol disk cache
//  ahead of playback, so the decoders never wait on the network for bytes
//  they are about to need.
//
//  A scheduler thread owns the queue and spawns at most maxThreads workers
//  at a time. It wakes whenever something is queued or a worker finishes,
//  reaps the threads that completed and starts as many new ones as the cap
//  allows.
//
//  Only the url and the protocol options are queued; the AVIOContext is
//  opened on the worker thread. Opening is itself a blocking network
//  operation, so keeping it off the calling thread is most of the point,
//  and a job that waits its turn costs nothing but memory.
//
//  Downloads are best effort. The pool is deliberately capped so that
//  prefetching does not compete for bandwidth with the reads the user is
//  actually waiting on.
//

class StreamerPool
{
public:
    //
    //  Protocol options (cookies, headers, ...) passed through to
    //  avio_open2 as an AVDictionary. Held by value so a queued job never
    //  outlives the caller's strings.
    //

    typedef std::vector<std::pair<std::string, std::string>> Options;

    //
    //  A maxThreads of 0 takes the count from RV_STREAM_PREFETCH_THREADS,
    //  falling back to 4.
    //

    explicit StreamerPool(size_t maxThreads = 0);
    ~StreamerPool();

    StreamerPool(const StreamerPool&) = delete;
    StreamerPool& operator=(const StreamerPool&) = delete;

    //
    //  Queue url for download and return immediately. Does nothing if the
    //  same url is already queued or in flight, or if the pool has been
    //  shut down.
    //

    void enqueue(const std::string& url, const Options& options);

    //
    //  Interrupt every in flight download, drop whatever is still queued
    //  and join the workers. Called by the destructor; safe to call twice.
    //

    void shutdown();

private:
    struct Job
    {
        std::string url;
        Options options;
    };

    void schedulerLoop();
    void workerFunc(Job job);
    void download(const Job& job);

    //
    //  Joins the workers that have run to completion. Must be called with
    //  m_mutex held.
    //

    void reapFinished();

    //
    //  Handed to avio_open2 so that an open or a read blocked on a dead
    //  connection can be unstuck at shutdown instead of waiting out the
    //  HTTP reconnect timeouts.
    //

    static int interruptCallback(void* opaque);

    const size_t m_maxThreads;
    std::atomic<bool> m_abort;

    //
    //  Reported at shutdown when RV_STREAM_PREFETCH_DEBUG is set.
    //

    std::atomic<int> m_completedCount;
    std::atomic<int> m_failedCount;
    std::atomic<int> m_abortedCount;
    std::atomic<long long> m_byteCount;

    std::mutex m_mutex;
    std::condition_variable m_wake;

    std::thread m_scheduler;
    std::list<std::thread> m_workers;
    std::set<std::thread::id> m_finished;

    std::deque<Job> m_queue;
    std::set<std::string> m_known;
    bool m_stopped;
};

#endif // __stream__StreamPreloadPool__h__
