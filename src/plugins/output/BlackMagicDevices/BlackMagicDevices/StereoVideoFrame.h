//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <DeckLinkAPI.h>

#ifdef PLATFORM_WINDOWS
typedef __int32 int32_t;
#endif

#include <boost/thread.hpp>

/*
 * An example class which may be used to output a frame or pair of frames to
 * a 3D capable output.
 *
 * This class implements the IDeckLinkVideoFrame interface which can
 * be used to operate on the left frame.
 *
 * Access to the right frame through the IDeckLinkVideoFrame3DExtensions
 * interface:
 *
 * 	IDeckLinkVideoFrame *rightEyeFrame;
 * 	hr = threeDimensionalFrame->GetFrameForRightEye(&rightEyeFrame);
 *
 * After which IDeckLinkVideoFrame operations are performed directly
 * on the rightEyeFrame object.
 */

class StereoVideoFrame : public IDeckLinkVideoFrame,
                         public IDeckLinkVideoFrame3DExtensions
{
 public:
  typedef boost::mutex::scoped_lock ScopedLock;
  typedef boost::mutex Mutex;
  typedef boost::condition_variable Condition;

  // IUnknown methods
  virtual HRESULT STDMETHODCALLTYPE QueryInterface( REFIID iid, LPVOID* ppv );
  virtual ULONG STDMETHODCALLTYPE AddRef( void );
  virtual ULONG STDMETHODCALLTYPE Release( void );

  // IDeckLinkVideoFrame methods
  virtual long STDMETHODCALLTYPE GetWidth( void );
  virtual long STDMETHODCALLTYPE GetHeight( void );
  virtual long STDMETHODCALLTYPE GetRowBytes( void );
  virtual BMDPixelFormat STDMETHODCALLTYPE GetPixelFormat( void );
  virtual BMDFrameFlags STDMETHODCALLTYPE GetFlags( void );
  virtual HRESULT STDMETHODCALLTYPE GetBytes( /* out */ void** buffer );

  virtual HRESULT STDMETHODCALLTYPE
  GetTimecode( /* in */ BMDTimecodeFormat format,
               /* out */ IDeckLinkTimecode** timecode );
  virtual HRESULT STDMETHODCALLTYPE
  GetAncillaryData( /* out */ IDeckLinkVideoFrameAncillary** ancillary );

  // IDeckLinkVideoFrame3DExtensions methods
  virtual BMDVideo3DPackingFormat STDMETHODCALLTYPE Get3DPackingFormat( void );
  virtual HRESULT STDMETHODCALLTYPE
  GetFrameForRightEye( /* out */ IDeckLinkVideoFrame** rightEyeFrame );

  StereoVideoFrame( IDeckLinkMutableVideoFrame* left,
                    IDeckLinkMutableVideoFrame* right = 0 );
  virtual ~StereoVideoFrame();

 protected:
  IDeckLinkMutableVideoFrame* m_frameLeft;
  IDeckLinkMutableVideoFrame* m_frameRight;
  int32_t m_refCount;
  Mutex m_refMutex;
};
