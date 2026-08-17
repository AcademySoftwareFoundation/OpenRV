//******************************************************************************
//
// Copyright (C) 2026 Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <stream/StreamPreloadPool.h>

#include <TwkUtil/EnvVar.h>

extern "C"
{
#include <libavformat/avio.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>
}

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

} // namespace

StreamerPool::StreamerPool(size_t maxThreads)
    : m_maxThreads(resolveThreadCount(maxThreads))
    , m_abort(false)
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

    AVIOContext* context = NULL;
    const int status = avio_open2(&context, job.url.c_str(), AVIO_FLAG_READ, &interrupt, &options);

    av_dict_free(&options);

    if (status < 0 || context == NULL)
    {
        return;
    }

    std::vector<unsigned char> buffer(readChunkSize);

    while (!m_abort.load())
    {
        const int n = avio_read(context, &buffer[0], readChunkSize);

        if (n == 0 || n == AVERROR_EOF)
            break;

        if (n < 0)
            break;
    }

    avio_closep(&context);
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
}
