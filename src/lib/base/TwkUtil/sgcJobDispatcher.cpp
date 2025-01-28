//*****************************************************************************/
// Copyright (c) 2018 Autodesk, Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//*****************************************************************************/

#include <TwkUtil/sgcJobDispatcher.h>
#include <TwkUtil/sgcMutex.h>
#include <TwkUtil/ThreadName.h>

#include <thread>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <vector>
#include <algorithm>
#include <string>

using namespace TwkUtil;

JobOps::Id JobDispatcher::nextId = 0;

class JobDispatcher::Imp
{
    //----------------------------------------------------------------------------
    //
    mutable Mutex m_mutex;
    using Jobs = std::vector<JobBase::Ptr>;
    Jobs m_waiting GUARDED_BY(m_mutex);
    Jobs m_running GUARDED_BY(m_mutex);

    std::atomic<unsigned> m_numPendingJobs{0};
    std::atomic<unsigned> m_numRunningJobs{0};
    mutable std::condition_variable m_signal;

    std::atomic<bool> m_shutdown{false};
    std::vector<std::thread> m_threadPool;

    size_t m_threadPoolSize;
    const std::string m_threadName;

    //----------------------------------------------------------------------------
    //
    template <typename Container>
    auto findById(const Container& data, JobOps::Id id)
    {
        return std::find_if(
            data.begin(), data.end(),
            [id](typename Container::value_type const& rhs) -> bool
            { return id == rhs->m_id; });
    }

public:
    //----------------------------------------------------------------------------
    //
    Imp(size_t maxJobs, const char* threadName)
        : m_threadPoolSize(maxJobs)
        , m_threadName(threadName ? threadName : "SGC-Pool")
    {
        start();
    }

    //----------------------------------------------------------------------------
    //
    ~Imp()
    {
        if (!m_shutdown)
            stop();
    }

    //----------------------------------------------------------------------------
    //
    void start()
    {
        if (!m_threadPool.empty())
            return;

        m_shutdown = false;
        m_threadPool.reserve(m_threadPoolSize);
        for (size_t i = 0; i < m_threadPoolSize; ++i)
        {
            m_threadPool.emplace_back(
                [this, i]() -> void
                {
                    std::string threadName(m_threadName.c_str());
                    threadName.append("#");
                    threadName.append(std::to_string(i + 1).c_str());

                    TwkUtil::setThreadName(threadName);

                    while (1)
                    {
                        typename Jobs::value_type job;
                        {
                            // Wait for a signal (new job added or dependency
                            // complete) or shutdown
                            //
                            MutexGuard lock{&m_mutex};
                            m_signal.wait(
                                lock.native(), [this]() -> bool
                                { return m_numPendingJobs || m_shutdown; });

                            // If shutdown occurred, finish executing all
                            // remaining jobs and then exit
                            //
                            if (m_shutdown && m_waiting.empty())
                                return;

                            // Find the next queued job that doesn't have a
                            // queued or running dependency.  If one is not
                            // found, go back to waiting
                            //
                            bool foundJob = false;
                            for (auto nextJob = m_waiting.begin(),
                                      end = m_waiting.end();
                                 nextJob != end; ++nextJob)
                            {
                                foundJob = findById(m_waiting,
                                                    (*nextJob)->m_dependency)
                                               == end
                                           && findById(m_running,
                                                       (*nextJob)->m_dependency)
                                                  == m_running.end();
                                if (foundJob)
                                {
                                    job = *nextJob;
                                    m_running.emplace_back(job);
                                    m_waiting.erase(nextJob);
                                    m_signal.notify_all();
                                    break;
                                }
                            }
                            if (!foundJob)
                                continue;
                        }

                        // If we have a viable job, execute it
                        //
                        if (!job)
                            continue;

                        --m_numPendingJobs;
                        ++m_numRunningJobs;
                        job->execute();
                        --m_numRunningJobs;

                        // Signal any waiting threads waiting for a dependency
                        // to complete
                        //
                        {
                            MutexGuard lock{&m_mutex};
                            if (auto runningJob =
                                    findById(m_running, job->m_id);
                                runningJob != m_running.end())
                            {
                                m_running.erase(runningJob);
                            }
                        }
                        m_signal.notify_all();
                    }
                });
        }
    }

    //----------------------------------------------------------------------------
    //
    void stop()
    {
        if (m_threadPool.empty())
            return;

        m_shutdown = true;
        m_signal.notify_all();
        for (auto& t : m_threadPool)
        {
            t.join();
        }
        m_threadPool.clear();
    }

    //----------------------------------------------------------------------------
    //
    void wait() const REQUIRES(!m_mutex)
    {
        MutexGuard lock{&m_mutex};

        while (m_numRunningJobs)
        {
            m_signal.wait(lock.native());
        }
    }

    //----------------------------------------------------------------------------
    //
    void resize(unsigned maxJobs)
    {
        stop();
        m_threadPoolSize = maxJobs;
        start();
    }

    //----------------------------------------------------------------------------
    //
    JobOps::Id addJob(const JobBase::Ptr& job) REQUIRES(!m_mutex)
    {
        {
            MutexGuard lock{&m_mutex};
            m_waiting.emplace_back(job);
        }
        ++m_numPendingJobs;
        m_signal.notify_all();

        return job->m_id;
    }

    //----------------------------------------------------------------------------
    //
    void removeJob(JobOps::Id id) REQUIRES(!m_mutex)
    {
        // If job is queued but not running, pull it out of the queue
        //
        MutexGuard lock{&m_mutex};

        auto it = findById(m_waiting, id);
        if (it != m_waiting.end())
        {
            m_waiting.erase(it);
            --m_numPendingJobs;

            m_signal.notify_all();
            return;
        }
    }

    //----------------------------------------------------------------------------
    //
    void waitJob(JobOps::Id id) REQUIRES(!m_mutex)
    {
        MutexGuard lock{&m_mutex};

        while ((findById(m_waiting, id) != m_waiting.end())
               || (findById(m_running, id) != m_running.end()))
        {
            m_signal.wait(lock.native());
        }
    }

    //----------------------------------------------------------------------------
    //
    void prioritizeJob(JobOps::Id id) REQUIRES(!m_mutex)
    {
        MutexGuard lock{&m_mutex};

        auto it = findById(m_waiting, id);
        if (it != m_waiting.end())
        {
            auto job = *it;
            m_waiting.erase(it);
            m_waiting.insert(m_waiting.begin(), job);

            m_signal.notify_all();
        }
    }
};

//-----------------------------------------------------------------------------
//
JobDispatcher::JobDispatcher(unsigned maxJobs, const char* threadName)
    : m_imp(new Imp(maxJobs, threadName))
{
}

//-----------------------------------------------------------------------------
//
JobDispatcher::~JobDispatcher() { delete m_imp; }

//-----------------------------------------------------------------------------
//
JobOps::Id JobDispatcher::addJob(const JobBase::Ptr& job)
{
    return m_imp->addJob(job);
}

//-----------------------------------------------------------------------------
//
void JobDispatcher::removeJob(JobOps::Id id) { return m_imp->removeJob(id); }

//-----------------------------------------------------------------------------
//
void JobDispatcher::wait() const { m_imp->wait(); }

//-----------------------------------------------------------------------------
//
void JobDispatcher::start() { m_imp->start(); }

//-----------------------------------------------------------------------------
//
void JobDispatcher::resize(unsigned maxJobs) { m_imp->resize(maxJobs); }

//-----------------------------------------------------------------------------
//
void JobDispatcher::stop() { m_imp->stop(); }

//-----------------------------------------------------------------------------
//
void JobDispatcher::waitJob(JobOps::Id id) { m_imp->waitJob(id); }

//-----------------------------------------------------------------------------
//
void JobDispatcher::prioritizeJob(JobOps::Id id) { m_imp->prioritizeJob(id); }
