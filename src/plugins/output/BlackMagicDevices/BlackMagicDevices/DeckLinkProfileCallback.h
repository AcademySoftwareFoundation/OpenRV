//
// Copyright (C) 2024  Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <DeckLinkAPI.h>

#include <atomic>
#include <condition_variable>
#include <mutex>

namespace BlackMagicDevices
{

  //
  // The DeckLinkProfileCallback is a callback class which is called when the
  // profile is about to change and when a new profile has been activated.
  //
  class DeckLinkProfileCallback : public IDeckLinkProfileCallback
  {
   public:
    DeckLinkProfileCallback( IDeckLinkProfile* requestedProfile );
    virtual ~DeckLinkProfileCallback();

    bool WaitForProfileActivation( void );

    // IDeckLinkInputCallback interface

#ifdef PLATFORM_WINDOWS
#define dlbool_t BOOL
#else
#define dlbool_t bool
#endif

    HRESULT STDMETHODCALLTYPE
    ProfileChanging( IDeckLinkProfile* profileToBeActivated,
                     dlbool_t streamsWillBeForcedToStop ) override;
    HRESULT STDMETHODCALLTYPE
    ProfileActivated( IDeckLinkProfile* activatedProfile ) override;

    // IUnknown interface

    HRESULT STDMETHODCALLTYPE QueryInterface( REFIID iid,
                                              LPVOID* ppv ) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;

   private:
    std::condition_variable m_profileActivatedCondition;
    std::mutex m_profileActivatedMutex;
    IDeckLinkProfile* m_requestedProfile;
    bool m_requestedProfileActivated;

    std::atomic<ULONG> m_refCount;
  };

}  // namespace BlackMagicDevices
