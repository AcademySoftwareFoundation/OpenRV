//
// SPDX-License-Identifier: Apache-2.0
//

#include <BlackMagicDevices/StereoVideoFrame.h>
#include <stdexcept>
#include <string.h>

#define CompareREFIID( iid1, iid2 ) \
  ( memcmp( &iid1, &iid2, sizeof( REFIID ) ) == 0 )

StereoVideoFrame::StereoVideoFrame( IDeckLinkMutableVideoFrame* left,
                                    IDeckLinkMutableVideoFrame* right )
    : m_frameLeft( left ), m_frameRight( right ), m_refCount( 1 )
{
  if( !m_frameLeft )
  {
    throw std::invalid_argument( "at minimum a left frame must be defined" );
  }
  m_frameLeft->AddRef();
  if( m_frameRight )
  {
    m_frameRight->AddRef();
  }
}

StereoVideoFrame::~StereoVideoFrame()
{
  // do not release the left right frames. the frames will be freed later by
  // DeckLinkVideoDevice
}

HRESULT StereoVideoFrame::QueryInterface( REFIID iid, LPVOID* ppv )
{
#ifdef PLATFORM_DARWIN
  CFUUIDBytes iunknown = CFUUIDGetUUIDBytes( IUnknownUUID );
#else
  REFIID iunknown = IID_IUnknown;
#endif

  if( CompareREFIID( iid, iunknown ) )
    *ppv = static_cast<IDeckLinkVideoFrame*>( this );
  else if( CompareREFIID( iid, IID_IDeckLinkVideoFrame ) )
    *ppv = static_cast<IDeckLinkVideoFrame*>( this );
  else if( CompareREFIID( iid, IID_IDeckLinkVideoFrame3DExtensions ) )
    *ppv = static_cast<IDeckLinkVideoFrame3DExtensions*>( this );
  else
  {
    *ppv = NULL;
    return E_NOINTERFACE;
  }

  AddRef();
  return S_OK;
}

ULONG StereoVideoFrame::AddRef( void )
{
#ifdef PLATFORM_WINDOWS
  return _InterlockedIncrement( (volatile long*)&m_refCount );
#else
  ScopedLock lock( m_refMutex );
  m_refCount++;
  return m_refCount;
#endif
}

ULONG StereoVideoFrame::Release( void )
{
#ifdef PLATFORM_WINDOWS
  ULONG newRefValue = _InterlockedDecrement( (volatile long*)&m_refCount );

  if( !newRefValue ) delete this;
  return newRefValue;
#else
  ScopedLock lock( m_refMutex );
  m_refCount--;
  return m_refCount;
#endif
}

long StereoVideoFrame::GetWidth( void )
{
  return m_frameLeft->GetWidth();
}

long StereoVideoFrame::GetHeight( void )
{
  return m_frameLeft->GetHeight();
}

long StereoVideoFrame::GetRowBytes( void )
{
  return m_frameLeft->GetRowBytes();
}

BMDPixelFormat StereoVideoFrame::GetPixelFormat( void )
{
  return m_frameLeft->GetPixelFormat();
}

BMDFrameFlags StereoVideoFrame::GetFlags( void )
{
  return m_frameLeft->GetFlags();
}

HRESULT StereoVideoFrame::GetBytes( /* out */ void** buffer )
{
  return m_frameLeft->GetBytes( buffer );
}

HRESULT StereoVideoFrame::GetTimecode( /* in */ BMDTimecodeFormat format,
                                       /* out */ IDeckLinkTimecode** timecode )
{
  return m_frameLeft->GetTimecode( format, timecode );
}

HRESULT StereoVideoFrame::GetAncillaryData(
    /* out */ IDeckLinkVideoFrameAncillary** ancillary )
{
  return m_frameLeft->GetAncillaryData( ancillary );
}

BMDVideo3DPackingFormat StereoVideoFrame::Get3DPackingFormat( void )
{
  return bmdVideo3DPackingLeftOnly;
}

HRESULT StereoVideoFrame::GetFrameForRightEye(
    /* out */ IDeckLinkVideoFrame** rightEyeFrame )
{
  if( m_frameRight )
    return m_frameRight->QueryInterface( IID_IDeckLinkVideoFrame,
                                         (void**)rightEyeFrame );
  else
    return S_FALSE;
}
