//
//  Copyright (c) 2014 Tweak Software. 
//  All rights reserved.
//  
//  SPDX-License-Identifier: Apache-2.0
//  
//
#ifndef __OutputVideoDevices__OutputVideoDevice__h__
#define __OutputVideoDevices__OutputVideoDevice__h__
#include <TwkGLF/GL.h>
#include <TwkUtil/Timer.h>
#include <boost/thread.hpp>
#include <TwkMovie/MovieIO.h>

#if defined(PLATFORM_WINDOWS)
#include <GL/gl.h>
#include <GL/glu.h>
#include <pthread.h>
#endif

#if defined(PLATFORM_LINUX)
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>
#include <GL/glxext.h>
#include <GL/glu.h>
#endif

#if defined(PLATFORM_DARWIN)
#include <TwkGLF/GL.h>
#endif

#include <TwkGLF/GLFBO.h>
#include <TwkGLF/GLFence.h>
#include <stl_ext/thread_group.h>
#include <iostream>
#include <vector>
#include <deque>

namespace IPCore {
class IPGraph;
}

namespace OutputVideoDevices {
typedef boost::mutex::scoped_lock ScopedLock;
typedef boost::mutex              Mutex;
typedef boost::condition_variable Condition;
class OutputVideoDeviceModule;

//
//  OutputVideoDevice 
//
//  OutputVideoDevice uses the video device API from TwkApp::VideoDevice to
//  have images streamed to it. Because its also a TwkMovie::Movie object
//  it can be connected to any of the media output plugins. In other words:
//  it tries to stream images and audio as fast as possible to the media
//  writers. 
//
//  The general layout of the code is similar to how the SDI
//  works with the exception that this VideoDevice will not take per-frame
//  audio like SDI does. For audio it interfaces directly with the
//  graph. There is a ring buffer of frames being transfered from the GPU
//  just like the SDI devices.
//
//  The idea is to stream *as fast as possible* directly from the GPU just
//  like SDI. So unlike rvio, this avoids any kind of software manipulation
//  of the pixels.
//
//  The media writing is done in a separate thread which has to be
//  synchronized with the render thread.
//  
//  In the future we should find a way to break this up so that we don't
//  have a TwkApp::VideoDevice which is also a TwkMovie::Movie.
//


class OutputVideoDevice : public TwkGLF::GLBindableVideoDevice, 
                          public TwkMovie::Movie
{
public:
    typedef std::vector<std::string>           StringVector;
    typedef std::pair<std::string,std::string> StringPair;
    typedef std::vector<StringPair>            StringPairVector;
    typedef VideoDevice::Time                  Time;
    typedef boost::thread                      Thread;
    typedef TwkUtil::Timer                     Timer;
    typedef TwkGLF::GLFence                    GLFence;
    typedef std::vector<unsigned char*>        BufferVector;
    typedef std::vector<Time>                  TimeVector;
    typedef std::vector<int>                   IntVector;
    typedef TwkGLF::GLFBO                      GLFBO;
    typedef TwkMovie::MovieWriter              MovieWriter;
    typedef TwkFB::FrameBuffer::DataType       FBDataType;

    //
    //  This is basically the ring buffer element. Each method uses
    //  different parts of the struct, but its treated as the internal
    //  ring buffer state by all of them 
    //

    struct FrameData
    {
        enum State { NotReady, Reading, Mapped, Transfering, NeedsUnmap, Ready };

        FrameData();
        FrameData(const FrameData&);
        ~FrameData();
        FrameData& operator= (const FrameData&);

        void lockImage(const char* threadName);
        void unlockImage();
        void lockState(const char* threadName);
        void unlockState();

        const GLFBO*   fbo;
        GLuint         globject;
        unsigned char* mappedPointer;
        unsigned char* imageData;
        State          state;
        GLFence*       fence;
        bool           locked;

        Mutex          imageMutex;
        Mutex          stateMutex;
    };

    typedef std::vector<FrameData> FrameDataVector;
    typedef std::deque<FrameData*> FrameDataQueue;

    //
    //  VideoChannel is a logical indepenent output. For example the
    //  stereo modes require two VideoChannels: one for the left and
    //  one for the right eyes. 
    //

    struct VideoChannel
    {
        VideoChannel(size_t bsize, size_t n);
        ~VideoChannel();

        size_t          bufferSizeInBytes;
        FrameDataVector data;
    };

    typedef std::vector<VideoChannel*> VideoChannelVector;

    //
    //  Constructor
    //

    OutputVideoDevice(OutputVideoDeviceModule*, const std::string& name, IPCore::IPGraph* graph);
    virtual ~OutputVideoDevice();

    //
    //  VideoDevice Video API
    //

    virtual Time outputTime() const;
    virtual Time inputTime() const;
    //virtual void resetClock() const;

    virtual void beginTransfer() const;
    virtual void endTransfer() const;

    virtual void makeCurrent() const;
    virtual void clearCaches() const;
    virtual void syncBuffers() const;

    virtual void open(const StringVector&);
    virtual void close();
    virtual bool isOpen() const;

    virtual size_t asyncMaxMappedBuffers() const;
    virtual Time deviceLatency() const;

    virtual size_t numVideoFormats() const;
    virtual VideoFormat videoFormatAtIndex(size_t) const;
    virtual void setVideoFormat(size_t);
    virtual size_t currentVideoFormat() const;

    virtual size_t numDataFormats() const;
    virtual DataFormat dataFormatAtIndex(size_t) const;
    virtual void setDataFormat(size_t);
    virtual size_t currentDataFormat() const;

    virtual size_t numSyncModes() const;
    virtual SyncMode syncModeAtIndex(size_t) const;
    virtual void setSyncMode(size_t);
    virtual size_t currentSyncMode() const;

    virtual size_t numSyncSources() const;
    virtual SyncSource syncSourceAtIndex(size_t) const;
    virtual void setSyncSource(size_t);
    virtual size_t currentSyncSource() const;

    virtual Resolution resolution() const;
    virtual Timing timing() const;
    virtual VideoFormat format() const;
    virtual size_t width() const;
    virtual size_t height() const;

    //
    //  VideoDevice Audio API: they are stubs in this case, since audio takes a
    //  different path (though the Movie API below).
    //

    virtual size_t numAudioFormats() const;
    virtual AudioFormat audioFormatAtIndex(size_t) const;
    virtual void setAudioFormat(size_t);
    virtual size_t currentAudioFormat() const;
    virtual void audioFrameSizeSequence(AudioFrameSizeVector&) const;
    virtual void transferAudio(void* interleavedData, size_t n) const;

    //
    //  GLBindableVideoDevice API
    //

    virtual bool isStereo() const;
    virtual bool isDualStereo() const;
    virtual bool willBlockOnTransfer() const;
    virtual void bind(const TwkGLF::GLVideoDevice*) const;
    virtual void bind2(const TwkGLF::GLVideoDevice*, const TwkGLF::GLVideoDevice*) const;
    virtual void transfer(const TwkGLF::GLFBO*) const;
    virtual void transfer2(const TwkGLF::GLFBO*, const TwkGLF::GLFBO*) const;
    virtual void unbind() const;

    void transferMain();
    void writerMain();
    void lockDevice(bool lock, const char* threadName = "") const;
    Mutex& deviceMutex() const { return m_deviceMutex; }

    void finalizeOutstandingPBOTransfer(VideoChannel*) const;

    void transferChannelPBO(VideoChannel*, const TwkGLF::GLFBO*) const;
    void transferChannelReadPixels(VideoChannel*, const TwkGLF::GLFBO*) const;
    void transferChannel(size_t, const TwkGLF::GLFBO*) const;

    void startGPUTransfer() const;
    void endGPUTransfer() const;
    void startOutputTransfer() const;
    void endOutputTransfer() const;
    void packBufferCopy(unsigned char*, size_t, unsigned char*, size_t, size_t);

    //
    //  Movie API
    //

    virtual void imagesAtFrame(const ReadRequest&, FrameBufferVector&);
    virtual size_t audioFillBuffer(const AudioReadRequest&, AudioBuffer&);

    TwkMovie::MovieWriter::WriteRequest writeRequestFromState() const;

protected:
    void initializeDataFormats();

private:
    size_t                 m_channels;
    FBDataType             m_dataType;
    StringVector           m_channelNames;
    size_t                 m_width;
    size_t                 m_height;
    size_t                 m_bits;
    bool                   m_float;
    float                  m_fps;
    bool                   m_profile;
    std::string            m_codec;
    std::string            m_audioCodec;
    std::string            m_comments;
    std::string            m_copyright;
    int                    m_frameStart;
    int                    m_frameEnd;
    float                  m_quality;
    float                  m_audioRate;
    TwkAudio::Layout       m_audioLayout;
    int                    m_writerThreads;
    StringPairVector       m_writerArgs;
    size_t                 m_ringBufferSize;
    size_t                 m_bufferSize;
    size_t                 m_bufferSizeInBytes;
    mutable size_t         m_bufferStride;
    float                  m_pixelAspect;
    float                  m_pixelScale;
    bool                   m_open;
    bool                   m_stereo;
    IPCore::IPGraph*       m_graph;
    std::string            m_filename;
    MovieWriter*           m_writer;
    TwkMovie::MovieInfo    m_writerInfo;
    bool                   m_audioInit;
    mutable bool           m_bound;
    mutable Mutex          m_deviceMutex;
    mutable bool           m_threadStop;
    mutable bool           m_threadDone;
    bool                   m_pbos;
    bool                   m_immediateCopy;
    GLenum                 m_fboInternalFormat;
    GLenum                 m_textureFormat;
    GLenum                 m_textureType;
    mutable size_t         m_texturePadding;
    mutable Timer          m_globalTimer;
    mutable TimeVector     m_gpuTimes;
    mutable TimeVector     m_transferTimes;
    mutable TimeVector     m_gpuBeginTime;
    mutable TimeVector     m_transferBeginTime;
    VideoChannelVector     m_videoChannels;
    mutable size_t         m_mappedBufferCount;
    mutable size_t         m_readBufferIndex;
    mutable size_t         m_readBufferCount;
    mutable size_t         m_writeBufferIndex;
    mutable size_t         m_writeBufferCount;
    InternalDataFormat     m_internalDataFormat;
    mutable Mutex          m_waitMutex;
    mutable Condition      m_waitCond;
    mutable Thread*        m_transferThread;
    mutable Thread*        m_writerThread;
    mutable FrameDataQueue m_fdQueue;
    mutable Mutex          m_fdQueueMutex;
    mutable Condition      m_fdQueueCond;
};

} // OutputVideoDevices

#endif // __OutputVideoDevices__OutputVideoDevice__h__
