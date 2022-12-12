//
//  Copyright (c) 2014 Tweak Software. 
//  All rights reserved.
//  
//  SPDX-License-Identifier: Apache-2.0
//  
//
#include <TwkApp/VideoModule.h>
#include <TwkGLF/GLFBO.h>
#include <TwkAudio/Audio.h>
#include <IPCore/IPGraph.h>
#include <IPCore/IPNode.h>
#include <OutputVideoDevices/OutputVideoDeviceModule.h>
#include <limits>
#include <fstream>

#ifdef PLATFORM_WINDOWS
#include <winsock2.h>
#include <GL/gl.h>
#include <GL/glu.h>
#define DEFAULT_RINGBUFFER_SIZE 4
#endif

#ifdef PLATFORM_LINUX
#include <GL/gl.h>
#define DEFAULT_RINGBUFFER_SIZE 3
#endif

#ifdef PLATFORM_DARWIN
#define DEFAULT_RINGBUFFER_SIZE 4
#endif

#include <stl_ext/replace_alloc.h>
#include <OutputVideoDevices/OutputVideoDevice.h>
#include <TwkExc/Exception.h>
#include <TwkUtil/TwkRegEx.h>
#include <TwkUtil/ProcessInfo.h>
#include <TwkUtil/Timer.h>
#include <TwkUtil/FrameUtils.h>
#include <TwkUtil/ThreadName.h>
#include <algorithm>
#include <string>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <TwkUtil/ByteSwap.h> // this needs to come after boost incls for vc12

#define GC_EXCLUDE_THIS_THREAD 

namespace OutputVideoDevices {
using namespace std;
using namespace TwkApp;
using namespace TwkGLF;
using namespace TwkUtil;
using namespace boost::program_options;
using namespace boost::algorithm;
using namespace boost;

namespace {
bool m_infoFeedback = false;

string
mapToEnvVar(string name)
{
    if (name == "TWK_OUTPUT_HELP") return "help";
    if (name == "TWK_OUTPUT_VERBOSE") return "verbose";
    if (name == "TWK_OUTPUT_PROFILE") return "profile";
    if (name == "TWK_OUTPUT_METHOD") return "method";
    if (name == "TWK_OUTPUT_RING_BUFFER_SIZE") return "ring-buffer-size";
    return "";
}

}

// //
// //  Movie API is used to generically read from the
// //  OutputVideoDevice queues
// //

// class MovieInterface : public TwkMovie::Movie
// {
//   public:
//     MovieInterface(OutputVideoDevice* d) : device(d) {}
//     virtual ~MovieInterface() {}
//     virtual void imagesAtFrame(const ReadRequest&, FrameBufferVector&);
//     virtual size_t audioFillBuffer(const AudioReadRequest&, AudioBuffer&);
//     OutputVideoDevice* device;
// };


OutputVideoDevice::VideoChannel::VideoChannel(size_t bsize, size_t n)
    : data(n),
      bufferSizeInBytes(bsize)
{
}

OutputVideoDevice::VideoChannel::~VideoChannel()
{
}

OutputVideoDevice::FrameData::FrameData() 
  : fbo(0),
    globject(0),
    imageData(0),
    state(NotReady),
    mappedPointer(0),
    fence(0),
    locked(false)
{
}

OutputVideoDevice::FrameData::FrameData(const OutputVideoDevice::FrameData& f) 
  : fbo(f.fbo),
    globject(f.globject),
    imageData(f.imageData),
    state(f.state),
    mappedPointer(f.mappedPointer),
    fence(f.fence),
    locked(false)
{
}

OutputVideoDevice::FrameData& 
OutputVideoDevice::FrameData::operator= (const OutputVideoDevice::FrameData& f)
{
    fbo               = f.fbo;
    globject          = f.globject;
    imageData         = f.imageData;
    state             = f.state;
    mappedPointer     = f.mappedPointer;
    fence             = f.fence;
    locked            = false;
    return *this;
}


OutputVideoDevice::FrameData::~FrameData() 
{ 
}

void 
OutputVideoDevice::FrameData::lockImage(const char* threadName) 
{ 
    Timer timer;
    timer.start();
    imageMutex.lock();
    Time t = timer.elapsed();
    if (t > 0.001 && m_infoFeedback) cerr << "INFO: " << threadName << ": lockImage for " << t << endl;
}

void OutputVideoDevice::FrameData::unlockImage()
{ 
    imageMutex.unlock();
}

void 
OutputVideoDevice::FrameData::lockState(const char* threadName) 
{
    Timer timer;
    timer.start();
    stateMutex.lock();
    Time t = timer.elapsed();
    if (t > 0.001 && m_infoFeedback) cerr << "INFO: " << threadName << ": lockState for " << t << endl;
}

void 
OutputVideoDevice::FrameData::unlockState() 
{ 
    stateMutex.unlock();
}

OutputVideoDevice::OutputVideoDevice(OutputVideoDeviceModule* m, const string& name, IPCore::IPGraph* graph)
    : GLBindableVideoDevice(m, name, 
                            BlockingTransfer |
                            ASyncReadBack |
                            ImageOutput | 
                            ProvidesSync | 
                            FixedResolution | 
                            NormalizedCoordinates |
                            Clock), // NOTE: no AudioOutput flag here on purpose
    m_channels(3),
    m_width(1920),
    m_height(1080),
    m_bits(8),
    m_float(false),
    m_fps(24.0f),
    m_profile(false),
    m_frameStart(1),
    m_frameEnd(1),
    m_quality(1.0f),
    m_writerThreads(1),
    m_ringBufferSize(DEFAULT_RINGBUFFER_SIZE),
    m_bufferSize(0),
    m_bufferSizeInBytes(0),
    m_bufferStride(0),
    m_pixelAspect(1.0f),
    m_pixelScale(1.0f),
    m_open(false),
    m_stereo(false),
    m_bound(false),
    m_threadStop(false),
    m_threadDone(false),
    m_pbos(true),
    m_texturePadding(0),
    m_mappedBufferCount(0),
    m_readBufferIndex(0),
    m_readBufferCount(0),
    m_writeBufferIndex(0),
    m_writeBufferCount(0),
    m_transferThread(0),
    m_writerThread(0),
    m_internalDataFormat(VideoDevice::RGB8),
    m_fboInternalFormat(GL_RGB8),
    m_textureFormat(GL_RGB),
    m_textureType(GL_UNSIGNED_BYTE),
    m_graph(graph),
    m_audioInit(true),
#ifdef PLATFORM_DARWIN
    m_immediateCopy(false)
#else
    m_immediateCopy(true)
#endif
{
}

OutputVideoDevice::~OutputVideoDevice()
{
}

void 
OutputVideoDevice::open(const StringVector& args)
{
    m_writer      = 0;
    m_open        = true;
    m_width       = 1920;
    m_height      = 1080;
    m_bits        = 8;
    m_float       = false;
    m_fps         = 24;
    m_pixelAspect = 1.0;
    m_pixelScale  = 1.0;

    m_pbos        = true;

    options_description desc("Output Device Options");

    desc.add_options()
        ("help,h", "Usage Message")
        ("verbose,v", "Verbose")
        ("codec", value<string>(), "Output Image Codec")
        ("audio-codec", value<string>(), "Output Audio Codec")
        ("audio-rate", value<float>(), "Output Audio Sample Rate")
        ("audio-layout", value<int>(), "Output Audio Channel Layout")
        ("has-audio", value<int>(), "Has Audio")
        ("quality", value<float>(), "Encoding Quality (float)")
        ("comments", value<string>(), "Comments")
        ("copyright", value<string>(), "Copyright")
        ("output,o", value<string>(), "Output File")
        ("args", value<StringVector>()->multitoken(), "Output Writer Arguments")
        ("chapterRanges", value<IntVector>()->multitoken(), "Chapter In/Out Pairs")
        ("chapterTitles", value<StringVector>()->multitoken(), "Chapter Titles")
        ("frame-start", value<int>(), "Frame Start")
        ("frame-end", value<int>(), "Frame End")
        ("size,s", value< vector<int> >()->multitoken(), "Output Resolution WIDTH HEIGHT")
        ("format,f", value<string>(), "Format (RGB[A] + {8,10,12,16,32} + [F], e.g. RGB8)")
        ("yuv", value<string>(), "Output YUV sampling (e.g. 4:2:2)")
        ("fps,f", value<float>(), "Output FPS")
        ("pixel-aspect,a", value<float>(), "Output Pixel Aspect Ratio")
        ("profile,p", "Output Debugging Profile (twk_output_profile_<ID>.dat)")
        ("method,m", value<string>(), "Method (dvp, sdvp, ipbo, ppbo, basic, p2p)")
        ("threads", value<int>(), "Writer Threads")
        ("ring-buffer-size,s", value<int>()->default_value(DEFAULT_RINGBUFFER_SIZE), "Ring Buffer Size");

    variables_map vm;

    try
    {
        store(command_line_parser(args).options(desc).run(), vm);
        store(parse_environment(desc, mapToEnvVar), vm);
        notify(vm);
    }
    catch (std::exception& e)
    {
        cerr << "ERROR: OUTPUT_ARGS: " << e.what() << endl;
    }
    catch (...) 
    {
        cerr << "ERROR: OUTPUT_ARGS: exception" << endl;
    }

    if (vm.count("help") > 0)
    {
        cerr << endl << desc << endl;
        TWK_THROW_EXC_STREAM("--help");
    }

    if (vm.count("size") > 0)
    {
        IntVector sizes = vm["size"].as<IntVector>();
        if (sizes.size() != 2) TWK_THROW_EXC_STREAM("--size WIDTH HEIGHT is required");

        m_width  = sizes[0];
        m_height = sizes[1];

        if (m_width == 0 || m_height == 0)
        {
            TWK_THROW_EXC_STREAM("--size WIDTH HEIGHT sizes must be non-0");
        }
    }

    if (vm.count("fps") > 0)
    {
        m_fps = vm["fps"].as<float>();
        if (m_fps <= 0.0) TWK_THROW_EXC_STREAM("--fps FPS invalid value");
    }

    if (vm.count("pixel-aspect") > 0)
    {
        m_pixelAspect = vm["pixel-aspect"].as<float>();
        if (m_pixelAspect <= 0.0) TWK_THROW_EXC_STREAM("--pixel-aspect FLOAT invalid value");
    }

    string dataType = "RGB8";
    m_float         = false;
    m_channels      = 3;
    m_bits          = 8;

    if (vm.count("format") > 0) dataType = vm["format"].as<string>();

    if (dataType == "RGB8") { m_bits = 8; m_channels = 3; }
    else if (dataType == "RGBA8") { m_bits = 8; m_channels = 4; }
    else if (dataType == "RGB10") { m_bits = 10; m_channels = 3; }
    else if (dataType == "RGB12") { m_bits = 12; m_channels = 3; }
    else if (dataType == "RGB16") { m_bits = 16; m_channels = 3; }
    else if (dataType == "RGBA16") { m_bits = 16; m_channels = 4; }
    else if (dataType == "RGB16F") { m_bits = 16; m_channels = 3; m_float = true; }
    else if (dataType == "RGBA16F") { m_bits = 16; m_channels = 4; m_float = true; }
    else if (dataType == "RGB32F") { m_bits = 32; m_channels = 3; m_float = true; }
    else if (dataType == "RGBA32F") { m_bits = 32; m_channels = 4; m_float = true; }

    m_filename = vm.count("output") > 0 ? vm["output"].as<string>() : string("out.#.tif");

    if (!(m_writer = TwkMovie::GenericIO::movieWriter(m_filename)))
    {
        m_open = false;
        TWK_THROW_EXC_STREAM("Failed to open output video device " << name());
    }

    //
    //  Get the rest of the options
    //

    m_infoFeedback  = vm.count("verbose") > 0;
    m_profile       = vm.count("profile") > 0;
    m_codec         = vm.count("codec") > 0 ? vm["codec"].as<string>() : "";
    m_audioCodec    = vm.count("audio-codec") > 0 ? vm["audio-codec"].as<string>() : "";
    m_audioRate     = vm.count("audio-rate") > 0 ? vm["audio-rate"].as<float>() : 48000.0;
    m_comments      = vm.count("comments") > 0 ? vm["comments"].as<string>() : "";
    m_copyright     = vm.count("copyright") > 0 ? vm["copyright"].as<string>() : "";
    m_quality       = vm.count("quality") > 0 ? vm["quality"].as<float>() : 1.0;
    m_writerThreads = vm.count("threads") > 0 ? vm["threads"].as<int>() : 1;
    m_frameStart    = vm.count("frame-start") > 0 ? vm["frame-start"].as<int>() : 1;
    m_frameEnd      = vm.count("frame-end") > 0 ? vm["frame-end"].as<int>() : m_frameStart;
    m_audioLayout   = vm.count("audio-layout") > 0 ?
        (TwkAudio::Layout) vm["audio-layout"].as<int>() : TwkAudio::Stereo_2;

    if (vm.count("args") > 0)
    {
        StringVector args = vm["args"].as<StringVector>();

        for (size_t i = 0; i < args.size(); i++)
        {
            StringVector buffer;
            algorithm::split(buffer, args[i], is_any_of(string("=")), token_compress_on);

            if (buffer.size() == 1)
            {
                m_writerArgs.push_back(StringPair(buffer[0], ""));
            }
            else if (buffer.size() > 1)
            {
                m_writerArgs.push_back(StringPair(buffer[0], buffer[1]));
            }
        }
    }
    
    if (vm.count("chapterRanges") > 0 && vm.count("chapterTitles") > 0)
    {
        IntVector ranges = vm["chapterRanges"].as<IntVector>();
        StringVector titles = vm["chapterTitles"].as<StringVector>();

        if (ranges.size() != (2 * titles.size()))
        {
            m_open = false;
            TWK_THROW_EXC_STREAM("Missmatched number of chapterRanges and chapterTitles");
        }

        m_info.chapters.resize(titles.size());
        for (size_t i = 0; i < titles.size(); i++)
        {
            m_info.chapters[i].title = titles[i];
            m_info.chapters[i].startFrame = ranges[(i*2)];
            m_info.chapters[i].endFrame = ranges[(i*2) + 1];
        }
    }


    initializeDataFormats();

    //
    //  The MovieWriter will examine our m_info for some of its
    //  params so initialize it here.
    //

    m_info.audio           = vm.count("has-audio") > 0 ? bool(vm["has-audio"].as<int>()) : false;
    m_info.audioSampleRate = m_audioRate;
    m_info.audioChannels   = layoutChannels(m_audioLayout);
    m_info.video           = true;
    m_info.start           = m_frameStart;
    m_info.end             = m_frameEnd;
    m_info.fps             = m_fps;
    m_info.inc             = 1;
    m_info.width           = m_width;
    m_info.height          = m_height;
    m_info.pixelAspect     = m_pixelAspect;
    m_info.uncropWidth     = m_width;
    m_info.uncropHeight    = m_height;
    m_info.uncropX         = 0;
    m_info.uncropY         = 0;
    m_info.numChannels     = m_channels;
    m_info.dataType        = m_dataType;

    //
    //  Find out what the writer wants
    //

    m_writerInfo = m_writer->open(m_info, m_filename, writeRequestFromState());

    //
    //  XXX fields in m_writerInfo filled in by the writer should now
    //  override: m_channels, m_dataType, m_internalDataFormat.  
    //
    //  At a later date: m_width, m_height, possibly m_pixelAspect.
    //

    switch (m_writerInfo.dataType)
    {
        case TwkFB::FrameBuffer::BIT:
            m_bits = 1;
            m_float = false;
            break;
        case TwkFB::FrameBuffer::UCHAR:
            m_bits = 8;
            m_float = false;
            break;
        default:
        case TwkFB::FrameBuffer::USHORT:
            m_bits = 16;
            m_float = false;
            break;
        case TwkFB::FrameBuffer::UINT:
            m_bits = 32;
            m_float = false;
            break;
        case TwkFB::FrameBuffer::HALF:
            m_bits = 16;
            m_float = true;
            break;
        case TwkFB::FrameBuffer::FLOAT:
            m_bits = 32;
            m_float = true;
            break;
        case TwkFB::FrameBuffer::DOUBLE:
            m_bits = 64;
            m_float = true;
            break;
        case TwkFB::FrameBuffer::PACKED_R10_G10_B10_X2:
        case TwkFB::FrameBuffer::PACKED_X2_B10_G10_R10:
            m_bits = 10;
            m_float = false;
            break;
        case TwkFB::FrameBuffer::PACKED_Cb8_Y8_Cr8_Y8:
        case TwkFB::FrameBuffer::PACKED_Y8_Cb8_Y8_Cr8:
            m_bits = 8;
            m_float = false;
            break;
    }
    m_channels = m_writerInfo.numChannels;

    initializeDataFormats();

    if (m_dataType != m_writerInfo.dataType)
    {
        TWK_THROW_EXC_STREAM("Internal Data Format and writer goal format do not match!");
    }

    if (m_channels != m_writerInfo.numChannels)
    {
        TWK_THROW_EXC_STREAM("Number of channels and writer goal format do not match!");
    }

    m_channelNames.resize(m_channels);

    switch (m_channels)
    {
      case 1:
        if (m_bits == 10) m_channelNames[0] = "RGB";
        break;
      default:
      case 3:
          m_channelNames[0] = "R";
          m_channelNames[1] = "G";
          m_channelNames[2] = "B";
          break;
      case 4:
          m_channelNames[0] = "R";
          m_channelNames[1] = "G";
          m_channelNames[2] = "B";
          m_channelNames[3] = "A";
          break;
    }

    //
    //  Internal state
    //

    m_bufferSize        = m_width * m_height * m_channels;
    m_bufferSizeInBytes = m_width * m_height * pixelSizeInBytes(m_internalDataFormat);
    m_bufferStride      = m_width *            pixelSizeInBytes(m_internalDataFormat);

    if (m_infoFeedback) cerr << "INFO: " << m_width << "x" << m_height << "@" << m_fps << endl;

    m_fboInternalFormat = TwkGLF::internalFormatFromDataFormat(m_internalDataFormat);
    GLenumPair epair    = TwkGLF::textureFormatFromDataFormat(m_internalDataFormat);
    m_textureFormat     = epair.first;
    m_textureType       = epair.second;
    m_texturePadding    = 0;
    if (m_textureType == GL_UNSIGNED_INT_10_10_10_2) m_textureType = GL_UNSIGNED_INT_2_10_10_10_REV;

    m_videoChannels.clear();

    m_videoChannels.push_back(new VideoChannel(m_bufferSizeInBytes, m_ringBufferSize));

    if (m_stereo)
    {
        m_videoChannels.push_back(new VideoChannel(m_bufferSizeInBytes, m_ringBufferSize));
    }
}

void 
OutputVideoDevice::close()
{
    if (m_open)
    {
        if (m_bound) unbind();
        m_open = false;
    }

    TwkGLF::GLBindableVideoDevice::close();
}

namespace {

struct ThreadTrampoline
{
    ThreadTrampoline(OutputVideoDevice* d, bool tthread) 
        : device(d), transferThread(tthread) {}
    void operator()() 
    {
        GC_EXCLUDE_THIS_THREAD;

        if (transferThread)
        {
            setThreadName("OVD Transfer");
            device->transferMain(); 
        }
        else
        {
            setThreadName("OVD Writer");
            device->writerMain();
        }
    }

    bool transferThread;
    OutputVideoDevice* device;
};

}

VideoDevice::Time OutputVideoDevice::inputTime() const { return 0; }
bool OutputVideoDevice::isOpen() const { return m_open; }
void OutputVideoDevice::beginTransfer() const { } 
void OutputVideoDevice::endTransfer() const { } 
void OutputVideoDevice::makeCurrent() const { }
void OutputVideoDevice::clearCaches() const { }
void OutputVideoDevice::syncBuffers() const { }

VideoDevice::Time 
OutputVideoDevice::outputTime() const
{
    lockDevice(true, "OUTPUT TIME");
    Time t = VideoDevice::outputTime();
    lockDevice(false);
    return t;
}

bool 
OutputVideoDevice::isStereo() const
{
    return false;
}

bool 
OutputVideoDevice::isDualStereo() const
{
    return false;
}

bool 
OutputVideoDevice::willBlockOnTransfer() const
{
    return false;
}

void 
OutputVideoDevice::bind(const TwkGLF::GLVideoDevice* device) const
{
    if (!m_open) return;

    m_threadStop      = false;
    m_threadDone      = false;

    m_gpuTimes.clear();
    m_transferTimes.clear();
    m_globalTimer.start();


    if (m_pbos)
    {
        //
        //  Generate the PBO buffers
        //

        for (size_t q = 0; q < m_videoChannels.size(); q++)
        {
            VideoChannel* vc = m_videoChannels[q];
            
            for (size_t i = 0; i < m_ringBufferSize; i++)
            {
                FrameData& f = vc->data[i];
                
                glGenBuffers(1, &f.globject); TWK_GLDEBUG;
                glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, f.globject); TWK_GLDEBUG;
                glBufferData(GL_PIXEL_PACK_BUFFER_ARB, m_bufferSizeInBytes, NULL, GL_DYNAMIC_READ); TWK_GLDEBUG;
                glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0); TWK_GLDEBUG;
                f.state = FrameData::Ready;
            }
        }
    }
    else  // readpixels
    {
        for (size_t q = 0; q < m_videoChannels.size(); q++)
        {
            VideoChannel* vc = m_videoChannels[q];
            
            for (size_t i = 0; i < m_ringBufferSize; i++)
            {
                FrameData& f = vc->data[i];
                f.state = FrameData::Ready;
            }
        }
    }

    resetClock();
    m_readBufferIndex  = 0;
    m_readBufferCount  = 0;
    m_writeBufferIndex = 0;
    m_writeBufferCount = 0;
    m_bound            = true;

    m_transferThread = new Thread(ThreadTrampoline(const_cast<OutputVideoDevice*>(this), true));
    m_writerThread   = new Thread(ThreadTrampoline(const_cast<OutputVideoDevice*>(this), false));
}

void 
OutputVideoDevice::bind2(const TwkGLF::GLVideoDevice* device0, 
                         const TwkGLF::GLVideoDevice* device1) const
{
    bind(device0);
}

void 
OutputVideoDevice::unbind() const
{
    if (!m_bound) return;

    //
    //  If we're using PBOs there may be a transfer in progress.  This means
    //  transferMain() will be blocked waiting to transfer to the queue, and
    //  the writer thread will be blocked waiting for the queue to have
    //  something in it.
    //

    if (m_pbos)
    {
                      finalizeOutstandingPBOTransfer(m_videoChannels[0]);
        if (m_stereo) finalizeOutstandingPBOTransfer(m_videoChannels[1]);
    }

    //
    //  Now there should be nothing blocking writer thread, so wait for it to
    //  complete.
    //

    m_writerThread->join();

    m_globalTimer.stop();

    //
    //  Shutdown transfer thread
    //

    lockDevice(true, "UNBIND");
    m_threadStop = true;
    lockDevice(false);

    lockDevice(true, "UNBIND");
    bool threadDone = m_threadDone;
    lockDevice(false);

    //
    //  Unlock any images that were locked by the reader thread.
    //

    for (size_t q = 0; q < m_videoChannels.size(); q++)
    {
        VideoChannel* vc = m_videoChannels[q];
        
        for (size_t i = 0; i < vc->data.size(); i++)
        {
            //
            //  Careful with the locking order here. If the thread is
            //  in the middle of a dvp transfer, or other operation,
            //  we don't want to pull the rug out from under it until
            //  after its completed.
            //

            FrameData& f = vc->data[i];
            f.lockState("UNBIND");
            if (f.state == FrameData::Reading && f.locked) f.unlockImage();
            f.unlockState();
            f.lockImage("UNBIND");
            f.fbo = 0;
            f.unlockImage();
        }
    }

    if (!threadDone) 
    {
        m_transferThread->join();
        delete m_transferThread;
        m_transferThread = 0;

        m_writerThread->join();
        delete m_writerThread;
        m_writerThread = 0;
    }

    //
    //  Delete all allocated resources
    //

    for (size_t q = 0; q < m_videoChannels.size(); q++)
    {
        VideoChannel* vc = m_videoChannels[q];
        
        for (size_t i = 0; i < vc->data.size(); i++)
        {
            FrameData& f = vc->data[i];

            if (m_pbos && !m_immediateCopy && f.state == FrameData::Mapped)
            {
                glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, f.globject); TWK_GLDEBUG;
                glUnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB); TWK_GLDEBUG;
                f.globject = 0;
            }

            if (m_pbos)
            {
                glDeleteBuffers(1, &f.globject); TWK_GLDEBUG;
            }

            if (f.imageData) 
            {
                TWK_DEALLOCATE_ARRAY(f.imageData);
                f.imageData = 0;
            }
        }
    }

    m_bound = false;

    if (m_profile)
    {
        if (!m_gpuTimes.empty())
        {
            Time accumTime = 0;
            Time minTime = numeric_limits<double>::max();
            Time maxTime = -numeric_limits<double>::max();
        
            for (size_t i = 5; i < m_gpuTimes.size(); i++)
            {
                Time t = m_gpuTimes[i];
                accumTime += t;
                minTime = std::min(minTime, t);
                maxTime = std::max(maxTime, t);
            }

            cerr << "INFO: GPU: " << (accumTime / m_gpuTimes.size()) 
                 << ", min=" << minTime 
                 << ", max=" << maxTime 
                 << ", count=" << m_gpuTimes.size()
                 << endl;
        }

        if (!m_transferTimes.empty())
        {
            Time accumTime = 0;
            Time minTime = numeric_limits<double>::max();
            Time maxTime = -numeric_limits<double>::max();
        
            for (size_t i = 5; i < m_transferTimes.size(); i++)
            {
                Time t = m_transferTimes[i];
                accumTime += t;
                minTime = std::min(minTime, t);
                maxTime = std::max(maxTime, t);
            }

            cerr << "INFO: TRANSFER: " << (accumTime / m_transferTimes.size()) 
                 << ", min=" << minTime 
                 << ", max=" << maxTime 
                 << ", count=" << m_transferTimes.size()
                 << endl;
        }

        if (!m_gpuTimes.empty() && !m_transferTimes.empty())
        {
            ostringstream filename;
            filename << "twk_aja_profile_" << TwkUtil::processID() << ".csv";
            ofstream file(filename.str().c_str());
            size_t count = m_gpuTimes.size();

            file << "GPUStart,GPUDuration,NTV2Begin,NTV2Duration" << endl;

            for (size_t i = 0, s = std::min(m_gpuTimes.size(), m_transferTimes.size()); i < s; i++)
            {
                file << m_gpuBeginTime[i] << ","
                     << m_gpuTimes[i] << "," 
                     << m_transferBeginTime[i] << ","
                     << m_transferTimes[i] << endl;
            }
        }
    }
}

void
OutputVideoDevice::transferChannel(size_t n, const GLFBO* fbo) const
{
    VideoChannel* vc = m_videoChannels[n];
    FrameData& fdread = vc->data[m_readBufferIndex];

    fbo->bind();
    fbo->beginExternalReadback();

    if (m_pbos) transferChannelPBO(vc, fbo);
    else        transferChannelReadPixels(vc, fbo);
}


void 
OutputVideoDevice::transfer(const TwkGLF::GLFBO* fbo) const
{
    if (!m_open) return;

    transferChannel(0, fbo);

    lockDevice(true, "READER in transfer");
    m_readBufferCount++;
    m_readBufferIndex = m_readBufferCount % m_ringBufferSize;
    incrementClock();

    lockDevice(false);
}

void 
OutputVideoDevice::transfer2(const TwkGLF::GLFBO* fbo0,
                             const TwkGLF::GLFBO* fbo1) const
{
}

size_t 
OutputVideoDevice::asyncMaxMappedBuffers() const
{
    return m_ringBufferSize;
}

VideoDevice::Time 
OutputVideoDevice::deviceLatency() const
{
    return Time(0.0);
}

void 
OutputVideoDevice::lockDevice(bool lock, const char* threadName) const
{
    if (lock) 
    {
        Timer timer;
        timer.start();
        m_deviceMutex.lock();
        Time t = timer.elapsed();
        if (t > 0.001 && m_infoFeedback) cerr << "INFO: " << threadName << ": lockDevice for " << t << endl;
    }
    else 
    {
        m_deviceMutex.unlock();
    }
}

size_t OutputVideoDevice::numVideoFormats() const { return 1; }
size_t OutputVideoDevice::currentVideoFormat() const { return 0; }
void OutputVideoDevice::setVideoFormat(size_t i) { } 
size_t OutputVideoDevice::numDataFormats() const { return 1; }
void OutputVideoDevice::setDataFormat(size_t i) { } 
size_t OutputVideoDevice::currentDataFormat() const { return 0; }
size_t OutputVideoDevice::numSyncModes() const { return 0; }
OutputVideoDevice::SyncMode OutputVideoDevice::syncModeAtIndex(size_t i) const { return SyncMode(""); }
void OutputVideoDevice::setSyncMode(size_t i) { }
size_t OutputVideoDevice::currentSyncMode() const { return 0; }
size_t OutputVideoDevice::numSyncSources() const { return 0; }
void OutputVideoDevice::setSyncSource(size_t i) { }
size_t OutputVideoDevice::currentSyncSource() const { return 0; }

OutputVideoDevice::VideoFormat
OutputVideoDevice::videoFormatAtIndex(size_t index) const
{
    return VideoFormat(m_width, m_height, m_pixelAspect, m_pixelScale, m_fps, "");
}

OutputVideoDevice::DataFormat 
OutputVideoDevice::dataFormatAtIndex(size_t i) const
{
    return DataFormat(m_internalDataFormat, "");
}

OutputVideoDevice::SyncSource 
OutputVideoDevice::syncSourceAtIndex(size_t i) const
{
    return SyncSource("");
}

size_t OutputVideoDevice::numAudioFormats() const { return 1; }
size_t OutputVideoDevice::currentAudioFormat() const { return 0; }
void OutputVideoDevice::setAudioFormat(size_t i) { }

void
OutputVideoDevice::audioFrameSizeSequence(AudioFrameSizeVector& fsizes) const
{
}

OutputVideoDevice::AudioFormat
OutputVideoDevice::audioFormatAtIndex(size_t index) const
{
    return AudioFormat();
}

VideoDevice::Resolution 
OutputVideoDevice::resolution() const
{
    return Resolution(m_width, m_height, m_pixelAspect, m_pixelScale);
}

VideoDevice::Timing 
OutputVideoDevice::timing() const
{
    return Timing(m_fps);
}

VideoDevice::VideoFormat 
OutputVideoDevice::format() const
{
    return VideoFormat(m_width, m_height, m_pixelAspect, m_pixelScale, m_fps, "");
}

size_t 
OutputVideoDevice::width() const
{
    return m_width; 
}

size_t 
OutputVideoDevice::height() const
{
    return m_height;
}

void 
OutputVideoDevice::transferAudio(void* interleavedData, size_t n) const
{
}

namespace {

//
//  In order to do this with a shader and have it be efficient we have to
//  write the shader to completely pack the subsampled data with no
//  scanline padding. Until we have that this will suffice.
//
//  NOTE: this is discarding every other chroma sample instead of
//  interpolating. Not the best way to do this. 
//

#ifdef _MSC_VER
#define restrict
#else
// Ref.: https://en.wikipedia.org/wiki/Restrict
// C++ does not have standard support for restrict, but many compilers have equivalents that
// usually work in both C++ and C, such as the GCC's and Clang's __restrict__, and
// Visual C++'s __declspec(restrict). In addition, __restrict is supported by those three
// compilers. The exact interpretation of these alternative keywords vary by the compiler:
#define restrict __restrict
#endif

void subsample422_8bit_UYVY(int width, int height, unsigned char* buffer)
{
    Timer timer;
    timer.start();

    unsigned char* p1 = buffer;

    if (width % 6 == 0)
    {
        for (size_t row = 0; row < height; row++)
        {
            for (unsigned char* restrict p0 = buffer + row * 3 * width, * restrict e = p0 + 3 * width;
                 p0 < e;
                 p0 += 6, p1 += 4)
            {
                //
                //  The comment out parts do the interpolation, but this
                //  can be done as a small blur after the YUV
                //  conversion. That saves a few microseconds
                //

                //p1[0] = (int(p0[1]) + int(p0[4])) >> 1;
                p1[0] = p0[1];
                p1[1] = p0[0]; 
                //p1[2] = (int(p0[2]) + int(p0[5])) >> 1;
                p1[2] = p0[2];
                p1[3] = p0[3];
            }
        }
    }
    else
    {
        for (size_t row = 0; row < height; row++)
        {
            size_t count = 0;

            for (unsigned char* restrict p0 = buffer + row * 3 * width, * restrict e = p0 + 3 * width;
                 p0 < e;
                 p0 += 3, count++)
            {
                *p1 = p0[count % 2 + 1]; p1++;
                *p1 = *p0; p1++;
            }
        }
    }

    timer.stop();
    //cout << timer.elapsed() << endl;
}

#define R10MASK 0x3FF00000
#define G10MASK 0xFFC00
#define B10MASK 0x3FF

void subsample422_10bit(int width, int height, uint32_t* buffer)
{
    Timer timer;
    timer.start();
    uint32_t* p1 = buffer;

    //
    //  NOTE: 2_10_10_10_INT_REV is *backwards* eventhough its GL_RGB
    //  (hence the REV). So Y is the lowest sig bits.
    //

    for (size_t row = 0; row < height; row++)
    {
        for (uint32_t* restrict p0 = buffer + row * width, * restrict e = p0 + width;
             p0 < e;
             p0 += 6)
        {
            const uint32_t A = *p0;
            const uint32_t B = p0[1];
            const uint32_t C = p0[2];
            const uint32_t D = p0[3];
            const uint32_t E = p0[4];
            const uint32_t F = p0[5];

            *p1 = (A & R10MASK) | ((A << 10) & G10MASK) | ((A >> 10) & B10MASK); p1++;
            *p1 = ((C << 20) & R10MASK) | (B & G10MASK) | (B & B10MASK); p1++;
            *p1 = ((C << 10) & R10MASK) | ((D << 10) & G10MASK) | ((B >> 20) & B10MASK); p1++;
            *p1 = ((F << 20) & R10MASK) | ((D >> 10) & G10MASK) | (E & B10MASK); p1++;
        }
    }

    timer.stop();
    //cout << timer.elapsed() << endl;
}

}


void
OutputVideoDevice::transferMain()
{
    lockDevice(true);
    lockDevice(false);

    lockDevice(true, "TRANSFER");
    bool threadStop = m_threadStop;
    lockDevice(false);

    bool sleeping = false;
    size_t spinCount = 0;

    size_t nchannels = 1;

    for (size_t vi = 0;
         !threadStop;
         vi = (vi + 1) % nchannels)
    {
        VideoChannel* vc  = m_videoChannels[vi];
        size_t        vi2 = (vi + 1) % nchannels;
        VideoChannel* vc2 = m_videoChannels[vi2];

        sleeping = false;
        const bool lastChannel = vi == nchannels-1;

        bool wrote = false;

        //
        //  Maybe transfer frame
        //

        lockDevice(true, "TRANSFER");
        size_t wi = m_writeBufferIndex;
        size_t wi2 = (wi + 1) % m_ringBufferSize;
        FrameData& f = vc->data[wi];
        FrameData& f2 = vc2->data[wi2];
        lockDevice(false);

        f.lockState("TRANSFER");
        const bool readyForTransfer = f.state == FrameData::Mapped;
        const bool reading          = f.state == FrameData::Reading;
        const bool shouldLock       = readyForTransfer | reading;
        f.unlockState();

        if (shouldLock)
        {
            f.lockImage("TRANSFER");

            //
            //  Update threadStop again in case we were blocked
            //

            lockDevice(true, "TRANSFER");
            threadStop = m_threadStop;
            lockDevice(false);

            if (!threadStop)
            {
                f.lockState("TRANSFER");
                f.state = FrameData::Transfering;
                f.unlockState();

                //
                //  If the output plugin requested a subsampled format do the
                //  subsampling here.  
                //  XXX as of now we are doing any necessary subsampling in the
                //  output plugin, but it should be done here to avoid
                //  duplication, and because we can eventually push it into
                //  hardware (OpenCL or whatever).
                //

                switch (m_internalDataFormat)
                {
                  case Y0CbY1Cr_8_422:
                      //subsample422_8bit_UYVY(m_width, m_height, (unsigned char*)t.videoBuffer);
                      break;
                  case YCrCb_AJA_10_422:
                      //subsample422_10bit(m_width, m_height, (uint32_t*)t.videoBuffer);
                      break;
                  default:
                      break;
                }

                //
                //  Add to writer queue.  imagesAtFrame() below will pull off
                //  queue and make FB for output plugin.
                //

                {
                    ScopedLock lock(m_fdQueueMutex);
                    m_fdQueue.push_back(&f);
                    m_fdQueueCond.notify_all();
                }

                //f.fbo->endExternalReadback();

                wrote = true;
                f.lockState("TRANSFER");
                f.state = FrameData::NeedsUnmap;
                f.unlockState();
            }
            
            f.unlockImage();
        }

        lockDevice(true, "TRANSFER");

        if (wrote && lastChannel)
        {
            m_writeBufferCount++;
            m_writeBufferIndex = m_writeBufferCount % m_ringBufferSize;
        }

        threadStop = m_threadStop;
        lockDevice(false);

        if (!wrote) 
        {
            vi--;
        }
    }

    //
    //  Shutdown ring buffer and exit thread
    //

    lockDevice(true, "TRANSFER");
    m_threadDone = true;
    lockDevice(false);
}

void
OutputVideoDevice::finalizeOutstandingPBOTransfer(VideoChannel* vc) const
{
    //
    //  Transfer last read PBO or pinned buffer
    //
    //  NOTE: we don't have to lock the device to read m_readBufferIndex
    //  because only this thread is allowed to change it.
    //

    if (m_readBufferCount <= 0) return;

    const size_t previousReadIndex = (m_readBufferIndex + m_ringBufferSize - 1) % m_ringBufferSize;

    FrameData& fdprev = vc->data[previousReadIndex];

    fdprev.lockState("PBO READER");
    bool reading = fdprev.state == FrameData::Reading;
    fdprev.unlockState();
    assert(reading);

    glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, fdprev.globject); TWK_GLDEBUG;
    GLubyte* p = (GLubyte*)glMapBuffer(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY_ARB); TWK_GLDEBUG;
        
    if (p)
    {
        if (m_immediateCopy)
        {
            if (!fdprev.imageData) 
            {
                fdprev.imageData = TWK_ALLOCATE_ARRAY_PAGE_ALIGNED(unsigned char, m_bufferSizeInBytes);
            }
            memcpy(fdprev.imageData, p, m_bufferSizeInBytes);
        }
        else
        {
            fdprev.mappedPointer = p;
        }
            
        fdprev.lockState("PBO READER");
        if (fdprev.mappedPointer || fdprev.imageData) fdprev.state = FrameData::Mapped;
        if (m_immediateCopy) glUnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB); TWK_GLDEBUG;
        //fdprev.fbo = 0;
        fdprev.unlockState();
    }
    else
    {
        fdprev.imageData = 0;
        fdprev.mappedPointer = 0;
        //fdprev.fbo = 0;
        glUnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB); TWK_GLDEBUG;
        cerr << "ERROR: not mapped" << endl;
    }

    endGPUTransfer();

    m_mappedBufferCount++;
    fdprev.locked = false;
    fdprev.unlockImage();
}

void
OutputVideoDevice::transferChannelPBO(VideoChannel* vc, const GLFBO* fbo) const
{
    finalizeOutstandingPBOTransfer(vc);

    //
    //  Initiate next PBO read. 
    //
    //  NOTE: we don't have to lock the device to read
    //  m_readBufferIndex because only this thread is allowed to change
    //  it.
    //

    FrameData& fdread = vc->data[m_readBufferIndex];

    //
    //  May block at this point if transferring 
    //

    fdread.lockImage("PBO READER"); // until unmapped above
    fdread.locked = true;

    fdread.lockState("PBO READER");
    bool transfer = fdread.state == FrameData::Transfering;
    bool unmapit  = fdread.state == FrameData::NeedsUnmap || transfer;
    bool mapped   = fdread.state == FrameData::Mapped;
    if (!mapped) 
    {
        fdread.state = FrameData::Reading;
        fdread.fbo = fbo;
    }
    fdread.unlockState();

    if (mapped)
    {
        //
        //  This can happen in the non immediateCopy case. We're
        //  basically about to read over an already mapped PBO. 
        //
        //  Prob can't happen anymore now that renderer waits until
        //  external readback happens on FBOs.
        //

        while (mapped)
        {
            fdread.locked = false;
            fdread.unlockImage();
            fdread.lockState("PBO READER WAIT");
            transfer = fdread.state == FrameData::Transfering;
            unmapit  = fdread.state == FrameData::NeedsUnmap || transfer;
            mapped   = fdread.state == FrameData::Mapped;
            fdread.unlockState();
#ifndef PLATFORM_WINDOWS
            usleep(1);
#endif
            fdread.lockImage("PBO READER");
            fdread.locked = true;
        }
            
        fdread.lockState("PBO READER");
        fdread.state = FrameData::Reading;
        fdread.fbo = fbo;
        fdread.unlockState();
    }

    glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, fdread.globject); TWK_GLDEBUG;

    if (unmapit)
    {
        if (!m_immediateCopy) glUnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB); TWK_GLDEBUG;
        fdread.mappedPointer = 0;
        m_mappedBufferCount--;
    }

    startGPUTransfer();

    glReadPixels(0,
                 0,
                 m_width,
                 m_height,
                 m_textureFormat,
                 m_textureType,
                 0); TWK_GLDEBUG;

    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

void
OutputVideoDevice::transferChannelReadPixels(VideoChannel* vc, const GLFBO* fbo) const
{
    //
    //  Vanillia glReadPixels case (only for comparison)
    //

    FrameData& fdread = vc->data[m_readBufferIndex];

    fdread.lockImage("READPIXELS READER");
    fdread.lockState("READPIXELS READER");
    fdread.state = FrameData::Reading;
    fdread.fbo = fbo;
    if (!fdread.imageData) 
        fdread.imageData = TWK_ALLOCATE_ARRAY_PAGE_ALIGNED(unsigned char, m_bufferSizeInBytes);
    fdread.unlockState();

    startGPUTransfer();

    glReadPixels(0,
                 0,
                 m_width,
                 m_height,
                 m_textureFormat,
                 m_textureType,
                 fdread.imageData); TWK_GLDEBUG;

    endGPUTransfer();

    fdread.lockState("READPIXELS READER");
    fdread.state = FrameData::Mapped;
    fdread.fbo = 0;
    fdread.unlockState();
    fdread.unlockImage();
}

void 
OutputVideoDevice::startGPUTransfer() const
{
    if (m_profile) m_gpuBeginTime.push_back(m_globalTimer.elapsed());
}

void 
OutputVideoDevice::endGPUTransfer() const
{
    if (m_profile) m_gpuTimes.push_back(m_globalTimer.elapsed() - m_gpuBeginTime.back());
}

void 
OutputVideoDevice::startOutputTransfer() const
{
    if (m_profile) m_transferBeginTime.push_back(m_globalTimer.elapsed());
}

void 
OutputVideoDevice::endOutputTransfer() const
{
    if (m_profile) m_transferTimes.push_back(m_globalTimer.elapsed() - m_transferBeginTime.back());
}

void 
OutputVideoDevice::packBufferCopy(unsigned char* src, size_t srcRowSize,
                                  unsigned char* dst, size_t dstRowSize,
                                  size_t rows)
{
    for (size_t row = 0; row < rows; row++)
    {
        memcpy(dst + dstRowSize * row,
               src + dstRowSize * row,
               dstRowSize);
    }
}

TwkMovie::MovieWriter::WriteRequest
OutputVideoDevice::writeRequestFromState() const
{
    MovieWriter::WriteRequest request;
    request.verbose        = m_infoFeedback;
    request.threads        = m_writerThreads;
    request.fps            = m_fps;
    request.compression    = m_codec;
    request.codec          = m_codec;
    request.audioCodec     = m_audioCodec;
    request.quality        = m_quality;
    request.pixelAspect    = m_pixelAspect;
    request.audioChannels  = channelsCount(m_audioLayout);
    request.audioRate      = m_audioRate;
    request.stereo         = false;
    request.comments       = m_comments;
    request.copyright      = m_copyright;
    request.parameters     = m_writerArgs;
    
    //if (rysamples || usamples)
    //{
        //request.preferCommonFormat = false;
        //request.keepPlanar = true;
    //}

    return request;
}

void
OutputVideoDevice::writerMain()
{
    m_writer->write(this);
}

//
// XXX imagesAtFrame always sends the next frame provided by the renderer
// regardless of the requested frame number.
//

void 
OutputVideoDevice::imagesAtFrame(const ReadRequest& request, FrameBufferVector& fbs)
{
    ScopedLock lock(m_fdQueueMutex);
    while (m_fdQueue.empty()) m_fdQueueCond.wait(lock);

    FrameData* fd = m_fdQueue.front();
    m_fdQueue.pop_front();

    fbs.resize(1);
    if (!fbs.front()) fbs.front() = new FrameBuffer();
    FrameBuffer& fb = *fbs.front();

    fb.restructure(m_width,
                   m_height,
                   1,
                   m_channels,
                   m_dataType,
                   m_immediateCopy ? fd->imageData : fd->mappedPointer,
                   &m_channelNames,
                   TwkFB::FrameBuffer::BOTTOMLEFT,
                   m_immediateCopy); // delete on destruction

    //
    //  Right now we can't know when to release the GLFBO from
    //  external readback because of the way the MovieWriter API
    //  works: the MovieWriter has the control loop and periodically
    //  calls this function to get another frame to
    //  write. Unfortunately, there's no way currently to know when it
    //  actually finished writing. One way we could potentially do
    //  that is to have some kind of a signal emitted by the FB when
    //  its "done", but it seems like we need to have an alternate or
    //  new writer API to make this efficient
    //
    //  So to get around the above problem we just copy the pixels. In
    //  In the immedate copy mode fd->imageData has unique memory so
    //  we just adopt it.
    //

    if (m_immediateCopy)
    {
        //
        //  Should we just reallocate here in this thread? This forces
        //  the rendering thread to do the memory allocation.
        //

        fd->imageData = 0;
    }
    else
    {
        fb.ownData(); // copies them now (in this thread)
    }

    //
    //  This will unlock the GLFBO for reuse by the render thread. 
    //

    fd->fbo->endExternalReadback();
}

size_t 
OutputVideoDevice::audioFillBuffer(const AudioReadRequest& request, AudioBuffer& buf)
{
    size_t nsamps = TwkAudio::timeToSamples(request.duration, m_audioRate);
    buf.reconfigure(nsamps, m_audioLayout, m_audioRate, request.startTime);
    IPCore::IPNode::AudioContext context(buf, m_fps);

    if (m_audioInit)
    {
        //
        //  Initialize as late as possible (which is right before we
        //  actually need the audio).
        //

        IPCore::IPGraph::AudioConfiguration config(
                m_audioRate, 
                m_audioLayout,
                nsamps,
                m_fps,
                false, // XXX arbitarily say we're _not_ going backwards.
                buf.startSample(),
                size_t((m_frameEnd - m_frameStart) / m_fps * m_audioRate + 0.49));

        m_graph->audioConfigure(config);
        m_audioInit = false;
    }

    return m_graph->audioFillBuffer(context);
}

void
OutputVideoDevice::initializeDataFormats()
{
    switch (m_bits)
    {
      default:
      case 8:
          m_internalDataFormat = m_channels == 3 ? RGB8 : RGBA8;
          m_dataType = TwkFB::FrameBuffer::UCHAR;
          break;
      case 10:
          m_internalDataFormat = RGB10X2Rev;
          m_dataType = TwkFB::FrameBuffer::PACKED_X2_B10_G10_R10;
          m_channels = 1;
          break;
      case 16:
          if (m_float)
          {
              m_internalDataFormat = m_channels == 3 ? RGB16F : RGBA16F;
              m_dataType = TwkFB::FrameBuffer::HALF;
          }
          else
          {
              m_internalDataFormat = m_channels == 3 ? RGB16 : RGBA16;
              m_dataType = TwkFB::FrameBuffer::USHORT;
          }
          break;
      case 32:
          m_internalDataFormat = m_channels == 3 ? RGB32F : RGBA32F;
          m_dataType = TwkFB::FrameBuffer::FLOAT;
          break;
    }
}

} // OutputVideoDevices
