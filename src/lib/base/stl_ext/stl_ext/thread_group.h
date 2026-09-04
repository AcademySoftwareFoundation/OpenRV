//******************************************************************************
// Copyright (c) 2003 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************
#ifndef __stl_ext__thread_group__h__
#define __stl_ext__thread_group__h__
#include <pthread.h>
#include <vector>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <stl_ext/stl_ext_config.h>

namespace stl_ext
{

    //
    //  class thread_group
    //
    //  A collection of pthreads. This class organizes the pthreads into a
    //  bundle which are dispatched by a constrol thread.
    //

    class STL_EXT_EXPORT thread_group
    {
    public:
        //
        //  Types
        //

        typedef void (*thread_function)(void*);
        typedef std::vector<thread_function> func_vector;
        typedef std::vector<void*> data_vector;
        typedef void* (*pthread_main_function)(void*);
        typedef int (*thread_create)(pthread_t*, const pthread_attr_t*, pthread_main_function, void*);
        typedef int (*thread_join)(pthread_t, void**);
        typedef int (*thread_detach)(pthread_t);

        //
        //  You can supply an alternative thread API by passing one of
        //  these to the constructor. This is currently used for programs
        //  that link against the Boehm GC and what to use thread_group to
        //  control GCed pthreads. (The Boehm GC overrides these pthread
        //  functions in order to handle multithreaded GC).
        //
        //  The default thread API is vanilla pthreads.
        //

        struct thread_api
        {
            thread_api()
                : create(pthread_create)
                , join(pthread_join)
                , detach(pthread_detach)
            {
            }

            thread_api(thread_create c, thread_join j, thread_detach d)
                : create(c)
                , join(j)
                , detach(d)
            {
            }

            thread_create create;
            thread_join join;
            thread_detach detach;
        };

        //
        //  Create the group with an initial number of threads or call
        //  add_thread later.
        //
        //  To lock the func/data to particular threads, send in appropriately
        //  sized vectors here, then send 0/0 in later dispatch calls.
        //

        thread_group(int num_initial_threads = 0, int stack_size_mult = 1, thread_api* = 0, const func_vector* = 0, const data_vector* = 0);
        ~thread_group();

        //----------------------------------------------------------------------
        //
        //  Control thread API
        //  These function should only be called by the control thread.
        //

        //
        //  Main thread calls control_wait() when the worker threads are doing
        //  something. The function will return when the workers are done
        //

        bool control_wait(bool allThreads = true,
                          double timeout_seconds = 0.0); // control thread API

        //  Queue a job to be executed by a worker thread asynchronously. The
        //  job will be placed in the work queue and executed by the next
        //  available worker. If all workers are busy, the job will wait
        //  in the queue until a worker becomes available.
        //

        void dispatch(thread_function func, void* data) { internal_dispatch(func, data, false); }

        //
        //  Dispatch, but if all threads are busy returns false otherwise true
        //

        bool maybe_dispatch(thread_function func, void* data) { return internal_dispatch(func, data, true); }

        //
        //  Use the function to add additional threads. Once a thread is added
        //  to the thread group, it exists until the thread group is destroyed.
        //
        //  To lock the func/data to particular threads, send in appropriately
        //  sized vectors here, then send 0/0 in later dispatch calls.
        //

        void add_thread(int num_threads = 1, size_t stack_mult = 1, const func_vector* = 0, const data_vector* = 0);

        //
        //  Number of worker threads in the thread group.
        //

        size_t num_threads() const { return m_num_threads; }

        //
        //  set debugging messages on/off
        //

        void debug(bool b) { m_debugging = b; }

        static void debug_all(bool b) { DEBUG_ALL = b; }

        void debug(const char*, ...);

        //
        //  Restart any waiting workers, using the func/data pair that
        //  was provided to them originally via dispatch or
        //  maybe_dispatch.  This is a NOOP if no workers are waiting,
        //  or if none have been dispatched.
        //

        void awaken_all_workers();

    private:
        struct WorkerState
        {
            size_t worker_id = 0;
            thread_group* group = nullptr;
            thread_function original_func = nullptr; // Function to recall
            void* original_data = nullptr;           // Data to recall
        };

        struct Job
        {
            thread_function func; // Function to execute
            void* data;           // Data to pass to the function
            bool recall;          // If true, use the thread's original func/data
        };

        //
        // Main loop for each worker thread.
        //

        void thread_main(WorkerState* state);

        bool internal_dispatch(thread_function func, void* data, bool maybe);

    private:
        thread_api m_api;
        pthread_attr_t m_thread_attr;
        std::vector<pthread_t> m_threads;
        size_t m_default_stack_size = 0;

        // Debug logging
        static bool DEBUG_ALL;
        static std::mutex DEBUG_MUTEX;
        int m_group_id;
        bool m_debugging = false;

        // Work queue and synchronization
        int m_num_threads = 0;
        std::atomic<int> m_num_jobs{0};
        std::atomic<bool> m_running{true};
        std::queue<Job> m_job_queue;
        std::mutex m_queue_mutex;
        std::condition_variable m_queue_cond;

        // For control_wait synchronization
        std::mutex m_finished_mutex;
        std::condition_variable m_finished_cond;
    };

} // namespace stl_ext

#endif // __stl_ext__thread_group__h__
