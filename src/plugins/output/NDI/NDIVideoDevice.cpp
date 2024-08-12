//
// Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <NDI/NDIVideoDevice.h>
#include <NDI/NDIModule.h>

#include <TwkExc/Exception.h>
#include <TwkFB/FastMemcpy.h>
#include <TwkFB/FastConversion.h>
#include <TwkGLF/GLFBO.h>
#include <TwkUtil/EnvVar.h>
#include <TwkUtil/TwkRegEx.h>
#include <TwkUtil/sgcHop.h>
#include <TwkUtil/sgcHopTools.h>
#include <array>
#include <boost/program_options.hpp>  // This has to come before the ByteSwap.h
#include <TwkUtil/ByteSwap.h>
#include <TwkGLF/GL.h>

#include <algorithm>
#include <cstddef>
#include <string>
#include <stl_ext/replace_alloc.h>

namespace NDI
{
  using namespace TwkApp;
  using namespace TwkGLF;
  using namespace TwkUtil;
  using namespace boost::program_options;

  constexpr int defaultRingBufferSize = 5;

  bool NDIVideoDevice::m_isInfoFeedback = false;

  struct NDISyncMode
  {
    size_t value;
    const char* description;
  };

  struct NDISyncSource
  {
    size_t value;
    const char* description;
  };

  static const std::vector<NDIDataFormat> dataFormats{
      { VideoDevice::RGBA8, NDIlib_FourCC_type_RGBA, true, "8-bit RGBA" },
      { VideoDevice::BGRA8, NDIlib_FourCC_type_BGRA, true, "8-bit BGRA" },
      { VideoDevice::YCbCr_P216_16_422, NDIlib_FourCC_video_type_P216, true,
        "16-bit YCbCr 4:2:2 (P216)" } };

  static const std::vector<NDIVideoFormat> videoFormats{
      { 1280, 720, 1.0, 23.98, 24000, 1001, "720p 23.98Hz" },
      { 1280, 720, 1.0, 24.00, 24000, 1000, "720p 24Hz" },
      { 1280, 720, 1.0, 29.97, 30000, 1001, "720p 29.97Hz" },
      { 1280, 720, 1.0, 30.00, 30000, 1000, "720p 30Hz" },
      { 1280, 720, 1.0, 50.00, 50000, 1000, "720p 50Hz" },
      { 1280, 720, 1.0, 59.94, 60000, 1001, "720p 59.94Hz" },
      { 1280, 720, 1.0, 60.00, 60000, 1000, "720p 60Hz" },
      { 1920, 1080, 1.0, 23.98, 24000, 1001, "1080p 23.98Hz" },
      { 1920, 1080, 1.0, 24.00, 24000, 1000, "1080p 24Hz" },
      { 1920, 1080, 1.0, 25.00, 25000, 1000, "1080p 25Hz" },
      { 1920, 1080, 1.0, 29.97, 30000, 1001, "1080p 29.97Hz" },
      { 1920, 1080, 1.0, 30.00, 30000, 1000, "1080p 30Hz" },
      { 1920, 1080, 1.0, 50.00, 50000, 1000, "1080p 50Hz" },
      { 1920, 1080, 1.0, 59.94, 60000, 1001, "1080p 59.94Hz" },
      { 1920, 1080, 1.0, 60.00, 60000, 1000, "1080p 60Hz" },
      { 2048, 1080, 1.0, 23.98, 24000, 1001,
        "1080p (2048x1080) DCI 2K 23.98Hz" },
      { 2048, 1080, 1.0, 24.00, 24000, 1000, "1080p (2048x1080) DCI 2K 24Hz" },
      { 2048, 1080, 1.0, 25.00, 25000, 1000, "1080p (2048x1080) DCI 2K 25Hz" },
      { 2048, 1080, 1.0, 29.97, 30000, 1001,
        "1080p (2048x1080) DCI 2K 29.97Hz" },
      { 2048, 1080, 1.0, 30.00, 30000, 1000, "1080p (2048x1080) DCI 2K 30Hz" },
      { 2048, 1080, 1.0, 50.00, 50000, 1000, "1080p (2048x1080) DCI 2K 50Hz" },
      { 2048, 1080, 1.0, 59.94, 60000, 1001,
        "1080p (2048x1080) DCI 2K 59.94Hz" },
      { 2048, 1080, 1.0, 60.00, 60000, 1000, "1080p (2048x1080) DCI 2K 60Hz" },
      { 3840, 2160, 1.0, 23.98, 24000, 1001,
        "2160p (3840x2160) UHD 4K 23.98Hz" },
      { 3840, 2160, 1.0, 24.00, 24000, 1000, "2160p (3840x2160) UHD 4K 24Hz" },
      { 3840, 2160, 1.0, 25.00, 25000, 1000, "2160p (3840x2160) UHD 4K 25Hz" },
      { 3840, 2160, 1.0, 29.97, 30000, 1001,
        "2160p (3840x2160) UHD 4K 29.97Hz" },
      { 3840, 2160, 1.0, 30.00, 30000, 1000, "2160p (3840x2160) UHD 4K 30Hz" },
      { 3840, 2160, 1.0, 50.00, 50000, 1000, "2160p (3840x2160) UHD 4K 50Hz" },
      { 3840, 2160, 1.0, 59.94, 60000, 1001,
        "2160p (3840x2160) UHD 4K 59.94Hz" },
      { 3840, 2160, 1.0, 60.00, 60000, 1000, "2160p (3840x2160) UHD 4K 60Hz" },
      { 4096, 2160, 1.0, 23.98, 24000, 1001,
        "2160p (4096x2160) DCI 4K 23.98Hz" },
      { 4096, 2160, 1.0, 24.00, 24000, 1000, "2160p (4096x2160) DCI 4K 24Hz" },
      { 4096, 2160, 1.0, 25.00, 25000, 1000, "2160p (4096x2160) DCI 4K 25Hz" },
      { 4096, 2160, 1.0, 29.97, 30000, 1001,
        "2160p (4096x2160) DCI 4K 29.97Hz" },
      { 4096, 2160, 1.0, 30.00, 30000, 1000, "2160p (4096x2160) DCI 4K 30Hz" },
      { 4096, 2160, 1.0, 50.00, 50000, 1000, "2160p (4096x2160) DCI 4K 50Hz" },
      { 4096, 2160, 1.0, 59.94, 50000, 1001,
        "2160p (4096x2160) DCI 4K 59.94Hz" },
      { 4096, 2160, 1.0, 60.00, 1, 1, "2160p (4096x2160) DCI 4K 60Hz" } };

  static const std::vector<NDIAudioFormat> audioFormats{
      { 48000, TwkAudio::Int16Format, 2, TwkAudio::Stereo_2,
        "16-bit 48kHz Stereo" },
      { 48000, TwkAudio::Int32Format, 2, TwkAudio::Stereo_2,
        "32-bit 48kHz Stereo" },
      { 48000, TwkAudio::Float32Format, 8, TwkAudio::Surround_7_1,
        "32-bit Float 48kHz 7.1 Surround" },
      { 48000, TwkAudio::Int32Format, 8, TwkAudio::SDDS_7_1,
        "32-bit 48kHz 7.1 Surround SDDS" },
      { 48000, TwkAudio::Int32Format, 16, TwkAudio::Generic_16,
        "32-bit 48kHz 16 channel" } };

  static const std::vector<NDISyncMode> syncModes{ { 0, "Free Running" } };

  NDIVideoDevice::PBOData::PBOData( GLuint g )
      : globject( g ), state( PBOData::State::Ready )
  {
    pthread_mutex_init( &mutex, nullptr );
    pthread_mutex_init( &stateMutex, nullptr );
  }

  NDIVideoDevice::PBOData::~PBOData()
  {
    pthread_mutex_destroy( &mutex );
    pthread_mutex_destroy( &stateMutex );
  }

  void NDIVideoDevice::PBOData::lockData()
  {
    Timer timer;
    timer.start();
    pthread_mutex_lock( &mutex );
    Time time = timer.elapsed();
    if( time > 0.001 )
    {
      std::cout << "lockData for " << time << '\n';
    }
  }

  void NDIVideoDevice::PBOData::unlockData()
  {
    pthread_mutex_unlock( &mutex );
  }

  void NDIVideoDevice::PBOData::lockState()
  {
    Timer timer;
    timer.start();
    pthread_mutex_lock( &stateMutex );
    Time time = timer.elapsed();
    if( time > 0.001 )
    {
      std::cout << "lockState for " << time << '\n';
    }
  }

  void NDIVideoDevice::PBOData::unlockState()
  {
    pthread_mutex_unlock( &stateMutex );
  }

  // Returns pixel size in bytes from NDI Video Type
  static int lineStrideInBytesFromNDIVideoType(
      const NDIlib_FourCC_video_type_e ndiVideoType, const int resX )
  {
    switch( ndiVideoType )
    {
      case NDIlib_FourCC_video_type_P216:
        return resX * sizeof( unsigned short );
      // Note: Returning a negative stride for BGRA,RGBA formats to compensate
      // for NDI expected image topness
      case NDIlib_FourCC_type_BGRA:
      case NDIlib_FourCC_type_RGBA:
        return -1 * resX * 4 * sizeof( unsigned char );
      default:
        std::cout << "ERROR: Unknown data format.\n";
#if defined( _DEBUG )
        assert( 0 );
#endif
        break;
    }
    return resX * 4;
  }

  // Returns the buffer size in bytes from NDI Video Type
  static size_t bufferSizeInBytesFromNDIVideoType(
      const NDIlib_FourCC_video_type_e ndiVideoType, const int resX,
      const int resY )
  {
    const size_t lineStrideInBytes =
        std::abs( lineStrideInBytesFromNDIVideoType( ndiVideoType, resX ) );
    switch( ndiVideoType )
    {
      case NDIlib_FourCC_video_type_P216:
        return 2 * lineStrideInBytes * resY;
      case NDIlib_FourCC_type_BGRA:
      case NDIlib_FourCC_type_RGBA:
        return lineStrideInBytes * resY;
      default:
        std::cout << "ERROR: Unknown data format.\n";
#if defined( _DEBUG )
        assert( 0 );
#endif
        break;
    }
    return lineStrideInBytes * resY;
  }

  //
  //  NDIVideoDevice
  //
  NDIVideoDevice::NDIVideoDevice( NDIModule* module, const std::string& name )
      : GLBindableVideoDevice( module, name,
                               BlockingTransfer | ImageOutput | ProvidesSync |
                                   FixedResolution | Clock | AudioOutput |
                                   NormalizedCoordinates ),
        m_readyFrame( nullptr ),
        m_needsFrameConverter( false ),
        m_hasAudio( false ),
        m_lastPboData( nullptr ),
        m_audioData{ nullptr },
        m_audioDataIndex( 0 ),
        m_isInitialized( false ),
        m_isPbos( true ),
        m_pboSize( defaultRingBufferSize ),
        m_videoFrameBufferSize( defaultRingBufferSize ),
        m_isOpen( false ),
        m_frameWidth( 0 ),
        m_frameHeight( 0 ),
        m_totalPlayoutFrames( 0 ),
        m_internalAudioFormat( 0 ),
        m_internalVideoFormat( 0 ),
        m_internalDataFormat( 0 ),
        m_internalSyncMode( 0 ),
        m_audioFormatSizeInBytes( 0 ),
        m_audioSamplesPerFrame( 0 ),
        m_audioChannelCount( 0 ),
        m_audioSampleRate( 0 ),
        m_audioFormat( TwkAudio::Int16Format ),
        m_textureFormat( 0 ),
        m_textureType( 0 )
  {
    m_audioFrameSizes.resize( 5 );

    //
    //  Add in all supported video formats
    //
    for( const NDIVideoFormat& videoFormat : videoFormats )
    {
      if( videoFormat.description == nullptr )
      {
        break;
      }
      bool videoFormatSupported = false;

      for( const NDIDataFormat& dataFormat : dataFormats )
      {
        if( dataFormat.description == nullptr )
        {
          break;
        }
        videoFormatSupported = true;
        if( m_ndiDataFormats.size() < dataFormats.size() )
        {
          m_ndiDataFormats.push_back( dataFormat );
        }
      }

      if( videoFormatSupported )
      {
        m_ndiVideoFormats.push_back( videoFormat );
      }
    }
  }

  NDIVideoDevice::~NDIVideoDevice()
  {
    if( m_isOpen )
    {
      close();
    }
  }

  size_t NDIVideoDevice::asyncMaxMappedBuffers() const
  {
    return m_pboSize;
  }

  VideoDevice::Time NDIVideoDevice::deviceLatency() const
  {
    return Time( 0 );
  }

  size_t NDIVideoDevice::numVideoFormats() const
  {
    return m_ndiVideoFormats.size();
  }

  NDIVideoDevice::VideoFormat NDIVideoDevice::videoFormatAtIndex(
      size_t index ) const
  {
    const NDIVideoFormat& videoFormat = m_ndiVideoFormats[index];
    return { static_cast<size_t>( videoFormat.width ),
             static_cast<size_t>( videoFormat.height ),
             videoFormat.pixelAspect,
             1.0F,
             static_cast<float>( videoFormat.hertz ),
             videoFormat.description };
  }

  void NDIVideoDevice::setVideoFormat( size_t index )
  {
    const size_t formatsCount = numVideoFormats();
    if( index >= formatsCount )
    {
      index = formatsCount - 1;
    }
    m_internalVideoFormat = index;

    //
    //  Update the data formats based on the video format
    //

    m_ndiDataFormats.clear();

    for( const NDIDataFormat& dataFormat : dataFormats )
    {
      if( dataFormat.description == nullptr )
      {
        break;
      }
      m_ndiDataFormats.push_back( dataFormat );
    }

    m_audioFrameSizes.clear();
  }

  size_t NDIVideoDevice::currentVideoFormat() const
  {
    return m_internalVideoFormat;
  }

  size_t NDIVideoDevice::numAudioFormats() const
  {
    return audioFormats.size();
  }

  NDIVideoDevice::AudioFormat NDIVideoDevice::audioFormatAtIndex(
      size_t index ) const
  {
    const NDIAudioFormat& audioFormat = audioFormats.at( index );
    return { static_cast<Time>( audioFormat.hertz ), audioFormat.precision,
             audioFormat.numChannels, audioFormat.layout,
             audioFormat.description };
  }

  void NDIVideoDevice::setAudioFormat( size_t index )
  {
    if( index > numAudioFormats() )
    {
      index = numAudioFormats() - 1;
    }
    m_internalAudioFormat = index;

    m_audioFrameSizes.clear();
  }

  size_t NDIVideoDevice::currentAudioFormat() const
  {
    return m_internalAudioFormat;
  }

  size_t NDIVideoDevice::numDataFormats() const
  {
    return m_ndiDataFormats.size();
  }

  NDIVideoDevice::DataFormat NDIVideoDevice::dataFormatAtIndex(
      size_t index ) const
  {
    return { m_ndiDataFormats[index].iformat,
             m_ndiDataFormats[index].description };
  }

  void NDIVideoDevice::setDataFormat( size_t index )
  {
    m_internalDataFormat = index;
  }

  size_t NDIVideoDevice::currentDataFormat() const
  {
    return m_internalDataFormat;
  }

  size_t NDIVideoDevice::numSyncSources() const
  {
    return 0;
  }

  NDIVideoDevice::SyncSource NDIVideoDevice::syncSourceAtIndex(
      size_t index ) const
  {
    return {};
  }

  size_t NDIVideoDevice::currentSyncSource() const
  {
    return 0;
  }

  size_t NDIVideoDevice::numSyncModes() const
  {
    return syncModes.size();
  }

  NDIVideoDevice::SyncMode NDIVideoDevice::syncModeAtIndex( size_t index ) const
  {
    const NDISyncMode& mode = syncModes.at( index );
    return { mode.description };
  }

  void NDIVideoDevice::setSyncMode( size_t index )
  {
    m_internalSyncMode = index;
  }

  size_t NDIVideoDevice::currentSyncMode() const
  {
    return m_internalSyncMode;
  }

  NDIVideoDevice::Timing NDIVideoDevice::timing() const
  {
    const NDIVideoFormat& videoFormat =
        m_ndiVideoFormats[m_internalVideoFormat];
    return { static_cast<float>( videoFormat.hertz ) };
  }

  NDIVideoDevice::VideoFormat NDIVideoDevice::format() const
  {
    const NDIVideoFormat& videoFormat =
        m_ndiVideoFormats[m_internalVideoFormat];
    return { static_cast<size_t>( videoFormat.width ),
             static_cast<size_t>( videoFormat.height ),
             videoFormat.pixelAspect,
             1.0F,
             static_cast<float>( videoFormat.hertz ),
             videoFormat.description };
  }

  void NDIVideoDevice::audioFrameSizeSequence(
      AudioFrameSizeVector& fsizes ) const
  {
    const int frameSizes = 5;
    m_audioFrameSizes.resize( frameSizes );
    fsizes.resize( frameSizes );
    for( size_t i = 0; i < frameSizes; i++ )
    {
      m_audioFrameSizes[i] = m_audioSamplesPerFrame;
      fsizes[i] = m_audioFrameSizes[i];
    }
  }

  void NDIVideoDevice::initialize()
  {
    if( m_isInitialized )
    {
      return;
    }

    m_isInitialized = true;
  }

  namespace
  {
    std::string mapToEnvVar( const std::string& name )
    {
      if( name == "RV_NDI_HELP" )
      {
        return "help";
      }
      if( name == "RV_NDI_VERBOSE" )
      {
        return "verbose";
      }
      if( name == "RV_NDI_METHOD" )
      {
        return "method";
      }
      if( name == "RV_NDI_RING_BUFFER_SIZE" )
      {
        return "ring-buffer-size";
      }
      return "";
    }
  }  // namespace

  void NDIVideoDevice::open( const StringVector& args )
  {
    if( !m_isInitialized )
    {
      initialize();
    }

    options_description description( "NDI Device Options" );

    description.add_options()( "help,h", "Usage Message" )(
        "verbose,v", "Verbose" )( "method,m", value<std::string>(),
                                  "Method (ipbo, basic)" )(
        "ring-buffer-size,s",
        value<int>()->default_value( defaultRingBufferSize ),
        "Ring Buffer Size" );

    variables_map variables;

    try
    {
      store( command_line_parser( args ).options( description ).run(),
             variables );
      store( parse_environment( description, mapToEnvVar ), variables );
      notify( variables );
    }
    catch( std::exception& e )
    {
      std::cout << "ERROR: DNI_ARGS: " << e.what() << '\n';
    }
    catch( ... )
    {
      std::cout << "ERROR: DNI_ARGS: exception\n";
    }

    if( variables.count( "help" ) > 0 )
    {
      std::cout << '\n' << description << '\n';
    }

    m_isInfoFeedback = variables.count( "verbose" ) > 0;

    if( variables.count( "ring-buffer-size" ) != 0 )
    {
      int ringBufferSize = variables["ring-buffer-size"].as<int>();
      m_pboSize = static_cast<size_t>( ringBufferSize );
      std::cout << "INFO: ringbuffer size " << ringBufferSize << '\n';
    }

    bool PBOsOK = true;
    if( variables.count( "method" ) != 0 )
    {
      std::string string = variables["method"].as<std::string>();

      if( string == "ipbo" )
      {
        PBOsOK = true;
      }
      else if( string == "basic" )
      {
        PBOsOK = false;
      }
    }

    m_isPbos = PBOsOK;

    if( m_isPbos )
    {
      std::cout << "INFO: using PBOs with immediate copy\n";
    }
    else
    {
      std::cout << "INFO: using basic readback\n";
    }

    m_audioDataIndex = 0;
    m_frameCount = 0;
    m_totalPlayoutFrames = 0;
    m_lastPboData = nullptr;

    // dynamically determine what pixel formats are supported based on the
    // desired video format
    m_ndiDataFormats.clear();

    for( const NDIDataFormat& dataFormat : dataFormats )
    {
      if( dataFormat.description == nullptr )
      {
        break;
      }
      m_ndiDataFormats.push_back( dataFormat );
    }

    const NDIVideoFormat& videoFormat =
        m_ndiVideoFormats[m_internalVideoFormat];
    m_frameWidth = static_cast<size_t>( videoFormat.width );
    m_frameHeight = static_cast<size_t>( videoFormat.height );

    const NDIDataFormat& dataFormat = m_ndiDataFormats[m_internalDataFormat];
    const std::string& dname = dataFormat.description;

    const NDIAudioFormat& audioFormat =
        audioFormats.at( m_internalAudioFormat );
    m_audioChannelCount = static_cast<unsigned long>( audioFormat.numChannels );
    m_audioFormat = audioFormat.precision;

    GLenumPair epair =
        TwkGLF::textureFormatFromDataFormat( dataFormat.iformat );
    m_textureFormat = epair.first;
    m_textureType = epair.second;

    m_videoFrameBufferSize = m_pboSize;

    m_ndiVideoFrame.xres = videoFormat.width;
    m_ndiVideoFrame.yres = videoFormat.height;
    m_ndiVideoFrame.FourCC = dataFormat.ndiFormat;
    m_ndiVideoFrame.frame_rate_N = static_cast<int>( videoFormat.frame_rate_N );
    m_ndiVideoFrame.frame_rate_D = static_cast<int>( videoFormat.frame_rate_D );

    m_audioSampleRate = static_cast<float>( audioFormat.hertz );
    const float frameDuration =
        videoFormat.frame_rate_D / videoFormat.frame_rate_N;
    m_audioSamplesPerFrame =
        static_cast<unsigned long>( m_audioSampleRate * frameDuration + 0.5 );

    // Allocate audio buffers
    m_audioFormatSizeInBytes = TwkAudio::formatSizeInBytes( m_audioFormat );
    m_audioData = new char[2 * m_audioSamplesPerFrame * m_audioChannelCount *
                           m_audioFormatSizeInBytes];
    memset( m_audioData, 0,
            2 * m_audioSamplesPerFrame * m_audioChannelCount *
                m_audioFormatSizeInBytes );

    if( m_audioFormat == TwkAudio::Int16Format )
    {
      m_ndiInterleaved16AudioFrame.sample_rate = audioFormat.hertz;
      m_ndiInterleaved16AudioFrame.no_channels =
          static_cast<int>( audioFormat.numChannels );
      m_ndiInterleaved16AudioFrame.no_samples =
          static_cast<int>( m_audioSamplesPerFrame );
    }
    else if( m_audioFormat == TwkAudio::Float32Format )
    {
      m_ndiInterleaved32fAudioFrame.sample_rate = audioFormat.hertz;
      m_ndiInterleaved32fAudioFrame.no_channels =
          static_cast<int>( audioFormat.numChannels );
      m_ndiInterleaved32fAudioFrame.no_samples =
          static_cast<int>( m_audioSamplesPerFrame );
    }
    else
    {
      m_ndiInterleaved32AudioFrame.sample_rate = audioFormat.hertz;
      m_ndiInterleaved32AudioFrame.no_channels =
          static_cast<int>( audioFormat.numChannels );
      m_ndiInterleaved32AudioFrame.no_samples =
          static_cast<int>( m_audioSamplesPerFrame );
    }

    if( !dataFormat.isRGB )
    {
      m_needsFrameConverter = true;
    }

    //
    // Create a queue of NDIVideoFrame objects to use for
    // scheduling output video frames.
    //
    const size_t bufferSizeInBytes = bufferSizeInBytesFromNDIVideoType(
        m_ndiVideoFrame.FourCC, m_frameWidth, m_frameHeight );
    for( size_t i = 0; i < m_videoFrameBufferSize; i++ )
    {
      NDIVideoFrame* outputFrame = nullptr;
      const NDIDataFormat& internalDataFormat =
          m_ndiDataFormats[m_internalDataFormat];

      outputFrame = new NDIVideoFrame[bufferSizeInBytes];
      std::memset( outputFrame, 0, bufferSizeInBytes );
      m_DLOutputVideoFrameQueue.push_back( outputFrame );

      if( !m_needsFrameConverter )
      {
        continue;
      }

      NDIVideoFrame* readbackFrame = nullptr;

      // TODO: Allocate readbackFrame for conversion
      m_DLReadbackVideoFrameQueue.push_back( readbackFrame );
    }

    m_readyFrame = m_DLOutputVideoFrameQueue.at( 0 );

    // Create an NDI sender
    m_ndiSender = NDIlib_send_create();
    if( m_ndiSender == nullptr )
    {
      std::cout << "ERROR: Could not create NDI sender.\n";
    }

    m_isOpen = true;
  }

  void NDIVideoDevice::close()
  {
    if( m_isOpen )
    {
      m_isOpen = false;

      unbind();

      m_hasAudio = false;
      if( m_audioData != nullptr )
      {
        delete[] m_audioData;
        m_audioData = nullptr;
      }
      m_needsFrameConverter = false;

      for( const auto& frame : m_DLOutputVideoFrameQueue )
      {
        NDIVideoFrame* ndiVideoFrame = static_cast<NDIVideoFrame*>( frame );
        delete[] ndiVideoFrame;
      }

      m_DLOutputVideoFrameQueue.clear();

      for( const auto& frame : m_DLReadbackVideoFrameQueue )
      {
        NDIVideoFrame* ndiVideoFrame = static_cast<NDIVideoFrame*>( frame );
        delete[] ndiVideoFrame;
      }

      m_DLReadbackVideoFrameQueue.clear();

      if( m_ndiSender != nullptr )
      {
        NDIlib_send_destroy( m_ndiSender );
        m_ndiSender = nullptr;
      }
    }

    TwkGLF::GLBindableVideoDevice::close();
  }

  bool NDIVideoDevice::isOpen() const
  {
    return m_isOpen;
  }

  void NDIVideoDevice::clearCaches() const {}

  void NDIVideoDevice::transferAudio( void* data, size_t ) const
  {
    if( data == nullptr )
    {
      m_hasAudio = false;
      return;
    }
    m_hasAudio = true;

    std::memcpy( &m_audioData[m_audioDataIndex * m_audioFormatSizeInBytes *
                              m_audioSamplesPerFrame * m_audioChannelCount],
                 data,
                 m_audioFormatSizeInBytes * m_audioSamplesPerFrame *
                     m_audioChannelCount );
    m_audioDataIndex = ( m_audioDataIndex + 1 ) % 2;
  }

  bool NDIVideoDevice::transferChannel( size_t index, const GLFBO* fbo ) const
  {
    HOP_PROF_FUNC();

    fbo->bind();

    NDIVideoFrame* readbackVideoFrame = nullptr;
    NDIVideoFrame* outputVideoFrame = m_DLOutputVideoFrameQueue.front();
    m_DLOutputVideoFrameQueue.push_back( outputVideoFrame );
    m_DLOutputVideoFrameQueue.pop_front();

    if( m_needsFrameConverter )
    {
      readbackVideoFrame = m_DLReadbackVideoFrameQueue.front();
      m_DLReadbackVideoFrameQueue.push_back( readbackVideoFrame );
      m_DLReadbackVideoFrameQueue.pop_front();
    }

    if( m_isPbos )
    {
      transferChannelPBO( index, fbo, outputVideoFrame, readbackVideoFrame );
    }
    else
    {
      transferChannelReadPixels( index, fbo, outputVideoFrame,
                                 readbackVideoFrame );
    }

    m_readyFrame = outputVideoFrame;

    // Set the new data pointer and calculate line stride
    // Note that the NDI SDK is expecting a top-down image whereas OpenGL is
    // giving a bottom up image for image formats without conversion.
    // So we might need to flip the frame vertically .
    // For this we'll tell NDI to use a negative stride and
    // tell it the frame's first line is at the end of the source buffer.
    const int lineStrideInBytes = lineStrideInBytesFromNDIVideoType(
        m_ndiVideoFrame.FourCC, m_ndiVideoFrame.xres );
    m_ndiVideoFrame.line_stride_in_bytes = lineStrideInBytes;
    m_ndiVideoFrame.p_data =
        ( lineStrideInBytes > 0 )
            ? m_readyFrame
            : ( m_readyFrame +
                ( m_ndiVideoFrame.yres - 1 ) * std::abs( lineStrideInBytes ) );

    if( m_hasAudio )
    {
      const size_t index = ( m_audioDataIndex == 1 ) ? 0 : 1;
      if( m_audioFormat == TwkAudio::Int16Format )
      {
        m_ndiInterleaved16AudioFrame.p_data = reinterpret_cast<short*>(
            &m_audioData[index * m_audioSamplesPerFrame * m_audioChannelCount *
                         m_audioFormatSizeInBytes] );
        NDIlib_util_send_send_audio_interleaved_16s(
            m_ndiSender, &m_ndiInterleaved16AudioFrame );
      }
      else if( m_audioFormat == TwkAudio::Float32Format )
      {
        m_ndiInterleaved32fAudioFrame.p_data = reinterpret_cast<float*>(
            &m_audioData[index * m_audioSamplesPerFrame * m_audioChannelCount *
                         m_audioFormatSizeInBytes] );
        NDIlib_util_send_send_audio_interleaved_32f(
            m_ndiSender, &m_ndiInterleaved32fAudioFrame );
      }
      else
      {
        m_ndiInterleaved32AudioFrame.p_data = reinterpret_cast<int*>(
            &m_audioData[index * m_audioSamplesPerFrame * m_audioChannelCount *
                         m_audioFormatSizeInBytes] );
        NDIlib_util_send_send_audio_interleaved_32s(
            m_ndiSender, &m_ndiInterleaved32AudioFrame );
      }
    }

    // Send the video frame
    NDIlib_send_send_video_v2( m_ndiSender, &m_ndiVideoFrame );

    return true;
  }

  void NDIVideoDevice::transferChannelPBO( size_t index, const GLFBO* fbo,
                                           NDIVideoFrame* outputVideoFrame,
                                           NDIVideoFrame* ) const
  {
    HOP_CALL( glFinish(); )
    HOP_PROF_FUNC();

    PBOData* lastPboData = m_lastPboData;

    const NDIDataFormat& dataFormat = m_ndiDataFormats[m_internalDataFormat];

    // GL_UNSIGNED_INT_10_10_10_2 is a non native GPU format which leads to
    // costly GPU readbacks. To work around this performance issue we will use
    // the GPU native GL_UNSIGNED_INT_2_10_10_10_REV format instead and perform
    // a CPU format conversion after readback. This is much more optimal than
    // letting the GPU do the conversion during readback. For reference:
    // SG-14524
    GLenum textureTypeToUse = m_textureType;
    bool perform_ABGR10_to_RGBA10_conversion = false;
    if( m_textureType == GL_UNSIGNED_INT_10_10_10_2 )
    {
      textureTypeToUse = GL_UNSIGNED_INT_2_10_10_10_REV;
      perform_ABGR10_to_RGBA10_conversion = true;
    }

    if( lastPboData != nullptr )
    {
      glBindBuffer( GL_PIXEL_PACK_BUFFER_ARB, lastPboData->globject );
      TWK_GLDEBUG;

      lastPboData->lockState();
      lastPboData->state = PBOData::State::Transferring;
      lastPboData->unlockState();

      int* p = static_cast<int*>(
          glMapBuffer( GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY_ARB ) );
      TWK_GLDEBUG;
      if( p != nullptr )
      {
        void* pFrame = nullptr;
        pFrame = outputVideoFrame;

        if( dataFormat.iformat == VideoDevice::YCbCr_P216_16_422 )
        {
          const size_t inBufStride = m_frameWidth * sizeof( uint32_t );
          const size_t outBufStride = m_frameWidth * sizeof( uint16_t );
          uint16_t* outY = reinterpret_cast<uint16_t*>( pFrame );
          uint16_t* outCbCr = reinterpret_cast<uint16_t*>(
              reinterpret_cast<uint8_t*>( pFrame ) +
              m_frameHeight * outBufStride );
          packedYUV444_10bits_to_P216_MP(
              m_frameWidth, m_frameHeight, reinterpret_cast<uint32_t*>( p ),
              outY, outCbCr, inBufStride, outBufStride, true /*flip*/ );
        }
        else
        {
          if( perform_ABGR10_to_RGBA10_conversion )
          {
            convert_ABGR10_to_RGBA10_MP(
                m_frameWidth, m_frameHeight, reinterpret_cast<uint32_t*>( p ),
                reinterpret_cast<uint32_t*>( pFrame ) );
          }
          else
          {
            FastMemcpy_MP( pFrame, p, m_frameHeight * m_frameWidth * 4 );
          }
        }
      }

      glUnmapBuffer( GL_PIXEL_PACK_BUFFER_ARB );
      TWK_GLDEBUG;
      lastPboData->lockState();
      lastPboData->state = PBOData::State::Ready;
      lastPboData->fbo->endExternalReadback();
      lastPboData->fbo = nullptr;
      lastPboData->unlockState();
    }

    {
#if defined( HOP_ENABLED )
      std::string hopMsg = std::string( "glReadPixels() next PBO" );
      const size_t pixelSize =
          TwkGLF::pixelSizeFromTextureFormat( m_textureFormat, m_textureType );
      hopMsg += std::string( " - width=" ) + std::to_string( m_frameWidth ) +
                std::string( ", height=" ) + std::to_string( m_frameHeight ) +
                std::string( ", pixelSize=" ) + std::to_string( pixelSize ) +
                std::string( ", textureFormat=" ) +
                std::to_string( m_textureFormat ) +
                std::string( ", textureType=" ) +
                std::to_string( m_textureType );
      HOP_PROF_DYN_NAME( hopMsg.c_str() );
#endif

      // next pbo read
      PBOData* pboData = m_pboQueue.front();
      if( pboData == nullptr )
      {
        std::cerr << "ERROR: pboData is NULL!\n";
      }
      else
      {
        m_pboQueue.push_back( pboData );
        m_pboQueue.pop_front();
      }
      glBindBuffer( GL_PIXEL_PACK_BUFFER_ARB, pboData->globject );
      TWK_GLDEBUG;
      pboData->lockState();
      pboData->fbo = fbo;
      bool isUnmapit = pboData->state == PBOData::State::Mapped;
      pboData->unlockState();

      if( isUnmapit )
      {
        glUnmapBuffer( GL_PIXEL_PACK_BUFFER_ARB );
        TWK_GLDEBUG;
      }

      glReadPixels( 0, 0, static_cast<int>( m_frameWidth ),
                    static_cast<int>( m_frameHeight ), m_textureFormat,
                    textureTypeToUse, nullptr );
      TWK_GLDEBUG;

      m_lastPboData = pboData;

      glBindBuffer( GL_PIXEL_PACK_BUFFER_ARB, 0 );
      HOP_CALL( glFinish(); )
    }
  }

  void NDIVideoDevice::transferChannelReadPixels(
      size_t, const GLFBO* fbo, NDIVideoFrame* outputVideoFrame,
      NDIVideoFrame* readbackVideoFrame ) const
  {
    HOP_PROF_FUNC();

    void* pFrame = nullptr;
    if( m_needsFrameConverter )
    {
      pFrame = readbackVideoFrame;
    }
    else
    {
      pFrame = outputVideoFrame;
    }

    const NDIDataFormat& dataFormat = m_ndiDataFormats[m_internalDataFormat];

    TwkUtil::Timer timer;
    timer.start();

    glReadPixels( 0, 0, static_cast<int>( m_frameWidth ),
                  static_cast<int>( m_frameHeight ), m_textureFormat,
                  m_textureType, pFrame );

    timer.stop();

    fbo->endExternalReadback();

    if( m_needsFrameConverter )
    {
      TwkUtil::Timer converterTimer;
      converterTimer.start();

      void* outData = nullptr;
      outData = outputVideoFrame;
      if( dataFormat.iformat == VideoDevice::CbY0CrY1_8_422 )
      {
        subsample422_8bit_UYVY_MP( m_frameWidth, m_frameHeight,
                                   reinterpret_cast<uint8_t*>( pFrame ),
                                   reinterpret_cast<uint8_t*>( outData ) );
      }
      else
      {
        subsample422_10bit_MP( m_frameWidth, m_frameHeight,
                               reinterpret_cast<uint32_t*>( pFrame ),
                               reinterpret_cast<uint32_t*>( outData ),
                               m_frameWidth * sizeof( uint32_t ),
                               m_frameWidth * sizeof( uint32_t ) );
      }

      converterTimer.stop();
    }
  }

  void NDIVideoDevice::transfer( const GLFBO* fbo ) const
  {
    HOP_ZONE( HOP_ZONE_COLOR_4 );
    HOP_PROF_FUNC();

    if( !m_isOpen )
    {
      return;
    }

    transferChannel( 0, fbo );

    incrementClock();
  }

  bool NDIVideoDevice::willBlockOnTransfer() const
  {
    return false;
  }

  bool NDIVideoDevice::readyForTransfer() const
  {
    return true;
  }

  void NDIVideoDevice::transfer2( const GLFBO* fbo1, const GLFBO* fbo2 ) const
  {
    if( !m_isOpen )
    {
      return;
    }

    if( m_isPbos )
    {
      // Note transferChannelPBO() 'fbo' argument represents
      // the fbo to be read into the next pbo; hence we call transferChannel()
      // with the right eye fbo first.
      transferChannel( 0, fbo2 );
      transferChannel( 1, fbo1 );
    }
    else
    {
      transferChannel( 0, fbo1 );
      transferChannel( 1, fbo2 );
    }

    incrementClock();
  }

  void NDIVideoDevice::unbind() const
  {
    if( m_isPbos )
    {
      for( const auto& pbo : m_pboQueue )
      {
        PBOData* data = pbo;
        if( data->state == PBOData::State::NeedsUnmap )
        {
          glBindBuffer( GL_PIXEL_PACK_BUFFER_ARB, data->globject );
          TWK_GLDEBUG;
          glUnmapBuffer( GL_PIXEL_PACK_BUFFER_ARB );
          TWK_GLDEBUG;
          glBindBuffer( GL_PIXEL_PACK_BUFFER, 0 );
        }
        glDeleteBuffers( 1, &( data->globject ) );
        TWK_GLDEBUG;
        delete pbo;
      }

      m_pboQueue.clear();
    }
  }

  void NDIVideoDevice::bind( const GLVideoDevice* ) const
  {
    if( m_isPbos )
    {
      size_t num = m_pboSize;

      for( size_t i = 0; i < num; i++ )
      {
        GLuint glObject = 0;
        glGenBuffers( 1, &glObject );
        TWK_GLDEBUG;
        glBindBuffer( GL_PIXEL_PACK_BUFFER_ARB, glObject );
        TWK_GLDEBUG;
        glBufferData( GL_PIXEL_PACK_BUFFER_ARB,
                      static_cast<long>( m_frameWidth * 4 * m_frameHeight ),
                      nullptr, GL_STATIC_READ );
        TWK_GLDEBUG;  // read back is always in 4 bytes
        glBindBuffer( GL_PIXEL_PACK_BUFFER_ARB, 0 );
        TWK_GLDEBUG;

        m_pboQueue.push_back( new PBOData( glObject ) );
      }
    }

    resetClock();
  }

  void NDIVideoDevice::bind2( const GLVideoDevice* device,
                              const GLVideoDevice* ) const
  {
    bind( device );
  }

}  // namespace NDI
