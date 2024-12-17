//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <TwkMovie/MovieIO.h>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/program_options.hpp>
#include <cstdlib>
#include <limits>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <vector>

namespace TwkMovie
{
    using namespace std;

    //
    //  class SideCar
    //
    //  This class is essentially the main.cpp of the sidecar process. It does
    //  all the real IO work and transfers pixels back to the launching
    //  process.
    //
    //  The sidecar is given a command and response queue from the launching
    //  process. The sidecar will allocate the memory used to transfer data
    //  between them. It is the responsibility of the sidecar to remove the
    //  shared memory it creates, but it should not remove the queues (this
    //  will be done by the launch process).
    //
    //  This code is not really thread safe. Right now only a single thread is
    //  allowed to enter run -- probably the main thread -- and when run
    //  completes communication has been severed.
    //

    class SideCar
    {
    public:
        //
        //  Types
        //

        typedef boost::interprocess::offset_ptr<void, int32_t, uint64_t>
            interop_offset_ptr;
        typedef boost::interprocess::message_queue_t<interop_offset_ptr>
            interop_message_queue;
        typedef interop_message_queue Queue;
        typedef boost::interprocess::shared_memory_object SharedMemory;
        typedef boost::interprocess::mapped_region MappedRegion;
        typedef std::pair<std::string, std::string> StringPair;
        typedef std::vector<TwkFB::FrameBuffer*> FrameBufferVector;
        typedef TwkAudio::AudioBuffer AudioBuffer;

        //
        //  Constructors
        //

        SideCar(const std::string& commandQueueName,
                const std::string& responseQueueName, size_t launchPID,
                MovieIO*);

        ~SideCar();

        //
        //  Run starts the command loop. It will throw if something goes
        //  wrong.
        //

        void run();

    private:
        StringPair nextCommand();
        void respond(const std::string&, const std::string& args = "");
        void createSharedData(const std::string&, size_t);
        bool openForRead(const std::string&);
        std::string serializeInfo();
        size_t serializeImages(const FrameBufferVector&);
        void decodeAudioConfigure(const std::string&);
        size_t decodeAudioFillBuffer(const std::string&);

    private:
        MovieIO* m_io;
        MovieReader* m_reader;
        MovieWriter* m_writer;
        MovieInfo m_info;
        char* m_buffer;
        Queue m_commandQueue;
        Queue m_responseQueue;
        SharedMemory* m_sharedObject;
        std::string m_sharedObjectName;
        MappedRegion* m_sharedRegion;
        size_t m_sharedRegionSize;
        size_t m_launchProcessPID;
        size_t m_imageRegionSize;
        void* m_imageRegion;
        size_t m_audioRegionSize;
        size_t m_audioOffset;
        void* m_audioRegion;
        AudioBuffer m_audioBuffer;
    };

} // namespace TwkMovie
