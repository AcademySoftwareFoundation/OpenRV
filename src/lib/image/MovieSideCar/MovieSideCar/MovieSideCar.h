//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#ifndef __MovieSideCar__MovieSideCar__h__
#define __MovieSideCar__MovieSideCar__h__
#include <TwkMovie/MovieReader.h>
#include <TwkMovie/MovieIO.h>
#include <algorithm>
#include <boost/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <cstdlib>
#include <limits>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <vector>
#include <list>
#include <QtCore/QProcess>

namespace TwkMovie
{

    void shutdownMovieSideCars(int sig = 0);

    class MovieSideCar : public MovieReader
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
        typedef boost::mutex Mutex;
        typedef boost::lock_guard<Mutex> Lock;

        //
        //  Constructors
        //
        //  If cloneable is true, than the clone() method will return a
        //  clone of the MovieSideCar. This can be bad if you're worried
        //  about too many open file descriptors or flooding the user's
        //  machine with multiple processes.
        //

        MovieSideCar(const std::string& pathToSideCar, bool cloneable = false);
        ~MovieSideCar();

        //
        //  MovieReader API
        //

        virtual void
        open(const std::string& filename, const MovieInfo& as = MovieInfo(),
             const Movie::ReadRequest& request = Movie::ReadRequest());

        //
        //  Movie API
        //

        virtual bool canConvertAudioRate() const;
        virtual void imagesAtFrame(const ReadRequest& request,
                                   FrameBufferVector&);
        virtual void identifiersAtFrame(const ReadRequest& request,
                                        IdentifierVector&);
        virtual size_t audioFillBuffer(const AudioReadRequest&, AudioBuffer&);
        virtual void audioConfigure(const AudioConfiguration& conf);
        virtual void flush();
        virtual Movie* clone() const;

        void shutdown();

    private:
        void sendCommand(const std::string&, const std::string& args = "");
        StringPair waitForResponse();
        void deserializeInfo(const std::string&);
        void identifier(int frame, std::ostream& o);
        void decodeMemoryInfo(const std::string&);

    private:
        Queue* m_commandQueue;
        Queue* m_responseQueue;
        SharedMemory* m_sharedObject;
        std::string m_sharedObjectName;
        MappedRegion* m_sharedRegion;
        size_t m_sharedRegionSize;
        char* m_buffer;
        std::string m_sidecarPath;
        std::string m_commandQueueName;
        std::string m_responseQueueName;
        std::string m_sharedDataName;
        Mutex m_mutex;
        bool m_cloneable;
        QProcess* m_sidecarProcess;
        bool m_canConvertAudio;
        char* m_audioRegion;
        size_t m_audioRegionSize;
        char* m_imageRegion;
        size_t m_imageRegionSize;
    };

} // namespace TwkMovie

#endif // __MovieSideCar__MovieSideCar__h__
