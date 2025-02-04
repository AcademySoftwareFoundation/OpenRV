//
//  Copyright (c) 2008 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <TwkExc/Exception.h>
#include <TwkMovie/ThreadedMovie.h>
#include <algorithm>

namespace TwkMovie
{
    using namespace std;

    static void trampoline(void* data)
    {
        ThreadedMovie* mov = reinterpret_cast<ThreadedMovie*>(data);
        mov->threadMain();
    }

    ThreadedMovie::ThreadedMovie(const Movies& movies, const Frames& frames,
                                 size_t stackMultiplier, ThreadAPI* api,
                                 InitializeFunc F)
        : m_movies(movies)
        , m_threadGroup(movies.size(), stackMultiplier, api)
        , m_frames(frames)
        , m_init(true)
        , m_currentIndex(0)
        , m_requestIndex(0)
        , m_initialize(F)
    {
        // if (!m_movie->isThreadSafe()) throw runtime_exception();
        m_info = movies.front()->info();
        m_threadSafe = false;
        pthread_mutex_init(&m_mapLock, 0);
        pthread_mutex_init(&m_runLock, 0);
        m_threadData.resize(movies.size());

        for (size_t i = 0; i < m_threadData.size(); i++)
        {
            m_threadData[i].movie = movies[i];
        }
    }

    ThreadedMovie::~ThreadedMovie()
    {
        m_threadGroup.control_wait();
        sort(m_movies.begin(), m_movies.end());

        for (size_t i = 0; i < m_movies.size(); i++)
        {
            if (i > 0 && m_movies[i] == m_movies[i - 1])
                continue;
            delete m_movies[i];
        }

        for (FBMap::iterator i = m_map.begin(); i != m_map.end(); ++i)
        {
            for (size_t q = 0; q < i->second.size(); q++)
                delete i->second[q];
        }

        pthread_mutex_destroy(&m_mapLock);
    }

    void ThreadedMovie::lock() { m_threadGroup.lock(m_mapLock); }

    void ThreadedMovie::unlock() { m_threadGroup.unlock(m_mapLock); }

    void ThreadedMovie::threadMain()
    {
        const size_t threads = m_threadGroup.num_threads();

        //
        //  HAVE TO MATCH THREADS WITH INPUTS EXACTLY -- OTHERWISE
        //  PER-THREAD STATE WILL GET MIXED UP (LIKE GL STATE)
        //
        //  Find the td struct keyed off of pthread_t here and use that
        //  exact one.
        //

        ThreadData* td = 0;
        pthread_t self = pthread_self();

        m_threadGroup.lock(m_runLock);

        //
        //  If we already have a ThreadData for this thread use it
        //

        for (size_t i = 0; !td && i < m_threadData.size(); i++)
        {
            ThreadData& d = m_threadData[i];

            if (d.init && pthread_equal(d.thread, self))
            {
                d.running = true;
                td = &d;
            }
        }

        //
        //  If we don't have a ThreadData find an available one and use
        //  that
        //

        bool first = false;

        for (size_t i = 0; !td && i < m_threadData.size(); i++)
        {
            ThreadData& d = m_threadData[i];

            if (!d.init)
            {
                d.init = true;
                first = true;
                d.thread = self;
                d.running = true;
                td = &d;
            }
        }

        m_threadGroup.unlock(m_runLock);

        if (m_initialize)
            m_initialize();

        do
        {
            lock();
            const size_t current = m_currentIndex;
            const size_t requested = m_requestIndex;

            if (current - requested < threads * 2 && current < m_frames.size())
            {
                //
                //  Bump the current index for the next thread
                //

                m_currentIndex++;
                const int frame = m_frames[current];
                bool exists = m_map.count(frame) > 0;
                unlock();

                if (!exists)
                {
                    td->request.frame = frame;
                    td->request.missing = false;
                    FrameBufferVector fbs;

                    try
                    {
                        // cout << "thread " << td->id << " @ frame " <<
                        // td->request.frame << endl;
                        td->movie->imagesAtFrame(td->request, fbs);
                    }
                    catch (std::exception& exc)
                    {
                        cerr << "WARNING: an exception was raised evaluting "
                                "frame "
                             << frame << ":" << endl;
                        cerr << exc.what() << endl;
                        unlock();
                        break;
                    }

                    lock();
                    m_map[frame] = fbs;
                    unlock();
                }
                else
                {
                    // cout << "thread " << td->id << " @ frame " <<
                    // td->request.frame
                    //<< " already in cache"
                    //<< endl;
                }
            }
            else
            {
                unlock();
                // cout << "thread " << td->id << " finished, current = " <<
                // current
                //<< ", requested = " << requested
                //<< endl;
                break;
            }
        } while (1);

        m_threadGroup.lock(m_runLock);
        td->running = false;
        m_threadGroup.unlock(m_runLock);

        // cout << "thread " << td->id << " no longer running" << endl;
    }

    void ThreadedMovie::dispatchAll()
    {
        const size_t threads = m_threadGroup.num_threads();

        //
        //  Launch any threads that aren't running. Some or all of these
        //  may exit immediately.
        //

        for (size_t i = 0; i < threads; i++)
        {
            //
            //  WRONG: we need to use the pthread_t of the dispatched
            //  thread to ensure we're always calling the same upstream
            //  code. So here we should just pass in the "this" pointer
            //  and in the thread main find the td struct.
            //

            m_threadGroup.maybe_dispatch(trampoline, this);
        }
    }

    void ThreadedMovie::imagesAtFrame(const ReadRequest& request,
                                      FrameBufferVector& fbs)
    {
        const size_t threads = m_threadGroup.num_threads();

        if (m_init)
        {
            //
            //  All requests should look the same (stereo, etc)
            //

            for (size_t i = 0; i < threads; i++)
            {
                ThreadData& td = m_threadData[i];
                td.id = i;
                td.running = false;
                td.request = request;
                td.threadedMovie = this;
                td.movie = m_movies[i];
            }

            m_init = false;
        }

        int frame = request.frame;
        const size_t n = m_threadGroup.num_threads();

        dispatchAll();

        lock();
        size_t current = m_currentIndex;
        size_t requested = m_requestIndex;
        unlock();

#if 0
    if (frame != m_frames[requested])
    {
        int f = m_frames[requested];
        TWK_THROW_EXC_STREAM("Frame mismatch in ThreadedMovie");
    }
#endif

        fbs.clear();

        for (size_t count = 0; true; count++)
        {
            lock();
            FBMap::iterator i = m_map.find(frame);
            FBMap::iterator e = m_map.end();
            unlock();

            if (i != e)
            {
                fbs = i->second;
                lock();
                m_map.erase(i);
                // cout << "consumed frame " << frame << endl;
                unlock();
                dispatchAll();
                break;
            }
            else
            {
                //
                //  Just wait for all of one to finish before continuing
                //  and have them restart.
                //

                m_threadGroup.control_wait(false);

                if (count > 0)
                {
                    m_currentIndex = requested;
                    dispatchAll();
                    m_threadGroup.control_wait(false);
                }
            }
        }

        lock();
        m_requestIndex++;
        unlock();
    }

    void ThreadedMovie::identifiersAtFrame(const ReadRequest& request,
                                           IdentifierVector& ids)
    {
        m_threadData.front().movie->identifiersAtFrame(request, ids);
    }

    size_t ThreadedMovie::audioFillBuffer(const AudioReadRequest& request,
                                          AudioBuffer& buffer)
    {
        return m_threadData.front().movie->audioFillBuffer(request, buffer);
    }

    void ThreadedMovie::audioConfigure(unsigned int channels,
                                       TwkAudio::Time rate, size_t bufferSize)
    {
    }

    void ThreadedMovie::flush() {}

    Movie* ThreadedMovie::clone() const { return 0; }

} // namespace TwkMovie
