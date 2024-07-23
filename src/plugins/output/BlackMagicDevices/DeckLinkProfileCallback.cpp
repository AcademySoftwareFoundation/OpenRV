//
// Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#include <BlackMagicDevices/DeckLinkProfileCallback.h>

#include <chrono>

namespace BlackMagicDevices
{

  static const std::chrono::seconds kProfileActivationTimeout{ 5 };

  //
  // Constructor
  //

  DeckLinkProfileCallback::DeckLinkProfileCallback(
      IDeckLinkProfile* requestedProfile )
      : m_refCount( 1 ),
        m_requestedProfile( requestedProfile ),
        m_requestedProfileActivated( false )
  {
    m_requestedProfile->AddRef();
  }

  //
  // Destructor
  //

  DeckLinkProfileCallback::~DeckLinkProfileCallback()
  {
    m_requestedProfile->Release();
  }

  //
  // WaitForProfileActivation()
  //
  bool DeckLinkProfileCallback::WaitForProfileActivation( void )
  {
    dlbool_t isActiveProfile = false;

    // Check whether requested profile is already the active profile, then we
    // can return without waiting
    if( ( m_requestedProfile->IsActive( &isActiveProfile ) == S_OK ) &&
        isActiveProfile )
    {
      return true;
    }

    std::unique_lock<std::mutex> lock( m_profileActivatedMutex );
    if( m_requestedProfileActivated )
    {
      return true;
    }

    // Wait until the ProfileActivated callback occurs
    return m_profileActivatedCondition.wait_for(
        lock, kProfileActivationTimeout,
        [&] { return m_requestedProfileActivated; } );
  }

  // IDeckLinkInputCallback interface

  //
  // IDeckLinkInputCallback::ProfileChanging
  //
  HRESULT STDMETHODCALLTYPE DeckLinkProfileCallback::ProfileChanging(
      IDeckLinkProfile* /*profileToBeActivated*/,
      dlbool_t /*streamsWillBeForcedToStop*/ )
  {
    return S_OK;
  }

  //
  // IDeckLinkInputCallback::ProfileActivated
  //
  HRESULT STDMETHODCALLTYPE DeckLinkProfileCallback::ProfileActivated(
      IDeckLinkProfile* activatedProfile )
  {
    {
      std::lock_guard<std::mutex> lock( m_profileActivatedMutex );
      m_requestedProfileActivated = true;
    }
    m_profileActivatedCondition.notify_one();

    return S_OK;
  }

  //
  // IUnknown interface
  //

  HRESULT STDMETHODCALLTYPE
  DeckLinkProfileCallback::QueryInterface( REFIID iid, LPVOID* ppv )
  {
    *ppv = nullptr;
    return E_NOINTERFACE;
  }

  ULONG STDMETHODCALLTYPE DeckLinkProfileCallback::AddRef()
  {
    return ++m_refCount;
  }

  ULONG STDMETHODCALLTYPE DeckLinkProfileCallback::Release()
  {
    ULONG refCount = --m_refCount;
    if( refCount == 0 ) delete this;

    return refCount;
  }

}  // namespace BlackMagicDevices