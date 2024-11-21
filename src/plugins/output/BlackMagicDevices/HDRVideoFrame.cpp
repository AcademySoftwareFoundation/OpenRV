//
// SPDX-License-Identifier: Apache-2.0
//

#include <BlackMagicDevices/HDRVideoFrame.h>

#include <cstring>
#include <sstream>

HDRMetadata HDRVideoFrame::s_metadata = HDRMetadata();

HDRVideoFrame::HDRVideoFrame( IDeckLinkMutableVideoFrame* frame )
    : m_videoFrame( frame ), m_refCount( 1 )
{
}

/// IUnknown methods

HRESULT HDRVideoFrame::QueryInterface( REFIID iid, LPVOID* ppv )
{
  HRESULT result = S_OK;

#ifdef PLATFORM_WINDOWS
  REFIID iunknown = IID_IUnknown;
#else
  CFUUIDBytes iunknown = CFUUIDGetUUIDBytes( IUnknownUUID );
#endif

  // Initialise the return result
  *ppv = nullptr;

  if( memcmp( &iid, &iunknown, sizeof( REFIID ) ) == 0 )
  {
    *ppv = this;
    AddRef();
  }
  else if( memcmp( &iid, &IID_IDeckLinkVideoFrame, sizeof( REFIID ) ) == 0 )
  {
    *ppv = static_cast<IDeckLinkVideoFrame*>( this );
    AddRef();
  }
  else if( memcmp( &iid, &IID_IDeckLinkVideoFrameMetadataExtensions,
                   sizeof( REFIID ) ) == 0 )
  {
    *ppv = static_cast<IDeckLinkVideoFrameMetadataExtensions*>( this );
    AddRef();
  }
  else
  {
    result = E_NOINTERFACE;
  }

  return result;
}

ULONG HDRVideoFrame::AddRef( void )
{
  return ++m_refCount;
}

ULONG HDRVideoFrame::Release( void )
{
  ULONG refCount = --m_refCount;
  if( refCount == 0 )
  {
    delete this;
    return 0;
  }
  return refCount;
}

/// IDeckLinkVideoFrameMetadataExtensions methods

HRESULT HDRVideoFrame::GetInt( BMDDeckLinkFrameMetadataID metadataID,
                               int64_t* value )
{
  HRESULT result = S_OK;

  switch( metadataID )
  {
    case bmdDeckLinkFrameMetadataHDRElectroOpticalTransferFunc:
      *value = s_metadata.electroOpticalTransferFunction;
      break;

    case bmdDeckLinkFrameMetadataColorspace:
      *value = bmdColorspaceRec2020;
      break;

    default:
      value = nullptr;
      result = E_INVALIDARG;
  }

  return result;
}

HRESULT HDRVideoFrame::GetFloat( BMDDeckLinkFrameMetadataID metadataID,
                                 double* value )
{
  HRESULT result = S_OK;

  switch( metadataID )
  {
    case bmdDeckLinkFrameMetadataHDRDisplayPrimariesRedX:
      *value = s_metadata.referencePrimaries.RedX;
      break;

    case bmdDeckLinkFrameMetadataHDRDisplayPrimariesRedY:
      *value = s_metadata.referencePrimaries.RedY;
      break;

    case bmdDeckLinkFrameMetadataHDRDisplayPrimariesGreenX:
      *value = s_metadata.referencePrimaries.GreenX;
      break;

    case bmdDeckLinkFrameMetadataHDRDisplayPrimariesGreenY:
      *value = s_metadata.referencePrimaries.GreenY;
      break;

    case bmdDeckLinkFrameMetadataHDRDisplayPrimariesBlueX:
      *value = s_metadata.referencePrimaries.BlueX;
      break;

    case bmdDeckLinkFrameMetadataHDRDisplayPrimariesBlueY:
      *value = s_metadata.referencePrimaries.BlueY;
      break;

    case bmdDeckLinkFrameMetadataHDRWhitePointX:
      *value = s_metadata.referencePrimaries.WhiteX;
      break;

    case bmdDeckLinkFrameMetadataHDRWhitePointY:
      *value = s_metadata.referencePrimaries.WhiteY;
      break;

    case bmdDeckLinkFrameMetadataHDRMaxDisplayMasteringLuminance:
      *value = s_metadata.maxDisplayMasteringLuminance;
      break;

    case bmdDeckLinkFrameMetadataHDRMinDisplayMasteringLuminance:
      *value = s_metadata.minDisplayMasteringLuminance;
      break;

    case bmdDeckLinkFrameMetadataHDRMaximumContentLightLevel:
      *value = s_metadata.maxContentLightLevel;
      break;

    case bmdDeckLinkFrameMetadataHDRMaximumFrameAverageLightLevel:
      *value = s_metadata.maxFrameAverageLightLevel;
      break;

    default:
      value = nullptr;
      result = E_INVALIDARG;
  }

  return result;
}

HRESULT HDRVideoFrame::GetFlag(
    BMDDeckLinkFrameMetadataID metadataID,
    IDeckLinkVideoFrameMetadataExtensions_GetFlagValueType* value )
{
  // Not expecting GetFlag
  return E_INVALIDARG;
}

HRESULT HDRVideoFrame::GetString(
    BMDDeckLinkFrameMetadataID metadataID,
    IDeckLinkVideoFrameMetadataExtensions_GetStringValueType* value )
{
  // Not expecting GetString
  return E_INVALIDARG;
}

HRESULT HDRVideoFrame::GetBytes( BMDDeckLinkFrameMetadataID metadataID,
                                 void* buffer, uint32_t* bufferSize )
{
  // Not expecting GetString
  return E_INVALIDARG;
}

std::string HDRVideoFrame::DumpHDRMetadata()
{
  std::ostringstream hdrMetedata;

  hdrMetedata << "redPrimaryX:                    "
              << s_metadata.referencePrimaries.RedX << std::endl
              << "redPrimaryY:                    "
              << s_metadata.referencePrimaries.RedY << std::endl
              << "greenPrimaryX:                  "
              << s_metadata.referencePrimaries.GreenX << std::endl
              << "greenPrimaryY:                  "
              << s_metadata.referencePrimaries.GreenY << std::endl
              << "bluePrimaryX:                   "
              << s_metadata.referencePrimaries.BlueX << std::endl
              << "bluePrimaryY:                   "
              << s_metadata.referencePrimaries.BlueY << std::endl
              << "whitePointX:                    "
              << s_metadata.referencePrimaries.WhiteX << std::endl
              << "whitePointY:                    "
              << s_metadata.referencePrimaries.WhiteY << std::endl
              << "minMasteringLuminance:          "
              << s_metadata.minDisplayMasteringLuminance << std::endl
              << "maxMasteringLuminance:          "
              << s_metadata.maxDisplayMasteringLuminance << std::endl
              << "maxContentLightLevel:           "
              << s_metadata.maxContentLightLevel << std::endl
              << "maxFrameAverageLightLevel:      "
              << s_metadata.maxFrameAverageLightLevel << std::endl
              << "electroOpticalTransferFunction: "
              << s_metadata.electroOpticalTransferFunction << std::endl;

  return hdrMetedata.str();
}

bool HDRVideoFrame::SetHDRMetadata( const std::string& data )
{
  // Very, very basic and brutal parsing routine for the hdr
  // argument string.
  // No error checking, no validation, no plan B, data must be
  // exactly as expected or it won't work.
  //
  // Sample values :
  //    redPrimaryX:                    0.708000004
  //    redPrimaryY:                    0.291999996
  //    greenPrimaryX:                  0.170000002
  //    greenPrimaryY:                  0.79699999
  //    bluePrimaryX:                   0.130999997
  //    bluePrimaryY:                   0.0460000001
  //    whitePointX:                    0.312700003
  //    whitePointY:                    0.328999996
  //    minMasteringLuminance:          0.00499999989
  //    maxMasteringLuminance:          10000.0
  //    maxContentLightLevel:           0.0
  //    maxFrameAverageLightLevel:      0.0
  //    electroOpticalTransferFunction: 2
  //
  // Expected format (to be entered in the "Additional Options"
  // box of the Video preferences panel):
  //
  // --hdmi-hdr-metadata=0.708000004,0.291999996,0.170000002,0.79699999,0.130999997,0.0460000001,0.312700003,0.328999996,0.00499999989,10000.0,0.0,0.0,2
  //
  // or via env var:
  //
  // setenv TWK_BLACKMAGIC_HDMI_HDR_METADATA
  // "0.708000004,0.291999996,0.170000002,0.79699999,0.130999997,0.0460000001,0.312700003,0.328999996,0.00499999989,10000.0,0.0,0.0,2"
  //
  // (the types - float vs int - are important!)

  // redPrimaryX
  size_t pos0 = 0;
  size_t pos1 = data.find( ",", pos0 );
  if( std::string::npos == pos1 ) return false;
  s_metadata.referencePrimaries.RedX =
      atof( data.substr( pos0, pos1 - pos0 ).c_str() );
  pos0 = pos1 + 1;

  // redPrimaryY
  pos1 = data.find( ",", pos0 );
  if( std::string::npos == pos1 ) return false;
  s_metadata.referencePrimaries.RedY =
      atof( data.substr( pos0, pos1 - pos0 ).c_str() );
  pos0 = pos1 + 1;

  // greenPrimaryX
  pos1 = data.find( ",", pos0 );
  if( std::string::npos == pos1 ) return false;
  s_metadata.referencePrimaries.GreenX =
      atof( data.substr( pos0, pos1 - pos0 ).c_str() );
  pos0 = pos1 + 1;

  // greenPrimaryY
  pos1 = data.find( ",", pos0 );
  if( std::string::npos == pos1 ) return false;
  s_metadata.referencePrimaries.GreenY =
      atof( data.substr( pos0, pos1 - pos0 ).c_str() );
  pos0 = pos1 + 1;

  // bluePrimaryX
  pos1 = data.find( ",", pos0 );
  if( std::string::npos == pos1 ) return false;
  s_metadata.referencePrimaries.BlueX =
      atof( data.substr( pos0, pos1 - pos0 ).c_str() );
  pos0 = pos1 + 1;

  // bluePrimaryY
  pos1 = data.find( ",", pos0 );
  if( std::string::npos == pos1 ) return false;
  s_metadata.referencePrimaries.BlueY =
      atof( data.substr( pos0, pos1 - pos0 ).c_str() );
  pos0 = pos1 + 1;

  // whitePointX
  pos1 = data.find( ",", pos0 );
  if( std::string::npos == pos1 ) return false;
  s_metadata.referencePrimaries.WhiteX =
      atof( data.substr( pos0, pos1 - pos0 ).c_str() );
  pos0 = pos1 + 1;

  // whitePointY
  pos1 = data.find( ",", pos0 );
  if( std::string::npos == pos1 ) return false;
  s_metadata.referencePrimaries.WhiteY =
      atof( data.substr( pos0, pos1 - pos0 ).c_str() );
  pos0 = pos1 + 1;

  // minMasteringLuminance
  pos1 = data.find( ",", pos0 );
  if( std::string::npos == pos1 ) return false;
  s_metadata.minDisplayMasteringLuminance =
      atof( data.substr( pos0, pos1 - pos0 ).c_str() );
  pos0 = pos1 + 1;

  // maxMasteringLuminance
  pos1 = data.find( ",", pos0 );
  if( std::string::npos == pos1 ) return false;
  s_metadata.maxDisplayMasteringLuminance =
      atof( data.substr( pos0, pos1 - pos0 ).c_str() );
  pos0 = pos1 + 1;

  // maxContentLightLevel
  pos1 = data.find( ",", pos0 );
  if( std::string::npos == pos1 ) return false;
  s_metadata.maxContentLightLevel =
      atof( data.substr( pos0, pos1 - pos0 ).c_str() );
  pos0 = pos1 + 1;

  // maxFrameAverageLightLevel
  pos1 = data.find( ",", pos0 );
  if( std::string::npos == pos1 ) return false;
  s_metadata.maxFrameAverageLightLevel =
      atof( data.substr( pos0, pos1 - pos0 ).c_str() );
  pos0 = pos1 + 1;

  // electroOpticalTransferFunction
  pos1 = data.find( ",", pos0 );
  if( pos1 != std::string::npos )
  {
    // Note : it is ok to pass npos for the length if there is no more commas
    // However, if there is another comma then we'll adjust the length
    // accordingly
    pos1 -= pos0;
  }
  s_metadata.electroOpticalTransferFunction =
      atoi( data.substr( pos0, pos1 ).c_str() );

  return true;
}
