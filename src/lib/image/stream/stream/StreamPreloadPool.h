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
// Class to manage threads opened by the Preloader for streamed media ONLY
// to start caching raw media locally instead of fetching them over HTTPS.
// Threads are added to a queue, woken up by a CV every time a new file is
// requested to be cached or if a file finished being cached.
//

class StreamerPool
{
public:
    using Options = std::vector<std::pair<std::string, std::string>>;

    StreamerPool(const StreamerPool&) = delete;
    StreamerPool(const StreamerPool&&) = delete;
    StreamerPool& operator=(const StreamerPool&) = delete;
    StreamerPool& operator=(const StreamerPool&&) = delete;

    // Singleton
    static StreamerPool& getPool()
    {
        static StreamerPool pool;
        return pool;
    }

    // Wakes
    void enqueue(const std::string& url, const Options& options);

    // Prefetch a short demuxed window without decoding it. Window jobs use
    // one reserved worker and supersede older queued window jobs.
    void enqueueWindow(const std::string& url, const Options& options, double startSeconds, double durationSeconds);

    //
    //  Interrupt every in flight download, drop whatever is still queued
    //  and join the workers.
    //

    void shutdown();

private:
    StreamerPool();
    ~StreamerPool();

    struct Job
    {
        std::string url;
        Options options;
        bool window = false;
        double startSeconds = 0.0;
        double durationSeconds = 0.0;
    };

    //
    // Main loop that starts workers. Sleeps if can't add workers on iteration.
    //

    void schedulerLoop();
    void workerFunc(Job job);

    //
    // Handles FFMPEG API calls and fully downloads the raw media of the file
    //
    void download(const Job& job);
    void downloadWindow(const Job& job);

    //
    // Checks which workers have finished, waits for the threads to finish for sure,
    // and releases a worker slot if possible.
    // Called when waking up the scheduler.
    // Must be called with m_mutex
    //

    void reapFinished();

    //
    // Callback if connection gets interrupted, to not waste time if connection dies.
    //

    static int interruptCallback(void* opaque);

private:
    const size_t m_maxThreads;
    std::atomic<bool> m_abort;

    std::mutex m_mutex;
    std::condition_variable m_wake; // verify if a new worker can be started

    std::thread m_scheduler;
    std::list<std::thread> m_activeWorkers;
    std::set<std::thread::id> m_finished;
    size_t m_activeWindowWorkers = 0;

    std::deque<Job> m_queue; // pending jobs if threads maxed out
    bool m_stopped;          // scheduler finished
};

#endif // __stream__StreamPreloadPool__h__
