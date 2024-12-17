//
// Copyright (C) 2023  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <TwkMovie/Movie.h>
#include <sys/types.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkFB/IO.h>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/pair.hpp>
#include <boost/interprocess/containers/vector.hpp>

#define TWK_SIDECAR_USE_QUEUE 1
#ifndef PLATFORM_LINUX
// on mac/windows message_queue uses a spin lock
#define TWK_SIDECAR_USE_QUEUE_TIMEOUT 1
#endif

namespace TwkMovie
{
    typedef std::pair<std::string, std::string> StringPair;

    namespace Shared
    {

        typedef boost::interprocess::managed_shared_memory::segment_manager
            SegmentManager;
        typedef boost::interprocess::allocator<char, SegmentManager>
            CharAllocator;

        typedef boost::interprocess::basic_string<char, std::char_traits<char>,
                                                  CharAllocator>
            String;

        typedef boost::interprocess::allocator<String, SegmentManager>
            StringAllocator;

        typedef boost::interprocess::vector<String, StringAllocator>
            StringVector;

    } // namespace Shared

    namespace Message
    {

        const size_t SizeInBytes();
        const size_t MaxInQueue();

        //
        //  Commands
        //
        const char* Acknowledge();
        const char* Shutdown();
        const char* OpenForRead();
        const char* FileOpened();
        const char* GetInfo();
        const char* ReadImage();
        const char* ReadAudio();
        const char* AudioConfigure();
        const char* CanConvertAudioRate();
        const char* ProtocolVersion();
        const char* Throw();
        const char* Flush();

        const char* CommandPrefix();

        bool isCommand(const char*);

        //
        //  Responses
        //
        const char* OK();
        const char* True();
        const char* False();
        const char* Failed();
        const char* TimeOut();

        std::string encodeMovieReadRequest(const Movie::ReadRequest&);
        void decodeMovieReadRequest(const std::string&, Movie::ReadRequest&);
        std::string encodeStringVector(const TwkFB::FBInfo::StringVector&);
        void decodeStringVector(const std::string& encoded,
                                TwkFB::FBInfo::StringVector& decoded);

        //
        //  Message parsing
        //
        //  Returns a StringPair of (CMD, ARG)
        //

        StringPair parseMessage(const char* processName, std::ostream* log,
                                const std::string&);

        //
        //  Message creation
        //
        //  Returns message. May truncate arg if it won't fit.
        //

        size_t newMessage(const char* processName, std::ostream* log,
                          const std::string& cmd, const std::string& arg,
                          char* outbuffer);

    } // namespace Message

    struct PODInfo
    {
        int width;
        int height;
        int uncropWidth;
        int uncropHeight;
        int uncropX;
        int uncropY;
        float pixelAspect;
        int numChannels;
        TwkFB::FrameBuffer::DataType dataType;
        TwkFB::FrameBuffer::Orientation orientation;
        bool video;
        int start;
        int end;
        int inc;
        float fps;
        float quality;
        bool audio;
        double audioSampleRate;
        int audioChannels;
        bool slowRandomAccess;
    };

} // namespace TwkMovie
