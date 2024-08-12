//
// Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <TwkGLF/GLVideoDevice.h>
#include <TwkGLF/GL.h>
#include <TwkUtil/Timer.h>
#include <boost/thread.hpp>

#ifdef PLATFORM_WINDOWS
#include <GL/gl.h>
#include <GL/glu.h>
#include <pthread.h>
#include <windows.h>
#include <process.h>
#endif

#if defined( PLATFORM_LINUX )
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>
#include <GL/glxext.h>
#include <GL/glu.h>
#endif

#if defined( PLATFORM_DARWIN )
#include <TwkGLF/GL.h>
#endif

#include <TwkGLF/GLFBO.h>
#include <TwkGLF/GLFence.h>
#include <iostream>
#include <stl_ext/thread_group.h>
#include <deque>
#include <BlackMagicDevices/HDRVideoFrame.h>
#include <BlackMagicDevices/StereoVideoFrame.h>

namespace BlackMagicDevices
{
  class BlackMagicModule;
  class PinnedMemoryAllocator;

  typedef boost::mutex::scoped_lock ScopedLock;
  typedef boost::mutex Mutex;
  typedef boost::condition_variable Condition;

  struct DeckLinkDataFormat
  {
    const char* desc{ nullptr };
    BMDPixelFormat value{ bmdFormatUnspecified };
    TwkApp::VideoDevice::InternalDataFormat iformat{ TwkApp::VideoDevice::InternalDataFormat::RGB8 };
    bool rgb{ false };
  };

  struct DeckLinkVideoFormat
  {
    int width{ 0 };
    int height{ 0 };
    float pa{ 0.0f };
    float hz{ 0.0f };
    const char* desc{ nullptr };
    BMDDisplayMode value{ bmdModeUnknown };
  };

  struct DeckLinkAudioFormat
  {
    double hz{ 0.0 };
    BMDAudioSampleRate sampleRate{ bmdAudioSampleRate48kHz };
    TwkAudio::Format prec{ TwkAudio::Format::UnknownFormat };
    BMDAudioSampleType sampleDepth{ bmdAudioSampleType32bitInteger };
    size_t numChannels{ 0 };
    TwkAudio::Layout layout{ TwkAudio::Layout::UnknownLayout };
    const char* desc{ nullptr };
  };

  typedef std::vector<DeckLinkVideoFormat> DeckLinkVideoFormatVector;
  typedef std::vector<DeckLinkDataFormat> DeckLinkDataFormatVector;

  struct DeckLinkVideo4KTransport
  {
    std::string desc;
    size_t value{ 0 };
  };

  class DeckLinkVideoDevice : public TwkGLF::GLBindableVideoDevice,
                              public IDeckLinkVideoOutputCallback,
                              public IDeckLinkAudioOutputCallback
  {
    friend class PinnedMemoryAllocator;

   public:
    //
    //  Types
    //

    typedef TwkUtil::Timer Timer;
    typedef TwkGLF::GLFence GLFence;
    typedef TwkGLF::GLFBO GLFBO;
    typedef std::vector<unsigned char*> BufferVector;
    typedef stl_ext::thread_group ThreadGroup;
    typedef std::vector<int> AudioBuffer;
    typedef std::map<void*, StereoVideoFrame*> StereoFrameMap;
    typedef std::map<void*, HDRVideoFrame*> HDRVideoFrameMap;
    typedef std::deque<IDeckLinkMutableVideoFrame*> DLVideoFrameDeque;

    struct PBOData
    {
      enum class State
      {
        Mapped,
        Transferring,
        NeedsUnmap,
        Ready
      };

      PBOData( GLuint g );
      ~PBOData();

      void lockData();
      void unlockData();
      void lockState();
      void unlockState();

      GLuint globject{ 0 };
      void* mappedPointer{ nullptr };
      State state{ State::Ready };
      GLFence* fence{ nullptr };
      const GLFBO* fbo{ nullptr };

     private:
      pthread_mutex_t mutex;
      pthread_mutex_t stateMutex;
    };

    struct FrameData
    {
      FrameData() : audioData( nullptr ), videoFrame( nullptr ) {}
      ~FrameData() {}
      void* audioData{ nullptr};
      IDeckLinkVideoFrame* videoFrame{ nullptr };
    };

    typedef std::deque<PBOData*> PBOQueue;

    //
    //  Constructors
    //

    DeckLinkVideoDevice( BlackMagicModule*, const std::string&, IDeckLink*,
                         IDeckLinkOutput* );
    virtual ~DeckLinkVideoDevice();

    virtual size_t asyncMaxMappedBuffers() const;
    virtual Time deviceLatency() const;

    virtual size_t numVideoFormats() const;
    virtual VideoFormat videoFormatAtIndex( size_t ) const;
    virtual void setVideoFormat( size_t );
    virtual size_t currentVideoFormat() const;

    virtual size_t numAudioFormats() const;
    virtual AudioFormat audioFormatAtIndex( size_t ) const;
    virtual void setAudioFormat( size_t );
    virtual size_t currentAudioFormat() const;

    virtual size_t numVideo4KTransports() const;
    virtual Video4KTransport video4KTransportAtIndex( size_t ) const;
    virtual void setVideo4KTransport( size_t );
    virtual size_t currentVideo4KTransport() const;

    virtual size_t numDataFormats() const;
    virtual DataFormat dataFormatAtIndex( size_t ) const;
    virtual void setDataFormat( size_t );
    virtual size_t currentDataFormat() const;

    virtual size_t numSyncSources() const;
    virtual SyncSource syncSourceAtIndex( size_t ) const;
    virtual void setSyncSource( size_t );
    virtual size_t currentSyncSource() const;

    virtual size_t numSyncModes() const;
    virtual SyncMode syncModeAtIndex( size_t ) const;
    virtual void setSyncMode( size_t );
    virtual size_t currentSyncMode() const;

    //  GLBindableVideoDevice API
    //

    virtual bool isStereo() const;
    virtual bool isDualStereo() const;

    virtual bool readyForTransfer() const;
    virtual void transfer( const TwkGLF::GLFBO* ) const;
    virtual void transfer2( const TwkGLF::GLFBO*, const TwkGLF::GLFBO* ) const;
    virtual void transferAudio( void* interleavedData, size_t n ) const;
    virtual bool willBlockOnTransfer() const;

    virtual size_t width() const
    {
      return m_frameWidth;
    }
    virtual size_t height() const
    {
      return m_frameHeight;
    }
    virtual void open( const StringVector& args );
    virtual void close();
    virtual bool isOpen() const;
    virtual void makeCurrent() const;
    virtual void clearCaches() const;
    virtual void syncBuffers() const;
    virtual VideoFormat format() const;
    virtual Timing timing() const;

    IDeckLinkOutput* deckLinkOutput() const
    {
      return m_outputAPI;
    }

    virtual void unbind() const;
    virtual void bind( const TwkGLF::GLVideoDevice* ) const;
    virtual void bind2( const TwkGLF::GLVideoDevice*,
                        const TwkGLF::GLVideoDevice* ) const;
    virtual void audioFrameSizeSequence( AudioFrameSizeVector& ) const;

    virtual HRESULT STDMETHODCALLTYPE QueryInterface( REFIID iid, LPVOID* ppv )
    {
      return E_NOINTERFACE;
    }
    virtual ULONG STDMETHODCALLTYPE AddRef()
    {
      return 1;
    }
    virtual ULONG STDMETHODCALLTYPE Release()
    {
      return 1;
    }

    virtual HRESULT STDMETHODCALLTYPE
    ScheduledFrameCompleted( IDeckLinkVideoFrame* completedFrame,
                             BMDOutputFrameCompletionResult result );
    virtual HRESULT STDMETHODCALLTYPE ScheduledPlaybackHasStopped();
#ifdef PLATFORM_WINDOWS
    virtual HRESULT STDMETHODCALLTYPE RenderAudioSamples( BOOL preroll );
#else
    virtual HRESULT STDMETHODCALLTYPE RenderAudioSamples( bool preroll );
#endif

   private:
    void initialize();
    bool transferChannel( size_t i, const TwkGLF::GLFBO* ) const;
    void transferChannelPBO( size_t i, const TwkGLF::GLFBO*,
                             IDeckLinkMutableVideoFrame*,
                             IDeckLinkMutableVideoFrame* ) const;
    void transferChannelReadPixels( size_t i, const TwkGLF::GLFBO*,
                                    IDeckLinkMutableVideoFrame*,
                                    IDeckLinkMutableVideoFrame* ) const;
    void ScheduleFrame() const;
    size_t bytesPerRow( BMDPixelFormat bmdFormat, size_t width ) const;

   private:
    DeckLinkVideoFormatVector m_decklinkVideoFormats;
    DeckLinkDataFormatVector m_decklinkDataFormats;
    bool m_supportsStereo;
    mutable size_t m_firstThreeCounter;
    mutable IDeckLinkMutableVideoFrame* m_readyFrame;
    mutable StereoVideoFrame* m_readyStereoFrame;
    mutable DLVideoFrameDeque m_DLOutputVideoFrameQueue;
    mutable DLVideoFrameDeque m_DLReadbackVideoFrameQueue;  // only rgb formats
    mutable bool m_needsFrameConverter;
    mutable bool m_hasAudio;
    mutable StereoFrameMap
        m_rightEyeToStereoFrameMap;  // indexed by the left eye
    mutable HDRVideoFrameMap m_FrameToHDRFrameMap;
    mutable PBOQueue m_pboQueue;
    IDeckLinkOutput* m_outputAPI;
    IDeckLink* m_deviceAPI;
    IDeckLinkConfiguration* m_configuration;
    bool m_useHDRMetadata{ false };
    mutable PBOData* m_lastPboData;
    mutable PBOData*
        m_secondLastPboData;  // use of stereo formats; stores left eye.
    BMDPixelFormat m_readPixelFormat;
    BMDPixelFormat m_outputPixelFormat;
    static const int NB_AUDIO_BUFFERS = 2;
    void* m_audioData[NB_AUDIO_BUFFERS];
    mutable int m_audioDataIndex;
    bool m_initialized;
    mutable bool m_bound;
    bool m_asyncSDISend;
    bool m_pbos;
    size_t m_pboSize;
    size_t m_videoFrameBufferSize;
    bool m_open;
    bool m_stereo;
    size_t m_frameWidth;
    size_t m_frameHeight;
    mutable size_t m_totalPlayoutFrames;
    BMDTimeValue m_frameDuration;
    BMDTimeScale m_frameTimescale;
    mutable int m_transferTextureID;
    int m_internalAudioFormat;
    int m_internalVideoFormat;
    int m_internalVideo4KTransport;
    int m_internalDataFormat;
    int m_internalSyncMode;
    int m_internalSyncSource;
    unsigned long m_framesPerSecond;
    unsigned long m_audioBufferSampleLength;
    unsigned long m_audioSamplesPerFrame;
    mutable bool m_frameCompleted;
    unsigned long m_audioChannelCount;
    BMDAudioSampleRate m_audioSampleRate;
    BMDAudioSampleType m_audioSampleDepth;
    TwkAudio::Format m_audioFormat;
    size_t m_audioFormatSizeInBytes{ 0 };
    GLenum m_textureFormat;
    GLenum m_textureType;
    static bool m_infoFeedback;
  };

}  // namespace BlackMagicDevices
