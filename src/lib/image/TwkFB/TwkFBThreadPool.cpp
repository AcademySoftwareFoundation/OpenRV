//*****************************************************************************/
// Copyright (c) 2019 Autodesk, Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//*****************************************************************************/

#include <TwkFB/TwkFBThreadPool.h>

#include <TwkUtil/SystemInfo.h>

#include <IlmThreadPool.h>

#include <algorithm>

namespace TwkFB
{
    namespace ThreadPool
    {

        static ILMTHREAD_NAMESPACE::ThreadPool memcpyThreadPool;
        static size_t numThreads;

        void initialize()
        {
            const char* memcpyThreadCount = getenv("RV_MEMCPY_THREAD_COUNT");
            numThreads =
                memcpyThreadCount
                    ? (size_t)atoi(memcpyThreadCount)
                    : std::min(TwkUtil::SystemInfo::numCPUs() / 4, (size_t)8);
            memcpyThreadPool.setNumThreads(numThreads);
        }

        void shutdown() { memcpyThreadPool.setNumThreads(0); }

        size_t getNumThreads() { return numThreads; }

        void addTask(ILMTHREAD_NAMESPACE::Task* task)
        {
            memcpyThreadPool.addTask(task);
        }

    } // namespace ThreadPool
} // namespace TwkFB
