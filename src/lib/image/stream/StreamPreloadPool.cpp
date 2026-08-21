//******************************************************************************
//
// Copyright (C) 2026 Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <cstddef>
#include <stream/StreamPreloadPool.h>

#include <TwkUtil/EnvVar.h>

#include <algorithm>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>
#include <libavutil/mathematics.h>
}

static ENVVAR_INT(evPrefetchThreads, "RV_STREAM_PREFETCH_THREADS", 4);

namespace
{
    size_t maxStreamerThreads()
    {
        const int configured = evPrefetchThreads.getValue();
        return configured > 0 ? static_cast<size_t>(configured) : static_cast<size_t>(evPrefetchThreads.getDefaultValue());
    }
} // namespace

StreamerPool::StreamerPool()
    : m_maxThreads(maxStreamerThreads())
    , m_abort(false)
    , m_stopped(false)
{
}

StreamerPool::~StreamerPool() { shutdown(); }

int StreamerPool::interruptCallback(void* opaque)
{
    const auto* const pool = static_cast<const StreamerPool*>(opaque);
    return pool->m_abort.load() ? 1 : 0;
}

void StreamerPool::enqueue(const std::string& url, const Options& options)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_stopped)
        {
            return;
        }

        Job job;
        job.url = url;
        job.options = options;
        m_queue.push_back(job);

        // Start the scheduler loop on a new thread the first time we add a file
        if (!m_scheduler.joinable())
        {
            m_scheduler = std::thread(&StreamerPool::schedulerLoop, this);
        }
    }

    m_wake.notify_all();
}

void StreamerPool::enqueueWindow(const std::string& url, const Options& options, double startSeconds, double durationSeconds)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_stopped)
        {
            return;
        }

        m_queue.erase(std::remove_if(m_queue.begin(), m_queue.end(), [](const Job& job) { return job.window; }), m_queue.end());

        Job job;
        job.url = url;
        job.options = options;
        job.window = true;
        job.startSeconds = startSeconds;
        job.durationSeconds = durationSeconds;
        m_queue.push_front(job);

        if (!m_scheduler.joinable())
        {
            m_scheduler = std::thread(&StreamerPool::schedulerLoop, this);
        }
    }

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
                        return m_stopped
                               || (!m_queue.empty()
                                   && (m_queue.front().window ? m_activeWindowWorkers == 0 : m_activeWorkers.size() < m_maxThreads));
                    });

        if (m_stopped)
        {
            return;
        }

        while (!m_queue.empty())
        {
            if (m_queue.front().window)
            {
                if (m_activeWindowWorkers != 0)
                    break;
            }
            else if (m_activeWorkers.size() >= m_maxThreads)
            {
                break;
            }

            Job job = m_queue.front();
            m_queue.pop_front();
            if (job.window)
                ++m_activeWindowWorkers;
            m_activeWorkers.emplace_back(&StreamerPool::workerFunc, this, job);
        }
    }
}

void StreamerPool::reapFinished()
{
    auto worker_it = m_activeWorkers.begin();

    while (worker_it != m_activeWorkers.end())
    {
        if (m_finished.count(worker_it->get_id()) > 0)
        {
            m_finished.erase(worker_it->get_id());
            worker_it->join();
            worker_it = m_activeWorkers.erase(worker_it);
        }
        else
        {
            ++worker_it;
        }
    }
}

void StreamerPool::workerFunc(Job job)
{
    if (job.window)
        downloadWindow(job);
    else
        download(job);

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (job.window)
            --m_activeWindowWorkers;
        m_finished.insert(std::this_thread::get_id());
    }

    m_wake.notify_all();
}

void StreamerPool::download(const Job& job)
{
    // 32 KB
    const int readChunkSize = 32 * 1024;

    AVDictionary* options = nullptr;

    // Add the options for FFMPEG reading over HTTPS
    for (const auto& [key, value] : job.options)
    {
        av_dict_set(&options, key.c_str(), value.c_str(), 0);
    }

    AVIOInterruptCB interrupt;
    interrupt.callback = &StreamerPool::interruptCallback;
    interrupt.opaque = this;

    AVIOContext* context = nullptr;
    const int status = avio_open2(&context, job.url.c_str(), AVIO_FLAG_READ, &interrupt, &options);

    av_dict_free(&options);

    if (status < 0 || context == nullptr)
    {
        return;
    }

    std::vector<unsigned char> buffer(readChunkSize);

    // Drain all the bytes from the stream to download the raw media
    while (!m_abort.load())
    {
        const int ret = avio_read(context, buffer.data(), readChunkSize);

        if (ret == 0 || ret == AVERROR_EOF)
        {
            break;
        }

        if (ret < 0)
        {
            break;
        }
    }

    avio_closep(&context);
}

void StreamerPool::downloadWindow(const Job& job)
{
    AVDictionary* options = nullptr;
    for (const auto& [key, value] : job.options)
    {
        av_dict_set(&options, key.c_str(), value.c_str(), 0);
    }

    AVFormatContext* context = avformat_alloc_context();
    if (context == nullptr)
    {
        av_dict_free(&options);
        return;
    }

    context->interrupt_callback.callback = &StreamerPool::interruptCallback;
    context->interrupt_callback.opaque = this;

    if (avformat_open_input(&context, job.url.c_str(), nullptr, &options) < 0)
    {
        av_dict_free(&options);
        avformat_free_context(context);
        return;
    }
    av_dict_free(&options);

    if (avformat_find_stream_info(context, nullptr) >= 0)
    {
        const int64_t mediaStart = context->start_time == AV_NOPTS_VALUE ? 0 : context->start_time;
        const int64_t start = mediaStart + static_cast<int64_t>(job.startSeconds * AV_TIME_BASE);
        const int64_t end = start + static_cast<int64_t>(job.durationSeconds * AV_TIME_BASE);

        if (avformat_seek_file(context, -1, INT64_MIN, start, INT64_MAX, AVSEEK_FLAG_BACKWARD) >= 0)
        {
            AVPacket* packet = av_packet_alloc();
            while (packet != nullptr && !m_abort.load() && av_read_frame(context, packet) >= 0)
            {
                const int64_t timestamp = packet->pts != AV_NOPTS_VALUE ? packet->pts : packet->dts;
                const int64_t packetTime = timestamp == AV_NOPTS_VALUE
                                               ? start
                                               : av_rescale_q(timestamp, context->streams[packet->stream_index]->time_base, AV_TIME_BASE_Q);
                av_packet_unref(packet);
                if (packetTime >= end)
                    break;
            }
            av_packet_free(&packet);
        }
    }

    avformat_close_input(&context);
}

void StreamerPool::shutdown()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_stopped)
        {
            return;
        }

        m_stopped = true;
        m_queue.clear();
    }

    m_abort.store(true);
    m_wake.notify_all();

    if (m_scheduler.joinable())
    {
        m_scheduler.join();
    }

    for (auto& worker : m_activeWorkers)
    {
        if (worker.joinable())
        {
            worker.join();
        }
    }

    m_activeWorkers.clear();
    m_finished.clear();

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_abort.store(false);
        m_stopped = false;
    }
}
