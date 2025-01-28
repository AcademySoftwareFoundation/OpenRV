//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <MovieSideCar/MovieSideCar.h>
#include <MovieSideCar/Common.h>
#include <TwkFB/IO.h>
#include <TwkMath/Function.h>
#include <TwkUtil/ProcessInfo.h>
#include <TwkMovie/Exception.h>
#include <TwkMovie/Movie.h>
#include <TwkMovie/MovieIO.h>
#include <TwkMath/Color.h>
#include <TwkUtil/File.h>
#include <TwkUtil/MemBuf.h>
#include <IOgto/IOgto.h>
#include <algorithm>
#include <assert.h>
#include <ctype.h>
#include <iostream>
#include <limits>
#include <stdlib.h>
#include <signal.h>
#include <stl_ext/stl_ext_algo.h>
#include <stl_ext/string_algo.h>
#include <boost/filesystem.hpp>
#include <boost/functional/hash.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <string>

namespace TwkMovie
{
    using namespace std;
    using namespace boost::program_options;
    using namespace boost::algorithm;
    using namespace boost::filesystem;
    using namespace boost;
    using namespace boost::posix_time;
    using namespace boost::gregorian;

    typedef interprocess::managed_shared_memory managed_shared_memory;
    typedef interprocess::interprocess_exception interprocess_exception;

    //----------------------------------------------------------------------
    //
    //  Utiltity functions
    //

    static std::list<MovieSideCar*> movieSideCarsRunning;
    static boost::mutex movieSideCarsRunning_mutex;

    void shutdownMovieSideCars(int sig)
    {
        boost::lock_guard<boost::mutex> lock(movieSideCarsRunning_mutex);
        for (std::list<MovieSideCar*>::iterator it =
                 movieSideCarsRunning.begin();
             it != movieSideCarsRunning.end(); ++it)
        {
            (*it)->shutdown();
        }
        movieSideCarsRunning.clear();
    }

    static void setupMovieSideCarSignalHander()
    {
#ifndef PLATFORM_WINDOWS
        signal(SIGBUS, shutdownMovieSideCars);
#endif
        signal(SIGTERM, shutdownMovieSideCars);
        signal(SIGSEGV, shutdownMovieSideCars);
        signal(SIGILL, shutdownMovieSideCars);
    }

    static void defaultMovieSideCarSignalHander()
    {
#ifndef PLATFORM_WINDOWS
        signal(SIGBUS, SIG_DFL);
#endif
        signal(SIGTERM, SIG_DFL);
        signal(SIGSEGV, SIG_DFL);
        signal(SIGILL, SIG_DFL);
    }

    static void addMovieSideCars(MovieSideCar* sideCarPtr)
    {
        boost::lock_guard<boost::mutex> lock(movieSideCarsRunning_mutex);
        if (movieSideCarsRunning.empty())
        {
            setupMovieSideCarSignalHander();
        }

        movieSideCarsRunning.push_back(sideCarPtr);
    }

    static void removeMovieSideCars(MovieSideCar* sideCarPtr)
    {
        boost::lock_guard<boost::mutex> lock(movieSideCarsRunning_mutex);
        movieSideCarsRunning.remove(sideCarPtr);

        if (movieSideCarsRunning.empty())
        {
            // Since there are no side cars running
            // set the signal handler back to its default
            defaultMovieSideCarSignalHander();
        }
    }

    namespace
    {

        void decodeChannelInfoVector(const string& buffer,
                                     TwkFB::FBInfo::ChannelInfoVector& v)
        {
            vector<string> parts;
            split(parts, buffer, is_any_of(string(",")));

            for (size_t i = 0; i < parts.size(); i++)
            {
                TwkFB::FBInfo::ChannelInfo ci;

                const string& s = parts[i];
                if (s.size() < 2)
                    continue;

                ci.type = (TwkFB::FrameBuffer::DataType)(s[0] - 'A');
                ci.name = s.substr(1, s.size() - 1);
                v.push_back(ci);
            }
        }

        void decodeLayerInfoVector(const string& buffer,
                                   TwkFB::FBInfo::LayerInfoVector& v)
        {
            vector<string> parts;
            split(parts, buffer, is_any_of(string("`")));

            if (buffer.size() == 0)
                return;

            if (parts.size() % 2 != 0)
            {
                cerr << "ERROR: encoded LayerInfoVector size % 2 != 0" << endl;
                return;
            }

            //
            //  Each LayerInfo is spread over 2 entries
            //

            for (size_t i = 0; i < parts.size(); i += 2)
            {
                TwkFB::FBInfo::LayerInfo li;
                li.name = parts[i];
                decodeChannelInfoVector(parts[i + 1], li.channels);
                v.push_back(li);
            }
        }

        void decodeViewInfoVector(const string& buffer,
                                  TwkFB::FBInfo::ViewInfoVector& v)
        {
            vector<string> parts;
            split(parts, buffer, is_any_of(string("|")));

            if (buffer.size() == 0)
                return;

            if (parts.size() % 3 != 0)
            {
                cerr << "ERROR: encoded ViewInfoVector size % 3 != 0" << endl;
                return;
            }

            //
            //  Each ViewInfo is spread over 3 entries
            //

            for (size_t i = 0; i < parts.size(); i += 3)
            {
                TwkFB::FBInfo::ViewInfo vi;
                vi.name = parts[i];
                decodeLayerInfoVector(parts[i + 1], vi.layers);
                decodeChannelInfoVector(parts[i + 2], vi.otherChannels);
                v.push_back(vi);
            }
        }

        void decodeAudioChannelsVector(const string& buffer,
                                       TwkAudio::ChannelsVector& v)
        {
            vector<string> parts;
            split(parts, buffer, is_any_of(string("|")));

            if (buffer.size() == 0)
                return;

            for (size_t i = 0; i < parts.size(); i++)
            {
                v.push_back(TwkAudio::Channels(atoi(parts[i].c_str())));
            }
        }

    } // namespace

    //----------------------------------------------------------------------

    MovieSideCar::MovieSideCar(const string& sidecar, bool cloneable)
        : MovieReader()
        , m_sidecarPath(sidecar)
        , m_commandQueue(0)
        , m_responseQueue(0)
        , m_buffer(new char[Message::SizeInBytes()])
        , m_cloneable(cloneable)
        , m_sidecarProcess(0)
        , m_sharedObject(0)
        , m_sharedRegion(0)
        , m_sharedRegionSize(0)
        , m_canConvertAudio(false)
        , m_audioRegion(0)
        , m_imageRegion(0)
    {
    }

    MovieSideCar::~MovieSideCar()
    {
        removeMovieSideCars(this);

        try
        {
            shutdown();

            if (m_commandQueue)
            {
                Queue::remove(m_commandQueueName.c_str());
            }

            if (m_responseQueue)
            {
                Queue::remove(m_responseQueueName.c_str());
            }
        }
        catch (...)
        {
        }

        if (m_sharedObject)
        {
            delete m_sharedObject;
            m_sharedObject = 0;
        }

        if (m_sharedRegion)
        {
            delete m_sharedRegion;
            m_sharedRegion = 0;
        }

        if (m_sidecarProcess)
        {
            delete m_sidecarProcess;
            m_sidecarProcess = 0;
        }
    }

    Movie* MovieSideCar::clone() const
    {
        if (!m_cloneable)
            return 0;

        MovieSideCar* mov = new MovieSideCar(m_sidecarPath, true);

        if (filename() != "")
        {
            mov->open(filename());
        }

        return mov;
    }

    void MovieSideCar::sendCommand(const string& cmd, const string& args)
    {
        size_t size =
            Message::newMessage("app    ", &cout, cmd, args, m_buffer);

#ifdef TWK_SIDECAR_USE_QUEUE
        try
        {
            m_commandQueue->send(m_buffer, size, 0);
        }
        catch (std::exception& e)
        {
            cout << "ERROR: send queue threw " << e.what() << endl;
        }
        catch (...)
        {
            cout << "ERROR: send queue unknown exception" << endl;
        }
#else
        m_sidecarProcess->write(m_buffer, size);
        m_sidecarProcess->waitForBytesWritten();
#endif
    }

    StringPair MovieSideCar::waitForResponse()
    {
#ifdef TWK_SIDECAR_USE_QUEUE
        unsigned int priority;
        Queue::size_type returnSize;

#ifdef TWK_SIDECAR_USE_QUEUE_TIMEOUT
        size_t timeoutCount = 0;

        for (bool valid = false; !valid; valid = Message::isCommand(m_buffer))
        {
            ptime now = second_clock::local_time();
            ptime timeout = now + microsec(100);

            size_t sleepMicoSecs = 100;

            try
            {
                //
                //  Instead of spinning -- allow the sidecar to get some work
                //  done by sleeping for a bit. After checking for a message,
                //  if we don't hear back in time do another little sleep.
                //
                //  The sleep time will progressively diminish until 10
                //  microsecond sleeps occur.
                //

                boost::this_thread::sleep(microsec(sleepMicoSecs));

                *m_buffer = 0;
                returnSize = 0;

                bool timedout = !m_responseQueue->timed_receive(
                    m_buffer, Message::SizeInBytes(), returnSize, priority,
                    timeout);
                if (returnSize == 0)
                    m_buffer[0] = 0;

                if (!timedout && returnSize > 0
                    && !Message::isCommand(m_buffer))
                {
                    cout << "SIDECAR MESSAGE: " << m_buffer << std::flush;
                }

                if (timedout)
                {
                    timeoutCount++;
                    sleepMicoSecs = std::max(sleepMicoSecs / 2, size_t(10));
                }
            }
            catch (std::exception& e)
            {
                cout << "ERROR: response queue threw " << e.what() << endl;
            }
            catch (...)
            {
                cout << "ERROR: response queue unknown exception" << endl;
            }
        }
#else
        for (bool valid = false; !valid; valid = Message::isCommand(m_buffer))
        {
            try
            {
                *m_buffer = 0;
                returnSize = 0;

                m_responseQueue->receive(m_buffer, Message::SizeInBytes(),
                                         returnSize, priority);

                if (returnSize == 0)
                    m_buffer[0] = 0;

                if (returnSize > 0 && !Message::isCommand(m_buffer))
                {
                    cout << "SIDECAR MESSAGE: " << m_buffer << std::flush;
                }
            }
            catch (std::exception& e)
            {
                cout << "ERROR: response queue threw " << e.what() << endl;
            }
            catch (...)
            {
                cout << "ERROR: response queue unknown exception" << endl;
            }
        }
#endif

#else

        for (bool valid = false; !valid;)
        {
            do
            {
                m_sidecarProcess->waitForReadyRead();
            } while (!m_sidecarProcess->canReadLine());

            size_t readBytes = m_sidecarProcess->readLine(
                m_buffer, Message::SizeInBytes() - 1);

            if (Message::isCommand(m_buffer))
            {
                valid = true;
            }
            else if (readBytes > 0)
            {
                cout << "SIDECAR MESSAGE: " << m_buffer << std::flush;
            }
        }
#endif

        return Message::parseMessage("app    ", &cout, m_buffer);
    }

    void MovieSideCar::open(const string& filename, const MovieInfo& info,
                            const Movie::ReadRequest& request)
    {
        Lock lock(m_mutex);

        //
        //  Start by creating the queues
        //

        ostringstream cname;
        ostringstream rname;

        m_filename = filename;

        //
        //  Need to make semi-unique names for the queues.
        //

        boost::hash<string> string_hash;
        ostringstream str;
        str << m_sidecarPath << size_t(this) << TwkUtil::processID();
        size_t pathHash = string_hash(str.str());

        cname << "command" << hex << pathHash;
        rname << "response" << hex << pathHash;

        m_commandQueueName = cname.str();
        m_responseQueueName = rname.str();

        for (size_t tries = 0;; tries++)
        {
            try
            {
                m_commandQueue = new Queue(
                    interprocess::create_only, m_commandQueueName.c_str(),
                    Message::MaxInQueue(), Message::SizeInBytes());

                m_responseQueue = new Queue(
                    interprocess::create_only, m_responseQueueName.c_str(),
                    Message::MaxInQueue(), Message::SizeInBytes());

                break;
            }
            catch (interprocess_exception& e)
            {
                if (tries == 1)
                    throw;
                cout << "WARNING: MovieSideCar: removing old queues" << endl;
                Queue::remove(m_commandQueueName.c_str());
                Queue::remove(m_responseQueueName.c_str());
            }
        }

        //
        //  Launch the sidecar
        //

        ostringstream cmd;

        cmd << '"' << m_sidecarPath << '"' << " --command "
            << m_commandQueueName << " --response " << m_responseQueueName
            << " --pid " << TwkUtil::processID();

        m_sidecarProcess = new QProcess();
        m_sidecarProcess->start(cmd.str().c_str());

        if (!m_sidecarProcess->waitForStarted())
        {
            delete m_sidecarProcess;
            m_sidecarProcess = 0;
            TWK_THROW_STREAM(IOException,
                             "MovieSideCar: failed to start sidecar "
                                 << m_sidecarPath);
        }

        //
        //  Send ACK
        //

        sendCommand(Message::Acknowledge());
        StringPair msg = waitForResponse();
        string response = msg.first;
        string responseArg = msg.second;

        if (response != Message::OK())
        {
            shutdown();
            TWK_THROW_STREAM(
                IOException,
                "MovieSideCar: failed to get acknowledge from sidecar");
        }

        //
        //  Test whether we can even talk to this one
        //

        sendCommand(Message::ProtocolVersion(), "1");

        msg = waitForResponse();
        response = msg.first;
        responseArg = msg.second;

        if (response != Message::OK())
        {
            shutdown();
            cout << "ERROR: got " << response << endl;
            TWK_THROW_STREAM(
                IOException,
                "MovieSideCar: sidecar does not support this version");
        }

        //
        //  Send OPEN_FOR_READ command along with filename. If it works
        //  we'll be able to get the shared memory for the pixel data and
        //  open it.
        //

        sendCommand(Message::OpenForRead(), filename);

        msg = waitForResponse();
        response = msg.first;
        responseArg = msg.second;

        if (response == Message::Failed())
        {
            shutdown();
            TWK_THROW_STREAM(IOException, "MovieSideCar: failed to open file");
        }
        else if (response != Message::FileOpened())
        {
            shutdown();
            TWK_THROW_STREAM(
                IOException,
                "MovieSideCar: unexpected response to Message::FileOpened");
        }

        addMovieSideCars(this);

        decodeMemoryInfo(responseArg);

        //
        //  Send GET_INFO to get the MovieInfo data which we'll use to
        //  initialize the m_info field
        //

        sendCommand(Message::GetInfo());

        msg = waitForResponse();
        response = msg.first;
        responseArg = msg.second;

        {
            //
            //  Only sure way to get the proxy attrs is to get an actual
            //  image. So GetInfo command causes SideCar process to do just
            //  that. Here we're reading it and extracting only its attrs.
            //

            FrameBufferVector gfbs;
            TwkFB::IOgto gtoreader;
            TwkFB::FrameBufferIO::ReadRequest gfbrequest;
            gtoreader.readImageInMemory(gfbs, m_imageRegion, m_imageRegionSize,
                                        m_filename, gfbrequest);

            gfbs.front()->copyAttributesTo(&m_info.proxy);
            for (size_t i = 0; i < gfbs.size(); i++)
                delete gfbs[i];
        }

        deserializeInfo(responseArg);

        sendCommand(Message::CanConvertAudioRate());
        msg = waitForResponse();
        response = msg.first;

        m_canConvertAudio = response == Message::True();
    }

    void MovieSideCar::shutdown()
    {
        // don't lock here

        sendCommand(Message::Shutdown());
        StringPair v = waitForResponse();

        if (m_sidecarProcess)
        {
            if (v.first != Message::OK())
            {
                m_sidecarProcess->kill();
            }
            else
            {
                m_sidecarProcess->waitForFinished();
            }

            delete m_sidecarProcess;
            m_sidecarProcess = 0;
        }
    }

    bool MovieSideCar::canConvertAudioRate() const { return m_canConvertAudio; }

    void MovieSideCar::imagesAtFrame(const ReadRequest& request,
                                     FrameBufferVector& fbs)
    {
        string arg = Message::encodeMovieReadRequest(request);

        Lock lock(m_mutex);
        sendCommand(Message::ReadImage(), arg);

        StringPair r = waitForResponse();
        const string& response = r.first;
        const string& sarg = r.second;

        if (response == Message::OK())
        {
            // image size is arg
            istringstream iarg(sarg);
            size_t size;
            iarg >> size;

            TwkFB::IOgto reader;
            // TwkUtil::MemBuf mb(data.first, data.second);
            // istream instr(&mb);

            TwkFB::FrameBufferIO::ReadRequest fbrequest;

            // reader.readImageStream(fbs, instr, m_filename, fbrequest);
            reader.readImageInMemory(fbs, m_imageRegion, size, m_filename,
                                     fbrequest);

            for (size_t i = 0; i < fbs.size(); i++)
            {
                FrameBuffer* fb = fbs[i];
                fb->setIdentifier("");
                identifier(request.frame, fb->idstream());
                fb->idstream() << "/" << i;
            }
        }
        else if (response == Message::Throw())
        {
            TWK_THROW_STREAM(
                IOException,
                "MovieSideCar: sidecar threw on read: " << r.second);
        }
    }

    void MovieSideCar::identifier(int frame, std::ostream& o)
    {
        if (frame < m_info.start)
            frame = m_info.start;
        if (frame > m_info.end)
            frame = m_info.end;
        // o << hex << frame << dec << ":" << m_filename;
        o << frame << ":" << m_filename;
    }

    void MovieSideCar::identifiersAtFrame(const ReadRequest& request,
                                          IdentifierVector& ids)
    {
        ostringstream str;
        identifier(request.frame, str);
        str << "/" << 0;
        ids.push_back(str.str());
    }

    size_t MovieSideCar::audioFillBuffer(const AudioReadRequest& request,
                                         AudioBuffer& buffer)
    {

        ostringstream str;
        str << request.startTime << "|" << request.duration << "|"
            << request.margin;

        const long margin = request.startTime == 0 ? 0 : request.margin;
        const long start =
            long(request.startTime * m_info.audioSampleRate + .49) - margin;
        size_t num =
            long(request.duration * m_info.audioSampleRate + .49) + margin * 2;

        buffer.ownData();
        buffer.reconfigure(num - 2 * request.margin, m_info.audioChannels,
                           m_canConvertAudio ? buffer.rate()
                                             : m_info.audioSampleRate,
                           request.startTime, margin);

        Lock lock(m_mutex);
        sendCommand(Message::ReadAudio(), str.str());
        StringPair r = waitForResponse();

        if (r.first == Message::Throw())
        {
            TWK_THROW_STREAM(IOException, r.second);
        }
        else if (r.first != Message::OK())
        {
            buffer.zero();
        }
        else
        {
            //
            //  Read from shared area
            //

            memcpy(buffer.pointer(), m_audioRegion, buffer.sizeInBytes());
            return buffer.size();
        }

        return 0;
    }

    void MovieSideCar::audioConfigure(const AudioConfiguration& config)
    // MovieSideCar::audioConfigure(unsigned int channels,
    //                              TwkAudio::Time rate,
    //                              size_t bufferSize)
    {
        ostringstream str;
        str << config.layout << "|" << config.rate << "|" << config.bufferSize;

        Lock lock(m_mutex);
        sendCommand(Message::AudioConfigure(), str.str());
        StringPair r = waitForResponse();

        if (r.first == Message::Throw())
        {
            TWK_THROW_STREAM(IOException, r.second);
        }
        else if (r.first != Message::OK())
        {
            TWK_THROW_STREAM(IOException, "audio failed in sidecar");
        }
    }

    void MovieSideCar::flush()
    {
        Lock lock(m_mutex);
        sendCommand(Message::Flush());
        StringPair r = waitForResponse();

        if (r.first == Message::Throw())
        {
            TWK_THROW_STREAM(IOException, r.second);
        }
    }

// have to a little MACRO hackery to concatenate around a '.'
#define FIELD(X, Y) X.Y
#define PARSE_FIELD(INDEX, NAME)           \
    {                                      \
        istringstream istr(fields[INDEX]); \
        istr >> FIELD(m_info, NAME);       \
    }
#define PARSE_FIELD2(INDEX, NAME, TYPE)    \
    {                                      \
        istringstream istr(fields[INDEX]); \
        int i;                             \
        istr >> i;                         \
        FIELD(m_info, NAME) = (TYPE)i;     \
    }

    void MovieSideCar::deserializeInfo(const string& info)
    {
        vector<string> parts;
        split(parts, info, is_any_of(string("@")));

        assert(parts.size() == 5);

        vector<string> fields;
        split(fields, parts[0], is_any_of(string(",")));
        assert(fields.size() == 21);

        //
        //  first field is empty
        //

        PARSE_FIELD(1, width);
        PARSE_FIELD(2, height);
        PARSE_FIELD(3, uncropWidth);
        PARSE_FIELD(4, uncropHeight);
        PARSE_FIELD(5, uncropX);
        PARSE_FIELD(6, uncropY);
        PARSE_FIELD(7, pixelAspect);
        PARSE_FIELD(8, numChannels);
        PARSE_FIELD2(9, dataType, TwkFB::FrameBuffer::DataType);
        PARSE_FIELD2(10, orientation, TwkFB::FrameBuffer::Orientation);
        PARSE_FIELD(11, video);
        PARSE_FIELD(12, start);
        PARSE_FIELD(13, end);
        PARSE_FIELD(14, inc);
        PARSE_FIELD(15, fps);
        PARSE_FIELD(16, quality);
        PARSE_FIELD(17, audio);
        PARSE_FIELD(18, audioSampleRate);
        PARSE_FIELD(19, slowRandomAccess);
        m_info.defaultView = fields[20];

        decodeChannelInfoVector(parts[1], m_info.channelInfos);
        decodeViewInfoVector(parts[2], m_info.viewInfos);
        Message::decodeStringVector(parts[3], m_info.views);
        Message::decodeStringVector(parts[4], m_info.layers);
        decodeAudioChannelsVector(parts[5], m_info.audioChannels);
    }

    void MovieSideCar::decodeMemoryInfo(const string& info)
    {
        vector<string> parts;
        size_t imageOffet;
        size_t audioOffset;

        split(parts, info, is_any_of(string("|")));

        {
            istringstream istr(parts[0]);
            istr >> m_sharedObjectName;
        }
        {
            istringstream istr(parts[1]);
            istr >> imageOffet;
        }
        {
            istringstream istr(parts[2]);
            istr >> m_imageRegionSize;
        }
        {
            istringstream istr(parts[3]);
            istr >> audioOffset;
        }
        {
            istringstream istr(parts[4]);
            istr >> m_audioRegionSize;
        }

        m_sharedObject = new SharedMemory(interprocess::open_only,
                                          m_sharedObjectName.c_str(),
                                          interprocess::read_only);

        m_sharedRegion =
            new MappedRegion(*m_sharedObject, interprocess::read_only);

        m_imageRegion = ((char*)m_sharedRegion->get_address()) + imageOffet;
        m_audioRegion = ((char*)m_sharedRegion->get_address()) + audioOffset;
    }

} // namespace TwkMovie
