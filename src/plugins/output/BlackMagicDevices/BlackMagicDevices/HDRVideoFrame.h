//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <DeckLinkAPI.h>

#ifdef PLATFORM_LINUX
typedef const char* IDeckLinkVideoFrameMetadataExtensions_GetStringValueType;
typedef bool IDeckLinkVideoFrameMetadataExtensions_GetFlagValueType;
#endif

#ifdef PLATFORM_WINDOWS
typedef BSTR IDeckLinkVideoFrameMetadataExtensions_GetStringValueType;
typedef BOOL IDeckLinkVideoFrameMetadataExtensions_GetFlagValueType;
#endif

#ifdef PLATFORM_DARWIN
typedef CFStringRef IDeckLinkVideoFrameMetadataExtensions_GetStringValueType;
typedef bool IDeckLinkVideoFrameMetadataExtensions_GetFlagValueType;
#endif

#include <stdint.h>
#include <atomic>
#include <memory>
#include <string>

struct ChromaticityCoordinates
{
  double RedX{ 0.0 };
  double RedY{ 0.0 };
  double GreenX{ 0.0 };
  double GreenY{ 0.0 };
  double BlueX{ 0.0 };
  double BlueY{ 0.0 };
  double WhiteX{ 0.0 };
  double WhiteY{ 0.0 };
};

struct HDRMetadata
{
  ChromaticityCoordinates referencePrimaries;
  double minDisplayMasteringLuminance{ 0.0 };
  double maxDisplayMasteringLuminance{ 0.0 };
  double maxContentLightLevel{ 0.0 };
  double maxFrameAverageLightLevel{ 0.0 };
  int64_t electroOpticalTransferFunction{ 0 };
};

class HDRVideoFrame : public IDeckLinkVideoFrame,
                      public IDeckLinkVideoFrameMetadataExtensions
{
 public:
  HDRVideoFrame( IDeckLinkMutableVideoFrame* frame );
  virtual ~HDRVideoFrame() {}

  // IUnknown interface
  virtual HRESULT QueryInterface( REFIID iid, LPVOID* ppv );
  virtual ULONG AddRef( void );
  virtual ULONG Release( void );

  // IDeckLinkVideoFrame interface
  virtual long GetWidth( void )
  {
    return m_videoFrame->GetWidth();
  }
  virtual long GetHeight( void )
  {
    return m_videoFrame->GetHeight();
  }
  virtual long GetRowBytes( void )
  {
    return m_videoFrame->GetRowBytes();
  }
  virtual BMDPixelFormat GetPixelFormat( void )
  {
    return m_videoFrame->GetPixelFormat();
  }
  virtual BMDFrameFlags GetFlags( void )
  {
    return m_videoFrame->GetFlags() | bmdFrameContainsHDRMetadata;
  }
  virtual HRESULT GetBytes( void** buffer )
  {
    return m_videoFrame->GetBytes( buffer );
  }
  virtual HRESULT GetTimecode( BMDTimecodeFormat format,
                               IDeckLinkTimecode** timecode )
  {
    return m_videoFrame->GetTimecode( format, timecode );
  }
  virtual HRESULT GetAncillaryData( IDeckLinkVideoFrameAncillary** ancillary )
  {
    return m_videoFrame->GetAncillaryData( ancillary );
  }

  // IDeckLinkVideoFrameMetadataExtensions interface
  virtual HRESULT GetInt( BMDDeckLinkFrameMetadataID metadataID,
                          int64_t* value );
  virtual HRESULT GetFloat( BMDDeckLinkFrameMetadataID metadataID,
                            double* value );
  virtual HRESULT GetFlag(
      BMDDeckLinkFrameMetadataID metadataID,
      IDeckLinkVideoFrameMetadataExtensions_GetFlagValueType* value );
  virtual HRESULT GetString(
      BMDDeckLinkFrameMetadataID metadataID,
      IDeckLinkVideoFrameMetadataExtensions_GetStringValueType* value );
  virtual HRESULT GetBytes( BMDDeckLinkFrameMetadataID metadataID, void* buffer,
                            uint32_t* bufferSize );

  static std::string DumpHDRMetadata();
  static void SetHDRMetadata( const HDRMetadata& metadata )
  {
    s_metadata = metadata;
  }

  // Set the HDR metadata after parsing the HDR metadata passed as comma
  // separated values in a string Returns false if parsing error, true if
  // sucessful
  static bool SetHDRMetadata( const std::string& data );

 private:
  IDeckLinkMutableVideoFrame* m_videoFrame;
  static HDRMetadata s_metadata;
  std::atomic<ULONG> m_refCount;
};
