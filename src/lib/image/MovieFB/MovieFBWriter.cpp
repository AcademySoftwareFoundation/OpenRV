//******************************************************************************
// Copyright (c) 2007 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <MovieFB/MovieFBWriter.h>
#include <TwkExc/TwkExcException.h>
#include <TwkFB/IO.h>
#include <TwkFB/Operations.h>
#include <TwkMovie/MovieIO.h>
#include <TwkUtil/FrameUtils.h>
#include <TwkUtil/File.h>
#include <TwkUtil/SystemInfo.h>
#include <TwkUtil/ThreadName.h>
#include <TwkUtil/Daemon.h>
#include <TwkUtil/File.h>
#include <iostream>
#include <stl_ext/string_algo.h>
#include <stl_ext/thread_group.h>
#include <limits>
#include <math.h>

namespace TwkMovie
{
    using namespace std;
    using namespace TwkFB;
    using namespace TwkUtil;

#if defined(TWK_LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN__)
#define AF_BYTEORDER AF_BYTEORDER_LITTLEENDIAN
#else
#define AF_BYTEORDER AF_BYTEORDER_BIGENDIAN
#endif

    typedef FrameBufferIO::StringPair StringPair;
    typedef FrameBufferIO::StringPairVector StringPairVector;

#if 0
#define DB_GENERAL 0x01
#define DB_ALL 0xff

#define DB_LEVEL DB_ALL

#define DB(x)                  \
    if (DB_GENERAL & DB_LEVEL) \
    cerr << dec << "MFBWriter: " << x << endl
#define DBL(level, x)     \
    if (level & DB_LEVEL) \
    cerr << dec << "MFBWriter: " << x << endl
#else
#define DB(x)
#define DBL(level, x)
#endif

    class WriteTask
    {
    public:
        enum State
        {
            Ready,
            InProgress,
            Finished
        };

        typedef MovieWriter::WriteRequest WriteRequest;

        WriteTask()
            : state(Finished)
        {
        }

        WriteTask(WriteRequest& r, int f, FrameBufferVector& v, string file)
            : request(r)
            , frame(f)
            , state(Finished)
            , fbs(v)
            , filename(file)
        {
        }

        int frame;
        string filename;
        State state;
        WriteRequest request;
        FrameBufferVector fbs;
    };

    class WriteTaskManager
    {
    public:
        WriteTaskManager(int size);
        ~WriteTaskManager();

        const WriteTask& taskByIndex(int index);
        void finishTask(int index);
        int nextReadyTaskIndex();
        void addTask(WriteTask& t);
        void waitAll();
        void lock();
        void unlock();
        void threadMain(int threadNumber);
        void dispatchThreads();

        class ThreadData
        {
        public:
            ThreadData(WriteTaskManager* m, int n)
                : manager(m)
                , threadNumber(n)
            {
            }

            WriteTaskManager* manager;
            int threadNumber;
        };

        // Special return values for WriteTaskManager::nextReadyTaskIndex()
        static const int A_TASK_IS_CURRENTLY_BEING_ADDED = -2;
        static const int NO_MORE_TASKS = -1;

    private:
        vector<WriteTask> tasks;
        stl_ext::thread_group threadGroup;
        pthread_mutex_t managerLock;
        pthread_cond_t waitCond;
        vector<ThreadData> threadData;

        // Indicates that a task is currently being added when non 0
        // Note: Lock the managerLock before reading/writing
        // currentlyAddingATask
        int currentlyAddingATask{0};
    };

    WriteTaskManager::WriteTaskManager(int size)
        : threadGroup(size, 8, 0)
    {
        tasks.resize(size);

        for (int i = 0; i < size; ++i)
            threadData.push_back(ThreadData(this, i));

        pthread_mutex_init(&managerLock, 0);
        pthread_cond_init(&waitCond, 0);
    }

    WriteTaskManager::~WriteTaskManager()
    {
        pthread_mutex_destroy(&managerLock);
        pthread_cond_destroy(&waitCond);
    }

    void WriteTaskManager::lock() { threadGroup.lock(managerLock); }

    void WriteTaskManager::unlock() { threadGroup.unlock(managerLock); }

    int WriteTaskManager::nextReadyTaskIndex()
    {
        int ret = NO_MORE_TASKS;

        lock();
        for (int i = 0; i < tasks.size(); ++i)
        {
            if (tasks[i].state == WriteTask::Ready)
            {
                tasks[i].state = WriteTask::InProgress;
                ret = i;
                break;
            }
        }
        if ((NO_MORE_TASKS == ret) && (currentlyAddingATask > 0))
        {
            ret = A_TASK_IS_CURRENTLY_BEING_ADDED;
        }
        unlock();

        DB("nextReadyTaskIndex " << ret);

        return ret;
    }

    void WriteTaskManager::finishTask(int index)
    {
        DB("finishTask " << index);

        lock();

        //
        //  Mark task complete
        //
        tasks[index].state = WriteTask::Finished;

        //
        //  Signal main thread that there's room for more tasks
        //
        stl_ext::thread_group::signal(waitCond);

        unlock();

        DB("finishTask " << index << " complete");
    }

    const WriteTask& WriteTaskManager::taskByIndex(int index)
    {
        return tasks[index];
    }

    void WriteTaskManager::threadMain(int threadNumber)
    {
        DB("thread " << threadNumber << " starting");
        int index;

        while ((index = nextReadyTaskIndex()) != NO_MORE_TASKS)
        {
            // If there is no more tasks to process but that a new task is
            // currently being added, then we do not want to exit the thread
            // function otherwise this could potentially lead to a thread
            // synchronization issue.
            if (A_TASK_IS_CURRENTLY_BEING_ADDED == index)
            {
                continue;
            }

            const WriteTask& t = taskByIndex(index);

            DB("thread " << threadNumber << " writing '" << t.filename);

            try
            {
                TwkFB::GenericIO::writeImages(t.fbs, t.filename, t.request);
            }
            catch (std::exception& exc)
            {
                cerr << "ERROR: std::exception while writing '" << t.filename
                     << "': " << exc.what() << endl;
            }
            catch (...)
            {
                cerr << "ERROR: unknown exception while writing '" << t.filename
                     << endl;
            }

            for (int i = 0; i < t.fbs.size(); i++)
                delete t.fbs[i];

            finishTask(index);

            DB("thread " << threadNumber << " finished writing '"
                         << t.filename);
        }
        DB("thread " << threadNumber << " finished");
    }

    static void trampoline(void* vdata)
    {
        WriteTaskManager::ThreadData* data =
            reinterpret_cast<WriteTaskManager::ThreadData*>(vdata);
        TwkUtil::setThreadName("MovieFBWriter");
        data->manager->threadMain(data->threadNumber);
    }

    void WriteTaskManager::addTask(WriteTask& t)
    {
        bool addedTask = false;

        lock();
        currentlyAddingATask++;
        unlock();

        while (!addedTask)
        {
            lock();

            for (int i = 0; i < tasks.size(); ++i)
            {
                if (tasks[i].state == WriteTask::Finished)
                {
                    DB("addTask succeeded, " << i);
                    tasks[i] = t;
                    tasks[i].state = WriteTask::Ready;
                    addedTask = true;
                    break;
                }
            }
            if (!addedTask)
            {
                DB("addTask waiting");
                //
                //  Wait for worker thread to signal that there's room for more
                //  tasks
                //
                static const size_t waitCondTimeOutInSecs = 5;
                if (!stl_ext::thread_group::wait_cond_time(
                        waitCond, managerLock, waitCondTimeOutInSecs * 1e6))
                {
                    DB("addTask waiting-timed out");

                    // Make sure that the worker threads have been dispatched
                    // Note that this is to compensate for an inherent race
                    // condition that can occur with this thread_group
                    // implementation where the WriteTaskManager::threadMain()
                    // might have exited because there was no more tasks to
                    // process but before the thread_group registers it as being
                    // finished (thread_group::_num_finished) a new task is
                    // added by WriteTaskManager::addTask() and the
                    // dispatchThreads() ending up as a noop because it is seen
                    // as not finished yet.
                    dispatchThreads();
                }
            }

            unlock();
        }

        dispatchThreads();

        lock();
        currentlyAddingATask--;
        unlock();
    }

    void WriteTaskManager::dispatchThreads()
    {
        DB("dispatching threads");

        const size_t threads = threadGroup.num_threads();

        for (size_t i = 0; i < threads; i++)
        {
            threadGroup.maybe_dispatch(trampoline, &threadData[i]);
        }
    }

    void WriteTaskManager::waitAll()
    {
        //
        //  Wait for all threads to finish
        //
        DB("waitAll()");
        threadGroup.control_wait(true);
        DB("waitAll() complete");
    }

    MovieFBWriter::MovieFBWriter() {}

    MovieFBWriter::~MovieFBWriter() {}

    bool MovieFBWriter::write(Movie* inMovie, const string& filepattern,
                              WriteRequest& writeRequest)
    {
        m_movie = inMovie;
        bool verbose = writeRequest.verbose;
        const string imagePattern = filepattern;
        bool stereo = writeRequest.stereo;

        Frames frames;

        const int fs = inMovie->info().start;
        const int fe = inMovie->info().end;
        const int inc = inMovie->info().inc;
        float fps = inMovie->info().fps;

        if (writeRequest.timeRangeOverride)
        {
            fps = writeRequest.fps;
        }

        //
        //  No difference between codec and compressor for MovieFB
        //

        if (writeRequest.compression == "")
        {
            writeRequest.compression = writeRequest.codec;
        }

        if (writeRequest.timeRangeOverride)
        {
            frames.resize(writeRequest.frames.size());
            copy(writeRequest.frames.begin(), writeRequest.frames.end(),
                 frames.begin());
        }
        else
        {
            for (int i = fs; i <= fe; i += inc)
            {
                frames.push_back(i);
            }
        }

        //
        //  Add some params for the frame I/O classes
        //

        StringPairVector& params = writeRequest.parameters;

        {
            ostringstream str;
            str << frames.size();
            params.insert(params.begin(),
                          StringPair("output/sequence_len", str.str()));
        }

        {
            ostringstream str;
            str << fs;
            params.insert(params.begin(),
                          StringPair("output/start_frame", str.str()));
        }

        {
            ostringstream str;
            str << fe;
            params.insert(params.begin(),
                          StringPair("output/end_frame", str.str()));
        }

        // this one is updated every frame
        params.insert(params.begin(), StringPair("output/frame", "_"));
        params.insert(params.begin(), StringPair("output/frame_position", "_"));

        string timeStr;
        string sequencePattern;

        const bool hasPatterns =
            splitSequenceName(imagePattern, timeStr, sequencePattern);
        if (hasPatterns)
        {
            WriteTaskManager manager(
                (writeRequest.threads) ? writeRequest.threads : 1);

            for (unsigned int i = 0; i < frames.size(); i++)
            {
                //
                //  Establish which frame we're writing
                //

                int f = frames[i];

                if (f < fs || f > fe)
                    continue;
                string filename =
                    replaceFrameSymbols(imagePattern, f, int(fps));

                {
                    //
                    //  Update the frame_position output param. NOTE:
                    //  params is an alias to writeRequest.parameters
                    //  above.
                    //

                    ostringstream str;
                    str << (f - fs);

                    ostringstream str2;
                    str2 << f;

                    params[0].second = str.str();
                    params[1].second = str2.str();
                }

                //
                //  Create the read request
                //

                FrameBufferVector fbs;
                Movie::ReadRequest request(f, stereo);
                if (writeRequest.views.size())
                    request.views = writeRequest.views;

                inMovie->imagesAtFrame(request, fbs);

                if (verbose)
                {
                    cout << "INFO: writing frame " << f << " ("
                         << int(float(i) / float(frames.size() - 1) * 10000.0)
                                / float(100.0)
                         << "% done)" << endl;
                }

                WriteTask task(writeRequest, f, fbs, filename);

                manager.addTask(task);
            }

            manager.waitAll();
        }
        else
        {
            //
            //  Its just one output frame so the input range is kind of
            //  pointless
            //

            //
            //  Make ReadRequest to send up the graph
            //

            FrameBufferVector fbs;
            Movie::ReadRequest request(frames.front(), stereo);
            if (writeRequest.views.size())
                request.views = writeRequest.views;

            inMovie->imagesAtFrame(request, fbs);

            //
            //  Call TwkFB's I/O to output the frame
            //

            if (verbose)
                cout << "INFO: writing " << imagePattern << endl;
            TwkFB::GenericIO::writeImages(fbs, imagePattern, writeRequest);

            //
            //  Clean up old fbs or we'll quickly run out of memory
            //

            for (int i = 0; i < fbs.size(); i++)
            {
                delete fbs[i];
            }
        }

        return true;
    }

} // namespace TwkMovie
