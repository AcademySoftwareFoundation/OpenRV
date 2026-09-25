//******************************************************************************
// Copyright (c) 2003 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#include <stl_ext/thread_group.h>

#include <assert.h>
#include <iostream>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cinttypes>
#include <errno.h>
#include <algorithm>
#include <chrono>

namespace stl_ext
{
    using namespace std;

    bool thread_group::DEBUG_ALL = false;
    std::mutex thread_group::DEBUG_MUTEX;

    thread_group::thread_group(int num_threads, int stack_mult, thread_api* api, const func_vector* funcs, const data_vector* datas)
    {
        if (api)
            m_api = *api;

        static std::atomic<int> next_group_id{0};
        m_group_id = ++next_group_id;
        m_debugging = DEBUG_ALL;

        pthread_attr_init(&m_thread_attr);
        pthread_attr_getstacksize(&m_thread_attr, &m_default_stack_size);

        if (num_threads)
            add_thread(num_threads, stack_mult, funcs, datas);
    }

    thread_group::~thread_group()
    {
        m_running = false;
        m_queue_cond.notify_all();
        for (auto& t : m_threads)
        {
            if (int err = m_api.join(t, NULL))
            {
                pthread_t thread = pthread_self();
                printf("~thread_group: %p -- join: %s", thread, strerror(err));
            }
        }
        pthread_attr_destroy(&m_thread_attr);
        debug("~thread_group: all threads joined");
    }

    void thread_group::debug(const char* fmt, ...)
    {
        if (!m_debugging)
            return;

        auto now = std::chrono::system_clock::now();
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
        auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count() % 1000000;

        std::lock_guard<std::mutex> lock(DEBUG_MUTEX);

        fprintf(stderr, "%" PRId64 " %06" PRId64, static_cast<int64_t>(seconds), static_cast<int64_t>(microseconds));
        fprintf(stderr, " [thread_group %d] ", m_group_id);
        va_list args;
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
        fprintf(stderr, "\n");
        fflush(stderr);
    }

    void thread_group::thread_main(WorkerState* state)
    {
        debug("worker %zu: started", state->worker_id);
        while (m_running)
        {
            Job job;
            {
                std::unique_lock<std::mutex> lock(m_queue_mutex);
                m_queue_cond.wait(lock, [this] { return !m_job_queue.empty() || !m_running; });
                if (!m_running && m_job_queue.empty())
                    break;
                job = m_job_queue.front();
                m_job_queue.pop();
            }

            // If this is a recall job, restore the original function/data for this worker
            if (job.recall)
            {
                job.func = state->original_func;
                job.data = state->original_data;
                // awaken_all_workers expects workers to have been created with
                // a function; a null original_func would decrement m_num_jobs
                // without doing any work, corrupting the job counter.
                assert(job.func);
                debug("worker %zu: recalling original func/data", state->worker_id);
            }

            // Execute the job if valid
            if (job.func)
            {
                debug("worker %zu: running func %p with data %p", state->worker_id, (void*)job.func, job.data);
                try
                {
                    job.func(job.data);
                }
                catch (...)
                {
                    debug("worker %zu: caught exception", state->worker_id);
                }
            }

            // Even though m_num_jobs is atomic, we lock m_finished_mutex here
            // to synchronize with threads waiting on m_finished_cond.
            // This ensures that decrement and notification are not separated,
            // avoiding missed wakeups or spurious waits.
            {
                std::lock_guard<std::mutex> lock(m_finished_mutex);
                m_num_jobs--;
                m_finished_cond.notify_all();
            }
            debug("worker %zu: job finished, jobs left: %d ", state->worker_id, m_num_jobs.load());
        }

        debug("worker %zu: returning", state->worker_id);
    }

    void thread_group::add_thread(int num_to_add, size_t stack_mult, const func_vector* funcs, const data_vector* datas)
    {
        debug("add_thread: resizing _threads to %d + %d, default_stack_size: %d, stack_mult: %d", m_threads.size(), num_to_add,
              m_default_stack_size, stack_mult);

        pthread_attr_setstacksize(&m_thread_attr, m_default_stack_size * stack_mult);

        if (funcs)
        {
            assert(funcs->size() == num_to_add);
            debug("add_thread: funcs size %d", funcs->size());
        }
        else
        {
            debug("add_thread: funcs is NULL");
        }

        for (int i = 0; i < num_to_add; ++i)
        {
            pthread_t thread;
            int ret;
            WorkerState* state = new WorkerState{};
            state->worker_id = m_threads.size();
            state->original_func = (funcs && funcs->size() > i) ? (*funcs)[i] : nullptr;
            state->original_data = (datas && datas->size() > i) ? (*datas)[i] : nullptr;
            state->group = this;

            ret = m_api.create(
                &thread, &m_thread_attr,
                [](void* arg) -> void*
                {
                    WorkerState* state = static_cast<WorkerState*>(arg);
                    state->group->thread_main(state);
                    delete state;
                    return nullptr;
                },
                state);

            if (ret != 0)
            {
                printf("add_thread: Error trying to create thread %d: %d", i, ret);
                abort();
            }

            m_threads.push_back(thread);
            m_num_threads++;
        }
        debug("add_thread: added %d threads, total now %d", num_to_add, m_num_threads);
    }

    bool thread_group::control_wait(bool allThreads, double seconds)
    {
        if (m_num_threads <= 0)
            return true;

        std::unique_lock<std::mutex> lock(m_finished_mutex);
        int current_num_jobs = m_num_jobs.load();

        // Predicate: all jobs finished (if allThreads), or any job finished
        auto predicate = [this, allThreads, current_num_jobs]() -> bool
        {
            if (allThreads)
                return m_num_jobs <= 0;
            else
                return m_num_jobs != current_num_jobs || m_num_jobs <= 0;
        };

        // Wait with timeout if specified, otherwise wait indefinitely
        if (seconds > 0.0)
        {
            auto timeout = std::chrono::steady_clock::now() + std::chrono::duration<double>(seconds);
            while (!predicate())
            {
                if (m_finished_cond.wait_until(lock, timeout) == std::cv_status::timeout)
                {
                    debug("control_wait: timed out waiting for %s job(s)", allThreads ? "all" : "any");
                    return false;
                }
            }
        }
        else
        {
            m_finished_cond.wait(lock, predicate);
        }
        return true;
    }

    bool thread_group::internal_dispatch(thread_function func, void* data, bool maybe)
    {
        bool dispatched = false;
        {
            std::lock_guard<std::mutex> lock(m_queue_mutex);
            if (!maybe || m_num_jobs < m_num_threads)
            {
                m_job_queue.push(Job{func, data, func ? false : true});
                m_num_jobs++;
                dispatched = true;
            }
        }
        if (dispatched)
        {
            debug("internal_dispatch: job dispatched");
            m_queue_cond.notify_one();
        }

        return dispatched;
    }

    void thread_group::awaken_all_workers()
    {
        {
            std::lock_guard<std::mutex> lock(m_queue_mutex);
            while (m_num_jobs < m_num_threads)
            {
                m_job_queue.push(Job{nullptr, nullptr, true});
                m_num_jobs++;
            }
            m_queue_cond.notify_all();
        }
        debug("awaken_all_workers: recall jobs enqueued");
    }
} // namespace stl_ext
