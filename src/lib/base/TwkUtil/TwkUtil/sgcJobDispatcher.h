//*****************************************************************************/
// Copyright (c) 2018 Autodesk, Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//*****************************************************************************/

#ifndef SGC_CORE_JOBDISPATCHER_H
#define SGC_CORE_JOBDISPATCHER_H

#include <TwkUtil/dll_defs.h>
#include <TwkUtil/sgcSharedPtr.h>

namespace TwkUtil
{

    // @brief These templated methods can be specialized by the client to modify
    //        job data depending on the operation being peformed by the
    //        dispatcher
    //
    namespace JobOps
    {
        using Id = int;
        constexpr Id NO_DEPENDENCY = -1;

        enum OpType
        {
            WAIT,   //< Called when a job has been queued by the dispatcher
            RUN,    //< Called when the dispatcher starts executing the job
            FINISH, //< Called when the job has finished executing
        };

        // @brief called when a job changes state
        //
        // @param generic JobData supplied to the addJob method
        //
        // @return modified generic JobData
        //
        template <typename JobDataType>
        JobDataType update(Id id, OpType op, JobDataType info);
    } // namespace JobOps

    /// @brief Dispatches a Job to a thread-pool and notifies on state change
    ///
    /// @remark stop and start must be called from the same thread
    ///
    class TWKUTIL_EXPORT JobDispatcher
    {
        class Imp;
        Imp* m_imp; //< Private implementation

        static JobOps::Id nextId;

        // @brief RefCounted Job base class
        //
        struct JobBase : public RefCountedMT
        {
            using Ptr = SharedPtr<JobBase>;
            const JobOps::Id m_id{++nextId};
            const JobOps::Id m_dependency{JobOps::NO_DEPENDENCY};

            explicit JobBase(JobOps::Id dependency)
                : m_dependency(dependency)
            {
            }

            virtual void execute() = 0;
            virtual ~JobBase() = default;
        };

        // @brief Job class for storing templated job data
        //
        template <typename JobDataType> struct Job : public JobBase
        {
            JobDataType m_jobType;

            // @brief construct the job and trigger waiting state
            //
            Job(const JobDataType& jobType, JobOps::Id dependency)
                : JobBase(dependency)
                , m_jobType(JobOps::update(m_id, JobOps::WAIT, jobType))
            {
            }

            // @brief trigger running state on start and finish state on
            // completion
            //
            void execute() override
            {
                m_jobType = JobOps::update(m_id, JobOps::RUN, m_jobType);
                m_jobType = JobOps::update(m_id, JobOps::FINISH, m_jobType);
            }
        };

        // @brief private addJob implementation
        //
        JobOps::Id addJob(const JobBase::Ptr& job);

        /// Forbidden operations
        ///
        JobDispatcher(const JobDispatcher&) = delete;
        JobDispatcher& operator=(const JobDispatcher&) = delete;

        JobDispatcher(JobDispatcher&&) = delete;
        JobDispatcher& operator=(JobDispatcher&&) = delete;

    public:
        /// @brief constructor
        ///
        /// @param maxJobs The maximum number of concurrent jobs. Used to
        /// construct
        ///                the thread pool
        /// @param threadName Name to set on the threads. Threads will
        //               have an index appended to the name.
        ///
        explicit JobDispatcher(unsigned maxJobs,
                               const char* threadName = nullptr);

        /// @brief destructor
        ///
        virtual ~JobDispatcher();

        /// @brief blocking wait until all jobs have completed
        ///
        void wait() const;

        /// @brief blocking wait until all jobs are completed and stop all
        /// threads.
        ///
        /// @remark automatically called on destruction
        ///
        void stop();

        /// @brief start all threads
        ///
        /// @remark automatically called on construction
        ///
        void start();

        /// @brief resize the thread pool
        ///
        /// @param maxJobs The maximum number of concurrent jobs. Used to
        /// construct
        ///                the thread pool
        ///
        /// @remark This will complete all running threads before the resize
        /// happens
        ///
        void resize(unsigned maxJobs);

        /// @brief add a new job to be executed
        ///
        /// @param jobInfo job data to be executed
        /// @param dependency id of job the added job depends on
        ///
        /// @return id of job
        ///
        /// @remark the job is queued for execution until there is a free thread
        /// in
        ///         the pool
        ///
        /// @remark when the job is run JobOps::update will be called with type
        /// RUN
        ///         from a thread in the thread pool
        ///
        /// @remark the caller is responsible for ensuring there are no circular
        ///         dependencies in jobs that are added. Not doing so can cause
        ///         deadlock.
        ///
        template <typename JobDataType>
        JobOps::Id addJob(const JobDataType& jobInfo,
                          JobOps::Id dependency = JobOps::NO_DEPENDENCY)
        {
            return addJob(
                JobBase::Ptr(new Job<JobDataType>(jobInfo, dependency)));
        }

        /// @brief Dequeue the job for execution by the thread pool
        ///
        /// @param id of job to remove
        ///
        /// @remark If the job has already started running or already completed
        ///         this will have no effect
        ///
        void removeJob(JobOps::Id id);

        /// @brief Wait the completion of a job
        ///
        /// @param id of job to wait
        ///
        void waitJob(JobOps::Id id);

        /// @brief Put the job in the front of the waiting queue
        ///
        /// @param id of the job to prioritize
        ///
        /// @remark If the job has already started running or already completed
        ///         this will have no effect
        ///
        void prioritizeJob(JobOps::Id id);
    };

} // namespace TwkUtil

#endif
