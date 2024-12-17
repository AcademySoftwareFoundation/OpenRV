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
        typedef int (*thread_create)(pthread_t*, const pthread_attr_t*,
                                     pthread_main_function, void*);
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

        thread_group(int num_initial_threads = 0, int stack_size_mult = 1,
                     thread_api* = 0, const func_vector* = 0,
                     const data_vector* = 0);
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

        //
        //  Call dispatch for each work thread you want to dispatch. Its an
        //  error to call this if there are not available threads.
        //

        void dispatch(thread_function, void*);

        //
        //  Dispatch, but if all threads are busy returns false otherwise true
        //

        bool maybe_dispatch(thread_function, void*, bool async = false);

        //
        //  Use the function to add additional threads. Once a thread is added
        //  to the thread group, it exists until the thread group is destroyed.
        //
        //  To lock the func/data to particular threads, send in appropriately
        //  sized vectors here, then send 0/0 in later dispatch calls.
        //

        void add_thread(int num_threads = 1, size_t stack_mult = 1,
                        const func_vector* = 0, const data_vector* = 0);

        //
        //  Number of worker threads in the thread group.
        //

        size_t num_threads() const { return _num_threads; }

        //
        //  set debugging messages on/off
        //

        void debug(bool b) { _debugging = b; }

        static void debug_all(bool b) { _debug_all = b; }

        //
        //  Cancel a thread if its running. USE WITH CAUTION
        //

        void cancel(pthread_t);

        //
        //  Pthreads wrappers
        //

        static void lock(pthread_mutex_t&);
        static void unlock(pthread_mutex_t&);
        static void wait_cond(pthread_cond_t&, pthread_mutex_t&);
        static bool wait_cond_time(pthread_cond_t&, pthread_mutex_t&,
                                   size_t usec);
        static void signal(pthread_cond_t&);
        static void broadcast(pthread_cond_t&);

        void debug(const char*, ...);

        //----------------------------------------------------------------------
        //
        //  Worker thread API
        //

        //
        //  Restart any waiting workers, using the func/data pair that
        //  was provided to them originally via dispatch or
        //  maybe_dispatch.  This is a NOOP if no workers are waiting,
        //  or if none have been dispatched.
        //

        void awaken_all_workers();

    private:
        static void* thread_main(void*);
        static void thread_cleanup(void*);

        void worker_wait(); // worker thread API
        void worker_jump();
        void worker_cleanup();
        void release_control();
        void release_worker();

        typedef std::vector<pthread_t> thread_vector;

        struct thread_package
        {
            // thread_package () {}
            // thread_package (thread_group* g, thread_function f, void* d) :
            // group(g), func(f), data(d) {}

            thread_group* group;
            thread_function func;
            void* data;
        };

        typedef std::vector<thread_package> package_vector;

        bool running() { return _running; }

    private:
        thread_api _api;
        pthread_mutex_t _wait_mutex;
        pthread_cond_t _wait_cond;

        pthread_mutex_t _worker_mutex;
        pthread_cond_t _worker_cond;

        pthread_mutex_t _control_mutex;
        pthread_cond_t _control_cond;

        pthread_mutex_t _debug_mutex;

        pthread_attr_t _thread_attr;

        bool _wait_flag;
        thread_vector _threads;
        package_vector _packages;

        int _num_finished;
        int _num_threads;
        thread_function _func;
        void* _data;
        bool _recall;
        bool _debugging;
        bool _running;
        pthread_t _join_thread;

        bool _dispatching;
        size_t _default_stack_size;
        static bool _debug_all;

        pthread_key_t _funcKey;
        pthread_key_t _dataKey;
    };

} // namespace stl_ext

#endif // __stl_ext__thread_group__h__
