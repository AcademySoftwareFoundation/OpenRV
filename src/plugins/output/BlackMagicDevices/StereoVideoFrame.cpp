/* -LICENSE-START-
** Copyright (c) 2024 Blackmagic Design
**  
** Permission is hereby granted, free of charge, to any person or organization 
** obtaining a copy of the software and accompanying documentation (the 
** "Software") to use, reproduce, display, distribute, sub-license, execute, 
** and transmit the Software, and to prepare derivative works of the Software, 
** and to permit third-parties to whom the Software is furnished to do so, in 
** accordance with:
** 
** (1) if the Software is obtained from Blackmagic Design, the End User License 
** Agreement for the Software Development Kit ("EULA") available at 
** https://www.blackmagicdesign.com/EULA/DeckLinkSDK; or
** 
** (2) if the Software is obtained from any third party, such licensing terms 
** as notified by that third party,
** 
** and all subject to the following:
** 
** (3) the copyright notices in the Software and this entire statement, 
** including the above license grant, this restriction and the following 
** disclaimer, must be included in all copies of the Software, in whole or in 
** part, and all derivative works of the Software, unless such copies or 
** derivative works are solely in the form of machine-executable object code 
** generated by a source language processor.
** 
** (4) THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
** OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
** FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT 
** SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE 
** FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE, 
** ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
** DEALINGS IN THE SOFTWARE.
** 
** A copy of the Software is available free of charge at 
** https://www.blackmagicdesign.com/desktopvideo_sdk under the EULA.
** 
** -LICENSE-END-
*/
//
//  StereoVideoFrame.cpp
//  Signal Generator
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
