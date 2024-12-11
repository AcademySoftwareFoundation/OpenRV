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
#endif

#if defined( PLATFORM_LINUX )
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>
#include <GL/glxext.h>
#include <GL/glu.h>
#endif

#include <TwkGLF/GLFBO.h>
#include <TwkGLF/GLFence.h>
#include <iostream>
#include <stl_ext/thread_group.h>
#include "ajatypes.h"
#include "ntv2enums.h"
#include "ntv2card.h"

namespace AJADevices
{
  class AJAModule;

  using ScopedLock = boost::mutex::scoped_lock;
  using Mutex = boost::mutex;
  using Condition = boost::condition_variable;

  struct KonaAudioFormat
  {
    double hz{ 0.0 };
    TwkAudio::Format prec{ TwkAudio::Format::UnknownFormat };
    size_t numChannels{ 0 };
    TwkAudio::Layout layout{ TwkAudio::Layout::UnknownLayout };
    std::string desc;
  };

  struct KonaVideoFormat
  {
    int width{ 0 };
    int height{ 0 };
    float pa{ 0.0f };
    float hz{ 0.0f };
    std::string desc;
    NTV2VideoFormat value{ NTV2_FORMAT_UNKNOWN };
  };

  enum DataFormatFlags
  {
    NoFlags = 0,
    RGB444 = 1 << 0,
    P2P = 1 << 1,
    DualLink = 1 << 2,
    ThreeG = 1 << 3,
    LegalRangeY = 1 << 4,
    FullRangeY = 1 << 5,

    RGB_3G = RGB444 | P2P | ThreeG,
    RGB_DualLink = RGB444 | P2P | DualLink,
  };

  struct KonaDataFormat
  {
    std::string desc;
    NTV2FrameBufferFormat value{ NTV2_FBF_INVALID };
    TwkApp::VideoDevice::InternalDataFormat iformat{ TwkApp::VideoDevice::InternalDataFormat::RGB8 };
    unsigned int flags{ 0 };
  };

  using KonaVideoFormatVector = std::vector<KonaVideoFormat>;
  using KonaDataFormatVector = std::vector<KonaDataFormat>;

  struct KonaSyncMode
  {
    std::string desc;
    size_t value{ 0 };
  };

  struct KonaSyncSource
  {
    std::string desc;
    size_t value{ 0 };
  };

  struct KonaVideo4KTransport
  {
    std::string desc;
    size_t value{ 0 };
  };

  //
  //  AJA SDI Video Device
  //
  //  This class has different paths to fetch data from the GPU and
  //  send it to the Kona card:
  //
  //      1) Using PBOs.  The GPU transfers pixels directly to the application 
  //         memory. The buffers are allocated by the GL driver and they
  //         need to be mapped/unmapped into the application space. 
  //         There are two flavors of this: immediate copy and pointer
  //         passing. Only the mac seems to be able to do pointer
  //         passing (probably because of pointer alignment restrictions
  //         in TransferWithAutoCirculate()).
  //
  //      2) Using glReadPixels() with no bells or whistles. This is
  //         really only to have a base of comparison for the other
  //         methods.
  //
  //
  //  There is a dedicated thread spawned to handle the blocking transfer to 
  //  the Kona card. Synchronization is dealt with through a "state" variable
  //  on each ring buffer frame. So each frame in the ringbuffer is an 
  //  independent state machine. The states are defined in terms of the sequence
  //  of events when using PBOs. For pinned buffers and glReadPixels() alone, 
  //  the un/mapping states are basically no-ops. The state progesses like this:
  //
  //      Ready->Reading->Mapped->Transferring->NeedsUnmap
  //                |                               |
  //                +---------------<---------------+
  //
  //  In the PBO case the Transfering and NeedsUnmap state transition
  //  happen in the transfer thread. All other state transitions happen
  //  in the main thread. In the DVP case only Reading happens in the
  //  main thread.
  //
  //  The main thread cannot proceed with PBO transfer if the current
  //  read buffer is not in the NeedsUnmap or Ready state. So the device
  //  reports that it will block to the renderer in that case.
  //
  //  The ring buffer size is fixed at 3-4 currently. The buffer size is
  //  mostly determined by how fast we can deliver the pixels to the
  //  Kona. 3 seems to be the smallest value that works with all
  //  hardware combos. Slower renderers (mac) may require 4. Note that
  //  the IPCore::ImageRenderer will have one output FBO for each ring
  //  buffer entry (or 2 if stereo) as reported by
  //  VideoDevice::asyncMaxMappedBuffers().
  //
  //  4K
  //  --
  //
  //  4K output can be achieved if a Kona 3G is burned with QUAD
  //  firmware. In that case our code can run UHD and DCI 4K formats out
  //  all four SDI outputs.
  //
  //  TODO
  //  ----
  //
  //  In theory, we could DMA one scanline at a time from the GPU and
  //  send it immediately to the AJA device. This may make it possible
  //  to reduce latency by one additional frame, but there is also more
  //  overhead involved so we're not doing it (yet).
  //
  //  For prosumer apps we want to let the user choose the output format
  //  in the AJA control panel like FCPX does and just use whatever is
  //  there. This is somewhat limited compared to what can be done under
  //  the hood, but it makes the app simpler for everyone. This would
  //  make it easier for people to e.g. hook up HDMI or analog output or
  //  even analog or SPI audio output without us having to do that.
  //

  class KonaVideoDevice : public TwkGLF::GLBindableVideoDevice
  {
   public:
    //
    //  Types
    //

    using Timer =  TwkUtil::Timer;
    using GLFence = TwkGLF::GLFence;
    using BufferVector = std::vector<unsigned char*>;
    using ThreadGroup = stl_ext::thread_group;
    using AudioBuffer = std::vector<int>;
    using AudioBufferVector = std::vector<AudioBuffer>;
    using TimeVector = std::vector<Time>;
    using GLFBO = TwkGLF::GLFBO;
    using StatusStruct = AUTOCIRCULATE_TRANSFER_STATUS_STRUCT;
    using ChannelVector = std::vector<NTV2Channel>;
    using VPIDVector = std::vector<std::pair<ULWord, ULWord>>;

    //
    //  This is basically the ring buffer element. Each method uses
    //  different parts of the struct, but its treated as the internal
    //  ring buffer state by all of them (including P2P)
    //

    struct FrameData
    {
      enum class State
      {
        NotReady,
        Reading,
        Mapped,
        Transfering,
        NeedsUnmap,
        Ready
      };

      FrameData();
      FrameData( const FrameData& );
      ~FrameData();
      FrameData& operator=( const FrameData& );

      void lockImage( const char* threadName );
      void unlockImage();
      void lockAudio( const char* threadName );
      void unlockAudio();
      void lockState( const char* threadName );
      void unlockState();

      const GLFBO* fbo{ nullptr };
      GLuint globject{ 0 };
      unsigned char* mappedPointer{ nullptr };
      unsigned char* imageData{ nullptr };
      AudioBuffer audioBuffer;
      State state{ State::NotReady };
      GLFence* fence{ nullptr };
      bool locked{ false };

      Mutex imageMutex;
      Mutex audioMutex;
      Mutex stateMutex;
    };

    using FrameDataVector = std::vector<FrameData>;

    //
    //  VideoChannel is a logical indepenent output. For example the
    //  stereo modes require two VideoChannels: one for the left and
    //  one for the right eyes. Mono dual link output on the other
    //  hand only requires one video channel even thopugh it uses two
    //  outputs (the image is cut up between the outputs in that case)
    //
    //  NOTE: for 4K output we only need one VideoChannel. The HW will
    //  cut the image into quadrants for us.
    //

    struct VideoChannel
    {
      VideoChannel( NTV2FrameBufferFormat format, NTV2Channel ch, size_t bsize,
                    size_t n );
      ~VideoChannel();

      size_t bufferSizeInBytes;
      AUTOCIRCULATE_STATUS auto_status;
      AUTOCIRCULATE_TRANSFER
      auto_transfer;  // transferStatus in auto_transfer.acTransferStatus

      NTV2Channel channel;

      FrameDataVector data;
    };

    using VideoChannelVector = std::vector<VideoChannel*>;

    //
    //  See next comment for explanation
    //

    enum class OperationMode
    {
      ProMode,
      SimpleMode
    };

    //
    //  Constructors
    //
    //  There are two ways to create a KonaVideoDevice: either as a
    //  SimpleMode device that has no options and sees only what the
    //  AJA control panel is doing or as a ProMode device which lets
    //  the user take complete control over the device.
    //
    //  The SimpleMode case is only available on Mac and Windows. In
    //  this mode the user is responsible for setting up and managing
    //  routing, formats, etc, via the AJA control panel or the cables
    //  program. The advantage when using this method is that other
    //  card capabilities which we currently are not supporting (or
    //  for new hardware) such as SPI optical output or analog or HDMI
    //  video output can be setup by the user and used by our code.
    //
    //  In the ProMode case we enumerate all available modes the card
    //  (and we) can support and setup routing ourselves when the
    //  device is opened. This is way RVSDI for example works. The
    //  advantage here is that more esoteric output like RGB 12,
    //  stereo, or quad 4K, can be used.
    //

    KonaVideoDevice( AJAModule*, const std::string& name, unsigned int deviceIndex,
                     unsigned int appID, OperationMode mode = OperationMode::ProMode );

    ~KonaVideoDevice() override;

    //
    //  KonaVideoDevice API
    //

    Time outputTime() const override;
    Time inputTime() const override;
    void resetClock() const override;

    void beginTransfer() const override;
    void endTransfer() const override;

    //
    //  GLBindableVideoDevice API
    //

    bool isStereo() const override;
    bool isDualStereo() const override;

    bool willBlockOnTransfer() const override;
    void bind( const TwkGLF::GLVideoDevice* ) const override;
    void bind2( const TwkGLF::GLVideoDevice*,
                        const TwkGLF::GLVideoDevice* ) const override;
    void transfer( const TwkGLF::GLFBO* ) const override;
    void transfer2( const TwkGLF::GLFBO*, const TwkGLF::GLFBO* ) const override;

    void unbind() const override;

    //
    //  VideoDevice Audio API
    //
    size_t numAudioFormats() const override;
    AudioFormat audioFormatAtIndex( size_t ) const override;
    void setAudioFormat( size_t ) override;
    size_t currentAudioFormat() const override;
    void audioFrameSizeSequence( AudioFrameSizeVector& ) const override;
    void transferAudio( void* interleavedData, size_t n ) const override;

    //
    //  VideoDevice Video API
    //

    size_t asyncMaxMappedBuffers() const override;
    Time deviceLatency() const override;

    size_t numVideoFormats() const override;
    VideoFormat videoFormatAtIndex( size_t ) const override;
    void setVideoFormat( size_t ) override;
    size_t currentVideoFormat() const override;

    size_t numVideo4KTransports() const override;
    Video4KTransport video4KTransportAtIndex( size_t ) const override;
    void setVideo4KTransport( size_t ) override;
    size_t currentVideo4KTransport() const override;

    size_t numDataFormats() const override;
    DataFormat dataFormatAtIndex( size_t ) const override;
    void setDataFormat( size_t ) override;
    size_t currentDataFormat() const override;

    size_t numSyncModes() const override;
    SyncMode syncModeAtIndex( size_t ) const override;
    void setSyncMode( size_t ) override;
    size_t currentSyncMode() const override;

    size_t numSyncSources() const override;
    SyncSource syncSourceAtIndex( size_t ) const override;
    void setSyncSource( size_t ) override;
    size_t currentSyncSource() const override;

    Resolution resolution() const override;
    Offset offset() const override;  // defaults to 0,0
    Timing timing() const override;
    VideoFormat format() const override;
    size_t width() const override;
    size_t height() const override;
    void open( const StringVector& ) override;
    void close() override;
    bool isOpen() const override;
    virtual void makeCurrent() const;
    void clearCaches() const override;
    void syncBuffers() const override;

    void threadMain();
    void lockDevice( bool lock, const char* threadName = "" ) const;
    Mutex& deviceMutex() const
    {
      return m_deviceMutex;
    }

   private:
    bool initialize();
    void transferChannel( size_t i, const TwkGLF::GLFBO* ) const;
    void transferChannelPBO( VideoChannel*, const TwkGLF::GLFBO* ) const;
    void transferChannelReadPixels( VideoChannel*, const TwkGLF::GLFBO* ) const;
    unsigned int channelsFromFormat( NTV2FrameBufferFormat ) const;
    NTV2HDMIBitDepth getHDMIOutBitDepth( const NTV2FrameBufferFormat f ) const;

    void showAutoCirculateState( NTV2AutoCirculateState, int );

    void startGPUTransfer() const;
    void endGPUTransfer() const;
    void startAJATransfer() const;
    void endAJATransfer() const;
    void packBufferCopy( unsigned char*, size_t, unsigned char*, size_t,
                         size_t );

    void queryCard();
    unsigned int appID() const;

    int audioFormatChannelCountForCard() const;

    bool tsiEnabled() const;
    void parseHDMIHDRMetadata( std::string data );
    void setHDMIHDRMetadata();
    void routeQuadRGB( NTV2Standard standard, const KonaVideoFormat& f,
                       const KonaDataFormat& d );
    void routeStereoRGB( NTV2Standard standard, const KonaVideoFormat& f,
                         const KonaDataFormat& d );
    void routeMonoRGB( NTV2Standard standard, const KonaVideoFormat& f,
                       const KonaDataFormat& d );
    void routeQuadYUV( NTV2Standard standard, const KonaVideoFormat& f,
                       const KonaDataFormat& d );
    void routeStereoYUV( NTV2Standard standard, const KonaVideoFormat& f,
                         const KonaDataFormat& d );
    void routeMonoYUV( NTV2Standard standard, const KonaVideoFormat& f,
                       const KonaDataFormat& d );
    void routeMux( bool tsiEnabled );
    void routeCSC( bool tsiEnabled, bool outputIsRGB );
    void route4KDownConverter( bool tsiEnabled, bool outputIsRGB );
    void routeMonitorOut( bool tsiEnabled, bool outputIsRGB );
    void routeHDMI( NTV2Standard standard, const KonaDataFormat& d,
                    bool tsiEnabled, bool outputIsRGB );

    unsigned int m_appID{ 0 };
    unsigned int m_deviceIndex{ 0 };
    NTV2DeviceID m_deviceID{ DEVICE_ID_INVALID };
    CNTV2Card* m_card{ nullptr };
    NTV2EveryFrameTaskMode m_taskMode{ NTV2_TASK_MODE_INVALID };
    OperationMode m_operationMode{ OperationMode::ProMode };
    ChannelVector m_channelVector;
    VPIDVector m_channelVPIDVector;
    mutable bool m_bound{ false};
    KonaVideoFormatVector m_konaVideoFormats;
    KonaDataFormatVector m_konaDataFormats;
    KonaVideoFormat m_actualVideoFormat;
    KonaDataFormat m_actualDataFormat;
    bool m_stereo{ false };
    bool m_quad{ false };
    bool m_quadQuad{ false };  // quad-quad-frame (8K) squares mode on the device.
    VideoChannelVector m_videoChannels;
    bool m_bidirectional{ false };
    size_t m_deviceNumVideoOutputs{ 0 };
    size_t m_deviceNumVideoChannels{};
    bool m_deviceHasDualLink{ false};
    bool m_deviceHas3G{ false };
    ULWord m_deviceHDMIVersion{ 0 };
    bool m_deviceHasHDMIStereo{ false };
    bool m_deviceHas4KDownConverter{ false };
    bool m_deviceHasCSC{ false };
    UWord m_deviceMaxAudioChannels{ 0 };
    bool m_deviceHas96kAudio{ false };
    size_t m_numHDMIOutputs{ 0 };
    bool m_usePausing{ false };
    bool m_setDesiredFrame{ false };
    static bool m_infoFeedback;
    bool m_profile{ false };
    bool m_acquire{ false };
    bool m_pbos{ false };
    bool m_immediateCopy{ false };
    bool m_3GA{ false };
    bool m_3G{ false };
    bool m_3GB{ false };
    bool m_dualLink{ false };
    bool m_yuvInternalFormat{ false };
    bool m_allowSegmentedTransfer{ false };
    bool m_simpleRouting{ false };
    bool m_useHDMIHDRMetadata{ false };
    HDRFloatValues m_hdrMetadata;

    mutable ThreadGroup m_threadGroup;
    mutable Mutex m_deviceMutex;
    mutable bool m_threadStop{ false };
    mutable bool m_threadDone{ false };

    // before open
    unsigned int m_audioFormat{ 0 };
    unsigned int m_videoFormat{ 0 };
    unsigned int m_dataFormat{ 0 };
    unsigned int m_syncMode{ 0 };
    unsigned int m_syncSource{ 0 };
    unsigned int m_video4KTransport{ 0 };
    int m_internalAudioFormat{ 0 };
    int m_internalVideoFormat{ 0 };
    int m_internalDataFormat{ 0 };
    int m_internalSyncMode{ 0 };
    int m_internalVideo4KTransport{ 0 };
    int m_internalSyncSource{ 0 };

    // after open
    int m_outFormat{ 0 };
    int m_outRate{ 0 };
    int m_width{ 0 };
    int m_height{ 0 };
    int m_linePitch{ 0 };
    size_t m_channels{ 0 };
    bool m_open{ false };
    size_t m_bufferSize{ 0 };
    size_t m_bufferSizeInBytes{ 0 };
    size_t m_ringBufferSize{ 0 };
    size_t m_highwater{ 0 };
    bool m_paused{ false };
    bool m_starting{ false };
    GLenum m_fboInternalFormat{ GL_RGBA };
    GLenum m_textureFormat{ GL_RGBA };
    GLenum m_textureType{ GL_UNSIGNED_BYTE };
    mutable size_t m_texturePadding{ 0 };
    mutable size_t m_dvpBufferStride{ 0 };
    mutable size_t m_bufferStride{ 0 };
    int m_hdmiOutputBitDepthOverride{ 0 };

    mutable Timer m_globalTimer;
    mutable TimeVector m_gpuTimes;
    mutable TimeVector m_konaTimes;
    mutable TimeVector m_gpuBeginTime;
    mutable TimeVector m_konaBeginTime;

    // cache values
    mutable size_t m_bufferLevel{ 0 };
    mutable size_t m_mappedBufferCount{ 0 };
    mutable bool m_autocirculateRunning{ false };
    mutable size_t m_readBufferIndex{ 0 };
    mutable size_t m_readBufferCount{ 0 };
    mutable size_t m_writeBufferIndex{ 0 };
    mutable size_t m_writeBufferCount{ 0 };
    mutable unsigned int m_lastState{ 0 };
    mutable int m_lastBL{ 0 };
  };

}  // namespace AJADevices
