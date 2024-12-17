//
//  Copyright (c) 2012 Tweak Software.
//  All rights reserved.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
#include <MovieSideCar/SideCar.h>
#include <MovieSideCar/Common.h>
#include <IOgto/IOgto.h>
#include <TwkUtil/ProcessInfo.h>
#include <TwkUtil/MemBuf.h>
#include <TwkMovie/Movie.h>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/pair.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <algorithm>

namespace TwkMovie
{
    using namespace std;
    using namespace boost::program_options;
    using namespace boost::algorithm;
    using namespace boost;
    using namespace TwkFB;
    using namespace boost::posix_time;
    using namespace boost::gregorian;

    //----------------------------------------------------------------------
    //
    //  Utilties
    //

    namespace
    {

        string encodeChannelInfoVector(const FBInfo::ChannelInfoVector& v)
        {
            //
            //  encode a channel info as a string where the first char gives
            //  the type and the rest is the name. The strings are separated
            //  by commas.
            //

            ostringstream str;

            for (size_t i = 0; i < v.size(); i++)
            {
                if (i)
                    str << ",";
                const FBInfo::ChannelInfo& ci = v[i];
                char t = (unsigned int)ci.type + 'A';
                str << t << ci.name;
            }

            return str.str();
        }

        string encodeLayerInfoVector(const FBInfo::LayerInfoVector& v)
        {
            //
            //  encode a layer info as a string. The fields are separated by '`'
            //  and the layer infos themselves are also separated by '`' so that
            //  means each layer info is 2 fields
            //
            //      `name`channels`name`channels`name`channels`...
            //

            ostringstream str;

            for (size_t i = 0; i < v.size(); i++)
            {
                if (i)
                    str << "`";
                const FBInfo::LayerInfo& li = v[i];
                str << li.name << '`' << encodeChannelInfoVector(li.channels);
            }

            return str.str();
        }

        string encodeViewInfoVector(const FBInfo::ViewInfoVector& v)
        {
            ostringstream str;

            //
            //  this one makes three entries:
            //
            //      name|layers|otherChannels|name|layers|otherChannels|...
            //
            //  and each of these is also separated by "|" so if you split the
            //  string each view info is three components
            //

            for (size_t i = 0; i < v.size(); i++)
            {
                if (i)
                    str << "|";
                const FBInfo::ViewInfo& vi = v[i];
                str << vi.name << '|' << encodeLayerInfoVector(vi.layers) << '|'
                    << encodeChannelInfoVector(vi.otherChannels);
            }

            return str.str();
        }

        string encodeAudioChannelsVector(const TwkAudio::ChannelsVector& v)
        {
            ostringstream str;

            //
            //  this one makes three entries:
            //
            //      name|layers|otherChannels|name|layers|otherChannels|...
            //
            //  and each of these is also separated by "|" so if you split the
            //  string each view info is three components
            //

            for (size_t i = 0; i < v.size(); i++)
            {
                if (i)
                    str << "|";
                const TwkAudio::Channels ch = v[i];
                str << (int)ch;
            }

            return str.str();
        }

    } // namespace

    //----------------------------------------------------------------------

    // static ofstream fout("log");

    SideCar::SideCar(const string& cname, const string& rname, size_t pid,
                     MovieIO* io)
        : m_launchProcessPID(pid)
        , m_io(io)
        , m_commandQueue(interprocess::open_only, cname.c_str())
        , m_responseQueue(interprocess::open_only, rname.c_str())
        , m_buffer(new char[Message::SizeInBytes()])
        , m_sharedRegion(0)
        , m_sharedObject(0)
        , m_reader(0)
        , m_writer(0)
        , m_audioRegionSize(0)
        , m_audioRegion(0)
        , m_imageRegionSize(0)
        , m_imageRegion(0)
    {
        // cout.setf(ios::unitbuf);
        // cin.setf(ios::unitbuf);
    }

    SideCar::~SideCar()
    {
        //
        //  Delete shared memory
        //

        if (m_sharedObject)
        {
            SharedMemory::remove(m_sharedObjectName.c_str());
            delete m_sharedObject;
            delete m_sharedRegion;
        }

        delete m_reader;
        delete m_writer;
        delete[] m_buffer;
    }

    void SideCar::respond(const string& msg, const string& args)
    {
        // size_t size = Message::newMessage("sidecar", &fout, msg, args,
        // m_buffer);
        size_t size = Message::newMessage("sidecar", 0, msg, args, m_buffer);

#ifdef TWK_SIDECAR_USE_QUEUE
        m_responseQueue.send(m_buffer, size, 0);
#else
        cout << m_buffer;
        cout.rdbuf()->pubsync();
#endif
    }

    SideCar::StringPair SideCar::nextCommand()
    {
#ifdef TWK_SIDECAR_USE_QUEUE
        unsigned int priority;
        Queue::size_type returnSize;

#if TWK_SIDECAR_USE_QUEUE_TIMEOUT
        size_t timeoutCount = 0;

        //
        //  message_queue blocks using a spin lock on mac and windows. So to
        //  prevent burning up the CPU there's a timeout on the block of 1/4
        //  sec. If it times out it sleeps for 1 millisecond and then on the
        //  next attempt it timeouts in 1 millisecond and goes back to
        //  sleep. This keeps the thread asleep most of the time when nothing
        //  is happening. If RV is caching this thread will almost certainly
        //  get asked to load another frame before 1/4 second is up in which
        //  case it goes back to the spin lock for 1/4 second.
        //

        for (bool valid = false; !valid; valid = Message::isCommand(m_buffer))
        {
            ptime now = second_clock::local_time();
            ptime timeout = now + millisec(timeoutCount == 0 ? 250 : 1);

            *m_buffer = 0;
            returnSize = 0;

            bool timedout =
                !m_commandQueue.timed_receive(m_buffer, Message::SizeInBytes(),
                                              returnSize, priority, timeout);

            if (returnSize == 0)
                m_buffer[0] = 0;

            if (timedout)
            {
                boost::this_thread::sleep(millisec(
                    std::min(timeoutCount * timeoutCount, size_t(500))));
                timeoutCount++;
            }
        }
#else
        for (bool valid = false; !valid; valid = Message::isCommand(m_buffer))
        {
            *m_buffer = 0;
            returnSize = 0;

            m_commandQueue.receive(m_buffer, Message::SizeInBytes(), returnSize,
                                   priority);
            if (returnSize == 0)
                m_buffer[0] = 0;
        }
#endif

#else
        //
        //  Note cin is tied to cout by default
        //

        cout.rdbuf()->pubsync();

        for (bool valid = false; !valid; valid = Message::isCommand(m_buffer))
        {
            cin.getline(m_buffer, Message::SizeInBytes() - 1);
            cin.ignore();
        }
#endif

        // return Message::parseMessage("sidecar", &fout, m_buffer);
        return Message::parseMessage("sidecar", 0, m_buffer);
    }

    static int framenum = 0;

    void SideCar::run()
    {
        while (1)
        {
            StringPair p = nextCommand();
            string cmd = p.first;
            string arg = p.second;

            try
            {
                //
                //  This try block will respond with a Throw() message if it
                //  catches anything
                //

                if (cmd == Message::Shutdown())
                {
                    respond(Message::OK());
                    break; // breaks the run loop
                }
                else if (cmd == Message::Acknowledge())
                {
                    respond(Message::OK());
                }
                else if (cmd == Message::ProtocolVersion())
                {
                    //
                    //  Future: check for protocol version to make sure they
                    //  can talk to each other
                    //
                    respond(Message::OK());
                }
                else if (cmd == Message::OpenForRead())
                {
                    if (openForRead(arg))
                    {
                        ostringstream str;
                        str << m_sharedObjectName << "|" << 0 // image offset
                            << "|" << m_imageRegionSize << "|" << m_audioOffset
                            << "|" << m_audioRegionSize;

                        respond(Message::FileOpened(), str.str());
                    }
                    else
                    {
                        respond(Message::Failed());
                    }
                }
                else if (cmd == Message::GetInfo())
                {
                    string sinfo = serializeInfo();
                    assert(sinfo.size() < 4000);
                    respond(Message::OK(), sinfo); // There's a size limit
                                                   // for message queue
                                                   // use
                }
                else if (cmd == Message::ReadImage())
                {
                    FrameBufferVector fbs;
                    Movie::ReadRequest request(0);
                    Message::decodeMovieReadRequest(arg, request);

                    framenum = request.frame;

                    m_reader->imagesAtFrame(request, fbs);
                    size_t s = serializeImages(fbs);

                    ostringstream str;
                    str << s;

                    // delete them
                    for (size_t i = 0; i < fbs.size(); i++)
                        delete fbs[i];

                    respond(Message::OK(), str.str());
                }
                else if (cmd == Message::Flush())
                {
                    m_reader->flush();
                }
                else if (cmd == Message::AudioConfigure())
                {
                    decodeAudioConfigure(arg);
                    respond(Message::OK());
                }
                else if (cmd == Message::ReadAudio())
                {
                    size_t size = decodeAudioFillBuffer(arg);
                    ostringstream str;
                    str << size;
                    respond(Message::OK(), str.str());
                }
                else if (cmd == Message::CanConvertAudioRate())
                {
                    if (m_reader->canConvertAudioRate())
                    {
                        respond(Message::True());
                    }
                    else
                    {
                        respond(Message::False());
                    }
                }
                else
                {
                    //
                    //  We don't know what this command is
                    //

                    respond(Message::Failed());
                }
            }
            catch (std::exception& e)
            {
                respond(Message::Throw(), e.what());
            }
            catch (...)
            {
                respond(Message::Throw(), "unknown exception");
            }
        }

        //
        //  If we get here we're shutting down cleanly
        //
    }

    bool SideCar::openForRead(const string& filename)
    {
        //
        //  There's a single unique shared memory segment for this process so
        //  we only need to hash in the PID to its name
        //

        ostringstream name;
        name << "shmem" << hex << TwkUtil::processID();

        try
        {
            m_reader = m_io->movieReader();
            m_reader->open(filename);
            m_info = m_reader->info();
        }
        catch (...)
        {
            return false;
        }

        size_t pixelSize = m_info.width * m_info.height * m_info.numChannels;

        switch (m_info.dataType)
        {
        case FrameBuffer::UCHAR:
            break;
        case FrameBuffer::USHORT:
        case FrameBuffer::HALF:
            pixelSize *= sizeof(short);
            break;
        case FrameBuffer::FLOAT:
            pixelSize *= sizeof(float);
            break;
        case FrameBuffer::UINT:
        case FrameBuffer::PACKED_R10_G10_B10_X2:
        case FrameBuffer::PACKED_X2_B10_G10_R10:
        case FrameBuffer::PACKED_Cb8_Y8_Cr8_Y8:
        case FrameBuffer::PACKED_Y8_Cb8_Y8_Cr8:
            pixelSize *= sizeof(int);
            break;
        case FrameBuffer::DOUBLE:
            pixelSize *= sizeof(double);
            break;
        }

        //
        //  this is a total guess: 2 x num attributes assuming each one is
        //  256 bytes long
        //
        //  Note: for sub-sampled planar images this will overestimate. It
        //  assumes planar formats will report the planes in numChannels
        //  -- if this isn't true it will crash.
        //

        const size_t attrSize = m_info.proxy.attributes().size() * 256 * 2;
        const size_t imageSize = attrSize + pixelSize;
        const size_t audioSize = 4096 * m_info.audioChannels.size()
                                 * (m_info.audio ? 1 : 0) * sizeof(float);
        const size_t overhead =
            4096 * 11; // to account for unexpected huge attr sizes

        size_t totalSize = pixelSize + audioSize + overhead;
        totalSize -= totalSize % 4096;

        createSharedData(name.str(), pixelSize + audioSize + overhead);

        m_imageRegionSize = imageSize;
        m_imageRegion = m_sharedRegion->get_address();

        if (m_info.audio)
        {
            //
            //  Align to page boundary
            //

            m_audioRegionSize = audioSize + 4096 - audioSize % 4096;
            m_audioOffset = totalSize - m_audioRegionSize;
            m_audioRegion =
                ((char*)m_sharedRegion->get_address()) + m_audioOffset;
        }

        //
        //  Allocate the regions we're going to use once. The sidecar will
        //  own these for its lifetime
        //

        return true;
    }

    void SideCar::createSharedData(const string& name, size_t size)
    {
        //
        //  Create a managed shared memory object and map the entire thing
        //

        m_sharedObject = new SharedMemory(
            interprocess::create_only, name.c_str(), interprocess::read_write);
        m_sharedObjectName = name;
        m_sharedObject->truncate(size);

        m_sharedRegion =
            new MappedRegion(*m_sharedObject, interprocess::read_write);
        m_sharedRegionSize = size;
    }

#define COPY_FIELD(NAME) str << "," << m_info.NAME

    string SideCar::serializeInfo()
    {
        ostringstream str;

        COPY_FIELD(width);
        COPY_FIELD(height);
        COPY_FIELD(uncropWidth);
        COPY_FIELD(uncropHeight);
        COPY_FIELD(uncropX);
        COPY_FIELD(uncropY);
        COPY_FIELD(pixelAspect);
        COPY_FIELD(numChannels);
        COPY_FIELD(dataType);
        COPY_FIELD(orientation);
        COPY_FIELD(video);
        COPY_FIELD(start);
        COPY_FIELD(end);
        COPY_FIELD(inc);
        COPY_FIELD(fps);
        COPY_FIELD(quality);
        COPY_FIELD(audio);
        COPY_FIELD(audioSampleRate);
        COPY_FIELD(slowRandomAccess);
        COPY_FIELD(defaultView);

        string channelInfos = encodeChannelInfoVector(m_info.channelInfos);
        string viewInfos = encodeViewInfoVector(m_info.viewInfos);
        string views = Message::encodeStringVector(m_info.views);
        string layers = Message::encodeStringVector(m_info.layers);
        string audioChannels = encodeAudioChannelsVector(m_info.audioChannels);

        str << "@" << channelInfos << "@" << viewInfos << "@" << views << "@"
            << layers << "@" << audioChannels;

        //
        //  Write the proxy fb to the image area
        //

        vector<const FrameBuffer*> cfbs(1);
        cfbs[0] = &m_info.proxy;

        IOgto writer;
        FrameBufferIO::WriteRequest request;

        ostringstream buffer;
        writer.writeImageStream(cfbs, buffer, request);
        string bstr = buffer.str();
        size_t size = bstr.size();
        const char* s = bstr.c_str();

        char* p = reinterpret_cast<char*>(m_imageRegion);
        memcpy(p, s, size);

        return str.str();
    }

    size_t SideCar::serializeImages(const FrameBufferVector& fbs)
    {
        //
        //  Make an ostringstream out of the shared memory buffer
        //  eventually. For the time being we'll do an extra copy to get this
        //  moving.
        //

        vector<const FrameBuffer*> cfbs(fbs.size());
        std::copy(fbs.begin(), fbs.end(), cfbs.begin());

        //
        //  NOTE: on the mac its FASTER to make a local copy first then
        //  memcpy() the final data into the shared memory.
        //

        // #define NOCOPY

        IOgto writer;
        FrameBufferIO::WriteRequest request;

#ifdef NOCOPY
        size_t size = m_imageRegionSize;
#else
        ostringstream buffer;
        writer.writeImageStream(cfbs, buffer, request);
        string bstr = buffer.str();
        size_t size = bstr.size();
        const char* s = bstr.c_str();

        assert(size < m_imageRegionSize);
#endif

        char* p = reinterpret_cast<char*>(m_imageRegion);

        if (!p)
        {
            // cout << "ERROR: sidecar cannot allocated " << size << " bytes for
            // image transfer" << endl;
        }
#ifndef NOCOPY
        else
        {
            memcpy(p, s, size);
        }
#endif

#ifdef NOCOPY
        TwkUtil::MemBuf mb(p, size);
        ostream sharedStream(&mb);
        writer.writeImageStream(cfbs, sharedStream, request);
#endif

        return size;
    }

    void SideCar::decodeAudioConfigure(const string& arg)
    {
        unsigned int layoutArg;
        TwkAudio::Time rate;
        size_t bufferSize;

        istringstream iarg(arg);
        char pipe;

        iarg >> layoutArg >> pipe >> rate >> pipe >> bufferSize;

        TwkAudio::Layout layout = TwkAudio::Layout(layoutArg);
        m_audioBuffer.reconfigure(bufferSize, TwkAudio::layoutChannels(layout),
                                  rate);

        Movie::AudioConfiguration conf(rate, layout, bufferSize);
        m_reader->audioConfigure(conf);
    }

    size_t SideCar::decodeAudioFillBuffer(const string& arg)
    {
        TwkAudio::Time startTime;
        TwkAudio::Time duration;
        unsigned int channels;
        size_t margin;

        istringstream iarg(arg);
        char pipe;

        iarg >> pipe >> startTime >> pipe >> duration >> pipe >> margin;

        Movie::AudioReadRequest request(startTime, duration, margin);

        m_audioBuffer.reconfigure(m_audioBuffer.size(),
                                  m_audioBuffer.channels(),
                                  m_audioBuffer.rate(), startTime, margin);

        size_t n = m_reader->audioFillBuffer(request, m_audioBuffer);
        memcpy(m_audioRegion, m_audioBuffer.pointer(),
               m_audioBuffer.sizeInBytes());

        return n;
    }

} // namespace TwkMovie
