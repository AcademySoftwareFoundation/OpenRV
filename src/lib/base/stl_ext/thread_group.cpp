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
#include <errno.h>
#include <algorithm>

#ifdef PLATFORM_WINDOWS
#include <time.h>
#include <winsock.h>
#else
#include <sys/time.h>
#include <sys/resource.h>
#endif

namespace stl_ext
{
    using namespace std;

    bool thread_group::_debug_all = false;

#ifdef PLATFORM_WINDOWS
    static int currentTime(struct timeval* tv)
    {
        union
        {
            // AJG - this used to be a LONG_LONG
            __int64 ns100; // time since 1 Jan 1601 in 100ns units
            FILETIME ft;
        } now;

        GetSystemTimeAsFileTime(&now.ft);
        tv->tv_usec = (long)((now.ns100 / 10LL) % 1000000LL);
        tv->tv_sec = (long)((now.ns100 - 116444736000000000LL) / 10000000LL);
        return (0);
    }
#endif

#if defined(PLATFORM_APPLE_MACH_BSD)
    static int currentTime(struct timeval* tp) { return gettimeofday(tp, 0); }
#endif

    thread_group::thread_group(int num_threads, int stack_mult, thread_api* api,
                               const func_vector* funcs,
                               const data_vector* datas)
        : _num_finished(0)
        , _num_threads(0)
        , _data(0)
        , _func(0)
        , _recall(false)
        , _debugging(false)
        , _running(true)
        , _wait_flag(true)
        , _dispatching(false)
        , _default_stack_size(0)
        , _api()
    {
        if (api)
            _api = *api;
        _debugging = _debug_all;
        pthread_mutex_init(&_wait_mutex, 0);
        pthread_mutex_init(&_worker_mutex, 0);
        pthread_mutex_init(&_control_mutex, 0);
        pthread_mutex_init(&_debug_mutex, 0);

        pthread_cond_init(&_wait_cond, 0);
        pthread_cond_init(&_worker_cond, 0);
        pthread_cond_init(&_control_cond, 0);

        memset(&_thread_attr, 0, sizeof(pthread_attr_t));
        pthread_attr_init(&_thread_attr);
        pthread_attr_getstacksize(&_thread_attr, &_default_stack_size);

        pthread_key_create(&_funcKey, 0);
        pthread_key_create(&_dataKey, 0);

        add_thread(num_threads, stack_mult, funcs, datas);
    }

    static void noop(void*) {}

    thread_group::~thread_group()
    {
        control_wait();
        _running = false;

        debug("~thread_group: control shutting down threads");

        while (_num_threads > 0)
        {
            debug("~thread_group: control dispatch with noop, num_threads = %d",
                  _num_threads);
            dispatch(noop, 0);
            debug("~thread_group: control joining worker: %d", _num_threads);

            // void *retval = 0;
            lock(_wait_mutex);
            unlock(_wait_mutex);
            lock(_worker_mutex);
            unlock(_worker_mutex);
            lock(_control_mutex);
            unlock(_control_mutex);

            //
            // We have to do this memcpy to a size_t type
            // do guard against a zero value for _join_thread.
            //
            size_t threadID = 0;
            memcpy(&threadID, &_join_thread,
                   std::min(sizeof(threadID), sizeof(_join_thread)));
            if (threadID)
            {
                if (int err = _api.join(_join_thread, NULL))
                {
                    pthread_t thread = pthread_self();
                    printf("~thread_group: %p -- join: %s", thread,
                           strerror(err));
                }
            }
            /* AJG - This WILL break something */
            //_join_thread = 0;
            debug("~thread_group: control successfully joined with worker");
        }

        debug("~thread_group: control destroying state");

        //
        //  need to wait until its ok to destroy all of this stuff.
        //
        debug("~thread_group: deleting keys");
        pthread_key_delete(_funcKey);
        pthread_key_delete(_dataKey);

        debug("~thread_group: destroying mutexes");
        pthread_mutex_destroy(&_wait_mutex);
        pthread_mutex_destroy(&_worker_mutex);
        pthread_mutex_destroy(&_control_mutex);

        debug("~thread_group: destroying _wait_cond");
        pthread_cond_destroy(&_wait_cond);
        debug("~thread_group: destroying _worker_cond");
        pthread_cond_destroy(&_worker_cond);
        debug("~thread_group: destroying _control_cond");
        pthread_cond_destroy(&_control_cond);

        debug("~thread_group: destroying attr");
        pthread_attr_destroy(&_thread_attr);

        debug("~thread_group: destroying debug");
        pthread_mutex_destroy(&_debug_mutex);
    }

    void thread_group::debug(const char* c, ...)
    {
        if (_debugging)
        {
            va_list ap;
            va_start(ap, c);
            lock(_debug_mutex);
            pthread_t thread = pthread_self();
            int worker_num = -1;

            for (int i = 0; i < _threads.size(); i++)
            {
                if (pthread_equal(thread, _threads[i]))
                {
                    worker_num = i;
                    break;
                }
            }

#ifndef PLATFORM_LINUX
            struct timeval tv;
            int t = currentTime(&tv);

            fprintf(stderr, "%d %d %p/%p/%d/%lu: ", (int)tv.tv_sec,
                    (int)tv.tv_usec, this, thread, worker_num + 1,
                    _threads.size());
#endif
            vfprintf(stderr, c, ap);
            fprintf(stderr, "\n");
            fflush(stderr);
            unlock(_debug_mutex);
        }
    }

    //
    //  This is the "main" function of each of the worker threads. The thread
    //  is not marked detached until its about to exit
    //

    void thread_group::thread_cleanup(void* arg)
    {
        thread_group* group = (thread_group*)arg;
        group->debug("worker clean up after cancellation");
        group->worker_cleanup();
    }

    void* thread_group::thread_main(void* arg)
    {
        thread_package pack = *((thread_package*)arg);
        pack.group->debug("thread_main: worker in main, thread_package: %p",
                          arg);

#ifdef PLATFORM_APPLE_MACH_BSD
        //
        //  Set the initial name to thread_group to indicate that the thread
        //  has not yet been used.
        //

        pthread_setname_np("thread_group");
#endif
        pack.group->debug("thread_main: pthread_setspecific: %p %p %p %p",
                          pack.group->_funcKey, (void*)pack.func,
                          pack.group->_dataKey, pack.data);

        pthread_setspecific(pack.group->_funcKey, (void*)pack.func);
        pthread_setspecific(pack.group->_dataKey, pack.data);

        // pthread_cleanup_push(thread_cleanup, arg);

        while (pack.group->running())
        {
            pack.group->debug("thread_main: running");
            pack.group->worker_wait();
            pack.group->worker_jump();
        }

        // pthread_cleanup_pop(1);

        // pack.group->debug("worker returning");
        pack.group->debug("thread_main: worker returning");
        return 0;
    }

    void thread_group::add_thread(int num_to_add, size_t stack_mult,
                                  const func_vector* funcs,
                                  const data_vector* datas)
    {
        debug("add_thread");
        if (num_to_add)
        {
            debug("add_thread: resizing _threads and _packages to %d + %d, "
                  "_default_stack_size: %d, stack_mult: %d",
                  _threads.size(), num_to_add, _default_stack_size, stack_mult);
            size_t s = _threads.size();
            _threads.resize(s + num_to_add);
            _packages.resize(_threads.size());
            pthread_attr_setstacksize(&_thread_attr,
                                      _default_stack_size * stack_mult);

            if (funcs)
                assert(funcs->size() == num_to_add);
            if (funcs)
            {
                debug("add_thread: funcs size %d", funcs->size());
            }
            else
            {
                debug("add_thread: funcs is NULL");
            }
            for (int i = s; i < _threads.size(); i++)
            {
                thread_package& pack = _packages[i];
                pack.group = this;
                pack.func = (funcs) ? (*funcs)[i - s] : 0;
                pack.data = (funcs) ? (*datas)[i - s] : 0;

                memset(&_threads[i], 0, sizeof(pthread_t));
                if (int err = _api.create(&_threads[i], &_thread_attr,
                                          thread_main, &pack))
                {
                    printf("add_thread: Error trying to create thread %d: %d",
                           i, err);
                    abort();
                }
                _num_threads++;
                debug("add_thread: added worker #%d", _num_threads);
            }
            debug("add_thread: control_wait");
            control_wait();
            debug("add_thread done.");
        }
    }

    void thread_group::lock(pthread_mutex_t& mutex)
    {
        if (int err = pthread_mutex_lock(&mutex))
        {
            pthread_t thread = pthread_self();
            printf("%p -- unable to lock: %s", thread, strerror(err));
            fflush(stdout);
        }
    }

    void thread_group::unlock(pthread_mutex_t& mutex)
    {
        if (int err = pthread_mutex_unlock(&mutex))
        {
            pthread_t thread = pthread_self();
            printf("%p -- unable to unlock: %s", thread, strerror(err));
            fflush(stdout);
        }
    }

    void thread_group::signal(pthread_cond_t& cond)
    {
        if (int err = pthread_cond_signal(&cond))
        {
            pthread_t thread = pthread_self();
            printf("%p -- signal: %s", thread, strerror(err));
            fflush(stdout);
        }
    }

    void thread_group::broadcast(pthread_cond_t& cond)
    {
        if (int err = pthread_cond_broadcast(&cond))
        {
            pthread_t thread = pthread_self();
            printf("%p -- broadcast: %s\n", thread, strerror(err));
            fflush(stdout);
        }
    }

    void thread_group::wait_cond(pthread_cond_t& cond, pthread_mutex_t& mutex)
    {
        if (int err = pthread_cond_wait(&cond, &mutex))
        {
            pthread_t thread = pthread_self();
            printf("%p -- cond_wait: %s\n", thread, strerror(err));
            fflush(stdout);
        }
    }

    bool thread_group::wait_cond_time(pthread_cond_t& cond,
                                      pthread_mutex_t& mutex, size_t usec)
    {
        timespec ts;

#ifdef PLATFORM_LINUX
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += usec / 1000000;
        size_t r = (usec % 1000000) * 1000 + ts.tv_nsec;

        if (r > 1000000000)
        {
            ts.tv_sec += r / 1000000000;
            ts.tv_nsec = r % 1000000000;
        }

#else
        struct timeval tv;
        currentTime(&tv);

        ts.tv_sec = tv.tv_sec;
        size_t us = usec + tv.tv_usec;

        if (us > 1000000)
        {
            ts.tv_sec += us / 1000000;
            ts.tv_nsec = (us % 1000000) * 1000;
        }
        else
        {
            ts.tv_nsec = us * 1000;
        }
#endif

        if (int err = pthread_cond_timedwait(&cond, &mutex, &ts))
        {
            if (err != ETIMEDOUT)
            {
                pthread_t thread = pthread_self();
                printf("ERROR: %p -- cond_wait_timed: %s\n", thread,
                       strerror(err));
                fflush(stdout);
            }
            //  fprintf (stderr, "err %d %s\n", err, strerror(err));

            return false;
        }

        return true;
    }

    void thread_group::release_control()
    {
        lock(_wait_mutex);
        // debug("worker releasing control thread");
        debug("release_control: _wait_mutex locked, signaling _wait_cond");
        signal(_wait_cond);
        unlock(_wait_mutex);
        debug("release_control: _wait_mutex unlocked");
    }

    void thread_group::release_worker()
    {
        lock(_worker_mutex);
        // debug("control releasing a worker thread");
        debug("release_worker: _worker_mutex locked, signaling _worker_cond");
        _wait_flag = false;
        signal(_worker_cond);
        unlock(_worker_mutex);
        debug("release_worker: _worker_mutex unlocked");
    }

    bool thread_group::control_wait(bool allThreads, double seconds)
    {
        bool timedout = false;

        if (_num_finished != _num_threads)
        {
            lock(_wait_mutex);
            debug("control_wait: _wait_mutex locked");

            //
            //  POSIX says that you can get a spurious wake up from a
            //  condition variable. We need to keep checking in that case.
            //

            int num_finished = _num_finished;

            if (allThreads)
            {
                while (_num_finished != _num_threads)
                {
                    debug("control_wait: control waiting on %d threads, "
                          "timeout %g seconds",
                          (_num_threads - _num_finished), seconds);

                    if (seconds)
                    {
                        timedout = !wait_cond_time(_wait_cond, _wait_mutex,
                                                   size_t(seconds * 1000000.0));
                        if (timedout)
                        {
                            debug(
                                "control_wait: control wait timed out, %d "
                                "threads remaining, _wait_cond and _wait_mutex",
                                (_num_threads - _num_finished));
                            break;
                        }
                    }
                    else
                    {
                        wait_cond(_wait_cond, _wait_mutex);
                        debug("control_wait: control wait_cond returned, "
                              "_wait_cond and _wait_mutex");
                    }
                }
            }
            else
            {
                while (_num_finished == num_finished)
                {
                    debug("control_wait: control waiting on any thread, "
                          "timeout %g seconds, _wait_cond and _wait_mutex",
                          seconds);

                    if (seconds)
                    {
                        timedout = !wait_cond_time(_wait_cond, _wait_mutex,
                                                   size_t(seconds * 1000000.0));
                        if (timedout)
                        {
                            debug("control_wait: control wait on any thread "
                                  "timed out");
                            break;
                        }
                    }
                    else
                    {
                        wait_cond(_wait_cond, _wait_mutex);
                    }
                }
            }

            unlock(_wait_mutex);
            debug("control_wait: control done waiting, _wait_mutex unlocked");
        }
        else
        {
            debug("control did not need to wait");
        }

        return !timedout;
    }

    void thread_group::cancel(pthread_t th)
    {
        lock(_worker_mutex);
        int f = _num_finished;
        unlock(_worker_mutex);
        if (f != _num_threads)
            pthread_cancel(th);
    }

    void thread_group::worker_wait()
    {
        lock(_worker_mutex);
        debug("worker_wait: _worker_mutex is locked");
        ++_num_finished;

        if (_num_finished == _num_threads)
        {
            debug("worker_wait: workers are all waiting");
            release_control();
        }
        else
        {
            if (_num_finished < 0 || _num_threads < 0)
            {
                _wait_flag = true;
                unlock(_worker_mutex);
                debug("worker_wait: ERROR: _num_finished %d _num_threads %d",
                      _num_finished, _num_threads);
                return;
            }
            debug("worker_wait: %d workers left", _num_threads - _num_finished);
        }

        while (_wait_flag)
        {
            debug("worker_wait: worker waiting on _worker_cond with "
                  "_worker_mutex");
            wait_cond(_worker_cond, _worker_mutex);
            debug("worker_wait: worker awoke");
        }

        _wait_flag = true;
        _num_finished--;
        unlock(_worker_mutex);

        debug("worker_wait: worker done waiting, _worker_mutex unlocked");
    }

    void thread_group::worker_jump()
    {
        if (running())
        {
            lock(_control_mutex);
            debug("worker_jump: worker loading jump point, recall %d", _recall);

            if (_recall)
            {
                _recall = false;

                thread_function savedFunc =
                    (thread_function)pthread_getspecific(_funcKey);
                void* savedData = pthread_getspecific(_dataKey);

                if (savedFunc)
                {
                    //  We want to just awaken this thread, using the func
                    //  and data that were originally given it by dispatch.
                    //  So recall them from thread-specific data.
                    //
                    _func = savedFunc;
                    _data = savedData;
                    debug("worker_jump: re-using data %x", _data);
                }
                else
                {
                    //  An attempt was made to awaken this thread before
                    //  it was run the first time, so go back to sleep.
                    //
                    debug("worker_jump: worker returning to wait");
                    signal(_control_cond);
                    unlock(_control_mutex);
                    return;
                }
            }
            else
            {
                //  This is the first time we've dispatched this thread, so
                //  store func/data for future awakenings.
                //
                pthread_setspecific(_funcKey, (void*)_func);
                pthread_setspecific(_dataKey, _data);
                debug("worker_jump: setting data %x", _data);
            }

            thread_function func = _func;
            void* data = _data;
            // assert(_dispatching);
            assert(_func);
            _func = 0;
            _data = 0;

            debug("worker_jump: worker signaling control");
            signal(_control_cond);
            unlock(_control_mutex);

            debug("worker_jump: worker jumping");

            try
            {
                (*func)(data);
            }
            catch (...)
            {
                debug("worker caught C++ exception");
            }

            debug("worker_jump: worker finished, returning to wait");
        }
        else
        {
            lock(_control_mutex);
            _num_threads--;
            _func = 0;
            _join_thread = pthread_self();
            debug("worker_jump: worker decremented, num_threads is now %d. "
                  "_control_mutex locked",
                  _num_threads);
            debug("worker_jump: worker signaling _control_cond");
            signal(_control_cond);
            unlock(_control_mutex);
            debug("worker_jump: worker exiting, _control_mutex unlocked");
        }
    }

    void thread_group::worker_cleanup()
    {
        lock(_worker_mutex);
        _wait_flag = false;
        unlock(_worker_mutex);
        worker_wait();

        lock(_control_mutex);
        _num_threads--;
        _join_thread = pthread_self();
        debug("worker_cleanup: worker decremented, num_threads is now %d, "
              "_control_mutex locked",
              _num_threads);
        debug("worker_cleanup: worker signaling _control_cond");
        signal(_control_cond);
        unlock(_control_mutex);
        debug("worker_cleanup: worker finishing, _control_mutex unlocked");
    }

    void thread_group::dispatch(thread_group::thread_function func, void* data)
    {
        debug("dispatch: control dispatching worker, func %p", func);

        if (_num_finished == 0 && running())
        {
            debug("dispatch: control: thread_group: can't dispatch: no idle "
                  "workers");
            //
            //  There's nothing to do, no need to abort() ...
            //  abort();
            //
            return;
        }

        lock(_control_mutex);

        _dispatching = true;
        assert(_func == 0);

        if (!func)
        {
            //
            //  This will tell the worker to recall it's original func/data.
            //
            debug("dispatch: worker recall original stuff %p %p", _func, _data);
            _recall = true;

            //  We need to set _func to something, so that the wait
            //  loop below functions as intended.
            //
            _func = (thread_function)0xdeadc0de;
            _data = 0;
        }
        else
        {
            _recall = false;
            _func = func;
            _data = data;
        }
        release_worker();
        debug("dispatch: control waiting for worker to load");

        while (_func)
        {
            wait_cond(_control_cond, _control_mutex);
        }

        debug("dispatch: control done waiting for worker to dispatch");
        _dispatching = false;
        unlock(_control_mutex);
    }

    bool thread_group::maybe_dispatch(thread_group::thread_function func,
                                      void* data, bool async)
    {
        bool ret = false;
        debug("control maybe dispatching worker num_finished %d",
              _num_finished);

        lock(_control_mutex);

        lock(_worker_mutex);
        bool workersWaiting = (0 != _num_finished);
        unlock(_worker_mutex);

        if (workersWaiting && _func == 0)
        {
            _dispatching = true;
            if (!func)
            {
                //
                //  This will tell the worker to recall it's original func/data.
                //
                _recall = true;

                //  We need to set _func to something, so that the wait
                //  loop below functions as intended.
                //
                _func = (thread_function)0xdeadc0de;
                _data = 0;
            }
            else
            {
                _recall = false;
                _func = func;
                _data = data;
            }
            release_worker();
            debug("control waiting for worker to load");

            while (!async && _func)
            {
                wait_cond(_control_cond, _control_mutex);
            }

            debug("control done waiting for worker to dispatch");
            _dispatching = false;
            ret = true;
        }

        unlock(_control_mutex);
        return ret;
    }

    void thread_group::awaken_all_workers()
    {
        debug("control awakening workers");

        //
        //  We'll check again in loop below with proper locking, but can bail
        //  now if all threads are busy.
        //
        if (_num_finished == 0)
            return;

        //
        //  In theory, we could sit here forever, awakening worker
        //  threads that immediately sleep again.  So don't try more
        //  times than we have threads.
        //
        int attemptCount = _num_threads;
        bool workersWaiting;

        do
        {
            ////////////////////////////////////////////////////////
            //
            //  Deadlock danger.  Note that if someone else locks
            //  these mutexs in the opposite order, we have a
            //  possible deadlock.
            //
            //  But we (and dispatch and redispatch) are already
            //  two-level locking in this order, since
            //  release_worker will lock _worker_mutex while
            //  _control_mutex is locked (see below).
            //
            //  Further, if we don't do this, and release
            //  _worker_mutex before we lock _control_mutex, there
            //  is a danger that _num_finished will drop to 0 after
            //  we release _worker_mutex, so there will be no
            //  workers in worker_wait when we call wait_cond below,
            //  in which case it will block until a worker enters
            //  worker_wait, which could take forever.
            //
            ////////////////////////////////////////////////////////

            lock(_control_mutex);

            lock(_worker_mutex);
            workersWaiting = (0 != _num_finished);
            unlock(_worker_mutex);

            if (workersWaiting)
            {
                if (_func != 0)
                {
                    unlock(_control_mutex);
                    return;
                }
                _dispatching = true;

                //  This will tell the worker to recall it's original func/data.
                //
                _recall = true;

                //  We need to set _func to something, so that the wait
                //  loop below functions as intended.
                //
                _func = (thread_function)0xdeadc0de;

                release_worker();

                debug("control waiting for awakened worker to load");
                while (_func)
                {
                    wait_cond(_control_cond, _control_mutex);
                }
                debug("control done waiting for worker to reawaken");

                _dispatching = false;
            }
            unlock(_control_mutex);
        } while (workersWaiting && --attemptCount);
    }

} // namespace stl_ext
