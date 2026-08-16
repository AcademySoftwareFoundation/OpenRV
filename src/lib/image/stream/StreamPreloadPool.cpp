//******************************************************************************
//
// Copyright (C) 2026 Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <stream/StreamPreloadPool.h>

#include <TwkUtil/EnvVar.h>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>

extern "C"
{
#include <libavformat/avio.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>
}

static ENVVAR_BOOL(evPrefetchDebug, "RV_STREAM_PREFETCH_DEBUG", false);
static ENVVAR_INT(evPrefetchThreads, "RV_STREAM_PREFETCH_THREADS", 4);

namespace
{

    //
    //  The shared protocol hands back a single block per read, so asking
    //  for more than the block size just leaves the tail of the buffer
    //  unused.
    //

    const int readChunkSize = 32 * 1024;

    //
    //  Composed first and written once: several workers log at the same
    //  time and a streamed message interleaves mid line.
    //

    void logLine(const std::string& message) { std::cerr << message + "\n" << std::flush; }

    double secondsSince(const std::chrono::steady_clock::time_point& start)
    {
        return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    }

    //
    //  Deliberately resolved here rather than as a default argument: the
    //  pool is a static member, and EnvVar::getValue initializes itself on
    //  demand, so reading it during static construction is safe.
    //

    size_t resolveThreadCount(size_t requested)
    {
        if (requested > 0)
            return requested;

        //
        //  An empty or non numeric value parses as 0, so fall back to the
        //  variable's own default rather than silently running single
        //  threaded.
        //

        const int configured = evPrefetchThreads.getValue();
        return configured > 0 ? size_t(configured) : size_t(evPrefetchThreads.getDefaultValue());
    }

    std::string errorString(int code)
    {
        char text[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_strerror(code, text, sizeof(text));
        return std::string(text);
    }

    std::string megabytes(long long bytes)
    {
        std::ostringstream str;
        str << std::fixed << std::setprecision(1) << (double(bytes) / (1024.0 * 1024.0)) << " MB";
        return str.str();
    }

} // namespace

StreamerPool::StreamerPool(size_t maxThreads)
    : m_maxThreads(resolveThreadCount(maxThreads))
    , m_abort(false)
    , m_completedCount(0)
    , m_failedCount(0)
    , m_abortedCount(0)
    , m_byteCount(0)
    , m_stopped(false)
{
}

StreamerPool::~StreamerPool() { shutdown(); }

int StreamerPool::interruptCallback(void* opaque)
{
    const StreamerPool* pool = static_cast<const StreamerPool*>(opaque);
    return pool->m_abort.load() ? 1 : 0;
}

void StreamerPool::enqueue(const std::string& url, const Options& options)
{
    bool queued = false;
    size_t pending = 0;

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_stopped)
            return;

        queued = m_known.insert(url).second;

        if (queued)
        {
            Job job;
            job.url = url;
            job.options = options;
            m_queue.push_back(job);
            pending = m_queue.size();

            //
            //  Start the scheduler on the first job rather than paying for
            //  an idle thread in sessions that never stream anything.
            //

            if (!m_scheduler.joinable())
            {
                m_scheduler = std::thread(&StreamerPool::schedulerLoop, this);
            }
        }
    }

    if (queued)
        m_wake.notify_all();

    //
    //  Logged outside the lock so a slow console cannot stall the workers.
    //

    if (evPrefetchDebug.getValue())
    {
        if (queued)
            logLine("INFO: prefetch queued " + url + " (pending " + std::to_string(pending) + ")");
        else
            logLine("INFO: prefetch already known, skipping " + url);
    }
}

void StreamerPool::schedulerLoop()
{
    for (;;)
    {
        std::unique_lock<std::mutex> lock(m_mutex);

        m_wake.wait(lock,
                    [this]
                    {
                        reapFinished();
                        return m_stopped || (!m_queue.empty() && m_workers.size() < m_maxThreads);
                    });

        if (m_stopped)
            return;

        while (!m_queue.empty() && m_workers.size() < m_maxThreads)
        {
            Job job = m_queue.front();
            m_queue.pop_front();
            m_workers.push_back(std::thread(&StreamerPool::workerFunc, this, job));
        }
    }
}

void StreamerPool::reapFinished()
{
    std::list<std::thread>::iterator i = m_workers.begin();

    while (i != m_workers.end())
    {
        if (m_finished.count(i->get_id()) > 0)
        {
            m_finished.erase(i->get_id());
            i->join();
            i = m_workers.erase(i);
        }
        else
        {
            ++i;
        }
    }
}

void StreamerPool::workerFunc(Job job)
{
    download(job);

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_known.erase(job.url);
        m_finished.insert(std::this_thread::get_id());
    }

    //
    //  Wake the scheduler so it can reap this thread and start whatever is
    //  next in the queue.
    //

    m_wake.notify_all();
}

void StreamerPool::download(const Job& job)
{
    AVDictionary* options = NULL;

    for (size_t i = 0; i < job.options.size(); i++)
    {
        av_dict_set(&options, job.options[i].first.c_str(), job.options[i].second.c_str(), 0);
    }

    AVIOInterruptCB interrupt;
    interrupt.callback = &StreamerPool::interruptCallback;
    interrupt.opaque = this;

    const std::chrono::steady_clock::time_point started = std::chrono::steady_clock::now();

    AVIOContext* context = NULL;
    const int status = avio_open2(&context, job.url.c_str(), AVIO_FLAG_READ, &interrupt, &options);

    av_dict_free(&options);

    if (status < 0 || context == NULL)
    {
        m_failedCount++;
        logLine("WARNING: prefetch could not open " + job.url);
        return;
    }

    //
    //  Worth reporting separately: on an https source this is the DNS
    //  lookup, the TCP connect and the TLS handshake, which is the latency
    //  the prefetch exists to move off the playback path.
    //

    const double openSeconds = secondsSince(started);
    const int64_t expected = avio_size(context);

    std::vector<unsigned char> buffer(readChunkSize);
    long long bytes = 0;
    int error = 0;

    while (!m_abort.load())
    {
        const int n = avio_read(context, &buffer[0], readChunkSize);

        if (n == 0 || n == AVERROR_EOF)
            break;

        //
        //  A short read is the end of the file, but a negative return is a
        //  real failure part way through. Distinguishing them matters: a
        //  download killed by an expired token or a rejected range request
        //  otherwise looks exactly like a complete one.
        //

        if (n < 0)
        {
            error = n;
            break;
        }

        bytes += n;
    }

    avio_closep(&context);

    const bool aborted = m_abort.load();
    const double totalSeconds = secondsSince(started);
    const bool truncated = (error == 0 && !aborted && expected > 0 && bytes < expected);

    m_byteCount += bytes;

    if (error != 0 || truncated)
        m_failedCount++;
    else if (aborted)
        m_abortedCount++;
    else
        m_completedCount++;

    std::ostringstream str;

    if (error != 0)
        str << "WARNING: prefetch read failed for ";
    else if (truncated)
        str << "WARNING: prefetch truncated for ";
    else if (aborted)
        str << "INFO: prefetch aborted ";
    else
        str << "INFO: prefetch done ";

    str << job.url << " (" << megabytes(bytes);

    if (expected > 0)
        str << " of " << megabytes(expected);

    str << " in " << std::fixed << std::setprecision(2) << totalSeconds << "s, open " << openSeconds << "s";

    if (totalSeconds > 0.0)
        str << ", " << megabytes(static_cast<long long>(double(bytes) / totalSeconds)) << "/s";

    if (error != 0)
        str << ", " << errorString(error);

    str << ")";

    //
    //  Failures are always worth reporting; the rest is noise unless the
    //  debug variable is set.
    //

    if (error != 0 || truncated || evPrefetchDebug.getValue())
        logLine(str.str());
}

void StreamerPool::shutdown()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_stopped)
            return;

        m_stopped = true;
        m_queue.clear();
        m_known.clear();
    }

    //
    //  Set after m_stopped so that a worker already inside avio_open2 or
    //  avio_read is interrupted rather than waited on.
    //

    m_abort.store(true);
    m_wake.notify_all();

    if (m_scheduler.joinable())
        m_scheduler.join();

    //
    //  The scheduler has exited and enqueue refuses to start anything new,
    //  so m_workers is stable from here on.
    //

    for (std::list<std::thread>::iterator i = m_workers.begin(); i != m_workers.end(); ++i)
    {
        if (i->joinable())
            i->join();
    }

    m_workers.clear();
    m_finished.clear();

    if (evPrefetchDebug.getValue())
    {
        std::ostringstream str;

        str << "INFO: prefetch summary (" << m_maxThreads << " threads): " << m_completedCount.load() << " completed, "
            << m_failedCount.load() << " failed, " << m_abortedCount.load() << " aborted, " << megabytes(m_byteCount.load()) << " total";

        logLine(str.str());
    }
}
